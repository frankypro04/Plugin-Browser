#pragma once
#include "PluginProcessor.h"

//==============================================================================
namespace Palette
{
    inline const juce::Colour bg0      { 0xff0b0e14 };
    inline const juce::Colour bg1      { 0xff131927 };
    inline const juce::Colour panel    { 0xff171d2b };
    inline const juce::Colour row      { 0xff1b2231 };
    inline const juce::Colour rowHover { 0xff242d44 };
    inline const juce::Colour accent   { 0xff7c5cff };
    inline const juce::Colour accent2  { 0xff22d3ee };
    inline const juce::Colour text     { 0xffe8ebf4 };
    inline const juce::Colour dim      { 0xff8a93a8 };
}

inline juce::Font uiFont (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
class BrowserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BrowserLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (14.0f, juce::Font::bold); }
    void drawTreeviewPlusMinusBox (juce::Graphics&, const juce::Rectangle<float>&, juce::Colour, bool isOpen, bool over) override;
    int getDefaultScrollbarWidth() override { return 8; }
};

//==============================================================================
class PluginBrowserEditor : public juce::AudioProcessorEditor,
                            private PluginBrowserProcessor::Listener,
                            private juce::ChangeListener,
                            private juce::ComponentListener,
                            private juce::Timer
{
public:
    explicit PluginBrowserEditor (PluginBrowserProcessor&);
    ~PluginBrowserEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int topH = 56;

    // Listener callbacks
    void hostedAboutToChange() override;
    void hostedChanged() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { listDirty = true; }
    void componentMovedOrResized (juce::Component&, bool moved, bool resized) override;
    void timerCallback() override;

    void rebuildTree();
    void showPluginView (bool show);
    void createHostedEditor();
    void destroyHostedEditor();
    void fitToHosted();
    void applyScale();

    PluginBrowserProcessor& processor;
    BrowserLookAndFeel laf;

    // Vista biblioteca
    juce::TextEditor search;
    juce::TextButton byMakerBtn { "Por fabricante" }, listBtn { "Lista" }, rescanBtn { "Escanear" };
    juce::ComboBox kindBox;
    juce::Label status;
    double progress = 0.0;
    juce::ProgressBar progressBar { progress };
    juce::TreeView tree;
    std::unique_ptr<juce::TreeViewItem> root;

    // Vista plugin
    juce::TextButton backBtn { "<  Biblioteca" }, unloadBtn { "Quitar" };
    juce::ComboBox scaleBox;
    juce::Label hostedName;
    std::unique_ptr<juce::AudioProcessorEditor> hostedEditor;

    bool pluginView = false;
    bool listDirty = true;
    bool ignoreHostedResize = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowserEditor)
};
