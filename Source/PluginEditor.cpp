#include "PluginEditor.h"

SmartTahrirEditor::SmartTahrirEditor (SmartTahrirProcessor& p)
 : AudioProcessorEditor (&p), processor (p)
{
    setSize (520, 340);
    title.setText ("SMART TAHRIR PROCESSOR", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::Font (22.0f, juce::Font::bold));
    addAndMakeVisible (title);

    status.setText ("Prototype 0.1  |  Audio pass-through test  |  Tahrir engine not implemented",
                    juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centred);
    status.setFont (juce::Font (12.0f));
    addAndMakeVisible (status);

    auto setup = [this] (juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 22);
        s.setName (name);
        addAndMakeVisible (s);
    };
    setup (enhance, "Enhance");
    setup (autoTahrir, "Auto Tahrir");
    setup (hold, "Hold Time (ms)");
    setup (depth, "Depth");

    enhanceAttachment = std::make_unique<Attachment> (processor.parameters, "enhance", enhance);
    autoAttachment = std::make_unique<Attachment> (processor.parameters, "autoTahrir", autoTahrir);
    holdAttachment = std::make_unique<Attachment> (processor.parameters, "hold", hold);
    depthAttachment = std::make_unique<Attachment> (processor.parameters, "depth", depth);
}

void SmartTahrirEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171a20));
    g.setColour (juce::Colour (0xffe8edf5));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 12.0f, 1.5f);
}

void SmartTahrirEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    title.setBounds (area.removeFromTop (48));
    area.removeFromTop (12);
    auto controls = area.removeFromTop (190);
    const int w = controls.getWidth() / 4;
    enhance.setBounds (controls.removeFromLeft (w));
    autoTahrir.setBounds (controls.removeFromLeft (w));
    hold.setBounds (controls.removeFromLeft (w));
    depth.setBounds (controls);
    status.setBounds (area.removeFromTop (48));
}
