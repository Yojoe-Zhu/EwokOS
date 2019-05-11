#ifndef _input_h_
#define _input_h_

#include <tstr.h>

enum KEY_CTRL {
	CTRL_A = 1, /* Move ahead */
	CTRL_B = 2, /* Move left */
	CTRL_C = 3, /* Stop */
	CTRL_D = 4, /* Delete right */
	CTRL_E = 5, /* Move end */
	CTRL_F = 6, /* Move right */
	CTRL_H = 8, /* Delete left */
	TAB = 9,    /* Same as Ctrl + I */
	CTRL_J = 10,/* Enter */
	CTRL_K = 11,/* Delete all the right */
	ENTER = 13, /* Enter, same as Ctrl + M */
	CTRL_N = 14,/* Down */
	CTRL_O = 15,/* Enter */
	CTRL_P = 16,/* Up */
	CTRL_R = 18,/* Reverse-i-search */
	CTRL_T = 20,/* Swap with the previous one */
	CTRL_U = 21,/* Delete all the left */
	CTRL_W = 23,/* Delete the left word */
	ESCAPE = 27,
	BACKSPACE = 127,
};

struct line_edit {
	char *prompt;
	unsigned int plen;

	char *buffer;
	unsigned int size;
	unsigned int pos;
	unsigned int len;
	unsigned int old_pos;
	unsigned int old_len;
	tstr_t *ts;
};

struct line_edit *line_edit_new(const char *prompt);
void line_edit_free(struct line_edit *l);
int do_line_edit(struct line_edit *l);

#endif
