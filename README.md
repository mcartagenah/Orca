# ORCΛ

<img src="https://raw.githubusercontent.com/hundredrabbits/100r.co/master/media/content/characters/orca.hello.png" width="300"/>

Orca is an [esoteric programming language](https://en.wikipedia.org/wiki/Esoteric_programming_language) designed to quickly create procedural sequencers, in which every letter of the alphabet is an operation, where lowercase letters operate on bang, uppercase letters operate each frame.

This application **is not a synthesizer, but a livecoding environment** capable of sending MIDI, OSC & UDP to your audio/visual interfaces, like Ableton, Renoise, VCV Rack or SuperCollider.

If you need **help**, visit the [chatroom](https://discord.gg/F7W98pXKd7), the [mailing list](https://lists.sr.ht/~rabbits/orca), join the [forum](https://llllllll.co/t/orca-live-coding-tool/17689) or watch a [tutorial](https://www.youtube.com/watch?v=ktcWOLeWP-g).

- [Download builds](https://hundredrabbits.itch.io/orca), available for **Linux, Windows and OSX**.
- Use [in your browser](https://hundredrabbits.github.io/Orca/), requires **webMidi**.
- Use [in a terminal](https://git.sr.ht/~rabbits/orca), written in C.
- Use [on small computers](https://git.sr.ht/~rabbits/orca-toy), written in assembly.
- Use [on the Monome Norns](https://llllllll.co/t/orca/22492), written in Lua.

## Install & Run

### Desktop (Electron)

If you wish to use Orca inside of [Electron](https://electronjs.org/), follow these steps:

```
git clone https://github.com/mcartagenah/Orca.git
cd Orca/desktop/
npm install
npm start
```

### Plugin (AU/VST3)

Orca is also available as an **AU/VST3 plugin** for use inside DAWs like Ableton Live, Logic Pro, etc. The plugin uses a native C++ engine and creates a virtual MIDI port named "Orca".

```
git clone https://github.com/mcartagenah/Orca.git
cd Orca/plugin/
make all
```

This builds and installs both the AU (`~/Library/Audio/Plug-Ins/Components/`) and VST3 (`~/Library/Audio/Plug-Ins/VST3/`) versions. macOS only.

#### Plugin Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Cmd+O` | Open .orca file |
| `Cmd+S` | Save as .orca/.life file |
| `Cmd+L` | Import modules (load .orca/.life files into inject cache) |
| `Cmd+G` | Toggle Life (Game of Life) mode |
| `Cmd+K` | Open command prompt |
| `Cmd+F` | Open command prompt with `find:` |
| `Cmd+=` | Zoom in |
| `Cmd+-` | Zoom out |
| `Cmd+Z` | Undo |
| `Cmd+Shift+Z` | Redo |
| Arrow keys | Move cursor |
| `Shift+Arrows` | Expand selection |
| `Cmd+C/V/X` | Copy/Paste/Cut |
| `Backspace` | Delete selected cells |

#### Plugin Parameters

- **Shuffle** (0-200%): DAW-automatable swing parameter. 0% = max inverse swing, 100% = straight, 200% = max swing.

#### MIDI Output

The plugin sends MIDI in two ways:
1. **Host MIDI bus**: Routed through the DAW's MIDI output (for instrument chains).
2. **Virtual MIDI port "Orca"**: Available system-wide to any application listening for MIDI input.

<img src='https://raw.githubusercontent.com/hundredrabbits/Orca/master/resources/preview.jpg' width="600"/>

## Operators

To display the list of operators inside of Orca, use `CmdOrCtrl+G`.

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
- `?` **pb**(channel value): Sends MIDI pitch bench.
- `;` **udp**: Sends UDP message.
- `=` **osc**(*path*): Sends OSC message.
- `$` **self**: Sends [ORCA command](#Commands).

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

## MIDI

The [MIDI](https://en.wikipedia.org/wiki/MIDI) operator `:` takes up to 5 inputs('channel, 'octave, 'note, velocity, length). 

For example, `:25C`, is a **C note, on the 5th octave, through the 3rd MIDI channel**, `:04c`, is a **C# note, on the 4th octave, through the 1st MIDI channel**. Velocity is an optional value from `0`(0/127) to `g`(127/127). Note length is the number of frames during which a note remains active. See it in action with [midi.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_midi.orca).

## MIDI MONO

The [MONO](https://en.wikipedia.org/wiki/Monophony) operator `%` takes up to 5 inputs('channel, 'octave, 'note, velocity, length). 

This operator is very similar to the default Midi operator, but **each new note will stop the previously playing note**, would its length overlap with the new one. Making certain that only a single note is ever played at once, this is ideal for monophonic analog synthesisers that might struggle to dealing with chords and note overlaps.

## MIDI CC

The [MIDI CC](https://www.sweetwater.com/insync/continuous-controller/) operator `!` takes 3 inputs('channel, 'knob, 'value).

It sends a value **between 0-127**, where the value is calculated as a ratio of 36, over a maximum of 127. For example, `!008`, is sending **28**, or `(8/36)*127` through the first channel, to the control mapped with `id0`. You can press **enter**, with the `!` operator selected, to assign it to a controller. By default, the operator sends to `CC64` [and up](https://nickfever.com/Music/midi-cc-list), the offset can be changed with the [command](#commands) `cc:0`, to set the offset to 0.

## MIDI PITCHBEND

The [MIDI PB](https://www.sweetwater.com/insync/pitch-bend/) operator `?` takes 3 inputs('channel, 'lsb, 'msb).

It sends two different values **between 0-127**, where the value is calculated as a ratio of 36, over a maximum of 127. For example, `?008`, is sending an MSB of **28**, or `(8/36)*127` and an LSB of 0 through the first midi channel.

## MIDI BANK SELECT / PROGRAM CHANGE

This is a command (see below) rather than an operator and it combines the [MIDI program change and bank select functions](https://www.sweetwater.com/sweetcare/articles/6-what-msb-lsb-refer-for-changing-banks-andprograms/). 

The syntax is `pg:channel;msb;lsb;program`. Channel is 0-15, msb/lsb/program are 0-127, but program will automatically be translated to 1-128 by the MIDI driver. `program` typically corresponds to a "patch" selection on a synth. Note that `msb` may also be identified as "bank" and `lsb` as "sub" in some applications (like Ableton Live). 

`msb` and `lsb` can be left blank if you only want to send a simple program change. For example, `pg:0;;;63` will set the synth to patch number 64 (without changing the bank)

## UDP

The [UDP](https://nodejs.org/api/dgram.html#dgram_socket_send_msg_offset_length_port_address_callback) operator `;` locks each consecutive eastwardly ports. For example, `;hello`, will send the string "hello", on bang, to the port `49160` on `localhost`. In commander, use `udp:7777` to select the **custom UDP port 7777**, and `ip:127.0.0.12` to change the target IP. UDP is not available in the browser version of Orca.

You can use the [listener.js](https://github.com/hundredrabbits/Orca/blob/master/resources/listener.js) to test UDP messages. See it in action with [udp.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_udp.orca).

## OSC

The [OSC](https://github.com/MylesBorins/node-osc) operator `=` locks each consecutive eastwardly ports. The first character is used for the path, the following characters are sent as integers using the [base36 Table](https://github.com/hundredrabbits/Orca#base36-table). In commander, use `osc:7777` to select the **custom OSC port 7777**, and `ip:127.0.0.12` to change the target IP. OSC is not available in the browser version of Orca.

For example, `=1abc` will send `10`, `11` and `12` to `/1`, via the port `49162` on `localhost`; `=a123` will send `1`, `2` and `3`, to the path `/a`. You can use the [listener.js](https://github.com/hundredrabbits/Orca/blob/master/resources/listener.js) to test OSC messages. See it in action with [osc.orca](https://git.sr.ht/~rabbits/orca-examples/tree/master/basics/_osc.orca) or try it with [SonicPi](https://github.com/hundredrabbits/Orca/blob/master/resources/TUTORIAL.md#sonicpi).

<img src='https://raw.githubusercontent.com/hundredrabbits/Orca/master/resources/preview.hardware.jpg' width="600"/>

## Advanced Controls

Some of Orca's features can be **controlled externally** via UDP though port `49160`, or via its own command-line interface. To activate the command-line prompt, press `CmdOrCtrl+K`. The prompt can also be used to inject patterns or change settings.

### Project Mode

You can **quickly inject orca files** into the currently active file, by using the command-line prompt — Allowing you to navigate across multiple files like you would a project. Press `CmdOrCtrl+L` to load multiple orca files, then press `CmdOrCtrl+B` and type the name of a loaded `.orca` file to inject it.

### Default Ports

| UDP Input  | OSC Input  | UDP Output | OSC Output |
| ---------- | ---------- | ---------- | -----------|
| 49160      | None       | 49161      | 49162

### Commands

All commands have a shorthand equivalent to their first two characters, for example, `write` can also be called using `wr`. You can see the full list of commands [here](https://github.com/hundredrabbits/Orca/blob/master/desktop/sources/scripts/commander.js).

- `play` Plays program.
- `stop` Stops program.
- `run` Runs current frame.
- `bpm:140` Sets bpm speed to `140`.
- `apm:160` Animates bpm speed to `160`.
- `frame:0` Sets the frame value to `0`.
- `skip:2` Adds `2`, to the current frame value.
- `rewind:2` Removes `2`, to the current frame value.
- `color:f00;0f0;00f` Colorizes the interface.
- `find:aV` Sends cursor to string `aV`.
- `select:3;4;5;6` Move cursor to position `3,4`, and select size `5:6`(optional).
- `inject:pattern;12;34` Inject the local file `pattern.orca`, at `12,34`(optional).
- `write:H;12;34` Writes glyph `H`, at `12,34`(optional).
- `time` Prints the time, in minutes seconds, since `0f`.
- `midi:1;2` Set Midi output device to `#1`, and input device to `#2`.
- `udp:1234` Set UDP output port to `1234`.
- `osc:1234` Set OSC output port to `1234`.
- `ip:127.0.0.12` Set target IP for UDP/OSC output.
- `groove:75;25` Set groove ratios (see [Groove](#groove)).

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

- **Cmd+G**: Opens a dialog to enter groove ratios manually (e.g. `75;25`).
- **Shuffle slider**: A DAW-automatable parameter (0-200%) that maps to a 3-step groove. 0% = max inverse swing, 100% = straight, 200% = max swing.

The current groove is displayed in the status bar as `groove:75;25;50`.

#### Commander (Command Prompt)

Press `Cmd+K` to open the command prompt, or `Cmd+F` to open with `find:` pre-filled. Type a command and press Enter to execute, or Escape to cancel. Up/Down arrows recall command history.

All commands support 2-letter shorthands (first two characters).

**Universal commands (both modes):**

| Command | Short | Description |
|---------|-------|-------------|
| `find:text` | `fi` | Find text in grid and move cursor to first match |
| `select:x;y;w;h` | `se` | Move cursor and optionally set selection size |
| `write:text;x;y` | `wr` | Write text at position (or cursor if x;y omitted) |
| `groove:75;25` | `gr` | Set groove ratios |
| `cc:0` | `cc` | Set MIDI CC offset |
| `pg:ch;msb;lsb;pgm` | `pg` | MIDI program change with optional bank select |
| `copy` | `co` | Copy selection |
| `paste` | `pa` | Paste clipboard |
| `erase` | `er` | Erase selection |
| `time` | `ti` | Write current time at cursor |
| `color:f00;0f0;00f` | `cl` | Set theme colors (bLow;bMed;bHigh as hex RGB) |
| `inject:name` | `in` | Inject cached module at cursor (or `inject:name;x;y`) |

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
| `rate:4` | `ra` | Set evolve rate (power of 2) |
| `pulse` / `hold` | `pu`/`ho` | Set note mode |
| `channel:5` | `ch` | Set paint channel (0-15) |
| `octave:4` | `oc` | Set paint octave |
| `minoct:2` | `mi` | Set minimum octave limit |
| `maxoct:6` | `mo` | Set maximum octave limit |
| `decay` | `de` | Toggle decay system (velocity + probability drop with age) |
| `minvel:60` | `mv` | Set floor velocity at max age (1-127, default 40) |
| `minprob:10` | `mp` | Set floor fire probability at max age (1-100%, default 10) |
| `maxnotes:4` | `mn` | Set max notes per step per channel (0=unlimited) |
| `seq` | `sq` | Toggle sequencer mode (row phase offset) |
| `lockoct` | `lo` | Toggle octave lock (prevent octave drift on birth) |
| `chord` | `cd` | Toggle chord tone filter (snap to root/3rd/5th) |
| `dedup` | `dd` | Toggle note deduplication (merge identical notes, scale velocity by count) |
| `dedupcc:1` | `dc` | Set CC number for dedup count modulation (-1=disabled) |
| `rule:23/36` | `ru` | Set CA rule in S/B notation (or preset: `life`, `highlife`, `34life`, `seeds`, `diamoeba`, `daynight`, `replicator`, `2x2`) |
| `reset` | -- | Reset to initial state |

The `$` (self) operator also sends commands through the commander. For example, `$groove:75;25` will set the groove when the `$` operator is banged.

## Life Mode

Orca includes a **Game of Life sequencer mode** — a completely different mode where the grid runs Conway's Game of Life rules with musical note cells. Cells are born, survive, and die according to GoL rules, triggering MIDI notes on birth and silencing on death.

### Entering Life Mode

Press `Cmd+G` to toggle between normal Orca mode and Life mode. In Life mode, the grid is replaced with a cellular automaton where each alive cell represents a musical note.

### How It Works

- **Alive cells** have a note (A-G), MIDI channel (0-15), and octave (0-7)
- **Birth**: When a dead cell has exactly 3 alive neighbors, a new cell is born. Its note is derived by stepping up or down within the current scale, based on the direction of propagation relative to its parents
- **Survival**: Cells with 2-3 neighbors survive
- **Death**: Cells with fewer than 2 or more than 3 neighbors die, sending a MIDI note-off
- **Toroidal wrapping**: The grid wraps around — patterns going off one edge reappear on the opposite side
- **Protected cells**: Locked cells are immune to death rules, acting as permanent anchors

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

**Sequencer mode** (`seq:on`) transforms the grid into a top-to-bottom step sequencer by staggering note emission across rows:

- Instead of all rows firing simultaneously, each row group fires on a different frame within the evolve cycle
- With `rate:8` and 16 rows: rows 0-1 fire on frame 0, rows 2-3 on frame 1, etc.
- Visual guides show phase group boundaries; a sweeping highlight shows the active row group
- Later rows naturally get shorter notes, reinforcing the sequential feel
- Disables ratchet clustering (cells fire individually with phase scheduling)
- No effect when `rate:1`

### Octave Lock

**Octave lock** (`lockoct:on`) prevents octave drift during note evolution. When cells are born, their note still evolves by stepping through the scale, but the octave stays locked to the parent median. Useful in sequencer mode to keep patterns in their intended register.

### Chord Tone Filter

**Chord filter** (`chord:on`) snaps all emitted notes to the nearest **chord tone** (root, 3rd, 5th degree of the current scale). This dramatically reduces dissonance by ensuring everything aligns to the triad:

- Works with any scale — for pentatonic scales (5 notes or fewer), all notes are considered chord tones
- Combine with `scale:minor` or `scale:major` for clean harmonic output
- Especially effective with dense patterns that would otherwise produce clashing intervals

### Note Deduplication

**Dedup** (`dedup:on`) merges identical `{channel, pitch}` notes into a single note, scaling velocity by the number of cells:

- **Velocity**: average velocity of all contributing cells + log2(count) × 15 boost. With decay on, this preserves the age-based variation while rewarding density
- **CC modulation** (optional): `dedupcc:1` sends CC1 (mod wheel) proportional to the count (8 cells = CC 127). Map this to filter cutoff or expression in your synth for timbral variation. Disabled by default (`dedupcc:-1`)
- Especially useful with dense patterns where many cells converge on the same note — instead of firing 5 identical C3s, fires one louder C3
- Pairs well with decay: average velocity keeps age dynamics, count boost rewards density

### Cellular Automata Rules

By default, Life mode uses Conway's Game of Life (B3/S23). You can change the rules with the `rule` command:

- `rule:23/3` — Conway's Life (S23/B3, default)
- `rule:23/36` — HighLife (S23/B36, self-replicating patterns)
- `rule:34/34` — 34 Life
- `rule:life` / `rule:highlife` / `rule:seeds` / `rule:diamoeba` / `rule:daynight` / `rule:replicator` / `rule:2x2` — named presets

Rules use S/B notation: survival digits before `/`, birth digits after. For example, `23/36` means cells survive with 2 or 3 neighbors, and are born with 3 or 6 neighbors.

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
| `Cmd+Shift+Up/Down` | Increase/decrease evolve rate (powers of 2) |
| `Cmd+Shift+K` | Toggle lock (protect) selected cells |
| `Cmd+P` | Enter stamp mode / cycle pattern within category |
| `Cmd+Shift+P` | Cycle pattern backward within category |
| `Cmd+]` / `Cmd+[` | Cycle pattern category |
| `Enter` (stamp mode) | Place pattern |
| `Escape` (stamp mode) | Cancel stamp mode |
| `Cmd+E` | Rotate selection 90° clockwise |
| `Cmd+Shift+H` | Mirror selection horizontal |
| `Cmd+Shift+J` | Mirror selection vertical |
| `Cmd+Up/Down` | Shift octave of selected cells |
| `Cmd+Left/Right` | Shift channel of selected cells |
| `Cmd+R` | Reset to initial state |
| `Cmd+Shift+R` | Save current state as new initial |
| `Cmd+Z` | Undo |
| `Cmd+Shift+Z` | Redo |
| `Cmd+S` | Save as .life file |

### .life File Format

Life mode uses its own file format (`.life`) separate from `.orca` files. Files can be saved with `Cmd+S` and loaded via drag-and-drop. The format stores grid dimensions, cell data (note, channel, octave, lock state), and all settings (scale, root, evolve rate, pulse mode, decay, min velocity, min probability, max notes, sequencer mode, octave lock, chord filter, dedup, dedup CC, min/max octave, CA rule).

## Base36 Table

Orca operates on a base of **36 increments**. Operators using numeric values will typically also operate on letters and convert them into values as per the following table. For instance `Do` will bang every *24th frame*. 

| **0** | **1** | **2** | **3** | **4** | **5** | **6** | **7** | **8** | **9** | **A** | **B**  | 
| :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:   | :-:    | 
| 0     | 1     | 2     | 3     | 4     | 5     | 6     | 7     | 8     | 9     | 10    | 11     |
| **C** | **D** | **E** | **F** | **G** | **H** | **I** | **J** | **K** | **L** | **M** | **N**  |
| 12    | 13    | 14    | 15    | 16    | 17    | 18    | 19    | 20    | 21    | 22    | 23     |
| **O** | **P** | **Q** | **R** | **S** | **T** | **U** | **V** | **W** | **X** | **Y** | **Z**  | 
| 24    | 25    | 26    | 27    | 28    | 29    | 30    | 31    | 32    | 33    | 34    | 35     |

## Transpose Table

The midi operator interprets any letter above the chromatic scale as a transpose value, for instance `3H`, is equivalent to `4A`.

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
- [Remora](https://github.com/martinberlin/Remora), a ESP32 Led controller firmware.

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
- See the [License](LICENSE.md) file for license rights and limitations (MIT).
- Pull Requests are welcome!
