#include "PluginProcessor.h"
#include "PluginEditor.h"

// If BinaryData is in a different include path, this will still work once HtmlUIData is built.
#if __has_include("BinaryData.h")
  #include "BinaryData.h"
#elif __has_include("../JuceLibraryCode/BinaryData.h")
  #include "../JuceLibraryCode/BinaryData.h"
#endif

static juce::String makeDataUrlFromHtml (const juce::String& html)
{
    // IMPORTANT: WebBrowserComponent::goToURL expects a String URL, not juce::URL.
    // Also: we must URL-escape the HTML.
    const auto escaped = juce::URL::addEscapeChars (html, true);
    return "data:text/html;charset=utf-8," + escaped;
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
    , web (juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden())
{
    addAndMakeVisible (web);
    setSize (1100, 700);

    loadUiFromBinaryData();
    startTimerHz (30); // UI-to-DSP polling, meters, etc.
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
    web.setBounds (getLocalBounds());
}

static bool binaryDataHasResource (const char* name, int& sizeOut)
{
#if defined(BinaryData::getNamedResource)
    if (auto* data = BinaryData::getNamedResource (name, sizeOut))
        return data != nullptr && sizeOut > 0;
#endif
    sizeOut = 0;
    return false;
}

static juce::String loadFirstHtmlFromBinaryDataOrEmpty()
{
#if defined(BinaryData::namedResourceList) && defined(BinaryData::namedResourceListSize)
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const char* nm = BinaryData::namedResourceList[i];
        if (nm == nullptr) continue;

        const juce::String name (nm);
        if (name.endsWithIgnoreCase (".html"))
        {
            int sz = 0;
            if (auto* data = BinaryData::getNamedResource (nm, sz))
                return juce::String::fromUTF8 (static_cast<const char*> (data), sz);
        }
    }
#endif
    return {};
}

void HtmlToVstPluginAudioProcessorEditor::loadUiFromBinaryData()
{
    juce::String html;

    // Preferred names (common cases)
    {
        int sz = 0;
        if (binaryDataHasResource ("index.html", sz))
        {
            auto* data = BinaryData::getNamedResource ("index.html", sz);
            html = juce::String::fromUTF8 ((const char*) data, sz);
        }
        else if (binaryDataHasResource ("ampex_ui.html", sz))
        {
            auto* data = BinaryData::getNamedResource ("ampex_ui.html", sz);
            html = juce::String::fromUTF8 ((const char*) data, sz);
        }
        else
        {
            // Fallback: load *any* embedded .html so we stop hard-failing on filenames.
            html = loadFirstHtmlFromBinaryDataOrEmpty();
        }
    }

    if (html.isEmpty())
    {
        const juce::String err =
            "<!DOCTYPE html><html><body style='background:#111;color:#eee;font-family:sans-serif'>"
            "<h2>UI load failed</h2>"
            "<p>BinaryData did not contain any .html resource.</p>"
            "<p>Fix: ensure HtmlUIData embeds your UI html (index.html or ampex_ui.html).</p>"
            "</body></html>";

        web.goToURL (makeDataUrlFromHtml (err));
        return;
    }

    web.goToURL (makeDataUrlFromHtml (html));
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    // Placeholder: later we’ll push meter values into JS like:
    // web.executeJavascript ("window.setVUMeters(" + juce::String(l) + "," + juce::String(r) + ");");
}
