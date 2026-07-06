# Code Patterns & Project Guidelines

This document establishes the coding patterns and architectural standards for the ESP32 RC Car project. Follow these guidelines to maintain consistency and modularity.

## Project Structure

The source tree is organized by **layer**, not by board. Each top-level directory is one
altitude of abstraction; board membership (transmitter vs. receiver) is expressed only in
`build_src_filter`, never by folder.

| Layer | Directory | Rule |
|-------|-----------|------|
| **L0 — Hardware drivers** | `drivers/` | Touches a peripheral directly (ADC, PWM, I²C, radio, Serial). Nothing higher-level lives here. |
| **L1 — Comm / link** | `comm/` | Messaging protocols built *on top of* the radio driver: framing, channel handshake, dedup, watchdog. |
| **L2 — UI** | `ui/` | Screens, menus, on-screen views. Draws via the display driver; owns no hardware. |
| **L2 — App** | `app/` | Top-level feature modes that orchestrate L0/L1/UI. |

```
src/
├── main_transmitter.cpp          # Transmitter entry point — TransmitterOperatingMode state machine + setup()/loop() only
├── main_receiver.cpp             # Receiver entry point — setup()/loop() only
├── config/
│   ├── ControlConfig.h           # Umbrella — includes the five tuning files below
│   ├── JoystickConfig.h          # Stick centers, deadzones, inversion, travel endpoints, expo/rate (X & Y)
│   ├── ServoConfig.h             # Steering-servo PWM range, smoothing, jitter deadband
│   ├── EscConfig.h               # ESC output deadband, throttle-invert
│   ├── HallConfig.h              # Hall speed-sensor pulses/rev, RPM window, debounce
│   ├── VehicleConfig.h           # Drivetrain geometry (wheel diameter, gear ratio) for km/h
│   ├── DebugConfig.h             # Per-subsystem debug flags and macros
│   ├── WifiConfig.h              # MAC addresses, ESP-NOW config
│   └── controller/               # Board pin maps (hardware wiring, kept apart from tuning)
│       ├── Esp32Pins.h           # Includes the board-specific pin file below by IS_TRANSMITTER/IS_RECEIVER
│       ├── Esp32PinsTransmitter.h # Transmitter GPIO pin assignments
│       └── Esp32PinsReceiver.h   # Receiver GPIO pin assignments
│
├── drivers/                      # L0 — ONLY code that touches a peripheral directly
│   ├── controls/                 # Joystick ADC sampling + buttons + DebugModeSwitch + InputConditioningLogic.h (tx deadzone/expo/rate, testable)
│   ├── debug/                    # Serial logging (DebugLogger) + Esp32SysInfo + PacketTrace (tx runtime trace gate)
│   ├── radio/
│   │   └── EspNowDriver.h/cpp    # ESP-NOW init, peer management, raw send/receive
│   ├── display/
│   │   ├── ScreenDriver.h/cpp    # Raw panel primitive interface (no layout)
│   │   ├── ScreenDriverFactory   # Constructs the concrete driver
│   │   └── protocols/sh1106/     # SH1106 OLED implementation
│   ├── esc/
│   │   ├── EscDriver.h/cpp       # ESC PWM output
│   │   └── EscLogic.h            # Pure speed computation — no Arduino dep, testable
│   ├── servo/
│   │   ├── ServoDriver.h/cpp     # Servo PWM output with smoothing
│   │   └── ServoLogic.h          # Pure angle computation — no Arduino dep, testable
│   └── hall/                     # Hall-effect speed sensor (receiver)
│       ├── HallSensorDriver.h/cpp # Pulse-counting speed sensor (receiver)
│       ├── HallLogic.h           # Pure RPM computation — no Arduino dep, testable
│       └── SpeedLogic.h          # Pure motor-RPM → km/h — no Arduino dep, testable (tx dashboard includes it)
│
├── comm/                         # L1 — messaging built on the radio driver
│   ├── DataTypes.h               # VehicleData / TelemetryData wire structs (shared contract)
│   ├── ChannelScanner.h/cpp      # Startup 2.4GHz congestion scan, picks the least busy channel   [both]
│   ├── ChannelAdvertiser.h/cpp   # Broadcasts/receives the chosen channel so both boards sync      [both]
│   ├── ChannelSync.h/cpp         # TX-side channel handshake orchestration                          [tx]
│   ├── TelemetryLink.h/cpp       # Inbound telemetry receipt, dedup, RTT accumulators               [tx]
│   ├── JoystickSender.h/cpp      # Outgoing joystick send-decision logic                            [tx]
│   └── VehicleCommandReceiver.h/cpp  # Inbound command receipt, servo/ESC dispatch, loss watchdog   [rx]
│
├── ui/                           # L2 — screens & menus (transmitter)
│   ├── Screen.h/cpp              # High-level draw API (DashboardData, showDashboard, …)
│   ├── ScreenUtils.h/cpp         # Shared drawing helpers
│   ├── ModeSelectMenu.h/cpp      # Top-level mode menu UI + shared "open menu" gesture
│   ├── DebugScreen.h/cpp         # Debug Info mode display
│   ├── WifiPingScreen.h/cpp      # WiFi Ping mode's stat display
│   ├── WifiScanScreen.h/cpp      # Startup channel-scan progress/result display
│   ├── SafetyScreen.h/cpp        # Safety-stop confirmation display
│   ├── SessionScreen.h/cpp       # Session Data stats + speed-over-time graph
│   └── calibration/              # On-screen calibration UI and CalibrationFlow (incl. ResponseTuningScreen feel tuner)
│
└── app/                          # L2 — top-level feature modes (transmitter)
    ├── DashboardMode.h/cpp       # Dashboard mode: connection state, link-quality indicator
    ├── DebugMode.h/cpp           # Debug Info mode
    ├── WifiPingMode.h/cpp        # WiFi Ping mode: fixed-rate ping + RTT/loss/jitter stats
    ├── SafetyMode.h/cpp          # Safety-stop mode: continuous safe-neutral e-stop
    ├── SessionMode.h/cpp         # Session Data mode: stats/graph pages
    ├── SessionTracker.h/cpp      # Accumulates session distance/time/speed from telemetry
    └── SessionStatsLogic.h       # Pure session-stats accumulation — no Arduino dep, testable

test/
├── test_esc_logic/               # Unity tests — ESC speed mapping
├── test_servo_logic/             # Unity tests — servo angle mapping
├── test_hall_logic/              # Unity tests — hall RPM computation
├── test_speed_logic/             # Unity tests — motor-RPM → km/h
├── test_input_conditioning_logic/ # Unity tests — deadzone/expo/rate conditioning
├── test_joystick_calibration_logic/ # Unity tests — calibration center/deadzone suggestion
└── test_session_stats_logic/     # Unity tests — session-stats accumulation
```

