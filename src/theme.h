/*
 * theme.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef THEME_H
#define THEME_H

#include "config.h"

#include <glib.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

void theme_load(const char * const theme_name);
void theme_init_colours(void);
gboolean theme_change(const char * const theme_name);
void theme_close(void);

int theme_text(void);
int theme_splash(void);
int theme_error(void);
int theme_incoming(void);
int theme_titlebartext(void);
int theme_titlebarbrackets(void);
int theme_statusbartext(void);
int theme_statusbarbrackets(void);
int theme_statusbaractive(void);
int theme_statusbarnew(void);
int theme_me(void);
int theme_them(void);
int theme_roominfo(void);
int theme_online(void);
int theme_offline(void);
int theme_away(void);
int theme_chat(void);
int theme_dnd(void);
int theme_xa(void);
int theme_typing(void);
int theme_gone(void);

#endif
