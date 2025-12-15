#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    This editor restores the working embedded HTML UI (the "AnalogExact ATR-102" UI)
    that existed in the v7 build, but loads it directly from an in-memory HTML string.

    Why this fixes your issue:
    - Your current build is showing the fallback "UI Placeholder" screen, which means
      the real WebView UI is not being created/loaded.
    - This file forces the editor to always create a WebBrowserComponent and load the
      working HTML UI immediately.
*/
class HtmlToVstAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& audioProcessor;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
