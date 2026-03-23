#include "PluginEditor.h"

// Stamp pattern categories
static constexpr struct StampCategoryDef { int start; int count; const char* name; } stampCategories[] = {
    { 0,  11, "Still Lifes" },
    { 11, 14, "Oscillators" },
    { 25,  9, "Spaceships" },
    { 34,  9, "Methuselahs" },
    { 43,  3, "Guns" },
    { 46,  2, "Puffers" },
};
static constexpr int numStampCategories = 6;

// ── GridComponent ──

GridComponent::GridComponent(OrcaProcessor& p)
    : processor(p)
{
    setWantsKeyboardFocus(true);
    startTimerHz(30); // 30fps UI refresh

    // Use a monospace font
    updateFontSize(fontSize);
}

void GridComponent::visibilityChanged() {
    if (isVisible())
        grabKeyboardFocus();
}

GridComponent::~GridComponent() {
    stopTimer();
    fileChooser.reset();
    delete[] lifeHistory;
}

void GridComponent::paint(juce::Graphics& g) {
    g.fillAll(bgColor);

    auto& engine = processor.engine;
    int gridW = engine.shadowW;
    int gridH = engine.shadowH;

    g.setFont(monoFont);

    // Phase offset: row group guides + active phase highlight
    if (engine.lifeMode && engine.lifeGrid.seqMode != orca::LifeGrid::SeqOff && engine.lifeGrid.evolveRate > 1) {
        int currentFrame = engine.lifeGrid.frameCounter;
        float totalW = gridW * tileW;

        // Draw separator lines between phase groups
        int prevPhase = -1;
        for (int y = 0; y < gridH; y++) {
            int ph = engine.lifeGrid.phaseForRow(y);
            if (ph != prevPhase && prevPhase >= 0) {
                float ly = y * tileH;
                g.setColour(juce::Colour(0x30ffffff));
                g.fillRect(0.0f, ly, totalW, 1.0f);
            }
            prevPhase = ph;
        }

        // Highlight the active phase rows
        for (int y = 0; y < gridH; y++) {
            if (engine.lifeGrid.phaseForRow(y) == currentFrame) {
                float py = y * tileH;
                g.setColour(juce::Colour(0x15ffffff));
                g.fillRect(0.0f, py, totalW, tileH);
            }
        }
    }

    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            int idx = x + gridW * y;
            bool isSelected = !stampMode &&
                               (x >= cursorX && x < cursorX + selectW &&
                                y >= cursorY && y < cursorY + selectH);
            if (engine.lifeMode) {
                drawLifeCell(g, x, y, engine.shadowLife[idx], isSelected);
            } else {
                char glyph = engine.shadowCells[idx];
                uint8_t portType = engine.shadowPorts[idx];
                bool isLocked = engine.shadowLocks[idx];
                drawCell(g, x, y, glyph, portType, isLocked, isSelected);
            }
        }
    }

    // Draw stamp mode ghost overlay
    if (stampMode && engine.lifeMode) {
        auto& p = orca::builtInPatterns[stampIndex];
        auto channelCol = juce::Colour(channelColorValues[engine.paintChannel % 16]);
        for (int py = 0; py < p.h; py++) {
            for (int px = 0; px < p.w; px++) {
                int srcIdx = px + p.w * py;
                if (p.data[srcIdx] != 'X') continue;
                int gx = cursorX + px, gy = cursorY + py;
                if (gx >= gridW || gy >= gridH) continue;
                float cellX = gx * tileW;
                float cellY = gy * tileH;
                g.setColour(channelCol.withAlpha(0.35f));
                g.fillRect(cellX, cellY, tileW, tileH);
                g.setColour(channelCol.withAlpha(0.7f));
                g.drawRect(cellX, cellY, tileW, tileH, 1.0f);
            }
        }
    }

    // Draw find preview highlights
    if (commander.hasFindPreview && commander.findHighlights.size() > 0) {
        g.setColour(bMed.withAlpha(0.4f));
        for (int idx : commander.findHighlights) {
            int hx = idx % gridW;
            int hy = idx / gridW;
            g.fillRect(hx * tileW, hy * tileH, tileW, tileH);
        }
    }

    // Draw cursor marker on top of selection
    {
        float cx = cursorX * tileW;
        float cy = cursorY * tileH;
        if (stampMode) {
            // In stamp mode, just draw a subtle outline so the pattern preview is visible
            g.setColour(juce::Colour(0x88ffffff));
            g.drawRect(cx, cy, tileW, tileH, 1.0f);
        } else {
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
    }

    // Two-line status bar
    g.setFont(monoFont);
    float statusY = getHeight() - 30.0f;

    if (commander.isActive) {
        // Commander prompt overlay — draw text with inline cursor
        g.setColour(bInv);
        juce::String prefix = "> ";
        juce::String before = prefix + commander.query.substring(0, commander.cursorPos);
        juce::String after  = commander.query.substring(commander.cursorPos);

        // Measure text widths to position cursor
        auto font = g.getCurrentFont();
        float beforeW = font.getStringWidthFloat(before);
        float cursorX = 4.0f + beforeW;

        // Draw full text
        g.drawText(prefix + commander.query, 4, (int)statusY, getWidth() - 8, 14,
                   juce::Justification::centredLeft);

        // Draw blinking cursor bar
        if (engine.shadowF % 2 == 0) {
            g.fillRect(cursorX, statusY + 1.0f, 1.5f, 12.0f);
        }

        // Second line: hint
        g.setColour(fLow);
        g.drawText("Enter to execute, Esc to cancel", 4, (int)(statusY + 14),
                   getWidth() - 8, 14, juce::Justification::centredLeft);
    } else if (engine.lifeMode) {
        // Life mode status bar
        // Line 1: LIFE  ch:X  oct:X  cursor_pos  generation
        {
            g.setColour(juce::Colour(channelColorValues[engine.paintChannel % 16]));
            juce::String line1;
            line1 << "LIFE  ch:" << (int)engine.paintChannel
                  << "  oct:" << (int)engine.paintOctave
                  << "  " << orca::LifeGrid::rootNoteName(engine.lifeGrid.rootNote)
                  << " " << orca::LifeGrid::scaleName(engine.lifeGrid.currentScale)
                  << "  " << (engine.lifeGrid.pulseMode ? "pulse" : "hold")
                  << "  rate:" << engine.lifeGrid.evolveRate
                  << (engine.lifeGrid.decay ? juce::String("  decay:") + juce::String((int)engine.lifeGrid.minVelocity) + "v/" + juce::String(engine.lifeGrid.minProb) + "%" : juce::String())
                  << (engine.lifeGrid.maxNotes > 0 ? juce::String("  max:") + juce::String(engine.lifeGrid.maxNotes) : juce::String())
                  << (engine.lifeGrid.seqMode == orca::LifeGrid::SeqForward ? "  seq" :
                      engine.lifeGrid.seqMode == orca::LifeGrid::SeqReverse ? "  seq:rev" :
                      engine.lifeGrid.seqMode == orca::LifeGrid::SeqMirror ? "  seq:mirror" :
                      engine.lifeGrid.seqMode == orca::LifeGrid::SeqRandom ? "  seq:random" : "")
                  << (engine.lifeGrid.lockOctave ? "  lockoct" : "")
                  << (engine.lifeGrid.chordDegreeCount > 0 ? juce::String("  chord:") + engine.lifeGrid.chordDegreesString() : juce::String())
                  << (engine.lifeGrid.dedup ? juce::String("  dedup") + (engine.lifeGrid.dedupCC >= 0 ? juce::String(" cc:") + juce::String(engine.lifeGrid.dedupCC) : juce::String()) : juce::String())
                  << (strcmp(engine.lifeGrid.ruleString, "23/3") != 0 ? juce::String("  rule:") + juce::String(engine.lifeGrid.ruleString) : juce::String())
                  << ((engine.lifeGrid.minOctave != 0 || engine.lifeGrid.maxOctave != 7) ?
                      juce::String("  oct:") + juce::String(engine.lifeGrid.minOctave) + "-" + juce::String(engine.lifeGrid.maxOctave) :
                      juce::String())
                  << "   " << cursorX << "," << cursorY
                  << "   gen:" << engine.shadowF;
            {
            }
            if (stampMode)
                line1 << "   STAMP: " << stampCategories[stampCategory].name
                      << " > " << orca::builtInPatterns[stampIndex].name;
            g.drawText(line1, 4, (int)statusY, getWidth() - 8, 14,
                       juce::Justification::centredLeft);
        }
        // Line 2: grid_size  population
        {
            g.setColour(fLow);
            juce::String line2;
            line2 << gridW << "x" << gridH
                  << "   pop:" << engine.lifeGrid.population()
                  << "   udp:" << processor.udpOutputPort
                  << "  osc:" << processor.oscOutputPort;
            g.drawText(line2, 4, (int)(statusY + 14), getWidth() - 8, 14,
                       juce::Justification::centredLeft);
        }
    } else {
        // Normal Orca status bar
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
            if (!injectCache.empty())
                line2 << "   " << (int)injectCache.size() << " mods";
            line2 << "   udp:" << processor.udpOutputPort
                  << "  osc:" << processor.oscOutputPort;
            g.drawText(line2, 4, (int)(statusY + 14), getWidth() - 8, 14,
                       juce::Justification::centredLeft);
        }
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

void GridComponent::drawLifeCell(juce::Graphics& g, int x, int y,
                                  const orca::LifeCell& cell, bool isSelected) {
    float px = x * tileW;
    float py = y * tileH;

    if (isSelected) {
        g.setColour(bInv);
        g.fillRect(px, py, tileW, tileH);
        g.setColour(fInv);
        char ch = cell.alive ? cell.note : '.';
        g.drawText(juce::String::charToString(ch),
                   (int)px, (int)py, (int)tileW, (int)tileH,
                   juce::Justification::centred);
        return;
    }

    if (!cell.alive) {
        // Dead cell: show grid markers like normal mode
        bool isMarker = (x % 8 == 0 && y % 8 == 0);
        bool isLocalGuide = (x % 2 == 0 && y % 2 == 0) &&
                            (std::abs(x - cursorX) <= 6 && std::abs(y - cursorY) <= 6);
        if (isMarker) {
            g.setColour(fLow);
            g.drawText("+", (int)px, (int)py, (int)tileW, (int)tileH,
                       juce::Justification::centred);
        } else if (isLocalGuide) {
            g.setColour(fLow);
            float dotX = px + tileW * 0.5f;
            float dotY = py + tileH * 0.5f;
            g.fillRect(dotX, dotY, 1.0f, 1.0f);
        }
        return;
    }

    // Alive cell: color by channel hue, brightness by octave
    juce::Colour baseCol(channelColorValues[cell.channel % 16]);
    // minOctave = 30% brightness, maxOctave = 100%
    float range = juce::jmax(1.0f, (float)(processor.engine.lifeGrid.maxOctave - processor.engine.lifeGrid.minOctave));
    float brightness = 0.3f + ((cell.octave - processor.engine.lifeGrid.minOctave) / range) * 0.7f;
    juce::Colour col = baseCol.withMultipliedBrightness(brightness);

    g.setColour(col);
    g.fillRect(px, py, tileW, tileH);
    // Draw note letter in white
    g.setColour(juce::Colour(0xffffffff));
    g.drawText(juce::String::charToString(cell.note),
               (int)px, (int)py, (int)tileW, (int)tileH,
               juce::Justification::centred);
    // Fired cells get a white border, locked cells get a dimmer one
    if (cell.fired) {
        g.setColour(juce::Colour(0xffffffff));
        g.drawRect(px, py, tileW, tileH, 1.0f);
    } else if (cell.locked) {
        g.setColour(juce::Colour(0x80ffffff));
        g.drawRect(px, py, tileW, tileH, 1.0f);
    }
}

void GridComponent::updateFontSize(float newSize) {
    fontSize = juce::jlimit(8.0f, 36.0f, newSize);
    monoFont = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), fontSize, juce::Font::plain));
    tileW = std::round(fontSize * 0.833f);  // ~10/12 ratio
    tileH = std::round(fontSize * 1.25f);   // ~15/12 ratio
}

