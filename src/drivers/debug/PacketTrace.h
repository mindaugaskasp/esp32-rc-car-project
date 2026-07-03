#pragma once

// Runtime gate for per-packet hot-path tracing on the transmitter (command send
// + telemetry receipt). Defaults to the compile-time DEBUG_PACKET_TRACE in
// config/DebugConfig.h so normal builds ship with it off (zero UART overhead),
// but the transmitter Debug menu can flip it at runtime for bench testing
// without a reflash. Per-packet Serial logging is heavy — leave it off while
// actually driving; the receiver's own trace stays compile-time only.
bool isPacketTraceEnabled();
void setPacketTraceEnabled(bool enabled);
void togglePacketTrace();
