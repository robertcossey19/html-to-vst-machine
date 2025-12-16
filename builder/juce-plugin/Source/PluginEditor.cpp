#include "PluginEditor.h"

// If your BinaryData is in a namespace, keep this as-is. JUCE default is BinaryData::
#include "BinaryData.h"

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
    , webView (juce::WebBrowserComponent::Options{}
                  .withNativeIntegrationEnabled()
                  .withKeepPageLoadedWhenBrowserIsHidden())
{
    setSize (1200, 720);

    addAndMakeVisible (webView);

    // A stable temp folder for the web UI files
    uiTempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("HtmlToVstPlugin_UI");
    uiTempDir.createDirectory();

    uiIndexFile = uiTempDir.getChildFile ("index.html");

    loadUIFromBinaryData();

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

static juce::String fallbackHtml (const juce::String& msg)
{
    return "<!doctype html><html><body style='background:#111;color:#eee;font-family:sans-serif'>"
           "<h2>UI load failed</h2>"
           "<p>" + msg + "</p>"
           "<p>Fix: ensure <b>builder/juce-plugin/ui/index.html</b> exists and is embedded by <b>HtmlUIData</b> BinaryData.</p>"
           "</body></html>";
}

void HtmlToVstPluginAudioProcessorEditor::loadUIFromBinaryData()
{
    int dataSize = 0;
    const void* data = BinaryData::getNamedResource ("index.html", dataSize);

    if (data == nullptr || dataSize <= 0)
    {
        // Show a readable error page (not percent-encoded garbage)
        const auto html = fallbackHtml ("BinaryData did not contain index.html (or expected filename).");
        uiIndexFile.replaceWithText (html, false, false, "\n");
        webView.goToURL (juce::URL (uiIndexFile).toString (true));
        return;
    }

    const juce::String html = juce::String::fromUTF8 (static_cast<const char*> (data), dataSize);

    // Write to a real file and load it as file:// URL (most reliable in plugins)
    const bool ok = uiIndexFile.replaceWithText (html, false, false, "\n");

    if (! ok)
    {
        const auto fb = fallbackHtml ("Failed to write temp UI file: " + uiIndexFile.getFullPathName());
        uiIndexFile.replaceWithText (fb, false, false, "\n");
    }

    webView.goToURL (juce::URL (uiIndexFile).toString (true));
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    // If you later wire VU meters, you’ll call JS like:
    // webView.evaluateJavascript ("window.setVUMeters(0.2, 0.18);");
    //
    // For now: do nothing (prevents random “gain bump” confusion from JS spam).
}
