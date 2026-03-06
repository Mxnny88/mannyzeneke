#include "PresetManager.h"
#include "Parameters.h"

//==============================================================================
const std::array<PresetManager::FactoryPreset, 8> PresetManager::FACTORY_PRESETS
{{
    { "01_Up_Classic",      "Simple upward arpeggio, 1 octave, 1/16th notes"           },
    { "02_Down_Sweep",      "Descending sweep, 2 octaves, swing 30%"                   },
    { "03_UpDown_Dance",    "Up-Down bounce, 2 octaves, fast gate"                     },
    { "04_Random_Glitch",   "Random pattern with velocity variation"                   },
    { "05_Chord_Pad",       "All notes simultaneously, long gate"                      },
    { "06_Seq_Bass_Line",   "Step sequencer bass pattern, 16 steps"                    },
    { "07_Seq_Melody",      "Step sequencer melodic pattern with ratchets"             },
    { "08_Hybrid_Groove",   "Hybrid: ARP + sequencer modulating swing"                 },
}};

//==============================================================================
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& a)
    : apvts (a)
{
}

//──────────────────────────────────────────────────────────────────────────────
juce::File PresetManager::getPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("ArpSequencer")
                   .getChildFile("Presets");
    if (!dir.exists())
        dir.createDirectory();
    return dir;
}

juce::File PresetManager::getPresetFile (const juce::String& presetName) const
{
    return getPresetDirectory().getChildFile(presetName + ".xml");
}

//──────────────────────────────────────────────────────────────────────────────
bool PresetManager::savePresetToFile (const juce::String& presetName) const
{
    if (presetName.isEmpty()) return false;

    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->setAttribute("presetName", presetName);
        xml->setAttribute("pluginVersion", "1.0.0");
        xml->setAttribute("savedAt", juce::Time::getCurrentTime().toISO8601(true));

        if (xml->writeTo(getPresetFile(presetName)))
        {
            DBG("PresetManager: saved preset '" << presetName << "'");
            return true;
        }
    }
    DBG("PresetManager: FAILED to save preset '" << presetName << "'");
    return false;
}

//──────────────────────────────────────────────────────────────────────────────
bool PresetManager::loadPresetFromFile (const juce::String& presetName)
{
    auto file = getPresetFile(presetName);
    if (!file.existsAsFile())
    {
        DBG("PresetManager: preset file not found: " << file.getFullPathName());
        return false;
    }

    if (auto xml = juce::XmlDocument::parse(file))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            apvts.replaceState(state);
            DBG("PresetManager: loaded preset '" << presetName << "'");
            return true;
        }
    }
    DBG("PresetManager: FAILED to parse preset '" << presetName << "'");
    return false;
}

//──────────────────────────────────────────────────────────────────────────────
bool PresetManager::deletePreset (const juce::String& presetName) const
{
    return getPresetFile(presetName).deleteFile();
}

//──────────────────────────────────────────────────────────────────────────────
juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.xml"))
        names.add(f.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}

