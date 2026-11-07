#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace
{
using Curve = std::array<float, SmartTahrirProcessor::curveSize>;

Curve makePersianPreset(int id)
{
    // These are original, editable pitch-contour starting points based on broad
    // Persian classical vocal ornament concepts, not transcriptions of a singer.
    std::vector<std::pair<float, float>> points;
    float fineRate = 0.0f, fineAmount = 0.0f;
    switch (id)
    {
        case 1: // Chakkoshi / hammer-like turns
            points = {{0,0},{.08f,.82f},{.16f,-.78f},{.25f,.66f},{.34f,-.72f},{.46f,.50f},{.58f,-.45f},{.72f,.25f},{.88f,.08f},{1,0}};
            break;
        case 2: // Fine connected tahrir
            points = {{0,0},{.10f,.28f},{.22f,-.24f},{.34f,.30f},{.46f,-.27f},{.58f,.25f},{.70f,-.20f},{.84f,.12f},{1,0}};
            fineRate = 5.0f; fineAmount = .13f;
            break;
        case 3: // Chahchaheh / alternating calls
            points = {{0,0},{.12f,.95f},{.24f,-.62f},{.38f,.88f},{.50f,-.68f},{.64f,.72f},{.78f,-.40f},{.91f,.22f},{1,0}};
            break;
        case 4: // Circular, rolling turn
            points = {{0,0},{.12f,.70f},{.25f,.92f},{.38f,.10f},{.50f,-.78f},{.63f,-.92f},{.76f,-.12f},{.89f,.50f},{1,0}};
            fineRate = 2.0f; fineAmount = .08f;
            break;
        case 5: // Stepwise ornament
            points = {{0,0},{.14f,.72f},{.28f,.72f},{.40f,.05f},{.52f,.05f},{.66f,-.62f},{.78f,-.62f},{.91f,.10f},{1,0}};
            break;
        case 6: // Phrase ending / descending cadence
            points = {{0,0},{.12f,.58f},{.25f,.32f},{.38f,.68f},{.51f,.18f},{.64f,-.20f},{.78f,-.55f},{.90f,-.30f},{1,-.72f}};
            break;
        case 7: // Compound ornament
            points = {{0,0},{.09f,.72f},{.18f,-.52f},{.30f,.40f},{.42f,-.30f},{.55f,.82f},{.66f,-.70f},{.78f,.38f},{.90f,-.18f},{1,0}};
            fineRate = 4.0f; fineAmount = .07f;
            break;
        case 8: // Long, spacious ornament
            points = {{0,0},{.18f,.30f},{.34f,.82f},{.48f,.48f},{.62f,-.52f},{.78f,-.28f},{.91f,.10f},{1,0}};
            fineRate = 1.5f; fineAmount = .05f;
            break;
        default: // Classical phrase, gentle opening and return
            points = {{0,0},{.16f,.45f},{.30f,-.18f},{.44f,.62f},{.58f,-.42f},{.72f,.28f},{.88f,.08f},{1,0}};
            break;
    }

    Curve result{};
    for (int i = 0; i < SmartTahrirProcessor::curveSize; ++i)
    {
        const float x = (float)i / (SmartTahrirProcessor::curveSize - 1);
        size_t k = 0;
        while (k + 1 < points.size() - 1 && x > points[k + 1].first) ++k;
        const float span = juce::jmax(0.0001f, points[k + 1].first - points[k].first);
        const float t = juce::jlimit(0.0f, 1.0f, (x - points[k].first) / span);
        const float smooth = t * t * (3.0f - 2.0f * t);
        float y = points[k].second + (points[k + 1].second - points[k].second) * smooth;
        if (fineRate > 0.0f)
        {
            const float envelope = std::sin(juce::MathConstants<float>::pi * x);
            y += fineAmount * envelope * std::sin(juce::MathConstants<float>::twoPi * fineRate * x);
        }
        result[(size_t)i] = juce::jlimit(-1.0f, 1.0f, y);
    }
    return result;
}
}

TahrirCurveEditor::TahrirCurveEditor() { curve.fill(0.0f); }

