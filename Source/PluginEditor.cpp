#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
// createEditor() – the one definition required by the plugin host
juce::AudioProcessorEditor* ArpSequencerAudioProcessor::createEditor()
{
    return new ArpSequencerAudioProcessorEditor (*this);
}

//==============================================================================
// Colour palette
static const Colour BG_DARK     { 0xFF1A1A2E };
static const Colour BG_MID      { 0xFF16213E };
static const Colour BG_PANEL    { 0xFF0F3460 };
static const Colour ACCENT      { 0xFFE94560 };
static const Colour ACCENT2     { 0xFF533483 };
static const Colour TEXT_BRIGHT { 0xFFEEEEEE };
static const Colour TEXT_DIM    { 0xFF888888 };
static const Colour STEP_ON     { 0xFF4ECDC4 };
static const Colour STEP_OFF    { 0xFF2A4858 };
static const Colour STEP_HI     { 0xFFFFE66D };

//==============================================================================
// LabelledKnob
//==============================================================================
LabelledKnob::LabelledKnob (const String& labelText, const String& tooltip)
    : tooltipText (tooltip)
{
    slider.setSliderStyle(Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 16);
    slider.setTooltip(tooltip);
    addAndMakeVisible(slider);

    label.setText(labelText, dontSendNotification);
    label.setJustificationType(Justification::centredTop);
    label.setFont(Font(11.0f, Font::bold));
    label.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(label);
}

void LabelledKnob::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromBottom(16));
    slider.setBounds(b);
}

void LabelledKnob::paint (Graphics&) {}

//==============================================================================
// StepButton
//==============================================================================
StepButton::StepButton()
{
    setTooltip("Right-click to edit step details");
}

void StepButton::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    Colour bg = data.active ? (highlighted ? STEP_HI : STEP_ON) : STEP_OFF;

    // Drop shadow / border
    g.setColour(bg.darker(0.4f));
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(bg);
    g.fillRoundedRectangle(b.reduced(1.0f), 4.0f);

    // Velocity bar at bottom
    if (data.active)
    {
        float velFrac = data.velocity / 127.0f;
        Colour velColour = bg.interpolatedWith(Colours::white, 0.3f);
        g.setColour(velColour.withAlpha(0.7f));
        float barH = b.getHeight() * 0.25f * velFrac;
        g.fillRoundedRectangle(b.reduced(3.0f).removeFromBottom(barH), 2.0f);
    }

    // Note name
    g.setColour(data.active ? TEXT_BRIGHT : TEXT_DIM);
    g.setFont(Font(10.0f, Font::bold));
    const StringArray noteNames { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    int oct  = data.note / 12 - 1;
    String noteName = noteNames[data.note % 12] + String(oct);
    g.drawText(noteName, b.toNearestInt(), Justification::centredTop, false);

    // Ratchet dots
    if (data.active && data.ratchet > 1)
    {
        g.setColour(ACCENT.withAlpha(0.9f));
        for (int r = 0; r < data.ratchet; ++r)
        {
            float dotX = b.getX() + 3.0f + r * 5.0f;
            float dotY = b.getBottom() - 6.0f;
            g.fillEllipse(dotX, dotY, 3.0f, 3.0f);
        }
    }

    // Probability indicator (dashed outline if < 100%)
    if (data.probability < 100.0f)
    {
        g.setColour(ACCENT2.withAlpha(0.8f));
        g.drawRoundedRectangle(b.reduced(1.0f), 4.0f, 1.0f);
    }
}

void StepButton::mouseDown (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        data.active = !data.active;
        repaint();
        if (onChange) onChange(data);
    }
    else if (e.mods.isRightButtonDown())
    {
        showEditPopup();
    }
}

