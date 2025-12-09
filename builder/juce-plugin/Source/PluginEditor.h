#pragma once

#include <JuceHeader.h>

class HtmlToVstAudioProcessor;

// Editor that shows the embedded HTML UI in a WebView
class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
