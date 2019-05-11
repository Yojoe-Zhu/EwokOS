#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "line-edit.h"

#define KEY_LEFT	8
#define RESIZE_BLOCK	128

static void line_edit_init(struct line_edit *l);

void *memmove(void *dest, const void *src, unsigned int size) {
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	unsigned int block;

	if ((d == s) || (!size))
		return dest;

	if ((d >= s + size) || (s >= d + size)) {
		return memcpy((void *)d, (const void *)s, size);
	}

	if (d < s) {
		block = s - d;
		do {
			if (size < block)
				block = size;
			memcpy(d, s, block);
			d += block;
			s += block;
			size -= block;
		} while (size);
	} else {
		block = d - s;
		d += size;
		s += size;
		do {
			if (size < block)
				block = size;
			d -= block;
			s -= block;
			memcpy(d, s, block);
			size -= block;
		} while (size);
	}

	return dest;
}

static void _refresh_line(struct line_edit *l) {
	unsigned int offset = 0;
	unsigned int size;

	tstr_empty(l->ts);

	/* '\r' not works as expected. */
	size = l->old_pos + l->plen;
	for (unsigned int i = 0; i < size; i++)
		tstr_addc(l->ts, KEY_LEFT);
	offset += size;

	/* Out put all the contents. */
	tstr_add(l->ts, l->prompt);
	offset += l->plen;

	tstr_add(l->ts, l->buffer);
	offset += l->len;

	if (l->old_len > l->len) {
		/* Overwrite the old characters. */
		size = l->old_len - l->len;
		for (unsigned int i = 0; i < size; i++)
			tstr_addc(l->ts, ' ');
		offset += size;

		size = l->old_len - l->pos;
	} else {
		size = l->len - l->pos;
	}

	/* Move cursor to the correct postion. */
	for (unsigned int i = 0; i < size; i++)
		tstr_addc(l->ts, KEY_LEFT);
	offset += size;

	size = offset;
#if 1
	write(_stdout, tstr_cstr(l->ts), size);
#else
	for (offset = 0; offset < size; offset++)
		putch(tstr_cstr(l->ts)[offset]);
#endif
}

static void _move_home(struct line_edit *l) {
	if (l->pos > 0) {
		l->old_pos = l->pos;
		l->pos = 0;
		_refresh_line(l);
	}
}

static void _move_end(struct line_edit *l) {
	if (l->pos != l->len) {
		l->old_pos = l->pos;
		l->pos = l->len;
		_refresh_line(l);
	}
}

static void _move_right(struct line_edit *l) {
	if (l->pos < l->len) {
		l->old_pos = l->pos;
		l->pos++;
		_refresh_line(l);
	}
}

static void _move_left(struct line_edit *l) {
	if (l->pos > 0) {
		l->old_pos = l->pos;
		l->pos--;
		_refresh_line(l);
	}
}

static void _edit_delete(struct line_edit *l) {
	if ((l->len > 0) && (l->pos < l->len)) {
		memmove(l->buffer + l->pos,
			l->buffer + l->pos + 1,
			l->len - (l->pos + 1));
		l->old_len = l->len;
		l->len--;
		l->buffer[l->len] = '\0';
		_refresh_line(l);
	}
}

static void _edit_backspace(struct line_edit *l) {
	if ((l->len > 0) && (l->pos > 0)) {
		memmove(l->buffer + l->pos - 1,
			l->buffer + l->pos,
			l->len - l->pos);
		l->old_len = l->len;
		l->old_pos = l->pos;
		l->pos--;
		l->len--;
		l->buffer[l->len] = '\0';
		_refresh_line(l);
	}
}

static void _edit_append(struct line_edit *l, int c) {
	l->old_len = l->len;
	l->old_pos = l->pos;

	l->buffer[l->len] = (char)c;
	l->len++;
	l->buffer[l->len] = '\0';
	l->pos = l->len;
	_refresh_line(l);
}

static void _edit_insert(struct line_edit *l, int c) {
	if (l->len + 1 == l->size) {
		/* Resize buffer.*/
		char *nb = (char *)malloc(l->size + RESIZE_BLOCK);
		memcpy(nb, l->buffer, l->size);
		free(l->buffer);
		l->buffer = nb;
		l->size += RESIZE_BLOCK;
	}

	if (l->len == l->pos) {
		_edit_append(l, c);
		return;
	}

	l->old_len = l->len;
	l->old_pos = l->pos;

	memmove(l->buffer + l->pos + 1,
		l->buffer + l->pos,
		l->len - l->pos);
	l->buffer[l->pos] = (char)c;
	l->pos++;
	l->len++;
	l->buffer[l->len] = '\0';
	_refresh_line(l);
}

static void _edit_swap(struct line_edit *l) {
	int c;
	unsigned int pos = l->pos;

	if ((pos > 0) && (pos == l->len))
		--pos;

	if ((pos > 0) && (pos < l->len)) {
		c = l->buffer[pos - 1];
		l->buffer[pos - 1] = l->buffer[pos];
		l->buffer[pos] = c;

		l->old_pos = l->pos;
		if (l->pos < l->len)
			l->pos++;

		_refresh_line(l);
	}
}

