#include "StepSequencer.h"
#include <algorithm>
#include <cmath>

//==============================================================================
StepSequencer::StepSequencer()
{
    // Default chain order: 0,1,2,3
    for (int i = 0; i < Params::NUM_PATTERNS; ++i)
        chainOrder[i] = i;
    reset();
}

void StepSequencer::prepare (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void StepSequencer::reset()
{
    currentStep     = 0;
    currentChainIdx = 0;
    lastStepPpq     = -1.0;
    noteIsOn        = false;
    lastNoteOut     = -1;
    noteOffPpq      = -1.0;
    ratchetSubStep  = 0;
    ratchetTotal    = 1;
}

//──────────────────────────────────────────────────────────────────────────────
int StepSequencer::currentPattern() const noexcept
{
    if (!chainEnabled)
        return activePattern;
    return chainOrder[currentChainIdx % chainLength];
}

bool StepSequencer::shouldFire (float probability) const
{
    if (probability >= 100.0f) return true;
    if (probability <= 0.0f)   return false;
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    return dist(rng) < probability;
}

//──────────────────────────────────────────────────────────────────────────────
void StepSequencer::checkNoteOff (double ppqPosition,
                                   double ppqEnd,
                                   double samplesPerBeat,
                                   int    numSamples,
                                   std::vector<SeqEvent>& outEvents)
{
    if (!noteIsOn || lastNoteOut < 0 || noteOffPpq < 0.0) return;

    if (noteOffPpq <= ppqEnd)
    {
        int offset = juce::jlimit(0, numSamples - 1,
            static_cast<int>((noteOffPpq - ppqPosition) * samplesPerBeat));

        SeqEvent ev;
        ev.type         = SeqEvent::Type::NoteOff;
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

//──────────────────────────────────────────────────────────────────────────────
void StepSequencer::emitNoteOn (const SeqStep& step,
                                 int sampleOffset,
                                 double stepStartPpq,
                                 std::vector<SeqEvent>& outEvents)
{
    // Kill previous note
    if (noteIsOn && lastNoteOut >= 0)
    {
        SeqEvent off;
        off.type         = SeqEvent::Type::NoteOff;
        off.note         = lastNoteOut;
        off.velocity     = 0;
        off.channel      = outputChannel;
        off.sampleOffset = sampleOffset;
        outEvents.push_back(off);
        noteIsOn = false;
    }

    if (!shouldFire(step.probability)) return;

    SeqEvent on;
    on.type         = SeqEvent::Type::NoteOn;
    on.note         = juce::jlimit(0, 127, step.note);
    on.velocity     = juce::jlimit(0, 127, step.velocity);
    on.channel      = outputChannel;
    on.sampleOffset = sampleOffset;
    outEvents.push_back(on);

    noteIsOn    = true;
    lastNoteOut = on.note;

    // Gate: fraction of step length for ratchet sub-steps
    float ratchetStepLen = static_cast<float>(stepLenBeats) / static_cast<float>(ratchetTotal);
    double gateLen = ratchetStepLen * (step.gate / 100.0f) * 0.98;
    noteOffPpq = stepStartPpq + gateLen;
}

//──────────────────────────────────────────────────────────────────────────────
void StepSequencer::process (int numSamples,
                               double ppqPosition,
                               bool   isPlaying,
                               std::vector<SeqEvent>& outEvents)
{
    outEvents.clear();

    const double samplesPerBeat = sampleRate * 60.0 / currentBpm;
    const double ppqEnd = ppqPosition + numSamples / samplesPerBeat;
    const double stepLenPpq = static_cast<double>(stepLenBeats);

    // Always check pending NoteOff
    checkNoteOff(ppqPosition, ppqEnd, samplesPerBeat, numSamples, outEvents);

    if (!isPlaying) return;

    // Initialise lastStepPpq on first run or after stop/start
    if (lastStepPpq < 0.0)
    {
        lastStepPpq  = std::floor(ppqPosition / stepLenPpq) * stepLenPpq;
        currentStep  = static_cast<int>(std::floor(ppqPosition / stepLenPpq))
                       % numSteps;
    }

    // Iterate through all step boundaries within this buffer
    double nextStepPpq = lastStepPpq + stepLenPpq;

    while (nextStepPpq <= ppqEnd)
    {
        int sampleOffset = juce::jlimit(0, numSamples - 1,
            static_cast<int>((nextStepPpq - ppqPosition) * samplesPerBeat));

        const int pat  = currentPattern();
        const auto& st = patterns[pat][currentStep];

        if (st.active)
        {
            ratchetTotal   = juce::jlimit(1, 4, st.ratchet);
            ratchetSubStep = 0;

            // Sub-steps for ratchet
            for (int r = 0; r < ratchetTotal; ++r)
            {
                double ratchetStepLen = stepLenPpq / static_cast<double>(ratchetTotal);
                double ratchetStart   = nextStepPpq + r * ratchetStepLen;
                int    ratchetOffset  = juce::jlimit(0, numSamples - 1,
                    static_cast<int>((ratchetStart - ppqPosition) * samplesPerBeat));

                if (ratchetOffset >= numSamples) break; // outside buffer

                // Check pending note-off before each ratchet hit
                if (r > 0)
                    checkNoteOff(ppqPosition, ratchetStart, samplesPerBeat,
                                 numSamples, outEvents);

                emitNoteOn(st, ratchetOffset, ratchetStart, outEvents);
            }
        }

        // Advance step and potentially chain
        ++currentStep;
        if (currentStep >= numSteps)
        {
            currentStep = 0;
            if (chainEnabled)
            {
                ++currentChainIdx;
                if (currentChainIdx >= chainLength)
                    currentChainIdx = 0;
            }
        }

        lastStepPpq = nextStepPpq;
        nextStepPpq += stepLenPpq;
    }

    // Sort events by sample offset
    std::sort(outEvents.begin(), outEvents.end(),
        [](const SeqEvent& a, const SeqEvent& b)
        { return a.sampleOffset < b.sampleOffset; });
}
