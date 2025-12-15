#include "PluginEditor.h"

// Try both include styles depending on how CMake/JUCE exposes BinaryData.h
#if __has_include("BinaryData.h")
  #include "BinaryData.h"
#elif __has_include(<BinaryData.h>)
  #include <BinaryData.h>
#endif

using namespace juce;

static String trimStartFast (String s)
{
    while (s.isNotEmpty())
    {
        auto c = s[0];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            s = s.substring (1);
        else
            break;
    }
    return s;
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setOpaque (true);
    addAndMakeVisible (webView);

    // reasonable default size
    setSize (1100, 640);

    loadUiFromBinaryData();
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    // Best effort cleanup
    if (tempHtmlFile.existsAsFile())
        tempHtmlFile.deleteFile();
}

void HtmlToVstPluginAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

String HtmlToVstPluginAudioProcessorEditor::urlDecodeToString (const String& s)
{
    String out;
    out.preallocateBytes ((size_t) s.getNumBytesAsUTF8());

    for (int i = 0; i < s.length();)
    {
        const auto ch = s[i];

        if (ch == '+')
        {
            out += ' ';
            ++i;
            continue;
        }

        if (ch == '%' && i + 2 < s.length())
        {
            auto h1 = s[i + 1];
            auto h2 = s[i + 2];

            auto hexVal = [] (juce_wchar c) -> int
            {
                if (c >= '0' && c <= '9') return (int) (c - '0');
                if (c >= 'a' && c <= 'f') return 10 + (int) (c - 'a');
                if (c >= 'A' && c <= 'F') return 10 + (int) (c - 'A');
                return -1;
            };

            const int v1 = hexVal (h1);
            const int v2 = hexVal (h2);

            if (v1 >= 0 && v2 >= 0)
            {
                const juce_wchar decoded = (juce_wchar) ((v1 << 4) | v2);
                out += decoded;
                i += 3;
                continue;
            }
        }

        out += ch;
        ++i;
    }

    return out;
}

bool HtmlToVstPluginAudioProcessorEditor::looksLikeHtml (const String& s)
{
    auto t = trimStartFast (s);

    // raw html
    if (t.startsWithChar ('<') && (t.containsIgnoreCase ("<html") || t.containsIgnoreCase ("<!doctype")))
        return true;

    // url-encoded html typically starts with %3C or %0A%3C
    if (t.startsWithIgnoreCase ("%3c") || t.containsIgnoreCase ("%3chtml") || t.containsIgnoreCase ("%3c%21doctype"))
        return true;

    return false;
}

String HtmlToVstPluginAudioProcessorEditor::makeMissingUiHtml (const String& extra)
{
    return R"HTML(<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>UI Missing</title>
  <style>
    html,body{height:100%;margin:0;background:#0b1020;color:#e5e7eb;font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial}
    .wrap{height:100%;display:flex;align-items:center;justify-content:center;padding:18px}
    .card{max-width:900px;width:100%;border:1px solid #334155;border-radius:14px;background:rgba(255,255,255,.04);padding:18px}
    h2{margin:0 0 8px 0;font-size:18px}
    p{margin:6px 0;color:#cbd5e1}
    code{display:block;white-space:pre-wrap;word-break:break-word;background:rgba(0,0,0,.35);border:1px solid rgba(255,255,255,.08);padding:12px;border-radius:12px;color:#e2e8f0}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h2>Embedded UI not found</h2>
      <p>The plugin built, but no embedded HTML resource looked like a UI page.</p>
      <p>Details:</p>
      <code>)HTML" + extra + R"HTML(</code>
    </div>
  </div>
</body>
</html>)HTML";
}

void HtmlToVstPluginAudioProcessorEditor::writeHtmlToTempAndLoad (const String& html)
{
    // Unique per instance so multiple plugin windows don’t fight over the file
    const auto fileName = "HtmlToVstUI_" + String::toHexString ((pointer_sized_int) this) + ".html";
    tempHtmlFile = File::getSpecialLocation (File::tempDirectory).getChildFile (fileName);

    {
        FileOutputStream out (tempHtmlFile);
        if (! out.openedOk())
        {
            const auto msg = "Failed to open temp file for UI:\n" + tempHtmlFile.getFullPathName();
            const auto fallback = makeMissingUiHtml (msg);

            FileOutputStream out2 (tempHtmlFile);
            if (out2.openedOk())
            {
                out2.writeText (fallback, false, false, "\n");
                out2.flush();
            }
        }
        else
        {
            out.setPosition (0);
            out.truncate();
            out.writeText (html, false, false, "\n");
            out.flush();
        }
    }

    auto u = URL (tempHtmlFile).withParameter ("t", String (Time::getMillisecondCounter()));
    webView.goToURL (u.toString (true));
    uiLoaded = true;
}

void HtmlToVstPluginAudioProcessorEditor::loadUiFromBinaryData()
{
#if defined(JUCE_TARGET_HAS_BINARY_DATA) && JUCE_TARGET_HAS_BINARY_DATA
    String debug;
    debug << "BinaryData scan:\n";

    auto tryResource = [&] (const String& name) -> bool
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (name.toRawUTF8(), size))
        {
            String text = String::fromUTF8 ((const char*) data, size);

            debug << " - " << name << " (" << String (size) << " bytes)\n";

            if (! looksLikeHtml (text))
                return false;

            // Decode URL-encoded HTML if needed
            auto t = trimStartFast (text);
            if (t.startsWithChar ('%') || t.containsIgnoreCase ("%3c"))
                text = urlDecodeToString (text);

            if (! looksLikeHtml (text))
                return false;

            writeHtmlToTempAndLoad (text);
            return true;
        }
        return false;
    };

    // Pass 1: prioritize anything with “html” in the resource name
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (auto* nm = BinaryData::namedResourceList[i])
        {
            const String name (nm);
            if (name.containsIgnoreCase ("html"))
                if (tryResource (name))
                    return;
        }
    }

    // Pass 2: brute force everything
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (auto* nm = BinaryData::namedResourceList[i])
        {
            const String name (nm);
            if (tryResource (name))
                return;
        }
    }

    writeHtmlToTempAndLoad (makeMissingUiHtml (debug));
#else
    writeHtmlToTempAndLoad (makeMissingUiHtml ("JUCE_TARGET_HAS_BINARY_DATA is disabled."));
#endif
}
