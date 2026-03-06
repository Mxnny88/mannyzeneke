#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

//==============================================================================
ArpSequencerAudioProcessor::ArpSequencerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", Params::createParameterLayout())
{
    // Reserve scratch buffers to avoid runtime allocations in audio thread
    arpEventBuf.reserve(256);
    seqEventBuf.reserve(256);

    // Initialise 8 empty presets
    presetNames.add("Init");
    for (int i = 1; i < NUM_PRESETS; ++i)
        presetNames.add("Preset " + juce::String(i + 1));

    // Save default state as preset 0
    presetData.resize(NUM_PRESETS);
    for (int i = 0; i < NUM_PRESETS; ++i)
        presetData.set(i, apvts.copyState());

    // Listen to all parameters for engine sync
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.addParameterListener(p->getParameterID(), this);
    }
}

ArpSequencerAudioProcessor::~ArpSequencerAudioProcessor()
{
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.removeParameterListener(p->getParameterID(), this);
    }
}

//==============================================================================
void ArpSequencerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    arpEngine.prepare(sampleRate, samplesPerBlock);
    sequencer.prepare(sampleRate, samplesPerBlock);

    arpEngine.reset();
    sequencer.reset();

    lastClockPpq = -1.0;
    DBG("ArpSequencer: prepared at " << sampleRate << " Hz, " << samplesPerBlock << " samples");
}

void ArpSequencerAudioProcessor::releaseResources()
{
    arpEngine.allNotesOff();
}

//==============================================================================
void ArpSequencerAudioProcessor::parameterChanged (const juce::String& /*paramID*/, float /*newValue*/)
{
    // Actual sync happens in syncEngineParams() each block for thread safety
}

//==============================================================================
void ArpSequencerAudioProcessor::syncEngineParams (double bpm)
{
    // ─── Global ──────────────────────────────────────────────────────────────
    const int outCh = static_cast<int>(*apvts.getRawParameterValue(Params::OUTPUT_CHANNEL_ID));
    arpEngine.setOutputChannel(outCh);
    sequencer.setOutputChannel(outCh);
    arpEngine.setBpm(bpm);
    sequencer.setBpm(bpm);

    // ─── ARP ─────────────────────────────────────────────────────────────────
    const int arpPatIdx  = static_cast<int>(*apvts.getRawParameterValue(Params::ARP_PATTERN_ID));
    arpEngine.setPattern(static_cast<Params::ArpPattern>(arpPatIdx));

    arpEngine.setOctaveRange(static_cast<int>(*apvts.getRawParameterValue(Params::ARP_OCTAVE_ID)));

    const int arpLenIdx = static_cast<int>(*apvts.getRawParameterValue(Params::ARP_NOTE_LEN_ID));
    arpEngine.setNoteLength(Params::NOTE_LENGTHS[juce::jlimit(0, Params::NUM_NOTE_LENGTHS-1, arpLenIdx)]);

    arpEngine.setSwing  (static_cast<float>(*apvts.getRawParameterValue(Params::ARP_SWING_ID)));
    arpEngine.setVelocityVariation(static_cast<int>(*apvts.getRawParameterValue(Params::ARP_VEL_VAR_ID)));
    arpEngine.setHoldMode (static_cast<bool>(*apvts.getRawParameterValue(Params::ARP_HOLD_ID)  > 0.5f));
    arpEngine.setLatchMode(static_cast<bool>(*apvts.getRawParameterValue(Params::ARP_LATCH_ID) > 0.5f));
    arpEngine.setGate    (static_cast<float>(*apvts.getRawParameterValue(Params::ARP_GATE_ID)));
    arpEngine.setTranspose(static_cast<int>(*apvts.getRawParameterValue(Params::ARP_TRANSPOSE_ID)));

    // ─── SEQ ─────────────────────────────────────────────────────────────────
    sequencer.setNumSteps(static_cast<int>(*apvts.getRawParameterValue(Params::SEQ_NUM_STEPS_ID)));

    const int seqLenIdx = static_cast<int>(*apvts.getRawParameterValue(Params::SEQ_STEP_LEN_ID));
    sequencer.setStepLength(Params::NOTE_LENGTHS[juce::jlimit(0, Params::NUM_NOTE_LENGTHS-1, seqLenIdx)]);

    sequencer.setActivePattern(static_cast<int>(*apvts.getRawParameterValue(Params::SEQ_PATTERN_ID)));
    sequencer.setChainEnabled (static_cast<bool>(*apvts.getRawParameterValue(Params::SEQ_CHAIN_EN_ID) > 0.5f));
    sequencer.setChainLength  (static_cast<int>(*apvts.getRawParameterValue(Params::SEQ_CHAIN_LEN_ID)));

    // Sync per-step data from APVTS into the sequencer
    for (int p = 0; p < Params::NUM_PATTERNS; ++p)
    {
        for (int s = 0; s < Params::NUM_STEPS; ++s)
        {
            SeqStep st;
            st.note        = static_cast<int>  (*apvts.getRawParameterValue(Params::stepNoteID   (p, s)));
            st.velocity    = static_cast<int>  (*apvts.getRawParameterValue(Params::stepVelID    (p, s)));
            st.gate        = static_cast<float>(*apvts.getRawParameterValue(Params::stepGateID   (p, s)));
            st.probability = static_cast<float>(*apvts.getRawParameterValue(Params::stepProbID   (p, s)));
            st.ratchet     = static_cast<int>  (*apvts.getRawParameterValue(Params::stepRatchetID(p, s)));
            st.active      = static_cast<bool> (*apvts.getRawParameterValue(Params::stepActiveID (p, s)) > 0.5f);
            sequencer.setStep(p, s, st);
        }
    }
}