void StepButton::showEditPopup()
{
    auto* popup = new Component();
    popup->setSize(240, 220);

    auto addSlider = [&](Component* parent, Slider*& sl, Label*& lb,
                         const String& text, double minV, double maxV, double val) {
        lb = new Label({}, text);
        lb->setFont(Font(11.0f));
        lb->setColour(Label::textColourId, TEXT_BRIGHT);
        parent->addAndMakeVisible(lb);

        sl = new Slider(Slider::LinearHorizontal, Slider::TextBoxRight);
        sl->setRange(minV, maxV, 1.0);
        sl->setValue(val, dontSendNotification);
        sl->setTextBoxStyle(Slider::TextBoxRight, false, 40, 18);
        parent->addAndMakeVisible(sl);
    };

    Slider *noteSl{}, *velSl{}, *gateSl{}, *probSl{}, *ratchetSl{};
    Label  *noteLb{}, *velLb{}, *gateLb{}, *probLb{}, *ratchetLb{};

    addSlider(popup, noteSl,    noteLb,    "Note",        0,   127, data.note);
    addSlider(popup, velSl,     velLb,     "Velocity",    0,   127, data.velocity);
    addSlider(popup, gateSl,    gateLb,    "Gate %",      1,   100, data.gate);
    addSlider(popup, probSl,    probLb,    "Prob %",      0,   100, data.probability);
    addSlider(popup, ratchetSl, ratchetLb, "Ratchet",     1,   4,   data.ratchet);

    int y = 10;
    for (auto [lb, sl] : std::initializer_list<std::pair<Label*, Slider*>>
         {{ noteLb, noteSl }, { velLb, velSl }, { gateLb, gateSl },
          { probLb, probSl }, { ratchetLb, ratchetSl }})
    {
        lb->setBounds(10, y, 70, 20);
        sl->setBounds(80, y, 150, 20);
        y += 32;
    }

    auto okBtn = std::make_unique<TextButton>("OK");
    auto cancelBtn = std::make_unique<TextButton>("Cancel");
    okBtn->setBounds(20, y + 8, 80, 24);
    cancelBtn->setBounds(140, y + 8, 80, 24);

    StepData newData = data;

    okBtn->onClick = [this, noteSl, velSl, gateSl, probSl, ratchetSl, newData]() mutable {
        newData.note        = (int)noteSl->getValue();
        newData.velocity    = (int)velSl->getValue();
        newData.gate        = (float)gateSl->getValue();
        newData.probability = (float)probSl->getValue();
        newData.ratchet     = (int)ratchetSl->getValue();
        data = newData;
        repaint();
        if (onChange) onChange(data);
        if (auto* cw = findParentComponentOfClass<CallOutBox>())
            cw->dismiss();
    };

    popup->addAndMakeVisible(okBtn.release());
    popup->addAndMakeVisible(cancelBtn.release());

    auto& box = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(popup),
                                                  getScreenBounds(), nullptr);
    (void)box;
}

//==============================================================================
// SequencerGrid
//==============================================================================
SequencerGrid::SequencerGrid (ArpSequencerAudioProcessor& proc)
    : processor (proc)
{
    for (int s = 0; s < Params::NUM_STEPS; ++s)
    {
        steps[s] = std::make_unique<StepButton>();
        steps[s]->onChange = [this, s](const StepButton::StepData& d) {
            onStepChanged(s, d);
        };
        addAndMakeVisible(*steps[s]);
    }
    refreshFromAPVTS();
    startTimerHz(15); // poll for step highlight at 15 Hz
}

SequencerGrid::~SequencerGrid()
{
    stopTimer();
}

void SequencerGrid::resized()
{
    const int cols = 8, rows = 2;
    int cellW = getWidth()  / cols;
    int cellH = getHeight() / rows;
    for (int s = 0; s < Params::NUM_STEPS; ++s)
    {
        int col = s % cols;
        int row = s / cols;
        steps[s]->setBounds(col * cellW, row * cellH, cellW, cellH);
    }
}

void SequencerGrid::paint (Graphics& g)
{
    g.fillAll(BG_PANEL.darker(0.2f));
}

void SequencerGrid::timerCallback()
{
    const int cur = processor.getSequencer().getCurrentStep();
    if (cur != lastHighlightedStep)
    {
        if (lastHighlightedStep >= 0 && lastHighlightedStep < Params::NUM_STEPS)
            steps[lastHighlightedStep]->setHighlighted(false);
        if (cur >= 0 && cur < Params::NUM_STEPS)
            steps[cur]->setHighlighted(true);
        lastHighlightedStep = cur;
    }
}

