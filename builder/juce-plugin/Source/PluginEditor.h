#pragma once

#include "PluginProcessor.h"

#if __has_include(<JuceHeader.h>)
  #include <JuceHeader.h>
#else
  #include <juce_gui_extra/juce_gui_extra.h>
#endif

class HtmlToVstPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit HtmlToVstPluginAudioProcessorEditor (HtmlToVstPluginAudioProcessor&);
    ~HtmlToVstPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadHtmlUi();
    void pushMetersToUi();

    HtmlToVstPluginAudioProcessor& processor;

   #if defined(JUCE_MAJOR_VERSION) && (JUCE_MAJOR_VERSION >= 7)
    class BrowserListener : public juce::WebBrowserComponent::PageLoadListener
    {
    public:
        explicit BrowserListener (HtmlToVstPluginAudioProcessor& p) : proc (p) {}

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            if (newURL.startsWithIgnoreCase ("juce:"))
            {
                proc.handleUiMessageUrl (newURL);
                return false; // cancel navigation
            }
            return true;
        }

    private:
        HtmlToVstPluginAudioProcessor& proc;
    };

    BrowserListener browserListener;
    juce::WebBrowserComponent webView;
   #else
    class InterceptingBrowser : public juce::WebBrowserComponent
    {
    public:
        explicit InterceptingBrowser (HtmlToVstPluginAudioProcessor& p) : proc (p) {}

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            if (newURL.startsWithIgnoreCase ("juce:"))
            {
                proc.handleUiMessageUrl (newURL);
                return false;
            }
            return true;
        }

    private:
        HtmlToVstPluginAudioProcessor& proc;
    };

    InterceptingBrowser webView;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HtmlToVstPluginAudioProcessorEditor)
};
