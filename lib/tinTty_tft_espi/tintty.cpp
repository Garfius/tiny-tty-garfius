#include <Adafruit_GFX.h>
#include "tintty.h"
#include <TFT_eSPI.h>
#include "input.h"
#include "config.h"

#define TIRQ_PIN 8
//static uint16_t my_4bit_palette[16];

/*
// @todo refactor
*/
uint16_t CHAR_WIDTH;
uint16_t CHAR_HEIGHT;
static char identifyTerminal[] = "\e[?1;0c\0";
bool tintty_cursor_key_mode_application;
//mutex_t my_mutex;
fameBufferControl myCheesyFB{UINT16_MAX, 0, UINT16_MAX, 0,  false, 0,false};
const int16_t TAB_SIZE = 4;
TFT_eSprite boldCharSpriteBuffer = TFT_eSprite(&tft);;
uint32_t owner_out =0;
void assureRefreshArea(int16_t x, int16_t y, int16_t w, int16_t h)
{
	//mutex_enter_blocking(&my_mutex);
	myCheesyFB.hasChanges = true;
	if (myCheesyFB.minX > x)
		myCheesyFB.minX = x;
	if (myCheesyFB.maxX < (x + w))
		myCheesyFB.maxX = (x + w);
	if (myCheesyFB.minY > y)
		myCheesyFB.minY = y;
	if (myCheesyFB.maxY < (y + h))
		myCheesyFB.maxY = (y + h);
	//mutex_exit(&my_mutex);
	
}
struct tintty_state
{
	// @todo consider storing cursor position as single int offset
	int16_t cursor_col, cursor_row;
	uint16_t bg_ansi_color, fg_ansi_color;
	bool bold,underline, Strikethrough; //@todo do real bold via char sprite bool=TFT_eSprite::pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y, uint16_t transparent);
	/*
		underline, Strikethrough, italic
	 */
	// cursor mode
	bool cursor_key_mode_application;

	// saved DEC cursor info (in screen coords)
	int16_t dec_saved_col, dec_saved_row, dec_saved_bg, dec_saved_fg;
	uint8_t dec_saved_g4bank;
	bool dec_saved_bold,dec_saved_underline, dec_saved_Strikethrough, dec_saved_no_wrap;

	// @todo deal with integer overflow
	int16_t top_row; // first displayed row in a logical scrollback buffer
	bool no_wrap;
	bool cursor_hidden;

	char out_char;
	int16_t out_char_col, out_char_row;
	uint8_t out_char_g4bank; // current set shift state, G0 to G3
	int16_t out_clear_before, out_clear_after;

	uint8_t g4bank_char_set[4];

	int16_t idle_cycle_count; // @todo track during blocking reads mid-command
};
static tintty_state state;
struct tintty_rendered
{
	int16_t cursor_col, cursor_row;
	int16_t top_row;
};
static tintty_rendered rendered;

