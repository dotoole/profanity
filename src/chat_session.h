/*
 * chat_session.h
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

#ifndef CHAT_SESSION_H
#define CHAT_SESSION_H

#include <glib.h>

void chat_sessions_init(void);
void chat_sessions_clear(void);
void chat_session_start(const char * const recipient,
    gboolean recipient_supports);
gboolean chat_session_exists(const char * const recipient);
void chat_session_end(const char * const recipient);
gboolean chat_session_get_recipient_supports(const char * const recipient);
void chat_session_set_recipient_supports(const char * const recipient,
    gboolean recipient_supports);

void chat_session_set_composing(const char * const recipient);
void chat_session_no_activity(const char * const recipient);
gboolean chat_session_is_inactive(const char * const recipient);
void chat_session_set_active(const char * const recipient);
gboolean chat_session_is_paused(const char * const recipient);
gboolean chat_session_is_gone(const char * const recipient);
void chat_session_set_gone(const char * const recipient);
void chat_session_set_sent(const char * const recipient);
gboolean chat_session_get_sent(const char * const recipient);

#endif
