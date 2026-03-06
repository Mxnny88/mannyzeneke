#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "ArpeggiatorEngine.h"
#include "StepSequencer.h"

//==============================================================================
/**
 * ArpSequencerAudioProcessor
 *
 * Main VST3/AU audio processor. Manages:
 *   - AudioProcessorValueTreeState (APVTS) for all parameters + automation
 *   - ArpeggiatorEngine for ARP mode
 *   - StepSequencer for SEQ mode
 *   - Hybrid mode: sequencer modulates ARP parameters
 *   - Preset save/load via APVTS serialisation
 *   - MIDI clock output
 *   - Error logging to JUCE debug output
 */
class ArpSequencerAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    //──────────────────────────────────────────────────────────────────────────
    ArpSequencerAudioProcessor();
    ~ArpSequencerAudioProcessor() override;

    //── AudioProcessor interface ──────────────────────────────────────────────
    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources()                                       override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor()  override;
    bool hasEditor()                     const  override { return true; }

    const juce::String getName()         const  override { return "ArpSequencer"; }
    bool acceptsMidi()                   const  override { return true; }
    bool producesMidi()                  const  override { return true; }
    bool isMidiEffect()                  const  override { return false; }
    double getTailLengthSeconds()        const  override { return 0.0; }

    int  getNumPrograms()                        override { return static_cast<int>(presetNames.size()); }
    int  getCurrentProgram()                     override { return currentPreset; }
    void setCurrentProgram (int index)           override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& name) override;

    void getStateInformation  (juce::MemoryBlock& dest) override;
    void setStateInformation  (const void* data, int size) override;

    //── Parameter listener ───────────────────────────────────────────────────
    void parameterChanged (const juce::String& paramID, float newValue) override;

    //── Public accessors for UI ───────────────────────────────────────────────
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    ArpeggiatorEngine&  getArpEngine()   noexcept { return arpEngine; }
    StepSequencer&      getSequencer()   noexcept { return sequencer; }

    /** Save current state as a named preset (index 0-7). */
    void savePreset    (int index, const juce::String& name);
    /** Load a preset by index. */
    void loadPreset    (int index);
    /** Returns all preset names. */
    const juce::StringArray& getPresetNames() const noexcept { return presetNames; }

    //── MIDI clock ────────────────────────────────────────────────────────────
    /** Returns whether MIDI clock is being output. */
    bool isMidiClockEnabled() const noexcept { return midiClockEnabled; }
    void setMidiClockEnabled (bool en)       noexcept { midiClockEnabled = en; }

    /** Expose current BPM for the UI. */
    double getCurrentBpm() const noexcept { return lastKnownBpm.load(); }

private:
    //── APVTS ─────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;

    //── Engines ───────────────────────────────────────────────────────────────
    ArpeggiatorEngine arpEngine;
    StepSequencer     sequencer;

    //── Preset storage ────────────────────────────────────────────────────────
    static constexpr int NUM_PRESETS = 8;
    juce::StringArray   presetNames;
    juce::Array<juce::ValueTree> presetData;
    int currentPreset = 0;

    //── MIDI processing helpers ───────────────────────────────────────────────
    /** Reads MIDI input events and feeds them to the ARP engine. */
    void processIncomingMidi (const juce::MidiBuffer& midiIn, int numSamples);

    /** Converts ARP events to MIDI messages in the output buffer. */
    void flushArpEvents  (const std::vector<ArpEvent>&  evts, juce::MidiBuffer& midi);
    void flushSeqEvents  (const std::vector<SeqEvent>&  evts, juce::MidiBuffer& midi);

    /** Sends MIDI clock ticks for the current buffer. */
    void sendMidiClock (double ppqPosition, double bpm, int numSamples,
                        juce::MidiBuffer& midi);

    /** Updates all engine parameters from APVTS (called at start of each block). */
    void syncEngineParams (double bpm);

    /** Applies hybrid modulation: sequencer step value modulates ARP params. */
    void applyHybridModulation();

    //── Thread-safe BPM cache ─────────────────────────────────────────────────
    std::atomic<double> lastKnownBpm { 120.0 };

    //── MIDI clock state ──────────────────────────────────────────────────────
    bool   midiClockEnabled   = false;
    double lastClockPpq       = -1.0;

    //── Event scratch buffers (avoid allocation in audio thread) ──────────────
    std::vector<ArpEvent> arpEventBuf;
    std::vector<SeqEvent> seqEventBuf;

    //── All notes off tracking ────────────────────────────────────────────────
    juce::MidiBuffer allNotesOffBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpSequencerAudioProcessor)
};
