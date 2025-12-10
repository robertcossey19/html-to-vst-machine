#include "PluginProcessor.h"
#include "PluginEditor.h"

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Embed your entire HTML interface here
    htmlData = R"html(
        <!-- PASTE YOUR FULL AMPEX HTML HERE -->
    )html";

    addAndMakeVisible(webView);

    webView.goToURL("data:text/html," + juce::URL::addEscapeChars(htmlData, true));

    setSize(900, 600);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor() {}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    webView.setBounds(getLocalBounds());
}