void GridComponent::resized() {
    int viewW = juce::jmax(1, (int)(getWidth() / tileW));
    int viewH = juce::jmax(1, (int)((getHeight() - 32.0f) / tileH));
    viewW = juce::jmin(viewW, orca::kMaxGridW);
    viewH = juce::jmin(viewH, orca::kMaxGridH);

    auto& grid = processor.engine.grid;
    // Grow Orca grid storage (never shrink — preserves off-screen content)
    int newW = juce::jmax(grid.w, viewW);
    int newH = juce::jmax(grid.h, viewH);

    const juce::SpinLock::ScopedLockType lock(processor.engineLock);

    if (newW != grid.w || newH != grid.h) {
        char buf[orca::kMaxGridSize];
        char newCells[orca::kMaxGridSize];
        memcpy(buf, grid.cells, grid.w * grid.h);

        for (int i = 0; i < newW * newH; i++) newCells[i] = '.';
        for (int y = 0; y < grid.h; y++) {
            for (int x = 0; x < grid.w; x++) {
                newCells[x + newW * y] = buf[x + grid.w * y];
            }
        }
        processor.engine.load(newW, newH, newCells, grid.f);
    }

    // Update Life grid wrap boundary to match visible area
    if (processor.engine.lifeMode)
        processor.engine.lifeGrid.setWrapSize(viewW, viewH);
}

void GridComponent::timerCallback() {
    // Snapshot all pending messages under lock, then process outside lock.
    // This keeps the lock held for only a tiny memcpy, never during I/O.

    char localCmds[OrcaProcessor::kMaxPendingCommands][OrcaProcessor::kMaxPendingCommandLen];
    int localCmdCount = 0;

    char localUdp[OrcaProcessor::kMaxPendingUdp][OrcaProcessor::kMaxPendingUdpLen];
    int localUdpCount = 0;

    OrcaProcessor::PendingOscMsg localOsc[OrcaProcessor::kMaxPendingOsc];
    int localOscCount = 0;

    // Also snapshot network config (protects juce::String from setStateInformation on background thread)
    juce::String localUdpIP;
    int localUdpPort = 0;
    int localOscPort = 0;

    {
        const juce::SpinLock::ScopedLockType lock(processor.pendingLock);

        localCmdCount = processor.pendingCommandCount;
        for (int i = 0; i < localCmdCount; i++)
            memcpy(localCmds[i], processor.pendingCommands[i], OrcaProcessor::kMaxPendingCommandLen);
        processor.pendingCommandCount = 0;

        localUdpCount = processor.pendingUdpCount;
        for (int i = 0; i < localUdpCount; i++)
            memcpy(localUdp[i], processor.pendingUdp[i], OrcaProcessor::kMaxPendingUdpLen);
        processor.pendingUdpCount = 0;

        localOscCount = processor.pendingOscCount;
        for (int i = 0; i < localOscCount; i++)
            localOsc[i] = processor.pendingOsc[i];
        processor.pendingOscCount = 0;

        localUdpIP = processor.udpTargetIP;
        localUdpPort = processor.udpOutputPort;
        localOscPort = processor.oscOutputPort;
    }

    // Process $ operator commands
    for (int i = 0; i < localCmdCount; i++) {
        commander.query = juce::String(localCmds[i]);
        commander.trigger(processor, *this);
    }

    // Send UDP messages
    if (localUdpCount > 0) {
        DBG("UDP: sending " + juce::String(localUdpCount) + " message(s) to " + localUdpIP + ":" + juce::String(localUdpPort));
        if (!processor.udpSocket)
            processor.setupUdpSocket();
        if (processor.udpSocket) {
            for (int i = 0; i < localUdpCount; i++) {
                int len = 0;
                while (len < OrcaProcessor::kMaxPendingUdpLen && localUdp[i][len] != '\0') len++;
                int sent = processor.udpSocket->write(localUdpIP, localUdpPort,
                                            localUdp[i], len);
                DBG("  UDP msg[" + juce::String(i) + "]: \"" + juce::String(localUdp[i]) + "\" len=" + juce::String(len) + " sent=" + juce::String(sent));
            }
        } else {
            DBG("  UDP socket is null after setup attempt!");
        }
    }

    // Send OSC messages — reconnect if port/IP changed
    if (localOscCount > 0) {
        DBG("OSC: sending " + juce::String(localOscCount) + " message(s) to " + localUdpIP + ":" + juce::String(localOscPort));
        if (!processor.oscSender) {
            processor.oscSender = std::make_unique<juce::OSCSender>();
            if (!processor.oscSender->connect(localUdpIP, localOscPort))
                DBG("OSC: Failed to connect to " + localUdpIP + ":" + juce::String(localOscPort));
            else
                DBG("OSC: Connected to " + localUdpIP + ":" + juce::String(localOscPort));
        }
        for (int i = 0; i < localOscCount; i++) {
            auto& msg = localOsc[i];
            juce::String path = "/" + juce::String::charToString(msg.path);
            juce::OSCMessage oscMsg(path);
            // Append each char as its base-36 value (matching original Orca)
            for (int j = 0; j < msg.dataLen; j++) {
                oscMsg.addInt32(orca::valueOf(msg.data[j]));
            }
            bool ok = processor.oscSender->send(oscMsg);
            DBG("  OSC msg[" + juce::String(i) + "]: path=" + path + " values=" + juce::String(msg.dataLen) + " sent=" + juce::String(ok ? 1 : 0));
        }
    }

    // Always refresh shadow buffer so UI reflects edits even when DAW is stopped.
    // When transport is running, skip grid.parse() (step() already did it) and
    // lock engineLock so we don't race with processBlock's engine.step().
    {
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        processor.engine.updateShadow(processor.transportRunning.load(std::memory_order_relaxed));
    }
    repaint();
}

