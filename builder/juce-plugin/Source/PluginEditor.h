#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HtmlToVstAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HtmlToVstAudioProcessor& processor;

    // Web UI
    juce::WebBrowserComponent webView;
    juce::File                tempHtmlFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