void _normalize_coordinates(tintty_display *display)
{
	// Calculate safe maximum row to prevent overflow - more conservative approach
	const int16_t SAFE_MAX_ROW = (UINT16_MAX / CHAR_HEIGHT) - (display->screen_row_count * 2);

	// Only normalize if any coordinate is approaching overflow
	if (state.cursor_row > SAFE_MAX_ROW ||
		state.top_row > SAFE_MAX_ROW ||
		state.out_char_row > SAFE_MAX_ROW)
	{
		// Calculate reduction amount - keep significant history but bring values down
		int16_t reduction = max(state.top_row - display->screen_row_count, SAFE_MAX_ROW / 4);

		// Ensure we have a meaningful reduction
		if (reduction <= 0)
		{
			reduction = SAFE_MAX_ROW / 4;
		}

		// Reduce all row-based coordinates by the same amount
		state.cursor_row = max(0, state.cursor_row - reduction);
		state.top_row = max(0, state.top_row - reduction);
		state.out_char_row = max(0, state.out_char_row - reduction);

		// Adjust saved cursor position (it's stored relative to top_row)
		// dec_saved_row is already relative, so no adjustment needed

		// Force a full screen refresh since coordinates have changed
		rendered.top_row = -1;	  // Force scroll recalculation
		rendered.cursor_col = -1; // Force cursor redraw
		rendered.cursor_row = -1;

		// Mark frame buffer for complete refresh
		assureRefreshArea(0, 0, TFT_AMPLADA,  (TFT_ALSSADA - KEYBOARD_HEIGHT));
	}
}
// @todo support negative cursor_row
void _render(tintty_display *display)
{
	// expose the cursor key mode state
	tintty_cursor_key_mode_application = state.cursor_key_mode_application;
	// if scrolling, prepare the "recycled" screen area
	if (state.top_row != rendered.top_row)
	{
		// clear the new piece of screen to be recycled as blank space
		// @todo handle scroll-up
		if (static_cast<unsigned long long>(state.top_row) * CHAR_HEIGHT > INT_MAX)
			giveErrorVisibility(3, 3);
		spr.setScrollRect(0,0,TFT_AMPLADA, (TFT_ALSSADA - KEYBOARD_HEIGHT));
		spr.scroll(0, -((state.top_row - rendered.top_row) * CHAR_HEIGHT) % display->screen_height); // scroll stext 0 pixels left/right, 16 up
		assureRefreshArea(0, 0, TFT_AMPLADA, (TFT_ALSSADA - KEYBOARD_HEIGHT));
		// update displayed scroll
		// display->set_vscroll(); // @todo deal with overflow from multiplication, visibilized
		// save rendered state
		rendered.top_row = state.top_row;
	}
	_normalize_coordinates(display);
	// render character if needed - optimized version
	if (state.out_char != 0)
	{
		// Cache calculations to avoid repeated multiplications
		const uint16_t x = state.out_char_col * CHAR_WIDTH;
		const int32_t row_offset = state.out_char_row - rendered.top_row;

		// Bounds check with faster comparison
		if (state.out_char_row > (UINT16_MAX / CHAR_HEIGHT))
			giveErrorVisibility(3, 2);

		// GFXfont: y is baseline (bottom of most chars), cell top is row*CHAR_HEIGHT
		// GLCD font: y is top-left. Use (CHAR_HEIGHT-1) as baseline offset for GFXfont.
		const uint16_t y = (row_offset * CHAR_HEIGHT) % display->screen_height;

		// Pre-calculate colors to avoid array lookup during rendering
		uint16_t fg_TFT__color = state.bold ? my_4bit_palette[state.fg_ansi_color + 8] : my_4bit_palette[state.fg_ansi_color];
		const uint16_t bg_TFT__color = my_4bit_palette[state.bg_ansi_color];

		// if fg_tft_color equals bg_tft_color, create a differential of 1 within uint16_t range, because lib\TFT_eSPI\Extensions\Sprite.cpp:2007 bool fillbg = (bg != color);
		if(fg_TFT__color == bg_TFT__color){
			if(fg_TFT__color == 0xFFFF){
				fg_TFT__color -= 1;
			}else{
				fg_TFT__color += 1;
			}
		}
		
		// 
		// write to sprite buffer - batch operations when possible
		// GFXfont does NOT erase background automatically, so fill the cell first
		#ifdef usingGFXfreefont
		spr.fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, bg_TFT__color);
		#endif
		#ifdef usingGFXfreefont
		spr.setCursor(x, y + (CHAR_HEIGHT - 1));
		spr.setTextColor(fg_TFT__color);
		#else
		spr.setCursor(x, y);
		spr.setTextColor(fg_TFT__color,bg_TFT__color);
		#endif
		spr.write(state.out_char);
		
		if(state.Strikethrough){
			spr.drawFastHLine(x, y + CHAR_HEIGHT / 2, CHAR_WIDTH, fg_TFT__color);
		}
		if(state.underline){
			spr.drawFastHLine(x, y + CHAR_HEIGHT - 1, CHAR_WIDTH, fg_TFT__color);
		}
		if(state.bold){
			#ifdef usingGFXfreefont
			boldCharSpriteBuffer.fillSprite(bg_TFT__color);  // Clear temp buffer first
			boldCharSpriteBuffer.setCursor(0, CHAR_HEIGHT - 1);
			#else
			boldCharSpriteBuffer.setCursor(0, 0);
			#endif
			boldCharSpriteBuffer.setTextColor(fg_TFT__color, bg_TFT__color);
			boldCharSpriteBuffer.write(state.out_char);
			boldCharSpriteBuffer.pushToSprite(&spr, x + 1, y, bg_TFT__color);
		}


		assureRefreshArea(x, y, CHAR_WIDTH, CHAR_HEIGHT);

		// line-before
		// @todo detect when straddling edge of buffer
		if (state.out_clear_before > 0)
		{
			const int16_t line_before_chars = min(state.out_char_col, state.out_clear_before);
			const int16_t lines_before = (state.out_clear_before - line_before_chars) / display->screen_col_count;
			if (static_cast<unsigned long long>(state.out_char_row) * CHAR_HEIGHT > UINT16_MAX)
				giveErrorVisibility(4, 1);
			display->fill_rect(
				(state.out_char_col - line_before_chars) * CHAR_WIDTH,
				((state.out_char_row - rendered.top_row) * CHAR_HEIGHT) % display->screen_height, // @todo deal with overflow from multiplication
				line_before_chars * CHAR_WIDTH,
				CHAR_HEIGHT,
				my_4bit_palette[state.bg_ansi_color]); // <--@todo canviar a background, bug= neteja amb color canviat
			for (int16_t i = 0; i < lines_before; i += 1)
			{
				if (static_cast<unsigned long long>(state.out_char_row - 1 - i) * CHAR_HEIGHT > UINT16_MAX)
					giveErrorVisibility(4, 2);
				if ((state.out_char_row - 1 - i) < 0)
					giveErrorVisibility(4, 3);
				display->fill_rect(
					0,
					(((state.out_char_row - rendered.top_row) - 1 - i) * CHAR_HEIGHT) % display->screen_height, // @todo deal with overflow from multiplication
					display->screen_width,
					CHAR_HEIGHT,
					my_4bit_palette[state.bg_ansi_color]);
			}
		}

		// line-after
		// @todo detect when straddling edge of buffer
		if (state.out_clear_after > 0)
		{
			const int16_t line_after_chars = min(display->screen_col_count - 1 - state.out_char_col, state.out_clear_after);
			const int16_t lines_after = (state.out_clear_after - line_after_chars) / display->screen_col_count;
			if (static_cast<unsigned long long>(state.out_char_row) * CHAR_HEIGHT > UINT16_MAX)
				giveErrorVisibility(4, 4);
			display->fill_rect(
				(state.out_char_col + 1) * CHAR_WIDTH,
				((state.out_char_row - rendered.top_row) * CHAR_HEIGHT) % display->screen_height, // @todo deal with overflow from multiplication
				line_after_chars * CHAR_WIDTH,
				CHAR_HEIGHT,
				my_4bit_palette[state.bg_ansi_color]);// <--@todo canviar a background, bug= neteja amb color canviat

			for (int16_t i = 0; i < lines_after; i += 1)
			{
				if (static_cast<unsigned long long>(state.out_char_row + 1 + i) * CHAR_HEIGHT > UINT16_MAX)
					giveErrorVisibility(5, 1);
				display->fill_rect(
					0,
					(((state.out_char_row - rendered.top_row) + 1 + i) * CHAR_HEIGHT) % display->screen_height, // @todo deal with overflow from multiplication
					display->screen_width,
					CHAR_HEIGHT,
					my_4bit_palette[state.bg_ansi_color]);
			}
		}

		// clear for next render
		state.out_char = 0;
		state.out_clear_before = 0;
		state.out_clear_after = 0;

	}
}

