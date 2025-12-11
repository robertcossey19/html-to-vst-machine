#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
    // Naive percent-encoding to make the HTML safe to embed in a data: URL
    String urlEncodeForDataURI (const String& s)
    {
        String encoded;
        encoded.preallocateBytes (s.getNumBytesAsUTF8() * 3 + 32);

        const char* raw = s.toRawUTF8();

        for (auto* c = raw; *c != 0; ++c)
        {
            const unsigned char ch = (unsigned char) *c;

            const bool needsEscape =
                (ch <= 0x20) || (ch >= 0x7f) ||
                ch == '%' || ch == '#' || ch == '?' ||
                ch == '&' || ch == '+' || ch == '"';

            if (! needsEscape)
            {
                encoded << (char) ch;
            }
            else
            {
                encoded << '%';
                encoded << String::toHexString ((int) ch).paddedLeft ('0', 2);
            }
        }

        return encoded;
    }
}

//==============================================================================

HtmlToVstAudioProcessorEditor::HtmlToVstAudioProcessorEditor (HtmlToVstAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    addAndMakeVisible (webView);
    webView.setOpaque (true);
    webView.setInterceptsMouseClicks (true, true);

    // Load embedded HTML UI from BinaryData (ampex_ui.html)
    String html = String::fromUTF8 (BinaryData::ampex_ui_html,
                                    BinaryData::ampex_ui_htmlSize);

    const String dataUrl = "data:text/html;charset=utf-8," + urlEncodeForDataURI (html);
    webView.goToURL (dataUrl);

    // Match the web UI aspect reasonably
    setSize (1000, 640);

    // Drive VU meters ~30 Hz
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

void HtmlToVstAudioProcessorEditor::timerCallback()
{
    // Processor exposes block RMS in dB (approx. -80..+12)
    const float vuL = audioProcessor.getCurrentVUL();
    const float vuR = audioProcessor.getCurrentVUR();

    // Defensive clamp
    const auto clampDb = [] (float db)
    {
        if (db < -80.0f) return -80.0f;
        if (db >  12.0f) return  12.0f;
        return db;
    };

    const float clL = clampDb (vuL);
    const float clR = clampDb (vuR);

    // This JS hook is implemented in ampex_ui.html
    String js;
    js << "if (window.HtmlToVstPluginSetMeters) { "
       << "window.HtmlToVstPluginSetMeters("
       << String (clL, 4) << ", "
       << String (clR, 4) << "); "
       << "}";

    webView.evaluateJavascript (js, nullptr);
}
