#include "PacketTrace.h"
#include "config/DebugConfig.h"

// Seeded from the compile-time default so behaviour is unchanged unless the
// Debug menu toggles it at runtime.
static bool packetTraceEnabled = DEBUG_PACKET_TRACE;

bool isPacketTraceEnabled() {
    return packetTraceEnabled;
}

void setPacketTraceEnabled(bool enabled) {
    packetTraceEnabled = enabled;
}

void togglePacketTrace() {
    packetTraceEnabled = !packetTraceEnabled;
}
