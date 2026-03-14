#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "engine/Operator.h"

class GridComponent : public juce::Component,
                      public juce::Timer,
                      public juce::FileDragAndDropTarget {
public:
    GridComponent(OrcaProcessor& p);
    ~GridComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Keyboard input
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void loadOrcaFile(const juce::File& file);
    void saveOrcaFile(const juce::File& file);
    void saveAs();

    juce::File currentFile; // currently loaded/saved file path

    // Cursor
    int cursorX = 0, cursorY = 0;
    int selectW = 1, selectH = 1;
    int dragOriginX = 0, dragOriginY = 0;

    // Clipboard
    char clipboard[orca::kMaxGridSize];
    int clipW = 0, clipH = 0;

    // Undo/Redo
    static constexpr int kMaxHistory = 128;
    struct Snapshot {
        char cells[orca::kMaxGridSize];
        int w, h;
    };
    Snapshot history[kMaxHistory];
    int historyCount = 0;
    int historyPos = -1;
    void pushHistory();
    void undo();
    void redo();

private:
    OrcaProcessor& processor;

    float fontSize = 12.0f;
    float tileW = 10.0f;
    float tileH = 15.0f;
    void updateFontSize(float newSize);

    // Theme colors (matches Orca dark theme from client.js)
    juce::Colour bgColor      { 0xff000000 };
    juce::Colour fHigh        { 0xffffffff };
    juce::Colour fMed         { 0xff777777 };
    juce::Colour fLow         { 0xff444444 };
    juce::Colour fInv         { 0xff000000 };
    juce::Colour bHigh        { 0xffeeeeee };
    juce::Colour bMed         { 0xff72dec2 };
    juce::Colour bLow         { 0xff444444 };
    juce::Colour bInv         { 0xffffb545 };

    juce::Font monoFont;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void drawCell(juce::Graphics& g, int x, int y, char glyph,
                  uint8_t portType, bool isLocked, bool isSelected);

    static const char* operatorName(char glyph);
    static const char* portName(char ownerGlyph, int portIdx);
};

class OrcaEditor : public juce::AudioProcessorEditor {
public:
    OrcaEditor(OrcaProcessor&);
    ~OrcaEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OrcaProcessor& orcaProcessor;
    GridComponent gridComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrcaEditor)
};
