#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace juce;

//===============================================================
HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Load embedded HTML UI:
    webView.setBrowserType (WebBrowserComponent::webViewType::webView2);

    String html = String::fromUTF8 (BinaryData::ampex_ui_html,
                                    BinaryData::ampex_ui_htmlSize);

    webView.goToString (html);
    addAndMakeVisible (webView);

    // Paint entire UI background, so make editor larger
    setSize (1400, 900);

    // Start VU meter polling timer
    startTimerHz (30);  // ~33ms refresh
}

//===============================================================
HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
}

//===============================================================
void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

//===============================================================
void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

//===============================================================
void HtmlToVstAudioProcessorEditor::timerCallback()
{
    // Pull VU values from processor
    vuLeft  = processor.currentVUL;
    vuRight = processor.currentVUR;

    // Expose them to HTML via JS:
    String js = "window.setVUMeters("
                + String(vuLeft) + ","
                + String(vuRight) + ");";

    webView.executeJavascript (js);
}
