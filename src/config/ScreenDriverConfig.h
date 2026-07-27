#pragma once

// Selects which OLED controller driver the transmitter builds against. Set
// ACTIVE_SCREEN_PROTOCOL to match the physical panel: the modules look
// identical but their controllers need different U8g2 init sequences (an
// SSD1306 init sequence leaves an SSD1309 blank, and vice versa), so the
// controller cannot be picked generically at runtime. Only the selected
// driver's ~1 KB framebuffer is compiled in — the others are guarded out.
//
// #define (not constexpr/enum) because this is a conditional-compilation flag:
// the factory and each protocol .cpp select on it with #if.
#define SCREEN_PROTOCOL_SSD1306 1
#define SCREEN_PROTOCOL_SSD1309 2
#define SCREEN_PROTOCOL_SH1106 3

#define ACTIVE_SCREEN_PROTOCOL SCREEN_PROTOCOL_SSD1309

// UI font scale for the active panel. All panels are 128x64 pixels, but they
// differ physically (0.96"/1.3"/2.42"), so the same glyphs look bulkier on a
// larger screen. This shifts every ScreenFont tier (Tiny<Small<Medium<Large)
// by N steps when resolving to a concrete U8g2 font, clamped to the ends:
//   0  = default sizes
//  -1  = one tier smaller everywhere (denser — good for the 2.42" SSD1309)
//  +1  = one tier larger everywhere
// Layout coordinates are unaffected (text width is measured post-scale).
constexpr int SCREEN_FONT_SCALE_OFFSET = 0;