void SequencerGrid::refreshFromAPVTS()
{
    auto& apvts = processor.getAPVTS();
    for (int s = 0; s < Params::NUM_STEPS; ++s)
    {
        StepButton::StepData d;
        d.note        = (int)  *apvts.getRawParameterValue(Params::stepNoteID   (activePattern, s));
        d.velocity    = (int)  *apvts.getRawParameterValue(Params::stepVelID    (activePattern, s));
        d.gate        = (float)*apvts.getRawParameterValue(Params::stepGateID   (activePattern, s));
        d.probability = (float)*apvts.getRawParameterValue(Params::stepProbID   (activePattern, s));
        d.ratchet     = (int)  *apvts.getRawParameterValue(Params::stepRatchetID(activePattern, s));
        d.active      = (float)*apvts.getRawParameterValue(Params::stepActiveID (activePattern, s)) > 0.5f;
        steps[s]->setStepData(d);
    }
}

void SequencerGrid::setActivePattern (int p)
{
    activePattern = jlimit(0, Params::NUM_PATTERNS-1, p);
    refreshFromAPVTS();
}

void SequencerGrid::onStepChanged (int stepIdx, const StepButton::StepData& d)
{
    auto& apvts = processor.getAPVTS();
    auto setParam = [&](const String& id, float val) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(val));
    };

    setParam(Params::stepNoteID   (activePattern, stepIdx), (float)d.note);
    setParam(Params::stepVelID    (activePattern, stepIdx), (float)d.velocity);
    setParam(Params::stepGateID   (activePattern, stepIdx), d.gate);
    setParam(Params::stepProbID   (activePattern, stepIdx), d.probability);
    setParam(Params::stepRatchetID(activePattern, stepIdx), (float)d.ratchet);
    setParam(Params::stepActiveID (activePattern, stepIdx), d.active ? 1.0f : 0.0f);
}

//==============================================================================
// ArpPanel
//==============================================================================
ArpPanel::ArpPanel (ArpSequencerAudioProcessor& proc)
    : processor (proc)
{
    auto& apvts = proc.getAPVTS();

    // Pattern combo
    for (const auto& name : Params::ARP_PATTERN_NAMES)
        patternBox.addItem(name, patternBox.getNumItems() + 1);
    patternLabel.setText("Pattern", dontSendNotification);
    patternLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(patternBox);
    addAndMakeVisible(patternLabel);
    patternAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::ARP_PATTERN_ID, patternBox);

    // Note length combo
    for (const auto& name : Params::NOTE_LENGTH_NAMES)
        noteLenBox.addItem(name, noteLenBox.getNumItems() + 1);
    noteLenLabel.setText("Note Len", dontSendNotification);
    noteLenLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(noteLenBox);
    addAndMakeVisible(noteLenLabel);
    noteLenAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::ARP_NOTE_LEN_ID, noteLenBox);

    // Knobs
    octaveKnob.getSlider().setRange(1.0, 4.0, 1.0);
    octaveKnob.getSlider().setNumDecimalPlacesToDisplay(0);
    swingKnob.getSlider().setRange(0.0, 100.0, 0.1);
    gateKnob.getSlider().setRange(1.0, 100.0, 0.1);
    velVarKnob.getSlider().setRange(0.0, 127.0, 1.0);
    transposeKnob.getSlider().setRange(-24.0, 24.0, 1.0);

    for (auto* knob : { &octaveKnob, &swingKnob, &gateKnob, &velVarKnob, &transposeKnob })
        addAndMakeVisible(*knob);

    octaveAtt    = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, Params::ARP_OCTAVE_ID,    octaveKnob.getSlider());
    swingAtt     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, Params::ARP_SWING_ID,     swingKnob.getSlider());
    gateAtt      = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, Params::ARP_GATE_ID,      gateKnob.getSlider());
    velVarAtt    = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, Params::ARP_VEL_VAR_ID,   velVarKnob.getSlider());
    transposeAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, Params::ARP_TRANSPOSE_ID, transposeKnob.getSlider());

    // Toggle buttons
    holdButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    latchButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    addAndMakeVisible(holdButton);
    addAndMakeVisible(latchButton);
    holdAtt  = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(apvts, Params::ARP_HOLD_ID,  holdButton);
    latchAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(apvts, Params::ARP_LATCH_ID, latchButton);
}

