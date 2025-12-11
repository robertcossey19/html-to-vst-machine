#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

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
      oversampling (2, 4,
                    dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                    true)
{
}

HtmlToVstAudioProcessor::~HtmlToVstAudioProcessor() = default;

//==============================================================================

const String HtmlToVstAudioProcessor::getName() const   { return JucePlugin_Name; }

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

    // Calibration values matching the WebAudio model
    const float uiInputDb      = 0.0f;
    const float uiOutputDb     = 0.0f;
    const float inputOffsetDb  = -5.0f;
    const float headroomDb     = -12.0f;
    const float fluxDb         = 2.6f;
    const float biasDb         = 1.8f;

    const float inputDbEff = uiInputDb + inputOffsetDb;
    const float inGainLin  = Decibels::decibelsToGain (inputDbEff);
    const float fluxLin    = Decibels::decibelsToGain (fluxDb);
    const float headroomLin= Decibels::decibelsToGain (headroomDb);
    const float outMakeupDb = 1.0f;
    const float outDbEff   = uiOutputDb + outMakeupDb;

    inputGain.setGainLinear    (inGainLin);
    fluxGain.setGainLinear     (fluxLin);
    headroomGain.setGainLinear (headroomLin);
    outputGain.setGainDecibels (outDbEff);

    // ============================
    // Bias Waveshaper (FIXED)
    // ============================
    biasShaper.functionToUse = [biasDb] (float x)
    {
        const float baseK = 1.6f;
        const float asym  = 1.0f - jlimit (-0.25f, 0.25f, biasDb * 0.05f);
        const float kPos  = baseK;
        const float kNeg  = baseK / asym;

        return x >= 0.0f ? std::tanh (x * kPos)
                         : std::tanh (x * kNeg);
    };

    // Tape head bump
    {
        using Coeff = dsp::IIR::Coefficients<float>;
        const float bumpFreq = 80.0f;
        const float bumpQ    = 1.1f;
        const float bumpGain = 2.0f + 0.4f;

        headBump.coefficients = Coeff::makePeakFilter (
            osRate, bumpFreq, bumpQ,
            Decibels::decibelsToGain (bumpGain)
        );
    }

    // Repro high-shelf tilt
    {
        using Coeff = dsp::IIR::Coefficients<float>;
        const float shelfFreq = 3180.0f;
        const float shelfQ    = 0.707f;
        const float shelfDb   = -0.75f;

        reproHighShelf.coefficients = Coeff::makeHighShelf (
            osRate, shelfFreq, shelfQ,
            Decibels::decibelsToGain (shelfDb)
        );
    }

    // ============================
    // Transformer Waveshaper (FIXED)
    // ============================
    transformerShape.functionToUse = [] (float x)
    {
        constexpr float k    = 3.0f;
        constexpr float gain = 0.98f;
        return std::tanh (x * k) * gain;
    };

    // Output lowpass ~ 22 kHz
    {
        using Coeff = dsp::IIR::Coefficients<float>;
        lowpassOut.coefficients = Coeff::makeLowPass (osRate, 22000.0f);
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

    // 16× oversampling (4 × 2× stages)
    auto osBlock = oversampling.processSamplesUp (block);
    dsp::ProcessContextReplacing<float> context (osBlock);

    // Full analog chain
    inputGain.process       (context);
    fluxGain.process        (context);
    headroomGain.process    (context);
    biasShaper.process      (context);
    headBump.process        (context);
    reproHighShelf.process  (context);
    transformerShape.process(context);
    lowpassOut.process      (context);
    outputGain.process      (context);

    oversampling.processSamplesDown (block);

    // Stereo Crosstalk
    if (buffer.getNumChannels() > 1)
    {
        const float ct   = Decibels::decibelsToGain (-40.0f);
        const float main = 1.0f - ct;
        const int n      = buffer.getNumSamples();

        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int i = 0; i < n; ++i)
        {
            const float L = left[i];
            const float R = right[i];

            left[i]  = main * L + ct * R;
            right[i] = main * R + ct * L;
        }
    }
}

//==============================================================================

bool HtmlToVstAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* HtmlToVstAudioProcessor::createEditor()
{
    return new HtmlToVstAudioProcessorEditor (*this);
}

//==============================================================================

void HtmlToVstAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream (destData, true).writeInt (0x41545231);
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
