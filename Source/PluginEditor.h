#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Parameters.h"

//==============================================================================
// Forward declarations
class ArpPanel;
class SequencerPanel;
class HybridPanel;
class PresetBar;
class StepButton;
class LabelledKnob;

//==============================================================================
/**
 * LabelledKnob – a rotary slider with a centred label beneath it.
 * Supports a tooltip string.
 */
class LabelledKnob : public juce::Component
{
public:
    LabelledKnob (const juce::String& labelText,
                  const juce::String& tooltip = {});

    juce::Slider& getSlider() noexcept { return slider; }

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag,
                         juce::Slider::TextBoxBelow };
    juce::Label  label;
    juce::String tooltipText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelledKnob)
};

//==============================================================================
/**
 * StepButton – a single cell in the step-sequencer grid.
 * Displays note name, velocity bar, active state.
 * Right-click opens a small popup to edit gate/prob/ratchet.
 */
class StepButton : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    struct StepData
    {
        int   note        = 60;
        int   velocity    = 100;
        float gate        = 80.0f;
        float probability = 100.0f;
        int   ratchet     = 1;
        bool  active      = true;
    };

    StepButton();

    void setStepData (const StepData& d) { data = d; repaint(); }
    const StepData& getStepData() const  { return data; }

    void setHighlighted (bool h) { highlighted = h; repaint(); }
    bool isHighlighted()  const  { return highlighted; }

    /** Callback: called when the user modifies step data. */
    std::function<void(const StepData&)> onChange;

    void paint    (juce::Graphics& g) override;
    void resized  ()                  override {}
    void mouseDown(const juce::MouseEvent& e) override;

private:
    StepData data;
    bool highlighted = false;

    void showEditPopup();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepButton)
};

//==============================================================================
/**
 * SequencerGrid – the 16-step grid component.
 */
class SequencerGrid : public juce::Component, public juce::Timer
{
public:
    SequencerGrid (ArpSequencerAudioProcessor& proc);
    ~SequencerGrid() override;

    void paint   (juce::Graphics& g) override;
    void resized ()                  override;
    void timerCallback()             override;  // polls currentStep for highlighting

    /** Read back from APVTS and refresh all cell data. */
    void refreshFromAPVTS();

    /** Pattern selector (0-3). */
    void setActivePattern (int p);
    int  getActivePattern() const noexcept { return activePattern; }

private:
    ArpSequencerAudioProcessor& processor;
    int activePattern = 0;
    int lastHighlightedStep = -1;

    std::array<std::unique_ptr<StepButton>, Params::NUM_STEPS> steps;

    void onStepChanged (int stepIdx, const StepButton::StepData& d);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerGrid)
};

//==============================================================================
/**
 * ArpPanel – ARP mode controls.
 */
class ArpPanel : public juce::Component
{
public:
    explicit ArpPanel (ArpSequencerAudioProcessor& proc);
    ~ArpPanel() override;

    void resized() override;

private:
    ArpSequencerAudioProcessor& processor;
    using APVTS = juce::AudioProcessorValueTreeState;

    // Pattern selector
    juce::ComboBox        patternBox;
    juce::Label           patternLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> patternAtt;

    // Knobs
    LabelledKnob octaveKnob   { "Octave",   "Octave range (1-4)" };
    LabelledKnob swingKnob    { "Swing",    "Swing / shuffle (0-100%)" };
    LabelledKnob gateKnob     { "Gate",     "Gate length (1-100%)" };
    LabelledKnob velVarKnob   { "Vel Var",  "Velocity variation (0-127)" };
    LabelledKnob transposeKnob{ "Transp.",  "Transpose in semitones (-24 to +24)" };

    // Note length
    juce::ComboBox        noteLenBox;
    juce::Label           noteLenLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> noteLenAtt;

    // Toggles
    juce::ToggleButton holdButton  { "Hold" };
    juce::ToggleButton latchButton { "Latch" };

    // APVTS attachments
    std::unique_ptr<APVTS::SliderAttachment>   octaveAtt, swingAtt, gateAtt, velVarAtt, transposeAtt;
    std::unique_ptr<APVTS::ButtonAttachment>   holdAtt, latchAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpPanel)
};

//==============================================================================
/**
 * SequencerPanel – SEQ mode controls + the 16-step grid.
 */
class SequencerPanel : public juce::Component
{
public:
    explicit SequencerPanel (ArpSequencerAudioProcessor& proc);
    ~SequencerPanel() override;

    void resized() override;
    void paint   (juce::Graphics& g) override;

private:
    ArpSequencerAudioProcessor& processor;
    using APVTS = juce::AudioProcessorValueTreeState;

    // Grid
    SequencerGrid grid;

    // Controls
    juce::ComboBox stepLenBox;
    juce::Label    stepLenLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> stepLenAtt;

