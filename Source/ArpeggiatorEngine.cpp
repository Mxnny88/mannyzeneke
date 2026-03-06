#include "ArpeggiatorEngine.h"
#include <algorithm>
#include <cmath>

//==============================================================================
ArpeggiatorEngine::ArpeggiatorEngine()
{
    reset();
}

void ArpeggiatorEngine::prepare (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void ArpeggiatorEngine::reset()
{
    heldNotes.clear();
    latchedNotes.clear();
    playedOrder.clear();
    currentStep         = 0;
    currentOctaveOffset = 0;
    directionUp         = true;
    stepParity          = 0;
    lastStepPpq         = -1.0;
    noteIsOn            = false;
    lastNoteOut         = -1;
    noteOffPpq          = -1.0;
    ratchetCount        = 1;
    ratchetSubStep      = 0;
}

//──────────────────────────────────────────────────────────────────────────────
void ArpeggiatorEngine::noteOn (int note, int velocity)
{
    // Ignore out-of-range notes
    if (note < 0 || note > 127) return;

    // Remove duplicates
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [note](const ArpNote& n){ return n.note == note; }), heldNotes.end());
    playedOrder.erase(std::remove_if(playedOrder.begin(), playedOrder.end(),
        [note](const ArpNote& n){ return n.note == note; }), playedOrder.end());

    ArpNote an { note, velocity };
    heldNotes.push_back(an);
    playedOrder.push_back(an);

    // Keep heldNotes sorted ascending for Up/Down patterns
    std::sort(heldNotes.begin(), heldNotes.end());

    // Latch: merge held into latched when the first note arrives
    if (latchMode)
    {
        bool found = false;
        for (auto& ln : latchedNotes)
        {
            if (ln.note == note) { ln.velocity = velocity; found = true; break; }
        }
        if (!found)
            latchedNotes.push_back(an);
    }
}

void ArpeggiatorEngine::noteOff (int note)
{
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [note](const ArpNote& n){ return n.note == note; }), heldNotes.end());
    playedOrder.erase(std::remove_if(playedOrder.begin(), playedOrder.end(),
        [note](const ArpNote& n){ return n.note == note; }), playedOrder.end());

    // Latch: keep latched notes alive – remove if same note pressed again
    if (latchMode && !heldNotes.empty())
    {
        // When a new set of notes is played over latched, swap
        latchedNotes = heldNotes;
    }
}

void ArpeggiatorEngine::allNotesOff()
{
    heldNotes.clear();
    playedOrder.clear();
    if (!latchMode)
        latchedNotes.clear();
    currentStep = 0;
}

//──────────────────────────────────────────────────────────────────────────────
int ArpeggiatorEngine::getNoteCount() const noexcept
{
    return static_cast<int>(buildNoteList().size());
}

