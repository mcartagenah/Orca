#pragma once

#include <juce_core/juce_core.h>

// Forward declarations
class OrcaProcessor;
class GridComponent;

class Commander {
public:
    bool isActive = false;
    juce::String query;
    int cursorPos = 0; // cursor position within query

    // Command history
    static constexpr int kMaxHistory = 32;
    juce::String commandHistory[kMaxHistory];
    int historyCount = 0;
    int historyIndex = -1;

    // Find preview state
    juce::Array<int> findHighlights; // cell indices that match find query
    bool hasFindPreview = false;

    void start(const juce::String& prefill = "") {
        isActive = true;
        query = prefill;
        cursorPos = query.length();
        historyIndex = -1;
        findHighlights.clear();
        hasFindPreview = false;
    }

    void stop() {
        isActive = false;
        query.clear();
        cursorPos = 0;
        findHighlights.clear();
        hasFindPreview = false;
    }

    void write(juce::juce_wchar ch) {
        query = query.substring(0, cursorPos)
              + juce::String::charToString(ch)
              + query.substring(cursorPos);
        cursorPos++;
    }

    void erase() {
        if (cursorPos > 0) {
            query = query.substring(0, cursorPos - 1) + query.substring(cursorPos);
            cursorPos--;
        }
    }

    void eraseForward() {
        if (cursorPos < query.length())
            query = query.substring(0, cursorPos) + query.substring(cursorPos + 1);
    }

    void moveCursorLeft()  { if (cursorPos > 0) cursorPos--; }
    void moveCursorRight() { if (cursorPos < query.length()) cursorPos++; }
    void moveCursorHome()  { cursorPos = 0; }
    void moveCursorEnd()   { cursorPos = query.length(); }

    // Move cursor to previous/next word boundary
    void moveCursorWordLeft() {
        if (cursorPos == 0) return;
        cursorPos--;
        while (cursorPos > 0 && query[cursorPos - 1] != ' ' && query[cursorPos - 1] != ':' && query[cursorPos - 1] != ';')
            cursorPos--;
    }

    void moveCursorWordRight() {
        int len = query.length();
        if (cursorPos >= len) return;
        cursorPos++;
        while (cursorPos < len && query[cursorPos] != ' ' && query[cursorPos] != ':' && query[cursorPos] != ';')
            cursorPos++;
    }

    // Select all text (for Cmd+A)
    void selectAll() { cursorPos = query.length(); }

    // Replace entire query (for paste)
    void replaceQuery(const juce::String& text) {
        query = text;
        cursorPos = query.length();
    }

    // Insert text at cursor (for paste)
    void insertAtCursor(const juce::String& text) {
        query = query.substring(0, cursorPos) + text + query.substring(cursorPos);
        cursorPos += text.length();
    }

    // Delete from cursor to end (Cmd+K / Ctrl+K)
    void killToEnd() {
        query = query.substring(0, cursorPos);
    }

    // Delete from start to cursor (Cmd+U / Ctrl+U)
    void killToStart() {
        query = query.substring(cursorPos);
        cursorPos = 0;
    }

    void historyUp() {
        if (historyCount == 0) return;
        if (historyIndex < historyCount - 1) historyIndex++;
        query = commandHistory[historyIndex];
        cursorPos = query.length();
    }

    void historyDown() {
        if (historyIndex > 0) {
            historyIndex--;
            query = commandHistory[historyIndex];
        } else if (historyIndex == 0) {
            historyIndex = -1;
            query.clear();
        }
        cursorPos = query.length();
    }

    void pushToHistory(const juce::String& cmd) {
        if (cmd.isEmpty()) return;
        // Shift history down
        int limit = (historyCount < kMaxHistory) ? historyCount : kMaxHistory - 1;
        for (int i = limit; i > 0; i--)
            commandHistory[i] = commandHistory[i - 1];
        commandHistory[0] = cmd;
        if (historyCount < kMaxHistory) historyCount++;
    }

    // Parse and execute command. Returns true if valid.
    // Implementation is in PluginEditor.cpp since it needs access to processor/editor.
    bool trigger(OrcaProcessor& processor, GridComponent& editor);

    // Update passive previews (find highlighting)
    void preview(OrcaProcessor& processor, GridComponent& editor);

private:
    // Parse "command:value" from query
    struct ParsedCommand {
        juce::String name;
        juce::String value;
        juce::StringArray parts; // value split by ';'
    };

    ParsedCommand parse() const {
        ParsedCommand result;
        int colonIdx = query.indexOfChar(':');
        if (colonIdx >= 0) {
            result.name = query.substring(0, colonIdx).toLowerCase();
            result.value = query.substring(colonIdx + 1);
            result.parts = juce::StringArray::fromTokens(result.value, ";", "");
        } else {
            result.name = query.toLowerCase().trim();
        }

        // 2-letter shorthand expansion
        if (result.name.length() == 2) {
            auto sh = result.name;
            if (sh == "fi") result.name = "find";
            else if (sh == "se") result.name = "select";
            else if (sh == "wr") result.name = "write";
            else if (sh == "gr") result.name = "groove";
            else if (sh == "er") result.name = "erase";
            else if (sh == "co") result.name = "copy";
            else if (sh == "pa") result.name = "paste";
            else if (sh == "cl") result.name = "color";
            else if (sh == "ti") result.name = "time";
            else if (sh == "sc") result.name = "scale";
            else if (sh == "ro") result.name = "root";
            else if (sh == "ra") result.name = "rate";
            else if (sh == "pu") result.name = "pulse";
            else if (sh == "ho") result.name = "hold";
            else if (sh == "ch") result.name = "channel";
            else if (sh == "oc") result.name = "octave";
            else if (sh == "mo") result.name = "maxoct";
            else if (sh == "mi") result.name = "minoct";
            else if (sh == "in") result.name = "inject";
            else if (sh == "pg") result.name = "pg";
            else if (sh == "ud") result.name = "udp";
            else if (sh == "os") result.name = "osc";
            else if (sh == "ip") result.name = "ip";
            else if (sh == "de") result.name = "decay";
            else if (sh == "mv") result.name = "minvel";
            else if (sh == "mp") result.name = "minprob";
            else if (sh == "mn") result.name = "maxnotes";
            else if (sh == "sq") result.name = "seq";
            else if (sh == "lo") result.name = "lockoct";
            else if (sh == "cd") result.name = "chord";
            else if (sh == "dd") result.name = "dedup";
            else if (sh == "dc") result.name = "dedupcc";
            else if (sh == "ru") result.name = "rule";
        }
        return result;
    }
};
