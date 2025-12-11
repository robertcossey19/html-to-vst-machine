#include "PluginEditor.h"

// If BinaryData is generated in your build (HtmlUIData target), include it.
// Your project’s binary data header is typically available as "BinaryData.h".
#include "BinaryData.h"

using namespace juce;

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setOpaque (true);

    // Give it a real size so the host doesn’t show a tiny white box
    setSize (1100, 720);

    addAndMakeVisible (webView);

    // Load UI once the component exists
    loadUI();

    // If you later want to push VU updates into JS, timer is ready
    startTimerHz (30);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    // If the web view ever fails, this background prevents “black”
    g.fillAll (Colour (0xFF1A202C));
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    // Optional: if you have meter values exposed, push them to JS here.
    // Leaving empty is fine and avoids any “white text” fallback.
    //
    // Example if you add getters later:
    // webView.executeJavascript ("window.setVUMeters(" + String(linL) + "," + String(linR) + ");");
}

String HtmlToVstAudioProcessorEditor::getEmbeddedHtml() const
{
    // BinaryData names depend on the original filename/path.
    // Try common ones; you can add more if your resource name differs.
    struct Candidate { const char* name; };
    static const Candidate candidates[] = {
        {"index_html"},
        {"ui_html"},
        {"app_html"},
        {"main_html"},
        {"analogexact_html"},
        {"atr102_html"},
        {"ui_index_html"},
    };

    for (auto& c : candidates)
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (c.name, size))
        {
            if (size > 0)
                return String::fromUTF8 (data, size);
        }
    }

    // Fallback: never show a blank/black view if BinaryData name changes
    return R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>UI Missing</title>
  <style>
    html,body{height:100%;margin:0;background:#1a202c;color:#e5e7eb;font-family:system-ui;}
    .wrap{display:flex;align-items:center;justify-content:center;height:100%;}
    .card{max-width:720px;padding:18px;border:1px solid #374151;border-radius:12px;background:rgba(0,0,0,.25)}
    code{white-space:pre-wrap;word-break:break-word;}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h2>Embedded UI not found</h2>
      <p>The plugin built, but the HTML resource name didn’t match what the editor expects.</p>
      <p>Fix: confirm the embedded file name in BinaryData and add it to the candidate list.</p>
    </div>
  </div>
</body>
</html>
)HTML";
}

void HtmlToVstAudioProcessorEditor::loadUI()
{
    if (loadedOnce)
        return;

    const auto html = getEmbeddedHtml();

    // CRITICAL FIX:
    // Do NOT call goToURL() with "%3C%21DOCTYPE..." by itself.
    // Use a proper data URL so the browser interprets it as HTML.
    const auto escaped = URL::addEscapeChars (html, true);
    const auto dataUrl = String ("data:text/html;charset=utf-8,") + escaped;

    webView.goToURL (dataUrl);

    loadedOnce = true;
}