Both `main_*.cpp` files are intentionally thin: they own only the top-level mode state machine (transmitter) or the `setup()`/`loop()` wiring (receiver). Per-mode behavior lives in `app/`, ESP-NOW receipt/send-decision logic lives in `comm/`, and views live in `ui/`. When a main file starts accumulating non-trivial logic again, extract it into a new class in the layer that matches its altitude (`comm/`, `ui/`, or `app/`) rather than letting it grow. Keep `drivers/` for peripheral-touching code only — if a new class merely *uses* a driver, it belongs in `comm/`, `ui/`, or `app/`, not `drivers/`.

---

## Driver Architecture Pattern

All hardware interfaces follow a consistent driver pattern.

### Header File (`DriverName.h`)
- Use `#pragma once` for include guards
- Include minimal dependencies
- Expose only public API functions; no global mutable variables
- Function naming: `init{Driver}()`, `update{Driver}()`, `set{Value}()`

```cpp
#pragma once
#include <Arduino.h>

void initServo();
void setServoAngle(int rawX);
void updateServo(int rawX);
```

### Implementation File (`DriverName.cpp`)
- Include the driver's own header first
- All internal state is `static` (file-scoped, not visible externally)
- Use comments for non-obvious logic; avoid restating what the code already says

```cpp
#include "ServoDriver.h"
#include "ServoLogic.h"
#include <config/controller/Esp32Pins.h>
#include <ESP32Servo.h>

static Servo servo;
static int currentServoMicros = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;

void initServo() {
    servo.attach(SERVO_PIN, SERVO_MIN_MICROS, SERVO_MAX_MICROS);
    servo.writeMicroseconds(currentServoMicros);
}
```

### Logic Header (`DriverNameLogic.h`) — for testable pure computation
When a driver contains non-trivial computation (mapping, deadzone, clamping), extract it into a companion header-only file with **no Arduino or hardware dependencies**. This allows the logic to be unit-tested natively without mocking.

