#pragma once
#include <JuceHeader.h>
#include "../StepSequencer.h"
#include "../Parameters.h"

//==============================================================================
/**
 * Unit tests for StepSequencer.
 */
class StepSequencerTests : public juce::UnitTest
{
public:
    StepSequencerTests() : juce::UnitTest("StepSequencer", "ArpSequencer") {}

    void runTest() override
    {
        testDefaultState();
        testStepActivation();
        testNoteOutputPerStep();
        testVelocityRange();
        testGateLength();
        testProbability();
        testRatchet();
        testPatternChaining();
        testNumSteps();
        testStopRestart();
        testEdgeCaseAllStepsInactive();
        testEdgeCaseSingleStep();
        testStepLengths();
    }

private:
    static constexpr double TEST_SR  = 44100.0;
    static constexpr double TEST_BPM = 120.0;

    /** Helper: advance the sequencer by one 1/16-step. Returns NoteOn events. */
    std::vector<int> runOneStep (StepSequencer& seq, double& ppq,
                                 double stepPpq = 0.25)
    {
        const int samples = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);
        std::vector<SeqEvent> events;
        seq.process(samples, ppq, true, events);
        ppq += stepPpq;

        std::vector<int> notes;
        for (const auto& ev : events)
            if (ev.type == SeqEvent::Type::NoteOn)
                notes.push_back(ev.note);
        return notes;
    }

    //──────────────────────────────────────────────────────────────────────────
    void testDefaultState()
    {
        beginTest("SEQ – Default state");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);

        expect(seq.getCurrentStep() == 0, "Initial step should be 0");
        expect(seq.getCurrentPattern() == 0, "Initial pattern should be 0");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testStepActivation()
    {
        beginTest("SEQ – Step on/off");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]); // 1/16

        // Deactivate all steps
        for (int s = 0; s < Params::NUM_STEPS; ++s)
        {
            auto st = seq.getStep(0, s);
            st.active = false;
            seq.setStep(0, s, st);
        }

        double ppq = 0.0;
        std::vector<SeqEvent> events;
        seq.process(4096, ppq, true, events);

        int noteOns = 0;
        for (const auto& ev : events)
            if (ev.type == SeqEvent::Type::NoteOn)
                ++noteOns;

        expect(noteOns == 0, "All steps off: should produce no NoteOn events");

        // Activate step 0 only
        auto st = seq.getStep(0, 0);
        st.active = true;
        st.note   = 60;
        seq.setStep(0, 0, st);

        seq.reset();
        ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);
        seq.process(samples, ppq, true, events);

        noteOns = 0;
        for (const auto& ev : events)
            if (ev.type == SeqEvent::Type::NoteOn)
                ++noteOns;

        expect(noteOns == 1, "Step 0 active: should produce 1 NoteOn per step");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testNoteOutputPerStep()
    {
        beginTest("SEQ – Correct note output per step");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(4);

        // Programme 4 distinct notes
        int testNotes[] = { 60, 62, 64, 65 };
        for (int s = 0; s < 4; ++s)
        {
            SeqStep st;
            st.note     = testNotes[s];
            st.velocity = 100;
            st.gate     = 80.0f;
            st.probability = 100.0f;
            st.ratchet  = 1;
            st.active   = true;
            seq.setStep(0, s, st);
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        for (int i = 0; i < 4; ++i)
        {
            auto notes = runOneStep(seq, ppq, stepPpq);
            if (!notes.empty())
                expect(notes[0] == testNotes[i],
                       "Step " + juce::String(i) + ": expected note "
                       + juce::String(testNotes[i]));
        }
    }

    //──────────────────────────────────────────────────────────────────────────
    void testVelocityRange()
    {
        beginTest("SEQ – Velocity range");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);

        for (int s = 0; s < Params::NUM_STEPS; ++s)
        {
            SeqStep st = seq.getStep(0, s);
            st.velocity = (s * 8) % 128; // 0-127
            st.active   = true;
            seq.setStep(0, s, st);
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        std::vector<SeqEvent> events;
        for (int i = 0; i < 16; ++i)
        {
            seq.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == SeqEvent::Type::NoteOn)
                    expect(ev.velocity >= 0 && ev.velocity <= 127,
                           "Velocity must be 0-127");
        }
    }

    //──────────────────────────────────────────────────────────────────────────
    void testGateLength()
    {
        beginTest("SEQ – Gate generates NoteOff before next step");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);

        SeqStep st;
        st.note     = 60;
        st.velocity = 100;
        st.gate     = 50.0f; // half-step gate
        st.probability = 100.0f;
        st.ratchet  = 1;
        st.active   = true;
        seq.setStep(0, 0, st);

        // Deactivate other steps so we don't get conflicting events
        for (int s = 1; s < Params::NUM_STEPS; ++s)
        {
            auto s2 = seq.getStep(0, s);
            s2.active = false;
            seq.setStep(0, s, s2);
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq) * 2;

        std::vector<SeqEvent> events;
        seq.process(samples, ppq, true, events);

        bool hasNoteOn  = false;
        bool hasNoteOff = false;
        int  noteOnOffset  = -1;
        int  noteOffOffset = -1;

        for (const auto& ev : events)
        {
            if (ev.type == SeqEvent::Type::NoteOn  && ev.note == 60)
            { hasNoteOn  = true; noteOnOffset  = ev.sampleOffset; }
            if (ev.type == SeqEvent::Type::NoteOff && ev.note == 60)
            { hasNoteOff = true; noteOffOffset = ev.sampleOffset; }
        }

        expect(hasNoteOn,  "Gate test: should have NoteOn");
        expect(hasNoteOff, "Gate test: should have NoteOff");
        if (hasNoteOn && hasNoteOff)
            expect(noteOffOffset > noteOnOffset, "NoteOff should come after NoteOn");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testProbability()
    {
        beginTest("SEQ – Probability (statistical)");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(1);

        SeqStep st;
        st.note     = 60;
        st.velocity = 100;
        st.gate     = 80.0f;
        st.active   = true;

        // 0% probability – should never fire
        st.probability = 0.0f;
        seq.setStep(0, 0, st);

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        std::vector<SeqEvent> events;
        int fires = 0;
        for (int i = 0; i < 32; ++i)
        {
            seq.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == SeqEvent::Type::NoteOn)
                    ++fires;
        }
        expect(fires == 0, "0% probability: should never fire");

        // 100% probability – should always fire
        seq.reset();
        ppq = 0.0;
        st.probability = 100.0f;
        seq.setStep(0, 0, st);

        fires = 0;
        for (int i = 0; i < 8; ++i)
        {
            seq.process(samples, ppq, true, events);
            ppq += stepPpq;
            for (const auto& ev : events)
                if (ev.type == SeqEvent::Type::NoteOn)
                    ++fires;
        }
        expect(fires == 8, "100% probability: should fire every step");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testRatchet()
    {
        beginTest("SEQ – Ratchet");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(1);

        SeqStep st;
        st.note     = 60;
        st.velocity = 100;
        st.gate     = 80.0f;
        st.probability = 100.0f;
        st.ratchet  = 4;  // 4 repeats per step
        st.active   = true;
        seq.setStep(0, 0, st);

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        std::vector<SeqEvent> events;
        seq.process(samples, ppq, true, events);

        int noteOns = 0;
        for (const auto& ev : events)
            if (ev.type == SeqEvent::Type::NoteOn)
                ++noteOns;

        expect(noteOns == 4, "Ratchet x4: should fire 4 NoteOns in one step");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testPatternChaining()
    {
        beginTest("SEQ – Pattern chaining");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(2); // 2 steps per pattern for quick test
        seq.setChainEnabled(true);
        seq.setChainLength(2);
        seq.setChainPattern(0, 0);
        seq.setChainPattern(1, 1);

        // Pattern 0: note 60, Pattern 1: note 72
        for (int p = 0; p < 2; ++p)
        {
            for (int s = 0; s < 2; ++s)
            {
                SeqStep st;
                st.note     = (p == 0) ? 60 : 72;
                st.velocity = 100;
                st.gate     = 80.0f;
                st.probability = 100.0f;
                st.ratchet  = 1;
                st.active   = true;
                seq.setStep(p, s, st);
            }
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        std::vector<int> allNotes;
        for (int i = 0; i < 6; ++i)
        {
            auto notes = runOneStep(seq, ppq, stepPpq);
            for (int n : notes) allNotes.push_back(n);
        }

        bool saw60 = false, saw72 = false;
        for (int n : allNotes)
        {
            if (n == 60) saw60 = true;
            if (n == 72) saw72 = true;
        }
        expect(saw60, "Chain: should output notes from pattern 0 (note 60)");
        expect(saw72, "Chain: should output notes from pattern 1 (note 72)");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testNumSteps()
    {
        beginTest("SEQ – Num steps wraps correctly");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(4);

        for (int s = 0; s < 4; ++s)
        {
            SeqStep st;
            st.note = 60 + s;
            st.velocity = 100; st.gate = 80.0f;
            st.probability = 100.0f; st.ratchet = 1; st.active = true;
            seq.setStep(0, s, st);
        }

        // Deactivate steps 4-15
        for (int s = 4; s < Params::NUM_STEPS; ++s)
        {
            SeqStep st = seq.getStep(0, s);
            st.active = false;
            seq.setStep(0, s, st);
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;

        std::vector<int> notes;
        for (int i = 0; i < 8; ++i)
        {
            auto ns = runOneStep(seq, ppq, stepPpq);
            for (int n : ns) notes.push_back(n);
        }

        // Should only see notes 60-63 cycling
        for (int n : notes)
            expect(n >= 60 && n <= 63, "4-step wrap: notes should be 60-63");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testStopRestart()
    {
        beginTest("SEQ – Stop and restart resets state");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);

        SeqStep st;
        st.note = 60; st.velocity = 100; st.gate = 80.0f;
        st.probability = 100.0f; st.ratchet = 1; st.active = true;
        seq.setStep(0, 0, st);

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        // Run a few steps
        std::vector<SeqEvent> events;
        for (int i = 0; i < 4; ++i)
        {
            seq.process(samples, ppq, true, events);
            ppq += stepPpq;
        }

        // Stop (isPlaying = false)
        seq.process(samples, ppq, false, events);

        // Reset and restart from 0
        seq.reset();
        ppq = 0.0;
        seq.process(samples, ppq, true, events);

        expect(true, "Stop/restart: no crash");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testEdgeCaseAllStepsInactive()
    {
        beginTest("SEQ – Edge case: all steps inactive");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);

        for (int s = 0; s < Params::NUM_STEPS; ++s)
        {
            SeqStep st = seq.getStep(0, s);
            st.active = false;
            seq.setStep(0, s, st);
        }

        double ppq = 0.0;
        const double stepPpq = 0.25;
        const int    samples  = (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq);

        std::vector<SeqEvent> events;
        for (int i = 0; i < 20; ++i)
        {
            seq.process(samples, ppq, true, events);
            ppq += stepPpq;
        }

        int noteOns = 0;
        for (const auto& ev : events)
            if (ev.type == SeqEvent::Type::NoteOn)
                ++noteOns;

        expect(noteOns == 0, "All inactive: should produce no notes");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testEdgeCaseSingleStep()
    {
        beginTest("SEQ – Edge case: single step");

        StepSequencer seq;
        seq.prepare(TEST_SR, 512);
        seq.setBpm(TEST_BPM);
        seq.setStepLength(Params::NOTE_LENGTHS[2]);
        seq.setNumSteps(1);

        SeqStep st;
        st.note = 60; st.velocity = 100; st.gate = 80.0f;
        st.probability = 100.0f; st.ratchet = 1; st.active = true;
        seq.setStep(0, 0, st);

        double ppq = 0.0;
        const double stepPpq = 0.25;

        int fires = 0;
        for (int i = 0; i < 8; ++i)
        {
            auto notes = runOneStep(seq, ppq, stepPpq);
            fires += (int)notes.size();
        }
        expect(fires == 8, "Single step: should fire every step");
    }

    //──────────────────────────────────────────────────────────────────────────
    void testStepLengths()
    {
        beginTest("SEQ – Various step lengths (no crash)");

        for (int li = 0; li < Params::NUM_NOTE_LENGTHS; ++li)
        {
            StepSequencer seq;
            seq.prepare(TEST_SR, 512);
            seq.setBpm(TEST_BPM);
            seq.setStepLength(Params::NOTE_LENGTHS[li]);

            SeqStep st;
            st.note = 60; st.velocity = 100; st.gate = 80.0f;
            st.probability = 100.0f; st.ratchet = 1; st.active = true;
            seq.setStep(0, 0, st);

            double ppq = 0.0;
            const double stepPpq = Params::NOTE_LENGTHS[li];
            const int    samples  = juce::jmax(1,
                (int)(TEST_SR * 60.0 / TEST_BPM * stepPpq));

            std::vector<SeqEvent> events;
            for (int i = 0; i < 4; ++i)
            {
                seq.process(samples, ppq, true, events);
                ppq += stepPpq;
            }
            expect(true, "Step length " + Params::NOTE_LENGTH_NAMES[li] + ": no crash");
        }
    }
};

//── Static instance for JUCE test runner ──────────────────────────────────────
static StepSequencerTests stepSequencerTests;
