#include "PluginEditor.h"

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      web (juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled())
{
    setSize (1100, 700);
    addAndMakeVisible (web);

    int dataSize = 0;
    const void* data = BinaryData::getNamedResource ("ampex_ui.html", dataSize);

    if (data == nullptr || dataSize <= 0)
    {
        // If this shows up, BinaryData embedding is still wrong (CMake/UI path)
        const juce::String err =
            "<html><body style='background:#111;color:#eee;font-family:sans-serif'>"
            "<h2>UI load failed</h2>"
            "<p>BinaryData did not contain <b>ampex_ui.html</b>.</p>"
            "<p>Fix: ensure HtmlUIData embeds Assets/UI/ampex_ui.html.</p>"
            "</body></html>";

        web.setPage (err, {});
        return;
    }

    const juce::String html = juce::String::fromUTF8 ((const char*) data, dataSize);
    web.setPage (html, {});
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() {}

void HtmlToVstAudioProcessorEditor::resized()
{
    web.setBounds (getLocalBounds());
}
