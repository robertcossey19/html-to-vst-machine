#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// Simple editor that shows the embedded HTML UI using WebBrowserComponent.
// HTML is loaded from JUCE BinaryData and written to a temp file.
// The WebBrowser then navigates to that file URL so Cubase renders it as real HTML.

class HtmlToVstAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor&);
    ~HtmlToVstAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    HtmlToVstAudioProcessor& processor;
    juce::WebBrowserComponent webView;

    void loadEmbeddedHtml();   // load HTML from BinaryData and show it

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstAudioProcessorEditor)
};
