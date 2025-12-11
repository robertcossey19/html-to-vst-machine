#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace juce;

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Size of your UI
    setSize (1200, 600);

    // Set up the web view
    addAndMakeVisible (webView);
    webView.setOpaque (false);

    // Your HTML UI (raw, NOT URL-encoded)
    static const char* html = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>ANALOGEXACT – ATR-102 UI</title>

<script src="https://cdn.tailwindcss.com"></script>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=Roboto+Mono&display=swap" rel="stylesheet">

<style>
  html,body{height:100%;background:#1a202c}
  body{font-family:'Inter',sans-serif;color:#e5e7eb}

  .metal-panel{background:linear-gradient(145deg,#4a5568,#3a4454);
    border:1px solid #718096;border-top-color:#a0aec0;}

  .module-bg{background:rgba(0,0,0,.2);border:1px solid rgba(0,0,0,.4);
    box-shadow:inset 0 2px 6px rgba(0,0,0,.4);}

  .knob{position:relative;width:80px;height:80px;border-radius:50%;
    background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;
    box-shadow:0 5px 10px rgba(0,0,0,.5),inset 0 2px 3px rgba(255,255,255,.08);
    display:flex;align-items:center;justify-content:center;
    cursor:pointer;user-select:none;touch-action:none;}

  .knob-ind{position:absolute;width:4px;height:12px;background:#e2e8f0;
    top:6px;border-radius:2px;transform-origin:center 34px;
    box-shadow:0 0 3px rgba(255,255,255,.5);}

  .vu-housing{background:#111827;border:4px solid #4b5563;border-radius:10px;
    padding:12px;box-shadow:inset 0 0 20px rgba(0,0,0,.8);}

  .vu-scale{position:relative;width:100%;height:60px;background:#ffebcd;
    border-radius:4px;overflow:hidden;}

  .vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;
    bottom:0;left:50%;transform-origin:bottom center;
    transition:transform .04s linear;box-shadow:0 0 5px #dc2626;}

  .switch-group button.active{
    background:#f6ad55;color:#1a202c;font-weight:700;
    box-shadow:inset 0 2px 4px rgba(0,0,0,.4);
  }

  #tech-panel{transition:max-height .5s ease,opacity .4s ease,transform .4s ease;
    transform:scaleY(.98);}
  #tech-panel.open{transform:scaleY(1);}

  .collapsible{transition:max-height .35s ease,opacity .25s ease,transform .25s ease;
    overflow:hidden;}
  .collapsible.hidden-state{
    max-height:0;opacity:0;transform:translateY(-4px);pointer-events:none;
  }

  .toggle-switch{display:inline-block;width:50px;height:26px;background:#4a5568;
    border-radius:13px;cursor:pointer;position:relative;border:1px solid #1a202c;}
  .toggle-switch.active{background:#f6ad55;}
  .toggle-switch-handle{position:absolute;top:2px;left:2px;width:20px;height:20px;
    background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.4);
    transition:transform .2s;}
  .toggle-switch.active .toggle-switch-handle{transform:translateX(24px);}
</style>
</head>

<body class="flex items-center justify-center min-h-screen p-3">
<div class="metal-panel rounded-2xl p-4 md:p-6 w-full max-w-6xl space-y-4">

  <!-- HEADER -->
  <div class="flex justify-between items-center pb-3 border-b border-black/30">
    <div>
      <h1 class="text-2xl md:text-3xl font-bold text-white tracking-wider">
        ANALOGEXACT
      </h1>
      <p class="text-sm text-gray-400">
        ATR-102 • Transformer I/O • 768 kHz oversampling
      </p>
    </div>
  </div>

  <!-- VU METERS -->
  <div class="grid grid-cols-1 md:grid-cols-2 gap-3">
    <div class="vu-housing">
      <div class="flex items-center justify-between text-xs text-gray-700 mb-1">
        <span class="text-gray-300">VU - L</span>
        <span class="text-gray-500">-20  -10   0   +3  +5</span>
      </div>
      <div class="vu-scale">
        <div id="vuL" class="vu-needle" style="transform:rotate(-70deg)"></div>
      </div>
    </div>

    <div class="vu-housing">
      <div class="flex items-center justify-between text-xs text-gray-700 mb-1">
        <span class="text-gray-300">VU - R</span>
        <span class="text-gray-500">-20  -10   0   +3  +5</span>
      </div>
      <div class="vu-scale">
        <div id="vuR" class="vu-needle" style="transform:rotate(-70deg)"></div>
      </div>
    </div>
  </div>

  <!-- INPUT / OUTPUT / MONITOR -->
  <div class="grid grid-cols-1 md:grid-cols-3 gap-4">

    <div class="module-bg p-4 rounded-lg space-y-3 flex flex-col items-center">
      <h2 class="text-lg font-bold">INPUT LEVEL</h2>
      <div class="knob" id="knob-in"><div class="knob-ind"></div></div>
      <span id="val-in" class="font-mono text-sm text-yellow-300">0.0 dB</span>
    </div>

    <div class="module-bg p-4 rounded-lg space-y-3 flex flex-col items-center">
      <h2 class="text-lg font-bold">OUTPUT LEVEL</h2>
      <div class="knob" id="knob-out"><div class="knob-ind"></div></div>
      <span id="val-out" class="font-mono text-sm text-yellow-300">0.0 dB</span>
    </div>

    <div class="module-bg p-4 rounded-lg space-y-3 flex flex-col items-center">
      <h2 class="text-lg font-bold">MONITOR</h2>
      <div id="monitor" class="switch-group flex rounded-md bg-gray-900 p-1 w-full">
        <button data-monitor="input" class="flex-1 py-2 rounded text-sm active">INPUT</button>
        <button data-monitor="repro" class="flex-1 py-2 rounded text-sm">REPRO</button>
      </div>
    </div>

  </div>

  <!-- TECH PANEL TOGGLE -->
  <div class="text-center pt-1">
    <button id="toggle-tech" class="text-yellow-400 hover:text-yellow-300 font-semibold text-sm">
      Technician’s Setup Panel ▼
    </button>
  </div>

  <!-- TECH PANEL SHELL (kept minimal here for brevity) -->
  <div id="tech-panel"
       class="max-h-0 opacity-0 overflow-hidden module-bg p-4 rounded-lg space-y-6">
    <p class="text-xs text-gray-300">
      Technician controls (bias, EQ, tape stock, flux) live here in the full UI.
    </p>
  </div>

</div>

<script>
// VU update from C++
window.setVUMeters = function (left, right) {
    const l = document.getElementById("vuL");
    const r = document.getElementById("vuR");
    if (!l || !r) return;

    const toDb = v => 20 * Math.log10(Math.max(v, 1e-9));
    const dbL = toDb(left);
    const dbR = toDb(right);

    const needle = db =>
        Math.max(-70, Math.min(20, (db + 50) * 2.25 - 70));

    l.style.transform = `rotate(${needle(dbL)}deg)`;
    r.style.transform = `rotate(${needle(dbR)}deg)`;
};

// Simple knob helper
function $(id) { return document.getElementById(id); }
function safe(n, f = 0) { const v = Number(n); return Number.isFinite(v) ? v : f; }

let state = { inDb:0, outDb:0 };

function makeKnob(knob, val, key, min, max, fmt) {
    const el  = $(knob);
    const ind = el.querySelector(".knob-ind");
    const txt = $(val);
    let dragging = false, startY = 0, startVal = 0;

    function setVal(v) {
        const c = Math.min(max, Math.max(min, safe(v, 0)));
        state[key] = c;
        const pct = (c - min) / (max - min);
        ind.style.transform = `rotate(${270*pct - 135}deg)`;
        txt.textContent = fmt(c);
    }

    setVal(state[key]);

    el.addEventListener("pointerdown", e => {
        dragging = true;
        startY = e.clientY;
        startVal = state[key];
        e.preventDefault();
    });

    el.addEventListener("pointermove", e => {
        if (!dragging) return;
        const dy = startY - e.clientY;
        setVal(startVal + (dy / 150) * (max - min));
    });

    el.addEventListener("pointerup", () => dragging = false);
}

makeKnob("knob-in",  "val-in",  "inDb",  -12, 12, v => `${v.toFixed(1)} dB`);
makeKnob("knob-out", "val-out", "outDb", -24,  6, v => `${v.toFixed(1)} dB`);

// Tech panel toggle
const techPanel = $("tech-panel");
const techBtn   = $("toggle-tech");
techBtn.addEventListener("click", () => {
    const open = techPanel.classList.contains("open");
    if (open) {
        techPanel.classList.remove("open");
        techPanel.style.maxHeight = "0";
        techPanel.style.opacity = "0";
        techBtn.textContent = "Technician’s Setup Panel ▼";
    } else {
        techPanel.classList.add("open");
        techPanel.style.maxHeight = techPanel.scrollHeight + "px";
        techPanel.style.opacity = "1";
        techBtn.textContent = "Technician’s Setup Panel ▲";
    }
});
</script>
</body>
</html>
)";

    // Use a data: URL so the HTML is self-contained
    auto encoded = juce::URL::addEscapeChars (html, true);
    webView.goToURL ("data:text/html;charset=utf-8," + encoded);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() = default;

void HtmlToVstAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
