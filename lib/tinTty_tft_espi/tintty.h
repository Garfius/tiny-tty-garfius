#include "Arduino.h"
#include "utils.h"
#include "pico/multicore.h"// https://github.com/dennisma/pico-mut/blob/master/mut.cpp

/*
#define TINTTY_CHAR_WIDTH (5+1)// see .setFreeFont(GLCD);
#define TINTTY_CHAR_HEIGHT (7+1)
*/
/**
 * Frame buffer(TFT_eSprite) state control
*/
struct fameBufferControl {
    uint16_t minX,maxX,minY,maxY;
    volatile bool hasChanges;
    volatile unsigned int lastRemoteDataTime;
    bool beep;
};
/**
 * Renderer callbacks.
 */
struct tintty_display {
    int16_t screen_width, screen_height;
    int16_t screen_col_count, screen_row_count; // width and height divided by char size
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);//tft.fillRect(x, y, w, h, color);
    void (*draw_pixels)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pixels);
    void (*set_vscroll)(int16_t offset); // scroll offset for entire screen
};
/**
 * Main entry point.
 * Peek/read callbacks are expected to block until input is available;
 * while sketch is waiting for input, it should call the tintty_idle() hook
 * to allow animating cursor, etc.
 */
void tintty_run(
    char (*peek_char)(),
    char (*read_char)(),
    void (*send_char)(char str),
    tintty_display *display
);

void refreshDisplayIfNeeded();
void assureRefreshArea(int16_t x, int16_t y, int16_t w, int16_t h);
void vTaskReadSerial();

extern tintty_display ili9341_display;  // Defined in main.cpp
//extern mutex_t my_mutex;
extern uint16_t CHAR_WIDTH;
extern uint16_t CHAR_HEIGHT;
extern bool tintty_cursor_key_mode_application;

static const uint16_t my_4bit_palette[] = {
    0x0000, // black
    0xB800, // red
    0x05E0, // green
    0xBFE0, // yellow
    0x001D, // blue
    0xB81B, // magenta
    0x05FB, // cyan
    0xEF7D, // white (light gray)
    0x8410, // bright black (dark gray)
    0xF800, // bright red
    0x07E0, // bright green
    0xFFE0, // bright yellow
    0x49FF, // bright blue
    0xF81F, // bright magenta
    0x07FF, // bright cyan
    0xFFFF  // bright white
};
/* {// CUSTOMIZE !
    TFT_BLACK,    //  0
    TFT_RED,      //  1
    TFT_GREEN,    //  2
    TFT_YELLOW,   //  3

    TFT_BLUE,     //  4
    TFT_MAGENTA,  //  5
    TFT_CYAN,     //  6
    TFT_WHITE,    //  7
    
    TFT_DARKGREY,     //  8
    TFT_PINK,         //  9
    TFT_GREENYELLOW,  // 10
    TFT_GOLD,         // 11
    
    TFT_SKYBLUE,      // 12
    TFT_VIOLET,       // 13
    TFT_CYAN,         // 14
    TFT_WHITE         // 15
};*/
       
// #define TFT_BG_COLOR 0x3044 // windows terminal bg color, not used
//fameBufferControl myCheesyFB;
