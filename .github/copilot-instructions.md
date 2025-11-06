this is a tft 'vt100 like' video terminal console on rp2040, which uses a on memory sprite 'spr' as a frame buffer, and 'tft' as direct tft display access.
uses 'core1' to process the receive and send serial console data, and 'core2' to output to display.
Cursor is drawn after 'spr'.


