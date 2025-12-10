this is a tft 'vt100 like' video terminal console on rp2040 using maxgerhardt/earlephilhower Arduino framework, and TFT_eSPI library.

Main structure: uses a on memory sprite 'spr' as a frame buffer, and 'tft' as direct tft display access, on screen keyboard for input.

The vt100 data input from (Serial mapped to 'Stream *userTty') is buffered at (buffer object, instance of CharBuffer) and parsed by tintty.cpp functions to spr. 
fameBufferControl myCheesyFB controls the spr modified area to be redrawn to tft, mantained by tintty.cpp functions, which shall take in account the cursor movement position and state.

Uses 'core1' to process the received serial data via called via: setup parseToBuffer tintty_run vTaskReadSerial

Uses 'core2' takes care of the outputs to user (display(spr to tft)), cursor,beep led via: loop1 refreshDisplayIfNeeded.  Also takes care of the on display keyboard via input_idle at refreshDisplayIfNeeded.

On display keyboard at (input.h and input.cpp)
Serial input parser to sprite and draw sprite to screen at (tintty.h and tintty.cpp)
Configuration at (config.h)
Touch screen calibration, buffers and others at (utils.h and utils.cpp)



