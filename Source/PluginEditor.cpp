#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <numeric>

TahrirCurveEditor::TahrirCurveEditor()
{
    curve.fill(0.0f);
}

void TahrirCurveEditor::setCurve(const std::array<float, SmartTahrirProcessor::curveSize>& c)
{
    curve = c;
    repaint();
}

void TahrirCurveEditor::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(0xff171b24));
    g.setColour(juce::Colour(0xff394252));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine((int)(area.getY() + area.getHeight() * i / 4.0f), area.getX(), area.getRight());
    g.drawHorizontalLine((int)area.getCentreY(), area.getX(), area.getRight());
    juce::Path path;
    for (int i = 0; i < SmartTahrirProcessor::curveSize; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float)i / (SmartTahrirProcessor::curveSize - 1);
        const float y = area.getCentreY() - curve[(size_t)i] * area.getHeight() * 0.44f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xff56d6c9));
    g.strokePath(path, juce::PathStrokeType(2.5f));
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(12.0f);
    g.drawText("HIGH", 5, 3, 42, 16, juce::Justification::left);
    g.drawText("LOW", 5, getHeight() - 19, 42, 16, juce::Justification::left);
}

void TahrirCurveEditor::editAt(juce::Point<float> p)
{
    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    if (bounds.getWidth() <= 1.0f || bounds.getHeight() <= 1.0f) return;
    const int index = juce::jlimit(0, SmartTahrirProcessor::curveSize - 1,
        (int)std::round((p.x - bounds.getX()) / bounds.getWidth() * (SmartTahrirProcessor::curveSize - 1)));
    const float value = juce::jlimit(-1.0f, 1.0f,
        (bounds.getCentreY() - p.y) / (bounds.getHeight() * 0.44f));
    curve[(size_t)index] = value;
    if (index > 0) curve[(size_t)(index - 1)] = 0.5f * (curve[(size_t)(index - 1)] + value);
    if (index + 1 < SmartTahrirProcessor::curveSize) curve[(size_t)(index + 1)] = 0.5f * (curve[(size_t)(index + 1)] + value);
    repaint();
    if (onCurveChanged) onCurveChanged(curve);
}

void TahrirCurveEditor::mouseDown(const juce::MouseEvent& e) { editAt(e.position); }
void TahrirCurveEditor::mouseDrag(const juce::MouseEvent& e) { editAt(e.position); }

SmartTahrirEditor::SmartTahrirEditor(SmartTahrirProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    setSize(720, 560);
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        addAndMakeVisible(s);
        addAndMakeVisible(l);
        l.setText(name, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, juce::Colours::white);
        l.setJustificationType(juce::Justification::centredLeft);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
    };
    title.setText("HADI  |  SMART TAHRIR", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(0xff56d6c9));
    addAndMakeVisible(title);
    subtitle.setText("Live vocal ornament processor - draw a pitch contour or import a clean tahrir WAV", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(subtitle);

    setupSlider(enhance, enhanceLabel, "Enhance");
    setupSlider(autoTahrir, autoLabel, "Auto Tahrir");
    setupSlider(hold, holdLabel, "Hold (ms)");
    setupSlider(depth, depthLabel, "Depth");
    setupSlider(speed, speedLabel, "Speed");

    enhanceAttachment = std::make_unique<SliderAttachment>(processor.parameters, "enhance", enhance);
    autoAttachment = std::make_unique<SliderAttachment>(processor.parameters, "autoTahrir", autoTahrir);
    holdAttachment = std::make_unique<SliderAttachment>(processor.parameters, "hold", hold);
    depthAttachment = std::make_unique<SliderAttachment>(processor.parameters, "depth", depth);
    speedAttachment = std::make_unique<SliderAttachment>(processor.parameters, "speed", speed);

    addAndMakeVisible(style);
    style.addItem("Persian", 1); style.addItem("Arabic", 2); style.addItem("Turkish", 3);
    styleLabel.setText("Style", juce::dontSendNotification);
    styleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(styleLabel);
    styleAttachment = std::make_unique<ComboAttachment>(processor.parameters, "style", style);

    addAndMakeVisible(curveEditor);
    curveEditor.setCurve({});
    curveEditor.onCurveChanged = [this](const auto& c)
    {
        processor.setCustomCurve(c, 0.55f);
        status.setText("Drawn curve is active.", juce::dontSendNotification);
    };

    addAndMakeVisible(importButton);
    addAndMakeVisible(drawButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(status);
    status.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    importButton.onClick = [this] { importSample(); };
    drawButton.onClick = [this]
    {
        processor.setCustomCurve(curveEditor.getCurve(), 0.55f);
        status.setText("Your drawn curve is active.", juce::dontSendNotification);
    };
    resetButton.onClick = [this]
    {
        processor.setCustomCurveEnabled(false);
        curveEditor.setCurve({});
        status.setText("Built-in style selected.", juce::dontSendNotification);
    };
}

void SmartTahrirEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff10141b));
}

