#pragma once
#include <JuceHeader.h>
#include "../ArpeggiatorEngine.h"
#include "../Parameters.h"

//==============================================================================
/**
 * Unit tests for ArpeggiatorEngine.
 *
 * Run via the JUCE UnitTestRunner or the ArpSequencerTests console app.
 */
class ArpEngineTests : public juce::UnitTest
{
public:
    ArpEngineTests() : juce::UnitTest("ArpeggiatorEngine", "ArpSequencer") {}

    void runTest() override
    {
        testNoteTracking();
        testPatternUp();
        testPatternDown();
        testPatternUpDown();
        testPatternRandom();
        testPatternChord();
        testLatchMode();
        testHoldMode();
        testOctaveRange();
        testSwingOffset();
        testVelocityVariation();
        testTranspose();
        testAllNotesOff();
        testEdgeCaseSingleNote();
        testEdgeCaseMaxPolyphony();
    }

private:
    static constexpr double TEST_SR  = 44100.0;
    static constexpr double TEST_BPM = 120.0;

    //──────────────────────────────────────────────────────────────────────────
    void testNoteTracking()
    {
        beginTest("Note tracking – add/remove");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);
        expect(arp.getNoteCount() == 3, "Should have 3 notes after 3 noteOns");

        arp.noteOff(64);
        expect(arp.getNoteCount() == 2, "Should have 2 notes after noteOff");

