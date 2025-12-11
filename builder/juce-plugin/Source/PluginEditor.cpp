#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

//==============================================================================
HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    // Size for your HTML UI
    setSize (980, 600);

    addAndMakeVisible (webView);

    // Load embedded HTML from BinaryData as a data: URL
    const auto html = String::fromUTF8 (BinaryData::ampex_ui_html,
                                        BinaryData::ampex_ui_htmlSize);

    const auto dataUrl = "data:text/html;charset=utf-8," + URL::addEscapeChars (html, true);
    webView.goToURL (dataUrl);

    // Pump VU data into JS
    startTimerHz (30); // ~30 fps
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    const float l = processor.getCurrentVUL();
    const float r = processor.getCurrentVUR();

    // Call window.setVUMeters(left, right) inside ampex_ui.html
    String js;
    js << "if (window.setVUMeters) window.setVUMeters("
       << String (l, 6) << ","
       << String (r, 6) << ");";

    webView.evaluateJavascript (js, nullptr);
}