bool GridComponent::keyPressed(const juce::KeyPress& key) {
    auto& grid = processor.engine.grid;
    int code = key.getKeyCode();
    bool shift = key.getModifiers().isShiftDown();
    bool cmd = key.getModifiers().isCommandDown();
    bool alt = key.getModifiers().isAltDown();

    // ── Commander input intercept ──
    if (commander.isActive) {
        if (code == juce::KeyPress::returnKey) {
            commander.trigger(processor, *this);
            return true;
        }
        if (code == juce::KeyPress::escapeKey) {
            commander.stop();
            return true;
        }

        // Backspace: delete char before cursor
        if (code == juce::KeyPress::backspaceKey) {
            if (cmd) commander.killToStart(); // Cmd+Backspace: delete to start
            else     commander.erase();
            commander.preview(processor, *this);
            return true;
        }

        // Delete: delete char after cursor
        if (code == juce::KeyPress::deleteKey) {
            commander.eraseForward();
            commander.preview(processor, *this);
            return true;
        }

        // Arrow keys: cursor movement
        if (code == juce::KeyPress::leftKey) {
            if (cmd)       commander.moveCursorHome();      // Cmd+Left: start
            else if (alt)  commander.moveCursorWordLeft();   // Alt+Left: prev word
            else           commander.moveCursorLeft();
            return true;
        }
        if (code == juce::KeyPress::rightKey) {
            if (cmd)       commander.moveCursorEnd();        // Cmd+Right: end
            else if (alt)  commander.moveCursorWordRight();  // Alt+Right: next word
            else           commander.moveCursorRight();
            return true;
        }
        if (code == juce::KeyPress::homeKey) { commander.moveCursorHome(); return true; }
        if (code == juce::KeyPress::endKey)  { commander.moveCursorEnd();  return true; }

        // History
        if (code == juce::KeyPress::upKey)   { commander.historyUp();   return true; }
        if (code == juce::KeyPress::downKey) { commander.historyDown(); return true; }

        // Cmd shortcuts
        if (cmd) {
            if (code == 'A' || code == 'a') {
                // Cmd+A: select all (move cursor to end)
                commander.selectAll();
                return true;
            }
            if (code == 'C' || code == 'c') {
                // Cmd+C: copy entire query
                juce::SystemClipboard::copyTextToClipboard(commander.query);
                return true;
            }
            if (code == 'V' || code == 'v') {
                // Cmd+V: paste at cursor
                auto clip = juce::SystemClipboard::getTextFromClipboard();
                // Strip newlines — single line only
                clip = clip.replaceCharacters("\r\n", "  ").trim();
                if (clip.isNotEmpty()) {
                    commander.insertAtCursor(clip);
                    commander.preview(processor, *this);
                }
                return true;
            }
            if (code == 'X' || code == 'x') {
                // Cmd+X: cut (copy + clear)
                juce::SystemClipboard::copyTextToClipboard(commander.query);
                commander.replaceQuery("");
                commander.preview(processor, *this);
                return true;
            }
            if (code == 'K' || code == 'k') {
                // Cmd+K while commander is open: kill to end of line
                commander.killToEnd();
                commander.preview(processor, *this);
                return true;
            }
            return true; // consume other Cmd combos
        }

        // Accumulate printable characters
        auto ch = key.getTextCharacter();
        if (ch != 0 && ch >= 32) {
            commander.write(ch);
            commander.preview(processor, *this);
        }
        return true; // consume all keys while commander is active
    }

    // Space bar: toggle standalone transport (play/pause) — only in standalone mode
    if (code == juce::KeyPress::spaceKey && !cmd && !shift
        && processor.wrapperType == OrcaProcessor::wrapperType_Standalone) {
        bool wasOn = processor.standalonePlay.load(std::memory_order_relaxed);
        processor.standalonePlay.store(!wasOn, std::memory_order_relaxed);
        return true;
    }

    // Cmd+K: open commander
    if (cmd && !shift && code == 'K') {
        commander.start();
        return true;
    }

    // Cmd+F: open commander with find:
    if (cmd && !shift && code == 'F') {
        commander.start("find:");
        return true;
    }

    // Cmd+L: toggle Life mode
    // Cmd+G: toggle Life (Game of Life) mode
    if (cmd && !shift && code == 'G') {
        auto& engine = processor.engine;
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        if (engine.lifeMode) {
            orca::MidiEvent events[orca::kMaxEvents];
            int eventCount = 0;
            engine.exitLifeMode(events, orca::kMaxEvents, eventCount);
            for (int i = 0; i < eventCount; i++)
                processor.pushLifeEvent(events[i]);
        } else {
            engine.enterLifeMode();
        }
        return true;
    }

    // Tab: cycle paint octave, Shift+Tab: cycle paint channel (Life mode)
    if (code == juce::KeyPress::tabKey && !cmd) {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            if (shift) {
                engine.paintChannel = (engine.paintChannel + 1) % 16;
            } else {
                engine.paintOctave = engine.paintOctave + 1;
                if (engine.paintOctave > engine.lifeGrid.maxOctave)
                    engine.paintOctave = engine.lifeGrid.minOctave;
            }
            return true;
        }
    }

    // Life mode: Cmd+R = reset to initial state
    if (cmd && !shift && code == 'R') {
        auto& engine = processor.engine;
        if (engine.lifeMode && engine.lifeGrid.hasInitialState) {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            orca::MidiEvent events[orca::kMaxEvents];
            int count = engine.lifeGrid.resetToInitial(events, orca::kMaxEvents);
            for (int i = 0; i < count; i++)
                processor.pushLifeEvent(events[i]);
            return true;
        }
    }

    // Life mode: Cmd+Shift+R = save current state as new initial
    if (cmd && shift && code == 'R') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            engine.lifeGrid.saveInitialState();
            return true;
        }
    }

    // Life mode: Cmd+Shift+S = cycle scale
    if (cmd && shift && code == 'S') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            int s = static_cast<int>(engine.lifeGrid.currentScale) + 1;
            if (s >= orca::LifeGrid::NumScales) s = 0;
            engine.lifeGrid.currentScale = static_cast<orca::LifeGrid::ScaleType>(s);
            return true;
        }
    }

    // Life mode: Cmd+M = toggle pulse/hold mode
    if (cmd && !shift && code == 'M') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            engine.lifeGrid.pulseMode = !engine.lifeGrid.pulseMode;
            return true;
        }
    }

    // Life mode: Cmd+N = cycle root note (C, C#, D, ... B)
    if (cmd && !shift && code == 'N') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            engine.lifeGrid.rootNote = (engine.lifeGrid.rootNote + 1) % 12;
            return true;
        }
    }

    // Life mode: Cmd+Shift+K = toggle lock (protect) selected cells
    if (cmd && shift && code == 'K') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            auto& lg = engine.lifeGrid;
            pushLifeHistory();
            for (int dy = 0; dy < selectH; dy++)
                for (int dx = 0; dx < selectW; dx++) {
                    int idx = lg.indexAt(cursorX + dx, cursorY + dy);
                    if (idx >= 0 && lg.cells[idx].alive)
                        lg.cells[idx].locked = !lg.cells[idx].locked;
                }
            return true;
        }
    }

    // Life mode: Cmd+P / Cmd+Shift+P = enter/cycle stamp mode within category
    if (cmd && code == 'P') {
        auto& engine = processor.engine;
        if (engine.lifeMode) {
            if (!stampMode) {
                stampMode = true;
                stampCategory = 0;
                stampIndex = stampCategories[0].start;
            } else {
                auto& cat = stampCategories[stampCategory];
                int posInCat = stampIndex - cat.start;
                if (shift)
                    posInCat = (posInCat - 1 + cat.count) % cat.count;
                else
                    posInCat = (posInCat + 1) % cat.count;
                stampIndex = cat.start + posInCat;
            }
            return true;
        }
    }

    // Stamp mode: Cmd+] / Cmd+[ = cycle category
    if (stampMode && cmd && (code == ']' || code == '[')) {
        if (code == ']')
            stampCategory = (stampCategory + 1) % numStampCategories;
        else
            stampCategory = (stampCategory - 1 + numStampCategories) % numStampCategories;
        stampIndex = stampCategories[stampCategory].start;
        return true;
    }

    // Stamp mode: Enter = place pattern
    if (stampMode && code == juce::KeyPress::returnKey) {
        auto& p = orca::builtInPatterns[stampIndex];
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        auto& lg = processor.engine.lifeGrid;
        pushLifeHistory();

        for (int py = 0; py < p.h; py++) {
            for (int px = 0; px < p.w; px++) {
                int srcIdx = px + p.w * py;
                char c = p.data[srcIdx];
                int destIdx = lg.indexAt(cursorX + px, cursorY + py);
                if (destIdx < 0) continue;

                if (c == 'X') {
                    int sLen = orca::LifeGrid::scaleLengths[static_cast<int>(lg.currentScale)];
                    int deg = juce::Random::getSystemRandom().nextInt(sLen);
                    int semi = orca::LifeGrid::scaleNotes[static_cast<int>(lg.currentScale)][deg];
                    lg.cells[destIdx].alive = true;
                    lg.cells[destIdx].note = orca::LifeGrid::semitoneToNote[semi];
                    lg.cells[destIdx].channel = processor.engine.paintChannel;
                    lg.cells[destIdx].octave = processor.engine.paintOctave;
                    lg.cells[destIdx].locked = false;
                }
            }
        }
        selectW = 1;
        selectH = 1;
        stampMode = false;
        return true;
    }

    // Stamp mode: Escape = cancel
    if (stampMode && code == juce::KeyPress::escapeKey) {
        stampMode = false;
        return true;
    }

    // Life mode: Cmd+Shift+Up/Down = increase/decrease evolve rate
    if (cmd && shift && processor.engine.lifeMode &&
        (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)) {
        auto& lg = processor.engine.lifeGrid;
        if (code == juce::KeyPress::upKey)
            lg.evolveRateUp();
        else
            lg.evolveRateDown();
        return true;
    }

    // Life mode selection transforms (only when selection > 1x1)
    if (cmd && processor.engine.lifeMode && (selectW > 1 || selectH > 1)) {
        auto& lg = processor.engine.lifeGrid;

        // Cmd+E: rotate selection 90° clockwise
        if (code == 'E') {
            pushLifeHistory();
            orca::LifeCell temp[orca::kMaxGridSize];
            for (int dy = 0; dy < selectH; dy++)
                for (int dx = 0; dx < selectW; dx++) {
                    int srcIdx = lg.indexAt(cursorX + dx, cursorY + dy);
                    if (srcIdx >= 0)
                        temp[dx + selectW * dy] = lg.cells[srcIdx];
                    else
                        temp[dx + selectW * dy] = orca::LifeCell();
                }
            // Rotated: new width = old height, new height = old width
            int newW = selectH, newH = selectW;
            for (int dy = 0; dy < newH; dy++)
                for (int dx = 0; dx < newW; dx++) {
                    int srcX = dy;
                    int srcY = newW - 1 - dx;
                    int dstIdx = lg.indexAt(cursorX + dx, cursorY + dy);
                    if (dstIdx >= 0)
                        lg.cells[dstIdx] = temp[srcX + selectW * srcY];
                }
            // Clear cells outside new bounds but inside old bounds
            for (int dy = 0; dy < juce::jmax(selectH, newH); dy++)
                for (int dx = 0; dx < juce::jmax(selectW, newW); dx++) {
                    if (dx >= newW || dy >= newH) {
                        int idx = lg.indexAt(cursorX + dx, cursorY + dy);
                        if (idx >= 0 && dx < selectW && dy < selectH)
                            lg.cells[idx] = orca::LifeCell();
                    }
                }
            selectW = newW;
            selectH = newH;
            return true;
        }

        // Cmd+Shift+H: mirror horizontal (flip left-right)
        if (code == 'H') {
            pushLifeHistory();
            for (int dy = 0; dy < selectH; dy++)
                for (int dx = 0; dx < selectW / 2; dx++) {
                    int leftIdx = lg.indexAt(cursorX + dx, cursorY + dy);
                    int rightIdx = lg.indexAt(cursorX + selectW - 1 - dx, cursorY + dy);
                    if (leftIdx >= 0 && rightIdx >= 0)
                        std::swap(lg.cells[leftIdx], lg.cells[rightIdx]);
                }
            return true;
        }

        // Cmd+Shift+J: mirror vertical (flip up-down)
        if (code == 'J') {
            pushLifeHistory();
            for (int dy = 0; dy < selectH / 2; dy++)
                for (int dx = 0; dx < selectW; dx++) {
                    int topIdx = lg.indexAt(cursorX + dx, cursorY + dy);
                    int botIdx = lg.indexAt(cursorX + dx, cursorY + selectH - 1 - dy);
                    if (topIdx >= 0 && botIdx >= 0)
                        std::swap(lg.cells[topIdx], lg.cells[botIdx]);
                }
            return true;
        }
    }

    // Life mode: Cmd+Up/Down = shift octave of selection
    if (cmd && processor.engine.lifeMode &&
        (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)) {
        auto& lg = processor.engine.lifeGrid;
        pushLifeHistory();
        int delta = (code == juce::KeyPress::upKey) ? 1 : -1;
        for (int dy = 0; dy < selectH; dy++)
            for (int dx = 0; dx < selectW; dx++) {
                int idx = lg.indexAt(cursorX + dx, cursorY + dy);
                if (idx >= 0 && lg.cells[idx].alive) {
                    int oct = lg.cells[idx].octave + delta;
                    lg.cells[idx].octave = static_cast<uint8_t>(juce::jlimit(0, lg.maxOctave, oct));
                }
            }
        return true;
    }

    // Life mode: Cmd+Left/Right = shift channel of selection
    if (cmd && processor.engine.lifeMode &&
        (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)) {
        auto& lg = processor.engine.lifeGrid;
        pushLifeHistory();
        int delta = (code == juce::KeyPress::rightKey) ? 1 : -1;
        for (int dy = 0; dy < selectH; dy++)
            for (int dx = 0; dx < selectW; dx++) {
                int idx = lg.indexAt(cursorX + dx, cursorY + dy);
                if (idx >= 0 && lg.cells[idx].alive) {
                    int ch = lg.cells[idx].channel + delta;
                    lg.cells[idx].channel = static_cast<uint8_t>(juce::jlimit(0, 15, ch));
                }
            }
        return true;
    }

    // Cmd+S: save as
    if (cmd && code == 'S') {
        saveAs();
        return true;
    }

    // Cmd+L: import modules (load .orca files into inject cache)
    if (cmd && !shift && code == 'L') {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Import modules", currentFile.existsAsFile() ? currentFile.getParentDirectory() : juce::File(),
            "*.orca;*.life");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
            [this](const juce::FileChooser& fc) {
                auto results = fc.getResults();
                for (auto& f : results) {
                    if (!f.existsAsFile()) continue;
                    auto name = f.getFileName();
                    auto content = f.loadFileAsString();
                    if (content.isNotEmpty())
                        injectCache[name] = content;
                }
            });
        return true;
    }

    // Cmd+O: open .orca or .life file
    if (cmd && code == 'O') {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Open file", juce::File(), "*.orca;*.life");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.existsAsFile()) {
                    if (result.hasFileExtension("orca"))
                        loadOrcaFile(result);
                    else if (result.hasFileExtension("life"))
                        loadLifeFile(result);
                }
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
        if (processor.engine.lifeMode) {
            if (shift) redoLife(); else undoLife();
        } else {
            if (shift) redo(); else undo();
        }
        return true;
    }

    // Cmd+C: copy
    if (cmd && code == 'C') {
        auto& engine = processor.engine;
        clipW = selectW;
        clipH = selectH;
        if (engine.lifeMode) {
            for (int dy = 0; dy < clipH; dy++)
                for (int dx = 0; dx < clipW; dx++) {
                    int idx = engine.lifeGrid.indexAt(cursorX + dx, cursorY + dy);
                    lifeClipboard[dx + clipW * dy] = (idx >= 0) ? engine.lifeGrid.cells[idx] : orca::LifeCell();
                }
        } else {
            for (int y = 0; y < clipH; y++)
                for (int x = 0; x < clipW; x++)
                    clipboard[x + clipW * y] = grid.glyphAt(cursorX + x, cursorY + y);
        }
        return true;
    }

    // Cmd+X: cut
    if (cmd && code == 'X') {
        auto& engine = processor.engine;
        clipW = selectW;
        clipH = selectH;
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        if (engine.lifeMode) {
            pushLifeHistory();
            for (int dy = 0; dy < clipH; dy++)
                for (int dx = 0; dx < clipW; dx++) {
                    int idx = engine.lifeGrid.indexAt(cursorX + dx, cursorY + dy);
                    if (idx >= 0) {
                        lifeClipboard[dx + clipW * dy] = engine.lifeGrid.cells[idx];
                        engine.lifeGrid.cells[idx] = orca::LifeCell();
                    }
                }
        } else {
            pushHistory();
            for (int y = 0; y < clipH; y++)
                for (int x = 0; x < clipW; x++) {
                    clipboard[x + clipW * y] = grid.glyphAt(cursorX + x, cursorY + y);
                    grid.write(cursorX + x, cursorY + y, '.');
                }
        }
        return true;
    }

    // Cmd+V: paste
    if (cmd && code == 'V') {
        auto& engine = processor.engine;
        if (clipW > 0 && clipH > 0) {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            if (engine.lifeMode) {
                pushLifeHistory();
                for (int dy = 0; dy < clipH; dy++)
                    for (int dx = 0; dx < clipW; dx++) {
                        int idx = engine.lifeGrid.indexAt(cursorX + dx, cursorY + dy);
                        if (idx >= 0)
                            engine.lifeGrid.cells[idx] = lifeClipboard[dx + clipW * dy];
                    }
            } else {
                pushHistory();
                for (int y = 0; y < clipH; y++)
                    for (int x = 0; x < clipW; x++)
                        grid.write(cursorX + x, cursorY + y, clipboard[x + clipW * y]);
            }
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
        auto& engine = processor.engine;
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        if (engine.lifeMode) {
            pushLifeHistory();
            for (int dy = 0; dy < selectH; dy++)
                for (int dx = 0; dx < selectW; dx++) {
                    int idx = engine.lifeGrid.indexAt(cursorX + dx, cursorY + dy);
                    if (idx >= 0) {
                        engine.lifeGrid.cells[idx] = orca::LifeCell();
                    }
                }
        } else {
            pushHistory();
            for (int y = 0; y < selectH; y++)
                for (int x = 0; x < selectW; x++)
                    grid.write(cursorX + x, cursorY + y, '.');
        }
        return true;
    }

    // Printable character input
    auto textChar = key.getTextCharacter();
    auto& engine = processor.engine;
    if (engine.lifeMode) {
        // Life mode: only accept note letters (A-G, a-g) and '.'
        char ch = static_cast<char>(textChar);
        if (ch == '.') {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            pushLifeHistory();
            int idx = engine.lifeGrid.indexAt(cursorX, cursorY);
            if (idx >= 0)
                engine.lifeGrid.cells[idx] = orca::LifeCell();
            return true;
        }
        // Accept A-G and a-g as notes
        char lower = (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
        if (lower >= 'a' && lower <= 'g') {
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            pushLifeHistory();
            int idx = engine.lifeGrid.indexAt(cursorX, cursorY);
            if (idx >= 0) {
                engine.lifeGrid.cells[idx].note = ch;
                engine.lifeGrid.cells[idx].channel = engine.paintChannel;
                engine.lifeGrid.cells[idx].octave = engine.paintOctave;
                engine.lifeGrid.cells[idx].alive = true;
            }
            return true;
        }
        return false;
    }

    // Normal Orca mode character input
    if (textChar == '.') {
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        pushHistory();
        grid.write(cursorX, cursorY, '.');
        return true;
    }
    if (textChar != 0 && orca::isOperatorGlyph(static_cast<char>(textChar))) {
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
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
    if (historyPos < 0 || historyCount == 0) return;
    auto& snap = history[historyPos];
    {
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        processor.engine.load(snap.w, snap.h, snap.cells, processor.engine.grid.f);
    }
    historyPos--;
}

void GridComponent::redo() {
    if (historyPos + 1 >= historyCount) return;
    historyPos++;
    auto& snap = history[historyPos];
    const juce::SpinLock::ScopedLockType lock(processor.engineLock);
    processor.engine.load(snap.w, snap.h, snap.cells, processor.engine.grid.f);
}

void GridComponent::pushLifeHistory() {
    if (!lifeHistory)
        lifeHistory = new LifeSnapshot[kMaxLifeHistory];

    if (lifeHistoryPos < lifeHistoryCount - 1)
        lifeHistoryCount = lifeHistoryPos + 1;

    if (lifeHistoryCount >= kMaxLifeHistory) {
        for (int i = 0; i < kMaxLifeHistory - 1; i++)
            lifeHistory[i] = lifeHistory[i + 1];
        lifeHistoryCount = kMaxLifeHistory - 1;
    }

    auto& snap = lifeHistory[lifeHistoryCount];
    auto& lg = processor.engine.lifeGrid;
    memcpy(snap.cells, lg.cells, sizeof(orca::LifeCell) * lg.w * lg.h);
    snap.w = lg.w;
    snap.h = lg.h;
    lifeHistoryCount++;
    lifeHistoryPos = lifeHistoryCount - 1;
}

void GridComponent::undoLife() {
    if (!lifeHistory || lifeHistoryPos < 0 || lifeHistoryCount == 0) return;
    auto& snap = lifeHistory[lifeHistoryPos];
    lifeHistoryPos--;
    const juce::SpinLock::ScopedLockType lock(processor.engineLock);
    auto& lg = processor.engine.lifeGrid;
    memcpy(lg.cells, snap.cells, sizeof(orca::LifeCell) * snap.w * snap.h);
}

void GridComponent::redoLife() {
    if (!lifeHistory || lifeHistoryPos + 1 >= lifeHistoryCount) return;
    lifeHistoryPos++;
    auto& snap = lifeHistory[lifeHistoryPos];
    const juce::SpinLock::ScopedLockType lock(processor.engineLock);
    auto& lg = processor.engine.lifeGrid;
    memcpy(lg.cells, snap.cells, sizeof(orca::LifeCell) * snap.w * snap.h);
}

// ── File loading ──

bool GridComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (auto& f : files)
        if (f.endsWith(".orca") || f.endsWith(".life")) return true;
    return false;
}

void GridComponent::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) {
    for (auto& f : files) {
        if (f.endsWith(".life")) {
            loadLifeFile(juce::File(f));
            break;
        }
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

    {
        const juce::SpinLock::ScopedLockType lock(processor.engineLock);
        processor.engine.load(w, h, buf, 0);
    }
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
    bool isLife = processor.engine.lifeMode;
    auto ext = isLife ? "*.life" : "*.orca";
    auto title = isLife ? "Save .life file" : "Save .orca file";
    fileChooser = std::make_unique<juce::FileChooser>(
        title, currentFile.existsAsFile() ? currentFile : juce::File(), ext);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [this, isLife](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result != juce::File()) {
                if (isLife)
                    saveLifeFile(result.withFileExtension("life"));
                else
                    saveOrcaFile(result.withFileExtension("orca"));
            }
        });
}

