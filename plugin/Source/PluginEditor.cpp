#include "PluginEditor.h"

// ── GridComponent ──

GridComponent::GridComponent(OrcaProcessor& p)
    : processor(p)
{
    setWantsKeyboardFocus(true);
    startTimerHz(30); // 30fps UI refresh

    // Use a monospace font
    updateFontSize(fontSize);
}

GridComponent::~GridComponent() {
    stopTimer();
    // Cancel any pending async file chooser to prevent callback on a dead object
    fileChooser.reset();
}

void GridComponent::paint(juce::Graphics& g) {
    g.fillAll(bgColor);

    auto& engine = processor.engine;
    int gridW = engine.shadowW;
    int gridH = engine.shadowH;

    g.setFont(monoFont);

    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            int idx = x + gridW * y;
            char glyph = engine.shadowCells[idx];
            uint8_t portType = engine.shadowPorts[idx];
            bool isLocked = engine.shadowLocks[idx];
            bool isSelected = (x >= cursorX && x < cursorX + selectW &&
                               y >= cursorY && y < cursorY + selectH);
            drawCell(g, x, y, glyph, portType, isLocked, isSelected);
        }
    }

    // Draw cursor marker on top of selection
    {
        float cx = cursorX * tileW;
        float cy = cursorY * tileH;
        g.setColour(bInv);
        g.fillRect(cx, cy, tileW, tileH);
        g.setColour(fInv);
        char curGlyphUnder = '.';
        if (cursorX < gridW && cursorY < gridH)
            curGlyphUnder = engine.shadowCells[cursorX + gridW * cursorY];
        char cursorChar = (curGlyphUnder != '.') ? curGlyphUnder : '@';
        g.drawText(juce::String::charToString(cursorChar),
                   (int)cx, (int)cy, (int)tileW, (int)tileH,
                   juce::Justification::centred);
    }

    // Two-line status bar (matches original Orca layout)
    g.setFont(monoFont);
    float statusY = getHeight() - 30.0f;

    // Get glyph and port info under cursor
    char curGlyph = '.';
    uint8_t curPort = 0;
    char curPortOwner = 0;
    uint8_t curPortIdx = 0;
    if (cursorX < gridW && cursorY < gridH) {
        int ci = cursorX + gridW * cursorY;
        curGlyph = engine.shadowCells[ci];
        curPort = engine.shadowPorts[ci];
        curPortOwner = engine.shadowPortOwner[ci];
        curPortIdx = engine.shadowPortIdx[ci];
    }

    // Line 1: operator_name (+ port_name if on a port)  cursor_pos  frame
    {
        g.setColour(fMed);
        juce::String line1;
        if (curPort > 0 && curPort != 10 && curPortOwner != 0) {
            // Cursor is on a port — show "ownerName portName"
            line1 << operatorName(curPortOwner) << " " << portName(curPortOwner, curPortIdx);
        } else {
            line1 << operatorName(curGlyph);
        }
        line1 << "   " << cursorX << "," << cursorY
              << "   " << engine.shadowF << "f";
        g.drawText(line1, 4, (int)statusY, getWidth() - 8, 14,
                   juce::Justification::centredLeft);
    }

    // Line 2: grid_size + groove info
    {
        g.setColour(fLow);
        juce::String line2;
        line2 << gridW << "x" << gridH;
        if (processor.grooveLength > 1) {
            line2 << "   groove:";
            for (int i = 0; i < processor.grooveLength; i++) {
                if (i > 0) line2 << ";";
                line2 << (int)std::round(processor.grooves[i] * 50.0);
            }
        }
        g.drawText(line2, 4, (int)(statusY + 14), getWidth() - 8, 14,
                   juce::Justification::centredLeft);
    }
}

