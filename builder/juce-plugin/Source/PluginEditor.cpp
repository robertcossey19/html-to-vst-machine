#include "PluginEditor.h"
#include "PluginProcessor.h"

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1100, 720);

    addAndMakeVisible(webView);
    webView.setBounds(getLocalBounds());

    // LOAD RAW HTML DIRECTLY FROM BINARYDATA — NOT URL ENCODED
    juce::String html = juce::String::fromUTF8(
        BinaryData::ampex_ui_html,
        BinaryData::ampex_ui_htmlSize
    );

    // Convert <script> tags so JUCE runs them properly
    html = html.replace("<script", "<script type='text/javascript'");

    // LOAD DIRECTLY
    webView.goToHTMLString(html);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() {}

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds(getLocalBounds());
}
