#include "PluginEditor.h"

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      web (juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled())
{
    setSize (1100, 700);
    addAndMakeVisible (web);

    // 🔥 LOAD EMBEDDED UI 🔥
    auto html = juce::String::fromUTF8 (
        BinaryData::ampex_ui_html,
        BinaryData::ampex_ui_htmlSize
    );

    web.setPage (html, {});
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() {}

void HtmlToVstAudioProcessorEditor::resized()
{
    web.setBounds (getLocalBounds());
}
