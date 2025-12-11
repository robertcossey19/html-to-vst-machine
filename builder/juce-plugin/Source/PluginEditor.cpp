#include "PluginEditor.h"
#include "BinaryData.h"   // <-- gives us BinaryData::ampex_ui_html

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1100, 720);

    addAndMakeVisible (webView);
    webView.setBounds (getLocalBounds());

    // Load raw HTML from the embedded BinaryData (ampex_ui.html)
    juce::String html (BinaryData::ampex_ui_html,
                       BinaryData::ampex_ui_htmlSize);

    // Make sure script tags are treated as JavaScript
    html = html.replace ("<script", "<script type='text/javascript'");

    // JUCE WebBrowserComponent doesn’t have goToHTMLString in this JUCE,
    // so we use a data: URL instead.
    juce::String dataUrl = "data:text/html," + juce::URL::addEscapeChars (html, true);

    webView.goToURL (dataUrl);
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