Rules:
- No `#include <Arduino.h>` or any hardware library
- Use only standard C++ (`<stdint.h>`, plain arithmetic)
- Implement Arduino-equivalent arithmetic inline (`map`, `constrain`) rather than calling the Arduino versions
- `inline` all functions to avoid ODR violations when included in multiple translation units
- The driver `.cpp` includes this header and calls the inline functions

```cpp
// EscLogic.h — no Arduino.h, no ESP32Servo.h
#pragma once

static const int ESC_MIN_MICROS = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;
static const int ESC_MAX_MICROS = 1900;

inline int computeEscMicros(int rawY) {
    if (rawY < 0) rawY = 0;
    if (rawY > 4095) rawY = 4095;
    int targetSpeed = (int)((long)rawY * (ESC_MAX_MICROS - ESC_MIN_MICROS) / 4095) + ESC_MIN_MICROS;
    if (rawY > 1900 && rawY < 2200) targetSpeed = ESC_NEUTRAL_MICROS;
    return targetSpeed;
}
```

---

## Configuration Patterns

### Typed constants — prefer `constexpr` over `#define`
Use `constexpr` for all typed numeric and boolean constants. Reserve `#define` for macros and conditional-compilation flags only.

```cpp
// Prefer
constexpr int SERVO_PIN = 32;
constexpr int ESC_PIN = 13;

// Avoid for typed values
#define SERVO_PIN 32   // no type safety, no scoping
```

Exception: the control-tuning config files use `#define` for constants shared between the native test environment and Arduino builds. This is acceptable because Arduino's `constexpr` support can be unreliable across toolchain versions; keep these as `#define` until verified otherwise.

### Group related configs; one file per subsystem
A config file must cover exactly one subsystem, so the relationship between its values is
obvious from proximity and the file reads as a coherent unit. When a config file starts mixing
unrelated concerns, split it by subsystem (as `ControlConfig.h` was split into
`JoystickConfig.h`, `ServoConfig.h`, `EscConfig.h`, `HallConfig.h`) and keep a thin umbrella
header (`ControlConfig.h`) that only `#include`s the parts, so existing include sites and a
future aggregate reference keep working. Keep values that are tuned or read together adjacent
within the file; when a constant in one file constrains one in another, cross-reference it by
name in a short comment rather than separating them silently. Board pin maps are hardware, not
tuning — they live under `config/controller/`, apart from the tuning files.

### No column-alignment padding — single spaces only
Use exactly one space around `=` and one space between a type and its identifier. Do **not**
pad with extra spaces to vertically align `=` signs, values, or names across adjacent lines.
Aligned columns look tidy but wreck diffs: renaming one constant reflows every neighbour.
`make check` enforces this (and `.clang-format` has alignment disabled).

```cpp
// Good — one space, no padding
static const uint8_t CHANNEL_SYNC_FALLBACK = 6;
static const int ESC_MIN_MICROS = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;

// Banned — alignment padding
static const uint8_t       CHANNEL_SYNC_FALLBACK    = 6;
static const int ESC_MIN_MICROS     = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;
```

### Avoid magic numbers
All numeric literals that encode hardware limits, protocol values, or calibration parameters must be named constants. Do not inline raw numbers in logic.

```cpp
// Good
static const int JOYSTICK_CENTER_RAW = 2048;
if (rawY > 1900 && rawY < 2200) ...  // bad — these should be named constants too
```

### Pin Configuration (`config/controller/Esp32Pins.h`)
All GPIO pin numbers as `const int`. Descriptive names: `DEVICE_PIN` format.
The canonical file is `src/config/controller/Esp32Pins.h` — the only copy; it selects the
board-specific pin file (`Esp32PinsTransmitter.h` / `Esp32PinsReceiver.h`, its siblings) by
`IS_TRANSMITTER` / `IS_RECEIVER`. Board pin maps live under `config/controller/` to keep
hardware wiring separate from the tuning configs.

### Debug Configuration (`config/DebugConfig.h`)
Boolean flags per subsystem with tagged macros. Flag the build down to zero overhead when disabled.

```cpp
#define DEBUG_JOYSTICK true
#define DEBUG_LOG_JOY(msg) if (DEBUG_JOYSTICK) { Serial.print("[JOY] "); Serial.println(msg); }
```

---

## Naming Conventions