void _ensure_cursor_vscroll(tintty_display *display)
{
	// move displayed window down to cover cursor
	// @todo support scrolling up as well
	if (state.cursor_row - state.top_row >= display->screen_row_count)
	{
		state.top_row = state.cursor_row - display->screen_row_count + 1;
	}

	// Check if coordinates are getting too large and normalize if needed
	if (state.cursor_row > (UINT16_MAX / CHAR_HEIGHT) - display->screen_row_count)
	{
		_normalize_coordinates(display);
	}
}

void _send_sequence(
	void (*send_char)(char ch),
	char *str)
{
	// send zero-terminated sequence character by character
	while (*str)
	{
		send_char(*str);
		str += 1;
	}
}

uint16_t _read_decimal(
	char (*peek_char)(),
	char (*read_char)())
{
	uint16_t accumulator = 0;

	while (isdigit(peek_char()))
	{
		const char digit_character = read_char();
		const uint16_t digit = digit_character - '0';
		accumulator = accumulator * 10 + digit;
	}

	return accumulator;
}

void _apply_graphic_rendition(
	uint16_t *arg_list,
	uint16_t arg_count)
{
	if (arg_count == 0)
	{
		// special case for resetting to default style
		state.bg_ansi_color = 0;
		state.fg_ansi_color = 7;
		state.underline= state.Strikethrough = state.bold = false;
		return;
	}

	// process commands
	// @todo support bold/etc for better colour support
	// @todo 39/49?
	for (uint16_t arg_index = 0; arg_index < arg_count; arg_index += 1)
	{
		const uint16_t arg_value = arg_list[arg_index];

		if (arg_value == 0)
		{
			// reset to default style
			state.bg_ansi_color = 0;
			state.fg_ansi_color = 7;
			state.underline = state.Strikethrough = state.bold = false;
		}
		else if (arg_value == 7)
		{
			state.bg_ansi_color = 7;
			state.fg_ansi_color = 0;
		}
		else if (arg_value == 9)
		{
			// strikethrough on
			state.Strikethrough = true;

		}
		else if (arg_value == 4)
		{
			// underline on
			state.underline = true;
		}
		else if (arg_value == 1)
		{
			// bold
			state.bold = true;
		}
		else if (arg_value >= 30 && arg_value <= 37)
		{
			// foreground ANSI colour
			state.fg_ansi_color = arg_value - 30;
		}
		else if (arg_value >= 40 && arg_value <= 47)
		{
			// background ANSI colour
			state.bg_ansi_color = arg_value - 40;
		}else if (arg_value >= 90 && arg_value <= 97)
		{
			// bright foreground ANSI colour
			state.fg_ansi_color = (arg_value - 90) + 8;
		}
		else if (arg_value >= 100 && arg_value <= 107)
		{
			// bright background ANSI colour
			state.bg_ansi_color = (arg_value - 100) + 8;
		}
	}
}
void saveCursor(){
	state.dec_saved_col = state.cursor_col;
	state.dec_saved_row = state.cursor_row - state.top_row; // relative to top
	state.dec_saved_bg = state.bg_ansi_color;
	state.dec_saved_fg = state.fg_ansi_color;
	state.dec_saved_g4bank = state.out_char_g4bank;
	state.dec_saved_bold = state.bold;
	state.dec_saved_Strikethrough = state.Strikethrough;
	state.dec_saved_underline = state.underline;
	state.dec_saved_no_wrap = state.no_wrap;
}
void restoreCursor(){
	state.cursor_col = state.dec_saved_col;
	state.cursor_row = state.dec_saved_row + state.top_row; // relative to top
	state.bg_ansi_color = state.dec_saved_bg;
	state.fg_ansi_color = state.dec_saved_fg;
	state.out_char_g4bank = state.dec_saved_g4bank;
	state.bold = state.dec_saved_bold;
	state.no_wrap = state.dec_saved_no_wrap;
	state.Strikethrough = state.dec_saved_Strikethrough;
	state.underline = state.dec_saved_underline;
}
void _apply_mode_setting(
	bool mode_on,
	uint16_t *arg_list,
	uint16_t arg_count)
{
	// process modes
	for (uint16_t arg_index = 0; arg_index < arg_count; arg_index += 1)
	{
		const uint16_t mode_id = arg_list[arg_index];

		switch (mode_id)
		{
		case 4:
			// insert/replace mode
			// @todo this should be off for most practical purposes anyway?
			// ... otherwise visually shifting line text is expensive
			break;

		case 20:
			// auto-LF
			// ignoring per http://vt100.net/docs/vt220-rm/chapter4.html section 4.6.6
			break;

		case 34:
			// cursor visibility
			state.cursor_hidden = !mode_on;
			break;
		}
	}
}