//──────────────────────────────────────────────────────────────────────────────
std::vector<ArpNote> ArpeggiatorEngine::buildNoteList() const
{
    // Priority: held > latched > nothing
    const std::vector<ArpNote>* base = &heldNotes;
    if (base->empty() && !latchedNotes.empty())
        base = &latchedNotes;
    if (base->empty())
        return {};

    // For AsPlayed pattern use playedOrder if non-empty
    if (pattern == Params::ArpPattern::AsPlayed && !playedOrder.empty())
        return playedOrder;

    std::vector<ArpNote> sorted = *base;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

//──────────────────────────────────────────────────────────────────────────────
int ArpeggiatorEngine::nextNoteIndex (const std::vector<ArpNote>& notes)
{
    const int n = static_cast<int>(notes.size());
    if (n == 0) return 0;

    switch (pattern)
    {
        case Params::ArpPattern::Up:
        {
            int idx = currentStep % n;
            if (currentStep % n == 0 && currentStep > 0)
            {
                ++currentOctaveOffset;
                if (currentOctaveOffset >= octaveRange)
                    currentOctaveOffset = 0;
            }
            return idx;
        }
        case Params::ArpPattern::Down:
        {
            int idx = (n - 1) - (currentStep % n);
            if (currentStep % n == 0 && currentStep > 0)
            {
                ++currentOctaveOffset;
                if (currentOctaveOffset >= octaveRange)
                    currentOctaveOffset = 0;
            }
            return idx;
        }
        case Params::ArpPattern::UpDown:
        {
            // Total steps: n * 2 - 2  (no repeat at extremes)
            int total = (n > 1) ? (n * 2 - 2) : 1;
            int pos   = currentStep % total;
            if (pos < n)
                return pos;
            else
                return total - pos;
        }
        case Params::ArpPattern::DownUp:
        {
            int total = (n > 1) ? (n * 2 - 2) : 1;
            int pos   = currentStep % total;
            int downIdx = (n - 1) - (pos < n ? pos : total - pos);
            return juce::jlimit(0, n-1, downIdx);
        }
        case Params::ArpPattern::Random:
        {
            std::uniform_int_distribution<int> dist(0, n - 1);
            return dist(rng);
        }
        case Params::ArpPattern::Chord:
            return -1; // Chord plays all notes simultaneously
        case Params::ArpPattern::AsPlayed:
            return currentStep % n;
        default:
            return currentStep % n;
    }
}

//──────────────────────────────────────────────────────────────────────────────
int ArpeggiatorEngine::applyVelocityVariation (int baseVel) const
{
    if (velocityVariation == 0) return baseVel;
    std::uniform_int_distribution<int> dist(-velocityVariation, velocityVariation);
    return juce::jlimit(1, 127, baseVel + dist(rng));
}

//──────────────────────────────────────────────────────────────────────────────
int ArpeggiatorEngine::swingOffset (bool isOddStep, double samplesPerBeat) const noexcept
{
    if (swingPct <= 0.0f || !isOddStep) return 0;
    // Swing amount = delay odd steps by up to 1/3 of a beat
    float swingFraction = (swingPct / 100.0f) * 0.333f;
    return static_cast<int>(swingFraction * samplesPerBeat * noteLengthBeats * 2.0);
}

//──────────────────────────────────────────────────────────────────────────────
void ArpeggiatorEngine::process (int numSamples,
                                  double ppqPosition,
                                  bool   isPlaying,
                                  std::vector<ArpEvent>& outEvents)
{
    outEvents.clear();

    const auto notes = buildNoteList();
    const bool hasNotes = !notes.empty();

    // --- Send pending NoteOff if it falls in this buffer ---
    if (noteIsOn && lastNoteOut >= 0 && noteOffPpq >= 0.0)
    {
        const double samplesPerBeat = sampleRate * 60.0 / currentBpm;
        const double ppqEnd = ppqPosition + numSamples / samplesPerBeat;

        if (noteOffPpq <= ppqEnd)
        {
            int offset = juce::jlimit(0, numSamples-1,
                static_cast<int>((noteOffPpq - ppqPosition) * samplesPerBeat));
            ArpEvent ev;
            ev.type         = ArpEvent::Type::NoteOff;
            ev.note         = lastNoteOut;
            ev.velocity     = 0;
            ev.channel      = outputChannel;
            ev.sampleOffset = offset;
            outEvents.push_back(ev);
            noteIsOn    = false;
            lastNoteOut = -1;
            noteOffPpq  = -1.0;
        }
    }

    if (!isPlaying || !hasNotes) return;

    const double samplesPerBeat = sampleRate * 60.0 / currentBpm;
    const double stepLenPpq     = static_cast<double>(noteLengthBeats);

    // Quantise: find next step boundary >= ppqPosition
    if (lastStepPpq < 0.0)
        lastStepPpq = std::floor(ppqPosition / stepLenPpq) * stepLenPpq;

    double nextStepPpq = lastStepPpq + stepLenPpq;
    const double ppqEnd = ppqPosition + numSamples / samplesPerBeat;

    // Possibly fire multiple steps if buffer is large relative to step length
    while (nextStepPpq <= ppqEnd)
    {
        int sampleOffset = juce::jlimit(0, numSamples - 1,
            static_cast<int>((nextStepPpq - ppqPosition) * samplesPerBeat));

        // Swing: delay odd-parity steps
        sampleOffset += swingOffset((stepParity & 1) != 0, samplesPerBeat);
        sampleOffset  = juce::jlimit(0, numSamples - 1, sampleOffset);

        const int noteIdx = nextNoteIndex(notes);

        // Helper lambda to emit a single note-on + schedule note-off
        auto emitNote = [&](int midiNote, int vel, int smplOffset)
        {
            midiNote = juce::jlimit(0, 127, midiNote + transpose);
            if (midiNote < 0 || midiNote > 127) return;

            // Kill previous note if still on
            if (noteIsOn && lastNoteOut >= 0)
            {
                ArpEvent off;
                off.type         = ArpEvent::Type::NoteOff;
                off.note         = lastNoteOut;
                off.velocity     = 0;
                off.channel      = outputChannel;
                off.sampleOffset = smplOffset;
                outEvents.push_back(off);
            }

            ArpEvent on;
            on.type         = ArpEvent::Type::NoteOn;
            on.note         = midiNote;
            on.velocity     = applyVelocityVariation(vel);
            on.channel      = outputChannel;
            on.sampleOffset = smplOffset;
            outEvents.push_back(on);

            noteIsOn    = true;
            lastNoteOut = midiNote;
            // Schedule note-off at gate end
            double gateLen = stepLenPpq * (gatePct / 100.0f) * 0.98; // tiny headroom
            noteOffPpq  = nextStepPpq + gateLen;
        };

        if (pattern == Params::ArpPattern::Chord)
        {
            // All notes simultaneously
            for (const auto& an : notes)
            {
                int midiNote = juce::jlimit(0, 127,
                    an.note + currentOctaveOffset * 12 + transpose);
                ArpEvent on;
                on.type         = ArpEvent::Type::NoteOn;
                on.note         = midiNote;
                on.velocity     = applyVelocityVariation(an.velocity);
                on.channel      = outputChannel;
                on.sampleOffset = sampleOffset;
                outEvents.push_back(on);

                double gateLen = stepLenPpq * (gatePct / 100.0f) * 0.98;
                noteOffPpq = nextStepPpq + gateLen;
                lastNoteOut = midiNote;
                noteIsOn = true;
            }
        }
        else if (noteIdx >= 0 && noteIdx < static_cast<int>(notes.size()))
        {
            const auto& an = notes[noteIdx];
            int midiNote = juce::jlimit(0, 127, an.note + currentOctaveOffset * 12);
            emitNote(midiNote, an.velocity, sampleOffset);
        }

        lastStepPpq = nextStepPpq;
        nextStepPpq += stepLenPpq;
        ++currentStep;
        ++stepParity;
    }

    // Sort events by sample offset for clean MIDI output
    std::sort(outEvents.begin(), outEvents.end(),
        [](const ArpEvent& a, const ArpEvent& b)
        { return a.sampleOffset < b.sampleOffset; });
}