// ── Save/Load .life files ──

void GridComponent::saveLifeFile(const juce::File& file) {
    auto& lg = processor.engine.lifeGrid;
    juce::String content;

    // Header line: dimensions and settings
    content << "LIFE " << lg.w << " " << lg.h
            << " scale:" << (int)lg.currentScale
            << " root:" << lg.rootNote
            << " rate:" << lg.evolveRate
            << " pulse:" << (lg.pulseMode ? 1 : 0)
            << " decay:" << (lg.decay ? 1 : 0)
            << " minvel:" << (int)lg.minVelocity
            << " minprob:" << lg.minProb
            << " maxnotes:" << lg.maxNotes
            << " seq:" << static_cast<int>(lg.seqMode)
            << " lockoct:" << (lg.lockOctave ? 1 : 0)
            << " chord:" << lg.chordDegreesString()
            << " dedup:" << (lg.dedup ? 1 : 0)
            << " dedupcc:" << lg.dedupCC
            << " minoct:" << lg.minOctave
            << " maxoct:" << lg.maxOctave
            << " rule:" << lg.ruleString
            << "\n";

    // Grid: one row per line
    // Alive cells: note + channel(hex) + octave + flags, e.g. "C03." or "C03!" (locked)
    // Dead cells: "...."
    for (int y = 0; y < lg.h; y++) {
        for (int x = 0; x < lg.w; x++) {
            int idx = lg.indexAt(x, y);
            auto& cell = lg.cells[idx];
            if (cell.alive) {
                char ch = (cell.channel < 10) ? ('0' + cell.channel) : ('a' + cell.channel - 10);
                char flags = cell.locked ? '!' : '.';
                content << cell.note << ch << (char)('0' + cell.octave) << flags;
            } else {
                content << "....";
            }
        }
        content << "\n";
    }

    file.replaceWithText(content);
    currentFile = file;
}

