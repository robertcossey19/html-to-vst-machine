#pragma once
#include <JuceHeader.h>

class HtmlToVstPluginAudioProcessor;

class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::WebBrowserComponent webView;
    juce::String htmlData;

    HtmlToVstPluginAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
