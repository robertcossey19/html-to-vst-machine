#include "PluginEditor.h"
#include "BinaryData.h"  // brings in BinaryData::ampex_ui_html etc.

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      webView (juce::WebBrowserComponent::Options{})
{
    setOpaque (true);

    // Size can be whatever fits your UI nicely
    setSize (1200, 700);

    addAndMakeVisible (webView);

    // Load embedded HTML (ampex_ui.html) from BinaryData
    juce::String html = juce::String::fromUTF8 (BinaryData::ampex_ui_html,
                                                BinaryData::ampex_ui_htmlSize);

    // Use a data: URL so we don't have to hit disk or the network
    juce::String url = "data:text/html;charset=utf-8," + juce::URL::addEscapeChars (html, true);
    webView.goToURL (url);

    // Drive VU updates ~30 fps
    startTimerHz (30);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    const float vuL = processor.getCurrentVUL();
    const float vuR = processor.getCurrentVUR();

    auto toDb = [] (float x) -> float
    {
        if (x <= 0.0f)
            return -90.0f;

        return 20.0f * std::log10 (x);
    };

    const float dbL = toDb (vuL);
    const float dbR = toDb (vuR);

    // JS hook: your ampex_ui.html should define:
    //   window.setVuMeters = function(dbL, dbR) { ... }
    const juce::String js = juce::String::formatted (
        "if (window.setVuMeters) window.setVuMeters(%f, %f);",
        (double) dbL,
        (double) dbR);

    webView.evaluateJavascript (js, nullptr);
}