void GridComponent::loadLifeFile(const juce::File& file) {
    auto content = file.loadFileAsString();
    if (content.isEmpty()) return;

    auto lines = juce::StringArray::fromLines(content);
    if (lines.isEmpty() || !lines[0].startsWith("LIFE ")) return;

    // Parse header
    auto header = lines[0];
    auto tokens = juce::StringArray::fromTokens(header, " ", "");
    if (tokens.size() < 3) return;

    int w = tokens[1].getIntValue();
    int h = tokens[2].getIntValue();
    w = juce::jlimit(1, orca::kMaxGridW, w);
    h = juce::jlimit(1, orca::kMaxGridH, h);

    // Parse optional settings from header
    int scale = 0, root = 0, rate = 4, pulse = 0, decayVal = 0, minvel = 40, minprob = 10, maxnotes = 0, seqVal = 0, lockoct = 0, dedupVal = 0, dedupcc = -1, minoct = 0, maxoct = 7;
    juce::String chordStr = "off";
    juce::String ruleStr = "23/3";
    for (int i = 3; i < tokens.size(); i++) {
        if (tokens[i].startsWith("scale:")) scale = tokens[i].substring(6).getIntValue();
        else if (tokens[i].startsWith("root:")) root = tokens[i].substring(5).getIntValue();
        else if (tokens[i].startsWith("rate:")) rate = tokens[i].substring(5).getIntValue();
        else if (tokens[i].startsWith("pulse:")) pulse = tokens[i].substring(6).getIntValue();
        else if (tokens[i].startsWith("decay:")) decayVal = tokens[i].substring(6).getIntValue();
        else if (tokens[i].startsWith("minvel:")) minvel = tokens[i].substring(7).getIntValue();
        else if (tokens[i].startsWith("minprob:")) minprob = tokens[i].substring(8).getIntValue();
        else if (tokens[i].startsWith("maxnotes:")) maxnotes = tokens[i].substring(9).getIntValue();
        else if (tokens[i].startsWith("seq:")) seqVal = tokens[i].substring(4).getIntValue();
        else if (tokens[i].startsWith("lockoct:")) lockoct = tokens[i].substring(8).getIntValue();
        else if (tokens[i].startsWith("chord:")) chordStr = tokens[i].substring(6);
        else if (tokens[i].startsWith("dedup:")) dedupVal = tokens[i].substring(6).getIntValue();
        else if (tokens[i].startsWith("dedupcc:")) dedupcc = tokens[i].substring(8).getIntValue();
        else if (tokens[i].startsWith("minoct:")) minoct = tokens[i].substring(7).getIntValue();
        else if (tokens[i].startsWith("maxoct:")) maxoct = tokens[i].substring(7).getIntValue();
        else if (tokens[i].startsWith("rule:")) ruleStr = tokens[i].substring(5);
    }

    // Enter Life mode with correct grid size (lock protects engine state from audio thread)
    {
    const juce::SpinLock::ScopedLockType lock(processor.engineLock);
    processor.engine.grid.reset(w, h);
    if (!processor.engine.lifeMode)
        processor.engine.enterLifeMode();
    auto& lg = processor.engine.lifeGrid;
    lg.resize(w, h);
    lg.currentScale = static_cast<orca::LifeGrid::ScaleType>(
        juce::jlimit(0, (int)orca::LifeGrid::NumScales - 1, scale));
    lg.rootNote = juce::jlimit(0, 11, root);
    lg.evolveRate = juce::jlimit(1, 32, rate);
    lg.pulseMode = (pulse != 0);
    lg.decay = (decayVal != 0);
    lg.minVelocity = static_cast<uint8_t>(juce::jlimit(1, 127, minvel));
    lg.minProb = juce::jlimit(1, 100, minprob);
    lg.maxNotes = juce::jmax(0, maxnotes);
    lg.seqMode = static_cast<orca::LifeGrid::SeqMode>(juce::jlimit(0, 4, seqVal));
    lg.lockOctave = (lockoct != 0);
    if (chordStr != "off" && chordStr.isNotEmpty())
        lg.setChordDegrees(chordStr.toRawUTF8());
    else
        lg.chordDegreeCount = 0;
    lg.dedup = (dedupVal != 0);
    lg.dedupCC = juce::jlimit(-1, 127, dedupcc);
    lg.minOctave = juce::jlimit(0, 8, minoct);
    lg.maxOctave = juce::jlimit(0, 8, maxoct);
    if (lg.minOctave > lg.maxOctave) lg.minOctave = lg.maxOctave;
    if (ruleStr.containsChar('/'))
        lg.setRuleFromString(ruleStr.toRawUTF8());
    else
        lg.setRulePreset(ruleStr.toRawUTF8());

    // Parse cell rows (lines 1..h)
    for (int y = 0; y < h && y + 1 < lines.size(); y++) {
        auto& line = lines[y + 1];
        // Each cell is 4 chars wide: note + channel(hex) + octave + flags
        for (int x = 0; x < w; x++) {
            int pos = x * 4;
            if (pos + 3 >= line.length()) break;

            char noteChar = static_cast<char>(line[pos]);
            char chChar = static_cast<char>(line[pos + 1]);
            char octChar = static_cast<char>(line[pos + 2]);
            char flagChar = static_cast<char>(line[pos + 3]);

            if (noteChar == '.') continue; // dead cell

            int idx = lg.indexAt(x, y);
            if (idx < 0) continue;

            lg.cells[idx].note = noteChar;
            lg.cells[idx].channel = static_cast<uint8_t>(
                (chChar >= 'a') ? (chChar - 'a' + 10) : (chChar - '0'));
            lg.cells[idx].octave = static_cast<uint8_t>(octChar - '0');
            lg.cells[idx].alive = true;
            lg.cells[idx].locked = (flagChar == '!');
        }
    }

    processor.engine.updateShadow();
    } // engineLock scope
    currentFile = file;

    // Resize window to fit
    if (auto* editor = findParentComponentOfClass<OrcaEditor>()) {
        int newWidth = (int)(w * tileW) + 4;
        int newHeight = (int)(h * tileH) + 24;
        editor->setSize(juce::jmax(400, newWidth), juce::jmax(300, newHeight));
    }
}

