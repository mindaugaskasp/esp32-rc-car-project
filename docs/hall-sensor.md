# Hall Sensor (RPM telemetry)

Motor RPM is measured on the receiver by a **KY-003 / 3144 unipolar Hall-effect sensor** on
`HALL_SENSOR_PIN` (**GPIO 4**) and echoed to the transmitter in every `TelemetryData` frame.
The dashboard converts it to km/h using the drivetrain geometry in `config/VehicleConfig.h`.

## Wiring

The 3144 needs **4.5 V minimum**, so power it from the 5 V rail rather than 3V3 — below that
the output is unstable and emits false pulses. Its signal then swings to ~5 V, which a divider
must drop before it reaches the ESP32's 3.3 V-tolerant GPIO:

```
        5V (BEC)
          │
        [VCC]──── sensor
        [GND]──── GND
        [ S ]──┬──── 10 kΩ ──── GPIO 4
               │
             20 kΩ
               │
              GND

5 V × 20 kΩ / (10 kΩ + 20 kΩ) = 3.33 V  →  safe for the ESP32
```

The module carries its own pull-up to VCC and an indicator LED; the output is **active-low**
(HIGH with no magnet, LOW when one is detected). The driver additionally configures the pin
`INPUT_PULLUP`, so a disconnected sensor idles HIGH rather than floating into phantom edges.

## Sensor orientation

The 3144 is **unipolar** — it responds only to a magnet's **south pole**. If the indicator LED
does not light when you bring a magnet close, flip the magnet over.

## Placement and `HALL_PULSES_PER_REV`

| Placement | `HALL_PULSES_PER_REV` | Notes |
|---|---|---|
| Near the motor rotor (bench testing) | `2` | A 4-pole rotor presents 2 south-pole faces per revolution |
| Reduction gear, single magnet | `1` | One magnet glued to the gear or wheel |
| Gear with N magnets | `N` | Space them evenly or the reading jitters |

## Noise filtering

The ESC and brushless motor put significant switching noise on the signal wire. Two layers of
filtering in `config/HallConfig.h` suppress false readings:

- **ISR debounce** (`HALL_MIN_PULSE_INTERVAL_US`, default 1000 µs) — pulses closer together
  than this are discarded in the interrupt handler. At 2 pulses/rev that ceiling is ~30 000 RPM,
  physically impossible here, so anything faster is noise.
- **Count/interval handoff** (`HALL_MIN_PULSES_FOR_RPM`, default 6) — below this many pulses in
  one `HALL_RPM_INTERVAL_MS` window the driver falls back to interval-based timing rather than
  counting, so low speeds stay readable. Raise it if idle noise leaks into the count path.

If false readings persist at idle, add a **100 nF ceramic capacitor** between the signal wire
and GND right at the sensor header.

## Verifying it works

Set `DEBUG_HALL_TO_SERIAL true` in `config/DebugConfig.h`, flash the receiver, and spin the
wheel by hand — the log is rate-limited well below the packet trace, so it will not stall the
control loop.