void GridComponent::drawCell(juce::Graphics& g, int x, int y, char glyph,
                              uint8_t portType, bool isLocked, bool isSelected) {
    float px = x * tileW;
    float py = y * tileH;

    // Determine style type (mirrors JS makeStyle)
    int styleType;
    if (isSelected) {
        styleType = 4; // selected
    } else if (portType > 0 && portType != 10) {
        styleType = portType; // port type from engine
    } else if (glyph == '*' && !isLocked) {
        styleType = 2; // bang as input
    } else if (isLocked) {
        styleType = 5; // locked
    } else if (portType == 10) {
        styleType = 0; // operator self
    } else {
        styleType = 20; // default
    }

    // Get colors (mirrors JS makeTheme)
    juce::Colour bgCol, fgCol;
    bool hasBg = false, hasFg = false;

    switch (styleType) {
        case 0:  hasBg = true; bgCol = bMed; hasFg = true; fgCol = fLow; break;  // operator
        case 1:  hasFg = true; fgCol = bMed; break;                              // haste input
        case 2:  hasFg = true; fgCol = bHigh; break;                             // input
        case 3:  hasBg = true; bgCol = bHigh; hasFg = true; fgCol = fLow; break; // output
        case 4:  hasBg = true; bgCol = bInv; hasFg = true; fgCol = fInv; break;  // selected
        case 5:  hasFg = true; fgCol = fMed; break;                              // locked
        case 6:  hasFg = true; fgCol = bInv; break;                              // reader
        case 8:  hasBg = true; bgCol = bLow; hasFg = true; fgCol = fHigh; break; // bang output
        case 9:  hasBg = true; bgCol = bInv; hasFg = true; fgCol = bgColor; break; // reader output
        default: hasFg = true; fgCol = fLow; break;                              // default
    }

    // Handle empty cells
    if (glyph == '.' && !hasBg && !isSelected) {
        // Empty port cells: show a colored dot in the port color
        if (hasFg && styleType > 0 && styleType < 20) {
            g.setColour(fgCol);
            float dotX = px + tileW * 0.5f;
            float dotY = py + tileH * 0.5f;
            g.fillRect(dotX - 0.5f, dotY - 0.5f, 2.0f, 2.0f);
            return;
        }

        bool isMarker = (x % 8 == 0 && y % 8 == 0);
        bool isLocalGuide = (x % 2 == 0 && y % 2 == 0) &&
                            (std::abs(x - cursorX) <= 6 && std::abs(y - cursorY) <= 6);

        if (isMarker) {
            // Major grid markers: '+' at 8x8 intervals
            g.setColour(fLow);
            g.drawText("+", (int)px, (int)py, (int)tileW, (int)tileH,
                       juce::Justification::centred);
        } else if (isLocalGuide) {
            // Local guide: tiny 1px dot at center of tile
            g.setColour(fLow);
            float dotX = px + tileW * 0.5f;
            float dotY = py + tileH * 0.5f;
            g.fillRect(dotX, dotY, 1.0f, 1.0f);
        }
        return;
    }

    // Draw background
    if (hasBg) {
        g.setColour(bgCol);
        g.fillRect(px, py, tileW, tileH);
    }

    // Draw glyph (selected empty cells show '.' in selection color)
    if (hasFg && (glyph != '.' || isSelected)) {
        g.setColour(fgCol);
        g.drawText(juce::String::charToString(glyph),
                   (int)px, (int)py, (int)tileW, (int)tileH,
                   juce::Justification::centred);
    }
}

void GridComponent::updateFontSize(float newSize) {
    fontSize = juce::jlimit(8.0f, 36.0f, newSize);
    monoFont = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), fontSize, juce::Font::plain));
    tileW = std::round(fontSize * 0.833f);  // ~10/12 ratio
    tileH = std::round(fontSize * 1.25f);   // ~15/12 ratio
}

void GridComponent::resized() {
    // Only grow the grid, never shrink (prevents losing content on resize)
    int viewW = juce::jmax(1, (int)(getWidth() / tileW));
    int viewH = juce::jmax(1, (int)((getHeight() - 32.0f) / tileH));
    viewW = juce::jmin(viewW, orca::kMaxGridW);
    viewH = juce::jmin(viewH, orca::kMaxGridH);

    auto& grid = processor.engine.grid;
    int newW = juce::jmax(grid.w, viewW);
    int newH = juce::jmax(grid.h, viewH);

    if (newW != grid.w || newH != grid.h) {
        // Grow grid, preserving all existing content
        char buf[orca::kMaxGridSize];
        memcpy(buf, grid.cells, grid.w * grid.h);

        char newCells[orca::kMaxGridSize];
        for (int i = 0; i < newW * newH; i++) newCells[i] = '.';
        for (int y = 0; y < grid.h; y++) {
            for (int x = 0; x < grid.w; x++) {
                newCells[x + newW * y] = buf[x + grid.w * y];
            }
        }
        processor.engine.load(newW, newH, newCells, grid.f);
    }
}

