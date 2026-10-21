#pragma once
#include <Arduino.h>

// Status strip for the fork: the area across the bottom of the landscape panel,
// below the 2x page and split from it by a separator line. The NEXT button
// takes the right end of the strip. Drawn directly on the panel (gfx), outside
// the page canvas, so canvas flushes never touch it. On the Navigation page it
// shows the GPS state; elsewhere it is blank for now.

void statusStripInit();     // draw the separator line; call once after TFT init
void statusStripClear();    // blank the strip (e.g. on leaving the Navigation page)
void statusStripDrawGps();  // GPS state: no data / garbled / searching / fix
