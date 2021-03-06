/*
 * server_events.c
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

#include <string.h>
#include <stdlib.h>

#include "config.h"

#include "chat_session.h"
#include "log.h"
#include "muc.h"
#include "config/preferences.h"
#include "config/account.h"
#include "roster_list.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#include <libotr/proto.h>
#endif

#include "ui/ui.h"

void
handle_room_join_error(const char * const room, const char * const err)
{
    if (muc_room_is_active(room)) {
        muc_leave_room(room);
    }
    ui_handle_room_join_error(room, err);
}

// handle presence stanza errors
void
handle_presence_error(const char *from, const char * const type,
    const char *err_msg)
{
    // handle error from recipient
    if (from != NULL) {
        ui_handle_recipient_error(from, err_msg);

    // handle errors from no recipient
    } else {
        ui_handle_error(err_msg);
    }
}

// handle message stanza errors
void
handle_message_error(const char * const from, const char * const type,
    const char * const err_msg)
{
    // handle errors from no recipient
    if (from == NULL) {
        ui_handle_error(err_msg);

    // handle recipient not found ('from' contains a value and type is 'cancel')
    } else if (type != NULL && (strcmp(type, "cancel") == 0)) {
        ui_handle_recipient_not_found(from, err_msg);
        if (prefs_get_boolean(PREF_STATES) && chat_session_exists(from)) {
            chat_session_set_recipient_supports(from, FALSE);
        }

    // handle any other error from recipient
    } else {
        ui_handle_recipient_error(from, err_msg);
    }
}

void
handle_login_account_success(char *account_name)
{
    ProfAccount *account = accounts_get_account(account_name);

#ifdef HAVE_LIBOTR
    otr_on_connect(account);
#endif

    ui_handle_login_account_success(account);

    // attempt to rejoin rooms with passwords
    GList *curr = muc_get_active_room_list();
    while (curr != NULL) {
        char *password = muc_get_room_password(curr->data);
        if (password != NULL) {
            char *nick = muc_get_room_nick(curr->data);
            presence_join_room(curr->data, nick, password);
        }
        curr = g_list_next(curr);
    }
    g_list_free(curr);

    log_info("%s logged in successfully", account->jid);
    account_free(account);
}

void
handle_lost_connection(void)
{
    cons_show_error("Lost connection.");
    roster_clear();
    muc_clear_invites();
    chat_sessions_clear();
    ui_disconnected();
}

void
handle_failed_login(void)
{
    cons_show_error("Login failed.");
    log_info("Login failed");
}

void
handle_software_version_result(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os)
{
    cons_show_software_version(jid, presence, name, version, os);
}

void
handle_disco_info(const char *from, GSList *identities, GSList *features)
{
    cons_show_disco_info(from, identities, features);
}

void
handle_disco_info_error(const char * const from, const char * const error)
{
    if (from) {
        cons_show_error("Service discovery failed for %s: %s", from, error);
    } else {
        cons_show_error("Service discovery failed: %s", error);
    }
}

void
handle_room_list(GSList *rooms, const char *conference_node)
{
    cons_show_room_list(rooms, conference_node);
}

void
handle_disco_items(GSList *items, const char *jid)
{
    cons_show_disco_items(items, jid);
}

void
handle_room_invite(jabber_invite_t invite_type,
    const char * const invitor, const char * const room,
    const char * const reason)
{
    if (!muc_room_is_active(room) && !muc_invites_include(room)) {
        cons_show_room_invite(invitor, room, reason);
        muc_add_invite(room);
    }
}

void
handle_room_broadcast(const char *const room_jid,
    const char * const message)
{
    if (muc_get_roster_received(room_jid)) {
        ui_room_broadcast(room_jid, message);
    } else {
        muc_add_pending_broadcast(room_jid, message);
    }
}

void
handle_room_subject(const char * const room_jid, const char * const subject)
{
    muc_set_subject(room_jid, subject);
    if (muc_get_roster_received(room_jid)) {
        ui_room_subject(room_jid, subject);
    }
}

void
handle_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message)
{
    ui_room_history(room_jid, nick, tv_stamp, message);
}

void
handle_room_message(const char * const room_jid, const char * const nick,
    const char * const message)
{
    ui_room_message(room_jid, nick, message);

    if (prefs_get_boolean(PREF_GRLOG)) {
        Jid *jid = jid_create(jabber_get_fulljid());
        groupchat_log_chat(jid->barejid, room_jid, nick, message);
        jid_destroy(jid);
    }
}

void
handle_duck_result(const char * const result)
{
    ui_duck_result(result);
}

void
handle_incoming_message(char *from, char *message, gboolean priv)
{
#ifdef HAVE_LIBOTR
    gboolean was_decrypted = FALSE;
    char *newmessage;

    prof_otrpolicy_t policy = otr_get_policy(from);
    char *whitespace_base = strstr(message,OTRL_MESSAGE_TAG_BASE);

    if (!priv) {
        //check for OTR whitespace (opportunistic or always)
        if (policy == PROF_OTRPOLICY_OPPORTUNISTIC || policy == PROF_OTRPOLICY_ALWAYS) {
            if (whitespace_base) {
                if (strstr(message, OTRL_MESSAGE_TAG_V2) || strstr(message, OTRL_MESSAGE_TAG_V1)) {
                    // Remove whitespace pattern for proper display in UI
                    // Handle both BASE+TAGV1/2(16+8) and BASE+TAGV1+TAGV2(16+8+8)
                    int tag_length	=	24;
                    if (strstr(message, OTRL_MESSAGE_TAG_V2) && strstr(message, OTRL_MESSAGE_TAG_V1)) {
                        tag_length = 32;
                    }
                    memmove(whitespace_base, whitespace_base+tag_length, tag_length);
                    char *otr_query_message = otr_start_query();
                    cons_show("OTR Whitespace pattern detected. Attempting to start OTR session...");
                    message_send(otr_query_message, from);
                }
            }
        }
        newmessage = otr_decrypt_message(from, message, &was_decrypted);

        // internal OTR message
        if (newmessage == NULL) {
            return;
        }
    } else {
        newmessage = message;
    }
    if (policy == PROF_OTRPOLICY_ALWAYS && !was_decrypted && !whitespace_base) {
        char *otr_query_message = otr_start_query();
        cons_show("Attempting to start OTR session...");
        message_send(otr_query_message, from);
    }

    ui_incoming_msg(from, newmessage, NULL, priv);

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_fulljid();
        Jid *jidp = jid_create(jid);

        char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);
        if (!was_decrypted || (strcmp(pref_otr_log, "on") == 0)) {
            chat_log_chat(jidp->barejid, from_jid->barejid, newmessage, PROF_IN_LOG, NULL);
        } else if (strcmp(pref_otr_log, "redact") == 0) {
            chat_log_chat(jidp->barejid, from_jid->barejid, "[redacted]", PROF_IN_LOG, NULL);
        }
        prefs_free_string(pref_otr_log);

        jid_destroy(jidp);
        jid_destroy(from_jid);
    }

    if (!priv)
        otr_free_message(newmessage);
#else
    ui_incoming_msg(from, message, NULL, priv);

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_fulljid();
        Jid *jidp = jid_create(jid);
        chat_log_chat(jidp->barejid, from_jid->barejid, message, PROF_IN_LOG, NULL);
        jid_destroy(jidp);
        jid_destroy(from_jid);
    }
#endif
}

void
handle_delayed_message(char *from, char *message, GTimeVal tv_stamp,
    gboolean priv)
{
    ui_incoming_msg(from, message, &tv_stamp, priv);

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_fulljid();
        Jid *jidp = jid_create(jid);
        chat_log_chat(jidp->barejid, from_jid->barejid, message, PROF_IN_LOG, &tv_stamp);
        jid_destroy(jidp);
        jid_destroy(from_jid);
    }
}

void
handle_typing(char *from)
{
    ui_contact_typing(from);
}

void
handle_gone(const char * const from)
{
    ui_recipient_gone(from);
}

void
handle_subscription(const char *from, jabber_subscr_t type)
{
    switch (type) {
    case PRESENCE_SUBSCRIBE:
        /* TODO: auto-subscribe if needed */
        cons_show("Received authorization request from %s", from);
        log_info("Received authorization request from %s", from);
        ui_print_system_msg_from_recipient(from, "Authorization request, type '/sub allow' to accept or '/sub deny' to reject");
        if (prefs_get_boolean(PREF_NOTIFY_SUB)) {
            notify_subscription(from);
        }
        break;
    case PRESENCE_SUBSCRIBED:
        cons_show("Subscription received from %s", from);
        log_info("Subscription received from %s", from);
        ui_print_system_msg_from_recipient(from, "Subscribed");
        break;
    case PRESENCE_UNSUBSCRIBED:
        cons_show("%s deleted subscription", from);
        log_info("%s deleted subscription", from);
        ui_print_system_msg_from_recipient(from, "Unsubscribed");
        break;
    default:
        /* unknown type */
        break;
    }
}