void GridComponent::timerCallback() {
    // Always refresh shadow buffer so UI reflects edits even when DAW is stopped
    processor.engine.updateShadow();
    repaint();
}

bool GridComponent::keyPressed(const juce::KeyPress& key) {
    auto& grid = processor.engine.grid;
    int code = key.getKeyCode();
    bool shift = key.getModifiers().isShiftDown();
    bool cmd = key.getModifiers().isCommandDown();

    // Cmd+S: save as
    if (cmd && code == 'S') {
        saveAs();
        return true;
    }

    // Cmd+G: set groove
    if (cmd && code == 'G') {
        auto* aw = new juce::AlertWindow("Set Groove",
            "Enter groove ratios (semicolon-separated, 50=normal).\n"
            "Example: 75;25 for swing, 50 for straight.",
            juce::AlertWindow::NoIcon);
        // Pre-fill with current groove
        juce::String currentGroove;
        for (int i = 0; i < processor.grooveLength; i++) {
            if (i > 0) currentGroove += ";";
            currentGroove += juce::String((int)(processor.grooves[i] * 50.0));
        }
        aw->addTextEditor("groove", currentGroove, "Groove values:");
        aw->addButton("OK", 1);
        aw->addButton("Cancel", 0);
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, aw](int result) {
                if (result == 1) {
                    auto text = aw->getTextEditorContents("groove");
                    auto parts = juce::StringArray::fromTokens(text, ";", "");
                    if (parts.size() > 0) {
                        double ratios[OrcaProcessor::kMaxGrooveSlots];
                        double sum = 0.0;
                        int count = juce::jmin(parts.size(), (int)OrcaProcessor::kMaxGrooveSlots - 1);
                        for (int i = 0; i < count; i++) {
                            int v = parts[i].getIntValue();
                            if (v == 0) v = 50;
                            ratios[i] = v / 50.0;
                            sum += ratios[i];
                        }
                        // Append closing ratio so the cycle averages to 1.0
                        ratios[count] = (count + 1) - sum;
                        count++;
                        processor.setGroove(ratios, count);

                        // Sync the DAW shuffle slider to match (centered: 100=straight)
                        // swing = (pct-100)/100 * 0.98, so pct = swing/0.98 * 100 + 100
                        if (count == 3) {
                            double swing = ratios[0] - 1.0; // positive = swing, negative = inverse
                            float pct = (float)(swing / 0.98 * 100.0 + 100.0);
                            pct = juce::jlimit(0.0f, 200.0f, pct);
                            processor.shuffleParam->setValueNotifyingHost(
                                processor.shuffleParam->getNormalisableRange().convertTo0to1(pct));
                            processor.lastShuffleValue = pct;
                        } else {
                            // Reset to center (straight)
                            float center = 100.0f;
                            processor.shuffleParam->setValueNotifyingHost(
                                processor.shuffleParam->getNormalisableRange().convertTo0to1(center));
                            processor.lastShuffleValue = center;
                        }
                    }
                }
                delete aw;
            }), true);
        return true;
    }

    // Cmd+O: open .orca file
    if (cmd && code == 'O') {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Open .orca file", juce::File(), "*");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.existsAsFile() && result.hasFileExtension("orca"))
                    loadOrcaFile(result);
            });
        return true;
    }

    // Cmd+=: zoom in, Cmd+-: zoom out
    if (cmd && (code == '+' || code == '=' || key.getTextCharacter() == '+' || key.getTextCharacter() == '=')) {
        updateFontSize(fontSize + 1.0f);
        if (auto* editor = findParentComponentOfClass<OrcaEditor>()) {
            auto& grid = processor.engine.grid;
            int newW = juce::jmax(400, (int)(grid.w * tileW) + 4);
            int newH = juce::jmax(300, (int)(grid.h * tileH) + 32);
            editor->setSize(newW, newH);
        }
        return true;
    }
    if (cmd && (code == '-' || key.getTextCharacter() == '-')) {
        updateFontSize(fontSize - 1.0f);
        if (auto* editor = findParentComponentOfClass<OrcaEditor>()) {
            auto& grid = processor.engine.grid;
            int newW = juce::jmax(400, (int)(grid.w * tileW) + 4);
            int newH = juce::jmax(300, (int)(grid.h * tileH) + 32);
            editor->setSize(newW, newH);
        }
        return true;
    }

    // Cmd+Z: undo, Cmd+Shift+Z: redo
    if (cmd && code == 'Z') {
        if (shift) redo(); else undo();
        return true;
    }

    // Cmd+C: copy
    if (cmd && code == 'C') {
        clipW = selectW;
        clipH = selectH;
        for (int y = 0; y < clipH; y++)
            for (int x = 0; x < clipW; x++)
                clipboard[x + clipW * y] = grid.glyphAt(cursorX + x, cursorY + y);
        return true;
    }

    // Cmd+X: cut
    if (cmd && code == 'X') {
        pushHistory();
        clipW = selectW;
        clipH = selectH;
        for (int y = 0; y < clipH; y++)
            for (int x = 0; x < clipW; x++) {
                clipboard[x + clipW * y] = grid.glyphAt(cursorX + x, cursorY + y);
                grid.write(cursorX + x, cursorY + y, '.');
            }
        return true;
    }

    // Cmd+V: paste
    if (cmd && code == 'V') {
        if (clipW > 0 && clipH > 0) {
            pushHistory();
            for (int y = 0; y < clipH; y++)
                for (int x = 0; x < clipW; x++)
                    grid.write(cursorX + x, cursorY + y, clipboard[x + clipW * y]);
        }
        return true;
    }

    // Arrow keys: move cursor or expand selection in all directions
    if (code == juce::KeyPress::leftKey) {
        if (shift) {
            if (cursorX > 0) { cursorX--; selectW++; }
        } else {
            cursorX = juce::jmax(0, cursorX - 1); selectW = 1; selectH = 1;
        }
        return true;
    }
    if (code == juce::KeyPress::rightKey) {
        if (shift) { selectW = juce::jmin(grid.w - cursorX, selectW + 1); }
        else { cursorX = juce::jmin(grid.w - 1, cursorX + 1); selectW = 1; selectH = 1; }
        return true;
    }
    if (code == juce::KeyPress::upKey) {
        if (shift) {
            if (cursorY > 0) { cursorY--; selectH++; }
        } else {
            cursorY = juce::jmax(0, cursorY - 1); selectW = 1; selectH = 1;
        }
        return true;
    }
    if (code == juce::KeyPress::downKey) {
        if (shift) { selectH = juce::jmin(grid.h - cursorY, selectH + 1); }
        else { cursorY = juce::jmin(grid.h - 1, cursorY + 1); selectW = 1; selectH = 1; }
        return true;
    }

    // Escape: deselect
    if (code == juce::KeyPress::escapeKey) {
        selectW = 1; selectH = 1;
        return true;
    }

    // Backspace/Delete: erase selection
    if (code == juce::KeyPress::backspaceKey || code == juce::KeyPress::deleteKey) {
        pushHistory();
        for (int y = 0; y < selectH; y++)
            for (int x = 0; x < selectW; x++)
                grid.write(cursorX + x, cursorY + y, '.');
        return true;
    }

    // Printable character: write glyph (cursor stays in place, like original Orca)
    auto textChar = key.getTextCharacter();
    if (textChar == '.') {
        pushHistory();
        grid.write(cursorX, cursorY, '.');
        return true;
    }
    if (textChar != 0 && orca::isOperatorGlyph(static_cast<char>(textChar))) {
        pushHistory();
        grid.write(cursorX, cursorY, static_cast<char>(textChar));
        return true;
    }

    return false;
}

