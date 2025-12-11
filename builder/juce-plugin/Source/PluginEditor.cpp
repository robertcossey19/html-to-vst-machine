#include "PluginEditor.h"

using namespace juce;

static String urlDecodeToString (const String& in)
{
    // Decode %XX and + into UTF-8 string
    MemoryOutputStream out;

    auto s = in.toRawUTF8();
    for (size_t i = 0; s[i] != 0; ++i)
    {
        const auto c = s[i];

        if (c == '+')
        {
            out.writeByte ((char) ' ');
            continue;
        }

        if (c == '%' && s[i + 1] && s[i + 2])
        {
            auto hexVal = [] (char h) -> int
            {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return 10 + (h - 'a');
                if (h >= 'A' && h <= 'F') return 10 + (h - 'A');
                return -1;
            };

            const int hi = hexVal (s[i + 1]);
            const int lo = hexVal (s[i + 2]);

            if (hi >= 0 && lo >= 0)
            {
                out.writeByte ((char) ((hi << 4) | lo));
                i += 2;
                continue;
            }
        }

        out.writeByte ((char) c);
    }

    return out.toString();
}

static bool looksLikeHtml (const String& text)
{
    auto t = text.trimStart();

    // If it's URL-encoded HTML, it often starts with %3C%21DOCTYPE or %3Chtml
    if (t.startsWithIgnoreCase ("%3C"))
        return true;

    return t.startsWithIgnoreCase ("<!doctype")
        || t.startsWithIgnoreCase ("<html")
        || t.containsIgnoreCase ("</html>");
}

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
    .card{max-width:820px;padding:18px;border:1px solid #374151;border-radius:12px;background:rgba(0,0,0,.25)}
    code{white-space:pre-wrap;word-break:break-word}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h2>Embedded UI not found</h2>
      <p>The plugin built, but no embedded BinaryData entry looked like HTML.</p>
      <p>This usually means the UI file wasn’t embedded into HtmlUIData (or it got excluded).</p>
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
      webView() // JUCE 7+ requires default ctor / Options, NOT bool
{
    setOpaque (true);
    addAndMakeVisible (webView);

    // Never start at 0x0 (black/blank)
    setSize (980, 560);

    loadUiFromBinaryData();

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
    // 1) Try known BinaryData list first (if present in your JUCE build)
    StringArray htmlishNames;

   #if defined (JUCE_TARGET_HAS_BINARY_DATA) && JUCE_TARGET_HAS_BINARY_DATA
    // Many JUCE BinaryData.h provide these:
    //   extern const char* namedResourceList[];
    //   extern const int namedResourceListSize;
    // If your BinaryData doesn't have them, this still compiles in most JUCE setups.
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (auto* nm = BinaryData::namedResourceList[i])
        {
            const String name (nm);

            if (name.containsIgnoreCase ("html")
                || name.endsWithIgnoreCase (".html")
                || name.endsWithIgnoreCase (".htm"))
                htmlishNames.add (name);
        }
    }
   #endif

    // 2) Scan *all* listed resources first for anything HTML-ish
    auto tryResourceName = [&] (const String& name) -> bool
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (name.toRawUTF8(), size))
        {
            if (size <= 0) return false;

            String text = String::fromUTF8 ((const char*) data, size);

            if (! looksLikeHtml (text))
                return false;

            // If it’s URL encoded (%3C...), decode it before setting.
            if (text.trimStart().startsWithIgnoreCase ("%3C"))
                text = urlDecodeToString (text);

            // CRITICAL: setHtml renders HTML. This prevents the %3C%21... white text box.
            webView.setHtml (text, URL ("about:blank"));
            uiLoaded = true;
            return true;
        }
        return false;
    };

    // Prefer HTML-ish names if present
    for (auto& n : htmlishNames)
        if (tryResourceName (n))
            return;

   #if defined (JUCE_TARGET_HAS_BINARY_DATA) && JUCE_TARGET_HAS_BINARY_DATA
    // If that didn’t work, brute-force every embedded resource and pick the first that looks like HTML.
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (auto* nm = BinaryData::namedResourceList[i])
        {
            if (tryResourceName (String (nm)))
                return;
        }
    }
   #endif

    // Nothing worked — render a readable diagnostic page (not URL encoded)
    String extra;
    if (htmlishNames.isEmpty())
        extra = "No resource names containing 'html' were found.\n";
    else
        extra = "HTML-ish resource names seen:\n- " + htmlishNames.joinIntoString ("\n- ");

    webView.setHtml (makeMissingUiHtml (extra), URL ("about:blank"));
    uiLoaded = false;
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    const auto lDb = processor.currentVUL.load();
    const auto rDb = processor.currentVUR.load();

    const auto js =
        "if (window.setVUMeters) window.setVUMeters("
        + String (lDb, 3) + "," + String (rDb, 3) + ");";

    webView.executeJavascript (js);
}
