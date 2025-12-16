#include "PluginEditor.h"

// If BinaryData naming changes, these are common candidates:
static const char* kUiCandidates[] =
{
    "index.html",
    "Index.html",
    "ui.html",
    "UI.html"
};

static juce::String makeFallbackHtml (const juce::String& reason)
{
    return "<!doctype html><html><body style='background:#111;color:#eee;font-family:sans-serif'>"
           "<h2>UI load failed</h2>"
           "<p>" + reason + "</p>"
           "<p>Fix: ensure your UI build embeds <b>index.html</b> into HtmlUIData (BinaryData).</p>"
           "</body></html>";
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
    , webView (juce::WebBrowserComponent::Options{}
                 .withNativeIntegrationEnabled()
                 .withKeepPageLoadedWhenBrowserIsHidden (true))
{
    setOpaque (true);
    addAndMakeVisible (webView);

    setSize (1100, 680);

    loadUiFromBinaryData();

    // If you later hook VU meters, this timer is where you’d push values into JS.
    startTimerHz (30);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    stopTimer();

    // Best effort cleanup of temp file
    if (uiTempHtmlFile.existsAsFile())
        uiTempHtmlFile.deleteFile();
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
    // NOTE:
    // You said meters/toggles aren’t hooked yet. This is where we’ll push metering + state.
    // For now we do nothing unless UI loaded.
    if (! uiLoaded)
        return;

    // Example (when you add getters in the processor):
    // auto [l, r] = audioProcessor.getVuRms01();
    // webView.runJS ("window.setVUMeters && window.setVUMeters(" + juce::String(l) + "," + juce::String(r) + ");");
}

void HtmlToVstPluginAudioProcessorEditor::loadUiFromBinaryData()
{
    int dataSize = 0;
    const void* dataPtr = nullptr;
    juce::String pickedName;

    // Try candidate names
    for (auto* name : kUiCandidates)
    {
        dataSize = 0;
        dataPtr  = BinaryData::getNamedResource (name, dataSize);
        if (dataPtr != nullptr && dataSize > 0)
        {
            pickedName = name;
            break;
        }
    }

    juce::String html;

    if (dataPtr == nullptr || dataSize <= 0)
    {
        html = makeFallbackHtml ("BinaryData did not contain index.html (or expected filename).");
    }
    else
    {
        html = juce::String::fromUTF8 (static_cast<const char*> (dataPtr), dataSize);

        // Guard against “empty” or junk
        if (html.trim().isEmpty())
            html = makeFallbackHtml ("Embedded UI file was found (" + pickedName + ") but was empty.");
    }

    // Write to temp file and load as file://
    uiTempHtmlFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getNonexistentChildFile ("HtmlToVstUI", ".html", false);

    const auto ok = uiTempHtmlFile.replaceWithText (html);

    if (! ok)
    {
        // If we can’t write, fall back to a data URL (still better than showing the escaped string)
        const auto dataUrl = "data:text/html;charset=utf-8," + juce::URL::addEscapeChars (html, true);
        webView.goToURL (dataUrl);
        uiLoaded = true;
        return;
    }

    const auto url = juce::URL (uiTempHtmlFile).toString (true);
    webView.goToURL (url);
    uiLoaded = true;
}
