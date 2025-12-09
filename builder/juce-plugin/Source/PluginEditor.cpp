#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

// BinaryData comes from the CMake juce_add_binary_data(HtmlToVst_UI ...)
extern const unsigned char*  BinaryData_ampex_ui_html;
extern const int             BinaryData_ampex_ui_htmlSize;

// Helper: get HTML from BinaryData (JUCE generates BinaryData namespace,
// but declarations above are safe; if needed we can switch to BinaryData::ampex_ui_html)
static juce::String getEmbeddedHtml()
{
    // In normal JUCE BinaryData this would be:
    // return juce::String::fromUTF8 (BinaryData::ampex_ui_html, BinaryData::ampex_ui_htmlSize);
    // but since we declared externs, use them directly if the names differ.
    // If it fails to link, we will adjust names to BinaryData::ampex_ui_html.
    return juce::String::fromUTF8 (BinaryData_ampex_ui_html, BinaryData_ampex_ui_htmlSize);
}

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      webView (false) // don't use popup
{
    setSize (900, 600);

    // Write embedded HTML to temp file and load it in the WebView
    auto html = getEmbeddedHtml();

    juce::File tempFile = juce::File::createTempFile ("ampex_ui.html");
    tempFile.replaceWithText (html);

    addAndMakeVisible (webView);
    webView.goToURL (tempFile.getFullPathName());
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() = default;

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
