#pragma once
#include <JuceHeader.h>

//==============================================================================
/**
 * PresetManager
 *
 * Handles serialisation of plugin state to/from user preset files on disk.
 * Presets are stored as XML files in the standard JUCE user-application-data
 * directory, under "ArpSequencer/Presets/".
 *
 * This class is complementary to the in-memory preset slots managed by the
 * PluginProcessor – it provides persistent file-based preset management for
 * a dedicated Save/Load-to-disk workflow.
 *
 * Thread safety: call only from the message thread.
 */
class PresetManager
{
public:
    //──────────────────────────────────────────────────────────────────────────
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager() = default;

    //── File operations ───────────────────────────────────────────────────────
    /** Save current APVTS state to a named file (overwrites if exists). */
    bool savePresetToFile  (const juce::String& presetName) const;

    /** Load a named preset from disk and apply it to the APVTS. */
    bool loadPresetFromFile(const juce::String& presetName);

    /** Delete a preset file from disk. */
    bool deletePreset      (const juce::String& presetName) const;

    /** Returns all preset names (file stems) found in the preset directory. */
    juce::StringArray getAllPresetNames() const;

    /** Returns the full path to the preset directory (creates it if needed). */
    juce::File getPresetDirectory() const;

    //── Built-in factory presets ──────────────────────────────────────────────
    /**
     * Writes 8 built-in factory presets to disk if they don't already exist.
     * Should be called once on plugin initialisation.
     */
    void writeFactoryPresetsIfNeeded();

    //── Import / Export ───────────────────────────────────────────────────────
    /** Exports the current state as an XML file chosen by the user. */
    void exportPreset (juce::Component* parentComponent);

    /** Imports a preset XML file chosen by the user. */
    void importPreset (juce::Component* parentComponent);

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::File getPresetFile (const juce::String& presetName) const;

    //── Factory preset XML data ───────────────────────────────────────────────
    struct FactoryPreset { juce::String name; juce::String description; };
    static const std::array<FactoryPreset, 8> FACTORY_PRESETS;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
