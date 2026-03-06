#pragma once
#include <JuceHeader.h>

//==============================================================================
// All parameter IDs, ranges and default values for the ArpSequencer plugin.
// Using constexpr to keep everything in one translation-unit-friendly header.
//==============================================================================

namespace Params
{
    //─── Global ────────────────────────────────────────────────────────────────
    static const juce::String MODE_ID          { "mode" };          // 0=ARP, 1=SEQ, 2=Hybrid
    static const juce::String BPM_SYNC_ID      { "bpmSync" };       // 0=free, 1=host sync
    static const juce::String FREE_BPM_ID      { "freeBpm" };       // 20–300
    static const juce::String OUTPUT_CHANNEL_ID{ "outChannel" };    // 1–16

    //─── Arpeggiator ───────────────────────────────────────────────────────────
    static const juce::String ARP_PATTERN_ID   { "arpPattern" };    // enum ArpPattern
    static const juce::String ARP_OCTAVE_ID    { "arpOctave" };     // 1–4
    static const juce::String ARP_NOTE_LEN_ID  { "arpNoteLen" };    // index into NoteLength[]
    static const juce::String ARP_SWING_ID     { "arpSwing" };      // 0–100 %
    static const juce::String ARP_VEL_VAR_ID   { "arpVelVar" };     // 0–127
    static const juce::String ARP_HOLD_ID      { "arpHold" };       // 0/1
    static const juce::String ARP_LATCH_ID     { "arpLatch" };      // 0/1
    static const juce::String ARP_GATE_ID      { "arpGate" };       // 1–100 %
    static const juce::String ARP_TRANSPOSE_ID { "arpTranspose" };  // -24 to +24 semitones

    //─── Step Sequencer ────────────────────────────────────────────────────────
    static const juce::String SEQ_NUM_STEPS_ID { "seqNumSteps" };   // 1–16
    static const juce::String SEQ_STEP_LEN_ID  { "seqStepLen" };    // index into NoteLength[]
    static const juce::String SEQ_PATTERN_ID   { "seqPattern" };    // active pattern 0–3
    static const juce::String SEQ_CHAIN_EN_ID  { "seqChainEn" };    // 0/1
    static const juce::String SEQ_CHAIN_LEN_ID { "seqChainLen" };   // 1–4

    // Per-step params – pattern P (0-3), step S (0-15)
    // ID format:  "seqNote_P_S", "seqVel_P_S", etc.
    inline juce::String stepNoteID   (int p, int s) { return "seqNote_"   + juce::String(p) + "_" + juce::String(s); }
    inline juce::String stepVelID    (int p, int s) { return "seqVel_"    + juce::String(p) + "_" + juce::String(s); }
    inline juce::String stepGateID   (int p, int s) { return "seqGate_"   + juce::String(p) + "_" + juce::String(s); }
    inline juce::String stepProbID   (int p, int s) { return "seqProb_"   + juce::String(p) + "_" + juce::String(s); }
    inline juce::String stepRatchetID(int p, int s) { return "seqRatchet_"+ juce::String(p) + "_" + juce::String(s); }
    inline juce::String stepActiveID (int p, int s) { return "seqActive_" + juce::String(p) + "_" + juce::String(s); }

    //─── Hybrid ────────────────────────────────────────────────────────────────
    static const juce::String HYBRID_SEQ_MOD_ARP_ID  { "hybridSeqModArp" };  // 0/1
    static const juce::String HYBRID_MOD_DEPTH_ID    { "hybridModDepth" };   // 0–100 %
    static const juce::String HYBRID_MOD_TARGET_ID   { "hybridModTarget" };  // enum ModTarget

    //─── Numeric constants ──────────────────────────────────────────────────────
    static constexpr int   NUM_PATTERNS  = 4;
    static constexpr int   NUM_STEPS     = 16;
    static constexpr float DEFAULT_BPM   = 120.0f;
    static constexpr int   MAX_VOICES    = 128;

    //─── Note-length table (in beats, i.e. quarter-note = 1.0) ─────────────────
    // Index:  0     1     2     3    4    5    6     7   8
    // Name: 1/32 1/16T 1/16  1/8T 1/8  1/4T 1/4  1/2   1
    static constexpr float NOTE_LENGTHS[] =
    {
        0.125f,          // 0  – 1/32
        0.1667f,         // 1  – 1/16 triplet
        0.25f,           // 2  – 1/16
        0.3333f,         // 3  – 1/8 triplet
        0.5f,            // 4  – 1/8
        0.6667f,         // 5  – 1/4 triplet
        1.0f,            // 6  – 1/4   (quarter)
        2.0f,            // 7  – 1/2   (half)
        4.0f             // 8  – whole
    };
    static constexpr int NUM_NOTE_LENGTHS = 9;

