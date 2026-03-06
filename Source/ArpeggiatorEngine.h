#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <random>
#include "Parameters.h"

//==============================================================================
/** Represents a single MIDI note held by the arpeggiator. */
struct ArpNote
{
    int  note     = 60;
    int  velocity = 100;
    bool operator<(const ArpNote& o) const noexcept { return note < o.note; }
    bool operator==(const ArpNote& o) const noexcept { return note == o.note; }
};

//==============================================================================
/** A lightweight event emitted by the arpeggiator for MIDI output. */
struct ArpEvent
{
    enum class Type { NoteOn, NoteOff };
    Type type       = Type::NoteOn;
    int  note       = 0;
    int  velocity   = 0;
    int  channel    = 1;
    /** Sample offset within the current processBlock buffer. */
    int  sampleOffset = 0;
};

//==============================================================================
/**
 * ArpeggiatorEngine
 *
 * Thread-safe (caller must hold a lock or ensure single-thread access in the
 * audio callback). All state mutated only from the audio thread.
 *
 * Patterns supported:
 *   Up, Down, UpDown, DownUp, Random, Chord, AsPlayed
 *
 * Features:
 *   - Octave range 1-4
 *   - Note length 1/32 to whole note
 *   - Swing / shuffle 0-100 %
 *   - Velocity variation 0-127
 *   - Hold & Latch modes
 *   - Gate 1-100 %
 *   - Transpose ±24 semitones
 */
class ArpeggiatorEngine
{
public:
    //──────────────────────────────────────────────────────────────────────────
    ArpeggiatorEngine();
    ~ArpeggiatorEngine() = default;

    //── Setup ─────────────────────────────────────────────────────────────────
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    //── MIDI note tracking (call from audio thread) ───────────────────────────
    void noteOn  (int note, int velocity);
    void noteOff (int note);
    void allNotesOff();

    //── Parameter setters (call before processBlock, audio-thread safe) ───────
    void setPattern        (Params::ArpPattern p)      noexcept { pattern        = p; }
    void setOctaveRange    (int octaves)               noexcept { octaveRange    = juce::jlimit(1, 4, octaves); }
    void setNoteLength     (float beats)               noexcept { noteLengthBeats= beats; }
    void setSwing          (float pct)                 noexcept { swingPct       = juce::jlimit(0.0f, 100.0f, pct); }
    void setVelocityVariation (int variation)          noexcept { velocityVariation = juce::jlimit(0, 127, variation); }
    void setHoldMode       (bool h)                    noexcept { holdMode       = h; }
    void setLatchMode      (bool l)                    noexcept { latchMode      = l; }
    void setGate           (float pct)                 noexcept { gatePct        = juce::jlimit(1.0f, 100.0f, pct); }
    void setTranspose      (int semitones)             noexcept { transpose      = juce::jlimit(-24, 24, semitones); }
    void setBpm            (double bpm)                noexcept { currentBpm     = bpm; }
    void setOutputChannel  (int ch)                    noexcept { outputChannel  = juce::jlimit(1, 16, ch); }

    //── Main processing – returns MIDI events to send ─────────────────────────
    /**
     * Generates arpeggio events for one processBlock call.
     * @param numSamples     Length of the current buffer
     * @param ppqPosition    Host playhead position in quarter notes
     * @param isPlaying      Whether the host transport is running
     * @param outEvents      Filled with NoteOn/NoteOff events (sorted by sampleOffset)
     */
    void process (int numSamples,
                  double ppqPosition,
                  bool   isPlaying,
                  std::vector<ArpEvent>& outEvents);

    //── Query ─────────────────────────────────────────────────────────────────
    bool isRunning()         const noexcept { return !buildNoteList().empty(); }
    int  getCurrentStep()    const noexcept { return currentStep; }
    int  getNoteCount()      const noexcept;

private:
    //── Internal helpers ──────────────────────────────────────────────────────
    std::vector<ArpNote> buildNoteList()      const;
    int   nextNoteIndex (const std::vector<ArpNote>& notes);
    int   applyVelocityVariation (int baseVel) const;
    /** Returns swing offset in samples for even/odd step. */
    int   swingOffset   (bool isOddStep, double samplesPerBeat) const noexcept;

    //── State ─────────────────────────────────────────────────────────────────
    double sampleRate         = 44100.0;
    double currentBpm         = 120.0;

    // Note pools
    std::vector<ArpNote> heldNotes;    // currently pressed (sorted ascending)
    std::vector<ArpNote> latchedNotes; // latched notes (held after release)
    std::vector<ArpNote> playedOrder;  // as-played order

    // Step tracking
    int   currentStep         = 0;   // index into expanded note list
    int   currentOctaveOffset = 0;   // 0 to octaveRange-1
    bool  directionUp         = true;// for UpDown/DownUp
    int   stepParity          = 0;   // even/odd for swing

    // Timing
    double lastStepPpq        = -1.0; // ppq of last triggered step
    bool   noteIsOn           = false;// is there a currently sounding arpegg note?
    int    lastNoteOut        = -1;   // MIDI note number of currently sounding note
    double noteOffPpq         = -1.0; // when to send NoteOff

    // Ratchet sub-step tracking
    int   ratchetCount        = 1;
    int   ratchetSubStep      = 0;

    // Parameters (audio-thread copies)
    Params::ArpPattern pattern          = Params::ArpPattern::Up;
    int                octaveRange      = 1;
    float              noteLengthBeats  = 0.25f; // 1/16th
    float              swingPct         = 0.0f;
    int                velocityVariation= 0;
    bool               holdMode         = false;
    bool               latchMode        = false;
    float              gatePct          = 80.0f;
    int                transpose        = 0;
    int                outputChannel    = 1;

    // RNG
    mutable std::mt19937 rng { std::random_device{}() };
};
