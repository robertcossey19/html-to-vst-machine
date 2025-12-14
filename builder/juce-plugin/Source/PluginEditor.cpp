#include "PluginEditor.h"
#include "BinaryData.h"

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , processor (p)
   #if defined(JUCE_MAJOR_VERSION) && (JUCE_MAJOR_VERSION >= 7)
    , browserListener (p)
    , webView (juce::WebBrowserComponent::Options{}.withPageLoadListener (&browserListener))
   #endif
{
    setSize (980, 640);

    addAndMakeVisible (webView);
    loadHtmlUi();

    startTimerHz (30);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor() = default;

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
    pushMetersToUi();
}

void HtmlToVstPluginAudioProcessorEditor::loadHtmlUi()
{
    int size = 0;
    auto* data = BinaryData::getNamedResource ("index.html", size);

    if (data == nullptr || size <= 0)
    {
        webView.goToURL ("data:text/html,<html><body><h3>Missing index.html in BinaryData</h3></body></html>");
        return;
    }

    const juce::String html (reinterpret_cast<const char*> (data), size);
    const juce::String b64 = juce::Base64::toBase64 (html.toRawUTF8(), (size_t) html.getNumBytesAsUTF8());

    const juce::String url = "data:text/html;base64," + b64;
    webView.goToURL (url);
}

void HtmlToVstPluginAudioProcessorEditor::pushMetersToUi()
{
    const float l = processor.getOutputMeterL();
    const float r = processor.getOutputMeterR();

    juce::String js;
    js << "window.dispatchEvent(new CustomEvent('juce-meter', { detail: { l:"
       << juce::String (l, 6)
       << ", r:"
       << juce::String (r, 6)
       << " } }));";

    webView.executeJavascript (js);
}