ArpPanel::~ArpPanel() = default;

void ArpPanel::resized()
{
    auto b = getLocalBounds().reduced(12);

    // Row 1: Pattern + NoteLen combos
    auto row1 = b.removeFromTop(40);
    patternLabel.setBounds(row1.removeFromLeft(70));
    patternBox.setBounds(row1.removeFromLeft(100).reduced(0, 5));
    row1.removeFromLeft(20);
    noteLenLabel.setBounds(row1.removeFromLeft(65));
    noteLenBox.setBounds(row1.removeFromLeft(80).reduced(0, 5));

    b.removeFromTop(8);

    // Row 2: 5 knobs
    auto row2 = b.removeFromTop(90);
    int knobW = row2.getWidth() / 5;
    for (auto* knob : { &octaveKnob, &swingKnob, &gateKnob, &velVarKnob, &transposeKnob })
        knob->setBounds(row2.removeFromLeft(knobW));

    b.removeFromTop(8);

    // Row 3: toggles
    auto row3 = b.removeFromTop(30);
    holdButton.setBounds(row3.removeFromLeft(80));
    row3.removeFromLeft(12);
    latchButton.setBounds(row3.removeFromLeft(80));
}

//==============================================================================
// SequencerPanel
//==============================================================================
SequencerPanel::SequencerPanel (ArpSequencerAudioProcessor& proc)
    : processor (proc), grid (proc)
{
    auto& apvts = proc.getAPVTS();

    addAndMakeVisible(grid);

    // Step length
    for (const auto& name : Params::NOTE_LENGTH_NAMES)
        stepLenBox.addItem(name, stepLenBox.getNumItems() + 1);
    stepLenLabel.setText("Step Len", dontSendNotification);
    stepLenLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(stepLenBox);
    addAndMakeVisible(stepLenLabel);
    stepLenAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::SEQ_STEP_LEN_ID, stepLenBox);

    // Num steps
    numStepsSlider.setRange(1.0, Params::NUM_STEPS, 1.0);
    numStepsSlider.setNumDecimalPlacesToDisplay(0);
    numStepsLabel.setText("Steps", dontSendNotification);
    numStepsLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(numStepsSlider);
    addAndMakeVisible(numStepsLabel);
    numStepsAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        apvts, Params::SEQ_NUM_STEPS_ID, numStepsSlider);

    // Pattern A/B/C/D buttons
    for (int i = 0; i < Params::NUM_PATTERNS; ++i)
    {
        patternButtons[i].setButtonText(String::charToString('A' + i));
        patternButtons[i].setClickingTogglesState(false);
        patternButtons[i].onClick = [this, i]()
        {
            grid.setActivePattern(i);
            // Update SEQ_PATTERN_ID param
            auto& ap = processor.getAPVTS();
            if (auto* p = ap.getParameter(Params::SEQ_PATTERN_ID))
                p->setValueNotifyingHost(p->convertTo0to1((float)i));
            for (int j = 0; j < Params::NUM_PATTERNS; ++j)
                patternButtons[j].setToggleState(j == i, dontSendNotification);
        };
        patternButtons[i].setToggleState(i == 0, dontSendNotification);
        addAndMakeVisible(patternButtons[i]);
    }

    // Chain
    chainButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    addAndMakeVisible(chainButton);
    chainEnAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, Params::SEQ_CHAIN_EN_ID, chainButton);

    chainLenSlider.setRange(1.0, Params::NUM_PATTERNS, 1.0);
    chainLenSlider.setNumDecimalPlacesToDisplay(0);
    chainLenLabel.setText("Chain Len", dontSendNotification);
    chainLenLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(chainLenSlider);
    addAndMakeVisible(chainLenLabel);
    chainLenAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        apvts, Params::SEQ_CHAIN_LEN_ID, chainLenSlider);
}

