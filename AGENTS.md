# AGENTS.md

This file applies to the entire repository. Orca is an upstream-derived live-coding sequencer with two front ends: the original JavaScript application and this fork's native JUCE plugin. Preserve upstream behavior unless a task explicitly changes it, and keep fork-specific behavior documented.

## Start here

Before editing:

1. Run `git status --short` and preserve all existing changes. The repository is often used with in-progress plugin work in the working tree.
2. Read the relevant section of `README.md`; it is the best user-facing description of current behavior.
3. Inspect recent commits touching the same subsystem. Recent fork work is concentrated under `plugin/` and usually updates `README.md` alongside behavior.
4. Treat `PLAN.md` and `.claude/plans/`, when present, as historical design notes, not current specifications. They are local/ignored context, predate several implemented features, and may contradict the code. For example, the original plugin plan says OSC/UDP are no-ops, while the current plugin implements both.
5. Do not modify `.claude/settings.local.json`, local plans, or other agent-specific files unless the user asks.

Do not overwrite, reformat, stage, or revert unrelated user changes. Do not commit or push unless explicitly requested.

## Repository map

- `README.md`: installation, operators, shortcuts, commands, plugin parameters, and Life mode behavior. Update it for user-visible changes.
- `index.html`, `manifest.json`, `sw.js`: browser/PWA entry point. The browser build loads the scripts under `desktop/sources/` directly.
- `desktop/`: Electron application and the canonical JavaScript implementation of classic Orca behavior.
  - `desktop/sources/scripts/core/orca.js`: grid storage, parsing, locking, and frame execution.
  - `desktop/sources/scripts/core/operator.js`: JavaScript operator base behavior.
  - `desktop/sources/scripts/core/library.js`: operator definitions.
  - `desktop/sources/scripts/core/io/`: MIDI, mono, CC, OSC, and UDP output stacks.
  - `desktop/sources/scripts/client.js`, `cursor.js`, `commander.js`, `clock.js`: UI, editing, commands, and scheduling.
- `plugin/`: macOS-oriented C++17 JUCE AU/VST3/Standalone implementation and the main surface of this fork.
  - `plugin/CMakeLists.txt`: plugin formats, sources, definitions, and Apple framework links.
  - `plugin/JUCE`: pinned git submodule. Treat it as third-party code.
  - `plugin/Source/engine/Types.h`: fixed limits and shared engine/MIDI types.
  - `plugin/Source/engine/Grid.*`: classic Orca grid state and parsing.
  - `plugin/Source/engine/Operator.*`: classic operator dispatch and behavior.
  - `plugin/Source/engine/{MidiStack,CcStack,MonoStack,EngineIO}.h`: fixed-capacity event collection.
  - `plugin/Source/engine/Engine.h`: top-level classic/Life dispatch and UI shadow buffers.
  - `plugin/Source/engine/LifeGrid.h`: Life mode state, cellular automata, sequencing, note generation, patterns, and loop state. It is intentionally header-only and large.
  - `plugin/Source/PluginProcessor.*`: audio-thread scheduling, APVTS parameters/state, host and CoreMIDI output, and deferred network messages.
  - `plugin/Source/PluginEditor.*`: rendering, keyboard/mouse editing, Commander execution, history, and `.orca`/`.life` I/O.
  - `plugin/Source/Commander.h`: Commander input state and parsing; execution lives in `PluginEditor.cpp`.
- `resources/`: manual, examples, and web assets inherited from Orca.

## Build and validation

Initialize the dependency after cloning:

```sh
git submodule update --init --recursive
```

Configure and build the plugin from the repository root:

```sh
cmake -S plugin -B plugin/build -DCMAKE_BUILD_TYPE=Release
cmake --build plugin/build --config Release --target OrcaPlugin_All -j8
```

For a focused smoke build, `OrcaPlugin_Standalone`, `OrcaPlugin_AU`, and `OrcaPlugin_VST3` are valid targets. Generated artifacts belong under ignored `plugin/build/`; never add them to git.

The `plugin/Makefile` is convenient but has important side effects:

- `make -C plugin build` only builds an already configured tree.
- `make -C plugin all`, `install`, `install-au`, and `install-vst3` replace plugins under `~/Library/Audio/Plug-Ins/`.
- `make -C plugin validate` installs the AU before running `auval`.
- `make -C plugin clean` removes the full plugin build tree.

Do not run install, validation, or clean targets unless the task calls for that side effect or the user approves it. A CMake build is the default non-invasive validation.

For the desktop application, use Node 16 from `desktop/.nvmrc`:

```sh
cd desktop
npm ci
npm start
```

The repository currently has no automated test suite and no CI workflow. Do not claim tests passed when only compilation passed. Report exactly which targets or manual checks were run. `npm run fix` runs StandardJS with `--fix` and mutates files; use it only when formatting the JavaScript intentionally.