//──────────────────────────────────────────────────────────────────────────────
void PresetManager::writeFactoryPresetsIfNeeded()
{
    // Helper: build a minimal ValueTree representing a given factory preset
    auto buildFactoryState = [](int presetIdx) -> juce::ValueTree
    {
        // We create the layout default state
        auto layout = Params::createParameterLayout();
        // Build a state tree with default values
        juce::ValueTree state("STATE");

        // Apply preset-specific overrides on top of defaults
        switch (presetIdx)
        {
            case 0: // 01_Up_Classic
                state.setProperty(Params::MODE_ID,          0, nullptr);
                state.setProperty(Params::ARP_PATTERN_ID,   0, nullptr); // Up
                state.setProperty(Params::ARP_OCTAVE_ID,    1, nullptr);
                state.setProperty(Params::ARP_NOTE_LEN_ID,  2, nullptr); // 1/16
                state.setProperty(Params::ARP_GATE_ID,      80, nullptr);
                state.setProperty(Params::ARP_SWING_ID,     0, nullptr);
                break;
            case 1: // 02_Down_Sweep
                state.setProperty(Params::MODE_ID,          0, nullptr);
                state.setProperty(Params::ARP_PATTERN_ID,   1, nullptr); // Down
                state.setProperty(Params::ARP_OCTAVE_ID,    2, nullptr);
                state.setProperty(Params::ARP_NOTE_LEN_ID,  4, nullptr); // 1/8
                state.setProperty(Params::ARP_SWING_ID,     30, nullptr);
                state.setProperty(Params::ARP_GATE_ID,      70, nullptr);
                break;
            case 2: // 03_UpDown_Dance
                state.setProperty(Params::MODE_ID,          0, nullptr);
                state.setProperty(Params::ARP_PATTERN_ID,   2, nullptr); // UpDown
                state.setProperty(Params::ARP_OCTAVE_ID,    2, nullptr);
                state.setProperty(Params::ARP_NOTE_LEN_ID,  2, nullptr); // 1/16
                state.setProperty(Params::ARP_GATE_ID,      50, nullptr);
                break;
            case 3: // 04_Random_Glitch
                state.setProperty(Params::MODE_ID,          0, nullptr);
                state.setProperty(Params::ARP_PATTERN_ID,   4, nullptr); // Random
                state.setProperty(Params::ARP_VEL_VAR_ID,   40, nullptr);
                state.setProperty(Params::ARP_SWING_ID,     20, nullptr);
                break;
            case 4: // 05_Chord_Pad
                state.setProperty(Params::MODE_ID,          0, nullptr);
                state.setProperty(Params::ARP_PATTERN_ID,   5, nullptr); // Chord
                state.setProperty(Params::ARP_NOTE_LEN_ID,  7, nullptr); // 1/2
                state.setProperty(Params::ARP_GATE_ID,      95, nullptr);
                break;
            case 5: // 06_Seq_Bass_Line
                state.setProperty(Params::MODE_ID,          1, nullptr); // SEQ
                state.setProperty(Params::SEQ_NUM_STEPS_ID, 16, nullptr);
                state.setProperty(Params::SEQ_STEP_LEN_ID,  2, nullptr);  // 1/16
                break;
            case 6: // 07_Seq_Melody
                state.setProperty(Params::MODE_ID,          1, nullptr);
                state.setProperty(Params::SEQ_NUM_STEPS_ID, 16, nullptr);
                state.setProperty(Params::SEQ_STEP_LEN_ID,  4, nullptr);  // 1/8
                break;
            case 7: // 08_Hybrid_Groove
                state.setProperty(Params::MODE_ID,              2, nullptr); // Hybrid
                state.setProperty(Params::ARP_PATTERN_ID,       2, nullptr); // UpDown
                state.setProperty(Params::ARP_OCTAVE_ID,        2, nullptr);
                state.setProperty(Params::HYBRID_SEQ_MOD_ARP_ID,1, nullptr);
                state.setProperty(Params::HYBRID_MOD_TARGET_ID, 1, nullptr); // Swing
                state.setProperty(Params::HYBRID_MOD_DEPTH_ID,  60, nullptr);
                break;
            default:
                break;
        }
        return state;
    };

    for (int i = 0; i < (int)FACTORY_PRESETS.size(); ++i)
    {
        const auto& fp = FACTORY_PRESETS[i];
        auto file = getPresetFile(fp.name);
        if (!file.existsAsFile())
        {
            auto state = buildFactoryState(i);
            if (auto xml = state.createXml())
            {
                xml->setAttribute("presetName",   fp.name);
                xml->setAttribute("description",  fp.description);
                xml->setAttribute("pluginVersion", "1.0.0");
                xml->setAttribute("factory",       "true");
                xml->writeTo(file);
                DBG("PresetManager: wrote factory preset '" << fp.name << "'");
            }
        }
    }
}

//──────────────────────────────────────────────────────────────────────────────
void PresetManager::exportPreset (juce::Component* parentComponent)
{
    juce::FileChooser fc ("Export Preset", juce::File::getSpecialLocation(
        juce::File::userDesktopDirectory), "*.xml");

    if (fc.browseForFileToSave(true))
    {
        auto file = fc.getResult().withFileExtension("xml");
        auto state = apvts.copyState();
        if (auto xml = state.createXml())
            xml->writeTo(file);
    }
    (void)parentComponent;
}

void PresetManager::importPreset (juce::Component* parentComponent)
{
    juce::FileChooser fc ("Import Preset", juce::File::getSpecialLocation(
        juce::File::userDesktopDirectory), "*.xml");

    if (fc.browseForFileToOpen())
    {
        auto file = fc.getResult();
        if (auto xml = juce::XmlDocument::parse(file))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            if (state.isValid())
                apvts.replaceState(state);
        }
    }
    (void)parentComponent;
}