SequencerPanel::~SequencerPanel() = default;

void SequencerPanel::paint (Graphics& g)
{
    g.fillAll(BG_MID);
}

void SequencerPanel::resized()
{
    auto b = getLocalBounds().reduced(10);

    // Control row at top
    auto ctrlRow = b.removeFromTop(36);
    stepLenLabel.setBounds(ctrlRow.removeFromLeft(58));
    stepLenBox.setBounds(ctrlRow.removeFromLeft(80).reduced(0, 6));
    ctrlRow.removeFromLeft(12);
    numStepsLabel.setBounds(ctrlRow.removeFromLeft(46));
    numStepsSlider.setBounds(ctrlRow.removeFromLeft(130).reduced(0, 6));

    b.removeFromTop(6);

    // Pattern buttons row
    auto patRow = b.removeFromTop(30);
    int btnW = 46;
    for (int i = 0; i < Params::NUM_PATTERNS; ++i)
        patternButtons[i].setBounds(patRow.removeFromLeft(btnW).reduced(2));

    b.removeFromTop(6);

    // Chain row
    auto chainRow = b.removeFromTop(28);
    chainButton.setBounds(chainRow.removeFromLeft(80));
    chainRow.removeFromLeft(12);
    chainLenLabel.setBounds(chainRow.removeFromLeft(70));
    chainLenSlider.setBounds(chainRow.removeFromLeft(130).reduced(0, 4));

    b.removeFromTop(8);

    // Grid takes the rest
    grid.setBounds(b);
}

//==============================================================================
// HybridPanel
//==============================================================================
HybridPanel::HybridPanel (ArpSequencerAudioProcessor& proc)
    : processor (proc)
{
    auto& apvts = proc.getAPVTS();

    seqModArpButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    addAndMakeVisible(seqModArpButton);
    seqModArpAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, Params::HYBRID_SEQ_MOD_ARP_ID, seqModArpButton);

    for (const auto& name : Params::MOD_TARGET_NAMES)
        modTargetBox.addItem(name, modTargetBox.getNumItems() + 1);
    modTargetLabel.setText("Mod Target", dontSendNotification);
    modTargetLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(modTargetBox);
    addAndMakeVisible(modTargetLabel);
    modTargetAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::HYBRID_MOD_TARGET_ID, modTargetBox);

    modDepthKnob.getSlider().setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(modDepthKnob);
    modDepthAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        apvts, Params::HYBRID_MOD_DEPTH_ID, modDepthKnob.getSlider());

    descLabel.setText(
        "In Hybrid mode, both the Arpeggiator and Sequencer run simultaneously.\n"
        "The Sequencer can modulate ARP parameters using the current step's velocity\n"
        "as a control source. Set the target parameter and modulation depth above.",
        dontSendNotification);
    descLabel.setColour(Label::textColourId, TEXT_DIM);
    descLabel.setFont(Font(11.5f));
    descLabel.setJustificationType(Justification::topLeft);
    addAndMakeVisible(descLabel);
}

HybridPanel::~HybridPanel() = default;

void HybridPanel::paint (Graphics& g)
{
    g.fillAll(BG_MID);
}

void HybridPanel::resized()
{
    auto b = getLocalBounds().reduced(14);

    seqModArpButton.setBounds(b.removeFromTop(28));
    b.removeFromTop(10);

    auto row2 = b.removeFromTop(36);
    modTargetLabel.setBounds(row2.removeFromLeft(80));
    modTargetBox.setBounds(row2.removeFromLeft(120).reduced(0, 6));
    row2.removeFromLeft(20);
    modDepthKnob.setBounds(row2.removeFromLeft(80));

    b.removeFromTop(14);
    descLabel.setBounds(b);
}