| Element | Pattern | Example |
|---------|---------|---------|
| Classes / types | PascalCase | `VehicleData`, `CalibrationFlow` |
| Functions | camelCase | `initServo()`, `readInput()`, `computeEscMicros()` |
| Constants (`const`, `constexpr`, `#define`) | UPPER_SNAKE_CASE | `SERVO_PIN`, `ESC_NEUTRAL_MICROS` |
| Macros | UPPER_SNAKE_CASE | `DEBUG_LOG_JOY()` |
| Private / static variables | camelCase | `currentServoMicros`, `pendingPacket` |
| Enum values | PascalCase | `EscOperationMode::PositiveRotationBandBrake` |
| Header files | PascalCase | `ServoDriver.h`, `EscLogic.h` |
| Implementation files | PascalCase | `ServoDriver.cpp`, `EscDriver.cpp` |

**Function naming is camelCase only** — do not use snake_case for new functions.

**No single-character variable names** — every variable must have a descriptive name that makes its purpose obvious without reading surrounding context. `received`, `packet`, `available` are acceptable; `t`, `d`, `p`, `x` are not. Code must be explicit, clear, and stupid-simple. The one exception is the graphics-primitive coordinate convention `x`, `y`, `w`, `h` in the display-driver drawing API (`text(int x, int y, …)`, `rect(int x, int y, int w, int h)`) — these are the universal names for pixel position/size and are clearer left as-is than expanded.

**No cryptic abbreviations — spell names out in full.** This applies to every identifier: types, enums, functions, variables, and constants. A name must be understandable on its own without expanding a mental acronym. Prefer `TransmitterOperatingMode` over `TxMode`, `ReceiverCommand` over `RxCmd`, `calibrationConfig` over `calibCfg`, `messageCount` over `msgCnt`. Only these widely-understood domain terms are exempt: `Esc`, `Pwm`, `Adc`, `Rpm`, `Rtt`, `Mac`, `I2C`, `Micros` (microseconds), and the fixed-width type suffixes. When in doubt, write it out — a longer name is always preferable to an ambiguous short one.

### Enums — always use `enum class`
Scoped enums prevent name collisions and make intent explicit. Never use unscoped `enum`.

```cpp
// Good
enum class EscMotorDirection : uint8_t { PositiveRotation = 1, Reversal = 2 };

// Avoid
enum EscMotorDirection { POSITIVE_ROTATION, REVERSAL };
```

---

## Comments — code must speak for itself

Comments are a last resort, not a habit. Make the code self-explanatory first: expressive
names, named constants instead of magic numbers, small well-named helpers, and clear control
flow carry the meaning. A comment is warranted **only when the intent cannot be conveyed by the
code itself** — the *why* behind a non-obvious choice: hardware quirks, protocol/timing
constraints, ISR-safety reasons, a workaround, or a cross-file invariant.

Banned:
- Comments that restate the code (`i++; // increment i`, `// set the servo angle` above `setServoAngle(...)`).
- Narrating the obvious, or a comment on every line/field. If a name would remove the comment, rename instead of commenting.
- Verbose banners and paragraphs where one short line (or nothing) would do.

```cpp
// Bad — restates the code, adds nothing
// loop over all APs and add their score to the channel
for (int apIndex = 0; apIndex < numAPs; apIndex++) { ... }

// Good — the code is clear on its own; comment only the non-obvious reason
// scanNetworks() must run before esp_now_init(): an active ESP-NOW session returns 0 APs.
int numAPs = WiFi.scanNetworks(false, true);
```

Prefer deleting a comment and improving the code over keeping an explanatory comment. Keep the
ones that capture a *why* a future reader could not recover from the code.

---

## Essential C++ Practices

Widely-accepted conventions from the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/),
scoped to what matters on an ESP32/Arduino target. These complement the driver, naming, and
config rules above.

### Const-correctness
- Mark every variable that never changes `const` (or `constexpr` if compile-time).
- Pass non-trivial parameters by `const&`; pass small POD types (`int`, `uint8_t`, `float`, an enum) by value.
- Mark member functions that do not modify state `const` (`int getCursor() const`).

```cpp
void showDashboard(const DashboardData& data);   // large struct → const&
int computeEscMicros(int rawY);                  // small POD → by value
```

### Initialize every variable at the point of declaration
No declaration should leave a variable indeterminate — value-initialize (`{}`) if there is no
better value. This is also enforced by cppcheck (`uninitMemberVarPrivate`).

