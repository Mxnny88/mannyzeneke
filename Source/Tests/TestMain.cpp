//==============================================================================
// ArpSequencer Unit Test Runner
// Includes all test headers and runs via JUCE UnitTestRunner.
//==============================================================================
#include <JuceHeader.h>

// Include test suites (static instances auto-register)
#include "ArpTests.h"
#include "SequencerTests.h"

int main (int /*argc*/, char** /*argv*/)
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    int totalFailed = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        if (result != nullptr)
        {
            juce::Logger::writeToLog("Test: " + result->unitTestName
                + " | Passes: " + juce::String(result->passes)
                + " | Failures: " + juce::String(result->failures));
            totalFailed += result->failures;
        }
    }

    juce::Logger::writeToLog("=== Total failures: " + juce::String(totalFailed) + " ===");
    return totalFailed > 0 ? 1 : 0;
}