void TahrirCurveEditor::setCurve(const Curve& c)
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
    g.drawText("PITCH UP", 5, 3, 70, 16, juce::Justification::left);
    g.drawText("PITCH DOWN", 5, getHeight() - 19, 90, 16, juce::Justification::left);
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
    setSize(760, 690);
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        addAndMakeVisible(s); addAndMakeVisible(l);
        l.setText(name, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, juce::Colours::white);
        l.setJustificationType(juce::Justification::centred);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
    };
    title.setText("HADI  |  SMART TAHRIR", juce::dontSendNotification);
    title.setFont(juce::Font(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(0xff56d6c9));
    addAndMakeVisible(title);
    subtitle.setText("Persian vocal ornament presets - each preset shows its pitch contour", juce::dontSendNotification);
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
    styleLabel.setText("Engine family", juce::dontSendNotification);
    styleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(styleLabel);
    styleAttachment = std::make_unique<ComboAttachment>(processor.parameters, "style", style);

    addAndMakeVisible(preset);
    preset.addItem("Classical Persian", 1);
    preset.addItem("Chakkoshi", 2);
    preset.addItem("Fine connected", 3);
    preset.addItem("Chahchaheh", 4);
    preset.addItem("Circular / rolling", 5);
    preset.addItem("Stepwise", 6);
    preset.addItem("Descending cadence", 7);
    preset.addItem("Compound tahrir", 8);
    preset.addItem("Long spacious", 9);
    presetLabel.setText("Iranian tahrir preset", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(presetLabel);
    presetDescription.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(presetDescription);
    preset.onChange = [this] { loadPreset(preset.getSelectedId()); };

    addAndMakeVisible(curveEditor);
    curveEditor.onCurveChanged = [this](const auto& c)
    {
        processor.setCustomCurve(c, 0.55f);
        preset.setSelectedId(0, juce::dontSendNotification);
        presetDescription.setText("Custom hand-drawn contour", juce::dontSendNotification);
        status.setText("Drawn curve is active.", juce::dontSendNotification);
    };
    addAndMakeVisible(importButton); addAndMakeVisible(drawButton); addAndMakeVisible(resetButton);
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
        preset.setSelectedId(1);
        loadPreset(1);
        status.setText("Built-in classical preset selected.", juce::dontSendNotification);
    };
    preset.setSelectedId(1);
    loadPreset(1);
    startTimerHz(30);
}

void SmartTahrirEditor::loadPreset(int presetId)
{
    if (presetId < 1 || presetId > 9) return;
    const auto c = makePersianPreset(presetId - 1);
    curveEditor.setCurve(c);
    processor.setCustomCurve(c, presetId == 9 ? 0.95f : 0.55f);
    static const char* descriptions[] = {
        "A balanced classical phrase ornament.",
        "Short accented up/down turns with a firm attack.",
        "A finer, more connected sequence of small turns.",
        "Alternating high and low calls in a lively phrase.",
        "A rounded, rolling motion with a circular feel.",
        "A stepped ornament with held pitch levels.",
        "A descending phrase ending / vocal cadence.",
        "A denser combination of several short turns.",
        "A slower, more spacious long-form ornament."
    };
    presetDescription.setText(descriptions[presetId - 1], juce::dontSendNotification);
    status.setText("Preset contour loaded. Adjust Depth, Speed and Hold to taste.", juce::dontSendNotification);
}

void SmartTahrirEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff10141b));
    if (meterBounds.isEmpty()) return;
    auto area = meterBounds.toFloat();
    const float labelW = 72.0f, gap = 10.0f;
    const float barW = area.getWidth() - labelW - gap - 58.0f;
    auto drawMeter = [&](float y, const juce::String& label, float level)
    {
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(12.0f);
        g.drawText(label, (int)area.getX(), (int)y, (int)labelW, 16, juce::Justification::centredLeft);
        juce::Rectangle<float> bar(area.getX()+labelW, y+2.0f, barW, 12.0f);
        g.setColour(juce::Colour(0xff303846)); g.fillRoundedRectangle(bar, 3.0f);
        const float db = level > 0.000001f ? juce::Decibels::gainToDecibels(level) : -60.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, (db+60.0f)/60.0f);
        auto fill = bar.withWidth(bar.getWidth()*norm);
        g.setColour(db > -3.0f ? juce::Colour(0xfff06a52) : juce::Colour(0xff56d6c9));
        g.fillRoundedRectangle(fill, 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawText(juce::String(db,1)+" dB", (int)(bar.getRight()+6), (int)y, 52, 16, juce::Justification::centredRight);
    };
    drawMeter(area.getY()+1.0f, "INPUT", processor.getInputLevel());
    drawMeter(area.getY()+19.0f, "OUTPUT", processor.getOutputLevel());
}
void SmartTahrirEditor::timerCallback() { repaint(meterBounds); }