```cpp
int currentRpm = 0;
PingStats stats{};      // zero-initialize all members
bool available = false;
```

### Prefer references over pointers when the argument is never null
Use `T&` for a required argument; reserve `T*` for genuinely optional/nullable ones, and
null-check pointers before dereferencing (`if (!driver) return;`).

### Casts — never C-style
Use `static_cast` for numeric/derived conversions and `reinterpret_cast` for byte-buffer
punning (ESP-NOW send/receive). C-style `(uint8_t*)x` casts are banned (cppcheck flags them).

```cpp
esp_now_send(mac, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
int rounded = static_cast<int>(value + 0.5f);
```

### `nullptr`, not `NULL` or `0`
Always use `nullptr` for pointers.

### Override virtuals explicitly
Every override of a virtual (e.g. a concrete `ScreenDriver`) must carry `override` so the
compiler catches signature drift. Use `final` when no further overriding is intended.

### Constructors that take one argument are `explicit`
Prevents silent implicit conversions (cppcheck flags `noExplicitConstructor`).

### No dynamic allocation, exceptions, or RTTI in firmware
Embedded target: avoid `new`/`delete`, `malloc`, `std::string`, STL containers that heap-allocate,
`throw`, and `dynamic_cast` on the hot path. Prefer fixed-size buffers and stack/static storage.
This is why the ESP-NOW structs are POD and the ISR pattern copies into a static buffer.

### `static` for internal linkage; no `using namespace` in headers
File-local helpers and state are `static` (see the driver pattern). Never put `using namespace`
at file scope in a header — it leaks into every translation unit that includes it.

### Use fixed-width integer types for wire and hardware values
`uint8_t`/`int16_t`/`uint32_t` for anything that crosses ESP-NOW, maps to a register, or has a
size contract. Reserve plain `int` for local arithmetic where width is irrelevant.

---

## Data Types (ESP-NOW Structs)

Structs used for ESP-NOW transmission must be POD (plain old data) types:
- No constructors, destructors, or virtual methods
- Fixed-width fields only (`int`, `float`, `uint8_t`, etc.)
- Add a `static_assert` to guard the expected wire size

```cpp
struct VehicleData {
    int servoPos;
    int escSpeed;
};
static_assert(sizeof(VehicleData) == 8, "VehicleData wire size changed");
```

This prevents silent breakage if a field is added or the struct is refactored.

---

## ISR Safety (ESP-NOW Callbacks)

ESP-NOW receive callbacks run in an ISR context. They **must not** allocate memory, call `Serial`, or take locks. The correct pattern is to copy incoming data into a pending buffer guarded by a critical section, then process it in `loop()`.

```cpp
// In callback (ISR context) — copy only, signal flag
void onDataReceive(const uint8_t *mac, const uint8_t *data, int len) {
    portENTER_CRITICAL(&mux);
    memcpy(&pendingPacket, data, len);
    pendingPacketAvailable = true;
    portEXIT_CRITICAL(&mux);
}

// In loop() — safe to do real work
void loop() {
    portENTER_CRITICAL(&mux);
    bool available = pendingPacketAvailable;
    if (available) { ... pendingPacketAvailable = false; }
    portEXIT_CRITICAL(&mux);
    if (available) processPacket();
}
```

Never call `Serial`, heap allocations, or blocking operations from a callback.

---

## `delay()` Policy

`delay()` is banned in driver update functions and `loop()`. It is only acceptable in:
- One-time init sequences (e.g., ESC calibration pulses where timing is the feature)
- `setup()` for hardware settling time

Prefer non-blocking timing (`millis()` deltas) everywhere else.

---

## Include Path Conventions

- Own driver header first in `.cpp` files
- Project headers: `"config/Name.h"` or `"drivers/category/Name.h"` (relative to `src/`, via `-Isrc` build flag)
- External libraries: `<LibraryName.h>`

```cpp
#include "EscDriver.h"          // own header first
#include "EscLogic.h"           // companion logic header
#include <config/controller/Esp32Pins.h>   // project config
#include <ESP32Servo.h>         // external library
```

---

## Build Configuration (`platformio.ini`)

- Separate environments per firmware target: `transmitter`, `receiver`
- `native` environment for host-side unit tests — no Arduino framework
- `build_src_filter` controls which source files compile per environment
- Use `-D IS_TRANSMITTER=1` / `-D IS_RECEIVER=1` build flags for conditional compilation
- Pin library versions with `@ ^x.y.z` to keep builds reproducible