void GridComponent::mouseDown(const juce::MouseEvent& event) {
    grabKeyboardFocus();
    dragOriginX = juce::jlimit(0, processor.engine.grid.w - 1,
                                (int)(event.position.x / tileW));
    dragOriginY = juce::jlimit(0, processor.engine.grid.h - 1,
                                (int)(event.position.y / tileH));
    cursorX = dragOriginX;
    cursorY = dragOriginY;
    selectW = 1;
    selectH = 1;
    repaint();
}

void GridComponent::mouseDrag(const juce::MouseEvent& event) {
    int dragX = juce::jlimit(0, processor.engine.grid.w - 1,
                              (int)(event.position.x / tileW));
    int dragY = juce::jlimit(0, processor.engine.grid.h - 1,
                              (int)(event.position.y / tileH));
    // Selection works in all directions from the click origin
    cursorX = juce::jmin(dragOriginX, dragX);
    cursorY = juce::jmin(dragOriginY, dragY);
    selectW = std::abs(dragX - dragOriginX) + 1;
    selectH = std::abs(dragY - dragOriginY) + 1;
    repaint();
}

// ── Undo/Redo ──

void GridComponent::pushHistory() {
    // Discard any redo history beyond current position
    if (historyPos < historyCount - 1)
        historyCount = historyPos + 1;

    // If full, shift everything left
    if (historyCount >= kMaxHistory) {
        for (int i = 0; i < kMaxHistory - 1; i++)
            history[i] = history[i + 1];
        historyCount = kMaxHistory - 1;
    }

    auto& snap = history[historyCount];
    auto& grid = processor.engine.grid;
    memcpy(snap.cells, grid.cells, grid.w * grid.h);
    snap.w = grid.w;
    snap.h = grid.h;
    historyCount++;
    historyPos = historyCount - 1;
}