void _exec_escape_question_command(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char ch))
{
	// @todo support multiple mode commands
	// per http://vt100.net/docs/vt220-rm/chapter4.html section 4.6.1,
	// ANSI and DEC modes cannot mix; that is, '[?25;20;?7l' is not a valid Esc-command
	// (noting this because https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html
	// makes it look like the question mark is a prefix)
	const uint16_t mode = _read_decimal(peek_char, read_char);
	const bool mode_on = (read_char() != 'l');

	switch (mode)
	{
	case 1:
		// cursor key mode (normal/application)
		state.cursor_key_mode_application = mode_on;
		break;

	case 7:
		// auto wrap mode
		state.no_wrap = !mode_on;
		break;

	case 25:
		// cursor visibility
		state.cursor_hidden = !mode_on;
		if (!state.cursor_hidden)
		{
			assureRefreshArea(state.cursor_col * CHAR_WIDTH,((state.cursor_row - state.top_row) * CHAR_HEIGHT) % (TFT_ALSSADA - KEYBOARD_HEIGHT),CHAR_WIDTH,CHAR_HEIGHT);
		}
		break;
	default:
		break;
	}
}

void _exec_escape_bracket_command_with_args(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char ch),
	tintty_display *display,
	uint16_t *arg_list,
	uint16_t arg_count)
{
// convenient arg getter
#define ARG(index, default_value) (arg_count > index ? arg_list[index] : default_value)

	// process next character after Escape-code, bracket and any numeric arguments
	const char command_character = read_char();

	switch (command_character)
	{
	case '?':
		// question-mark commands
		_exec_escape_question_command(peek_char, read_char, send_char);
		break;

	case 'A':
		// cursor up (no scroll)
		state.cursor_row = max(state.top_row, state.cursor_row - ARG(0, 1));
		break;

	case 'B':
		// cursor down (no scroll)
		state.cursor_row = min(state.top_row + display->screen_row_count - 1, state.cursor_row + ARG(0, 1));
		break;

	case 'C':
		// cursor right (no scroll)
		state.cursor_col = min(display->screen_col_count - 1, state.cursor_col + ARG(0, 1));
		break;

	case 'D':
		// cursor left (no scroll)
		state.cursor_col = max(0, state.cursor_col - ARG(0, 1));
		break;

	case 'H':
	case 'f':
		// Direct Cursor Addressing (row;col)
		state.cursor_col = max(0, min(display->screen_col_count - 1, ARG(1, 1) - 1));
		state.cursor_row = state.top_row + max(0, min(display->screen_row_count - 1, ARG(0, 1) - 1));
		break;

	case 'J':
		// clear screen
		state.out_char = ' ';
		state.out_char_col = state.cursor_col;
		state.out_char_row = state.cursor_row;

		{
			const int16_t rel_row = state.cursor_row - state.top_row;

			state.out_clear_before = ARG(0, 0) != 0
										 ? rel_row * display->screen_col_count + state.cursor_col
										 : 0;
			state.out_clear_after = ARG(0, 0) != 1
										? (display->screen_row_count - 1 - rel_row) * display->screen_col_count + (display->screen_col_count - 1 - state.cursor_col)
										: 0;
		}
		//assureRefreshArea(0, 0, TFT_AMPLADA, (TFT_ALSSADA - KEYBOARD_HEIGHT));
		break;

	case 'K':
		// clear line
		state.out_char = ' ';
		state.out_char_col = state.cursor_col;
		state.out_char_row = state.cursor_row;

		state.out_clear_before = ARG(0, 0) != 0
									 ? state.cursor_col
									 : 0;
		state.out_clear_after = ARG(0, 0) != 1
									? display->screen_col_count - 1 - state.cursor_col
									: 0;
		/*x1 = state.cursor_col * CHAR_WIDTH;
		y1 = ((state.cursor_row - state.top_row) * CHAR_HEIGHT) % (TFT_ALSSADA - KEYBOARD_HEIGHT);
		assureRefreshArea()
		*/
		break;
	case 'm':
		// graphic rendition mode
		_apply_graphic_rendition(arg_list, arg_count);
		break;

	case 'h':
		// set mode
		_apply_mode_setting(true, arg_list, arg_count);
		break;

	case 'l':
		// unset mode
		_apply_mode_setting(false, arg_list, arg_count);
		break;
	case 's':
		saveCursor();
	break;
	case 'u':
		restoreCursor();
	break;
	case 'c':
		for (int i = 0; i < sizeof(identifyTerminal); i++) {
			bufferoUT.addChar(identifyTerminal[i]);
		}
	break;
	case 'n':
		if (ARG(0, 0) == 6) {
			char cpr_response[32];
			int16_t report_row = (state.cursor_row - state.top_row) + 1;
			int16_t report_col = state.cursor_col + 1;
			snprintf(cpr_response, sizeof(cpr_response), "\x1b[%d;%dR", report_row, report_col);
			for (int i = 0; cpr_response[i] != '\0'; i++) {
				bufferoUT.addChar(cpr_response[i]);
			}
		}
		break;
	case 'P':
		// Delete the indicated # of characters on current line, use TFT_eSprite::setScrollRect and TFT_eSprite::scroll
		spr.setScrollRect(state.cursor_col * CHAR_WIDTH, (state.cursor_row - state.top_row) * CHAR_HEIGHT % display->screen_height, (display->screen_col_count - state.cursor_col) * CHAR_WIDTH, CHAR_HEIGHT, my_4bit_palette[state.bg_ansi_color]);
		spr.scroll(-ARG(0, 1) * CHAR_WIDTH, 0);
		assureRefreshArea(state.cursor_col * CHAR_WIDTH, (state.cursor_row - state.top_row) * CHAR_HEIGHT % display->screen_height, (display->screen_col_count - state.cursor_col) * CHAR_WIDTH, CHAR_HEIGHT);
		break;
	case 't':
		if (ARG(0, 0) == 18) {
			char cpr_response[32];
			int16_t report_row = (state.cursor_row - state.top_row) + 1;
			int16_t report_col = state.cursor_col + 1;
			snprintf(cpr_response, sizeof(cpr_response), "\x1b[8;%d;%dt", display->screen_row_count , display->screen_col_count);
			for (int i = 0; cpr_response[i] != '\0'; i++) {
				bufferoUT.addChar(cpr_response[i]);
			}
		}
		break;
	default:
		break;

	}
	
}

