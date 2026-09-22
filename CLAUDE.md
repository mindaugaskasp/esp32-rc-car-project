# Code Patterns & Project Guidelines

ESP32 RC car: `transmitter` and `receiver` firmware built from one source tree.
**Quality and maintainability outrank speed.** A slower change that leaves the tree clearer is the correct change — never trade structure for a quick fix.

## Hard rules — non-negotiable

- **DRY.** One fact, one place. Duplicated logic, or a constant repeated across files, is a defect — extract it. Both boards share `config/` and `comm/`: never fork a value between them, and reflash both when a shared constant changes.
- **YAGNI.** Build only what a current caller needs — no speculative parameters, hooks, or abstraction for an imagined future. Delete dead code on sight.
- **SOLID.** One responsibility per class; depend on the narrowest interface that works; a subtype must honour its base's contract (see `ScreenDriver`).
- **Clean code.** Small functions doing one thing at one level of abstraction; early returns over nesting; no side effect hidden behind an innocuous name.
- **Names must be obvious to someone who has never seen this codebase.** A function name states what it does and what it returns: `computeServoMicros`, not `calc` or `doServo`. If a name needs a comment to be understood, rename it.
- Never leave a workaround unexplained; never silence a warning instead of fixing its cause.
- No prose comments, meaning must be communicated via code only, unless it is non obvious, specific nuanced issue.
- No class/file can be longer than 200 lines, no function can be longer  than 35 lines. Non negotiable.

## Layers — organise by altitude, not by board

| Layer | Directory | Rule |
|---|---|---|
| L0 drivers | `drivers/` | Touches a peripheral directly (ADC, PWM, I²C, radio, Serial). Nothing higher. |
| L1 comm | `comm/` | Messaging on the radio: framing, handshake, dedup, watchdog. |
| L2 ui | `ui/` | Screens and menus. Draws via the display driver; owns no hardware. |
| L2 app | `app/` | Top-level feature modes orchestrating L0/L1/UI. |

Board membership lives **only** in `build_src_filter`, never in a folder name. `config/` holds tuning, one subsystem per file; `config/controller/` holds pin maps. Both `main_*.cpp` stay thin — mode state machine and `setup()`/`loop()` wiring only. A class that merely *uses* a driver belongs in `comm/`/`ui/`/`app/`, not `drivers/`.

**`build_src_filter` is MANDATORY for every new file.** The transmitter pulls `drivers/ comm/ ui/ app/`; the headless receiver pulls `drivers/ comm/` only. A board-specific file in a shared layer MUST be excluded from the board that does not use it or that build fails: add `-<path>` (directory or single file) to the other board's filter in `platformio.ini`, where current exclusions live. `ui/` and `app/` need nothing — the receiver never includes them.

## Drivers and config

- `DriverName.h` — `#pragma once`, public API only, no global mutable state. Functions named `initX()` / `updateX()` / `setX()`.
- `DriverName.cpp` — own header first, then project headers by full path, then external libraries. All internal state `static`.
- `DriverNameLogic.h` — every non-trivial computation goes here: header-only, `inline`, **no Arduino or hardware dependency**, standard C++ only. This is what makes logic natively testable; write `map`/`constrain` arithmetic out inline.
- `constexpr` for typed constants; `#define` only for macros, conditional-compilation flags, and tuning values shared with the native test build.
- Keep values tuned or read together adjacent; when a constant constrains one in another file, cross-reference it by name in a short comment.
- **No alignment padding** — one space around `=` and between type and name. Aligned columns wreck diffs; `make check` and `.clang-format` enforce this.
- **No magic numbers**: every hardware limit, protocol value and calibration parameter is a named constant.

## Naming and comments

Classes/types PascalCase · functions/variables camelCase · constants and macros UPPER_SNAKE_CASE · enum values PascalCase · files PascalCase (`ServoDriver.h`).

- **No single-character names.** Sole exception: `x`/`y`/`w`/`h` in the display drawing API.
- **No cryptic abbreviations** — `TransmitterOperatingMode`, not `TxMode`. Exempt only: `Esc`, `Pwm`, `Adc`, `Rpm`, `Rtt`, `Mac`, `I2C`, `Micros`, fixed-width suffixes.
- Always `enum class`, never unscoped `enum`.
- A comment is a last resort, justified only when intent cannot be carried by the code: a hardware quirk, a timing or protocol constraint, an ISR-safety reason, a workaround, or a cross-file invariant. Never restate the code; if a rename would remove the comment, rename instead.

## C++ on this target

- `const` everything that never changes; `const&` for non-trivial parameters, by value for small PODs; `const` member functions that do not mutate.
- Initialise every variable at its declaration (`{}` when there is no better value).
- References for required arguments; pointers only when genuinely nullable, and null-checked before use.
- `static_cast`/`reinterpret_cast` only — C-style casts are banned. `nullptr`, never `NULL`.
- `override` on every virtual override; single-argument constructors `explicit`.
- Fixed-width integers for anything crossing the wire, mapping to a register, or carrying a size contract; plain `int` only where width is irrelevant.
- **No dynamic allocation, exceptions or RTTI** — no `new`/`malloc`/`std::string`/heap containers/`throw`/`dynamic_cast`. Fixed-size buffers, stack or static storage.
- `static` for internal linkage; never `using namespace` at file scope in a header.

## Runtime constraints

Wire structs are POD — fixed-width fields, no constructors or virtuals, guarded by a `static_assert` on `sizeof`. ESP-NOW receive callbacks run in ISR context: **no allocation, no `Serial`, no locks, no blocking** — copy into a static pending buffer inside a critical section, set a flag, and do the real work in `loop()`. `delay()` is banned in `loop()` and in driver update functions. It is allowed in one-time init where the timing *is* the feature, in `setup()` for settling, and in a one-shot event that provably cannot overlap active control (see `twitchServo()`, which runs once on link-up before the ESC arms) — such a use must carry a comment saying why it is safe. Use `millis()` deltas everywhere else. Target loop cycle 20–50 ms with no blocking I/O.

## Testing

Tests live in `test/test_<name>/` and run natively under Unity (`make test`); the `native` env compiles no production `.cpp`, only the headers a test includes. Only pure logic is testable — `*Logic.h` functions, struct layout, mapping/deadzone/clamping — so extract the computation into a `*Logic.h` first. Anything calling `analogRead`, `Servo`, `Serial`, `millis` or ESP-NOW is not. New suites need no `platformio.ini` entry. Assert against config symbols, never hardcoded tuned values, so tests survive recalibration.

## Workflow

`make help` lists every target. `make test` runs unit tests, `make check` runs cppcheck on both firmwares, `build-tx`/`build-rx` static-check then compile, and `upload-tx`/`upload-rx` flash using ports cached in `.ports` (`make set-tx`/`set-rx`, overridable with `PORT=`). Every build and upload runs `pio check` first — **no medium or high severity defect may exist for a build to pass.** Formatting is governed by `.clang-format`.
