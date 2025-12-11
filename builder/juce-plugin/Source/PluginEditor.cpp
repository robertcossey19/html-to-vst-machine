#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (900, 600);

    auto html = juce::String::fromUTF8 (BinaryData::ampex_ui_html, 
                                        BinaryData::ampex_ui_htmlSize);

    webView.reset (new juce::WebBrowserComponent());
    webView->goToURL ("data:text/html," + juce::URL::addEscapeChars (html, true));
    addAndMakeVisible (*webView);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
}

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    if (webView.get() != nullptr)
        webView->setBounds (getLocalBounds());
}
