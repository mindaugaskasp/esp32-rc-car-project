#pragma once

#define DEBUG_JOYSTICK  true
#define DEBUG_SERVO     false
#define DEBUG_ESC       true

#define DEBUG_LOG_JOY(msg) if(DEBUG_JOYSTICK) { Serial.print("[JOY]: "); Serial.println(msg); }
#define DEBUG_LOG_ESC(msg) if(DEBUG_ESC) { Serial.print("[ESC]: "); Serial.println(msg); }