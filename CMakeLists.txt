cmake_minimum_required(VERSION 3.24)
project(HADI VERSION 0.4.0)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 7.0.12 GIT_SHALLOW TRUE)
FetchContent_Declare(signalsmith-stretch GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git GIT_TAG 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE signalsmith-stretch)
juce_add_plugin(SmartTahrirProcessor COMPANY_NAME "Hadi Zangoui" BUNDLE_ID "com.hadizangoui.hadi"
 PLUGIN_MANUFACTURER_CODE Hadi PLUGIN_CODE Hd01 FORMATS VST3 PRODUCT_NAME "HADI"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx Tools)
juce_generate_juce_header(SmartTahrirProcessor)
target_sources(SmartTahrirProcessor PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(SmartTahrirProcessor PRIVATE cxx_std_17)
target_compile_definitions(SmartTahrirProcessor PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SmartTahrirProcessor PRIVATE juce::juce_audio_utils juce::juce_dsp signalsmith-stretch
 PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
