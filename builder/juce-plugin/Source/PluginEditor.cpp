#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>

//==============================================================================
// Embedded HTML UI extracted from the working VST3 you provided (v7).
// This is loaded via a data: URL (base64) so it does NOT depend on copying files
// into the VST3 bundle at build time.
//
// If you later want to split this out again into BinaryData/resources, you can,
// but this approach is the most "drop-in" and least fragile while you're wiring DSP.
//==============================================================================

static const char* getWorkingUiHtmlUtf8()
{
    // IMPORTANT:
    // This raw string literal is the full working UI page from your older build.
    // It uses CDN Tailwind + Google Fonts + inline JS for knob dragging & VU meters.
    static constexpr const char* kHtml = R"JUCEHTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ANALOGEXACT – ATR-102 UI</title>

  <!-- Tailwind (CDN build; OK for plugin UI prototypes) -->
  <script src="https://cdn.tailwindcss.com"></script>

  <!-- Fonts -->
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600;700&family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">

  <style>
    :root{
      --panel:#1f2228;
      --panel2:#2b2f38;
      --accent:#c3a86b;
      --accent2:#7d6a3d;
      --ink:#d8dbe2;
      --ink2:#9aa0aa;
      --danger:#d05b5b;
      --ok:#58c08b;
      --shadow: rgba(0,0,0,.55);
    }
    body{
      margin:0;
      background:#0d0f12;
      color:var(--ink);
      font-family: Roboto, system-ui, -apple-system, Segoe UI, sans-serif;
      user-select:none;
    }
    .frame{
      width:980px;
      height:540px;
      margin:20px auto;
      background: linear-gradient(180deg, #1b1e24 0%, #12141a 100%);
      border-radius:18px;
      box-shadow: 0 18px 60px rgba(0,0,0,.65);
      border:1px solid rgba(255,255,255,.06);
      position:relative;
      overflow:hidden;
    }
    .topbar{
      height:70px;
      padding:12px 18px;
      display:flex;
      align-items:center;
      justify-content:space-between;
      border-bottom:1px solid rgba(255,255,255,.06);
      background: radial-gradient(1200px 160px at 20% -10%, rgba(195,168,107,.25), transparent 60%),
                  linear-gradient(180deg, rgba(255,255,255,.03), transparent);
    }
    .brand{
      display:flex; gap:14px; align-items:center;
    }
    .logo{
      width:46px; height:46px;
      border-radius:12px;
      background: linear-gradient(145deg, #2c2f36, #171a20);
      border:1px solid rgba(255,255,255,.08);
      box-shadow: inset 0 1px 1px rgba(255,255,255,.06),
                  0 10px 24px rgba(0,0,0,.45);
      display:grid; place-items:center;
      position:relative;
    }
    .logo:before{
      content:"";
      width:18px; height:18px;
      border-radius:50%;
      background: radial-gradient(circle at 30% 30%, #fff, var(--accent) 55%, #3a2f18 100%);
      box-shadow:0 0 0 4px rgba(195,168,107,.2);
    }
    .brand h1{
      font-family: Orbitron, sans-serif;
      font-weight:700;
      letter-spacing:.12em;
      font-size:18px;
      line-height:1;
    }
    .brand .sub{
      font-size:12px;
      letter-spacing:.22em;
      color:var(--ink2);
      margin-top:2px;
    }
    .status{
      display:flex; align-items:center; gap:10px;
      font-size:12px;
      color:var(--ink2);
    }
    .pill{
      padding:6px 10px;
      border-radius:999px;
      background: rgba(255,255,255,.04);
      border:1px solid rgba(255,255,255,.08);
      display:flex; align-items:center; gap:8px;
    }
    .dot{
      width:8px; height:8px;
      border-radius:50%;
      background:var(--ok);
      box-shadow:0 0 10px rgba(88,192,139,.6);
    }
    .main{
      padding:18px;
      display:grid;
      grid-template-columns: 290px 1fr 290px;
      gap:18px;
      height: calc(100% - 70px);
      box-sizing:border-box;
    }
    .panel{
      background: linear-gradient(180deg, rgba(255,255,255,.03), transparent 35%),
                  linear-gradient(180deg, var(--panel2), var(--panel));
      border:1px solid rgba(255,255,255,.07);
      border-radius:16px;
      box-shadow: inset 0 1px 1px rgba(255,255,255,.05),
                  0 10px 24px rgba(0,0,0,.45);
      padding:14px;
      position:relative;
    }
    .panel-title{
      font-family: Orbitron, sans-serif;
      letter-spacing:.12em;
      font-size:12px;
      color:rgba(255,255,255,.75);
      margin-bottom:10px;
      display:flex; align-items:center; justify-content:space-between;
    }
    .panel-title span.small{
      font-family:Roboto;
      letter-spacing:.18em;
      font-size:10px;
      color:rgba(255,255,255,.45);
    }

    /* Center transport (tape) */
    .tape{
      background: radial-gradient(700px 260px at 50% 10%, rgba(195,168,107,.14), transparent 65%),
                  linear-gradient(180deg, #1a1d24, #12141a);
      border:1px solid rgba(255,255,255,.07);
      border-radius:16px;
      position:relative;
      overflow:hidden;
    }
    .tape-inner{
      padding:16px;
      height:100%;
      display:flex;
      flex-direction:column;
      gap:16px;
      box-sizing:border-box;
    }
    .meters{
      display:grid;
      grid-template-columns:1fr 1fr;
      gap:12px;
    }
    .meter{
      background: linear-gradient(180deg, rgba(255,255,255,.03), rgba(0,0,0,.25));
      border:1px solid rgba(255,255,255,.06);
      border-radius:14px;
      padding:12px;
      position:relative;
      overflow:hidden;
      height:140px;
    }
    .meter .label{
      position:absolute;
      top:10px; left:12px;
      font-size:11px;
      letter-spacing:.18em;
      color:rgba(255,255,255,.6);
      font-family: Orbitron, sans-serif;
    }
    .vu-face{
      position:absolute;
      left:12px; right:12px;
      bottom:12px; top:32px;
      background: radial-gradient(140px 90px at 50% 60%, rgba(255,255,255,.05), transparent 65%),
                  linear-gradient(180deg, rgba(255,255,255,.02), rgba(0,0,0,.35));
      border-radius:12px;
      border:1px solid rgba(255,255,255,.05);
    }
    .ticks{
      position:absolute;
      inset:12px;
      border-radius:10px;
    }
    .needle{
      position:absolute;
      width:2px;
      height:68px;
      background: linear-gradient(180deg, #f1e6c9, #a8883e);
      left:50%;
      bottom:22px;
      transform-origin: bottom center;
      transform: rotate(-50deg);
      box-shadow:0 0 10px rgba(195,168,107,.45);
    }
    .hub{
      position:absolute;
      width:14px; height:14px;
      border-radius:50%;
      background: radial-gradient(circle at 30% 30%, #fff, var(--accent) 55%, #3a2f18 100%);
      left:50%;
      bottom:16px;
      transform: translateX(-50%);
      box-shadow:0 0 0 4px rgba(195,168,107,.15);
    }
    .scale{
      position:absolute;
      left:14px; right:14px;
      top:50px;
      display:flex;
      justify-content:space-between;
      font-size:9px;
      color:rgba(255,255,255,.45);
    }

    /* Knobs */
    .knob-grid{
      display:grid;
      grid-template-columns: 1fr 1fr;
      gap:12px;
    }
    .knob{
      background: linear-gradient(180deg, rgba(255,255,255,.03), rgba(0,0,0,.25));
      border:1px solid rgba(255,255,255,.06);
      border-radius:14px;
      padding:12px;
      display:flex;
      gap:10px;
      align-items:center;
    }
    .knob .name{
      flex:1;
      font-size:11px;
      letter-spacing:.16em;
      color:rgba(255,255,255,.7);
      font-family: Orbitron, sans-serif;
    }
    .knob .val{
      font-size:11px;
      color:rgba(255,255,255,.55);
      letter-spacing:.08em;
      width:52px;
      text-align:right;
    }
    .dial{
      width:62px; height:62px;
      border-radius:50%;
      background: radial-gradient(circle at 30% 30%, #3a3f49, #171a20 70%);
      border:1px solid rgba(255,255,255,.08);
      box-shadow: inset 0 1px 2px rgba(255,255,255,.06),
                  0 10px 20px rgba(0,0,0,.5);
      position:relative;
      cursor:grab;
    }
    .dial:active{ cursor:grabbing; }
    .dial:after{
      content:"";
      position:absolute;
      width:3px; height:18px;
      background: linear-gradient(180deg, #f7f1de, var(--accent));
      top:8px;
      left:50%;
      transform: translateX(-50%);
      border-radius:2px;
      box-shadow:0 0 10px rgba(195,168,107,.35);
    }

    /* switches */
    .switch{
      display:flex;
      justify-content:space-between;
      align-items:center;
      padding:10px 12px;
      border-radius:14px;
      border:1px solid rgba(255,255,255,.06);
      background: linear-gradient(180deg, rgba(255,255,255,.03), rgba(0,0,0,.25));
      margin-bottom:10px;
    }
    .switch .sname{
      font-size:11px;
      letter-spacing:.16em;
      color:rgba(255,255,255,.7);
      font-family: Orbitron, sans-serif;
    }
    .toggle{
      width:48px; height:26px;
      border-radius:999px;
      background: rgba(255,255,255,.08);
      border:1px solid rgba(255,255,255,.12);
      position:relative;
      cursor:pointer;
    }
    .toggle .thumb{
      position:absolute;
      width:22px; height:22px;
      border-radius:50%;
      top:1px; left:2px;
      background: radial-gradient(circle at 30% 30%, #fff, #b7bcc6 55%, #3b404a 100%);
      box-shadow:0 8px 14px rgba(0,0,0,.45);
      transition: all .18s ease;
    }
    .toggle.on{
      background: rgba(88,192,139,.22);
      border-color: rgba(88,192,139,.35);
    }
    .toggle.on .thumb{ left:24px; }

    /* bottom strip */
    .bottom{
      display:flex;
      gap:12px;
      align-items:center;
      justify-content:space-between;
      padding-top:6px;
    }
    .bigbtn{
      flex:1;
      padding:12px 14px;
      border-radius:14px;
      text-align:center;
      font-family: Orbitron, sans-serif;
      letter-spacing:.12em;
      font-size:12px;
      color:rgba(255,255,255,.8);
      background: linear-gradient(180deg, rgba(195,168,107,.22), rgba(0,0,0,.25));
      border:1px solid rgba(195,168,107,.35);
      box-shadow:0 10px 22px rgba(0,0,0,.5);
      cursor:pointer;
    }
    .bigbtn:active{ transform: translateY(1px); }
    .hint{
      font-size:11px;
      color:rgba(255,255,255,.45);
      letter-spacing:.06em;
    }

    /* subtle corner screws */
    .screw{
      width:10px; height:10px;
      border-radius:50%;
      position:absolute;
      background: radial-gradient(circle at 30% 30%, #fff, rgba(255,255,255,.15) 45%, rgba(0,0,0,.7) 100%);
      border:1px solid rgba(255,255,255,.08);
      opacity:.65;
    }
    .s1{ top:10px; left:12px; }
    .s2{ top:10px; right:12px; }
    .s3{ bottom:10px; left:12px; }
    .s4{ bottom:10px; right:12px; }
  </style>
</head>
<body>
  <div class="frame">
    <div class="screw s1"></div><div class="screw s2"></div><div class="screw s3"></div><div class="screw s4"></div>

    <div class="topbar">
      <div class="brand">
        <div class="logo"></div>
        <div>
          <h1>ANALOGEXACT</h1>
          <div class="sub">ATR‑102 TAPE MACHINE</div>
        </div>
      </div>

      <div class="status">
        <div class="pill"><span class="dot"></span>LIVE</div>
        <div class="pill">VST3</div>
        <div class="pill">HTML UI</div>
      </div>
    </div>

    <div class="main">
      <!-- Left panel -->
      <div class="panel">
        <div class="panel-title">
          <div>INPUT / CAL</div>
          <span class="small">LEVEL</span>
        </div>

        <div class="knob-grid">
          <div class="knob" data-param="input">
            <div class="dial" data-val="0"></div>
            <div class="name">INPUT</div>
            <div class="val"><span class="readout">0.0</span> dB</div>
          </div>

          <div class="knob" data-param="bias">
            <div class="dial" data-val="0"></div>
            <div class="name">BIAS</div>
            <div class="val"><span class="readout">0.0</span></div>
          </div>

          <div class="knob" data-param="hf">
            <div class="dial" data-val="0"></div>
            <div class="name">HF EQ</div>
            <div class="val"><span class="readout">0.0</span> dB</div>
          </div>

          <div class="knob" data-param="lf">
            <div class="dial" data-val="0"></div>
            <div class="name">LF EQ</div>
            <div class="val"><span class="readout">0.0</span> dB</div>
          </div>
        </div>

        <div class="panel-title mt-4">
          <div>MODES</div>
          <span class="small">TOGGLES</span>
        </div>

        <div class="switch">
          <div class="sname">NOISE</div>
          <div class="toggle" data-toggle="noise"><div class="thumb"></div></div>
        </div>

        <div class="switch">
          <div class="sname">SATURATION</div>
          <div class="toggle on" data-toggle="sat"><div class="thumb"></div></div>
        </div>

        <div class="switch">
          <div class="sname">HISS</div>
          <div class="toggle" data-toggle="hiss"><div class="thumb"></div></div>
        </div>

        <div class="bottom">
          <div class="bigbtn" id="calBtn">CALIBRATE</div>
        </div>
        <div class="hint mt-2">Drag knobs up/down. Toggle switches. VU meters can be updated from C++.</div>
      </div>

      <!-- Center tape -->
      <div class="tape">
        <div class="tape-inner">
          <div class="meters">
            <div class="meter">
              <div class="label">LEFT</div>
              <div class="vu-face">
                <div class="scale"><span>-20</span><span>-10</span><span>0</span><span>+3</span></div>
                <div class="needle" id="needleL"></div>
                <div class="hub"></div>
              </div>
            </div>

            <div class="meter">
              <div class="label">RIGHT</div>
              <div class="vu-face">
                <div class="scale"><span>-20</span><span>-10</span><span>0</span><span>+3</span></div>
                <div class="needle" id="needleR"></div>
                <div class="hub"></div>
              </div>
            </div>
          </div>

          <div class="panel" style="flex:1;">
            <div class="panel-title">
              <div>TRANSPORT</div>
              <span class="small">TAPE</span>
            </div>

            <div class="grid grid-cols-3 gap-3">
              <div class="bigbtn" id="playBtn">PLAY</div>
              <div class="bigbtn" id="stopBtn">STOP</div>
              <div class="bigbtn" id="recBtn">REC</div>
            </div>

            <div class="mt-4 grid grid-cols-2 gap-3">
              <div class="knob" data-param="ips">
                <div class="dial" data-val="0"></div>
                <div class="name">IPS</div>
                <div class="val"><span class="readout">15</span></div>
              </div>
              <div class="knob" data-param="wow">
                <div class="dial" data-val="0"></div>
                <div class="name">WOW/FLUT</div>
                <div class="val"><span class="readout">0.0</span>%</div>
              </div>
            </div>

            <div class="hint mt-3">This UI is pure HTML/CSS/JS rendered inside the plugin editor.</div>
          </div>
        </div>
      </div>

      <!-- Right panel -->
      <div class="panel">
        <div class="panel-title">
          <div>OUTPUT</div>
          <span class="small">MIX</span>
        </div>

        <div class="knob-grid">
          <div class="knob" data-param="output">
            <div class="dial" data-val="0"></div>
            <div class="name">OUTPUT</div>
            <div class="val"><span class="readout">0.0</span> dB</div>
          </div>

          <div class="knob" data-param="mix">
            <div class="dial" data-val="0"></div>
            <div class="name">MIX</div>
            <div class="val"><span class="readout">100</span>%</div>
          </div>

          <div class="knob" data-param="comp">
            <div class="dial" data-val="0"></div>
            <div class="name">COMP</div>
            <div class="val"><span class="readout">0.0</span></div>
          </div>

          <div class="knob" data-param="limit">
            <div class="dial" data-val="0"></div>
            <div class="name">LIMIT</div>
            <div class="val"><span class="readout">0.0</span></div>
          </div>
        </div>

        <div class="panel-title mt-4">
          <div>OPTIONS</div>
          <span class="small">A/B</span>
        </div>

        <div class="switch">
          <div class="sname">A/B COMPARE</div>
          <div class="toggle" data-toggle="ab"><div class="thumb"></div></div>
        </div>

        <div class="switch">
          <div class="sname">BYPASS</div>
          <div class="toggle" data-toggle="bypass"><div class="thumb"></div></div>
        </div>

        <div class="bottom">
          <div class="bigbtn" id="resetBtn">RESET</div>
        </div>
        <div class="hint mt-2">You can wire knob values to DSP parameters next.</div>
      </div>
    </div>
  </div>

<script>
  // Simple knob dragging UI (front-end only).
  // Later: send these values to JUCE/C++ via a native bridge if desired.

  function clamp(v, a, b){ return Math.max(a, Math.min(b, v)); }

  const knobEls = document.querySelectorAll('.knob');
  knobEls.forEach(k => {
    const dial = k.querySelector('.dial');
    const readout = k.querySelector('.readout');

    let value = parseFloat(dial.dataset.val || "0"); // -1..+1 range (generic)
    let dragging = false;
    let startY = 0;
    let startVal = 0;

    function update(){
      // Map value (-1..1) to rotation
      const deg = (value * 135);
      dial.style.transform = `rotate(${deg}deg)`;

      // Display
      if (k.dataset.param === "mix") {
        const pct = Math.round(((value + 1) * 0.5) * 100);
        readout.textContent = pct;
      } else if (k.dataset.param === "ips") {
        const ips = value < 0 ? 7.5 : 15;
        readout.textContent = ips;
      } else {
        readout.textContent = (value * 10).toFixed(1);
      }
    }

    dial.addEventListener('mousedown', (e) => {
      dragging = true;
      startY = e.clientY;
      startVal = value;
      e.preventDefault();
    });

    window.addEventListener('mousemove', (e) => {
      if (!dragging) return;
      const delta = (startY - e.clientY) / 120; // sensitivity
      value = clamp(startVal + delta, -1, 1);
      update();
    });

    window.addEventListener('mouseup', () => dragging = false);

    update();
  });

  // Toggles
  document.querySelectorAll('.toggle').forEach(t => {
    t.addEventListener('click', () => t.classList.toggle('on'));
  });

  // Buttons
  document.getElementById('resetBtn').addEventListener('click', () => location.reload());
  document.getElementById('calBtn').addEventListener('click', () => alert("Calibrate (UI-only placeholder)."));
  document.getElementById('playBtn').addEventListener('click', () => console.log("PLAY"));
  document.getElementById('stopBtn').addEventListener('click', () => console.log("STOP"));
  document.getElementById('recBtn').addEventListener('click', () => console.log("REC"));

  // VU meter update hook (call from C++ by evaluating JS):
  // window.setVUMeters(left, right) expects 0..1 linear values.
  window.setVUMeters = function (left, right)
  {
    const nL = document.getElementById("needleL");
    const nR = document.getElementById("needleR");

    // map 0..1 -> -50..+20 degrees (approx)
    const mapNeedle = (x) => (-50 + (clamp(x, 0, 1) * 70));
    nL.style.transform = `rotate(${mapNeedle(left)}deg)`;
    nR.style.transform = `rotate(${mapNeedle(right)}deg)`;
  };

  // Demo motion (remove when C++ drives meters)
  let t = 0;
  setInterval(() => {
    t += 0.06;
    const l = 0.5 + 0.35 * Math.sin(t);
    const r = 0.5 + 0.35 * Math.sin(t * 1.2 + 1.0);
    window.setVUMeters(l, r);
  }, 33);

</script>
</body>
</html>)JUCEHTML";

    return kHtml;
}

static juce::String makeDataUrlFromHtmlUtf8 (const char* htmlUtf8)
{
    // Use base64 so we don't have to URL-escape the HTML.
    const auto len = std::strlen (htmlUtf8);
    const auto b64 = juce::Base64::toBase64 (htmlUtf8, len);
    return "data:text/html;base64," + b64;
}

//==============================================================================
HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
   #if JUCE_MAJOR_VERSION >= 7
    webView = std::make_unique<juce::WebBrowserComponent> (juce::WebBrowserComponent::Options {});
   #else
    // Older JUCE fallback (should not be hit in your case).
    webView = std::make_unique<juce::WebBrowserComponent> (false);
   #endif

    addAndMakeVisible (*webView);

    // Load the working UI immediately.
    const auto uiUrl = makeDataUrlFromHtmlUtf8 (getWorkingUiHtmlUtf8());
    webView->goToURL (uiUrl);

    setResizable (true, true);
    setSize (980, 580);
}

HtmlToVstAudioProcessorEditor::~HtmlToVstAudioProcessorEditor() = default;

void HtmlToVstAudioProcessorEditor::paint (juce::Graphics& g)
{
    // A background in case the browser needs a moment to initialise.
    g.fillAll (juce::Colours::black);
}

void HtmlToVstAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
