#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
// Simple HTML UI editor: shows the embedded ampex_ui.html in a WebBrowserComponent.
// No parameter or VU wiring yet – this is just to ensure the HTML renders correctly.
//
class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
