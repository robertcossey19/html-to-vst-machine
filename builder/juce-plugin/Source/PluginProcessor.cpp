#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

using namespace juce;

//==============================================================================
// Static calibration + non-linear curves matching the WebAudio implementation.
//==============================================================================

namespace
{
    constexpr float kUiInputDbDefault  = 0.0f;
    constexpr float kUiOutputDbDefault = 0.0f;
    constexpr float kInputOffsetDb     = -5.0f;  // GUI 0dB => -5dB at record path
    constexpr float kHeadroomDb        = -12.0f;
    constexpr float kFluxDb            = 2.6f;   // 250 nWb/m over ref
    constexpr float kBiasDb            = 1.8f;   // over-bias calibration

    float biasCurveFunction (float x) noexcept
    {
        constexpr float baseK = 1.6f;
        const float asym = 1.0f
                         - juce::jlimit (-0.25f, 0.25f, kBiasDb * 0.05f); // ±0.25
        const float kPos = baseK;
        const float kNeg = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    }

    float transformerCurveFunction (float x) noexcept
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    }
}

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
    :
#if ! JucePlugin_PreferredChannelConfigurations
      AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
#else
      AudioProcessor (BusesProperties()),
#endif
      // 16x oversampling = 4 stages of 2x
      oversampling (2, 4,
                    dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                    true)
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const String HtmlToVstAudioProcessor::getName() const
{
    // Avoid needing JucePlugin_Name macro; keep host label simple and stable.
    return "HTML to VST Plugin";
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
    // We support only stereo <-> stereo
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

    dsp::ProcessSpec spec;
    spec.sampleRate       = currentSampleRate * (double) oversampling.getOversamplingFactor();
    spec.maximumBlockSize = (uint32) (samplesPerBlock * oversampling.getOversamplingFactor());
    spec.numChannels      = (uint32) numChannels;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);

    inputGain.prepare       (spec);
    fluxGain.prepare        (spec);
    headroomGain.prepare    (spec);
    headBump.prepare        (spec);
    reproHighShelf.prepare  (spec);
    biasShaper.prepare      (spec);
    transformerShape.prepare(spec);
    lowpassOut.prepare      (spec);
    outputGain.prepare      (spec);

    // Linear phase where possible
    headBump.reset();
    reproHighShelf.reset();
    lowpassOut.reset();

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
    // Speed: 15 ips
    // Flux:  250 nWb/m
    // Tape:  456
    // Headroom: -12 dB
    // Input UI = 0 dB, but original UI had -5 dB offset in record path
    // Output UI = 0 dB
    // Transformer: ON
    //
    const float uiInputDb   = kUiInputDbDefault;
    const float uiOutputDb  = kUiOutputDbDefault;
    const float inputDbEff  = uiInputDb + kInputOffsetDb;
    const float fluxDb      = kFluxDb;
    const float headroomDb  = kHeadroomDb;
    const float outMakeupDb = 1.0f;  // transformer & flux compensation
    const float outDbEff    = uiOutputDb + outMakeupDb;

    const float inGainLin    = Decibels::decibelsToGain (inputDbEff);
    const float fluxLin      = Decibels::decibelsToGain (fluxDb);
    const float headroomLin  = Decibels::decibelsToGain (headroomDb);

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    // --- Bias curves / asymmetry ---
    biasShaper.functionToUse = biasCurveFunction;

    // --- Tape EQ: head bump + repro shelf (NAB @ 15 ips, 456) ---
    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGain = 2.0f + 0.4f; // base + extra for 456

        using Coeff = dsp::IIR::Coefficients<float>;

        headBump.coefficients = Coeff::makePeakFilter (osRate,
                                                       bumpFreq,
                                                       bumpQ,
                                                       Decibels::decibelsToGain (bumpGain));
    }

    {
        // Repro high shelf: NAB @ 15 ips, slightly negative tilt
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;

        using Coeff = dsp::IIR::Coefficients<float>;

        reproHighShelf.coefficients = Coeff::makeHighShelf (osRate,
                                                            shelfFreq,
                                                            shelfQ,
                                                            Decibels::decibelsToGain (shelfDb));
    }

    // --- Transformer output curve ---
    transformerShape.functionToUse = transformerCurveFunction;

    // --- Output bandwidth limit ~ 22 kHz at 768 kHz OS ---
    {
        using Coeff = dsp::IIR::Coefficients<float>;
        const float lpFreq = 22000.0f;
        lowpassOut.coefficients = Coeff::makeLowPass (osRate, lpFreq);
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

    // --- 16x oversampling up ---
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> context (osBlock);

    // --- Core ATR-style chain at 768 kHz ---
    inputGain.process       (context);
    fluxGain.process        (context);
    headroomGain.process    (context);
    biasShaper.process      (context);
    headBump.process        (context);
    reproHighShelf.process  (context);
    transformerShape.process(context);
    lowpassOut.process      (context);
    outputGain.process      (context);

    // --- Downsample back to project rate ---
    oversampling.processSamplesDown (block);

    // --- Stereo crosstalk matrix (after downsampling) + VU metering ---
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    const int n = buffer.getNumSamples();

    if (right != nullptr)
    {
        const float ct   = Decibels::decibelsToGain (-40.0f);   // ~ -40 dB crosstalk
        const float main = 1.0f - ct;                           // keep level ~ unity

        double sumL = 0.0;
        double sumR = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            const float outL = main * l + ct * r;
            const float outR = main * r + ct * l;

            left[i]  = outL;
            right[i] = outR;

            sumL += (double) outL * (double) outL;
            sumR += (double) outR * (double) outR;
        }

        const float rmsL = n > 0 ? std::sqrt ((float) (sumL / (double) n)) : 0.0f;
        const float rmsR = n > 0 ? std::sqrt ((float) (sumR / (double) n)) : 0.0f;

        const float dbL  = Decibels::gainToDecibels (rmsL, -80.0f);
        const float dbR  = Decibels::gainToDecibels (rmsR, -80.0f);

        currentVUL.store (dbL);
        currentVUR.store (dbR);
    }
    else
    {
        double sum = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float s = left[i];
            sum += (double) s * (double) s;
        }

        const float rms = n > 0 ? std::sqrt ((float) (sum / (double) n)) : 0.0f;
        const float db  = Decibels::gainToDecibels (rms, -80.0f);

        currentVUL.store (db);
        currentVUR.store (db);
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
    // For now: no parameters, just placeholder blob.
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
