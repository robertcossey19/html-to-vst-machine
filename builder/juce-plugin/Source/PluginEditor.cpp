#include "PluginEditor.h"

//==============================================================================

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setOpaque (true);
    setSize (1000, 600); // plenty of room for the HTML UI

    // JUCE 8 WebBrowserComponent: create with default options
    juce::WebBrowserComponent::Options options;
    webView = std::make_unique<juce::WebBrowserComponent> (options);

    addAndMakeVisible (*webView);

   #if JUCE_MAC
    webView->setOpaque (false);
   #endif

    // -------------------------------------------------------------------------
    // IMPORTANT PART:
    //
    // We load the embedded HTML from BinaryData and give it to the webview
    // as a proper HTML document. NO URL-encoding here. The white box full of
    // %3C!DOCTYPE... was because the HTML was being URL-encoded and then shown
    // as plain text.
    // -------------------------------------------------------------------------

    juce::String html = juce::String::fromUTF8 (BinaryData::ampex_ui_html,
                                                BinaryData::ampex_ui_htmlSize);

    // Use a data: URI with the RAW HTML (no URL::addEscapeChars).
    // Most WebKit-based views happily treat this as HTML.
    juce::String dataUri = "data:text/html;charset=utf-8," + html;

    // If some hosts are picky about data: URIs, you could later move to a
    // resource-provider-based WebBrowserComponent, but this keeps changes
    // minimal and close to what you already had.
    webView->goToURL (dataUri);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    // Let webView be destroyed automatically
}

//==============================================================================

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background behind the webview – usually never visible.
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
