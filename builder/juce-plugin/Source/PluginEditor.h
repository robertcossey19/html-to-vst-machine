#pragma once

#include <juce_gui_extra/juce_gui_extra.h>   // for juce::Component, juce::WebBrowserComponent, etc.
#include "PluginProcessor.h"                 // HtmlToVstAudioProcessor declaration

class HtmlToVstAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    // JUCE Editor overrides
    void paint   (juce::Graphics& g) override;
    void resized() override;

private:
    // Reference back to the processor
    HtmlToVstAudioProcessor& audioProcessor;

    // Our embedded UI is drawn with a JUCE WebBrowserComponent
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITHOUT_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