// ── Inject block ──

void GridComponent::injectBlock(const juce::String& content, int atX, int atY) {
    if (content.isEmpty()) return;

    auto& engine = processor.engine;
    const juce::SpinLock::ScopedLockType lock(processor.engineLock);

    if (engine.lifeMode) {
        // Life mode: parse .life format
        auto lines = juce::StringArray::fromLines(content);
        if (lines.isEmpty()) return;

        // Check if it's a .life file (has LIFE header)
        if (lines[0].startsWith("LIFE ")) {
            pushLifeHistory();
            auto& lg = engine.lifeGrid;
            auto tokens = juce::StringArray::fromTokens(lines[0], " ", "");
            int fw = (tokens.size() >= 2) ? tokens[1].getIntValue() : 0;

            for (int y = 0; y + 1 < lines.size(); y++) {
                auto& line = lines[y + 1];
                for (int x = 0; x < fw; x++) {
                    int pos = x * 4; // 4 chars per cell: note+channel+octave+flags
                    if (pos + 3 >= line.length()) break;

                    char noteChar = static_cast<char>(line[pos]);
                    if (noteChar == '.') continue; // skip dead cells

                    int idx = lg.indexAt(atX + x, atY + y);
                    if (idx < 0) continue;

                    char chChar = static_cast<char>(line[pos + 1]);
                    char octChar = static_cast<char>(line[pos + 2]);
                    char flags = static_cast<char>(line[pos + 3]);
                    lg.cells[idx].note = noteChar;
                    lg.cells[idx].channel = static_cast<uint8_t>(
                        (chChar >= 'a') ? (chChar - 'a' + 10) : (chChar - '0'));
                    lg.cells[idx].octave = static_cast<uint8_t>(octChar - '0');
                    lg.cells[idx].alive = true;
                    lg.cells[idx].locked = (flags == '!');
                }
            }
        }
    } else {
        // Orca mode: parse as plain text grid
        pushHistory();
        auto& grid = engine.grid;
        auto lines = juce::StringArray::fromLines(content);

        // Remove trailing empty lines
        while (lines.size() > 0 && lines[lines.size() - 1].trimEnd().isEmpty())
            lines.remove(lines.size() - 1);

        for (int y = 0; y < lines.size(); y++) {
            auto& line = lines[y];
            for (int x = 0; x < line.length(); x++) {
                char c = static_cast<char>(line[x]);
                grid.write(atX + x, atY + y, c);
            }
        }
    }

    engine.updateShadow();
}

// ── Commander ──

