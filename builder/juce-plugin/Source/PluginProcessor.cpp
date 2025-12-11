#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
   : juce::AudioProcessor (
#if ! JucePlugin_PreferredChannelConfigurations
       BusesProperties()
           .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#else
       BusesProperties()
#endif
     ),
     oversampling (2, 4,
                   juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                   true)
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const juce::String HtmlToVstAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HtmlToVstAudioProcessor::acceptsMidi() const       { return false; }
bool HtmlToVstAudioProcessor::producesMidi() const      { return false; }
bool HtmlToVstAudioProcessor::isMidiEffect() const      { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  HtmlToVstAudioProcessor::getNumPrograms()              { return 1; }
int  HtmlToVstAudioProcessor::getCurrentProgram()           { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int)       {}
const juce::String HtmlToVstAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================

#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // stereo in / stereo out only
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//==============================================================================

void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    numChannels       = juce::jmax (1, getTotalNumOutputChannels());

    const auto factor = (size_t) oversampling.getOversamplingFactor();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * (double) factor;
    spec.maximumBlockSize = (juce::uint32) (samplesPerBlock * (int) factor);
    spec.numChannels      = (juce::uint32) numChannels;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    inputGain.reset();
    fluxGain.reset();
    headroomGain.reset();
    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();
    outputGain.reset();

    inputGain.prepare       (spec);
    fluxGain.prepare        (spec);
    headroomGain.prepare    (spec);
    headBump.prepare        (spec);
    reproHighShelf.prepare  (spec);
    lowpassOut.prepare      (spec);
    outputGain.prepare      (spec);

    updateDSP();
}

void HtmlToVstAudioProcessor::releaseResources()
{
}

//==============================================================================

void HtmlToVstAudioProcessor::updateDSP()
{
    if (currentSampleRate <= 0.0)
        return;

    const auto osFactor = (double) oversampling.getOversamplingFactor();
    const auto osRate   = currentSampleRate * osFactor;

    // ---- DEFAULT CALIBRATION TO MATCH WEB VERSION ----
    //
    // Speed:     15 ips
    // Flux:      250 nWb/m  (~ +2.6 dB)
    // Tape:      456
    // Headroom:  -12 dB
    // UI Input:  0 dB (but record path has -5 dB offset)
    // UI Output: 0 dB, +1 dB makeup for transformer core
    //
    const float uiInputDb      = 0.0f;
    const float uiOutputDb     = 0.0f;
    const float inputOffsetDb  = -5.0f;     // UI 0 dB = -5 dB at record path
    const float headroomDb     = -12.0f;
    const float fluxDb         =  2.6f;     // 250 nWb/m
    const float outMakeupDb    =  1.0f;     // transformer & flux compensation

    const float inputDbEff = uiInputDb + inputOffsetDb;
    const float inGainLin  = juce::Decibels::decibelsToGain (inputDbEff);
    const float fluxLin    = juce::Decibels::decibelsToGain (fluxDb);
    const float headroomLin= juce::Decibels::decibelsToGain (headroomDb);
    const float outDbEff   = uiOutputDb + outMakeupDb;

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    using Coeff = juce::dsp::IIR::Coefficients<float>;

    // --- Head bump (NAB-style) + extra for 456 ---
    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGain = 2.0f + 0.4f; // base + extra for 456

        auto bumpCoeffs = Coeff::makePeakFilter (osRate,
                                                 bumpFreq,
                                                 bumpQ,
                                                 juce::Decibels::decibelsToGain (bumpGain));

        headBump.coefficients = bumpCoeffs;
    }

    // --- Repro high shelf (NAB @ 15 ips) ---
    {
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;

        auto shelfCoeffs = Coeff::makeHighShelf (osRate,
                                                 shelfFreq,
                                                 shelfQ,
                                                 juce::Decibels::decibelsToGain (shelfDb));

        reproHighShelf.coefficients = shelfCoeffs;
    }

    // --- Output bandwidth limit ~22 kHz at 768 kHz oversampled ---
    {
        const float lpFreq = 22000.0f;
        auto lpCoeffs = Coeff::makeLowPass (osRate, lpFreq);
        lowpassOut.coefficients = lpCoeffs;
    }
}

//==============================================================================

void HtmlToVstAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 0)
        return;

    juce::dsp::AudioBlock<float> block (buffer);

    // --- 16x oversampling up ---
    auto osBlock = oversampling.processSamplesUp (block);
    juce::dsp::ProcessContextReplacing<float> context (osBlock);

    // Core gain structure in the oversampled domain
    inputGain.process       (context);
    fluxGain.process        (context);
    headroomGain.process    (context);

    // --- Bias + transformer waveshaping (manual, no WaveShaper::function) ---
    {
        const auto numCh      = osBlock.getNumChannels();
        const auto numSamples = osBlock.getNumSamples();

        // Bias asymmetry curve (from WebAudio version)
        const float baseK = 1.6f;
        const float asym  = 1.0f - juce::jlimit (-0.25f, 0.25f, biasDb * 0.05f);
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            float* data = osBlock.getChannelPointer (ch);

            for (size_t i = 0; i < numSamples; ++i)
            {
                float x = data[i];

                // Bias asymmetry – under/over-bias alters even/odd balance
                float shaped = (x >= 0.0f ? std::tanh (x * kPos)
                                          : std::tanh (x * kNeg));

                // Transformer core (moderate 3rd/5th emphasis)
                constexpr float transformerK    = 3.0f;
                constexpr float transformerGain = 0.98f;
                shaped = std::tanh (shaped * transformerK) * transformerGain;

                data[i] = shaped;
            }
        }
    }

    // --- Head & repro EQ @ 768 kHz ---
    headBump.process       (context);
    reproHighShelf.process (context);
    lowpassOut.process     (context);
    outputGain.process     (context);

    // --- Downsample back to project rate ---
    oversampling.processSamplesDown (block);

    // --- Stereo crosstalk matrix (post-processing at session rate) ---
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    const int n = buffer.getNumSamples();

    if (right != nullptr)
    {
        const float ct   = juce::Decibels::decibelsToGain (-40.0f); // ~-40 dB L<->R
        const float main = 1.0f - ct;                               // keep level ≈ unity

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            left[i]  = main * l + ct * r;
            right[i] = main * r + ct * l;
        }
    }

    // --- VU RMS (linear) for left / right ---
    {
        double sumL = 0.0;
        double sumR = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            sumL += (double) l * (double) l;

            if (right != nullptr)
            {
                const float r = right[i];
                sumR += (double) r * (double) r;
            }
        }

        const double denom = (double) juce::jmax (1, n);
        const float rmsL = std::sqrt ((float) (sumL / denom));
        const float rmsR = right != nullptr ? std::sqrt ((float) (sumR / denom))
                                            : rmsL;

        currentVUL.store (rmsL);
        currentVUR.store (rmsR);
    }
}

//==============================================================================

bool HtmlToVstAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // For now: no parameters, just an ID blob.
    juce::MemoryOutputStream (destData, true).writeInt ((int) 0x41545231); // "ATR1"
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