## Implementation rules

### Preserve classic Orca semantics

The JavaScript engine is the reference for classic grid/operator behavior. When changing a shared operator, compare both implementations and keep these semantics aligned unless the feature is intentionally plugin-only:

- Base-36 values use `0123456789abcdefghijklmnopqrstuvwxyz`.
- Uppercase operators run every frame; lowercase operators run when banged.
- `.` is an empty cell and `*` is a bang.
- Parsing, port locking, default values, case behavior, MIDI scaling, and frame order are observable language behavior.

A new or changed shared operator usually requires updates to JavaScript `library.js`, C++ `Operator.h/.cpp`, plugin port/name rendering in `PluginEditor.cpp`, and `README.md`.

### Keep the audio callback real-time safe

`OrcaProcessor::processBlock()` runs on the audio thread. In code reachable from it:

- Avoid heap allocation, file access, dialogs, network I/O, logging, unbounded work, and blocking locks.
- Prefer the existing fixed-size arrays and `kMax*` limits. Handle overflow explicitly and safely.
- Send UI-originated MIDI through `pendingLifeEvents`, and defer `$` commands, UDP, and OSC through the pending queues for `GridComponent::timerCallback()`.
- Keep critical sections short. Copy pending data while holding `pendingLock`, then perform I/O after releasing it.
- Use atomics for simple cross-thread triggers/counters, following the existing memory-order style.

The editor/message thread and audio thread share `processor.engine`. Mutations from the editor must use `processor.engineLock`; audio-thread stepping already uses that lock. Refresh shadow state after edits. Do not introduce direct UI reads from mutable engine arrays when an existing shadow field can be used.

### Maintain parameter and state compatibility

APVTS parameter IDs and choice ordering are external compatibility surfaces for DAW sessions and automation. When adding or changing a plugin parameter:

1. Define it in `createParameterLayout()` in the appropriate group.
2. Add and initialize its cached pointer in `OrcaProcessor`.
3. Synchronize it to engine state in `syncParamsToLifeGrid()` or the appropriate processor path.
4. Route Commander/UI changes through `setValueNotifyingHost()` so host automation sees them.
5. Update state restore, `.life` serialization, UI/status text, and `README.md` where applicable.

Do not rename existing parameter IDs or reorder choice values casually. Keep preset arrays and enums synchronized with their `AudioParameterChoice` order.

The plugin persists three related formats: DAW XML state, plain-text `.orca` grids, and metadata-rich `.life` files. Preserve backward-compatible defaults when extending any of them. Keep Life storage dimensions (`w`/`h`) distinct from visible toroidal dimensions (`wrapW`/`wrapH`).

### Keep UI, commands, and docs in sync

For a new Commander command, update its full name, optional two-letter shorthand in `Commander.h`, execution/preview behavior in `PluginEditor.cpp`, and the README command table. Check for shorthand collisions.

For shortcuts, ensure the implementation and README agree. macOS command-key behavior is the plugin's current baseline; do not assume the Electron shortcuts are identical.

User-visible plugin features normally need all relevant layers updated: engine behavior, processor parameter/state wiring, editor interaction/rendering, file serialization, and README documentation.

## Style and scope

- C++ uses C++17, four-space indentation, braces on the same line, `orca` namespace for engine code, and JUCE types in the plugin/UI layer. Follow the surrounding file rather than applying a bulk formatter.
- Existing plugin sources use CRLF line endings. Avoid whole-file line-ending churn.
- JavaScript follows StandardJS style: two-space indentation, single quotes, no semicolons, and function-constructor patterns used by the existing code.
- Prefer fixed-capacity storage in engine/audio paths. JUCE containers and strings are acceptable on the message thread.
- Keep changes narrow. Do not refactor the large editor or Life engine as collateral work.
- Do not edit `plugin/JUCE` unless the task is specifically a JUCE dependency change.
- Do not add generated build products, installed plugin bundles, `.DS_Store`, editor settings, or local agent permission files.

## Completion checklist

Before handing off a change:

1. Recheck `git status` and `git diff` for unrelated or accidental formatting changes.
2. Build the affected plugin target for C++ changes; smoke-test Electron/browser behavior for JavaScript changes when feasible.
3. Check thread safety, fixed-buffer bounds, APVTS choice ordering, and state/file backward compatibility for affected code.
4. Update `README.md` for user-visible behavior, commands, shortcuts, parameters, or file-format changes.
5. State validation limitations plainly, especially where DAW routing, CoreMIDI, AU validation, OSC/UDP, or interactive UI behavior was not exercised.

Recent commit messages favor concise imperative summaries such as `Add ...`, `Update ...`, or `Fix ...`. Match that style only if the user asks for a commit.