//==============================================================================
// PresetBar
//==============================================================================
PresetBar::PresetBar (ArpSequencerAudioProcessor& proc)
    : processor (proc)
{
    populatePresetBox();
    addAndMakeVisible(presetBox);

    saveButton.onClick = [this]() { onSave(); };
    addAndMakeVisible(saveButton);

    loadButton.onClick = [this]() { onLoad(); };
    addAndMakeVisible(loadButton);

    bpmLabel.setFont(Font(12.0f, Font::bold));
    bpmLabel.setColour(Label::textColourId, ACCENT);
    bpmLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(bpmLabel);
}

PresetBar::~PresetBar() = default;

void PresetBar::populatePresetBox()
{
    presetBox.clear();
    const auto& names = processor.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem(names[i], i + 1);
    presetBox.setSelectedId(processor.getCurrentProgram() + 1, dontSendNotification);
}

void PresetBar::onSave()
{
    // JUCE 7: use a simple AlertWindow with an input field
    auto* aw = new AlertWindow("Save Preset", "Enter preset name:", AlertWindow::NoIcon);
    aw->addTextEditor("name", processor.getPresetNames()[processor.getCurrentProgram()]);
    aw->addButton("OK",     1, KeyPress(KeyPress::returnKey));
    aw->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

    aw->enterModalState(true,
        ModalCallbackFunction::create([this, aw](int result)
        {
            if (result == 1)
            {
                String name = aw->getTextEditorContents("name");
                if (name.isNotEmpty())
                {
                    int idx = presetBox.getSelectedId() - 1;
                    if (idx < 0) idx = 0;
                    processor.savePreset(idx, name);
                    populatePresetBox();
                }
            }
        }), true);
}

void PresetBar::onLoad()
{
    int idx = presetBox.getSelectedId() - 1;
    if (idx >= 0)
        processor.loadPreset(idx);
}

void PresetBar::resized()
{
    auto b = getLocalBounds().reduced(4);
    bpmLabel.setBounds(b.removeFromRight(120));
    saveButton.setBounds(b.removeFromRight(60).reduced(2));
    loadButton.setBounds(b.removeFromRight(60).reduced(2));
    b.removeFromRight(8);
    b.removeFromLeft(4);
    presetBox.setBounds(b.removeFromLeft(200).reduced(0, 4));
}

void PresetBar::paint (Graphics& g)
{
    g.fillAll(BG_DARK.darker(0.3f));
    g.setColour(ACCENT.withAlpha(0.5f));
    g.drawLine(0, (float)getHeight()-1, (float)getWidth(), (float)getHeight()-1, 1.0f);
}

//==============================================================================
// CustomLookAndFeel
//==============================================================================
ArpSequencerAudioProcessorEditor::CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(Slider::thumbColourId,              ACCENT);
    setColour(Slider::rotarySliderFillColourId,   ACCENT);
    setColour(Slider::rotarySliderOutlineColourId,BG_PANEL);
    setColour(Slider::trackColourId,              BG_PANEL);
    setColour(ComboBox::backgroundColourId,BG_PANEL);
    setColour(ComboBox::textColourId,      TEXT_BRIGHT);
    setColour(ComboBox::arrowColourId,     ACCENT);
    setColour(ComboBox::outlineColourId,   ACCENT.withAlpha(0.4f));
    setColour(ToggleButton::textColourId,  TEXT_BRIGHT);
    setColour(ToggleButton::tickColourId,  ACCENT);
    setColour(TextButton::buttonColourId,  BG_PANEL);
    setColour(TextButton::buttonOnColourId,ACCENT);
    setColour(TextButton::textColourOnId,  TEXT_BRIGHT);
    setColour(TextButton::textColourOffId, TEXT_DIM);
    setColour(Label::textColourId,         TEXT_BRIGHT);
    setColour(Slider::textBoxTextColourId, TEXT_DIM);
    setColour(Slider::textBoxBackgroundColourId, BG_DARK.withAlpha(0.0f));
    setColour(Slider::textBoxOutlineColourId,    Colours::transparentBlack);
}

