#include "PluginEditor.h"
#include <cctype>

#if __has_include("BinaryData.h")
  #include "BinaryData.h"
  #define HTMLTOVST_HAS_BINARYDATA 1
#else
  #define HTMLTOVST_HAS_BINARYDATA 0
#endif

static float getParamValue (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float fallback = 0.0f)
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load();
    return fallback;
}

static bool setParamValue (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float newValue)
{
    auto* p  = apvts.getParameter (id);
    auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
    if (rp == nullptr)
        return false;

    const auto norm = rp->convertTo0to1 (newValue);
    rp->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
    return true;
}

static juce::String urlDecode (const juce::String& s)
{
    // Minimal percent-decoder (no dependency on juce::URL helpers)
    auto in = s.toStdString();
    std::string out;
    out.reserve (in.size());

    auto hex = [] (char c) -> int
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    for (size_t i = 0; i < in.size(); ++i)
    {
        const char c = in[i];

        if (c == '+')
        {
            out.push_back (' ');
            continue;
        }

        if (c == '%' && i + 2 < in.size())
        {
            const int hi = hex (in[i + 1]);
            const int lo = hex (in[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
                out.push_back ((char) ((hi << 4) | lo));
                i += 2;
                continue;
            }
        }

        out.push_back (c);
    }

    return juce::String (out);
}

static juce::StringPairArray parseQuery (const juce::String& query)
{
    juce::StringPairArray params;
    if (query.isEmpty())
        return params;

    auto parts = juce::StringArray::fromTokens (query, "&", "");
    for (auto& part : parts)
    {
        auto eq = part.indexOfChar ('=');
        if (eq <= 0)
            continue;

        auto key = urlDecode (part.substring (0, eq).trim());
        auto val = urlDecode (part.substring (eq + 1).trim());
        params.set (key, val);
    }
    return params;
}

static juce::String getCmdFromJuceUrl (const juce::String& url, juce::StringPairArray& outParams)
{
    // Expects:
    //   juce://ready
    //   juce://setParam?id=inGain&value=1.23
    outParams.clear();

    auto s = url.trim();

    if (! s.startsWithIgnoreCase ("juce://"))
        return {};

    s = s.fromFirstOccurrenceOf ("juce://", false, true);

    auto qPos = s.indexOfChar ('?');
    if (qPos < 0)
        return s; // no query string

    auto cmd   = s.substring (0, qPos);
    auto query = s.substring (qPos + 1);
    outParams  = parseQuery (query);
    return cmd;
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    audioProcessor.apvts.addParameterListener ("inGain",  this);
    audioProcessor.apvts.addParameterListener ("drive",   this);
    audioProcessor.apvts.addParameterListener ("outGain", this);

    inGainValue.store  (getParamValue (audioProcessor.apvts, "inGain",  0.0f));
    driveValue.store   (getParamValue (audioProcessor.apvts, "drive",   0.0f));
    outGainValue.store (getParamValue (audioProcessor.apvts, "outGain", 0.0f));

    browser = std::make_unique<Browser> (*this);
    addAndMakeVisible (*browser);

    const auto html = getIndexHtml();

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("HTMLtoVST_UI");
    dir.createDirectory();

    auto indexFile = dir.getChildFile ("index.html");
    indexFile.replaceWithText (html);

    browser->goToURL (juce::URL (indexFile).toString (true));

    setSize (780, 520);
    startTimerHz (30);
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    stopTimer();

    audioProcessor.apvts.removeParameterListener ("inGain",  this);
    audioProcessor.apvts.removeParameterListener ("drive",   this);
    audioProcessor.apvts.removeParameterListener ("outGain", this);

    browser.reset();
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

void HtmlToVstPluginAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "inGain")  inGainValue.store (newValue);
    if (parameterID == "drive")   driveValue.store  (newValue);
    if (parameterID == "outGain") outGainValue.store (newValue);

    dirty.store (true);
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    if (! dirty.exchange (false))
        return;

    sendAllParamsToJS();
}

bool HtmlToVstPluginAudioProcessorEditor::handleBrowserURL (const juce::String& url)
{
    if (! url.startsWithIgnoreCase ("juce://"))
        return true; // allow normal navigation

    juce::StringPairArray params;
    const auto cmd = getCmdFromJuceUrl (url, params);

    if (cmd.equalsIgnoreCase ("ready"))
    {
        dirty.store (true);
        return false;
    }

    if (cmd.equalsIgnoreCase ("setParam"))
    {
        const auto id = params["id"];
        const auto vS = params["value"];

        if (id.isNotEmpty() && vS.isNotEmpty())
        {
            const float v = (float) vS.getDoubleValue();
            setParamValue (audioProcessor.apvts, id, v);
        }
        return false;
    }

    return false;
}

void HtmlToVstPluginAudioProcessorEditor::sendParamToJS (const juce::String& paramID, float value)
{
    if (browser == nullptr)
        return;

    juce::String js;
    js << "if (window.__setOneParam) window.__setOneParam("
       << juce::JSON::toString (paramID) << ", "
       << juce::String (value, 6) << ");";

    browser->evaluateJavascript (js);
}

