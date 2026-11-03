#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmartTahrirEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SmartTahrirEditor (SmartTahrirProcessor&);
    ~SmartTahrirEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SmartTahrirProcessor& processor;
    juce::Slider enhance, autoTahrir, hold, depth;
    juce::Label title, status;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> enhanceAttachment, autoAttachment, holdAttachment, depthAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTahrirEditor)
};
