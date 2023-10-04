/* Copyright (C) 2022 - 2023 Andy Frank Schoknecht
 * Use of this source code is governed by the BSD-3-Clause
 * license, that can be found in the LICENSE file.
 */

/* This is the hui main file.
 */

#define _SCRIPTS_H_IMPL

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "common.h"
#include "config.h"
#include "hstring.h"
#include "license_str.h"
#include "menu.h"
#include "sequences.h"

long unsigned term_x_len,
	      term_y_len;

long unsigned count_menu_entries(const struct Menu *menu);

void
draw_menu(long unsigned       *stdout_y,
          const long unsigned  cursor,
	  const struct Menu   *cur_menu);

void handle_c(void (*c) (struct String *), struct String *feedback);

void
handle_cmd(const char          *cmdin,
	   int                 *active,
	   const struct Menu   *cur_menu,
	   long unsigned       *cursor,
	   struct String       *feedback,
	   unsigned long       *feedback_lines);

int handle_cmdline_opts(const int argc, const char **argv);

void
handle_key(const char          key,
           int                *active,
	   enum InputMode     *imode,
	   char               *cmdin,
           const struct Menu **cur_menu,
	   long unsigned      *cursor,
	   long unsigned      *menu_stack_len,
	   const struct Menu **menu_stack,
           struct String      *feedback,
	   long unsigned      *feedback_lines);

void
handle_key_cmdline(const char           key,
		   char                *cmdin,
		   int                 *active,
		   long unsigned       *cursor,
		   const struct Menu   *cur_menu,
		   enum InputMode      *imode,
		   struct String       *feedback,
		   unsigned long       *feedback_lines);

void
handle_sh(const char    *sh,
          struct String *feedback,
	  unsigned long *feedback_lines);

long unsigned count_menu_entries(const struct Menu *menu)
{
	long unsigned i;
	
	for (i = 0; menu->entries[i].type != ET_NONE; i++) {}
	
	return i;
}

void
draw_menu(long unsigned       *stdout_y,
          const long unsigned  cursor,
	  const struct Menu   *cur_menu)
{
	long unsigned i = 0,
	              available_rows;

	/* calc first entry to be drawn */
	available_rows = term_y_len - *stdout_y - 1;
	if (cursor > available_rows)
		i = cursor - available_rows;

	/* draw */
	for (; cur_menu->entries[i].type != ET_NONE; i++) {
		if (*stdout_y >= term_y_len)
			break;

		if (cursor == i) {
			hprintf(ENTRY_HOVER_FG, ENTRY_HOVER_BG,
				"%s%s\n", ENTRY_PREPEND,
				cur_menu->entries[i].caption);
		} else {
			hprintf(ENTRY_FG, ENTRY_BG,
				"%s%s\n", ENTRY_PREPEND,
				cur_menu->entries[i].caption);
		}

		*stdout_y += 1;
	}
}

void handle_c(void (*c) (struct String *), struct String *feedback)
{
	struct String temp = String_new();
	
	String_bleach(feedback);
	
	c(&temp);
	
	String_append(feedback, temp.str, temp.len);
	
	String_free(&temp);
}

void
handle_cmd(const char          *cmdin,
	   int                 *active,
	   const struct Menu   *cur_menu,
	   long unsigned       *cursor,
	   struct String       *feedback,
	   unsigned long       *feedback_lines)
{
	long long n;
	long unsigned menu_len;

	if (strcmp(cmdin, "q") == 0
	    || strcmp(cmdin, "quit") == 0
	    || strcmp(cmdin, "exit") == 0) {
		*active = 0;
	} else {
		n = atoll(cmdin);

		if (n > 0) {
			menu_len = count_menu_entries(cur_menu);
			if ((long unsigned) n >= menu_len)
				*cursor = menu_len - 1;
			else
				*cursor = n - 1;
		} else {
			set_feedback(feedback,
                                     feedback_lines,
				     "Command not recognised",
				     term_y_len);
			return;
		}
	}
}

int handle_cmdline_opts(const int argc, const char **argv)
{
	if (2 == argc) {
		switch (argv[1][1]) {
		case 'v':
			printf("%s: version %s\n", "hui", VERSION);
			return 1;
			break;

		case 'a':
			printf("\"%s\" aka %s %s is "
			       "licensed under the BSD-3-Clause license.\n"
			       "You should have received a copy of the license "
			       "along with this program.\n\n"
			       "The source code of this program is available "
			       "at:"
			       "\nhttps://github.com/SchokiCoder/hui\n\n"
			       "If you did not receive a copy of the license, "
			       "see below:\n\n"
			       "%s\n\n%s\n\n%s\n",
			       "House User Interface", "hui", VERSION,
			       MSG_COPYRIGHT, MSG_CLAUSES, MSG_WARRANTY);
			return 1;
			break;
		
		default:
			return 0;
		}
	}
	
	return 0;
}

