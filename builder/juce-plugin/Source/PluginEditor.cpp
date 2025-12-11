#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryData.h" // generated BinaryData (contains ampex_ui.html)

using namespace juce;

//==============================================================================

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      webView (false) // don't allow popups
{
    setOpaque (true);

    // Web UI fills entire editor
    addAndMakeVisible (webView);

    // Load the embedded HTML (ampex_ui.html with power/load/export removed)
    loadEmbeddedHTML();

    // Size: you can tweak this if you want a different default window
    setSize (1024, 540);

    // Poll the processor’s VU meters ~30 FPS
    startTimerHz (30);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

//==============================================================================

void HtmlToVstAudioProcessorEditor::loadEmbeddedHTML()
{
    // ampex_ui.html is embedded as BinaryData::ampex_ui_html
    const String html = String::fromUTF8 (BinaryData::ampex_ui_html,
                                          BinaryData::ampex_ui_htmlSize);

    // Feed it to the WebBrowserComponent via a data: URL
    const String dataUrl = "data:text/html;charset=utf-8," + URL::addEscapeChars (html, true);

    webView.goToURL (dataUrl);
}

// Timer: push VU data into the HTML DOM via JS
void HtmlToVstAudioProcessorEditor::timerCallback()
{
    const float vuL = processor.getCurrentVUL();
    const float vuR = processor.getCurrentVUR();

    // Don’t spam JS if nothing really changed
    const float deltaL = std::abs (vuL - lastSentVUL);
    const float deltaR = std::abs (vuR - lastSentVUR);

    if (deltaL < 0.5f && deltaR < 0.5f)
        return;

    lastSentVUL = vuL;
    lastSentVUR = vuR;

    sendVuToWebView (vuL, vuR);
}

void HtmlToVstAudioProcessorEditor::sendVuToWebView (float vuLeftDb, float vuRightDb)
{
    // Same angle mapping as the WebAudio version:
    // angle = clamp( (dB + 50)*2.25 - 70, -70, +20 )
    const auto toAngle = [] (float db)
    {
        const float angle = (db + 50.0f) * 2.25f - 70.0f;
        return jlimit (-70.0f, 20.0f, angle);
    };

    const float angleL = toAngle (vuLeftDb);
    const float angleR = toAngle (vuRightDb);

    String js;
    js  << "(()=>{"
        << "const nl=document.getElementById('vuL');"
        << "const nr=document.getElementById('vuR');"
        << "if(!nl||!nr)return;"
        << "nl.style.transform='rotate(" << angleL << "deg)';"
        << "nr.style.transform='rotate(" << angleR << "deg)';"
        << "})();";

    // JUCE 7: evaluateJavascript(script, callback = nullptr)
    webView.evaluateJavascript (js);
}