void GridComponent::undo() {
    if (historyPos <= 0) return;
    historyPos--;
    auto& snap = history[historyPos];
    processor.engine.load(snap.w, snap.h, snap.cells, processor.engine.grid.f);
}

void GridComponent::redo() {
    if (historyPos >= historyCount - 1) return;
    historyPos++;
    auto& snap = history[historyPos];
    processor.engine.load(snap.w, snap.h, snap.cells, processor.engine.grid.f);
}

// ── File loading ──

bool GridComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (auto& f : files)
        if (f.endsWith(".orca")) return true;
    return false;
}

void GridComponent::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) {
    for (auto& f : files) {
        if (f.endsWith(".orca")) {
            loadOrcaFile(juce::File(f));
            break;
        }
    }
}

void GridComponent::loadOrcaFile(const juce::File& file) {
    auto content = file.loadFileAsString();
    if (content.isEmpty()) return;

    // Parse .orca format: lines of characters
    auto lines = juce::StringArray::fromLines(content);

    // Remove trailing empty lines
    while (lines.size() > 0 && lines[lines.size() - 1].trimEnd().isEmpty())
        lines.remove(lines.size() - 1);

    if (lines.isEmpty()) return;

    // Find dimensions
    int h = lines.size();
    int w = 0;
    for (auto& line : lines)
        w = juce::jmax(w, line.length());

    w = juce::jmin(w, orca::kMaxGridW);
    h = juce::jmin(h, orca::kMaxGridH);

    // Build flat grid string
    char buf[orca::kMaxGridSize];
    for (int i = 0; i < w * h; i++) buf[i] = '.';

    for (int y = 0; y < h; y++) {
        auto& line = lines[y];
        for (int x = 0; x < w && x < line.length(); x++) {
            char c = static_cast<char>(line[x]);
            buf[x + w * y] = c;
        }
    }

    processor.engine.load(w, h, buf, 0);
    currentFile = file;

    // Resize window to fit
    if (auto* editor = findParentComponentOfClass<OrcaEditor>()) {
        int newWidth = (int)(w * tileW) + 4;
        int newHeight = (int)(h * tileH) + 24;
        editor->setSize(juce::jmax(400, newWidth), juce::jmax(300, newHeight));
    }
}

