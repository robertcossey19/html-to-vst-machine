#include "PluginEditor.h"

using namespace juce;

static String makeMissingUiHtml (const String& extra = {})
{
    return String (R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>UI Missing</title>
  <style>
    html,body{height:100%;margin:0;background:#1a202c;color:#e5e7eb;font-family:system-ui}
    .wrap{display:flex;align-items:center;justify-content:center;height:100%}
    .card{max-width:760px;padding:18px;border:1px solid #374151;border-radius:12px;background:rgba(0,0,0,.25)}
    code{white-space:pre-wrap;word-break:break-word}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h2>Embedded UI not found</h2>
      <p>The plugin built, but the HTML resource name didn’t match what the editor expects.</p>
      <p>Fix: confirm the embedded file name in BinaryData and add it to the candidate list.</p>
      <code>)HTML") + extra + R"HTML(</code>
    </div>
  </div>
</body>
</html>
)HTML");
}

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      webView() // IMPORTANT: default ctor, not { false }
{
    setOpaque (true);
    addAndMakeVisible (webView);

    // Give it a sane default size so it never starts at 0x0 (black screen)
    setSize (980, 560);

    loadUiFromBinaryData();

    // VU bridge / keepalive
    startTimerHz (30);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstAudioProcessorEditor::loadUiFromBinaryData()
{
    auto tryName = [&] (const char* name) -> bool
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (name, size))
        {
            if (size > 0)
            {
                const auto html = String::fromUTF8 ((const char*) data, size);
                // CRITICAL: setHtml renders HTML. goToURL on encoded HTML gives the %3C… box.
                webView.setHtml (html, URL ("about:blank"));
                uiLoaded = true;
                return true;
            }
        }
        return false;
    };

    // Try a bunch of likely names so we don't break when the generator renames.
    // Add your actual resource name here once we confirm it (see command below).
    const char* candidates[] =
    {
        "ui_html",
        "index_html",
        "index_htm",
        "main_html",
        "ui_min_html",
        "analogexact_html",
        "AnalogExact_html",
        "ANALOGEXACT_html"
    };

    for (auto* c : candidates)
        if (tryName (c))
            return;

    // If none matched, show a clear message (rendered, NOT URL-encoded)
    webView.setHtml (makeMissingUiHtml ("Tried names:\n- " + StringArray (candidates, (int) std::size (candidates)).joinIntoString ("\n- ")),
                     URL ("about:blank"));
    uiLoaded = false;
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    // Push VU into JS if the page defines window.setVUMeters(leftDb,rightDb) or similar.
    // Your processor stores dB RMS already.
    const auto lDb = processor.currentVUL.load();
    const auto rDb = processor.currentVUR.load();

    // Send dB, let JS map it however it wants.
    const auto js = "if (window.setVUMeters) window.setVUMeters(" + String (lDb, 3) + "," + String (rDb, 3) + ");";
    webView.executeJavascript (js);
}