```ini
[env:transmitter]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    madhephaestus/ESP32Servo @ ^3.0.5
    olikraus/U8g2 @ ^2.35.0
build_flags = -D IS_TRANSMITTER=1 -D BAUD_RATE=115200 -Isrc
build_src_filter = +<main_transmitter.cpp> +<config/> +<drivers/> +<comm/> +<ui/> +<app/> -<drivers/esc/> -<drivers/servo/> -<drivers/hall/> -<comm/VehicleCommandReceiver.cpp>

[env:native]
platform = native
build_flags = -std=c++17 -Isrc
build_src_filter = -<*>   ; tests depend on headers only — no production .cpp needed
```

---

## Main Entry Point Pattern

### Setup phase
1. `Serial.begin(BAUD_RATE)`
2. Drivers in dependency order: screen → wifi → hardware actuators
3. Register callbacks
4. Log ready state

### Loop phase
- Process any pending data (from ISR buffer)
- Read inputs and dispatch
- Target cycle: 20–50 ms
- No blocking operations; no `delay()`

---

## Testing

### Running tests
```sh
make test   # runs all native Unity tests
```

### Native unit test environment
Tests live in `test/test_<name>/test_<name>.cpp` and run on the host via PlatformIO's Unity framework — no hardware required. The `[env:native]` environment excludes all production `.cpp` files; tests compile only against the headers they include.

### What to test
Only pure logic is testable natively. The rule is:

| Testable | Not testable without mocking |
|----------|------------------------------|
| `*Logic.h` computation functions | Anything calling `analogRead`, `Servo`, `Serial`, `millis`, ESP-NOW |
| Struct layout / `static_assert` | Driver `init*` / `update*` functions |
| Input mapping, deadzone, clamping | ISR callbacks |

### Adding a new test suite
1. Extract the pure computation into a `*Logic.h` header (no Arduino dep)
2. Create `test/test_<name>/test_<name>.cpp`
3. `#include <unity.h>` and `#include "drivers/.../NameLogic.h"`
4. Provide `setUp()`, `tearDown()`, test functions, and `main()` calling `UNITY_BEGIN/END`
5. No new entry in `platformio.ini` required — the `native` env picks up all `test/` directories automatically

---

## Adding a New Driver

1. Create `src/drivers/category/` if it doesn't exist
2. Create `DriverName.h` — `#pragma once`, public API only
3. If the driver has non-trivial computation, create `DriverNameLogic.h` — pure functions, no Arduino dep
4. Create `DriverName.cpp` — include own header first, all state `static`
5. Add `#include "drivers/category/DriverName.h"` in the relevant main file
6. Call `init{Driver}()` in `setup()`
7. Add pins to `config/controller/Esp32Pins.h` if needed
8. Add debug macros to `config/DebugConfig.h` if needed
9. Add library to `platformio.ini` if needed
10. Write tests for any `*Logic.h` functions
11. **Update `build_src_filter` in `platformio.ini`** — see rule below

### `build_src_filter` rule — MANDATORY for every new driver or class

The transmitter pulls in `+<drivers/> +<comm/> +<ui/> +<app/>`; the receiver (headless)
pulls in only `+<drivers/> +<comm/>`. So `ui/` and `app/` are transmitter-only by
construction, but any **board-specific file inside a shared layer** (`drivers/` or `comm/`)
MUST be excluded from the board that doesn't use it, or the build will fail.

| New file is used by | Action required |
|---------------------|-----------------|
| Both boards | Nothing — already included via its layer |
| Transmitter only, and it lives in `ui/` or `app/` | Nothing — the receiver never includes those layers |
| Receiver only (in `drivers/` or `comm/`) | Add `-<path>` to the **transmitter** `build_src_filter` |
| Transmitter only (in `drivers/` or `comm/`) | Add `-<path>` to the **receiver** `build_src_filter` |

Excludes can target a whole directory (`-<drivers/esc/>`) or a single file
(`-<comm/JoystickSender.cpp>`) — use a file-level exclude when a board-specific file shares a
directory with cross-board files, as in `comm/`.

**Current exclusions (update this table when adding files):**

