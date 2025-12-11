#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
// HTML to VST Plugin editor — hosts the ATR-102 UI HTML in a WebBrowserComponent
//==============================================================================
class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;

    // false = keep the page loaded even if the editor is hidden
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