void
handle_key_cmdline(const char           key,
		   char                *cmdin,
		   int                 *active,
		   long unsigned       *cursor,
		   const struct Menu   *cur_menu,
		   enum InputMode      *imode,
		   struct String       *feedback,
		   unsigned long       *feedback_lines)
{
	switch (key) {
		case '\n':
			handle_cmd(cmdin,
			           active,
				   cur_menu,
				   cursor,
				   feedback,
				   feedback_lines);
			/* fall through */
		case SIGINT:
		case SIGTSTP:
			strn_bleach(cmdin, CMD_IN_LEN);
			*imode = IM_NORMAL;
			break;

		default:
			str_add_char(cmdin, key);
			break;
	}
}

void
handle_key(const char          key,
           int                *active,
	   enum InputMode     *imode,
	   char               *cmdin,
           const struct Menu **cur_menu,
	   long unsigned      *cursor,
	   long unsigned      *menu_stack_len,
	   const struct Menu **menu_stack,
           struct String      *feedback,
	   long unsigned      *feedback_lines)
{
	if (IM_CMD == *imode) {
		handle_key_cmdline(key,
				   cmdin,
				   active,
				   cursor,
				   *cur_menu,
				   imode,
				   feedback,
				   feedback_lines);
		return;
	}
	
	switch (key) {
	case KEY_QUIT:
		*active = 0;
		break;

	case KEY_DOWN:
		if ((*cur_menu)->entries[*cursor + 1].type != ET_NONE)
			*cursor += 1;
		break;

	case KEY_UP:
		if (*cursor > 0)
			*cursor -= 1;
		break;

	case KEY_RIGHT:
		if (ET_SUBMENU == (*cur_menu)->entries[*cursor].type) {
			*cur_menu = (*cur_menu)->entries[*cursor].submenu;
			menu_stack[*menu_stack_len] = *cur_menu;
			*menu_stack_len += 1;
			feedback_lines = 0;
			*cursor = 0;
		}
		break;

	case KEY_LEFT:
		if (*menu_stack_len > 1) {
			*menu_stack_len -= 1;
			*cur_menu = menu_stack[*menu_stack_len - 1];
			feedback_lines = 0;
			*cursor = 0;
		}
		break;

	case KEY_EXEC:
		if (ET_SHELL == (*cur_menu)->entries[*cursor].type) {
			handle_sh((*cur_menu)->entries[*cursor].shell,
			          feedback,
				  feedback_lines);
		} else if (ET_C == (*cur_menu)->entries[*cursor].type) {
			handle_c((*cur_menu)->entries[*cursor].c, feedback);
		} else {
			return;
		}
		String_rtrim(feedback);
		*feedback_lines = str_lines(feedback->str, term_x_len);
		
		if (0 == feedback_lines) {
			set_feedback(feedback,
		                     feedback_lines,
				     "Executed without feedback",
				     term_y_len);
		}
		break;

	case KEY_CMD:
		*imode = IM_CMD;
		break;

	case SIGINT:
	case SIGTSTP:
		*active = 0;
		break;
	}
}

void
handle_sh(const char    *sh,
          struct String *feedback,
	  unsigned long *feedback_lines)
{
	FILE *p;
	char buf[STRING_BLOCK_SIZE];
	long unsigned buf_len;
	int read = 1;

	String_bleach(feedback);

	p = popen(sh, "r");
	if (NULL == p) {
		set_feedback(feedback,
		             feedback_lines,
			     "ERROR shell could not execute",
			     term_y_len);
		return;
	}

	while (read) {
		buf_len = fread(buf, sizeof(char), STRING_BLOCK_SIZE, p);
		if (buf_len < STRING_BLOCK_SIZE)
			read = 0;

		String_append(feedback, buf, buf_len);
	}
	pclose(p);
}

int main(const int argc, const char **argv)
{
	int                active = 1;
	char               c;
	char               cmdin[CMD_IN_LEN] = "\0";
	long unsigned      cursor = 0;
	const struct Menu *cur_menu = &MENU_MAIN;
	enum InputMode     imode = IM_NORMAL;
	struct String      feedback = String_new();
	long unsigned      feedback_lines = 0;
	const struct Menu *menu_stack[MENU_STACK_SIZE] = {[0] = &MENU_MAIN};
	long unsigned      menu_stack_len = 1;
	long unsigned      stdout_y;

	if (handle_cmdline_opts(argc, argv) != 0)
		return 0;
	
	term_get_size(&term_x_len, &term_y_len);
	term_set_raw();

	while (active) {
		printf(SEQ_CLEAR);
		stdout_y = 0;

		if (feedback_lines > 1)
			system(/*PAGER + " " + */ feedback.str); // TODO

		draw_upper(HEADER, &stdout_y, cur_menu->title, term_x_len);
		draw_menu(&stdout_y, cursor, cur_menu);
		draw_lower(cmdin, &feedback, imode, term_y_len);

		if (IM_CMD == imode)
			printf(SEQ_CRSR_SHOW);
		else
			printf(SEQ_CRSR_HIDE);

		if (read(STDIN_FILENO, &c, 1) > 0) {
			handle_key(c,
				   &active,
				   &imode,
				   cmdin,
				   &cur_menu,
				   &cursor,
				   &menu_stack_len,
				   menu_stack,
				   &feedback,
				   &feedback_lines);
		}
	}

	term_restore();

#ifndef STRING_NOT_ON_HEAP
	String_free(&feedback);
#endif

	return 0;
}