    static const juce::StringArray NOTE_LENGTH_NAMES
    {
        "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1"
    };

    //─── Arpeggiator pattern names ──────────────────────────────────────────────
    enum class ArpPattern { Up=0, Down, UpDown, DownUp, Random, Chord, AsPlayed, COUNT };
    static const juce::StringArray ARP_PATTERN_NAMES
    {
        "Up", "Down", "Up-Down", "Down-Up", "Random", "Chord", "As Played"
    };

    //─── Hybrid mod targets ─────────────────────────────────────────────────────
    enum class ModTarget { ArpOctave=0, ArpSwing, ArpGate, ArpVelVar, COUNT };
    static const juce::StringArray MOD_TARGET_NAMES
    {
        "Octave", "Swing", "Gate", "Vel Var"
    };

    //─── Helper: build the full APVTS layout ────────────────────────────────────
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Global
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            MODE_ID, "Mode",
            juce::StringArray{"ARP","Sequencer","Hybrid"}, 0));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            BPM_SYNC_ID, "BPM Sync", true));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            FREE_BPM_ID, "Free BPM",
            juce::NormalisableRange<float>(20.0f, 300.0f, 0.01f, 0.5f), DEFAULT_BPM));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            OUTPUT_CHANNEL_ID, "Output Channel", 1, 16, 1));

        // Arpeggiator
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            ARP_PATTERN_ID, "ARP Pattern",
            ARP_PATTERN_NAMES, 0));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            ARP_OCTAVE_ID, "ARP Octave Range", 1, 4, 1));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            ARP_NOTE_LEN_ID, "ARP Note Length",
            NOTE_LENGTH_NAMES, 2));  // default 1/16

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            ARP_SWING_ID, "ARP Swing",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            ARP_VEL_VAR_ID, "ARP Velocity Variation", 0, 127, 0));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            ARP_HOLD_ID, "ARP Hold", false));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            ARP_LATCH_ID, "ARP Latch", false));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            ARP_GATE_ID, "ARP Gate",
            juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f), 80.0f));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            ARP_TRANSPOSE_ID, "ARP Transpose", -24, 24, 0));

        // Sequencer global
        layout.add(std::make_unique<juce::AudioParameterInt>(
            SEQ_NUM_STEPS_ID, "Seq Num Steps", 1, NUM_STEPS, NUM_STEPS));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            SEQ_STEP_LEN_ID, "Seq Step Length",
            NOTE_LENGTH_NAMES, 2));  // default 1/16

        layout.add(std::make_unique<juce::AudioParameterInt>(
            SEQ_PATTERN_ID, "Active Pattern", 0, NUM_PATTERNS-1, 0));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            SEQ_CHAIN_EN_ID, "Pattern Chain", false));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            SEQ_CHAIN_LEN_ID, "Chain Length", 1, NUM_PATTERNS, NUM_PATTERNS));

        // Per-step params
        for (int p = 0; p < NUM_PATTERNS; ++p)
        {
            for (int s = 0; s < NUM_STEPS; ++s)
            {
                layout.add(std::make_unique<juce::AudioParameterInt>(
                    stepNoteID(p, s), "Pattern " + juce::String(p+1) + " Step " + juce::String(s+1) + " Note",
                    0, 127, 60));  // Middle C default

                layout.add(std::make_unique<juce::AudioParameterInt>(
                    stepVelID(p, s), "P" + juce::String(p+1) + " S" + juce::String(s+1) + " Velocity",
                    0, 127, 100));

                layout.add(std::make_unique<juce::AudioParameterFloat>(
                    stepGateID(p, s), "P" + juce::String(p+1) + " S" + juce::String(s+1) + " Gate",
                    juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 80.0f));

                layout.add(std::make_unique<juce::AudioParameterFloat>(
                    stepProbID(p, s), "P" + juce::String(p+1) + " S" + juce::String(s+1) + " Probability",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));

                layout.add(std::make_unique<juce::AudioParameterInt>(
                    stepRatchetID(p, s), "P" + juce::String(p+1) + " S" + juce::String(s+1) + " Ratchet",
                    1, 4, 1));

                layout.add(std::make_unique<juce::AudioParameterBool>(
                    stepActiveID(p, s), "P" + juce::String(p+1) + " S" + juce::String(s+1) + " Active",
                    true));
            }
        }

        // Hybrid
        layout.add(std::make_unique<juce::AudioParameterBool>(
            HYBRID_SEQ_MOD_ARP_ID, "Seq Modulates ARP", false));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            HYBRID_MOD_DEPTH_ID, "Mod Depth",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            HYBRID_MOD_TARGET_ID, "Mod Target",
            MOD_TARGET_NAMES, 0));

        return layout;
    }

} // namespace Params
