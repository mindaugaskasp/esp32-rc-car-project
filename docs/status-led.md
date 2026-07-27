# Status LED (onboard WS2812)

Serial-independent boot/health indicator on both boards. Read it first when a board "does nothing."

Onboard WS2812 on `STATUS_LED_PIN` (transmitter GPIO38, receiver GPIO48). Driver: `src/drivers/debug/StatusLedDriver.{h,cpp}`.

Both boards use the same scheme, so a glance at the pair tells you which side is unhappy.

| LED state | Meaning | Likely action |
|-----------|---------|---------------|
| **Solid white** (briefly at power-on) | `setup()` is running (booting) | Normal — wait for a blink |
| **Solid white, never blinks** | Hung in `setup()` (I2C / WiFi / ESP-NOW init) | Check serial init stage; on the transmitter suspect the screen bus |
| **Green blink** | Running, but no ESP-NOW link — no telemetry (TX) / no commands (RX) | Power on the other board; check `TRANSMITTER_MAC` / `RECEIVER_MAC` and antennas |
| **Blue blink** | Link active — packets flowing both ways | None — healthy |
| **Blue → green** | Link lost mid-drive (RX watchdog trips, ESC forced neutral) | Check range / interference / battery |
| **Off / no light** | Not powered, firmware not running, or wrong `STATUS_LED_PIN` | Check power/flash; if firmware runs but dark, fix the pin (TX 38, RX 48; try 21) |

Both blinking blue = healthy link. Both green = they can't hear each other. One blue and one green is
transient (one side notices the drop first); if it persists, the green side isn't receiving.

After a dropout the transmitter re-advertises its channel every ~5 s, so the pair re-links on its
own — no reboot needed.

## Notes

- **White → green** is the normal boot sequence. Getting *stuck on white* is the key diagnostic: the board booted and reached `setup()`, but an init step blocked.
- Colors available for adding custom stages: `Off, Red, Green, Blue, Yellow, Cyan, Magenta, White` — call `setStatusLed(StatusColor::X)` (solid) or `blinkStatusLed(color, periodMs)` (non-blocking; needs `updateStatusLed()` each loop tick).
- This board's WS2812 uses **RGB byte order** (driver already compensates for `neopixelWrite`'s GRB). If colors look wrong on a different board, revisit that swap in `StatusLedDriver.cpp`.
- Pin/type is board-specific — see `STATUS_LED_PIN` in `config/controller/Esp32PinsTransmitter.h`.
