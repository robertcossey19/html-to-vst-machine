#include "PluginEditor.h"

//==============================================================================
static juce::String makeErrorHtml (const juce::String& msg)
{
    return "<!doctype html><html><body style='background:#111;color:#eee;font-family:sans-serif'>"
           "<h2>UI load failed</h2><p>" + msg + "</p>"
           "<p>Fix: ensure <b>builder/juce-plugin/ui/index.html</b> exists and is embedded into HtmlUIData (BinaryData).</p>"
           "</body></html>";
}

static void navigateToHtml (juce::WebBrowserComponent& web, const juce::String& html)
{
    // WebBrowserComponent doesn't have setPage in modern JUCE. Use a data: URL.
    const auto escaped = juce::URL::addEscapeChars (html, true);
    web.goToURL (juce::URL ("data:text/html;charset=utf-8," + escaped));
}

//==============================================================================
HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p)
    , processor (p)
    , webView (juce::WebBrowserComponent::Options{}
                 .withNativeIntegrationEnabled()
                 .withKeepPageLoadedWhenBrowserIsHidden())
{
    setSize (1100, 700);
    addAndMakeVisible (webView);

    loadUiFromBinaryData();

    // Update VU meters ~30 fps
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

void HtmlToVstPluginAudioProcessorEditor::loadUiFromBinaryData()
{
    int dataSize = 0;

    // This name MUST match what JUCE BinaryData generated from the filename.
    // If the file is ui/index.html, the resource name is typically "index.html".
    const char* data = BinaryData::getNamedResource ("index.html", dataSize);

    if (data == nullptr || dataSize <= 0)
    {
        // Try a couple of common alternates in case the build system renamed it.
        data = BinaryData::getNamedResource ("ui/index.html", dataSize);
    }

    if (data == nullptr || dataSize <= 0)
    {
        navigateToHtml (webView, makeErrorHtml ("BinaryData did not contain index.html (or expected filename)."));
        return;
    }

    const juce::String html = juce::String::fromUTF8 (data, dataSize);
    navigateToHtml (webView, html);
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    // Feed meter data into the UI if the page defined window.setVUMeters(l,r)
    const float inM  = processor.getInputMeter();
    const float outM = processor.getOutputMeter();

    const juce::String js = "if (window.setVUMeters) window.setVUMeters("
                            + juce::String (inM, 6) + ", "
                            + juce::String (outM, 6) + ");";

    // JUCE uses evaluateJavascript (NOT executeJavascript)
    webView.evaluateJavascript (js);
}