bool Commander::trigger(OrcaProcessor& processor, GridComponent& editor) {
    auto cmd = parse();
    pushToHistory(query);

    auto& engine = processor.engine;
    auto& grid = engine.grid;
    bool isLife = engine.lifeMode;
    bool handled = true;

    // ── Universal commands ──

    if (cmd.name == "find") {
        // Find text in grid and move cursor to first match
        if (cmd.value.isNotEmpty()) {
            auto target = cmd.value;
            int len = target.length();
            int gw = isLife ? engine.lifeGrid.w : grid.w;
            int gh = isLife ? engine.lifeGrid.h : grid.h;
            for (int y = 0; y < gh; y++) {
                for (int x = 0; x <= gw - len; x++) {
                    bool match = true;
                    for (int i = 0; i < len && match; i++) {
                        char gc;
                        if (isLife) {
                            int idx = engine.lifeGrid.indexAt(x + i, y);
                            gc = (idx >= 0 && engine.lifeGrid.cells[idx].alive)
                                 ? engine.lifeGrid.cells[idx].note : '.';
                        } else {
                            gc = grid.glyphAt(x + i, y);
                        }
                        if (gc != (char)target[i]) match = false;
                    }
                    if (match) {
                        editor.cursorX = x;
                        editor.cursorY = y;
                        editor.selectW = len;
                        editor.selectH = 1;
                        stop();
                        return true;
                    }
                }
            }
        }
    }
    else if (cmd.name == "select") {
        // select:x;y or select:x;y;w;h
        if (cmd.parts.size() >= 2) {
            editor.cursorX = cmd.parts[0].getIntValue();
            editor.cursorY = cmd.parts[1].getIntValue();
            if (cmd.parts.size() >= 4) {
                editor.selectW = juce::jmax(1, cmd.parts[2].getIntValue());
                editor.selectH = juce::jmax(1, cmd.parts[3].getIntValue());
            }
        }
    }
    else if (cmd.name == "write") {
        // write:text or write:text;x;y
        if (cmd.value.isNotEmpty()) {
            juce::String text = cmd.parts[0];
            int wx = (cmd.parts.size() >= 2) ? cmd.parts[1].getIntValue() : editor.cursorX;
            int wy = (cmd.parts.size() >= 3) ? cmd.parts[2].getIntValue() : editor.cursorY;
            if (!isLife) {
                const juce::SpinLock::ScopedLockType lock(processor.engineLock);
                for (int i = 0; i < text.length(); i++)
                    grid.write(wx + i, wy, (char)text[i]);
            }
        }
    }
    else if (cmd.name == "groove") {
        // groove:75;25 or groove:50 (reset)
        if (cmd.parts.size() >= 1) {
            double newGrooves[OrcaProcessor::kMaxGrooveSlots];
            int newLen = juce::jmin(cmd.parts.size(), (int)OrcaProcessor::kMaxGrooveSlots);
            double total = 0;
            for (int i = 0; i < newLen; i++) {
                newGrooves[i] = cmd.parts[i].getDoubleValue() / 50.0;
                total += newGrooves[i];
            }
            // Auto-append balancing ratio
            if (newLen < OrcaProcessor::kMaxGrooveSlots) {
                double avg = (double)(newLen + 1);
                double balance = avg - total;
                if (balance > 0.01) {
                    newGrooves[newLen] = balance;
                    newLen++;
                }
            }
            {
                const juce::SpinLock::ScopedLockType lock(processor.pendingLock);
                for (int i = 0; i < newLen; i++)
                    processor.grooves[i] = newGrooves[i];
                processor.grooveLength = newLen;
                processor.grooveIndex = 0;
            }
        }
    }
    else if (cmd.name == "cc") {
        // cc:N — set CC offset
        if (cmd.value.isNotEmpty())
            engine.io.cc.setOffset(cmd.value.getIntValue());
    }
    else if (cmd.name == "pg") {
        // pg:channel;msb;lsb;program — MIDI program change with optional bank select
        // msb and lsb can be blank for simple program change: pg:0;;;63
        if (cmd.parts.size() >= 4) {
            int ch = juce::jlimit(0, 15, cmd.parts[0].getIntValue());
            int pgm = juce::jlimit(0, 127, cmd.parts[3].getIntValue());
            auto msbStr = cmd.parts[1].trim();
            auto lsbStr = cmd.parts[2].trim();

            // Bank Select MSB (CC #0) — only if specified
            if (msbStr.isNotEmpty()) {
                orca::MidiEvent e;
                e.bytes[0] = static_cast<uint8_t>(0xB0 | ch);
                e.bytes[1] = 0x00; // CC #0 = Bank Select MSB
                e.bytes[2] = static_cast<uint8_t>(juce::jlimit(0, 127, msbStr.getIntValue()));
                e.numBytes = 3;
                processor.pushLifeEvent(e);
            }

            // Bank Select LSB (CC #32) — only if specified
            if (lsbStr.isNotEmpty()) {
                orca::MidiEvent e;
                e.bytes[0] = static_cast<uint8_t>(0xB0 | ch);
                e.bytes[1] = 0x20; // CC #32 = Bank Select LSB
                e.bytes[2] = static_cast<uint8_t>(juce::jlimit(0, 127, lsbStr.getIntValue()));
                e.numBytes = 3;
                processor.pushLifeEvent(e);
            }

            // Program Change
            orca::MidiEvent e;
            e.bytes[0] = static_cast<uint8_t>(0xC0 | ch);
            e.bytes[1] = static_cast<uint8_t>(pgm);
            e.numBytes = 2;
            processor.pushLifeEvent(e);
        }
    }
    else if (cmd.name == "udp") {
        // udp:port — set UDP output port
        if (cmd.value.isNotEmpty()) {
            int port = cmd.value.getIntValue();
            if (port >= 1000 && port <= 65535) {
                {
                    const juce::SpinLock::ScopedLockType lock(processor.pendingLock);
                    processor.udpOutputPort = port;
                }
                processor.udpSocket.reset();
            }
        }
    }
    else if (cmd.name == "osc") {
        // osc:port — set OSC output port (supports presets)
        if (cmd.value.isNotEmpty()) {
            auto val = cmd.value.toLowerCase().trim();
            int port = 0;
            if (val == "tidal" || val == "tidalcycles") port = 6010;
            else if (val == "sonic" || val == "sonicpi") port = 4559;
            else if (val == "sc" || val == "supercollider") port = 57120;
            else if (val == "norns") port = 10111;
            else port = val.getIntValue();

            if (port >= 1000 && port <= 65535) {
                {
                    const juce::SpinLock::ScopedLockType lock(processor.pendingLock);
                    processor.oscOutputPort = port;
                }
                processor.oscSender.reset();
            }
        }
    }
    else if (cmd.name == "ip") {
        // ip:address — set target IP for UDP/OSC
        if (cmd.value.isNotEmpty()) {
            {
                const juce::SpinLock::ScopedLockType lock(processor.pendingLock);
                processor.udpTargetIP = cmd.value.trim();
            }
            processor.udpSocket.reset();
            processor.oscSender.reset();
        }
    }
    else if (cmd.name == "copy") {
        // Trigger copy via keyboard simulation
        editor.keyPressed(juce::KeyPress('C', juce::ModifierKeys::commandModifier, 0));
    }
    else if (cmd.name == "paste") {
        editor.keyPressed(juce::KeyPress('V', juce::ModifierKeys::commandModifier, 0));
    }
    else if (cmd.name == "erase") {
        editor.keyPressed(juce::KeyPress(juce::KeyPress::backspaceKey));
    }
    else if (cmd.name == "time") {
        // Write current time to grid at cursor
        if (!isLife) {
            auto now = juce::Time::getCurrentTime();
            auto timeStr = now.formatted("%H%M%S");
            const juce::SpinLock::ScopedLockType lock(processor.engineLock);
            for (int i = 0; i < timeStr.length(); i++)
                grid.write(editor.cursorX + i, editor.cursorY, (char)timeStr[i]);
        }
    }
    else if (cmd.name == "color") {
        // color:f_low;f_med;f_high (hex RGB values)
        // For now, minimal implementation
        if (cmd.parts.size() >= 3) {
            auto parseHex = [](const juce::String& s) -> juce::Colour {
                auto hex = s.trim();
                if (hex.length() == 3) {
                    // Expand shorthand: f00 → ff0000
                    juce::String full;
                    full << hex[0] << hex[0] << hex[1] << hex[1] << hex[2] << hex[2];
                    return juce::Colour((uint32_t)full.getHexValue32() | 0xff000000u);
                }
                return juce::Colour((uint32_t)hex.getHexValue32() | 0xff000000u);
            };
            editor.bLow  = parseHex(cmd.parts[0]);
            editor.bMed  = parseHex(cmd.parts[1]);
            editor.bHigh = parseHex(cmd.parts[2]);
        }
    }

    else if (cmd.name == "inject") {
        // inject:name or inject:name;x;y — inject cached block at position
        if (cmd.value.isNotEmpty()) {
            juce::String name = cmd.parts[0];
            int ix = (cmd.parts.size() >= 2) ? cmd.parts[1].getIntValue() : editor.cursorX;
            int iy = (cmd.parts.size() >= 3) ? cmd.parts[2].getIntValue() : editor.cursorY;

            // Auto-append extension if needed
            if (!name.endsWithIgnoreCase(".orca") && !name.endsWithIgnoreCase(".life"))
                name += isLife ? ".life" : ".orca";

            auto it = editor.injectCache.find(name);
            if (it != editor.injectCache.end()) {
                editor.injectBlock(it->second, ix, iy);
                editor.selectW = 1;
                editor.selectH = 1;
            }
        }
    }

    // ── Life mode commands ──

    else if (isLife && cmd.name == "scale") {
        // scale:major, scale:minor, scale:pentatonic, etc.
        auto val = cmd.value.toLowerCase().trim();
        for (int i = 0; i < orca::LifeGrid::NumScales; i++) {
            if (val == juce::String(orca::LifeGrid::scaleName(
                    static_cast<orca::LifeGrid::ScaleType>(i)))) {
                engine.lifeGrid.currentScale = static_cast<orca::LifeGrid::ScaleType>(i);
                break;
            }
        }
    }
    else if (isLife && cmd.name == "root") {
        // root:C, root:C#, root:D, etc.
        auto val = cmd.value.trim();
        for (int i = 0; i < 12; i++) {
            if (val.equalsIgnoreCase(orca::LifeGrid::rootNoteName(i))) {
                engine.lifeGrid.rootNote = i;
                break;
            }
        }
    }
    else if (isLife && cmd.name == "rate") {
        int r = cmd.value.getIntValue();
        if (r >= 1 && r <= 32)
            engine.lifeGrid.evolveRate = r;
    }
    else if (isLife && (cmd.name == "pulse" || cmd.name == "hold")) {
        engine.lifeGrid.pulseMode = (cmd.name == "pulse");
    }
    else if (isLife && cmd.name == "decay") {
        auto val = cmd.value.toLowerCase().trim();
        if (val == "on" || val == "1") engine.lifeGrid.decay = true;
        else if (val == "off" || val == "0") engine.lifeGrid.decay = false;
        else engine.lifeGrid.decay = !engine.lifeGrid.decay;
    }
    else if (isLife && cmd.name == "minvel") {
        if (cmd.value.isNotEmpty())
            engine.lifeGrid.minVelocity = static_cast<uint8_t>(
                juce::jlimit(1, 127, cmd.value.getIntValue()));
    }
    else if (isLife && cmd.name == "minprob") {
        if (cmd.value.isNotEmpty())
            engine.lifeGrid.minProb = juce::jlimit(1, 100, cmd.value.getIntValue());
    }
    else if (isLife && cmd.name == "maxnotes") {
        if (cmd.value.isNotEmpty())
            engine.lifeGrid.maxNotes = juce::jmax(0, cmd.value.getIntValue());
    }
    else if (isLife && cmd.name == "lockoct") {
        auto val = cmd.value.toLowerCase().trim();
        if (val == "on" || val == "1") engine.lifeGrid.lockOctave = true;
        else if (val == "off" || val == "0") engine.lifeGrid.lockOctave = false;
        else engine.lifeGrid.lockOctave = !engine.lifeGrid.lockOctave;
    }
    else if (isLife && cmd.name == "chord") {
        auto val = cmd.value.trim();
        if (val == "off" || val == "0")
            engine.lifeGrid.chordDegreeCount = 0;
        else if (val.isNotEmpty())
            engine.lifeGrid.setChordDegrees(val.toRawUTF8());
        else {
            // Toggle: if on, turn off; if off, set default 135
            if (engine.lifeGrid.chordDegreeCount > 0)
                engine.lifeGrid.chordDegreeCount = 0;
            else
                engine.lifeGrid.setChordDegrees("135");
        }
    }
    else if (isLife && cmd.name == "dedup") {
        auto val = cmd.value.toLowerCase().trim();
        if (val == "on" || val == "1") engine.lifeGrid.dedup = true;
        else if (val == "off" || val == "0") engine.lifeGrid.dedup = false;
        else engine.lifeGrid.dedup = !engine.lifeGrid.dedup;
    }
    else if (isLife && cmd.name == "dedupcc") {
        if (cmd.value.isNotEmpty())
            engine.lifeGrid.dedupCC = juce::jlimit(-1, 127, cmd.value.getIntValue());
    }
    else if (isLife && cmd.name == "seq") {
        auto val = cmd.value.toLowerCase().trim();
        if (val == "off" || val == "0") engine.lifeGrid.seqMode = orca::LifeGrid::SeqOff;
        else if (val == "forward" || val == "fwd" || val == "on" || val == "1") engine.lifeGrid.seqMode = orca::LifeGrid::SeqForward;
        else if (val == "reverse" || val == "rev") engine.lifeGrid.seqMode = orca::LifeGrid::SeqReverse;
        else if (val == "mirror" || val == "mir") engine.lifeGrid.seqMode = orca::LifeGrid::SeqMirror;
        else if (val == "random" || val == "rand" || val == "rnd") engine.lifeGrid.seqMode = orca::LifeGrid::SeqRandom;
        else {
            // Toggle: off → forward → off
            engine.lifeGrid.seqMode = (engine.lifeGrid.seqMode == orca::LifeGrid::SeqOff)
                ? orca::LifeGrid::SeqForward : orca::LifeGrid::SeqOff;
        }
    }
    else if (isLife && cmd.name == "rule") {
        auto val = cmd.value.toLowerCase().trim();
        if (val.containsChar('/')) {
            // S/B notation: e.g. "23/3", "23/36", "34/34"
            engine.lifeGrid.setRuleFromString(val.toRawUTF8());
        } else {
            // Named preset
            engine.lifeGrid.setRulePreset(val.toRawUTF8());
        }
    }
    else if (isLife && cmd.name == "channel") {
        if (cmd.value.isNotEmpty())
            engine.paintChannel = static_cast<uint8_t>(
                juce::jlimit(0, 15, cmd.value.getIntValue()));
    }
    else if (isLife && cmd.name == "octave") {
        if (cmd.value.isNotEmpty())
            engine.paintOctave = static_cast<uint8_t>(
                juce::jlimit(0, engine.lifeGrid.maxOctave, cmd.value.getIntValue()));
    }
    else if (isLife && cmd.name == "maxoct") {
        if (cmd.value.isNotEmpty()) {
            engine.lifeGrid.maxOctave = juce::jlimit(0, 8, cmd.value.getIntValue());
            if (engine.lifeGrid.minOctave > engine.lifeGrid.maxOctave)
                engine.lifeGrid.minOctave = engine.lifeGrid.maxOctave;
        }
    }
    else if (isLife && cmd.name == "minoct") {
        if (cmd.value.isNotEmpty()) {
            engine.lifeGrid.minOctave = juce::jlimit(0, 8, cmd.value.getIntValue());
            if (engine.lifeGrid.maxOctave < engine.lifeGrid.minOctave)
                engine.lifeGrid.maxOctave = engine.lifeGrid.minOctave;
        }
    }
    else if (isLife && cmd.name == "reset") {
        orca::MidiEvent events[orca::kMaxEvents];
        int eventCount = engine.lifeGrid.resetToInitial(events, orca::kMaxEvents);
        for (int i = 0; i < eventCount; i++)
            processor.pushLifeEvent(events[i]);
    }
    else {
        handled = false;
    }

    stop();
    return handled;
}

