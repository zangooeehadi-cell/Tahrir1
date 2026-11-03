#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat>("enhance", "Enhance", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("autoTahrir", "Auto Tahrir", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("hold", "Hold Time", NormalisableRange<float>(100.0f, 1500.0f, 1.0f), 500.0f));
    p.push_back (std::make_unique<AudioParameterFloat>("depth", "Depth", NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));
    return { p.begin(), p.end() };
}

SmartTahrirProcessor::SmartTahrirProcessor()
 : AudioProcessor (BusesProperties()
                   .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                   .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
   parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void SmartTahrirProcessor::prepareToPlay (double, int) {}
void SmartTahrirProcessor::releaseResources() {}

void SmartTahrirProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // Prototype 0.1: transparent pass-through only. No pitch detection or tahrir processing yet.
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();
    for (int ch = inputChannels; ch < outputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
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