void _exec_escape_bracket_command(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char ch),
	tintty_display *display)
{
	const uint16_t MAX_COMMAND_ARG_COUNT = 10;
	uint16_t arg_list[MAX_COMMAND_ARG_COUNT];
	uint16_t arg_count = 0;

	// start parsing arguments if any
	// (this means that '' is treated as no arguments, but '0;' is treated as two arguments, each being zero)
	// @todo ignore trailing semi-colon instead of treating it as marking an extra zero arg?
	if (isdigit(peek_char()))
	{
		// keep consuming arguments while we have space
		while (arg_count < MAX_COMMAND_ARG_COUNT)
		{
			// consume decimal number
			arg_list[arg_count] = _read_decimal(peek_char, read_char);
			arg_count += 1;

			// stop processing if next char is not separator
			if (peek_char() != ';')
			{
				break;
			}

			// consume separator before starting next argument
			read_char();
		}
	}

	_exec_escape_bracket_command_with_args(
		peek_char,
		read_char,
		send_char,
		display,
		arg_list,
		arg_count);
}

// set the characters displayed for given G0-G3 bank
void _exec_character_set(
	uint8_t g4bank_index,
	char (*read_char)())
{
	switch (read_char())
	{
	case 'A':
	case 'B':
		// normal character set (UK/US)
		state.g4bank_char_set[g4bank_index] = 0;
		break;

	case '0':
		// line-drawing
		state.g4bank_char_set[g4bank_index] = 1;
		break;

	default:
		// alternate sets are unsupported
		state.g4bank_char_set[g4bank_index] = 0;
		break;
	}
}

