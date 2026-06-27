# Code Patterns & Project Guidelines

This document establishes the coding patterns and architectural standards for the ESP32 RC Car project. Follow these guidelines to maintain consistency and modularity.

## Project Structure

```
src/
├── main_transmitter.cpp          # Transmitter entry point
├── main_receiver.cpp             # Receiver entry point
├── config/
│   ├── ControlConfig.h           # Joystick calibration, servo/ESC tuning constants
│   ├── DebugConfig.h             # Per-subsystem debug flags and macros
│   ├── Esp32Pins.h               # GPIO pin assignments
│   └── WifiConfig.h              # MAC addresses, ESP-NOW config
└── drivers/
    ├── controls/                 # Joystick ADC sampling
    ├── debug/                    # Serial + screen logging (DebugLogger)
    ├── esc/
    │   ├── EscDriver.h/cpp       # ESC PWM output
    │   ├── EscLogic.h            # Pure speed computation — no Arduino dep, testable
    │   └── EscCalibration.h/cpp  # Throttle-range calibration sequence
    ├── screen/
    │   ├── Screen.h/cpp          # Display abstraction
    │   ├── ScreenDriver.h/cpp    # Driver interface
    │   ├── ScreenDriverFactory   # Constructs the concrete driver
    │   ├── protocols/sh1106/     # SH1106 OLED implementation
    │   └── calibration/          # On-screen calibration UI and CalibrationFlow
    ├── servo/
    │   ├── ServoDriver.h/cpp     # Servo PWM output with smoothing
    │   └── ServoLogic.h          # Pure angle computation — no Arduino dep, testable
    └── wifi/
        ├── EspNowDriver.h/cpp    # ESP-NOW init, peer management, send/receive
        └── DataTypes.h           # VehicleData / TelemetryData structs

test/
├── test_esc_logic/               # Unity tests — ESC speed mapping
└── test_servo_logic/             # Unity tests — servo angle mapping
```

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
#include <config/Esp32Pins.h>
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

static const int ESC_MIN_MICROS     = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;
static const int ESC_MAX_MICROS     = 1900;

inline int computeEscMicros(int rawY) {
    if (rawY < 0)    rawY = 0;
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
constexpr int ESC_PIN   = 13;

// Avoid for typed values
#define SERVO_PIN 32   // no type safety, no scoping
```

Exception: `ControlConfig.h` uses `#define` for constants shared between the native test environment and Arduino builds. This is acceptable because Arduino's `constexpr` support can be unreliable across toolchain versions; keep these as `#define` until verified otherwise.

### Avoid magic numbers
All numeric literals that encode hardware limits, protocol values, or calibration parameters must be named constants. Do not inline raw numbers in logic.

```cpp
// Good
static const int JOYSTICK_CENTER_RAW = 2048;
if (rawY > 1900 && rawY < 2200) ...  // bad — these should be named constants too
```

### Pin Configuration (`config/Esp32Pins.h`)
All GPIO pin numbers as `const int`. Descriptive names: `DEVICE_PIN` format.

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
| Implementation files | PascalCase | `ServoDriver.cpp`, `EscCalibration.cpp` |

**Function naming is camelCase only** — do not use snake_case for new functions.

### Enums — always use `enum class`
Scoped enums prevent name collisions and make intent explicit. Never use unscoped `enum`.

```cpp
// Good
enum class EscMotorDirection : uint8_t { PositiveRotation = 1, Reversal = 2 };

// Avoid
enum EscMotorDirection { POSITIVE_ROTATION, REVERSAL };
```

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
#include <config/Esp32Pins.h>   // project config
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
build_src_filter = +<main_transmitter.cpp> +<drivers/> +<config/> -<drivers/esc/> -<drivers/servo/>

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
7. Add pins to `config/Esp32Pins.h` if needed
8. Add debug macros to `config/DebugConfig.h` if needed
9. Add library to `platformio.ini` if needed
10. Write tests for any `*Logic.h` functions

---

## Development Workflow

```sh
make help           # show all targets
make test           # run unit tests (no hardware needed)
make build-tx       # compile transmitter
make build-rx       # compile receiver
make ports          # list connected USB serial devices
make set-tx PORT=… # save transmitter port (persisted in .ports, gitignored)
make set-rx PORT=… # save receiver port
make upload-tx      # flash transmitter (uses saved port)
make upload-rx      # flash receiver
make monitor-tx     # serial monitor at transmitter baud rate
make monitor-rx     # serial monitor at receiver baud rate
```

`.ports` stores per-machine port assignments and is gitignored. One-off overrides still work: `make upload-tx PORT=/dev/cu.usbserial-xxx`.

---

## Performance Considerations

- `loop()` cycle target: 20–50 ms
- No blocking I/O in `loop()` — use ISR buffer pattern for ESP-NOW
- Average analog reads over multiple samples to reduce noise (see `Controls.cpp`)
- Servo smoothing: step toward target each cycle rather than snapping (see `ServoDriver.cpp`)
- Packet-loss watchdog: reset ESC to neutral if no packet received within timeout
