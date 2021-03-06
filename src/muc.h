/*
 * muc.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#ifndef MUC_H
#define MUC_H

#include <glib.h>

#include "contact.h"
#include "jid.h"
#include "tools/autocomplete.h"

void muc_init(void);
void muc_close(void);
void muc_join_room(const char * const room, const char * const nick,
    const char * const password, gboolean autojoin);
void muc_leave_room(const char * const room);
gboolean muc_room_is_active(const char * const room);
gboolean muc_room_is_autojoin(const char * const room);
GList* muc_get_active_room_list(void);
char* muc_get_room_nick(const char * const room);
char* muc_get_room_password(const char * const room);

void muc_set_room_pending_nick_change(const char * const room, const char * const new_nick);
gboolean muc_is_room_pending_nick_change(const char * const room);
void muc_complete_room_nick_change(const char * const room,
    const char * const nick);
char * muc_get_old_nick(const char * const room, const char * const new_nick);

gboolean muc_add_to_roster(const char * const room, const char * const nick,
    const char * const show, const char * const status);
void muc_remove_from_roster(const char * const room, const char * const nick);
GList * muc_get_roster(const char * const room);
Autocomplete muc_get_roster_ac(const char * const room);
gboolean muc_nick_in_roster(const char * const room, const char * const nick);
PContact muc_get_participant(const char * const room, const char * const nick);
void muc_set_roster_received(const char * const room);
gboolean muc_get_roster_received(const char * const room);

void muc_set_roster_pending_nick_change(const char * const room,
    const char * const new_nick, const char * const old_nick);
char* muc_complete_roster_nick_change(const char * const room,
    const char * const nick);

void muc_add_invite(const char *room);
void muc_remove_invite(const char * const room);
gint muc_invite_count(void);
GSList* muc_get_invites(void);
gboolean muc_invites_include(const char * const room);
void muc_reset_invites_ac(void);
char* muc_find_invite(char *search_str);
void muc_clear_invites(void);

void muc_set_subject(const char * const room, const char * const subject);
char * muc_get_subject(const char * const room);
void muc_add_pending_broadcast(const char * const room, const char * const message);
GList * muc_get_pending_broadcasts(const char * const room);

void muc_autocomplete(char *input, int *size);
void muc_reset_autocomplete(const char * const room);

gboolean muc_requires_config(const char * const room);
void muc_set_requires_config(const char * const room, gboolean val);

#endif
