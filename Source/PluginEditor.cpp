#include "PluginEditor.h"

SmartTahrirEditor::SmartTahrirEditor (SmartTahrirProcessor& p)
 : AudioProcessorEditor (&p), processor (p)
{
    setSize (700, 470);
    title.setText ("SMART TAHRIR", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::Font (27.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, juce::Colour (0xfff4d58d));
    addAndMakeVisible (title);

    subtitle.setText ("LIVE VOCAL ORNAMENT PROCESSOR", juce::dontSendNotification);
    subtitle.setJustificationType (juce::Justification::centred);
    subtitle.setFont (juce::Font (12.0f));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xffaab3c2));
    addAndMakeVisible (subtitle);

    auto setupSlider = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffe8edf5));
        addAndMakeVisible (l);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffd9a441));
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff4f4f4));
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff282d36));
        addAndMakeVisible (s);
    };

    setupSlider (enhance, enhanceLabel, "Enhance");
    setupSlider (autoTahrir, autoLabel, "Auto Tahrir");
    setupSlider (hold, holdLabel, "Hold (ms)");
    setupSlider (depth, depthLabel, "Pitch Depth");
    setupSlider (speed, speedLabel, "Speed");

    styleLabel.setText ("Style", juce::dontSendNotification);
    styleLabel.setJustificationType (juce::Justification::centred);
    styleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8edf5));
    addAndMakeVisible (styleLabel);
    style.addItem ("Persian", 1);
    style.addItem ("Arabic", 2);
    style.addItem ("Turkish", 3);
    style.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff282d36));
    style.setColour (juce::ComboBox::textColourId, juce::Colour (0xfff4f4f4));
    addAndMakeVisible (style);

    status.setText ("Experimental v0.2 | Sustained-level trigger | 8 ms reported latency",
                    juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centred);
    status.setFont (juce::Font (11.0f));
    status.setColour (juce::Label::textColourId, juce::Colour (0xff9aa5b5));
    addAndMakeVisible (status);

    enhanceAttachment = std::make_unique<SliderAttachment> (processor.parameters, "enhance", enhance);
    autoAttachment = std::make_unique<SliderAttachment> (processor.parameters, "autoTahrir", autoTahrir);
    holdAttachment = std::make_unique<SliderAttachment> (processor.parameters, "hold", hold);
    depthAttachment = std::make_unique<SliderAttachment> (processor.parameters, "depth", depth);
    speedAttachment = std::make_unique<SliderAttachment> (processor.parameters, "speed", speed);
    styleAttachment = std::make_unique<ComboAttachment> (processor.parameters, "style", style);
}

void SmartTahrirEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171a20));
    g.setColour (juce::Colour (0xff343b48));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 14.0f, 1.5f);
    g.setColour (juce::Colour (0xff252b35));
    g.fillRoundedRectangle (juce::Rectangle<float> (22.0f, 105.0f, (float) getWidth() - 44.0f, 300.0f), 12.0f);
}

void SmartTahrirEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    title.setBounds (area.removeFromTop (42));
    subtitle.setBounds (area.removeFromTop (24));
    area.removeFromTop (20);

    const int cellW = area.getWidth() / 5;
    auto controls = area.removeFromTop (250);
    juce::Slider* sliders[] = { &enhance, &autoTahrir, &hold, &depth, &speed };
    juce::Label* labels[] = { &enhanceLabel, &autoLabel, &holdLabel, &depthLabel, &speedLabel };
    for (int i = 0; i < 5; ++i)
    {
        auto cell = controls.removeFromLeft (cellW);
        labels[i]->setBounds (cell.removeFromTop (30));
        sliders[i]->setBounds (cell.reduced (5, 0));
    }

    auto styleArea = area.removeFromTop (55);
    styleLabel.setBounds (styleArea.removeFromLeft (110));
    style.setBounds (styleArea.removeFromLeft (190).reduced (4));
    status.setBounds (area.removeFromTop (35));
}
