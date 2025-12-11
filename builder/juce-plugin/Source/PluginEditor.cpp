#include "PluginEditor.h"

//==============================================================================

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      webView() // default constructor only – no Options argument
{
    addAndMakeVisible (webView);

    // Load embedded HTML from BinaryData into data: URL
    {
        juce::String html = juce::String::fromUTF8 (BinaryData::ampex_ui_html,
                                                    BinaryData::ampex_ui_htmlSize);

        juce::String url = "data:text/html;charset=utf-8," +
                           juce::URL::addEscapeChars (html, true);

        webView.goToURL (url);
    }

    setOpaque (true);
    setSize (980, 640);

    // Poll VU meters ~30 times per second
    startTimerHz (30);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

//==============================================================================

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    // Pull VU from processor (dB)
    const float vuL = processor.getCurrentVUL();
    const float vuR = processor.getCurrentVUR();

    // Normalise to 0–1 for UI:
    // -40 dB => 0.0, +6 dB => 1.0
    auto norm = [] (float db)
    {
        const float clamped = juce::jlimit (-40.0f, 6.0f, db);
        return (clamped + 40.0f) / 46.0f;
    };

    const float nL = norm (vuL);
    const float nR = norm (vuR);

    // Call JS updateVU(leftNormalized, rightNormalized) if it exists
    juce::String js;
    js << "if(window.updateVU){updateVU("
       << juce::String (nL, 3) << ","
       << juce::String (nR, 3) << ");}";

    webView.evaluateJavascript (js);
}
