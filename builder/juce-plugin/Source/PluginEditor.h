#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//===============================================================
class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;

    // WebView for HTML UI
    juce::WebBrowserComponent webView;

    // VU meter state we update from audio processor
    float vuLeft  = 0.0f;
    float vuRight = 0.0f;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