// @todo terminal reset
// @todo parse modes with arguments even if they are no-op
void _exec_escape_code(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char ch),
	tintty_display *display)
{
	// read next character after Escape-code
	// @todo time out?
	char esc_character = read_char();

	// @todo support for (, ), #, c, cursor save/restore
	switch (esc_character)
	{
	case '[':
		_exec_escape_bracket_command(peek_char, read_char, send_char, display);
		break;

	case 'D':
		// index (move down and possibly scroll)
		state.cursor_row += 1;
		_ensure_cursor_vscroll(display);
		break;

	case 'M':
		// reverse index (move up and possibly scroll)
		state.cursor_row -= 1;
		_ensure_cursor_vscroll(display);
		break;

	case 'E':
		// next line
		state.cursor_row += 1;
		state.cursor_col = 0;
		_ensure_cursor_vscroll(display);
		break;

	case 'Z':
		// Identify Terminal (DEC Private)
		_send_sequence(send_char, identifyTerminal); // DA response: no options
		break;

	case '7':
		/*
		save cursor
		VT100/ANSI: "\e7" (ESC 7) == xterm/modern: "\e[s" (ESC [ s)
		@todo verify that the screen-relative coordinate approach is valid
		*/
		saveCursor();
		break;

	case '8':
		// restore cursor
		restoreCursor();
		break;

	case '=':
	case '>':
		// keypad mode setting - ignoring
		break;

	case '(':
		// set G0
		_exec_character_set(0, read_char);
		break;

	case ')':
		// set G1
		_exec_character_set(1, read_char);
		break;

	case '*':
		// set G2
		_exec_character_set(2, read_char);
		break;

	case '+':
		// set G3
		_exec_character_set(3, read_char);
		break;

	default:
		// unrecognized character, silently ignore
		int aa = 3;
		break;
	}
}