// ── Save .orca file ──

void GridComponent::saveOrcaFile(const juce::File& file) {
    auto& grid = processor.engine.grid;
    char buf[orca::kMaxGridSize + orca::kMaxGridH]; // grid + newlines
    int len = grid.format(buf, sizeof(buf));
    file.replaceWithData(buf, (size_t)len);
    currentFile = file;
}

void GridComponent::saveAs() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save .orca file", currentFile.existsAsFile() ? currentFile : juce::File(), "*.orca");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result != juce::File())
                saveOrcaFile(result.withFileExtension("orca"));
        });
}

// ── Operator name lookup ──

const char* GridComponent::operatorName(char glyph) {
    switch (orca::toLower(glyph)) {
        case 'a': return "add";       case 'b': return "subtract";
        case 'c': return "clock";     case 'd': return "delay";
        case 'e': return "east";      case 'f': return "if";
        case 'g': return "generator"; case 'h': return "halt";
        case 'i': return "increment"; case 'j': return "jumper";
        case 'k': return "konkat";    case 'l': return "lesser";
        case 'm': return "multiply";  case 'n': return "north";
        case 'o': return "read";      case 'p': return "push";
        case 'q': return "query";     case 'r': return "random";
        case 's': return "south";     case 't': return "track";
        case 'u': return "uclid";     case 'v': return "variable";
        case 'w': return "west";      case 'x': return "write";
        case 'y': return "jymper";    case 'z': return "lerp";
        case ':': return "midi";      case '!': return "cc";
        case '?': return "pb";        case '%': return "mono";
        case '=': return "osc";       case ';': return "udp";
        case '*': return "bang";      case '#': return "comment";
        case '~': return "probability"; case '^': return "scale";
        case '{': return "buffer";  case '}': return "freeze";
        case '|': return "gate";    case '&': return "arp";
        case '@': return "markov";
        case '[': return "strum";
        case ']': return "chord";
        case '>': return "humanize";
        case '<': return "ratchet";
        case '\\': return "swing";
        default:  return "empty";
    }
}

// ── Port name lookup ──

