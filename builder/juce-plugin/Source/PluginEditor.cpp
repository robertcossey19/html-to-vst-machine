#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryData.h"

using namespace juce;

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Basic plugin window size (you can change this later)
    setResizable (true, true);
    setSize (1000, 600);

    // Make the WebBrowserComponent visible
    addAndMakeVisible (webView);

   #if JUCE_MAC
    // Don't try to open external URLs in the system browser
    webView.setAllowInsecureHTTPContent (false);
   #endif

    // === IMPORTANT PART ===
    // The HTML is currently URL-encoded (%3Chtml%3E...) inside BinaryData.
    // We must DECODE it before calling loadHTML, otherwise you get the text wall you pasted.

    String encodedHtml (BinaryData::html_to_vst_plugin_html,
                        BinaryData::html_to_vst_plugin_htmlSize);

    // Decode %3C, %20, etc. into real characters using JUCE's URL helper
    String decodedHtml;
    URL::removeEscapeChars (encodedHtml, decodedHtml);

    // Now feed the decoded HTML string to the internal browser
    // Base URL empty => all assets must be inline or https://-based
    webView.loadHTML (decodedHtml, URL());
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    // On some hosts, explicitly clearing the browser can help avoid crashes on shutdown
    webView.goToURL ("about:blank");
}

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