void ArpSequencerAudioProcessorEditor::CustomLookAndFeel::drawRotarySlider
    (Graphics& g, int x, int y, int w, int h,
     float sliderPos, float startAngle, float endAngle, Slider& /*slider*/)
{
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float radius = jmin(w, h) * 0.38f;

    // Track arc
    Path trackArc;
    trackArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(BG_PANEL);
    g.strokePath(trackArc, PathStrokeType(4.0f, PathStrokeType::curved,
                                          PathStrokeType::rounded));

    // Fill arc
    float angle = startAngle + sliderPos * (endAngle - startAngle);
    Path fillArc;
    fillArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour(ACCENT);
    g.strokePath(fillArc, PathStrokeType(4.0f, PathStrokeType::curved,
                                         PathStrokeType::rounded));

    // Thumb dot
    float thumbX = cx + radius * std::sin(angle);
    float thumbY = cy - radius * std::cos(angle);
    g.setColour(TEXT_BRIGHT);
    g.fillEllipse(thumbX - 4.0f, thumbY - 4.0f, 8.0f, 8.0f);

    // Centre dot
    g.setColour(BG_MID);
    g.fillEllipse(cx - radius * 0.5f, cy - radius * 0.5f, radius, radius);
}

void ArpSequencerAudioProcessorEditor::CustomLookAndFeel::drawButtonBackground
    (Graphics& g, Button& button, const Colour& /*bgColour*/,
     bool isMouseOver, bool isButtonDown)
{
    auto b = button.getLocalBounds().toFloat();
    Colour base = button.getToggleState() ? ACCENT : BG_PANEL;
    if (isButtonDown)  base = base.darker(0.3f);
    if (isMouseOver)   base = base.brighter(0.1f);

    g.setColour(base);
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(ACCENT.withAlpha(0.5f));
    g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
}

//==============================================================================
// ArpSequencerAudioProcessorEditor
//==============================================================================
ArpSequencerAudioProcessorEditor::ArpSequencerAudioProcessorEditor
    (ArpSequencerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      presetBar (p),
      arpPanel (p),
      seqPanel (p),
      hybridPanel (p)
{
    setLookAndFeel(&customLnF);

    addAndMakeVisible(presetBar);

    // Mode selector
    modeBox.addItem("ARP",       1);
    modeBox.addItem("Sequencer", 2);
    modeBox.addItem("Hybrid",    3);
    modeLabel.setText("Mode:", dontSendNotification);
    modeLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(modeLabel);
    modeAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.getAPVTS(), Params::MODE_ID, modeBox);
    modeBox.onChange = [this]()
    {
        int m = modeBox.getSelectedId() - 1;
        switchTab(m);
    };

    // Tab buttons
    for (auto* btn : { &arpTabBtn, &seqTabBtn, &hybridTabBtn })
    {
        btn->setClickingTogglesState(false);
        addAndMakeVisible(btn);
    }
    arpTabBtn.onClick    = [this]() { switchTab(0); };
    seqTabBtn.onClick    = [this]() { switchTab(1); };
    hybridTabBtn.onClick = [this]() { switchTab(2); };

    // Global BPM controls
    bpmSyncButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    addAndMakeVisible(bpmSyncButton);
    bpmSyncAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        p.getAPVTS(), Params::BPM_SYNC_ID, bpmSyncButton);

    freeBpmSlider.setRange(20.0, 300.0, 0.01);
    freeBpmSlider.setSkewFactorFromMidPoint(120.0);
    freeBpmLabel.setText("Free BPM", dontSendNotification);
    freeBpmLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(freeBpmSlider);
    addAndMakeVisible(freeBpmLabel);
    freeBpmAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        p.getAPVTS(), Params::FREE_BPM_ID, freeBpmSlider);

    // Output channel
    for (int ch = 1; ch <= 16; ++ch)
        outChannelBox.addItem("Ch " + String(ch), ch);
    outChannelLabel.setText("Out Ch", dontSendNotification);
    outChannelLabel.setColour(Label::textColourId, TEXT_DIM);
    addAndMakeVisible(outChannelBox);
    addAndMakeVisible(outChannelLabel);
    outChannelAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.getAPVTS(), Params::OUTPUT_CHANNEL_ID, outChannelBox);

    // MIDI clock toggle
    midiClockButton.setColour(ToggleButton::textColourId, TEXT_BRIGHT);
    midiClockButton.onClick = [this]() {
        audioProcessor.setMidiClockEnabled(midiClockButton.getToggleState());
    };
    addAndMakeVisible(midiClockButton);

    // Panels
    addAndMakeVisible(arpPanel);
    addChildComponent(seqPanel);
    addChildComponent(hybridPanel);

    switchTab(0);
    updateTabButtons();

    // Resizable with min/max
    setResizable(true, true);
    setResizeLimits(640, 440, 1280, 880);
    setSize(800, 560);

    startTimerHz(4); // for BPM label refresh
}

