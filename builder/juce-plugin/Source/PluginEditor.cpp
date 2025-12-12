#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

static String findFirstHtmlResourceName()
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (auto* nm = BinaryData::namedResourceList[i])
        {
            const String name (nm);
            // BinaryData names typically end like: index_html
            if (name.endsWithIgnoreCase ("_html") || name.containsIgnoreCase ("html"))
                return name;
        }
    }
    return {};
}

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
: AudioProcessorEditor (&p), processor (p), webView (*this)
{
    setOpaque (true);
    setSize (980, 520);

    addAndMakeVisible (webView);
    loadUiToTempFileAndNavigate();

    startTimerHz (30); // VU updates
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor()
{
    stopTimer();
    if (tempHtmlFile.existsAsFile())
        tempHtmlFile.deleteFile();
}

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

bool HtmlToVstAudioProcessorEditor::BridgeBrowser::pageAboutToLoad (const String& newURL)
{
    // UI -> Plugin: juce://set?param=inGainDb&value=-6.0
    URL u (newURL);

    if (u.getScheme().equalsIgnoreCase ("juce"))
    {
        const auto cmd   = u.getHost().isNotEmpty() ? u.getHost() : u.getSubPath().trimCharactersAtStart ("/");
        const auto param = u.getParameterValue ("param");
        const auto value = u.getParameterValue ("value");

        if (cmd.containsIgnoreCase ("set") && param.isNotEmpty())
        {
            const float v = value.getFloatValue();
            owner.processor.setParameterFromUI (param, v);
        }

        // swallow navigation
        return false;
    }

    return true;
}

void HtmlToVstAudioProcessorEditor::BridgeBrowser::pageFinishedLoading (const String&)
{
    owner.injectBridgeJs();
}

void HtmlToVstAudioProcessorEditor::loadUiToTempFileAndNavigate()
{
    const auto resName = findFirstHtmlResourceName();
    int sz = 0;

    if (resName.isEmpty())
        return;

    if (auto* data = BinaryData::getNamedResource (resName.toRawUTF8(), sz))
    {
        const String html = String::fromUTF8 ((const char*) data, sz);

        tempHtmlFile = File::getSpecialLocation (File::tempDirectory)
            .getChildFile ("HtmlToVst_UI.html");

        tempHtmlFile.deleteFile();
        tempHtmlFile.replaceWithText (html);

        // cache-bust so hosts reload
        const auto urlStr = URL (tempHtmlFile)
            .withParameter ("t", String (Time::getMillisecondCounter()))
            .toString (true);

        webView.goToURL (urlStr);
        bridgeInjected = false;
    }
}

void HtmlToVstAudioProcessorEditor::injectBridgeJs()
{
    if (bridgeInjected)
        return;

    bridgeInjected = true;

    // This does NOT require you to edit HTML.
    // It tries to:
    // - Provide window.JUCE.setParam(name,value)
    // - Auto-bind sliders/toggles that look like gain/transformer
    const String js = R"JS(
(() => {
  if (window.__juceBridgeInstalled) return;
  window.__juceBridgeInstalled = true;

  function send(param, value){
    try {
      const url = "juce://set?param=" + encodeURIComponent(param) + "&value=" + encodeURIComponent(value);
      window.location.href = url;
    } catch(e) {}
  }

  function norm(s){ return (s||"").toString().toLowerCase(); }

  function guessParam(el){
    const t = [
      norm(el.id),
      norm(el.name),
      norm(el.getAttribute("aria-label")),
      norm(el.className),
      norm(el.getAttribute("data-param")),
      norm(el.getAttribute("data-parameter")),
      norm(el.getAttribute("title")),
      norm(el.innerText)
    ].join(" ");

    if (t.includes("transform")) return "transformerOn";
    const hasGain = t.includes("gain");
    const hasIn   = t.includes("input") || t.includes("in ") || t.includes("pre") || t.includes("record");
    const hasOut  = t.includes("output") || t.includes("out ") || t.includes("post") || t.includes("playback");

    if (hasGain && hasIn)  return "inGainDb";
    if (hasGain && hasOut) return "outGainDb";
    return null;
  }

  function mapValue(el, v){
    const type = norm(el.type);
    if (type === "checkbox") return el.checked ? 1 : 0;

    // If it looks normalized, map 0..1 -> -24..+24
    const min = parseFloat(el.min);
    const max = parseFloat(el.max);
    const val = parseFloat(v);

    if (Number.isFinite(min) && Number.isFinite(max) && Number.isFinite(val)) {
      const span = max - min;
      if (span <= 2.0) {
        const n = (val - min) / (span <= 0 ? 1 : span);
        return (-24 + n * 48).toFixed(3);
      }
    }
    return v;
  }

  // Provide a stable API your UI can call (even if it wasn’t written for JUCE)
  window.JUCE = window.JUCE || {};
  window.JUCE.setParam = (name, value) => send(name, value);

  // Wrap common names some UIs use
  const wrapNames = ["setParam","setParameter","sendParam","emitParam","updateParam"];
  wrapNames.forEach(fn => {
    const orig = window[fn];
    window[fn] = function(name, value){
      try { send(name, value); } catch(e) {}
      if (typeof orig === "function") return orig.apply(this, arguments);
    };
  });

  // Auto-bind typical HTML controls
  document.addEventListener("input", (e) => {
    const el = e.target;
    if (!el) return;
    const p = guessParam(el);
    if (!p) return;
    if (el.type === "range" || el.tagName === "INPUT") {
      send(p, mapValue(el, el.value));
    }
  }, true);

  document.addEventListener("change", (e) => {
    const el = e.target;
    if (!el) return;
    const p = guessParam(el);
    if (!p) return;
    if (el.type === "checkbox") {
      send(p, el.checked ? 1 : 0);
    }
  }, true);

  // Meter update hook (we'll call this from JUCE)
  window.__juceUpdateMeters = (lDb, rDb) => {
    // Try common function names first
    const fns = ["setMeters","updateMeters","updateVU","setVU","onVU","meterUpdate"];
    for (const fn of fns) {
      if (typeof window[fn] === "function") { try { window[fn](lDb, rDb); return; } catch(e) {} }
    }
    // Otherwise: set CSS vars so a UI can use them if it wants
    try {
      document.documentElement.style.setProperty("--vuLdb", lDb);
      document.documentElement.style.setProperty("--vuRdb", rDb);
    } catch(e) {}
  };
})();
)JS";

    webView.evaluateJavascript (js);
}

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    const float l = processor.getVuLDb();
    const float r = processor.getVuRDb();

    const String js = "window.__juceUpdateMeters && window.__juceUpdateMeters("
        + String (l, 2) + "," + String (r, 2) + ");";

    webView.evaluateJavascript (js);
}