    juce::Slider   numStepsSlider { juce::Slider::LinearHorizontal,
                                    juce::Slider::TextBoxRight };
    juce::Label    numStepsLabel;
    std::unique_ptr<APVTS::SliderAttachment> numStepsAtt;

    // Pattern selectors (A/B/C/D buttons)
    std::array<juce::TextButton, Params::NUM_PATTERNS> patternButtons;

    // Chain controls
    juce::ToggleButton chainButton { "Chain" };
    juce::Slider       chainLenSlider { juce::Slider::LinearHorizontal,
                                        juce::Slider::TextBoxRight };
    juce::Label        chainLenLabel;
    std::unique_ptr<APVTS::ButtonAttachment> chainEnAtt;
    std::unique_ptr<APVTS::SliderAttachment> chainLenAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerPanel)
};

//==============================================================================
/**
 * HybridPanel – Hybrid mode routing controls.
 */
class HybridPanel : public juce::Component
{
public:
    explicit HybridPanel (ArpSequencerAudioProcessor& proc);
    ~HybridPanel() override;

    void resized() override;
    void paint   (juce::Graphics& g) override;

private:
    ArpSequencerAudioProcessor& processor;
    using APVTS = juce::AudioProcessorValueTreeState;

    juce::ToggleButton seqModArpButton { "Sequencer modulates ARP" };
    juce::ComboBox     modTargetBox;
    juce::Label        modTargetLabel;
    LabelledKnob       modDepthKnob { "Depth", "Modulation depth (0-100%)" };

    std::unique_ptr<APVTS::ButtonAttachment>   seqModArpAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> modTargetAtt;
    std::unique_ptr<APVTS::SliderAttachment>   modDepthAtt;

    juce::Label descLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridPanel)
};

//==============================================================================
/**
 * PresetBar – top bar with preset selector and save/load buttons.
 */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (ArpSequencerAudioProcessor& proc);
    ~PresetBar() override;

    void resized() override;
    void paint   (juce::Graphics& g) override;

private:
    ArpSequencerAudioProcessor& processor;
    juce::ComboBox    presetBox;
    juce::TextButton  saveButton { "Save" };
    juce::TextButton  loadButton { "Load" };
    juce::Label       bpmLabel;

    void populatePresetBox();
    void onSave();
    void onLoad();

    juce::Timer* bpmTimer = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};

//==============================================================================
/**
 * ArpSequencerAudioProcessorEditor – the main plugin window.
 *
 * Layout:
 *   ┌──────────────────────── Preset Bar ─────────────────────────┐
 *   │ Mode: [ARP] [SEQ] [Hybrid]  BPM: 120.0   BPM Sync: [✓]     │
 *   ├──────── Tab: ARP / SEQ / Hybrid ────────────────────────────┤
 *   │                                                              │
 *   │   (selected panel content)                                   │
 *   │                                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */
class ArpSequencerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Timer
{
public:
    explicit ArpSequencerAudioProcessorEditor (ArpSequencerAudioProcessor&);
    ~ArpSequencerAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;
    void timerCallback()           override;  // for BPM display refresh

private:
    ArpSequencerAudioProcessor& audioProcessor;
    using APVTS = juce::AudioProcessorValueTreeState;

    // Top bar
    PresetBar presetBar;

    // Mode selector
    juce::TextButton arpTabBtn     { "ARP" };
    juce::TextButton seqTabBtn     { "Sequencer" };
    juce::TextButton hybridTabBtn  { "Hybrid" };

    // Global controls
    juce::ToggleButton bpmSyncButton { "Host BPM" };
    juce::Slider       freeBpmSlider { juce::Slider::LinearHorizontal,
                                       juce::Slider::TextBoxRight };
    juce::Label        freeBpmLabel;
    juce::ComboBox     outChannelBox;
    juce::Label        outChannelLabel;
    juce::ToggleButton midiClockButton { "MIDI Clock" };

    std::unique_ptr<APVTS::ButtonAttachment>  bpmSyncAtt;
    std::unique_ptr<APVTS::SliderAttachment>  freeBpmAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> outChannelAtt;

    // Mode parameter
    juce::ComboBox     modeBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> modeAtt;
    juce::Label        modeLabel;

    // Panels
    ArpPanel      arpPanel;
    SequencerPanel seqPanel;
    HybridPanel    hybridPanel;

    int activeTab = 0; // 0=ARP, 1=SEQ, 2=Hybrid

    void switchTab (int tabIdx);
    void updateTabButtons();

    // Custom look and feel
    struct CustomLookAndFeel : public juce::LookAndFeel_V4
    {
        CustomLookAndFeel();
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float sliderPos, float startAngle, float endAngle,
                               juce::Slider& slider) override;
        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& bgColour,
                                   bool isMouseOver, bool isButtonDown) override;
    };

    CustomLookAndFeel customLnF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpSequencerAudioProcessorEditor)
};