| Path | Excluded from |
|------|--------------|
| `drivers/esc/` | transmitter |
| `drivers/servo/` | transmitter |
| `drivers/hall/` | transmitter |
| `comm/VehicleCommandReceiver.cpp` | transmitter |
| `drivers/display/` | receiver |
| `drivers/controls/DebugModeSwitch.cpp` | receiver |
| `drivers/debug/PacketTrace.cpp` | receiver |
| `comm/ChannelSync.cpp` | receiver |
| `comm/TelemetryLink.cpp` | receiver |
| `comm/JoystickSender.cpp` | receiver |
| `ui/` (whole layer) | receiver — not included at all |
| `app/` (whole layer) | receiver — not included at all |

Forgetting this step causes "not declared in this scope" or linker errors in the environment that shouldn't compile that file.

---

## Adding a New `comm/` / `ui/` / `app/` Class

Use this when the new code merely *uses* drivers rather than touching hardware — link/protocol
logic, a screen/view, or a top-level mode. **If it touches a peripheral directly, it is a
driver — follow "Adding a New Driver" instead.** Pick the layer by altitude:

| It is… | Layer | Directory |
|--------|-------|-----------|
| Messaging built on the radio (framing, handshake, dedup, send/receive, watchdog) | L1 | `comm/` |
| A screen, menu, or on-screen view | L2 | `ui/` |
| A top-level feature mode that orchestrates drivers/comm/ui | L2 | `app/` |

Steps:
1. Create `ClassName.h` — `#pragma once`, public API only; the `extern` singleton instance if used that way (see existing classes).
2. If it contains non-trivial pure computation, extract it into a companion `ClassNameLogic.h` (no Arduino dep) and unit-test it — the `*Logic.h` pattern is layer-independent.
3. Create `ClassName.cpp` — own header first, then project headers by full path (`"comm/…"`, `"ui/…"`, `"drivers/category/…"`, `"config/…"`), then external libraries. All internal state `static`.
4. Reference it from its caller (a `main_*.cpp`, an `app/` mode, or another class) by **full path** — `#include "comm/ClassName.h"`, never a bare relative include across layers.
5. Update `build_src_filter` **only if the file is board-specific and lives in a shared layer** (`comm/`): add a file-level `-<comm/ClassName.cpp>` to the board that doesn't use it. Files in `ui/` and `app/` need nothing — those layers are transmitter-only already. See the exclusions table above.

New `comm/`, `ui/`, or `app/` **directories** are rare — prefer adding files to the existing
layer. If you do add a new top-level layer, wire it into both `build_src_filter` lines and add
it to the layer table under "Project Structure".

---

## Development Workflow

```sh
make help           # show all targets
make test           # run unit tests (no hardware needed)
make check          # static analysis (cppcheck) on both firmwares
make check-tx       # static analysis on transmitter only
make check-rx       # static analysis on receiver only
make build-tx       # static-check, then compile transmitter
make build-rx       # static-check, then compile receiver
make ports          # list connected USB serial devices
make set-tx PORT=… # save transmitter port (persisted in .ports, gitignored)
make set-rx PORT=… # save receiver port
make upload-tx      # flash transmitter (uses saved port)
make upload-rx      # flash receiver
make monitor-tx     # serial monitor at transmitter baud rate
make monitor-rx     # serial monitor at receiver baud rate
```

`.ports` stores per-machine port assignments and is gitignored. One-off overrides still work: `make upload-tx PORT=/dev/cu.usbserial-xxx`.

### Static analysis

`make build-tx`, `make build-rx`, and the `upload-*` targets each run `pio check` (cppcheck,
bundled with PlatformIO — no separate install) **before** compiling, so no firmware is built
without passing the quality gate. Config lives in each env's `check_*` keys in `platformio.ini`.
Only `src/` is analyzed (library deps under `.pio/` are suppressed). The build halts on any
defect at or above `FAIL_ON` severity (default `medium`) — **no medium- or high-severity
defects may be present for a build to succeed; keep the tree clean of them.** Low-severity
findings are reported but do not block; override the floor per-invocation, e.g.
`make build-tx FAIL_ON=low`. Formatting is governed by `.clang-format` (run
`clang-format -i` or enable format-on-save in your editor).

---

## Performance Considerations

- `loop()` cycle target: 20–50 ms
- No blocking I/O in `loop()` — use ISR buffer pattern for ESP-NOW
- Average analog reads over multiple samples to reduce noise (see `Controls.cpp`)
- Servo smoothing: step toward target each cycle rather than snapping (see `ServoDriver.cpp`)
- Packet-loss watchdog: reset ESC to neutral if no packet received within timeout
