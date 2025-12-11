#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================

HtmlToVstAudioProcessor::HtmlToVstAudioProcessor()
   : AudioProcessor (
        BusesProperties()
            .withInput  ("Input",  AudioChannelSet::stereo(), true)
            .withOutput ("Output", AudioChannelSet::stereo(), true)
     )
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const String HtmlToVstAudioProcessor::getName() const                  { return JucePlugin_Name; }
bool HtmlToVstAudioProcessor::acceptsMidi() const                      { return false; }
bool HtmlToVstAudioProcessor::producesMidi() const                     { return false; }
bool HtmlToVstAudioProcessor::isMidiEffect() const                     { return false; }
double HtmlToVstAudioProcessor::getTailLengthSeconds() const           { return 0.0; }

int HtmlToVstAudioProcessor::getNumPrograms()                          { return 1; }
int HtmlToVstAudioProcessor::getCurrentProgram()                       { return 0; }
void HtmlToVstAudioProcessor::setCurrentProgram (int)                  {}
const String HtmlToVstAudioProcessor::getProgramName (int)             { return {}; }
void HtmlToVstAudioProcessor::changeProgramName (int, const String&)   {}

//==============================================================================

#if ! JucePlugin_PreferredChannelConfigurations
bool HtmlToVstAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
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

    inputGain.prepare       (spec);
    fluxGain.prepare        (spec);
    headroomGain.prepare    (spec);
    headBump.prepare        (spec);
    reproHighShelf.prepare  (spec);
    biasShaper.prepare      (spec);
    transformerShape.prepare(spec);
    lowpassOut.prepare      (spec);
    outputGain.prepare      (spec);

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
    // Speed:      15 ips
    // Flux:       250 nWb/m
    // Tape:       456
    // Headroom:   -12 dB
    // Input UI:   0 dB, but record path had -5 dB offset
    // Output UI:  0 dB
    //
    const float uiInputDb      = 0.0f;
    const float uiOutputDb     = 0.0f;
    const float inputOffsetDb  = -5.0f;
    const float headroomDb     = -12.0f;
    const float fluxDb         =  2.6f;    // 250 nWb/m ~= +2.6 dB
    const float biasDb         =  1.8f;

    const float inputDbEff  = uiInputDb + inputOffsetDb;
    const float inGainLin   = Decibels::decibelsToGain (inputDbEff);
    const float fluxLin     = Decibels::decibelsToGain (fluxDb);
    const float headroomLin = Decibels::decibelsToGain (headroomDb);
    const float outMakeupDb = 1.0f;   // transformer + flux comp
    const float outDbEff    = uiOutputDb + outMakeupDb;

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    // --- Bias curve / asymmetry (matches WebAudio createBiasCurve) ---
    biasShaper.functionToUse = [biasDb] (float x)
    {
        const float baseK = 1.6f;
        const float asym  = 1.0f - jlimit (-0.25f, 0.25f, biasDb * 0.05f);
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };

    // --- Tape EQ: head bump + repro shelf (NAB @ 15 ips, 456) ---
    using Coeff = dsp::IIR::Coefficients<float>;

    {
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGain = 2.0f + 0.4f; // base + extra for 456

        auto bumpCoeffs = Coeff::makePeakFilter (osRate,
                                                 bumpFreq,
                                                 bumpQ,
                                                 Decibels::decibelsToGain (bumpGain));

        headBump.coefficients = bumpCoeffs;
    }

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

    // --- Transformer output curve (matches createTransformerOutCurve) ---
    transformerShape.functionToUse = [] (float x)
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // --- Output LPF ~22 kHz at 768 kHz OS ---
    {
        const float lpFreq = 22000.0f;
        auto lpCoeffs = Coeff::makeLowPass (osRate, lpFreq);
        lowpassOut.coefficients = lpCoeffs;
    }
}

//==============================================================================

void HtmlToVstAudioProcessor::updateVuFromBuffer (AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    auto calcDb = [numSamples] (const float* data) -> float
    {
        double sum = 0.0;
        for (int i = 0; i < numSamples; ++i)
            sum += (double) data[i] * (double) data[i];

        auto rms = std::sqrt (sum / jmax (1, numSamples));
        return Decibels::gainToDecibels ((float) rms, -100.0f);
    };

    const float lDb = buffer.getNumChannels() > 0 ? calcDb (buffer.getReadPointer (0))
                                                 : -90.0f;
    const float rDb = buffer.getNumChannels() > 1 ? calcDb (buffer.getReadPointer (1))
                                                 : lDb;

    currentVUL.store (lDb);
    currentVUR.store (rDb);
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

    // --- ATR-102 style chain @ 768 kHz ---
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

    // --- Stereo crosstalk matrix (after downsampling, matches WebAudio) ---
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    if (right != nullptr)
    {
        const int n = buffer.getNumSamples();
        const float ct   = Decibels::decibelsToGain (-40.0f);   // ~ -40 dB
        const float main = 1.0f - ct;                           // keep unity-ish

        for (int i = 0; i < n; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            left[i]  = main * l + ct * r;
            right[i] = main * r + ct * l;
        }
    }

    // --- Update VU (dB RMS) for UI bridge ---
    updateVuFromBuffer (buffer);
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
