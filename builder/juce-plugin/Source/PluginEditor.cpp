#include "PluginProcessor.h"
#include "PluginEditor.h"

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- Load embedded HTML from BinaryData ---
    // NOTE: We will set the correct BinaryData::xxxx name in Step 1C.
    juce::String html (BinaryData::html_to_vst_plugin_html, 
                       BinaryData::html_to_vst_plugin_htmlSize);

    webView.setOpaque (true);
    webView.loadHTML (html);      // This prevents “URL cannot be shown”
    addAndMakeVisible (webView);

    setSize (900, 650);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() {}

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