void
handle_contact_offline(char *barejid, char *resource, char *status)
{
    gboolean updated = roster_contact_offline(barejid, resource, status);

    if (resource != NULL && updated) {
        char *show_console = prefs_get_string(PREF_STATUSES_CONSOLE);
        char *show_chat_win = prefs_get_string(PREF_STATUSES_CHAT);
        Jid *jid = jid_create_from_bare_and_resource(barejid, resource);
        PContact contact = roster_get_contact(barejid);
        if (p_contact_subscription(contact) != NULL) {
            if (strcmp(p_contact_subscription(contact), "none") != 0) {

                // show in console if "all"
                if (g_strcmp0(show_console, "all") == 0) {
                    cons_show_contact_offline(contact, resource, status);

                // show in console of "online"
                } else if (g_strcmp0(show_console, "online") == 0) {
                    cons_show_contact_offline(contact, resource, status);
                }

                // show in chat win if "all"
                if (g_strcmp0(show_chat_win, "all") == 0) {
                    ui_chat_win_contact_offline(contact, resource, status);

                // show in char win if "online" and presence online
                } else if (g_strcmp0(show_chat_win, "online") == 0) {
                    ui_chat_win_contact_offline(contact, resource, status);
                }
            }
        }
        prefs_free_string(show_console);
        prefs_free_string(show_chat_win);
        jid_destroy(jid);
    }
}