static void _edit_del2end(struct line_edit *l) {
	if (l->pos < l->len) {
		l->buffer[l->pos] = '\0';
		l->old_len = l->len;
		l->len = l->pos;
		_refresh_line(l);
	}
}

static void _edit_del2home(struct line_edit *l) {
	if (l->pos > 0) {
		memmove(l->buffer,
				l->buffer + l->pos,
				l->len - l->pos);
		l->old_len = l->len;
		l->old_pos = l->pos;
		l->len -= l->pos;
		l->pos = 0;
		l->buffer[l->len] = '\0';
		_refresh_line(l);
	}
}

static void _edit_delete_word(struct line_edit *l) {
	unsigned int pos = l->pos;
	unsigned int deta;

	while ((pos > 0) && (l->buffer[pos - 1]) == ' ')
		pos--;

	while ((pos > 0) && (l->buffer[pos - 1]) != ' ')
		pos--;

   deta = l->pos - pos;
   memmove(l->buffer + pos,
			l->buffer + l->pos,
			l->len - l->pos);

	l->old_len = l->len;
	l->old_pos = l->pos;
	l->len -= deta;
	l->pos = pos;
	l->buffer[l->len] = '\0';
	_refresh_line(l);
}

static void _edit_clear(struct line_edit *l) {
	if (l->pos + 2 < l->size) {
		l->old_pos = l->pos;
		l->old_len = l->len;

		l->buffer[l->pos++] = '^';
		l->buffer[l->pos++] = 'C';

		/* Move to the end */
		if (l->pos > l->len)
			l->len = l->pos;
		else
			l->pos = l->len;

		_refresh_line(l);
	}
}

static int _handle_escape(struct line_edit *l) {
	int seq[3];

	seq[0] = getch();
	if (seq[0] == 0)
		return -1;

	seq[1] = getch();
	if (seq[1] == 0)
		return -1;

	if (seq[0] == 'O') {
		if (seq[1] == 'H')
			_move_home(l);
		else if (seq[1] == 'F')
			_move_end(l);
	} else if (seq[0] == '[') {
		if ((seq[1] >= '0') && (seq[1] <= '9')) {
			seq[2] = getch();
			if (seq[2] == 0)
				return -1;

			if ((seq[2] == '~') && (seq[1] == '3'))
				_edit_delete(l);

			return 0;
		}

		switch (seq[1]) {
		/* TODO: Not support up/down */
		case 'A':
		case 'B':
			break;
		case 'C':
			_move_right(l);
			break;
		case 'D':
			_move_left(l);
			break;
		case 'H':
			_move_home(l);
			break;
		case 'F':
			_move_end(l);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int _handle_key(struct line_edit *l, int c) {
	switch (c) {
	case CTRL_A:
		_move_home(l);
		break;
	case CTRL_B:
		_move_left(l);
		break;
	case CTRL_C:
		_edit_clear(l);
		return -1;
	case CTRL_D:
		_edit_delete(l);
		break;
	case CTRL_E:
		_move_end(l);
		break;
	case CTRL_F:
		_move_right(l);
		break;
	case CTRL_H:
	case BACKSPACE:
		_edit_backspace(l);
		break;
	case CTRL_K:
		_edit_del2end(l);
		break;
	/* Got a whole line. */
	case CTRL_J:
	case CTRL_O:
	case ENTER:
		/* End this loop */
		_move_end(l);
		putch(ENTER);
		return 1;

	/* TODO: Not support. */
	case TAB:
	case CTRL_R:
	/* TODO: Not support up/down */
	case CTRL_N:
	case CTRL_P:
		break;
	case CTRL_T:
		_edit_swap(l);
		break;
	case CTRL_U:
		_edit_del2home(l);
		break;
	case CTRL_W:
		_edit_delete_word(l);
		break;
	case ESCAPE:
		return _handle_escape(l);
	default:
		if (c >= ' ')
			_edit_insert(l, c);
		break;
	}

	/* Continue */
	return 0;
}

static void line_edit_init(struct line_edit *l) {
	l->len = 0;
	l->pos = 0;
	l->old_len = 0;
	l->old_pos = 0;
	l->buffer[0] = '\0';
}

struct line_edit *line_edit_new(const char *prompt) {
	struct line_edit *l;

	l = (struct line_edit *)malloc(sizeof(struct line_edit));
	l->size = RESIZE_BLOCK;
	l->buffer = (char *)malloc(l->size);

	l->plen = strlen(prompt);
	l->prompt = (char *)malloc(l->plen + 1);
	strcpy(l->prompt, prompt);

	l->ts = tstr_new("line-edit", MFS);

	line_edit_init(l);

	return l;
}

void line_edit_free(struct line_edit *l) {
	if (l) {
		tstr_free(l->ts);
		free(l->prompt);
		free(l->buffer);
		free(l);
	}
}

int do_line_edit(struct line_edit *l) {
	int ret;
	int c;

	line_edit_init(l);
	_refresh_line(l);

	while (1) {
		c = getch();
		if (c == 0)
			return -1;

		ret = _handle_key(l, c);
		if (ret > 0)
			break;

		if (ret < 0) {
			putch(ENTER);
			line_edit_init(l);
			_refresh_line(l);
		}
	}

	return 0;
}
