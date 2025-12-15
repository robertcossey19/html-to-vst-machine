/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

HtmlToVstPluginAudioProcessorEditor::InterceptingBrowser::InterceptingBrowser (HtmlToVstPluginAudioProcessor& p)
    : juce::WebBrowserComponent(),
      processor (p)
{
}

bool HtmlToVstPluginAudioProcessorEditor::InterceptingBrowser::pageAboutToLoad (const juce::String& newURL)
{
    // Intercept messages from the HTML UI.
    // Example: juce://set?param=gain&value=0.5
    if (newURL.startsWithIgnoreCase ("juce:"))
    {
        processor.handleUiMessageUrl (newURL);
        return false; // prevent navigation
    }

    return true;
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      webView (p)
{
    setOpaque (true);
    setSize (820, 520);

    addAndMakeVisible (webView);

    loadHtmlUi();
    startTimerHz (30);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    if (! uiLoaded)
    {
        if (uiRetryCount++ < 8)
            loadHtmlUi();

        return;
    }

    pushMetersToUi();
}

void HtmlToVstPluginAudioProcessorEditor::loadHtmlUi()
{
    // HtmlUIData target should generate BinaryData::index_html / index_htmlSize
    const auto* data = BinaryData::index_html;
    const auto size  = static_cast<size_t> (BinaryData::index_htmlSize);

    if (data == nullptr || size == 0)
        return;

    const auto htmlBase64 = juce::Base64::toBase64 (data, size);
    const juce::String url = "data:text/html;base64," + htmlBase64;

    webView.goToURL (url);
    uiLoaded = true;
}

void HtmlToVstPluginAudioProcessorEditor::pushMetersToUi()
{
    const auto l = processor.getOutputMeterL();
    const auto r = processor.getOutputMeterR();

    const juce::String js =
        "if (window.juce && window.juce.onMeters) {"
        "  window.juce.onMeters({ l: " + juce::String (l, 6) + ", r: " + juce::String (r, 6) + " });"
        "}";

    // Your JUCE has evaluateJavascript (not executeJavascript)
    webView.evaluateJavascript (js);
}