void Commander::preview(OrcaProcessor& processor, GridComponent& editor) {
    auto cmd = parse();
    findHighlights.clear();
    hasFindPreview = false;

    auto& engine = processor.engine;
    bool isLife = engine.lifeMode;

    if (cmd.name == "find" && cmd.value.isNotEmpty()) {
        hasFindPreview = true;
        auto target = cmd.value;
        int len = target.length();
        int gw = isLife ? engine.lifeGrid.w : engine.grid.w;
        int gh = isLife ? engine.lifeGrid.h : engine.grid.h;
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x <= gw - len; x++) {
                bool match = true;
                for (int i = 0; i < len && match; i++) {
                    char gc;
                    if (isLife) {
                        int idx = engine.lifeGrid.indexAt(x + i, y);
                        gc = (idx >= 0 && engine.lifeGrid.cells[idx].alive)
                             ? engine.lifeGrid.cells[idx].note : '.';
                    } else {
                        gc = engine.grid.glyphAt(x + i, y);
                    }
                    if (gc != (char)target[i]) match = false;
                }
                if (match) {
                    for (int i = 0; i < len; i++)
                        findHighlights.add(x + i + gw * y);
                }
            }
        }
    }

    // Inject preview: show selection scaled to block dimensions
    if (cmd.name == "inject" && cmd.value.isNotEmpty()) {
        juce::String name = cmd.parts[0];
        if (!name.endsWithIgnoreCase(".orca") && !name.endsWithIgnoreCase(".life"))
            name += isLife ? ".life" : ".orca";

        auto it = editor.injectCache.find(name);
        if (it != editor.injectCache.end()) {
            auto lines = juce::StringArray::fromLines(it->second);
            while (lines.size() > 0 && lines[lines.size() - 1].trimEnd().isEmpty())
                lines.remove(lines.size() - 1);

            if (lines.size() > 0) {
                int bh = lines.size();
                int bw = 0;

                // For .life files, parse header for dimensions
                if (lines[0].startsWith("LIFE ")) {
                    auto tokens = juce::StringArray::fromTokens(lines[0], " ", "");
                    bw = (tokens.size() >= 2) ? tokens[1].getIntValue() : 0;
                    bh = (tokens.size() >= 3) ? tokens[2].getIntValue() : lines.size() - 1;
                } else {
                    for (auto& line : lines)
                        bw = juce::jmax(bw, line.length());
                }

                if (cmd.parts.size() >= 2) editor.cursorX = cmd.parts[1].getIntValue();
                if (cmd.parts.size() >= 3) editor.cursorY = cmd.parts[2].getIntValue();
                editor.selectW = juce::jmax(1, bw);
                editor.selectH = juce::jmax(1, bh);
            }
        }
    }
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