void
handle_contact_online(char *barejid, Resource *resource,
    GDateTime *last_activity)
{
    gboolean updated = roster_update_presence(barejid, resource, last_activity);

    if (updated) {
        char *show_console = prefs_get_string(PREF_STATUSES_CONSOLE);
        char *show_chat_win = prefs_get_string(PREF_STATUSES_CHAT);
        PContact contact = roster_get_contact(barejid);
        if (p_contact_subscription(contact) != NULL) {
            if (strcmp(p_contact_subscription(contact), "none") != 0) {

                // show in console if "all"
                if (g_strcmp0(show_console, "all") == 0) {
                    cons_show_contact_online(contact, resource, last_activity);

                // show in console of "online" and presence online
                } else if (g_strcmp0(show_console, "online") == 0 &&
                        resource->presence == RESOURCE_ONLINE) {
                    cons_show_contact_online(contact, resource, last_activity);

                }

                // show in chat win if "all"
                if (g_strcmp0(show_chat_win, "all") == 0) {
                    ui_chat_win_contact_online(contact, resource, last_activity);

                // show in char win if "online" and presence online
                } else if (g_strcmp0(show_chat_win, "online") == 0 &&
                        resource->presence == RESOURCE_ONLINE) {
                    ui_chat_win_contact_online(contact, resource, last_activity);
                }
            }
        }
        prefs_free_string(show_console);
        prefs_free_string(show_chat_win);
    }
}