const char* GridComponent::portName(char ownerGlyph, int portIdx) {
    // Port names match the original Orca JS library port definitions.
    // portIdx corresponds to the slot in OperatorInstance::ports[].
    char g = orca::toLower(ownerGlyph);

    // Letter operators with (a, b, output) pattern: a, b, f, l, m, r
    if (g == 'a' || g == 'b' || g == 'f' || g == 'l' || g == 'm' || g == 'r') {
        switch (portIdx) {
            case 0: return "a";  case 1: return "b";  case 2: return "output";
        }
    }

    // Rate/mod/output pattern: c, d
    if (g == 'c' || g == 'd') {
        switch (portIdx) {
            case 0: return "rate";  case 1: return "mod";  case 2: return "output";
        }
    }

    // Step/mod/output: i
    if (g == 'i') {
        switch (portIdx) {
            case 0: return "step";  case 1: return "mod";  case 2: return "output";
        }
    }

    // Step/max/output: u
    if (g == 'u') {
        switch (portIdx) {
            case 0: return "step";  case 1: return "max";  case 2: return "output";
        }
    }

    // Rate/target/output: z
    if (g == 'z') {
        switch (portIdx) {
            case 0: return "rate";  case 1: return "target";  case 2: return "output";
        }
    }

    // x/y/len: g, q
    if (g == 'g' || g == 'q') {
        switch (portIdx) {
            case 0: return "x";  case 1: return "y";  case 2: return "len";
        }
    }

    // x/y/output: o
    if (g == 'o') {
        switch (portIdx) {
            case 0: return "x";  case 1: return "y";  case 2: return "output";
        }
    }

    // key/len/val: p
    if (g == 'p') {
        switch (portIdx) {
            case 0: return "key";  case 1: return "len";  case 2: return "val";  case 3: return "output";
        }
    }

    // key/len/output: t
    if (g == 't') {
        switch (portIdx) {
            case 0: return "key";  case 1: return "len";  case 2: return "output";
        }
    }

    // x/y/val: x
    if (g == 'x') {
        switch (portIdx) {
            case 0: return "x";  case 1: return "y";  case 2: return "val";  case 3: return "output";
        }
    }

    // write/read: v
    if (g == 'v') {
        switch (portIdx) {
            case 0: return "write";  case 1: return "read";  case 2: return "output";
        }
    }

    // halt: h
    if (g == 'h') {
        if (portIdx == 2) return "output";
    }

    // konkat: k
    if (g == 'k') {
        if (portIdx == 0) return "len";
    }

    // MIDI operators: : and %
    if (g == ':' || g == '%') {
        switch (portIdx) {
            case 0: return "channel";  case 1: return "octave";  case 2: return "note";
            case 3: return "velocity"; case 4: return "length";
        }
    }

    // CC: !
    if (g == '!') {
        switch (portIdx) {
            case 0: return "channel";  case 1: return "knob";  case 2: return "value";
        }
    }

    // Pitch bend: ?
    if (g == '?') {
        switch (portIdx) {
            case 0: return "channel";  case 1: return "lsb";  case 2: return "msb";
        }
    }

    // Jumper/jymper: j, y — dynamic ports
    if (g == 'j' || g == 'y') {
        switch (portIdx) {
            case 0: return "input";  case 1: return "output";
        }
    }

    // Probability: ~
    if (ownerGlyph == '~') {
        switch (portIdx) {
            case 0: return "chance";  case 1: return "output";
        }
    }

    // Scale: ^
    if (ownerGlyph == '^') {
        switch (portIdx) {
            case 0: return "note";  case 1: return "scale";  case 2: return "output";
        }
    }

    // Buffer: {
    if (ownerGlyph == '{') {
        switch (portIdx) {
            case 0: return "len";  case 1: return "value";
        }
    }

    // Freeze: }
    if (ownerGlyph == '}') {
        switch (portIdx) {
            case 0: return "value";  case 1: return "output";
        }
    }

    // Gate: |
    if (ownerGlyph == '|') {
        switch (portIdx) {
            case 0: return "threshold";  case 1: return "value";  case 2: return "output";
        }
    }

    // Arp: &
    if (ownerGlyph == '&') {
        switch (portIdx) {
            case 0: return "speed";  case 1: return "pattern";  case 2: return "len";
        }
    }

    // Markov: @
    if (ownerGlyph == '@') {
        switch (portIdx) {
            case 0: return "len";  case 1: return "output";
        }
    }

    // Strum: [
    if (ownerGlyph == '[') {
        switch (portIdx) {
            case 0: return "len";  case 1: return "rate";
        }
    }

    // Chord: ]
    if (ownerGlyph == ']') {
        switch (portIdx) {
            case 0: return "root";  case 1: return "type";
        }
    }

    // Humanize: >
    if (ownerGlyph == '>') {
        if (portIdx == 0) return "max";
    }

    // Ratchet: <
    if (ownerGlyph == '<') {
        switch (portIdx) {
            case 0: return "subdivisions";  case 1: return "period";
        }
    }

    // Swing: backslash
    if (ownerGlyph == '\\') {
        if (portIdx == 0) return "delay";
    }

    return "";
}

// ── OrcaEditor ──

OrcaEditor::OrcaEditor(OrcaProcessor& p)
    : AudioProcessorEditor(&p), orcaProcessor(p), gridComponent(p)
{
    addAndMakeVisible(gridComponent);
    setSize(p.editorWidth, p.editorHeight);
    setResizable(true, true);
}

OrcaEditor::~OrcaEditor() {
    // Remove the grid component before this editor is destroyed,
    // so its destructor runs while the processor reference is still valid.
    removeChildComponent(&gridComponent);
}

void OrcaEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff000000));
}

void OrcaEditor::resized() {
    gridComponent.setBounds(getLocalBounds());
    // Persist window size so it restores when reopened
    orcaProcessor.editorWidth = getWidth();
    orcaProcessor.editorHeight = getHeight();
}