void SmartTahrirEditor::resized()
{
    auto r = getLocalBounds().reduced(18);
    title.setBounds(r.removeFromTop(34));
    subtitle.setBounds(r.removeFromTop(25));
    r.removeFromTop(3);
    meterBounds = r.removeFromTop(38);
    r.removeFromTop(3);
    auto controls = r.removeFromTop(138);
    const int cellWidth = controls.getWidth() / 5;
    juce::Component* sliders[] = { &enhance, &autoTahrir, &hold, &depth, &speed };
    juce::Label* labels[] = { &enhanceLabel, &autoLabel, &holdLabel, &depthLabel, &speedLabel };
    for (int i = 0; i < 5; ++i)
    {
        auto cell = controls.removeFromLeft(cellWidth);
        labels[i]->setBounds(cell.removeFromTop(22));
        sliders[i]->setBounds(cell);
    }
    r.removeFromTop(3);
    auto styleRow = r.removeFromTop(28);
    styleLabel.setBounds(styleRow.removeFromLeft(110));
    style.setBounds(styleRow.removeFromLeft(170));
    r.removeFromTop(4);
    auto presetRow = r.removeFromTop(30);
    presetLabel.setBounds(presetRow.removeFromLeft(150));
    preset.setBounds(presetRow.removeFromLeft(220));
    presetDescription.setBounds(presetRow);
    r.removeFromTop(4);
    curveLabel.setText("PITCH CONTOUR PREVIEW  |  horizontal = time, vertical = pitch movement", juce::dontSendNotification);
    curveLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    curveLabel.setBounds(r.removeFromTop(22));
    curveEditor.setBounds(r.removeFromTop(170));
    r.removeFromTop(6);
    auto buttons = r.removeFromTop(32);
    importButton.setBounds(buttons.removeFromLeft(175));
    buttons.removeFromLeft(8);
    drawButton.setBounds(buttons.removeFromLeft(145));
    buttons.removeFromLeft(8);
    resetButton.setBounds(buttons.removeFromLeft(85));
    status.setBounds(r.removeFromTop(26));
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
                status.setText("Could not read this WAV file.", juce::dontSendNotification); return;
            }
            const int total = (int)juce::jmin<int64>((int64)(reader->sampleRate * 12.0), reader->lengthInSamples);
            juce::AudioBuffer<float> audio(1, total), source((int)reader->numChannels, total);
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
                if (best > 0.45 && bestLag > 0) { f0[(size_t)frame] = (float)(reader->sampleRate / bestLag); valid.push_back(f0[(size_t)frame]); }
            }
            if (valid.size() < frames / 4)
            {
                status.setText("Not enough clear pitched frames. Use a clean, single-voice tahrir WAV.", juce::dontSendNotification); return;
            }
            std::sort(valid.begin(), valid.end());
            const float median = valid[valid.size() / 2];
            Curve contour{};
            for (int i = 0; i < frames; ++i)
                contour[(size_t)i] = f0[(size_t)i] > 0.0f
                    ? juce::jlimit(-1.0f, 1.0f, 0.5f * std::log2(f0[(size_t)i] / median) * 12.0f / 4.0f)
                    : (i > 0 ? contour[(size_t)(i - 1)] : 0.0f);
            curveEditor.setCurve(contour);
            preset.setSelectedId(0, juce::dontSendNotification);
            presetDescription.setText("Contour extracted from the selected WAV.", juce::dontSendNotification);
            const float duration = (float)((double)total / reader->sampleRate);
            processor.setCustomCurve(contour, duration);
            status.setText("WAV contour imported (" + juce::String(duration, 2) + " s). Pitch contour only; not audio cloning.", juce::dontSendNotification);
        });
}
