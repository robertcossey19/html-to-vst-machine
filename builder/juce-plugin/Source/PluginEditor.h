#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

// Forward declaration – defined in PluginProcessor.h
class HtmlToVstAudioProcessor;

//==============================================================================
// Editor class: shows HTML UI via WebBrowserComponent using embedded HTML
//==============================================================================

class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;

    juce::WebBrowserComponent webView;
    juce::String              htmlData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