//==============================================================================
void ArpSequencerAudioProcessor::applyHybridModulation()
{
    const bool seqModArp = *apvts.getRawParameterValue(Params::HYBRID_SEQ_MOD_ARP_ID) > 0.5f;
    if (!seqModArp) return;

    const float modDepth  = *apvts.getRawParameterValue(Params::HYBRID_MOD_DEPTH_ID) / 100.0f;
    const int   modTarget = static_cast<int>(*apvts.getRawParameterValue(Params::HYBRID_MOD_TARGET_ID));

    // Use the current step's velocity (normalised 0-1) as modulation source
    const int   pat  = sequencer.getCurrentPattern();
    const int   step = sequencer.getCurrentStep();
    const float mod  = (sequencer.getStep(pat, step).velocity / 127.0f) * modDepth;

    switch (static_cast<Params::ModTarget>(modTarget))
    {
        case Params::ModTarget::ArpOctave:
            arpEngine.setOctaveRange(1 + static_cast<int>(mod * 3.0f)); // 1-4
            break;
        case Params::ModTarget::ArpSwing:
            arpEngine.setSwing(mod * 100.0f);
            break;
        case Params::ModTarget::ArpGate:
            arpEngine.setGate(1.0f + mod * 99.0f);
            break;
        case Params::ModTarget::ArpVelVar:
            arpEngine.setVelocityVariation(static_cast<int>(mod * 127.0f));
            break;
        default:
            break;
    }
}

//==============================================================================
void ArpSequencerAudioProcessor::processIncomingMidi (const juce::MidiBuffer& midiIn,
                                                        int /*numSamples*/)
{
    for (const auto meta : midiIn)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            arpEngine.noteOn(msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff())
            arpEngine.noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            arpEngine.allNotesOff();
    }
}

//──────────────────────────────────────────────────────────────────────────────
void ArpSequencerAudioProcessor::flushArpEvents (const std::vector<ArpEvent>& evts,
                                                   juce::MidiBuffer& midi)
{
    for (const auto& ev : evts)
    {
        juce::MidiMessage msg;
        if (ev.type == ArpEvent::Type::NoteOn)
            msg = juce::MidiMessage::noteOn (ev.channel, ev.note, (juce::uint8)ev.velocity);
        else
            msg = juce::MidiMessage::noteOff(ev.channel, ev.note);

        midi.addEvent(msg, ev.sampleOffset);
    }
}

void ArpSequencerAudioProcessor::flushSeqEvents (const std::vector<SeqEvent>& evts,
                                                   juce::MidiBuffer& midi)
{
    for (const auto& ev : evts)
    {
        juce::MidiMessage msg;
        if (ev.type == SeqEvent::Type::NoteOn)
            msg = juce::MidiMessage::noteOn (ev.channel, ev.note, (juce::uint8)ev.velocity);
        else
            msg = juce::MidiMessage::noteOff(ev.channel, ev.note);

        midi.addEvent(msg, ev.sampleOffset);
    }
}

//──────────────────────────────────────────────────────────────────────────────
void ArpSequencerAudioProcessor::sendMidiClock (double ppqPosition,
                                                  double bpm,
                                                  int    numSamples,
                                                  juce::MidiBuffer& midi)
{
    // MIDI clock: 24 pulses per quarter note
    static constexpr double PPQN = 24.0;
    const double samplesPerBeat  = getSampleRate() * 60.0 / bpm;
    const double samplesPerPulse = samplesPerBeat / PPQN;
    const double ppqEnd          = ppqPosition + numSamples / samplesPerBeat;

    if (lastClockPpq < 0.0)
        lastClockPpq = std::floor(ppqPosition * PPQN) / PPQN;

    double nextPulsePpq = lastClockPpq + (1.0 / PPQN);
    while (nextPulsePpq <= ppqEnd)
    {
        int offset = juce::jlimit(0, numSamples - 1,
            static_cast<int>((nextPulsePpq - ppqPosition) * samplesPerBeat));
        midi.addEvent(juce::MidiMessage::midiClock(), offset);
        lastClockPpq  = nextPulsePpq;
        nextPulsePpq += (1.0 / PPQN);
    }
}

