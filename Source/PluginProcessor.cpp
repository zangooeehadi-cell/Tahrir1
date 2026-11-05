#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat>("enhance", "Enhance", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("autoTahrir", "Auto Tahrir", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("hold", "Hold Time", NormalisableRange<float>(100.0f, 1500.0f, 1.0f), 500.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("depth", "Pitch Depth", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("speed", "Speed", NormalisableRange<float>(3.0f, 12.0f, 0.01f), 7.0f));
    p.push_back (std::make_unique<AudioParameterChoice>("style", "Style",
        StringArray {"Persian", "Arabic", "Turkish"}, 0));
    return { p.begin(), p.end() };
}

SmartTahrirProcessor::SmartTahrirProcessor()
 : AudioProcessor (BusesProperties()
                   .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                   .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
   parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void SmartTahrirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    fixedLatencySamples = juce::jmax (8, (int) std::round (currentSampleRate * 0.008));
    setLatencySamples (fixedLatencySamples);
    delayCapacity = juce::jmax (fixedLatencySamples * 8, (int) std::ceil (currentSampleRate * 0.06) + samplesPerBlock + 16);
    delayMemory.assign ((size_t) juce::jmax (1, getTotalNumInputChannels()),
                        std::vector<float> ((size_t) delayCapacity, 0.0f));
    writePosition = 0;
    lfoPhase = 0.0;
    voicedHoldSamples = 0.0;
    gateEnvelope = 0.0f;
}

void SmartTahrirProcessor::releaseResources()
{
    delayMemory.clear();
}

float SmartTahrirProcessor::readDelay (int channel, float delaySamples) const noexcept
{
    if (channel < 0 || channel >= (int) delayMemory.size() || delayCapacity < 4)
        return 0.0f;

    const auto& line = delayMemory[(size_t) channel];
    float readPosition = (float) writePosition - delaySamples;
    while (readPosition < 0.0f)
        readPosition += (float) delayCapacity;
    while (readPosition >= (float) delayCapacity)
        readPosition -= (float) delayCapacity;

    const int indexA = (int) readPosition;
    const int indexB = (indexA + 1) % delayCapacity;
    const float fraction = readPosition - (float) indexA;
    return line[(size_t) indexA] + fraction * (line[(size_t) indexB] - line[(size_t) indexA]);
}

float SmartTahrirProcessor::makeTahrirCurve (float p, int style) const noexcept
{
    p -= std::floor (p);
    if (style == 1) // Arabic: rounded, flowing multi-turn ornament
        return 0.72f * std::sin (juce::MathConstants<float>::twoPi * p)
             + 0.20f * std::sin (juce::MathConstants<float>::twoPi * 2.0f * p + 0.45f);
    if (style == 2) // Turkish: quicker, more angular turns
        return 0.62f * std::sin (juce::MathConstants<float>::twoPi * p)
             + 0.30f * std::sin (juce::MathConstants<float>::twoPi * 3.0f * p + 0.8f);

    // Persian: asymmetric rise/fall contour, intentionally not a plain sine vibrato.
    if (p < 0.24f) return -0.25f + 1.10f * (p / 0.24f);
    if (p < 0.58f) return 0.85f - 1.55f * ((p - 0.24f) / 0.34f);
    if (p < 0.78f) return -0.70f + 0.92f * ((p - 0.58f) / 0.20f);
    return 0.22f - 0.47f * ((p - 0.78f) / 0.22f);
}

void SmartTahrirProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (buffer.getNumChannels(), (int) delayMemory.size());
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || delayCapacity <= 0)
        return;

    const float autoAmount = parameters.getRawParameterValue ("autoTahrir")->load();
    const float enhance = parameters.getRawParameterValue ("enhance")->load();
    const float holdMs = parameters.getRawParameterValue ("hold")->load();
    const float depth = parameters.getRawParameterValue ("depth")->load();
    const float speed = parameters.getRawParameterValue ("speed")->load();
    const int style = (int) parameters.getRawParameterValue ("style")->load();

    double energy = 0.0;
    int energyCount = 0;
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* in = buffer.getReadPointer (ch);
        for (int i = 0; i < samples; ++i)
        {
            energy += (double) in[i] * (double) in[i];
            ++energyCount;
        }
    }
    const float rms = energyCount > 0 ? (float) std::sqrt (energy / (double) energyCount) : 0.0f;
    const bool voicedEnergy = rms > 0.006f;

    if (voicedEnergy)
        voicedHoldSamples += samples;
    else
        voicedHoldSamples = 0.0;

    const double requiredHold = currentSampleRate * ((double) holdMs / 1000.0);
    const float targetGate = (autoAmount > 0.1f && voicedHoldSamples >= requiredHold) ? 1.0f : 0.0f;
    const float attack = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * 0.025));
    const float release = 1.0f - std::exp (-1.0f / (float) (currentSampleRate * 0.12));

    const float baseDelay = (float) fixedLatencySamples;
    const float modulationSeconds = 0.00075f * (depth / 100.0f) * (0.35f + 0.65f * enhance / 100.0f);
    const float modulationSamples = modulationSeconds * (float) currentSampleRate;
    const float wet = autoAmount / 100.0f;

    for (int i = 0; i < samples; ++i)
    {
        gateEnvelope += (targetGate - gateEnvelope) * (targetGate > gateEnvelope ? attack : release);
        const float curve = makeTahrirCurve ((float) lfoPhase, style);
        const float modDelay = juce::jlimit (3.0f, (float) delayCapacity - 3.0f,
                                             baseDelay + curve * modulationSamples * gateEnvelope);

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const float input = data[i];
            auto& line = delayMemory[(size_t) ch];
            line[(size_t) writePosition] = input;

            const float stableDelayed = readDelay (ch, baseDelay);
            const float ornamented = readDelay (ch, modDelay);
            data[i] = stableDelayed + (ornamented - stableDelayed) * wet;
        }

        writePosition = (writePosition + 1) % delayCapacity;
        lfoPhase += (double) speed / currentSampleRate;
        if (lfoPhase >= 1.0)
            lfoPhase -= std::floor (lfoPhase);
    }

    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, samples);
}

juce::AudioProcessorEditor* SmartTahrirProcessor::createEditor()
{
    return new SmartTahrirEditor (*this);
}

void SmartTahrirProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SmartTahrirProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartTahrirProcessor();
}