void HtmlToVstPluginAudioProcessorEditor::sendAllParamsToJS()
{
    if (browser == nullptr)
        return;

    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("inGain",  (double) inGainValue.load());
    obj->setProperty ("drive",   (double) driveValue.load());
    obj->setProperty ("outGain", (double) outGainValue.load());

    const juce::var v (obj.release());
    const auto json = juce::JSON::toString (v);

    juce::String js;
    js << "if (window.__setParams) window.__setParams(" << json << ");";

    browser->evaluateJavascript (js);
}

juce::String HtmlToVstPluginAudioProcessorEditor::getIndexHtml()
{
#if HTMLTOVST_HAS_BINARYDATA
    // Most robust: ask BinaryData for the named resource (no hard-coded symbol names)
    int size = 0;
    if (auto* data = BinaryData::getNamedResource ("index.html", size))
        if (size > 0)
            return juce::String::fromUTF8 ((const char*) data, size);
#endif

    // Fallback embedded UI
    return R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>HTMLtoVST</title>
<style>
  :root{--bg:#0b0e14;--panel:#121826;--line:rgba(255,255,255,.12);--txt:#e9eefb;--mut:#a9b5d1;}
  html,body{height:100%;margin:0;background:linear-gradient(180deg,#080b12,#0b0e14 60%);color:var(--txt);font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial;}
  .wrap{height:100%;display:flex;align-items:center;justify-content:center;padding:18px;}
  .card{width:min(760px,100%);background:rgba(18,24,38,.92);border:1px solid var(--line);border-radius:18px;box-shadow:0 12px 40px rgba(0,0,0,.45);padding:18px;}
  .top{display:flex;align-items:flex-end;justify-content:space-between;gap:12px;margin-bottom:14px;}
  .brand{font-weight:800;letter-spacing:.08em}
  .sub{font-size:12px;color:var(--mut)}
  .row{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
  .mod{background:rgba(0,0,0,.22);border:1px solid var(--line);border-radius:16px;padding:14px}
  .lbl{display:flex;justify-content:space-between;font-size:12px;color:var(--mut);margin-bottom:8px}
  input[type=range]{width:100%;}
  .val{font-variant-numeric:tabular-nums;color:var(--txt)}
  .hint{margin-top:12px;font-size:12px;color:var(--mut)}
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <div class="top">
      <div>
        <div class="brand">HTMLtoVST</div>
        <div class="sub">Web UI (wired) — inGain / drive / outGain</div>
      </div>
      <div class="sub">juce:// bridge OK</div>
    </div>

    <div class="row">
      <div class="mod">
        <div class="lbl"><span>Input</span><span class="val" id="inGainVal">0.00 dB</span></div>
        <input id="inGain" type="range" min="-24" max="24" step="0.1" value="0">
      </div>
      <div class="mod">
        <div class="lbl"><span>Drive</span><span class="val" id="driveVal">0.000</span></div>
        <input id="drive" type="range" min="0" max="1" step="0.001" value="0">
      </div>
      <div class="mod">
        <div class="lbl"><span>Output</span><span class="val" id="outGainVal">0.00 dB</span></div>
        <input id="outGain" type="range" min="-24" max="24" step="0.1" value="0">
      </div>
    </div>

    <div class="hint">Host automation updates this UI too.</div>
  </div>
</div>

<script>
(function(){
  function send(cmd, params){
    let q = "";
    if(params){
      q = "?" + Object.keys(params).map(k => encodeURIComponent(k)+"="+encodeURIComponent(params[k])).join("&");
    }
    window.location.href = "juce://" + cmd + q;
  }

  function setText(id, txt){ const el=document.getElementById(id); if(el) el.textContent=txt; }

  function bindRange(id, fmt){
    const el = document.getElementById(id);
    const valEl = document.getElementById(id + "Val");
    function upd(){
      const v = parseFloat(el.value);
      if(valEl) valEl.textContent = fmt(v);
    }
    el.addEventListener("input", () => {
      upd();
      send("setParam", { id, value: el.value });
    });
    upd();
  }

  window.__setParams = function(obj){
    if(!obj) return;
    if(typeof obj.inGain === "number"){ inGain.value=obj.inGain; setText("inGainVal", obj.inGain.toFixed(2)+" dB"); }
    if(typeof obj.drive === "number"){ drive.value=obj.drive; setText("driveVal", obj.drive.toFixed(3)); }
    if(typeof obj.outGain === "number"){ outGain.value=obj.outGain; setText("outGainVal", obj.outGain.toFixed(2)+" dB"); }
  };

  window.__setOneParam = function(id, value){
    const el = document.getElementById(id);
    if(!el) return;
    el.value = value;
    if(id==="inGain")  setText("inGainVal",  Number(value).toFixed(2)+" dB");
    if(id==="drive")   setText("driveVal",   Number(value).toFixed(3));
    if(id==="outGain") setText("outGainVal", Number(value).toFixed(2)+" dB");
  };

  bindRange("inGain",  v => v.toFixed(2)+" dB");
  bindRange("drive",   v => v.toFixed(3));
  bindRange("outGain", v => v.toFixed(2)+" dB");

  send("ready");
})();
</script>
</body>
</html>
)HTML";
}
