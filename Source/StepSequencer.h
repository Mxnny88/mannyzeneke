#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <random>
#include "Parameters.h"

//==============================================================================
/** Data for one step in the sequencer. */
struct SeqStep
{
    int   note        = 60;    // MIDI note 0-127
    int   velocity    = 100;   // 0-127
    float gate        = 80.0f; // % of step length (1-100)
    float probability = 100.0f;// % chance of firing (0-100)
    int   ratchet     = 1;     // repeats within this step (1-4)
    bool  active      = true;  // step on/off
};

//==============================================================================
/** Event emitted by the sequencer for MIDI output. */
struct SeqEvent
{
    enum class Type { NoteOn, NoteOff };
    Type type         = Type::NoteOn;
    int  note         = 0;
    int  velocity     = 0;
    int  channel      = 1;
    int  sampleOffset = 0;
};

//==============================================================================
/**
 * StepSequencer
 *
 * 16-step sequencer with 4 storable patterns and optional pattern chaining.
 * Runs in sync with host BPM or in free-running mode.
 * All state is managed from the audio thread (single-threaded access assumed).
 *
 * Features:
 *   - Per-step: note, velocity, gate length, probability, ratchet (1-4)
 *   - 4 patterns (16 steps each), chainable 1-4 patterns
 *   - BPM sync or free-running
 *   - MIDI clock output (not implemented here; handled by processor)
 */
class StepSequencer
{
public:
    //──────────────────────────────────────────────────────────────────────────
    StepSequencer();
    ~StepSequencer() = default;

    //── Setup ─────────────────────────────────────────────────────────────────
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    //── Step data accessors (safe to call from any thread for UI display) ─────
    SeqStep&       getStep    (int pattern, int step)       { return patterns[pattern][step]; }
    const SeqStep& getStep    (int pattern, int step) const { return patterns[pattern][step]; }
    void           setStep    (int pattern, int step, const SeqStep& s) { patterns[pattern][step] = s; }

    //── Parameter setters ─────────────────────────────────────────────────────
    void setNumSteps    (int n)            noexcept { numSteps    = juce::jlimit(1, Params::NUM_STEPS, n); }
    void setStepLength  (float beats)      noexcept { stepLenBeats= beats; }
    void setBpm         (double bpm)       noexcept { currentBpm  = bpm; }
    void setOutputChannel (int ch)         noexcept { outputChannel = juce::jlimit(1, 16, ch); }

    /** Active pattern (0-3) – only used when chain is disabled. */
    void setActivePattern  (int p)         noexcept { activePattern = juce::jlimit(0, Params::NUM_PATTERNS-1, p); }

    /** Chain: which patterns to play in sequence. */
    void setChainEnabled   (bool en)       noexcept { chainEnabled = en; }
    void setChainLength    (int len)       noexcept { chainLength  = juce::jlimit(1, Params::NUM_PATTERNS, len); }
    /** Set which pattern is at position idx in the chain (order 0-3). */
    void setChainPattern   (int idx, int patternNum) noexcept
    {
        if (idx >= 0 && idx < Params::NUM_PATTERNS)
            chainOrder[idx] = juce::jlimit(0, Params::NUM_PATTERNS-1, patternNum);
    }

    //── Query ─────────────────────────────────────────────────────────────────
    int  getCurrentStep()    const noexcept { return currentStep; }
    int  getCurrentPattern() const noexcept { return currentPattern(); }

    //── Main processing ───────────────────────────────────────────────────────
    /**
     * Generates sequencer events for one processBlock call.
     * @param numSamples   Length of current buffer
     * @param ppqPosition  Host playhead position in beats
     * @param isPlaying    Host transport state
     * @param outEvents    Filled with NoteOn/NoteOff events
     */
    void process (int numSamples,
                  double ppqPosition,
                  bool   isPlaying,
                  std::vector<SeqEvent>& outEvents);

private:
    //── Pattern storage ───────────────────────────────────────────────────────
    std::array<std::array<SeqStep, Params::NUM_STEPS>, Params::NUM_PATTERNS> patterns;

    //── Parameters ────────────────────────────────────────────────────────────
    int    numSteps       = Params::NUM_STEPS;
    float  stepLenBeats   = 0.25f;   // 1/16th note default
    double currentBpm     = 120.0;
    double sampleRate     = 44100.0;
    int    outputChannel  = 1;
    int    activePattern  = 0;

    // Chain
    bool chainEnabled = false;
    int  chainLength  = Params::NUM_PATTERNS;
    std::array<int, Params::NUM_PATTERNS> chainOrder { 0, 1, 2, 3 };

    //── Runtime state ─────────────────────────────────────────────────────────
    int    currentStep        = 0;
    int    currentChainIdx    = 0;   // position in chain
    double lastStepPpq        = -1.0;
    bool   noteIsOn           = false;
    int    lastNoteOut        = -1;
    double noteOffPpq         = -1.0;

    // Ratchet sub-step
    int    ratchetSubStep     = 0;
    int    ratchetTotal       = 1;

    // RNG for probability
    mutable std::mt19937 rng { std::random_device{}() };

    //── Helpers ───────────────────────────────────────────────────────────────
    int  currentPattern() const noexcept;
    bool shouldFire (float probability) const;
    void emitNoteOn  (const SeqStep& step, int sampleOffset, double stepStartPpq,
                      std::vector<SeqEvent>& outEvents);
    void checkNoteOff(double ppqPosition, double ppqEnd, double samplesPerBeat,
                      int numSamples, std::vector<SeqEvent>& outEvents);
};
