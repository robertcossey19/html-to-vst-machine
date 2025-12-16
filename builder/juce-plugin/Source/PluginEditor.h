#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstPluginAudioProcessor& audioProcessor;

    struct Browser : public juce::WebBrowserComponent
    {
        using juce::WebBrowserComponent::WebBrowserComponent;
    };

    Browser webView;

    juce::File uiTempDir;
    juce::File uiIndexFile;

    void timerCallback() override;

    void loadUIFromBinaryData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
