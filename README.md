# ORCΛ

<img src="https://raw.githubusercontent.com/hundredrabbits/100r.co/master/media/content/characters/orca.hello.png" width="300" alt="Orca character" />

Orca is an [esoteric programming language](https://en.wikipedia.org/wiki/Esoteric_programming_language) for quickly building procedural sequencers. Each letter is an operation: uppercase operators run every frame, while lowercase operators run only when they receive a bang (`*`).

Orca is **not a synthesizer**. It is a live-coding environment that sends MIDI, OSC, and UDP to instruments and audiovisual software such as Ableton Live, Renoise, VCV Rack, and SuperCollider.

This repository is a fork of the original project by [Hundred Rabbits](https://100r.co/). It retains the browser and Electron versions and adds a native macOS AU/VST3/Standalone plugin, a musical Game of Life mode, new operators, groove, and other workflow improvements.

## Contents

- [Choose a version](#choose-a-version)
- [Build and run](#build-and-run)
- [Plugin usage](#plugin-usage)
- [Operators](#operators)
- [MIDI, UDP, and OSC](#midi-udp-and-osc)
- [Desktop Commander and project mode](#desktop-commander-and-project-mode)
- [Groove](#groove)
- [Plugin Commander](#plugin-commander)
- [Life mode](#life-mode)
- [Reference tables](#reference-tables)

## Choose a version

| Version | Best for | Notes |
|---------|----------|-------|
| Browser/PWA | Trying classic Orca without installing it | Uses Web MIDI; browser builds do not provide UDP or OSC |
| Electron desktop | The original standalone Orca workflow | Cross-platform; supports MIDI, UDP, and OSC |
| JUCE plugin | Running Orca inside a DAW or as a native standalone app | This fork's macOS AU/VST3/Standalone build; includes Life mode and DAW automation |

Upstream builds and ports remain available:

- [Download upstream desktop builds](https://hundredrabbits.itch.io/orca) for Linux, Windows, and macOS.
- Use [the upstream browser version](https://hundredrabbits.github.io/Orca/) with Web MIDI.
- Use [in a terminal](https://git.sr.ht/~rabbits/orca), written in C.
- Use [on small computers](https://git.sr.ht/~rabbits/orca-toy), written in assembly.
- Use [on the Monome Norns](https://llllllll.co/t/orca/22492), written in Lua.

For help with the Orca language, visit the [chatroom](https://discord.gg/F7W98pXKd7), [mailing list](https://lists.sr.ht/~rabbits/orca), [forum](https://llllllll.co/t/orca-live-coding-tool/17689), or [introductory tutorial](https://www.youtube.com/watch?v=ktcWOLeWP-g).

If you enjoy this fork's additions, you can support the maintainer:

[![Support on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/X7X31XSQK3)

## Build and run

Clone the fork once, including the JUCE submodule used by the plugin:

```sh
git clone --recurse-submodules https://github.com/mcartagenah/Orca.git
cd Orca
```

If the repository was cloned without submodules, initialize JUCE with:

```sh
git submodule update --init --recursive
```

### Browser/PWA

The root `index.html` loads the same JavaScript sources as the Electron app. Serve the repository root rather than opening the file directly:

```sh
python3 -m http.server 8000
```

Then open `http://localhost:8000`. Allow MIDI access when prompted. UDP and OSC are unavailable in the browser build.

### Desktop (Electron)

The desktop app uses Node 16 (`desktop/.nvmrc`):

```sh
cd desktop
npm ci
npm start
```

### Plugin (AU/VST3/Standalone)

The plugin requires macOS, CMake 3.22 or newer, Xcode Command Line Tools, and the JUCE submodule. A non-installing Release build is:

```sh
cmake -S plugin -B plugin/build -DCMAKE_BUILD_TYPE=Release
cmake --build plugin/build --config Release --target OrcaPlugin_All -j8
```

Artifacts are written beneath `plugin/build/OrcaPlugin_artefacts/Release/`.

To build and install the AU and VST3 into the current user's plugin folders:

```sh
make -C plugin configure
make -C plugin all
```

`make all` replaces existing Orca bundles in `~/Library/Audio/Plug-Ins/Components/` and `~/Library/Audio/Plug-Ins/VST3/`. Build with CMake first if you do not want to modify installed plugins. The standalone app can be run from `plugin/build/OrcaPlugin_artefacts/Release/Standalone/Orca.app`.

## Plugin usage

The plugin is a silent Music Device: it generates MIDI but does not synthesize audio. Load it before an instrument in a MIDI-capable track, or route the system-wide virtual MIDI source named `Orca` to another application.

### General keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Cmd+O` | Open an `.orca` or `.life` file |
| `Cmd+S` | Save as `.orca` or `.life`, according to the current mode |
| `Cmd+L` | Import `.orca`/`.life` modules into the injection cache |
| `Cmd+G` | Toggle Life mode |
| `Cmd+K` | Open the Plugin Commander |
| `Cmd+F` | Open command prompt with `find:` |
| `Cmd+=` | Zoom in |
| `Cmd+-` | Zoom out |
| `Cmd+Z` | Undo |
| `Cmd+Shift+Z` | Redo |
| Arrow keys | Move cursor |
| `Shift+Arrows` | Expand selection |
| `Cmd+C/V/X` | Copy/Paste/Cut |
| `Backspace` | Delete selected cells |
| `Space` (Standalone only) | Toggle the internal transport |

Life mode adds [mode-specific shortcuts](#life-mode-keyboard-shortcuts).

### DAW-automatable parameters

The following parameters are exposed to the host:

| Group | Parameter | Range | Default | Description |
|-------|-----------|-------|---------|-------------|
| Shuffle | Shuffle | 0-200% | 100% | Swing amount: 0% inverse, 100% straight, 200% maximum swing |
| Life: Timing | Evolve Rate | 1-32 | 4 | Frames between evolutions |
| Life: Timing | Seq Mode | Off/Forward/Reverse/Mirror/Random/Euclid | Off | Sequencer scan mode |
| Life: Timing | Euclid Pulses | 1-32 | 3 | Active phases in Euclid sequencer mode |
| Life: Timing | Seq Horizontal | On/Off | Off | Scan columns left-to-right instead of rows top-to-bottom |
| Life: Timing | Pulse Mode | On/Off | On | Pulse (retrigger) vs Hold (sustain) |
| Life: Timing | Conductor Mode | On/Off | Off | Evolve only on Enter or incoming MIDI Note On |
| Life: Pitch | Scale | 15 types | Chromatic | Active musical scale |
| Life: Pitch | Root Note | C-B | C | Scale root note |
| Life: Pitch | Min Octave | 0-8 | 0 | Lower octave limit |
| Life: Pitch | Max Octave | 0-8 | 7 | Upper octave limit |
| Life: Pitch | Lock Octave | On/Off | Off | Prevent octave drift on birth |
| Life: Pitch | Chord Filter | Off/135/1357/125/145/1356/12356/1234567 | Off | Snap to chord degrees |
| Life: Dynamics | Decay | On/Off | Off | Age-based velocity + probability drop |
| Life: Dynamics | Min Velocity | 1-127 | 40 | Velocity floor at max age |
| Life: Dynamics | Min Probability | 1-100% | 10 | Probability floor at max age |
| Life: Dynamics | Max Notes | 0-32 | 0 | Per-channel per-step note cap (0=unlimited) |
| Life: Processing | Dedup | On/Off | Off | Merge identical {channel, pitch} notes |
| Life: Processing | Dedup CC | -1 to 127 | -1 | CC for dedup count modulation (-1=off) |
| Life: Processing | CA Rule | Presets | Life | Cellular automata rule set |
| Life: Pitch | Microtune | On/Off | Off | Pitch bend based on neighbor count |
| Life: Pitch | Microtune Amount | 0-100 | 50 | Pitch bend intensity |

Parameter-backed Commander commands and keyboard shortcuts notify the host, so they can be recorded in DAW automation. Editor-only state, such as the current cursor and paint channel, is not automatable.

### MIDI output

The plugin sends MIDI in two ways:

1. **Host MIDI bus**: Routed through the DAW's MIDI output (for instrument chains).
2. **Virtual MIDI port "Orca"**: Available system-wide to any application listening for MIDI input.

<img src="resources/preview.jpg" width="600" alt="Orca grid interface" />

## Operators

In the Electron app, `CmdOrCtrl+G` toggles the operator guide. In the plugin, the status bar identifies the operator and port beneath the cursor; `Cmd+G` is reserved for Life mode.

- `A` **add**(*a* b): Outputs sum of inputs.
- `B` **subtract**(*a* b): Outputs difference of inputs.
- `C` **clock**(*rate* mod): Outputs modulo of frame.
- `D` **delay**(*rate* mod): Bangs on modulo of frame.
- `E` **east**: Moves eastward, or bangs.
- `F` **if**(*a* b): Bangs if inputs are equal.
- `G` **generator**(*x* *y* *len*): Writes operands with offset.
- `H` **halt**: Halts southward operand.
- `I` **increment**(*step* mod): Increments southward operand.
- `J` **jumper**(*val*): Outputs northward operand.
- `K` **konkat**(*len*): Reads multiple variables.
- `L` **less**(*a* *b*): Outputs smallest of inputs.
- `M` **multiply**(*a* b): Outputs product of inputs.
- `N` **north**: Moves Northward, or bangs.
- `O` **read**(*x* *y* read): Reads operand with offset.
- `P` **push**(*len* *key* val): Writes eastward operand.
- `Q` **query**(*x* *y* *len*): Reads operands with offset.
- `R` **random**(*min* max): Outputs random value.
- `S` **south**: Moves southward, or bangs.
- `T` **track**(*key* *len* val): Reads eastward operand.
- `U` **uclid**(*step* max): Bangs on Euclidean rhythm.
- `V` **variable**(*write* read): Reads and writes variable.
- `W` **west**: Moves westward, or bangs.
- `X` **write**(*x* *y* val): Writes operand with offset.
- `Y` **jymper**(*val*): Outputs westward operand.
- `Z` **lerp**(*rate* target): Transitions operand to input.
- `*` **bang**: Bangs neighboring operands.
- `#` **comment**: Halts a line.

### IO

- `:` **midi**(channel octave note velocity length): Sends a MIDI note.
- `%` **mono**(channel octave note velocity length): Sends monophonic MIDI note.
- `!` **cc**(channel knob value): Sends MIDI control change.
- `?` **pb**(channel value): Sends MIDI pitch bend.
- `;` **udp**: Sends UDP message.
- `=` **osc**(*path*): Sends OSC message.
- `$` **self**: Sends a command to the active platform's Commander.

### Generative

- `~` **probability**(chance): Bangs with probability (0=never, z=always).
- `^` **scale**(*note* scale): Quantizes value to musical scale (0=chromatic, 1=major, 2=minor, 3=pentatonic, 4=blues, 5=dorian, 6=mixolydian, 7=harmonic minor).
- `{` **buffer**(*len* val): Shift register — on bang, shifts south row right and inserts value at position 0.
- `}` **freeze**(*val*): Sample and hold — on bang, captures input; otherwise holds previous value.
- `|` **gate**(*threshold* val): Passes value southward if >= threshold, otherwise outputs `.`.
- `&` **arp**(*speed* *pattern* len notes...): Arpeggiator — cycles through eastward notes (0=up, 1=down, 2=updown, 3=random).
- `@` **markov**(len states...): State machine — on bang, uses current state as index into eastward cells to determine next state.
- `[` **strum**(len rate): On bang, outputs sequential `*` bangs southward — one per `rate` frames. Len controls how many bangs, rate controls frames between each bang. Uses 2 state cells (position, rate countdown) then len output cells below.
- `]` **chord**(root type): Outputs chord notes southward. Root is a note letter (uppercase=natural, lowercase=sharp, e.g. C=C, c=C#). Types: 0=major, 1=minor, 2=dim, 3=aug, 4=sus2, 5=sus4, 6=maj7, 7=min7, 8=dom7.
- `>` **humanize**(max): On bang, delays the output bang by a random 0-max frames. State stored in south cell, bang output one cell below.
- `<` **ratchet**(subdivisions period): On bang, outputs N evenly-spaced bangs over the given period of frames. State stored in south cell, bang output one cell below.
- `\` **swing**(delay): Alternates between immediate and delayed bangs. Odd bangs pass through instantly, even bangs are delayed by N frames. Uses 3 south cells (toggle, countdown, output).
- `/` **deflect**: Redirects adjacent movers (N/S/E/W) to point away. Any mover next to `/` gets rewritten to face outward — e.g. `S` to the east of `/` becomes `E`, `S` above `/` becomes `N`. Works as a passive obstacle that redirects instead of destroying. Preserves uppercase/lowercase.

## MIDI, UDP, and OSC

### MIDI notes

The [MIDI](https://en.wikipedia.org/wiki/MIDI) operator `:` takes five inputs: channel, octave, note, velocity, and length.

For example, `:25C` sends C5 on MIDI channel 3; `:04c` sends C-sharp 4 on MIDI channel 1. Orca channel values are zero-based, while many MIDI interfaces display channels as 1-16. Velocity is optional and ranges from `0` (0/127) to `g` (127/127). Length is the number of frames for which the note remains active. See [midi.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_midi.orca) for an example.

### Monophonic MIDI

The [mono](https://en.wikipedia.org/wiki/Monophony) operator `%` takes the same five inputs as `:`.

Each new note stops the previous note on that channel if their lengths overlap. This makes `%` suitable for monophonic synthesizers and patches that should not receive chords.

### MIDI CC

The MIDI CC operator `!` takes channel, knob, and value inputs.

Values are scaled from Orca's base-36 range to MIDI's 0-127 range. For example, `!008` sends value 28 to the first channel's offset controller. The default CC offset is 64; use `cc:0` to start at CC 0.

### MIDI pitch bend

The pitch-bend operator `?` takes channel, LSB, and MSB inputs.

Both value bytes are scaled to 0-127. For example, `?008` sends LSB 0 and MSB 28 on the first MIDI channel.

### Bank select and program change

Program changes are sent through the Commander rather than an operator:

```text
pg:channel;msb;lsb;program
```

Channel is 0-15; MSB, LSB, and program are 0-127. Many instruments display program 0 as patch 1. MSB and LSB may also be labeled bank and sub-bank. Leave either bank field blank to omit it; for example, `pg:0;;;63` sends program 63 without changing the bank.

### UDP

The UDP operator `;` consumes consecutive cells to its east and sends them as a string when banged. For example, `;hello` sends `hello` to the configured output. The default output is `127.0.0.1:49161`; use `udp:7777` to change the port and `ip:127.0.0.12` to change the target. UDP is available in the Electron and plugin builds, not the browser build.

You can use [`resources/listener.js`](resources/listener.js) to test UDP messages. See it in action with [udp.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_udp.orca).

### OSC

The OSC operator `=` consumes consecutive cells to its east. The first character becomes the OSC path; the remaining characters are sent as integers using the [base-36 table](#base-36-table). The default output is `127.0.0.1:49162`; use `osc:7777` and `ip:127.0.0.12` to change it. OSC is available in the Electron and plugin builds, not the browser build.

For example, `=1abc` sends 10, 11, and 12 to `/1`; `=a123` sends 1, 2, and 3 to `/a`. Use [`resources/listener.js`](resources/listener.js) to test OSC, see [osc.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_osc.orca), or follow the [Sonic Pi example](resources/TUTORIAL.md#sonicpi).

<img src="resources/preview.hardware.jpg" width="600" alt="Orca controlling external hardware" />

## Desktop Commander and project mode

This section describes the browser/Electron Commander. The plugin has a related but separate [Plugin Commander](#plugin-commander).

In Electron, Orca can receive commands over UDP port `49160`. Press `CmdOrCtrl+K` to open the local Commander prompt, which can control transport, inject patterns, and change settings.

### Project Mode

Project mode lets you inject saved Orca files into the active grid. Press `CmdOrCtrl+L` to load multiple `.orca` files, then press `CmdOrCtrl+B` and enter a loaded file's name and optional coordinates.

### Default Ports

| UDP Input | OSC Input | UDP Output | OSC Output |
|-----------|-----------|------------|------------|
| 49160 | — | 49161 | 49162 |

### Desktop Commander commands

Desktop commands accept their first two characters as shorthand; for example, `write` can be entered as `wr`. The implementation in [`commander.js`](desktop/sources/scripts/commander.js) is the authoritative list.

- `play`: Play the program.
- `stop`: Stop the program.
- `run`: Run one frame.
- `bpm:140`: Set the tempo to 140 BPM.
- `apm:160`: Animate the tempo toward 160 BPM.
- `frame:0`: Set the frame counter to 0.
- `skip:2`: Advance the frame counter by 2.
- `rewind:2`: Move the frame counter back by 2.
- `color:f00;0f0;00f`: Change the interface colors.
- `find:aV`: Move the cursor to the first `aV` match.
- `select:3;4;5;6`: Move to `(3,4)` and optionally select a `5x6` block.
- `inject:pattern;12;34`: Inject `pattern.orca`, optionally at `(12,34)`.
- `write:H;12;34`: Write `H`, optionally at `(12,34)`.
- `time`: Write the elapsed minutes and seconds since frame 0.
- `midi:1;2`: Select MIDI output device 1 and input device 2.
- `udp:1234`: Set the UDP output port.
- `osc:1234`: Set the OSC output port.
- `ip:127.0.0.12`: Set the UDP/OSC target address.
- `groove:75;25`: Set groove ratios; see [Groove](#groove).

## Groove

Orca supports **groove/shuffle** to create swing and humanized timing. Groove works by varying the duration of each tick in a repeating cycle.

### Ratios

Groove ratios use **50 as the baseline** (1.0x normal speed):

| Value | Ratio | Effect |
|-------|-------|--------|
| 25 | 0.5x | Half duration (faster tick) |
| 50 | 1.0x | Normal timing |
| 75 | 1.5x | 1.5x duration (slower tick) |
| 99 | ~2.0x | Near-double duration |

### Examples

- `groove:75;25` — Classic swing. Long-short-balanced cycle.
- `groove:25;75` — Inverse swing. Short-long-balanced cycle.
- `groove:50` — Straight timing (reset to normal).

A **closing ratio** is automatically appended to balance the cycle, so the average tempo stays the same. For example, `groove:75;25` becomes `[75, 25, 50]` internally — a 3-step cycle.

### Desktop (Electron)

Use the command prompt (`CmdOrCtrl+K`) and type `groove:75;25`.

### Plugin (AU/VST3)

- Enter ratios through the Plugin Commander, for example `groove:75;25`.
- Automate the Shuffle parameter from 0-200%. It maps to a balanced three-step groove: 0% is maximum inverse swing, 100% is straight, and 200% is maximum swing.

The current groove is displayed in the status bar as `groove:75;25;50`.

## Plugin Commander

Press `Cmd+K` to open the plugin's command prompt, or `Cmd+F` to open it with `find:` pre-filled. Press Enter to execute, Escape to cancel, and Up/Down to navigate command history.

The tables list each supported shorthand. Universal commands work in classic and Life modes; Life commands are accepted only while Life mode is active.

**Universal commands (both modes):**

| Command | Short | Description |
|---------|-------|-------------|
| `find:text` | `fi` | Find text in grid and move cursor to first match |
| `select:x;y;w;h` | `se` | Move cursor and optionally set selection size |
| `groove:75;25` | `gr` | Set groove ratios |
| `cc:0` | `cc` | Set MIDI CC offset |
| `pg:ch;msb;lsb;pgm` | `pg` | MIDI program change with optional bank select |
| `copy` | `co` | Copy selection |
| `paste` | `pa` | Paste clipboard |
| `erase` | `er` | Erase selection |
| `color:f00;0f0;00f` | `cl` | Set theme colors (bLow;bMed;bHigh as hex RGB) |
| `inject:name` | `in` | Inject cached module at cursor (or `inject:name;x;y`) |

**Classic-mode commands:**

| Command | Short | Description |
|---------|-------|-------------|
| `write:text;x;y` | `wr` | Write text at position (or cursor if x;y is omitted) |
| `time` | `ti` | Write the current time at the cursor |
| `clean` | -- | Remove all movers (N/S/E/W) and bangs (*) from grid (skips halted) |
| `autoclean` | -- | Toggle auto-clean on transport stop (`autoclean:on`/`off`) |

**Network commands:**

| Command | Short | Description |
|---------|-------|-------------|
| `udp:7777` | `ud` | Set UDP output port |
| `osc:7777` | `os` | Set OSC output port |
| `ip:127.0.0.1` | `ip` | Set target IP address |
| `osc:tidal` | -- | OSC preset for TidalCycles |
| `osc:sonicpi` | -- | OSC preset for Sonic Pi |
| `osc:supercollider` | -- | OSC preset for SuperCollider |
| `osc:norns` | -- | OSC preset for Norns |

**Life mode commands:**

| Command | Short | Description |
|---------|-------|-------------|
| `scale:minor` | `sc` | Set scale by name |
| `root:D` | `ro` | Set root note |
| `rate:4` | `ra` | Set evolve rate (1-32) |
| `pulse` / `hold` | `pu`/`ho` | Set note mode |
| `channel:5` | `ch` | Set paint channel (0-15) |
| `octave:4` | `oc` | Set paint octave |
| `minoct:2` | `mi` | Set minimum octave limit |
| `maxoct:6` | `mo` | Set maximum octave limit |
| `decay` | `de` | Toggle decay system (velocity + probability drop with age) |
| `minvel:60` | `mv` | Set floor velocity at max age (1-127, default 40) |
| `minprob:10` | `mp` | Set floor fire probability at max age (1-100%, default 10) |
| `maxnotes:4` | `mn` | Set max notes per step per channel (0=unlimited) |
| `seq` | `sq` | Toggle sequencer mode between off and forward |
| `seq:forward/reverse/mirror/random/euclid:N` | `sq` | Select a sequencer mode; `N` sets Euclid pulses |
| `euclid:3` | `eu` | Set the Euclid pulse count (1-32) |
| `orient:h` | `or` | Set horizontal scan; use `orient:v` for vertical, or omit the value to toggle |
| `lockoct` | `lo` | Toggle octave lock (prevent octave drift on birth) |
| `chord` | `cd` | Toggle chord filter (off ↔ 135). Use `chord:1357`, `chord:125`, etc. for custom degrees |
| `dedup` | `dd` | Toggle note deduplication (merge identical notes, scale velocity by count) |
| `dedupcc:1` | `dc` | Set CC number for dedup count modulation (-1=disabled) |
| `rule:23/36` | `ru` | Set a survival/birth rule or preset: `life`, `highlife`, `34life`, `seeds`, `diamoeba`, `daynight`, `replicator`, `2x2`, `morley` |
| `conductor` | `cn` | Toggle conductor mode (manual evolution) |
| `microtune` | `mt` | Toggle microtuning pitch bend |
| `microtune:75` | `mt` | Set microtune amount (0-100) |
| `loop:8` | `lp` | Arm loop recording for N generations (auto-plays when done) |
| `loop:stop` | `lp` | Stop loop playback |
| `loop` | `lp` | Toggle loop playback on/off |
| `reset` | -- | Reset to initial state |

The `$` (self) operator also sends commands through the commander. For example, `$groove:75;25` will set the groove when the `$` operator is banged.

## Life Mode

Life mode is a plugin-only cellular-automata sequencer. Instead of executing Orca operators, the grid evolves musical cells: births, survivors, and deaths drive MIDI notes while the DAW transport provides timing.

### Quick start

1. Load the plugin before a MIDI instrument and start the DAW transport. In the standalone build, press Space to start its internal transport.
2. Press `Cmd+G` to enter Life mode.
3. Press Tab to choose an octave and Shift+Tab to choose a MIDI channel.
4. Type `A-G` for natural notes or `a-g` for sharps. Use the arrow keys to move and Backspace to erase.
5. Open the [Plugin Commander](#plugin-commander) with `Cmd+K` to set the scale, evolution rate, sequencer mode, and other musical rules.

### Entering Life Mode

Press `Cmd+G` to toggle between classic Orca and Life mode. Entering Life mode converts existing letter cells into live cells using the active paint channel and octave. Leaving Life mode silences Life notes and copies surviving note letters back to the classic grid.

### How It Works

- **Alive cells** carry a note (`A-G` or `a-g`), MIDI channel (0-15), octave (0-8), age, and lock state.
- **Birth:** Under the default rule, a dead cell with exactly three neighbors is born. Its pitch moves through the selected scale according to its position relative to its parents.
- **Survival:** Under the default rule, live cells with two or three neighbors survive.
- **Death:** Other live cells die and release their MIDI notes.
- **Toroidal wrapping:** The simulation wraps at every edge.
- **Protected cells:** Locked cells ignore death rules and act as permanent anchors.

### Modes

- **Pulse mode** (default): Notes retrigger on every evolution step. Clusters of same-note cells produce ratchets (rapid sequential notes)
- **Hold mode**: Notes trigger on birth and sustain until death

### Scales

Notes evolve within a selected scale. Available scales: chromatic, major, minor, pentatonic, dorian, phrygian, lydian, mixolydian, locrian, harmonic minor, melodic minor, minor pentatonic, blues, whole tone, diminished.

### Decay

**Decay** (`decay:on`) is a unified aging system that makes cells fade over time — both in volume and reliability:

- **Newborn cells** play at full velocity (127) and always fire (100% probability)
- **As cells age**, both velocity and probability decay linearly over 64 generations
- At max age, velocity reaches `minvel` (default 40) and probability reaches `minprob` (default 10%)
- `minvel:80` — raise the velocity floor (louder old cells)
- `minprob:30` — raise the probability floor (more reliable old cells)
- Models organic aging: young cells are loud and reliable, old cells become quiet and sporadic

### Density Thinning

**Max notes** (`maxnotes:N`) caps how many notes fire per step **per MIDI channel**:

- `maxnotes:0` = unlimited (default)
- `maxnotes:4` = at most 4 notes per channel per step
- Prevents wall-of-notes from dense patterns while giving each channel its own budget

### Sequencer Mode

Sequencer mode staggers note emission across row or column groups during each evolution cycle. Use `seq:<mode>` to select a mode, `orient:v` for a top-to-bottom row scan, or `orient:h` for a left-to-right column scan. All controls are also available as DAW parameters.

| Variant | Description |
|---------|-------------|
| `seq:off` | Emit eligible notes together without phase scanning |
| `seq:forward` | Scan in the selected orientation |
| `seq:reverse` | Scan in the opposite direction |
| `seq:mirror` | Ping-pong — alternates direction each evolution cycle |
| `seq:random` | Randomize phase order whenever the grid evolves |
| `seq:euclid:N` | Evenly distribute `N` active phases across the evolution rate |

- With `rate:8` and 16 rows in vertical orientation, rows 0-1 fire on frame 0, rows 2-3 on frame 1, and so on.
- Visual separators show phase groups, and a translucent sweep marks the active group.
- `euclid:3` changes the Euclid pulse count without changing the selected sequencer mode.
- Sequencer mode emits cells individually through phase scheduling rather than ratchet clustering.
- Phase scanning has no audible effect when `rate:1`.

### Octave Lock

**Octave lock** (`lockoct:on`) prevents octave drift during note evolution. When cells are born, their note still evolves by stepping through the scale, but the octave stays locked to the parent median. Useful in sequencer mode to keep patterns in their intended register.

### Chord Tone Filter

**Chord filter** snaps all emitted notes to the nearest chord tone of the current scale. This dramatically reduces dissonance by ensuring everything aligns harmonically:

- `chord` (toggle) — cycles between off and triad (degrees 1, 3, 5)
- `chord:135` — triad (root, 3rd, 5th)
- `chord:1357` — seventh chord (root, 3rd, 5th, 7th)
- `chord:125` — sus2 voicing
- `chord:145` — sus4 voicing
- `chord:1356` — sixth chord
- `chord:12356` — add2 chord
- `chord:1234567` — full scale (all degrees allowed)
- Custom degrees can be specified as any combination of digits 1-7

Works with any scale — for pentatonic scales (5 notes or fewer), all notes are considered chord tones. Especially effective with dense patterns that would otherwise produce clashing intervals.

### Note Deduplication

**Dedup** (`dedup:on`) merges identical `{channel, pitch}` notes into a single note, scaling velocity by the number of cells:

- **Velocity**: average velocity of all contributing cells + log2(count) × 15 boost. With decay on, this preserves the age-based variation while rewarding density
- **CC modulation** (optional): `dedupcc:1` sends CC1 (mod wheel) proportional to the count (8 cells = CC 127). Map this to filter cutoff or expression in your synth for timbral variation. Disabled by default (`dedupcc:-1`)
- Especially useful with dense patterns where many cells converge on the same note — instead of firing 5 identical C3s, fires one louder C3
- Pairs well with decay: average velocity keeps age dynamics, count boost rewards density

### Cellular Automata Rules

By default, Life mode uses Conway's Game of Life (conventionally B3/S23). Commander values use `survival/birth` order, so Conway is written `23/3`:

| Preset | Survival/Birth | Description |
|--------|-------------|-------------|
| `life` | 23/3 | Conway's Game of Life (default) |
| `highlife` | 23/36 | Self-replicating patterns |
| `34life` | 34/34 | Exploding/chaotic growth |
| `seeds` | /2 | All cells die, birth with 2 neighbors |
| `diamoeba` | 5678/35678 | Diamond-shaped amoeba patterns |
| `daynight` | 34678/3678 | Symmetric day/night behavior |
| `replicator` | 1357/1357 | Self-replicating patterns |
| `2x2` | 125/36 | 2×2 block patterns |
| `morley` | 245/368 | Move/Morley |

For custom rules, digits before `/` are survival counts and digits after `/` are birth counts. For example, `rule:23/36` selects HighLife.

### Conductor Mode

**Conductor mode** (`conductor` or `Cmd+T`) gives you manual control over when the grid evolves. Instead of evolving automatically every N frames, the grid only advances when you trigger it:

- **Enter key**: Press Enter to trigger one evolution step
- **External MIDI**: Any MIDI NoteOn received by the plugin triggers one evolution step — connect a keyboard, sequencer, or another plugin to control the pace externally

When combined with **sequencer mode**, the sequencer continues scanning rows independently at the evolve rate, but the GoL rules (birth/death/survival) only fire on trigger. This lets you keep the rhythmic pulse going while deciding when the pattern should evolve.

Mid-cycle triggers are supported: if you press Enter while the sequencer is mid-scan, the grid evolves immediately without disturbing the current seq phase.

### Microtuning

**Microtuning** (`microtune` or `Cmd+U`) adds pitch bend messages before each note based on how many neighbors that cell has:

- Cells with **2 neighbors** (the GoL equilibrium) get no bend (center = 8192)
- Cells with **fewer neighbors** bend downward
- Cells with **more neighbors** bend upward
- `microtune:75` sets the bend intensity (0-100, default 50)

This creates organic pitch variation tied to the local density of the cellular automaton — isolated cells sound slightly flat while crowded cells sound sharp. The pitch bend is channel-wide (per MIDI spec), so it affects all notes on that channel.

### Generation Loop

**Generation loop** records a sequence of evolution snapshots and loops them back:

1. `loop:8` — arms recording for the next 8 evolutions
2. As the grid evolves, each generation's grid state and MIDI events are captured
3. When recording completes, playback starts automatically — the grid cycles through the recorded generations instead of running live GoL rules
4. `loop:stop` or `Cmd+E` — stops playback and returns to live evolution
5. `loop` (bare) or `Cmd+E` — toggles playback on/off

The loop stores full grid snapshots (including ratchets and phase notes for seq mode), so playback is faithful to the original recording. Up to 64 generations can be recorded.

Status bar shows `loop:rec 3/8` during recording and `loop:5/8` during playback.

### Octave Range

Control the octave range with `minoct:N` and `maxoct:N`:

- `minoct:2` — no notes below octave 2
- `maxoct:5` — no notes above octave 5
- Born cells are clamped to the active range
- Octave cycling (Tab) wraps within the range
- Tighter ranges reduce dissonance from extreme octave spread

### Pattern Library

Life mode includes a library of classic GoL patterns organized by category:

- **Still Lifes** (11): Block, Beehive, Loaf, Boat, Tub, Pond, Ship, Long Boat, Barge, Mango, Eater 1
- **Oscillators** (14): Blinker, Toad, Beacon, Pulsar, Pentadecathlon, Clock, Octagon 2, Figure 8, Tumbler, Fumarole, Queen Bee Shuttle, Twin Bees Shuttle, Ants, Turning Toads
- **Spaceships** (9): Glider (4 directions), LWSS, MWSS, HWSS, Copperhead, Loafer
- **Methuselahs** (9): R-pentomino, Diehard, Acorn, Pi, B-heptomino, Thunderbird, Herschel, Rabbits, Lidka
- **Guns** (3): Gosper Gun, Simkin Gun, Simkin Gun 1B
- **Puffers** (2): Puffer 1, Switchengine

Patterns are placed with random notes from the current scale, using the active paint channel and octave.

### Life Mode Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Cmd+G` | Toggle Life mode on/off |
| `A-G` / `a-g` | Place a note cell |
| `Tab` | Cycle paint octave |
| `Shift+Tab` | Cycle paint channel |
| `Cmd+Shift+S` | Cycle scale |
| `Cmd+N` | Cycle root note (C, C#, D, ... B) |
| `Cmd+M` | Toggle pulse/hold mode |
| `Cmd+Shift+Up/Down` | Double/halve evolve rate |
| `Cmd+Shift+K` | Toggle lock (protect) selected cells |
| `Cmd+P` | Enter stamp mode / cycle pattern within category |
| `Cmd+Shift+P` | Cycle pattern backward within category |
| `Cmd+]` / `Cmd+[` | Cycle pattern category |
| `Enter` (conductor) | Trigger evolution |
| `Enter` (stamp mode) | Place pattern |
| `Escape` (stamp mode) | Cancel stamp mode |
| `Cmd+E` | Toggle recorded-loop playback |
| `Cmd+Shift+E` | Rotate selection 90° clockwise |
| `Cmd+T` | Toggle conductor mode |
| `Cmd+U` | Toggle microtuning |
| `Cmd+Shift+H` | Mirror selection horizontal |
| `Cmd+Shift+J` | Mirror selection vertical |
| `Cmd+Up/Down` | Shift octave of selected cells |
| `Cmd+Left/Right` | Shift channel of selected cells |
| `Cmd+R` | Reset to initial state |
| `Cmd+Shift+R` | Save current state as new initial |
| `Cmd+Z` | Undo |
| `Cmd+Shift+Z` | Redo |
| `Cmd+S` | Save as a `.life` file |

### .life File Format

Classic `.orca` files are plain-text grids. Life mode uses a separate text-based `.life` format because every cell also needs channel, octave, and lock metadata.

Save the active format with `Cmd+S`; open either format with `Cmd+O` or drag it onto the plugin. A `.life` file stores grid dimensions, live-cell metadata, scale, root, evolution rate, pulse mode, decay controls, maximum-note limit, sequencer mode, octave lock and range, chord filter, deduplication settings, and the cellular-automata rule. Settings absent from older files use defaults when loaded.

The current `.life` loader restores Off through Random sequencer modes. It does not yet round-trip Euclid mode or store the paint cursor, conductor/microtuning state, generation loops, Euclid pulse count, or scan orientation. DAW project state stores automatable parameters separately through the host.

## Reference tables

### Base-36 table

Orca uses base 36. Numeric operator inputs accept digits and letters according to the following table. For example, `Do` bangs every 24th frame.

| **0** | **1** | **2** | **3** | **4** | **5** | **6** | **7** | **8** | **9** | **A** | **B**  | 
| :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:    | 
| 0     | 1     | 2     | 3     | 4     | 5     | 6     | 7     | 8     | 9     | 10    | 11     |
| **C** | **D** | **E** | **F** | **G** | **H** | **I** | **J** | **K** | **L** | **M** | **N**  |
| 12    | 13    | 14    | 15    | 16    | 17    | 18    | 19    | 20    | 21    | 22    | 23     |
| **O** | **P** | **Q** | **R** | **S** | **T** | **U** | **V** | **W** | **X** | **Y** | **Z**  | 
| 24    | 25    | 26    | 27    | 28    | 29    | 30    | 31    | 32    | 33    | 34    | 35     |

### Transpose table

The MIDI operators interpret letters beyond the chromatic scale as transpositions. For example, `3H` is equivalent to `4A`.

| **0** | **1** | **2** | **3** | **4** | **5** | **6** | **7** | **8** | **9** | **A** | **B**  | 
| :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:    | 
| _     | _     | _     | _     | _     | _     | _     | _     | _     | _     | A0    | B0     |
| **C** | **D** | **E** | **F** | **G** | **H** | **I** | **J** | **K** | **L** | **M** | **N**  |
| C0    | D0    | E0    | F0    | G0    | A0    | B0    | C1    | D1    | E1    | F1    | G1     | 
| **O** | **P** | **Q** | **R** | **S** | **T** | **U** | **V** | **W** | **X** | **Y** | **Z**  | 
| A1    | B1    | C2    | D2    | E2    | F2    | G2    | A2    | B2    | C3    | D3    | E3     | 

## Companion Applications

- [Pilot](https://github.com/hundredrabbits/pilot), a companion synth tool.
- [Aioi](https://github.com/MAKIO135/aioi), a companion to send complex OSC messages.
- [Estra](https://github.com/kyleaedwards/estra), a companion sampler tool.
- [Gull](https://github.com/qleonetti/gull), a companion sampler, slicer and synth tool.
- [Sonic Pi](https://in-thread.sonic-pi.net/t/using-orca-to-control-sonic-pi-with-osc/2381/), a livecoding environment.
- [Remora](https://github.com/martinberlin/Remora), ESP32 LED controller firmware.

## Links

- [Overview Video](https://www.youtube.com/watch?v=RaI_TuISSJE)
- [Orca Podcast](https://futureofcoding.org/episodes/045)
- [Ableton & Unity3D](https://www.elizasj.com/unity_live_orca/)
- [Japanese Tutorial](https://qiita.com/rucochanman/items/98a4ea988ae99e04b333)
- [German Tutorial](http://tropone.de/2019/03/13/orca-ein-sequenzer-der-kryptischer-nicht-aussehen-kann-und-ein-versuch-einer-anleitung/)
- [French Tutorial](http://makingsound.fr/blog/orca-sequenceur-modulaire/)
- [Examples & Templates](https://git.sr.ht/~rabbits/orca-examples)

## Extras

- This application supports the [Ecosystem Theme](https://github.com/hundredrabbits/Themes).
- Download and share your patches on [PatchStorage](http://patchstorage.com/platform/orca/).
- Support this project through [Patreon](https://www.patreon.com/hundredrabbits).
- See [LICENSE.md](LICENSE.md) for license rights and limitations (MIT).
- Pull requests are welcome!