void
handle_leave_room(const char * const room)
{
    muc_leave_room(room);
}

void
handle_room_nick_change(const char * const room,
    const char * const nick)
{
    ui_room_nick_change(room, nick);
}

void
handle_room_requires_config(const char * const room)
{
    muc_set_requires_config(room, TRUE);
    ui_room_requires_config(room);
}

void
handle_room_destroy(const char * const room)
{
    muc_leave_room(room);
    ui_room_destroyed(room);
}

void
handle_room_configure(const char * const room, DataForm *form)
{
    ui_handle_room_configuration(room, form);
}

void
handle_room_configuration_form_error(const char * const room, const char * const message)
{
    ui_handle_room_configuration_form_error(room, message);
}

void
handle_room_config_submit_result(const char * const room)
{
    ui_handle_room_config_submit_result(room);
}

void
handle_room_config_submit_result_error(const char * const room, const char * const message)
{
    ui_handle_room_config_submit_result_error(room, message);
}

void
handle_room_roster_complete(const char * const room)
{
    if (muc_room_is_autojoin(room)) {
        ui_room_join(room, FALSE);
    } else {
        ui_room_join(room, TRUE);
    }
    muc_remove_invite(room);
    muc_set_roster_received(room);
    GList *roster = muc_get_roster(room);
    ui_room_roster(room, roster, NULL);

    char *subject = muc_get_subject(room);
    if (subject != NULL) {
        ui_room_subject(room, subject);
    }

    GList *pending_broadcasts = muc_get_pending_broadcasts(room);
    if (pending_broadcasts != NULL) {
        GList *curr = pending_broadcasts;
        while (curr != NULL) {
            ui_room_broadcast(room, curr->data);
            curr = g_list_next(curr);
        }
    }
}

void
handle_room_member_presence(const char * const room,
    const char * const nick, const char * const show,
    const char * const status)
{
    gboolean updated = muc_add_to_roster(room, nick, show, status);

    if (updated) {
        char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
        if (g_strcmp0(muc_status_pref, "all") == 0) {
            ui_room_member_presence(room, nick, show, status);
        }
        prefs_free_string(muc_status_pref);
    }
}

void
handle_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    muc_add_to_roster(room, nick, show, status);

    char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
    if (g_strcmp0(muc_status_pref, "none") != 0) {
        ui_room_member_online(room, nick, show, status);
    }
    prefs_free_string(muc_status_pref);
}

void
handle_room_member_offline(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    muc_remove_from_roster(room, nick);

    char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
    if (g_strcmp0(muc_status_pref, "none") != 0) {
        ui_room_member_offline(room, nick);
    }
    prefs_free_string(muc_status_pref);
}

void
handle_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    ui_room_member_nick_change(room, old_nick, nick);
}

void
handle_group_add(const char * const contact,
    const char * const group)
{
    ui_group_added(contact, group);
}

void
handle_group_remove(const char * const contact,
    const char * const group)
{
    ui_group_removed(contact, group);
}

void
handle_roster_remove(const char * const barejid)
{
    ui_roster_remove(barejid);
}

void
handle_roster_add(const char * const barejid, const char * const name)
{
    ui_roster_add(barejid, name);
}

void
handle_autoping_cancel(void)
{
    prefs_set_autoping(0);
    cons_show_error("Server ping not supported, autoping disabled.");
}

void
handle_xmpp_stanza(const char * const msg)
{
    ui_handle_stanza(msg);
}

void
handle_ping_result(const char * const from, int millis)
{
    if (from == NULL) {
        cons_show("Ping response from server: %dms.", millis);
    } else {
        cons_show("Ping response from %s: %dms.", from, millis);
    }
}

void
handle_ping_error_result(const char * const from, const char * const error)
{
    if (error == NULL) {
        cons_show_error("Error returned from pinging %s.", from);
    } else {
        cons_show_error("Error returned from pinging %s: %s.", from, error);
    }
}
