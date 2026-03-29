#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "engine/Operator.h"
#include "engine/LifeGrid.h"
#include "Commander.h"
#include <map>

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
    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void loadOrcaFile(const juce::File& file);
    void saveOrcaFile(const juce::File& file);
    void loadLifeFile(const juce::File& file);
    void saveLifeFile(const juce::File& file);
    void saveAs();

    juce::File currentFile; // currently loaded/saved file path

    // Commander (command prompt)
    Commander commander;

    // Inject cache (loaded via Cmd+L, referenced by inject: command)
    std::map<juce::String, juce::String> injectCache;
    void injectBlock(const juce::String& content, int atX, int atY);

    // Stamp mode (pattern preview + placement)
    bool stampMode = false;
    int stampIndex = 0;    // index into builtInPatterns
    int stampCategory = 0; // current category index

    // Cursor
    int cursorX = 0, cursorY = 0;
    int selectW = 1, selectH = 1;
    int dragOriginX = 0, dragOriginY = 0;

    // Clipboard
    char clipboard[orca::kMaxGridSize];
    orca::LifeCell lifeClipboard[orca::kMaxGridSize];
    int clipW = 0, clipH = 0;

    // Undo/Redo (normal Orca mode)
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

    // Undo/Redo (Life mode)
    static constexpr int kMaxLifeHistory = 64;
    struct LifeSnapshot {
        orca::LifeCell cells[orca::kMaxGridSize];
        int w, h;
    };
    LifeSnapshot* lifeHistory = nullptr; // heap-allocated on demand
    int lifeHistoryCount = 0;
    int lifeHistoryPos = -1;
    void pushLifeHistory();
    void undoLife();
    void redoLife();

    friend class Commander;

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

    // Life mode channel colors (16 channels)
    static constexpr uint32_t channelColorValues[16] = {
        0xffff5555, 0xffff8844, 0xffffcc33, 0xff88ee44,
        0xff44dd66, 0xff44ddaa, 0xff44ccdd, 0xff4488ee,
        0xff5555ff, 0xff8844ff, 0xffbb44ee, 0xffdd44aa,
        0xffee4477, 0xffaa6644, 0xff888888, 0xffeeeeee,
    };

    void drawCell(juce::Graphics& g, int x, int y, char glyph,
                  uint8_t portType, bool isLocked, bool isSelected);
    void drawLifeCell(juce::Graphics& g, int x, int y,
                      const orca::LifeCell& cell, bool isSelected);

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
