/*
 * theme.c
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "log.h"
#include "theme.h"

static GString *theme_loc;
static GKeyFile *theme;

struct colour_string_t {
    char *str;
    NCURSES_COLOR_T colour;
};

static int num_colours = 9;
static struct colour_string_t colours[] = {
    { "default", -1 },
    { "white", COLOR_WHITE },
    { "green", COLOR_GREEN },
    { "red", COLOR_RED },
    { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },
    { "cyan", COLOR_CYAN },
    { "black", COLOR_BLACK },
    { "magenta", COLOR_MAGENTA },
};

// colour preferences
static struct colours_t {
        NCURSES_COLOR_T bkgnd;
        NCURSES_COLOR_T titlebar;
        NCURSES_COLOR_T statusbar;
        NCURSES_COLOR_T titlebartext;
        gboolean titlebartext_bright;
        NCURSES_COLOR_T titlebarbrackets;
        gboolean titlebarbrackets_bright;
        NCURSES_COLOR_T statusbartext;
        gboolean statusbartext_bright;
        NCURSES_COLOR_T statusbarbrackets;
        gboolean statusbarbrackets_bright;
        NCURSES_COLOR_T statusbaractive;
        gboolean statusbaractive_bright;
        NCURSES_COLOR_T statusbarnew;
        gboolean statusbarnew_bright;
        NCURSES_COLOR_T maintext;
        gboolean maintext_bright;
        NCURSES_COLOR_T splashtext;
        gboolean splashtext_bright;
        NCURSES_COLOR_T online;
        gboolean online_bright;
        NCURSES_COLOR_T away;
        gboolean away_bright;
        NCURSES_COLOR_T xa;
        gboolean xa_bright;
        NCURSES_COLOR_T dnd;
        gboolean dnd_bright;
        NCURSES_COLOR_T chat;
        gboolean chat_bright;
        NCURSES_COLOR_T offline;
        gboolean offline_bright;
        NCURSES_COLOR_T typing;
        gboolean typing_bright;
        NCURSES_COLOR_T gone;
        gboolean gone_bright;
        NCURSES_COLOR_T error;
        gboolean error_bright;
        NCURSES_COLOR_T incoming;
        gboolean incoming_bright;
        NCURSES_COLOR_T roominfo;
        gboolean roominfo_bright;
        NCURSES_COLOR_T me;
        gboolean me_bright;
        NCURSES_COLOR_T them;
        gboolean them_bright;
} colour_prefs;

static NCURSES_COLOR_T _lookup_colour(const char * const colour);
static void _set_colour(gchar *val, NCURSES_COLOR_T *pref,
    NCURSES_COLOR_T def);
static void _load_colours(void);

void
theme_load(const char * const theme_name)
{
    log_info("Loading theme");
    theme = g_key_file_new();

    if (theme_name != NULL) {
        theme_loc = g_string_new(getenv("HOME"));
        g_string_append(theme_loc, "/.profanity/themes/");
        g_string_append(theme_loc, theme_name);
        g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
            NULL);
    }

    _load_colours();
}

gboolean
theme_change(const char * const theme_name)
{
    // use default theme
    if (strcmp(theme_name, "default") == 0) {
        g_key_file_free(theme);
        theme = g_key_file_new();
        _load_colours();
        return TRUE;
    } else {
        GString *new_theme_file = g_string_new(getenv("HOME"));
        g_string_append(new_theme_file, "/.profanity/themes/");
        g_string_append(new_theme_file, theme_name);

        // no theme file found
        if (!g_file_test(new_theme_file->str, G_FILE_TEST_EXISTS)) {
            log_info("Theme does not exist \"%s\"", theme_name);
            g_string_free(new_theme_file, TRUE);
            return FALSE;

        // load from theme file
        } else {
            g_string_free(theme_loc, TRUE);
            theme_loc = new_theme_file;
            log_info("Changing theme to \"%s\"", theme_name);
            g_key_file_free(theme);
            theme = g_key_file_new();
            g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
                NULL);
            _load_colours();
            return TRUE;
        }
    }
}

void
theme_close(void)
{
    g_key_file_free(theme);
}

void
theme_init_colours(void)
{
    // main text
    init_pair(1, colour_prefs.maintext, colour_prefs.bkgnd);
    init_pair(2, colour_prefs.splashtext, colour_prefs.bkgnd);
    init_pair(3, colour_prefs.error, colour_prefs.bkgnd);
    init_pair(4, colour_prefs.incoming, colour_prefs.bkgnd);

    // title bar
    init_pair(10, colour_prefs.titlebartext, colour_prefs.titlebar);
    init_pair(11, colour_prefs.titlebarbrackets, colour_prefs.titlebar);

    // status bar
    init_pair(20, colour_prefs.statusbartext, colour_prefs.statusbar);
    init_pair(21, colour_prefs.statusbarbrackets, colour_prefs.statusbar);
    init_pair(22, colour_prefs.statusbaractive, colour_prefs.statusbar);
    init_pair(23, colour_prefs.statusbarnew, colour_prefs.statusbar);

    // chat
    init_pair(30, colour_prefs.me, colour_prefs.bkgnd);
    init_pair(31, colour_prefs.them, colour_prefs.bkgnd);

    // room chat
    init_pair(40, colour_prefs.roominfo, colour_prefs.bkgnd);

    // statuses
    init_pair(50, colour_prefs.online, colour_prefs.bkgnd);
    init_pair(51, colour_prefs.offline, colour_prefs.bkgnd);
    init_pair(52, colour_prefs.away, colour_prefs.bkgnd);
    init_pair(53, colour_prefs.chat, colour_prefs.bkgnd);
    init_pair(54, colour_prefs.dnd, colour_prefs.bkgnd);
    init_pair(55, colour_prefs.xa, colour_prefs.bkgnd);

    // states
    init_pair(60, colour_prefs.typing, colour_prefs.bkgnd);
    init_pair(61, colour_prefs.gone, colour_prefs.bkgnd);
}

static NCURSES_COLOR_T
_lookup_colour(const char * const colour)
{
    int i;
    for (i = 0; i < num_colours; i++) {
        if (strcmp(colours[i].str, colour) == 0) {
            return colours[i].colour;
        }
    }

    return -99;
}

static void
_set_colour(gchar *val, NCURSES_COLOR_T *pref,
    NCURSES_COLOR_T def)
{
    if(!val) {
        *pref = def;
    } else {
        NCURSES_COLOR_T col = _lookup_colour(val);
        if (col == -99) {
            *pref = def;
        } else {
            *pref = col;
        }
    }
}

static void
_load_colours(void)
{
    gchar *bkgnd_val = g_key_file_get_string(theme, "colours", "bkgnd", NULL);
    _set_colour(bkgnd_val, &colour_prefs.bkgnd, -1);

    gchar *titlebar_val = g_key_file_get_string(theme, "colours", "titlebar", NULL);
    _set_colour(titlebar_val, &colour_prefs.titlebar, COLOR_BLUE);

    gchar *statusbar_val = g_key_file_get_string(theme, "colours", "statusbar", NULL);
    _set_colour(statusbar_val, &colour_prefs.statusbar, COLOR_BLUE);

    gchar *titlebartext_val = g_key_file_get_string(theme, "colours", "titlebartext", NULL);
    _set_colour(titlebartext_val, &colour_prefs.titlebartext, COLOR_WHITE);
    colour_prefs.titlebartext_bright = g_key_file_get_boolean(theme, "colours", "titlebartext_bright", NULL);

    gchar *titlebarbrackets_val = g_key_file_get_string(theme, "colours", "titlebarbrackets", NULL);
    _set_colour(titlebarbrackets_val, &colour_prefs.titlebarbrackets, COLOR_CYAN);
    colour_prefs.titlebarbrackets_bright = g_key_file_get_boolean(theme, "colours", "titlebarbrackets_bright", NULL);

    gchar *statusbartext_val = g_key_file_get_string(theme, "colours", "statusbartext", NULL);
    _set_colour(statusbartext_val, &colour_prefs.statusbartext, COLOR_WHITE);
    colour_prefs.statusbartext_bright = g_key_file_get_boolean(theme, "colours", "statusbartext_bright", NULL);

    gchar *statusbarbrackets_val = g_key_file_get_string(theme, "colours", "statusbarbrackets", NULL);
    _set_colour(statusbarbrackets_val, &colour_prefs.statusbarbrackets, COLOR_CYAN);
    colour_prefs.statusbarbrackets_bright = g_key_file_get_boolean(theme, "colours", "statusbarbrackets_bright", NULL);

    gchar *statusbaractive_val = g_key_file_get_string(theme, "colours", "statusbaractive", NULL);
    _set_colour(statusbaractive_val, &colour_prefs.statusbaractive, COLOR_CYAN);
    colour_prefs.statusbaractive_bright = g_key_file_get_boolean(theme, "colours", "statusbaractive_bright", NULL);

    gchar *statusbarnew_val = g_key_file_get_string(theme, "colours", "statusbarnew", NULL);
    _set_colour(statusbarnew_val, &colour_prefs.statusbarnew, COLOR_WHITE);
    colour_prefs.statusbarnew_bright = g_key_file_get_boolean(theme, "colours", "statusbarnew_bright", NULL);

    gchar *maintext_val = g_key_file_get_string(theme, "colours", "maintext", NULL);
    _set_colour(maintext_val, &colour_prefs.maintext, COLOR_WHITE);
    colour_prefs.maintext_bright = g_key_file_get_boolean(theme, "colours", "maintext_bright", NULL);

    gchar *splashtext_val = g_key_file_get_string(theme, "colours", "splashtext", NULL);
    _set_colour(splashtext_val, &colour_prefs.splashtext, COLOR_CYAN);
    colour_prefs.splashtext_bright = g_key_file_get_boolean(theme, "colours", "splashtext_bright", NULL);

    gchar *online_val = g_key_file_get_string(theme, "colours", "online", NULL);
    _set_colour(online_val, &colour_prefs.online, COLOR_GREEN);
    colour_prefs.online_bright = g_key_file_get_boolean(theme, "colours", "online_bright", NULL);

    gchar *away_val = g_key_file_get_string(theme, "colours", "away", NULL);
    _set_colour(away_val, &colour_prefs.away, COLOR_CYAN);
    colour_prefs.away_bright = g_key_file_get_boolean(theme, "colours", "away_bright", NULL);

    gchar *chat_val = g_key_file_get_string(theme, "colours", "chat", NULL);
    _set_colour(chat_val, &colour_prefs.chat, COLOR_GREEN);
    colour_prefs.chat_bright = g_key_file_get_boolean(theme, "colours", "chat_bright", NULL);

    gchar *dnd_val = g_key_file_get_string(theme, "colours", "dnd", NULL);
    _set_colour(dnd_val, &colour_prefs.dnd, COLOR_RED);
    colour_prefs.dnd_bright = g_key_file_get_boolean(theme, "colours", "dnd_bright", NULL);

    gchar *xa_val = g_key_file_get_string(theme, "colours", "xa", NULL);
    _set_colour(xa_val, &colour_prefs.xa, COLOR_CYAN);
    colour_prefs.xa_bright = g_key_file_get_boolean(theme, "colours", "xa_bright", NULL);

    gchar *offline_val = g_key_file_get_string(theme, "colours", "offline", NULL);
    _set_colour(offline_val, &colour_prefs.offline, COLOR_RED);
    colour_prefs.offline_bright = g_key_file_get_boolean(theme, "colours", "offline_bright", NULL);

    gchar *typing_val = g_key_file_get_string(theme, "colours", "typing", NULL);
    _set_colour(typing_val, &colour_prefs.typing, COLOR_YELLOW);
    colour_prefs.typing_bright = g_key_file_get_boolean(theme, "colours", "typing_bright", NULL);

    gchar *gone_val = g_key_file_get_string(theme, "colours", "gone", NULL);
    _set_colour(gone_val, &colour_prefs.gone, COLOR_RED);
    colour_prefs.gone_bright = g_key_file_get_boolean(theme, "colours", "gone_bright", NULL);

    gchar *error_val = g_key_file_get_string(theme, "colours", "error", NULL);
    _set_colour(error_val, &colour_prefs.error, COLOR_RED);
    colour_prefs.error_bright = g_key_file_get_boolean(theme, "colours", "error_bright", NULL);

    gchar *incoming_val = g_key_file_get_string(theme, "colours", "incoming", NULL);
    _set_colour(incoming_val, &colour_prefs.incoming, COLOR_YELLOW);
    colour_prefs.incoming_bright = g_key_file_get_boolean(theme, "colours", "incoming_bright", NULL);

    gchar *roominfo_val = g_key_file_get_string(theme, "colours", "roominfo", NULL);
    _set_colour(roominfo_val, &colour_prefs.roominfo, COLOR_YELLOW);
    colour_prefs.roominfo_bright = g_key_file_get_boolean(theme, "colours", "roominfo_bright", NULL);

    gchar *me_val = g_key_file_get_string(theme, "colours", "me", NULL);
    _set_colour(me_val, &colour_prefs.me, COLOR_YELLOW);
    colour_prefs.me_bright = g_key_file_get_boolean(theme, "colours", "me_bright", NULL);

    gchar *them_val = g_key_file_get_string(theme, "colours", "them", NULL);
    _set_colour(them_val, &colour_prefs.them, COLOR_GREEN);
    colour_prefs.them_bright = g_key_file_get_boolean(theme, "colours", "them_bright", NULL);
}

int
theme_text(void)
{
    if (colour_prefs.maintext_bright) {
        return (COLOR_PAIR(1) | A_BOLD);
    } else {
        return (COLOR_PAIR(1));
    }
}

int
theme_splash(void)
{
    if (colour_prefs.splashtext_bright) {
        return (COLOR_PAIR(2) | A_BOLD);
    } else {
        return (COLOR_PAIR(2));
    }
}

int
theme_error(void)
{
    if (colour_prefs.error_bright) {
        return (COLOR_PAIR(3) | A_BOLD);
    } else {
        return (COLOR_PAIR(3));
    }
}

int
theme_incoming(void)
{
    if (colour_prefs.incoming_bright) {
        return (COLOR_PAIR(4) | A_BOLD);
    } else {
        return (COLOR_PAIR(4));
    }
}

int
theme_titlebartext(void)
{
    if (colour_prefs.titlebartext_bright) {
        return (COLOR_PAIR(10) | A_BOLD);
    } else {
        return (COLOR_PAIR(10));
    }
}

int
theme_titlebarbrackets(void)
{
    if (colour_prefs.titlebarbrackets_bright) {
        return (COLOR_PAIR(11) | A_BOLD);
    } else {
        return (COLOR_PAIR(11));
    }
}

int
theme_statusbartext(void)
{
    if (colour_prefs.statusbartext_bright) {
        return (COLOR_PAIR(20) | A_BOLD);
    } else {
        return (COLOR_PAIR(20));
    }
}

int
theme_statusbarbrackets(void)
{
    if (colour_prefs.statusbarbrackets_bright) {
        return (COLOR_PAIR(21) | A_BOLD);
    } else {
        return (COLOR_PAIR(21));
    }
}

int
theme_statusbaractive(void)
{
    if (colour_prefs.statusbaractive_bright) {
        return (COLOR_PAIR(22) | A_BOLD);
    } else {
        return (COLOR_PAIR(22));
    }
}

int
theme_statusbarnew(void)
{
    if (colour_prefs.statusbarnew_bright) {
        return (COLOR_PAIR(23) | A_BOLD);
    } else {
        return (COLOR_PAIR(23));
    }
}

int
theme_me(void)
{
    if (colour_prefs.me_bright) {
        return (COLOR_PAIR(30) | A_BOLD);
    } else {
        return (COLOR_PAIR(30));
    }
}

int
theme_them(void)
{
    if (colour_prefs.them_bright) {
        return (COLOR_PAIR(31) | A_BOLD);
    } else {
        return (COLOR_PAIR(31));
    }
}

int
theme_roominfo(void)
{
    if (colour_prefs.roominfo_bright) {
        return (COLOR_PAIR(40) | A_BOLD);
    } else {
        return (COLOR_PAIR(40));
    }
}

int
theme_online(void)
{
    if (colour_prefs.online_bright) {
        return (COLOR_PAIR(50) | A_BOLD);
    } else {
        return (COLOR_PAIR(50));
    }
}

int
theme_offline(void)
{
    if (colour_prefs.offline_bright) {
        return (COLOR_PAIR(51) | A_BOLD);
    } else {
        return (COLOR_PAIR(51));
    }
}

int
theme_away(void)
{
    if (colour_prefs.away_bright) {
        return (COLOR_PAIR(52) | A_BOLD);
    } else {
        return (COLOR_PAIR(52));
    }
}

int
theme_chat(void)
{
    if (colour_prefs.chat_bright) {
        return (COLOR_PAIR(53) | A_BOLD);
    } else {
        return (COLOR_PAIR(53));
    }
}

int
theme_dnd(void)
{
    if (colour_prefs.dnd_bright) {
        return (COLOR_PAIR(54) | A_BOLD);
    } else {
        return (COLOR_PAIR(54));
    }
}

int
theme_xa(void)
{
    if (colour_prefs.xa_bright) {
        return (COLOR_PAIR(55) | A_BOLD);
    } else {
        return (COLOR_PAIR(55));
    }
}

int
theme_typing(void)
{
    if (colour_prefs.typing_bright) {
        return (COLOR_PAIR(60) | A_BOLD);
    } else {
        return (COLOR_PAIR(60));
    }
}

int
theme_gone(void)
{
    if (colour_prefs.gone_bright) {
        return (COLOR_PAIR(61) | A_BOLD);
    } else {
        return (COLOR_PAIR(61));
    }
}