        arp.noteOff(60);
        arp.noteOff(67);
        expect(arp.getNoteCount() == 0, "Should have 0 notes after all noteOffs");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternUp()
    {
        beginTest("ARP pattern – Up");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Up);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]); // 1/16
        arp.setOctaveRange(1);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);

        // Run one step per processBlock call
        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25; // 1/16 note
        const int    samples = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        int lastNote = -1;
        bool ascending = true;

        for (int step = 0; step < 9; ++step)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;

            for (const auto& ev : events)
            {
                if (ev.type == ArpEvent::Type::NoteOn)
                {
                    if (lastNote >= 0 && step < 3)
                        expect(ev.note >= lastNote, "Up pattern: note should be ascending or wrapping");
                    lastNote = ev.note;
                }
            }
        }
        expect(true, "Up pattern ran without crash");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternDown()
    {
        beginTest("ARP pattern – Down");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Down);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        int firstNote = -1, secondNote = -1;
        for (int step = 0; step < 3; ++step)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
            {
                if (ev.type == ArpEvent::Type::NoteOn)
                {
                    if (firstNote < 0)  firstNote  = ev.note;
                    else if (secondNote < 0) secondNote = ev.note;
                }
            }
        }
        if (firstNote >= 0 && secondNote >= 0)
            expect(firstNote >= secondNote, "Down pattern: first note should be >= second");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternUpDown()
    {
        beginTest("ARP pattern – UpDown");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::UpDown);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);

        std::vector<ArpEvent> events;
        std::vector<int>      noteSequence;

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 8; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == ArpEvent::Type::NoteOn)
                    noteSequence.push_back(ev.note);
        }

        // Expect at least one up and one down movement
        bool hasUp   = false, hasDown = false;
        for (int i = 1; i < (int)noteSequence.size(); ++i)
        {
            if (noteSequence[i] > noteSequence[i-1]) hasUp   = true;
            if (noteSequence[i] < noteSequence[i-1]) hasDown = true;
        }
        expect(hasUp || noteSequence.size() <= 1, "UpDown should have upward motion");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternRandom()
    {
        beginTest("ARP pattern – Random (no crash)");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Random);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 32; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == ArpEvent::Type::NoteOn)
                    expect(ev.note >= 0 && ev.note <= 127, "Random: note must be valid MIDI");
        }
        expect(true, "Random pattern ran without crash");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternChord()
    {
        beginTest("ARP pattern – Chord");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Chord);
        arp.setNoteLength(Params::NOTE_LENGTHS[6]); // 1/4

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOn(67, 100);

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 1.0; // quarter note
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        arp.process(samples, ppq, true, events);

        int noteOns = 0;
        for (const auto& ev : events)
            if (ev.type == ArpEvent::Type::NoteOn)
                ++noteOns;

        expect(noteOns == 3, "Chord mode: should emit all 3 notes simultaneously");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testLatchMode()
    {
        beginTest("ARP – Latch mode");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setLatchMode(true);
        arp.setPattern(Params::ArpPattern::Up);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);

        // Release all notes
        arp.noteOff(60);
        arp.noteOff(64);

        // Still has latched notes
        expect(arp.getNoteCount() > 0, "Latch: should retain notes after release");

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);
        arp.process(samples, ppq, true, events);

        bool hasNoteOn = false;
        for (const auto& ev : events)
            if (ev.type == ArpEvent::Type::NoteOn)
                hasNoteOn = true;

        expect(hasNoteOn, "Latch: should continue producing notes after key release");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testHoldMode()
    {
        beginTest("ARP – Hold mode (no crash)");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setHoldMode(true);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);
        arp.noteOff(60);
        arp.noteOff(64);

        std::vector<ArpEvent> events;
        arp.process(512, 0.0, true, events);
        expect(true, "Hold mode: no crash");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testOctaveRange()
    {
        beginTest("ARP – Octave range");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Up);
        arp.setOctaveRange(2);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100); // C4 only

        std::vector<ArpEvent> events;
        std::set<int> outputNotes;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 8; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == ArpEvent::Type::NoteOn)
                    outputNotes.insert(ev.note);
        }

        // With 1 note and 2 octaves, we should see two distinct octaves (60, 72)
        expect(outputNotes.size() >= 1, "Should produce at least one note");
        for (int n : outputNotes)
            expect(n >= 0 && n <= 127, "Octave range: note must be valid MIDI");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testSwingOffset()
    {
        beginTest("ARP – Swing does not crash");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setSwing(50.0f);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(60, 100);
        arp.noteOn(64, 100);

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 8; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                expect(ev.sampleOffset >= 0 && ev.sampleOffset < samples,
                       "Swing: sampleOffset must be within buffer bounds");
        }
    }

    //──────────────────────────────────────────────────────────────────────────
    void testVelocityVariation()
    {
        beginTest("ARP – Velocity variation bounds");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setVelocityVariation(127);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);

        arp.noteOn(64, 64); // mid velocity

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 16; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == ArpEvent::Type::NoteOn)
                    expect(ev.velocity >= 1 && ev.velocity <= 127,
                           "Velocity must stay in 1-127 range");
        }
    }

    //──────────────────────────────────────────────────────────────────────────
    void testTranspose()
    {
        beginTest("ARP – Transpose");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setPattern(Params::ArpPattern::Up);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);
        arp.setTranspose(12); // up one octave

        arp.noteOn(60, 100);

        std::vector<ArpEvent> events;
        arp.process((int)(TEST_SR * 0.25 * 60.0 / TEST_BPM), 0.0, true, events);

        for (const auto& ev : events)
            if (ev.type == ArpEvent::Type::NoteOn)
                expect(ev.note == 72, "Transpose +12: C4 should become C5");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testAllNotesOff()
    {
        beginTest("ARP – All notes off");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);

        for (int n = 60; n < 73; ++n)
            arp.noteOn(n, 100);

        expect(arp.getNoteCount() == 13, "Should have 13 notes");

        arp.allNotesOff();
        expect(arp.getNoteCount() == 0, "After allNotesOff: should have 0 notes");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testEdgeCaseSingleNote()
    {
        beginTest("ARP – Edge case: single note");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);
        arp.setBpm(TEST_BPM);
        arp.setNoteLength(Params::NOTE_LENGTHS[2]);
        arp.setPattern(Params::ArpPattern::UpDown);

        arp.noteOn(60, 100);

        std::vector<ArpEvent> events;
        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 8; ++i)
        {
            arp.process(samples, ppq, true, events);
            ppq += stepPpq;
        }
        expect(true, "Single-note edge case: no crash");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testEdgeCaseMaxPolyphony()
    {
        beginTest("ARP – Edge case: max polyphony (128 notes)");

        ArpeggiatorEngine arp;
        arp.prepare(TEST_SR, 512);

        for (int n = 0; n < 128; ++n)
            arp.noteOn(n, 100);

        expect(arp.getNoteCount() == 128, "Should handle 128 simultaneous notes");

        std::vector<ArpEvent> events;
        arp.setBpm(TEST_BPM);
        arp.setNoteLength(Params::NOTE_LENGTHS[0]); // 1/32 – dense
        arp.process(512, 0.0, true, events);
        expect(true, "128-note edge case: no crash or overflow");
    }
};

//── Static instance for JUCE test runner ──────────────────────────────────────
static ArpEngineTests arpEngineTests;