//==============================================================================
void ArpSequencerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear audio output (this is a MIDI effect)
    buffer.clear();

    //── Get host playhead info ────────────────────────────────────────────────
    auto* playHead = getPlayHead();
    double ppqPosition = 0.0;
    bool   isPlaying   = false;
    double bpm         = 120.0;

    if (playHead != nullptr)
    {
        if (auto posInfo = playHead->getPosition())
        {
            isPlaying  = posInfo->getIsPlaying();

            if (auto ppq = posInfo->getPpqPosition())
                ppqPosition = *ppq;

            if (auto tempo = posInfo->getBpm())
                bpm = *tempo;
        }
    }

    // Fall back to free BPM if host not providing tempo or bpm sync is off
    const bool bpmSync = *apvts.getRawParameterValue(Params::BPM_SYNC_ID) > 0.5f;
    if (!bpmSync)
        bpm = static_cast<double>(*apvts.getRawParameterValue(Params::FREE_BPM_ID));

    bpm = juce::jlimit(20.0, 300.0, bpm);
    lastKnownBpm.store(bpm);

    //── Sync engine parameters ────────────────────────────────────────────────
    syncEngineParams(bpm);

    //── Determine operating mode ──────────────────────────────────────────────
    const int mode = static_cast<int>(*apvts.getRawParameterValue(Params::MODE_ID));
    // 0 = ARP, 1 = SEQ, 2 = Hybrid

    const int numSamples = buffer.getNumSamples();
    juce::MidiBuffer outMidi;

    //── Process incoming MIDI (always feed ARP) ───────────────────────────────
    processIncomingMidi(midiMessages, numSamples);

    //── Hybrid modulation (before engines run) ────────────────────────────────
    if (mode == 2)
        applyHybridModulation();

    //── Generate output based on mode ────────────────────────────────────────
    if (mode == 0 || mode == 2)  // ARP or Hybrid
    {
        arpEngine.process(numSamples, ppqPosition, isPlaying, arpEventBuf);
        flushArpEvents(arpEventBuf, outMidi);
    }

    if (mode == 1 || mode == 2)  // SEQ or Hybrid
    {
        sequencer.process(numSamples, ppqPosition, isPlaying, seqEventBuf);
        flushSeqEvents(seqEventBuf, outMidi);
    }

    //── MIDI clock ────────────────────────────────────────────────────────────
    if (midiClockEnabled && isPlaying)
        sendMidiClock(ppqPosition, bpm, numSamples, outMidi);

    //── Replace input MIDI with output MIDI ──────────────────────────────────
    midiMessages.swapWith(outMidi);
}

//==============================================================================
//── Preset management ─────────────────────────────────────────────────────────
void ArpSequencerAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < NUM_PRESETS)
    {
        loadPreset(index);
        currentPreset = index;
    }
}

const juce::String ArpSequencerAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < presetNames.size())
        return presetNames[index];
    return {};
}

void ArpSequencerAudioProcessor::changeProgramName (int index, const juce::String& name)
{
    if (index >= 0 && index < presetNames.size())
        presetNames.set(index, name);
}

void ArpSequencerAudioProcessor::savePreset (int index, const juce::String& name)
{
    if (index < 0 || index >= NUM_PRESETS) return;
    presetData.set(index, apvts.copyState());
    presetNames.set(index, name);
    currentPreset = index;
}

void ArpSequencerAudioProcessor::loadPreset (int index)
{
    if (index < 0 || index >= NUM_PRESETS) return;
    if (presetData[index].isValid())
    {
        apvts.replaceState(presetData[index]);
        arpEngine.reset();
        sequencer.reset();
    }
}

//==============================================================================
//── State save / load (for DAW session recall) ────────────────────────────────
void ArpSequencerAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();

    // Attach preset data to saved state
    auto presetsXml = state.getOrCreateChildWithName("Presets", nullptr);
    for (int i = 0; i < NUM_PRESETS; ++i)
    {
        auto child = presetsXml.getOrCreateChildWithName("Preset" + juce::String(i), nullptr);
        child.setProperty("name", presetNames[i], nullptr);
        if (presetData[i].isValid())
        {
            auto data = presetData[i];
            child.addChild(data, -1, nullptr);
        }
    }

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}

void ArpSequencerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            // Restore presets
            auto presetsXml = state.getChildWithName("Presets");
            if (presetsXml.isValid())
            {
                for (int i = 0; i < NUM_PRESETS; ++i)
                {
                    auto child = presetsXml.getChildWithName("Preset" + juce::String(i));
                    if (child.isValid())
                    {
                        presetNames.set(i, child.getProperty("name", "Preset " + juce::String(i+1)).toString());
                        if (child.getNumChildren() > 0)
                            presetData.set(i, child.getChild(0));
                    }
                }
                // Remove presets node before restoring main state
                state.removeChild(presetsXml, nullptr);
            }
            apvts.replaceState(state);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArpSequencerAudioProcessor();
}