void _main(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char str),
	tintty_display *display)
{
	// start in default idle state
	char initial_character = read_char();

	if (initial_character >= 0x20 && initial_character <= 0x7e)
	{
		// output displayable character
		state.out_char = initial_character;
		state.out_char_col = state.cursor_col;
		state.out_char_row = state.cursor_row;

		// update caret
		state.cursor_col += 1;

		if (state.cursor_col >= display->screen_col_count)
		{
			if (state.no_wrap)
			{
				state.cursor_col = display->screen_col_count - 1;
			}
			else
			{
				state.cursor_col = 0;
				state.cursor_row += 1;
				_ensure_cursor_vscroll(display);
			}
		}

		// reset idle state
		state.idle_cycle_count = 0;
	}
	else
	{
		// @todo bell, answer-back (0x05), delete
		switch (initial_character)
		{
		case '\a':
			// Trigger beep without forcing a display region refresh; mutex copy loop will still pick this up.
			myCheesyFB.beep = true;
			break;
		case '\n':
			// line-feed
			state.cursor_row += 1;
			_ensure_cursor_vscroll(display);
			break;

		case '\r':
			// carriage-return
			state.cursor_col = 0;
			break;

		case '\b':
			// backspace
			state.cursor_col -= 1;

			if (state.cursor_col < 0)
			{
				if (state.no_wrap)
				{
					state.cursor_col = 0;
				}
				else
				{
					state.cursor_col = display->screen_col_count - 1;
					state.cursor_row -= 1;
					_ensure_cursor_vscroll(display);
				}
			}

			break;

		case '\t':
			// tab
			{
				// @todo blank out the existing characters? not sure if that is expected
				const int16_t tab_num = state.cursor_col / TAB_SIZE;
				state.cursor_col = min(display->screen_col_count - 1, (tab_num + 1) * TAB_SIZE);
			}
			break;

		case '\e':
			// Escape-command
			_exec_escape_code(peek_char, read_char, send_char, display);
			break;

		case '\x0f':
			// Shift-In (use G0)
			// see also the fun reason why these are called this way:
			// https://en.wikipedia.org/wiki/Shift_Out_and_Shift_In_characters
			state.out_char_g4bank = 0;
			break;

		case '\x0e':
			// Shift-Out (use G1)
			state.out_char_g4bank = 1;
			break;

			// default:

			// nothing, just animate cursor
			// state.idle_cycle_count = (state.idle_cycle_count + 1) % IDLE_CYCLE_MAX;
		}
	}

	_render(display);
}
void vTaskReadSerial()
{

	bool data_received = false;

	// Read all available data in one go for better performance
	while (userTty->available() > 0)
	{
		buffer.addChar((char)userTty->read());
		data_received = true;
	}

	// Only update timestamp when new data arrives
	if (data_received)
	{
		myCheesyFB.lastRemoteDataTime = millis();
	}
	
	// Send output data if available
	if (bufferoUT.head != bufferoUT.tail)
	{
		userTty->print(bufferoUT.consumeChar());
	}
}
void refreshDisplayIfNeeded()
{
	uint32_t current_time = millis();
	bool calPosarCursor;
	uint16_t x1, y1,x2,y2,minX,minY ;
	static uint32_t beepStartTime;
	static bool isBeeping;
	static uint32_t lastCursorBlinkTime = 0;
	static bool cursorBlinkVisible = true;
	
	while (true)
	{
		yield();
		if(!input_idle())continue;
		current_time = millis();
		
		// Handle cursor blinking
		if (current_time - lastCursorBlinkTime >= TINTTY_BLINK_INTERVAL_MS)
		{
			lastCursorBlinkTime = current_time;
			cursorBlinkVisible = !cursorBlinkVisible;
			assureRefreshArea(state.cursor_col * CHAR_WIDTH, ((state.cursor_row - state.top_row) * CHAR_HEIGHT) % (TFT_ALSSADA - KEYBOARD_HEIGHT), CHAR_WIDTH, CHAR_HEIGHT);	
		}
		
		// if myCheesyFB.beep, do blinking using digitalWrite at errorLed for timeperiod of beepTimeMillis at blink speed of beepBlinkSpeedMillis on/off, use digitalread to check current state and reuse current_time to avoid delay()
		// @ todo test
		if (myCheesyFB.beep)
		{
			if (!isBeeping)
			{
				beepStartTime = current_time;
				isBeeping = true;
				digitalWrite(errorLed, HIGH);
			}
			else
			{
				if (current_time - beepStartTime >= beepTimeMillis)
				{
					// stop beeping
					isBeeping = false;
					myCheesyFB.beep = false;
					digitalWrite(errorLed, LOW);
				}
				else
				{
					// toggle LED state based on blink speed
					if (((current_time - beepStartTime) / beepBlinkSpeedMillis) % 2 == 0)
					{
						digitalWrite(errorLed, HIGH);
					}
					else
					{
						digitalWrite(errorLed, LOW);
					}
				}
			}
		}
		
		calPosarCursor = (state.cursor_row >-1) && (rendered.cursor_col != state.cursor_col || rendered.cursor_row != state.cursor_row);
		
		if ((!myCheesyFB.hasChanges || (current_time < (myCheesyFB.lastRemoteDataTime + snappyMillisLimit))) && ((!calPosarCursor || state.cursor_hidden) || (current_time < (myCheesyFB.lastRemoteDataTime + snappyMillisLimit))))
			continue;
		
		myCheesyFB.hasChanges =false;

		x1 = state.cursor_col * CHAR_WIDTH;
		y1 = ((state.cursor_row - state.top_row) * CHAR_HEIGHT) % (TFT_ALSSADA - KEYBOARD_HEIGHT);
		x2 = rendered.cursor_col * CHAR_WIDTH;
	
		// Si s'ha mogut el cursor , posa a refresh des del vell al nou
		if(calPosarCursor && !state.cursor_hidden){
			lastCursorBlinkTime =0;
			cursorBlinkVisible = false;
			y2 = ((rendered.cursor_row - rendered.top_row) * CHAR_HEIGHT) % (TFT_ALSSADA - KEYBOARD_HEIGHT);
			minX = min(x1, x2);
			minY = min(y1, y2);
			assureRefreshArea(minX, minY, (max(x1, x2) - minX)+CHAR_WIDTH, (max(y1, y2) - minY)+CHAR_HEIGHT);
		}
		
		// if the cursor state is within the bounds of myCheesyFB, set calPosarCursor to true
		if (!state.cursor_hidden && (x1 >= myCheesyFB.minX && x1 < myCheesyFB.maxX && y1 >= myCheesyFB.minY && y1 < myCheesyFB.maxY)){
			calPosarCursor = true;
		}
		
		spr.pushSprite(myCheesyFB.minX, myCheesyFB.minY, myCheesyFB.minX, myCheesyFB.minY, myCheesyFB.maxX - myCheesyFB.minX, myCheesyFB.maxY - myCheesyFB.minY);
		
		// posar cursor si cal with blinking
		if (calPosarCursor && !state.cursor_hidden)
		{
			if (cursorBlinkVisible)
			{
				// Draw cursor as white block when visible
				tft.fillRect(x1, y1, CHAR_WIDTH, CHAR_HEIGHT, my_4bit_palette[7]); // White block cursor
			}
			else
			{
				// Redraw the character at cursor position from sprite when cursor is hidden
				spr.pushSprite(x1, y1, x1, y1, CHAR_WIDTH, CHAR_HEIGHT);
			}

			rendered.cursor_col = state.cursor_col;
			rendered.cursor_row = state.cursor_row;
		}		
		myCheesyFB = fameBufferControl{UINT16_MAX, 0, UINT16_MAX, 0,  myCheesyFB.hasChanges, 0,myCheesyFB.beep};
	}
}
void tintty_run(
	char (*peek_char)(),
	char (*read_char)(),
	void (*send_char)(char str),
	tintty_display *display)
{
	/*for (int i = 0; i < 16; i++)
    {
		my_4bit_palette[i] = default_4bit_palette[i];
	}*/
	// set up initial state
	state.cursor_col = 0;
	state.cursor_row = 0;
	state.top_row = 0;
	state.no_wrap = 0;
	state.cursor_hidden = 0;
	state.bg_ansi_color = 0;
	state.fg_ansi_color = 7;
	state.bold = state.underline =  state.Strikethrough= false;
	
	state.cursor_key_mode_application = false;

	state.dec_saved_col = 0;
	state.dec_saved_row = 0;
	state.dec_saved_bg = state.bg_ansi_color;
	state.dec_saved_fg = state.fg_ansi_color;
	state.dec_saved_g4bank = 0;
	state.dec_saved_bold = state.bold;
	state.dec_saved_no_wrap = false;

	state.out_char = 0;
	state.out_char_g4bank = 0;
	state.g4bank_char_set[0] = 0;
	state.g4bank_char_set[1] = 0;
	state.g4bank_char_set[2] = 0;
	state.g4bank_char_set[3] = 0;

	rendered.cursor_col = -1;
	rendered.cursor_row = -1;

	boldCharSpriteBuffer.createSprite(CHAR_WIDTH, CHAR_HEIGHT);
	boldCharSpriteBuffer.setColorDepth(spr.getColorDepth());
	boldCharSpriteBuffer.setTextSize(spr.textsize);
	boldCharSpriteBuffer.fillSprite(TFT_BLACK);
	
	// clear screen & initial render
	display->fill_rect(0, 0, display->screen_width, display->screen_height, TFT_BLACK);
	_render(display);
	
	// clear input buffer
	userTty->flush();
	while (userTty->available() > 0)
	userTty->read();
	
	// (this works with the agetty --wait-cr option to help wait until Arduino boots)
	// send CR to indicate that the screen is ready
	send_char('\r');
	while (1)
	{
		_main(peek_char, read_char, send_char, display);
	}
}
//void tintty_idle(tintty_display *display)