ArpSequencerAudioProcessorEditor::~ArpSequencerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//──────────────────────────────────────────────────────────────────────────────
void ArpSequencerAudioProcessorEditor::switchTab (int tabIdx)
{
    activeTab = tabIdx;
    arpPanel.setVisible   (tabIdx == 0);
    seqPanel.setVisible   (tabIdx == 1);
    hybridPanel.setVisible(tabIdx == 2);
    updateTabButtons();
    resized();
}

void ArpSequencerAudioProcessorEditor::updateTabButtons()
{
    arpTabBtn.setToggleState   (activeTab == 0, dontSendNotification);
    seqTabBtn.setToggleState   (activeTab == 1, dontSendNotification);
    hybridTabBtn.setToggleState(activeTab == 2, dontSendNotification);
}

void ArpSequencerAudioProcessorEditor::timerCallback()
{
    // Update BPM display in preset bar
    presetBar.repaint();
    // Reflect BPM in preset bar label
    double bpm = audioProcessor.getCurrentBpm();
    // (the label is updated via repaint; set text here if desired)
    (void)bpm;
}

//──────────────────────────────────────────────────────────────────────────────
void ArpSequencerAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(BG_DARK);

    // Title
    g.setColour(ACCENT);
    g.setFont(Font("Arial", 14.0f, Font::bold));
    g.drawText("ArpSequencer", getLocalBounds().removeFromLeft(160).withY(40).withHeight(24),
               Justification::centredLeft, false);
}

void ArpSequencerAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();

    // Preset bar at top
    presetBar.setBounds(b.removeFromTop(36));

    // Global controls row
    auto ctrlRow = b.removeFromTop(38);
    ctrlRow.reduce(8, 4);

    modeLabel.setBounds(ctrlRow.removeFromLeft(44));
    modeBox.setBounds(ctrlRow.removeFromLeft(100).reduced(0, 5));
    ctrlRow.removeFromLeft(16);
    bpmSyncButton.setBounds(ctrlRow.removeFromLeft(90).reduced(0, 6));
    freeBpmLabel.setBounds(ctrlRow.removeFromLeft(64));
    freeBpmSlider.setBounds(ctrlRow.removeFromLeft(130).reduced(0, 6));
    ctrlRow.removeFromLeft(12);
    outChannelLabel.setBounds(ctrlRow.removeFromLeft(52));
    outChannelBox.setBounds(ctrlRow.removeFromLeft(70).reduced(0, 5));
    ctrlRow.removeFromLeft(10);
    midiClockButton.setBounds(ctrlRow.removeFromLeft(100).reduced(0, 6));

    // Tab buttons
    auto tabRow = b.removeFromTop(32);
    tabRow.reduce(8, 2);
    int tabW = 110;
    arpTabBtn.setBounds   (tabRow.removeFromLeft(tabW).reduced(2));
    seqTabBtn.setBounds   (tabRow.removeFromLeft(tabW).reduced(2));
    hybridTabBtn.setBounds(tabRow.removeFromLeft(tabW).reduced(2));

    // Divider line (drawn in paint)
    b.removeFromTop(2);

    // Panels
    arpPanel.setBounds   (b);
    seqPanel.setBounds   (b);
    hybridPanel.setBounds(b);
}
