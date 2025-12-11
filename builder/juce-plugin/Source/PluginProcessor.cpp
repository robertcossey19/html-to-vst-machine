#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
   #if ! JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
   #else
    : AudioProcessor (BusesProperties()),
   #endif
      oversampling (2, 4,
                    dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                    true) // serial
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const String HtmlToVstAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HtmlToVstAudioProcessor::acceptsMidi() const       { return false; }
bool HtmlToVstAudioProcessor::producesMidi() const      { return false; }
bool HtmlToVstAudioProcessor::isMidiEffect() const      { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HtmlToVstAudioProcessor::getNumPrograms()           { return 1; }
int HtmlToVstAudioProcessor::getCurrentProgram()        { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int)   {}
const String HtmlToVstAudioProcessor::getProgramName (int) { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const String&) {}

//==============================================================================

#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // We support only stereo → stereo
    if (layouts.getMainInputChannelSet()  != AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//==============================================================================

void HtmlToVstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    numChannels       = jmax (1, getTotalNumOutputChannels());

    const auto osFactor = (uint32) oversampling.getOversamplingFactor();

    dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * (double) osFactor;
    spec.maximumBlockSize = (uint32) (samplesPerBlock * osFactor);
    spec.numChannels      = (uint32) numChannels;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    inputGain.prepare      (spec);
    fluxGain.prepare       (spec);
    headroomGain.prepare   (spec);
    outputGain.prepare     (spec);

    headBump.prepare       (spec);
    reproHighShelf.prepare (spec);
    lowpassOut.prepare     (spec);

    biasShaper.prepare     (spec);
    transformerShape.prepare (spec);

    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();
    biasShaper.reset();
    transformerShape.reset();

    inputGain.setRampDurationSeconds (0.01);
    fluxGain.setRampDurationSeconds  (0.01);
    headroomGain.setRampDurationSeconds (0.01);
    outputGain.setRampDurationSeconds   (0.01);

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

    // === DEFAULT CALIBRATION (mirrors the PWA version) =======================
    //
    // Speed:    15 ips
    // Flux:     250 nWb/m  (~ +2.6 dB)
    // Tape:     456
    // Headroom: -12 dB
    // UI input  0 dB  → record path -5 dB
    // UI output 0 dB  → +1 dB transformer makeup
    //
    const float uiInputDb      = 0.0f;
    const float uiOutputDb     = 0.0f;
    const float inputOffsetDb  = -5.0f;   // Web UI 0 dB == -5 dB at record path
    const float headroomDb     = -12.0f;
    const float fluxDb         =  2.6f;   // 250 nWb/m
    const float biasDb         =  1.8f;   // over-bias calibration
    const float outMakeupDb    =  1.0f;   // transformer + flux compensation

    // --- Gain structure ------------------------------------------------------
    const float inputDbEff    = uiInputDb + inputOffsetDb;
    const float inGainLin     = Decibels::decibelsToGain (inputDbEff);
    const float fluxLin       = Decibels::decibelsToGain (fluxDb);
    const float headroomLin   = Decibels::decibelsToGain (headroomDb);
    const float outDbEff      = uiOutputDb + outMakeupDb;

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    // --- Bias non-linearity (odd / even control) ----------------------------
    biasShaper.functionToUse = [biasDb] (float x)
    {
        const float baseK = 1.6f;
        const float asym  = 1.0f - jlimit (-0.25f, 0.25f, biasDb * 0.05f); // ±0.25
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };

    // --- Tape EQ: LF head bump + repro HF shelf -----------------------------
    using Coeff = dsp::IIR::Coefficients<float>;

    // LF head bump (NAB @ 15 ips + 456 bump)
    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpDb   = 2.0f + 0.4f; // base + 456 profile

        auto bumpCoeffs = Coeff::makePeakFilter (osRate,
                                                 bumpFreq,
                                                 bumpQ,
                                                 Decibels::decibelsToGain (bumpDb));
        headBump.coefficients = bumpCoeffs;
    }

    // Repro HF shelf (NAB / 15 ips, slightly negative tilt)
    {
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;

        auto shelfCoeffs = Coeff::makeHighShelf (osRate,
                                                 shelfFreq,
                                                 shelfQ,
                                                 Decibels::decibelsToGain (shelfDb));
        reproHighShelf.coefficients = shelfCoeffs;
    }

    // Transformer output curve (approximate 3rd/5th harmonic pattern)
    transformerShape.functionToUse = [] (float x)
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // Final bandwidth limit at oversampled rate (~22 kHz)
    {
        const float lpFreq = 22000.0f;
        auto lpCoeffs = Coeff::makeLowPass (osRate, lpFreq);
        lowpassOut.coefficients = lpCoeffs;
    }
}

//==============================================================================

void HtmlToVstAudioProcessor::processBlock (AudioBuffer<float>& buffer,
                                            MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    ignoreUnused (midi);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 0)
        return;

    dsp::AudioBlock<float> block (buffer);

    // --- Upsample to 768 kHz (16x) ------------------------------------------
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> context (osBlock);

    // --- Core ATR-style chain at high rate ----------------------------------
    inputGain.process       (context);
    fluxGain.process        (context);
    headroomGain.process    (context);
    biasShaper.process      (context);
    headBump.process        (context);
    reproHighShelf.process  (context);
    transformerShape.process(context);
    lowpassOut.process      (context);
    outputGain.process      (context);

    // --- Downsample back to project rate ------------------------------------
    oversampling.processSamplesDown (block);

    // --- Stereo crosstalk matrix (post-processing) --------------------------
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    if (right != nullptr)
    {
        const int n = buffer.getNumSamples();
        const float ct   = Decibels::decibelsToGain (-40.0f);   // ~ -40 dB crosstalk
        const float main = 1.0f - ct;                           // keep level ~ unity

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            left[i]  = main * l + ct * r;
            right[i] = main * r + ct * l;
        }
    }

    // --- VU metering (RMS → dB, post-crosstalk) -----------------------------
    if (buffer.getNumChannels() >= 2)
    {
        const int n = buffer.getNumSamples();
        const float* readL = buffer.getReadPointer (0);
        const float* readR = buffer.getReadPointer (1);

        double sumL = 0.0;
        double sumR = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float l = readL[i];
            const float r = readR[i];
            sumL += (double) l * (double) l;
            sumR += (double) r * (double) r;
        }

        const float rmsL = n > 0 ? std::sqrt ((float) (sumL / n)) : 0.0f;
        const float rmsR = n > 0 ? std::sqrt ((float) (sumR / n)) : 0.0f;

        currentVUL = Decibels::gainToDecibels (rmsL, -80.0f);
        currentVUR = Decibels::gainToDecibels (rmsR, -80.0f);
    }
    else
    {
        currentVUL = currentVUR = -80.0f;
    }
}

//==============================================================================

bool HtmlToVstAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // Placeholder: simple magic number so future versions can detect old state.
    MemoryOutputStream (destData, true).writeInt (0x41545231); // "ATR1"
}

void HtmlToVstAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ignoreUnused (data, sizeInBytes);
}

//==============================================================================

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HtmlToVstAudioProcessor();
}
