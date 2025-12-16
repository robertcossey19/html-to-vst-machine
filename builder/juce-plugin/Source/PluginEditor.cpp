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

//==============================================================================
// Browser
HtmlToVstPluginAudioProcessorEditor::Browser::Browser()
    : WebBrowserComponent (WebBrowserComponent::Options{})
{
}

bool HtmlToVstPluginAudioProcessorEditor::Browser::pageAboutToLoad (const String& newURL)
{
    if (onAboutToLoad)
        return onAboutToLoad (newURL);
    return true;
}

void HtmlToVstPluginAudioProcessorEditor::Browser::pageFinishedLoading (const String& url)
{
    if (onFinished)
        onFinished (url);
}

//==============================================================================
// Editor
HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setOpaque (true);

    addAndMakeVisible (webView);
    setSize (1100, 640);

    // Intercept juce:// messages from injected JS
    webView.onAboutToLoad = [this] (const String& url) -> bool
    {
        if (url.startsWithIgnoreCase ("juce://"))
        {
            handleJuceUrl (url);
            return false; // cancel nav
        }
        return true;
    };

    // Inject JS after any page load
    webView.onFinished = [this] (const String&)
    {
        injectBridgeJavascript();
    };

    loadUiFromBinaryData();

    // meter / polling timer
    startTimerHz (20);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    stopTimer();

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

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    if (! uiLoaded)
        return;

    const float inDb  = audioProcessor.getInputMeterDb();
    const float outDb = audioProcessor.getOutputMeterDb();

    // If the HTML defines window.juceSetMeters(inDb,outDb), use it
    // Otherwise try common element ids
    String js;
    js << "try {"
       << "  if (window.juceSetMeters) { window.juceSetMeters(" << inDb << "," << outDb << "); }"
       << "  var i=document.getElementById('meterIn');  if(i) i.style.width=Math.max(0,Math.min(100,((" << inDb << "+60)/60)*100))+'%';"
       << "  var o=document.getElementById('meterOut'); if(o) o.style.width=Math.max(0,Math.min(100,((" << outDb << "+60)/60)*100))+'%';"
       << "  var it=document.getElementById('meterInDb');  if(it) it.textContent=(" << inDb << ").toFixed(1)+' dB';"
       << "  var ot=document.getElementById('meterOutDb'); if(ot) ot.textContent=(" << outDb << ").toFixed(1)+' dB';"
       << "} catch(e) {}";

    webView.executeJavascript (js);
}

//==============================================================================
// Minimal URL decoding
String HtmlToVstPluginAudioProcessorEditor::urlDecode (const String& s)
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

    if (t.startsWithChar ('<') && (t.containsIgnoreCase ("<html") || t.containsIgnoreCase ("<!doctype")))
        return true;

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
      <p>No embedded HTML resource looked like a UI page.</p>
      <p>Details:</p>
      <code>)HTML" + extra + R"HTML(</code>
    </div>
  </div>
</body>
</html>)HTML";
}

//==============================================================================
// Query param parser that works regardless of JUCE URL API differences
String HtmlToVstPluginAudioProcessorEditor::getQueryParam (const String& url, const String& key)
{
    const auto qPos = url.indexOfChar ('?');
    if (qPos < 0)
        return {};

    const auto query = url.substring (qPos + 1);
    const auto parts = StringArray::fromTokens (query, "&", "");

    for (auto& part : parts)
    {
        const auto eq = part.indexOfChar ('=');
        if (eq <= 0) continue;

        const auto k = part.substring (0, eq);
        const auto v = part.substring (eq + 1);

        if (k == key)
            return urlDecode (v);
    }

    return {};
}

//==============================================================================
// UI loading
void HtmlToVstPluginAudioProcessorEditor::writeHtmlToTempAndLoad (const String& html)
{
    const auto fileName = "HtmlToVstUI_" + String::toHexString ((pointer_sized_int) this) + ".html";
    tempHtmlFile = File::getSpecialLocation (File::tempDirectory).getChildFile (fileName);

    {
        FileOutputStream out (tempHtmlFile);
        if (! out.openedOk())
            return;

        out.setPosition (0);
        out.truncate();
        out.writeText (html, false, false, "\n");
        out.flush();
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

            auto t = trimStartFast (text);
            if (t.startsWithChar ('%') || t.containsIgnoreCase ("%3c"))
                text = urlDecode (text);

            if (! looksLikeHtml (text))
                return false;

            writeHtmlToTempAndLoad (text);
            return true;
        }
        return false;
    };

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

