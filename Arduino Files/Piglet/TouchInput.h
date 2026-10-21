#pragma once
#include <Arduino.h>

// Touch input for the fork. The physical button is gone; an on-screen button
// in a corner of the TFT takes its place. touchButtonPressed() reports whether
// that corner is currently being touched, and it is fed into the existing
// button gesture state machine (single / double / triple / long press) in
// Piglet.ino exactly as a physical button would be.

void touchInit();            // bring up the XPT2046 (shared SPI bus)
bool touchButtonPressed();   // true while the on-screen corner button is touched
void drawTouchButton();      // draw the corner button on the full panel (gfx)
void refreshTouchButton();   // redraw it (page-specific hint line) if it has been drawn
