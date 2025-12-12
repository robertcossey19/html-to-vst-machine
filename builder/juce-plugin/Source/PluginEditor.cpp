#include "PluginEditor.h"

static juce::String makeFallbackHtml()
{
    // Only used if BinaryData doesn't contain an index.html
    return R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>HTML to VST Plugin</title>
<style>
  body{font-family:system-ui,-apple-system,sans-serif;margin:16px;background:#0b0f14;color:#e7eef7}
  .row{display:flex;gap:16px;align-items:center;margin:12px 0}
  input[type="range"]{width:260px}
  .meter{height:14px;width:260px;background:#1b2430;border-radius:10px;overflow:hidden}
  .bar{height:100%;width:0%;background:#3ddc84}
  .muted{color:#9fb0c3;font-size:12px}
</style>
</head>
<body>
<h2>HTML to VST Plugin (Fallback UI)</h2>
<div class="row">
  <label>Input Gain (dB): <span id="inDb">0</span></label>
  <input id="in" type="range" min="-24" max="24" value="0" step="0.1"/>
</div>
<div class="row">
  <label>Output Gain (dB): <span id="outDb">0</span></label>
  <input id="out" type="range" min="-24" max="24" value="0" step="0.1"/>
</div>
<div class="row">
  <label><input id="xfmr" type="checkbox" checked/> Transformer</label>
</div>

<div class="row">
  <div>
    <div class="muted">Meter L</div>
    <div class="meter"><div id="ml" class="bar"></div></div>
  </div>
</div>
<div class="row">
  <div>
    <div class="muted">Meter R</div>
    <div class="meter"><div id="mr" class="bar"></div></div>
  </div>
</div>

<script>
  const inR = document.getElementById('in');
  const outR = document.getElementById('out');
  const xfmr = document.getElementById('xfmr');
  const inDb = document.getElementById('inDb');
  const outDb = document.getElementById('outDb');

  function send(param, value) {
    // raw value (not normalized)
    location.href = `juce://set?param=${encodeURIComponent(param)}&value=${encodeURIComponent(value)}`;
  }

  inR.addEventListener('input', () => { inDb.textContent = inR.value; send('inputGainDb', inR.value); });
  outR.addEventListener('input', () => { outDb.textContent = outR.value; send('outputGainDb', outR.value); });
  xfmr.addEventListener('change', () => { send('transformerOn', xfmr.checked ? 1 : 0); });

  window.addEventListener('juce-meter', (e) => {
    const l = Math.max(0, Math.min(1, e.detail.l || 0));
    const r = Math.max(0, Math.min(1, e.detail.r || 0));
    document.getElementById('ml').style.width = (l * 100).toFixed(1) + '%';
    document.getElementById('mr').style.width = (r * 100).toFixed(1) + '%';
  });
</script>
</body>
</html>
)HTML";
}

HtmlToVstPluginAudioProcessorEditor::HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor& p)
: AudioProcessorEditor (&p),
  processor (p),
  webView (*this)
{
    setSize (920, 560);
    addAndMakeVisible (webView);

    loadHtmlUi();
    startTimerHz (30); // meter updates + keepalive
}

HtmlToVstPluginAudioProcessorEditor::~HtmlToVstPluginAudioProcessorEditor()
{
    stopTimer();
}

void HtmlToVstPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void HtmlToVstPluginAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void HtmlToVstPluginAudioProcessorEditor::loadHtmlUi()
{
    uiTempDir  = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("html_to_vst_ui");
    uiTempDir.createDirectory();

    uiTempFile = uiTempDir.getChildFile ("index.html");

    int size = 0;
    const void* data = BinaryData::getNamedResource ("index.html", size);

    juce::String html;

    if (data != nullptr && size > 0)
        html = juce::String::fromUTF8 ((const char*) data, size);
    else
        html = makeFallbackHtml();

    uiTempFile.replaceWithText (html);

    // Load with cache-bust query
    const auto urlStr = juce::URL (uiTempFile).toString (true)
                        + "?t=" + juce::String (juce::Time::getMillisecondCounter());

    webView.goToURL (urlStr);
    uiLoaded = true;
}

void HtmlToVstPluginAudioProcessorEditor::timerCallback()
{
    if (! uiLoaded)
        return;

    const float l = processor.getMeterL();
    const float r = processor.getMeterR();

    // Push meters into the HTML as a CustomEvent
    const juce::String js =
        "window.dispatchEvent(new CustomEvent('juce-meter',{detail:{l:"
        + juce::String (l, 6) + ",r:" + juce::String (r, 6) + "}}));";

    webView.executeJavascript (js);
}