void SmartTahrirEditor::resized()
{
    auto r = getLocalBounds().reduced(18);
    title.setBounds(r.removeFromTop(34));
    subtitle.setBounds(r.removeFromTop(28));
    r.removeFromTop(8);
    auto controls = r.removeFromTop(170);
    const int w = controls.getWidth() / 6;
    juce::Component* sliders[] = { &enhance, &autoTahrir, &hold, &depth, &speed };
    juce::Label* labels[] = { &enhanceLabel, &autoLabel, &holdLabel, &depthLabel, &speedLabel };
    for (int i = 0; i < 5; ++i)
    {
        auto cell = controls.removeFromLeft(w);
        labels[i]->setBounds(cell.removeFromTop(24));
        sliders[i]->setBounds(cell);
    }
    styleLabel.setBounds(controls.removeFromTop(24));
    style.setBounds(controls.removeFromTop(32));
    r.removeFromTop(8);
    curveLabel.setText("CUSTOM PITCH CONTOUR  (drag inside the graph)", juce::dontSendNotification);
    curveLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    curveLabel.setBounds(r.removeFromTop(22));
    curveEditor.setBounds(r.removeFromTop(190));
    r.removeFromTop(8);
    auto buttons = r.removeFromTop(34);
    importButton.setBounds(buttons.removeFromLeft(180));
    buttons.removeFromLeft(8);
    drawButton.setBounds(buttons.removeFromLeft(150));
    buttons.removeFromLeft(8);
    resetButton.setBounds(buttons.removeFromLeft(90));
    status.setBounds(r.removeFromTop(28));
}

void SmartTahrirEditor::importSample()
{
    chooser = std::make_unique<juce::FileChooser>("Choose a clean tahrir WAV file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;
            juce::AudioFormatManager manager;
            manager.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
            if (!reader || reader->lengthInSamples < 1024 || reader->sampleRate <= 0.0)
            {
                status.setText("Could not read this WAV file.", juce::dontSendNotification);
                return;
            }
            const int total = (int)juce::jmin<int64>((int64)(reader->sampleRate * 12.0), reader->lengthInSamples);
            juce::AudioBuffer<float> audio(1, total);
            juce::AudioBuffer<float> source((int)reader->numChannels, total);
            reader->read(&source, 0, total, 0, true, true);
            for (int i = 0; i < total; ++i)
            {
                float v = 0.0f;
                for (int ch = 0; ch < source.getNumChannels(); ++ch) v += source.getSample(ch, i);
                audio.setSample(0, i, v / (float)source.getNumChannels());
            }

            constexpr int win = 1024, frames = SmartTahrirProcessor::curveSize;
            std::array<float, frames> f0{};
            std::vector<float> valid;
            const int hop = juce::jmax(128, total / frames);
            for (int frame = 0; frame < frames; ++frame)
            {
                const int centre = juce::jlimit(win / 2, total - win / 2, frame * hop + win / 2);
                int bestLag = 0; double best = 0.0;
                for (int lag = 22; lag <= 300; ++lag)
                {
                    double cross = 0.0, e1 = 0.0, e2 = 0.0;
                    for (int n = 0; n < win - lag; n += 2)
                    {
                        const float a = audio.getSample(0, centre - win / 2 + n);
                        const float b = audio.getSample(0, centre - win / 2 + n + lag);
                        cross += (double)a * b; e1 += (double)a * a; e2 += (double)b * b;
                    }
                    const double corr = cross / (std::sqrt(e1 * e2) + 1.0e-12);
                    if (corr > best) { best = corr; bestLag = lag; }
                }
                if (best > 0.45 && bestLag > 0)
                {
                    f0[(size_t)frame] = (float)(reader->sampleRate / bestLag);
                    valid.push_back(f0[(size_t)frame]);
                }
            }
            if (valid.size() < frames / 4)
            {
                status.setText("Not enough clear pitched frames. Use a clean, single-voice tahrir WAV.", juce::dontSendNotification);
                return;
            }
            std::sort(valid.begin(), valid.end());
            const float median = valid[valid.size() / 2];
            std::array<float, frames> contour{};
            for (int i = 0; i < frames; ++i)
            {
                if (f0[(size_t)i] > 0.0f)
                    contour[(size_t)i] = juce::jlimit(-1.0f, 1.0f,
                        0.5f * std::log2(f0[(size_t)i] / median) * 12.0f / 4.0f);
                else
                    contour[(size_t)i] = i > 0 ? contour[(size_t)i - 1] : 0.0f;
            }
            curveEditor.setCurve(contour);
            const float duration = (float)((double)total / reader->sampleRate);
            processor.setCustomCurve(contour, duration);
            status.setText("WAV contour imported (" + juce::String(duration, 2) + " s). Pitch contour only; not audio cloning.", juce::dontSendNotification);
        });
}
