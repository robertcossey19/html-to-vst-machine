#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>

class HtmlToVstPluginAudioProcessor;

//==============================================================================
// This is the editor (GUI) for the plugin. It hosts a WebBrowserComponent that
// renders the ATR-102 HTML UI from BinaryData.
class HtmlToVstPluginAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HtmlToVstPluginAudioProcessor& audioProcessor;

    // Web view that will load the HTML UI bundled via BinaryData.
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