//==============================================================================
// Bridge: JS -> C++ via juce://set?param=ID&value=0..1
void HtmlToVstPluginAudioProcessorEditor::handleJuceUrl (const String& urlString)
{
    // Examples:
    // juce://set?param=inputGain&value=0.52
    // juce://toggle?param=transformer&value=1
    const auto host = urlString.fromFirstOccurrenceOf ("juce://", false, false)
                              .upToFirstOccurrenceOf ("?", false, false)
                              .toLowerCase();

    const auto param = getQueryParam (urlString, "param");
    const auto value = getQueryParam (urlString, "value");

    if (param.isEmpty())
        return;

    const float v = value.isNotEmpty() ? (float) value.getDoubleValue() : 0.0f;

    if (host == "set" || host == "toggle")
        audioProcessor.setParamNormalized (param, jlimit (0.0f, 1.0f, v));
}

// Inject a generic hook layer so “moving knobs” actually changes APVTS
void HtmlToVstPluginAudioProcessorEditor::injectBridgeJavascript()
{
    // This script:
    // 1) defines window.juce.setParam / toggleParam
    // 2) auto-hooks:
    //    - elements with data-param
    //    - common ids/names: input, output, gain, mix, drive, transformer, bypass
    // 3) provides window.juceSetMeters(inDb,outDb) fallback if HTML wants it
    const String js = R"JS(
(function(){
  try {
    if (window.juce && window.juce.__installed) return;

    function nav(u){ window.location.href = u; }

    window.juce = {
      __installed: true,
      setParam: function(param, value){
        nav("juce://set?param=" + encodeURIComponent(param) + "&value=" + encodeURIComponent(value));
      },
      toggleParam: function(param, on){
        nav("juce://toggle?param=" + encodeURIComponent(param) + "&value=" + (on ? "1" : "0"));
      }
    };

    function hookEl(el, param){
      if (!el || !param) return;

      var tag = (el.tagName || "").toLowerCase();
      var type = (el.type || "").toLowerCase();

      var send = function(){
        if (type === "checkbox" || type === "radio") {
          window.juce.toggleParam(param, !!el.checked);
        } else {
          var v = parseFloat(el.value);
          if (!isFinite(v)) v = 0;
          // If UI uses 0..100 or -60..+12 etc, clamp to 0..1 best-effort:
          if (v > 1.0001) v = v / 100.0;
          if (v < 0) v = 0;
          if (v > 1) v = 1;
          window.juce.setParam(param, v);
        }
      };

      el.addEventListener("input", send);
      el.addEventListener("change", send);
    }

    // 1) data-param
    document.querySelectorAll("[data-param]").forEach(function(el){
      hookEl(el, el.getAttribute("data-param"));
    });

    // 2) common ids/names -> canonical param IDs
    var map = {
      "input": "inputGain",
      "inputgain": "inputGain",
      "in": "inputGain",
      "output": "outputGain",
      "outputgain": "outputGain",
      "out": "outputGain",
      "gain": "outputGain",
      "mix": "mix",
      "drive": "drive",
      "saturation": "drive",
      "transformer": "transformer",
      "xfmr": "transformer",
      "bypass": "bypass"
    };

    Object.keys(map).forEach(function(k){
      var id = document.getElementById(k);
      if (id) hookEl(id, map[k]);

      document.querySelectorAll("[name='"+k+"']").forEach(function(el){
        hookEl(el, map[k]);
      });
    });

    // Optional: meters helper (if the HTML defines #vuIn/#vuOut, etc.)
    window.juceSetMeters = function(inDb, outDb){
      try {
        // if the UI uses needles, it can override this function.
        var clamp = function(x){ return Math.max(-60, Math.min(12, x)); };
        var inC = clamp(inDb), outC = clamp(outDb);

        var a = document.getElementById("vuIn");
        var b = document.getElementById("vuOut");
        if (a) a.style.transform = "rotate(" + ((inC + 60) / 72 * 90 - 45) + "deg)";
        if (b) b.style.transform = "rotate(" + ((outC + 60) / 72 * 90 - 45) + "deg)";
      } catch(e) {}
    };
  } catch(e) {}
})();
)JS";

    webView.executeJavascript (js);
}

//==============================================================================
