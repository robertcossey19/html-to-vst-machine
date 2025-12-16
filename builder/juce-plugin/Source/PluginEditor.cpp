#include "PluginEditor.h"
#include "PluginProcessor.h"

// BinaryData.h comes from the HtmlUIData target (juce_add_binary_data)
#if __has_include("BinaryData.h")
  #include "BinaryData.h"
#elif __has_include(<BinaryData.h>)
  #include <BinaryData.h>
#endif

static juce::String loadEmbeddedHtmlOrError()
{
   #if JUCE_TARGET_HAS_BINARY_DATA
    int size = 0;

    // Try the repo’s current embedded UI name first, then the old one
    const char* const candidates[] = { "ampex_ui.html", "index.html" };

    for (auto* name : candidates)
    {
        if (auto* data = BinaryData::getNamedResource (name, size))
        {
            // Treat as UTF-8 HTML
            return juce::String::fromUTF8 ((const char*) data, size);
        }
    }
   #endif

    // Fallback error page
    return R"HTML(
<!doctype html>
<html>
  <head><meta charset="utf-8"><title>UI load failed</title></head>
  <body style="font-family:system-ui; padding:16px; background:#111; color:#eee;">
    <h2>UI load failed</h2>
    <p>BinaryData did not contain <code>ampex_ui.html</code> (or <code>index.html</code>).</p>
    <p>Fix: ensure your UI build embeds one of those filenames into HtmlUIData (BinaryData).</p>
  </body>
</html>
)HTML";
}

static void navigateWebToHtml (juce::WebBrowserComponent& web, const juce::String& html)
{
    // Escape for data: URL
    const auto escaped = juce::URL::addEscapeChars (html, true);

    // IMPORTANT: goToURL takes a String, NOT juce::URL
    web.goToURL ("data:text/html;charset=utf-8," + escaped);
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1100, 700);

    addAndMakeVisible (web);

    // Load embedded UI
    navigateWebToHtml (web, loadEmbeddedHtmlOrError());

    startTimerHz (30);
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    web.setBounds (getLocalBounds());
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    // You can push meters/params to the UI here later if needed.
    // Keeping empty avoids breaking builds while you stabilize UI loading.
}
