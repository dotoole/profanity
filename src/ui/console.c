/*
 * console.c
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

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "command/command.h"
#include "common.h"
#include "log.h"
#include "muc.h"
#include "roster_list.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "ui/window.h"
#include "ui/windows.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

static void _cons_splash_logo(void);
void _show_roster_contacts(GSList *list, gboolean show_groups);

static void
_cons_show_time(void)
{
    ProfWin *console = wins_get_console();
    win_save_print(console, '-', NULL, NO_EOL, 0, "", "");
}

static void
_cons_show_word(const char * const word)
{
    ProfWin *console = wins_get_console();
    win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", word);
}

static void
_cons_debug(const char * const msg, ...)
{
    ProfWin *console = wins_get_console();
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        va_list arg;
        va_start(arg, msg);
        GString *fmt_msg = g_string_new(NULL);
        g_string_vprintf(fmt_msg, msg, arg);
        win_save_println(console, fmt_msg->str);
        g_string_free(fmt_msg, TRUE);
        va_end(arg);
    }
}

static void
_cons_show(const char * const msg, ...)
{
    ProfWin *console = wins_get_console();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_save_println(console, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

static void
_cons_show_error(const char * const msg, ...)
{
    ProfWin *console = wins_get_console();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_save_print(console, '-', NULL, 0, COLOUR_ERROR, "", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    cons_alert();
}

static void
_cons_show_typing(const char * const barejid)
{
    ProfWin *console = wins_get_console();
    const char * display_usr = NULL;
    PContact contact = roster_get_contact(barejid);
    if (contact != NULL) {
        if (p_contact_name(contact) != NULL) {
            display_usr = p_contact_name(contact);
        } else {
            display_usr = barejid;
        }
    } else {
        display_usr = barejid;
    }

    win_save_vprint(console, '-', NULL, 0, COLOUR_TYPING, "", "!! %s is typing a message...", display_usr);
    cons_alert();
}

static void
_cons_show_incoming_message(const char * const short_from, const int win_index)
{
    ProfWin *console = wins_get_console();

    int ui_index = win_index;
    if (ui_index == 10) {
        ui_index = 0;
    }
    win_save_vprint(console, '-', NULL, 0, COLOUR_INCOMING, "", "<< incoming from %s (%d)", short_from, ui_index);

    cons_alert();
}

static void
_cons_about(void)
{
    ProfWin *console = wins_get_console();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (prefs_get_boolean(PREF_SPLASH)) {
        _cons_splash_logo();
    } else {

        if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
            win_save_vprint(console, '-', NULL, 0, 0, "", "Welcome to Profanity, version %sdev.%s.%s", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            win_save_vprint(console, '-', NULL, 0, 0, "", "Welcome to Profanity, version %sdev", PACKAGE_VERSION);
#endif
        } else {
            win_save_vprint(console, '-', NULL, 0, 0, "", "Welcome to Profanity, version %s", PACKAGE_VERSION);
        }
    }

    win_save_vprint(console, '-', NULL, 0, 0, "", "Copyright (C) 2012 - 2014 James Booth <%s>.", PACKAGE_BUGREPORT);
    win_save_println(console, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>");
    win_save_println(console, "");
    win_save_println(console, "This is free software; you are free to change and redistribute it.");
    win_save_println(console, "There is NO WARRANTY, to the extent permitted by law.");
    win_save_println(console, "");
    win_save_println(console, "Type '/help' to show complete help.");
    win_save_println(console, "");

    if (prefs_get_boolean(PREF_VERCHECK)) {
        cons_check_version(FALSE);
    }

    pnoutrefresh(console->win, 0, 0, 1, 0, rows-3, cols-1);

    cons_alert();
}

static void
_cons_check_version(gboolean not_available_msg)
{
    ProfWin *console = wins_get_console();
    char *latest_release = release_get_latest();

    if (latest_release != NULL) {
        gboolean relase_valid = g_regex_match_simple("^\\d+\\.\\d+\\.\\d+$", latest_release, 0, 0);

        if (relase_valid) {
            if (release_is_new(latest_release)) {
                win_save_vprint(console, '-', NULL, 0, 0, "", "A new version of Profanity is available: %s", latest_release);
                win_save_println(console, "Check <http://www.profanity.im> for details.");
                win_save_println(console, "");
            } else {
                if (not_available_msg) {
                    win_save_println(console, "No new version available.");
                    win_save_println(console, "");
                }
            }

            cons_alert();
        }
        free(latest_release);
    }
}

static void
_cons_show_login_success(ProfAccount *account)
{
    ProfWin *console = wins_get_console();
    win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "%s logged in successfully, ", account->jid);

    resource_presence_t presence = accounts_get_login_presence(account->name);
    const char *presence_str = string_from_resource_presence(presence);

    int presence_colour = win_presence_colour(presence_str);
    win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", "%s", presence_str);
    win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " (priority %d)",
        accounts_get_priority_for_presence_type(account->name, presence));
    win_save_print(console, '-', NULL, NO_DATE, 0, "", ".");
    cons_alert();
}

static void
_cons_show_wins(void)
{
    ProfWin *console = wins_get_console();
    cons_show("");
    cons_show("Active windows:");
    GSList *window_strings = wins_create_summary();

    GSList *curr = window_strings;
    while (curr != NULL) {
        win_save_println(console, curr->data);
        curr = g_slist_next(curr);
    }

    cons_show("");
    cons_alert();
}

static void
_cons_show_room_invites(GSList *invites)
{
    cons_show("");
    if (invites == NULL) {
        cons_show("No outstanding chat room invites.");
    } else {
        cons_show("Chat room invites, use /join or /decline commands:");
        while (invites != NULL) {
            cons_show("  %s", invites->data);
            invites = g_slist_next(invites);
        }
    }

    cons_alert();
}

static void
_cons_show_info(PContact pcontact)
{
    ProfWin *console = wins_get_console();
    win_show_info(console, pcontact);

    cons_alert();
}

static void
_cons_show_caps(const char * const fulljid, Resource *resource)
{
    ProfWin *console = wins_get_console();
    cons_show("");

    Capabilities *caps = caps_lookup(fulljid);
    if (caps) {
        const char *resource_presence = string_from_resource_presence(resource->presence);

        int presence_colour = win_presence_colour(resource_presence);
        win_save_vprint(console, '-', NULL, NO_EOL, presence_colour, "", "%s", fulljid);
        win_save_print(console, '-', NULL, NO_DATE, 0, "", ":");

        // show identity
        if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
            win_save_print(console, '-', NULL, NO_EOL, 0, "", "Identity: ");
            if (caps->name != NULL) {
                win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->name);
                if ((caps->category != NULL) || (caps->type != NULL)) {
                    win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                }
            }
            if (caps->type != NULL) {
                win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->type);
                if (caps->category != NULL) {
                    win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                }
            }
            if (caps->category != NULL) {
                win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->category);
            }
            win_save_newline(console);
        }
        if (caps->software != NULL) {
            win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "Software: %s", caps->software);
        }
        if (caps->software_version != NULL) {
            win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->software_version);
        }
        if ((caps->software != NULL) || (caps->software_version != NULL)) {
            win_save_newline(console);
        }
        if (caps->os != NULL) {
            win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "OS: %s", caps->os);
        }
        if (caps->os_version != NULL) {
            win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->os_version);
        }
        if ((caps->os != NULL) || (caps->os_version != NULL)) {
            win_save_newline(console);
        }

        if (caps->features != NULL) {
            win_save_println(console, "Features:");
            GSList *feature = caps->features;
            while (feature != NULL) {
                win_save_vprint(console, '-', NULL, 0, 0, "", " %s", feature->data);
                feature = g_slist_next(feature);
            }
        }
        caps_destroy(caps);

    } else {
        cons_show("No capabilities found for %s", fulljid);
    }

    cons_alert();
}

static void
_cons_show_software_version(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os)
{
    ProfWin *console = wins_get_console();
    if ((name != NULL) || (version != NULL) || (os != NULL)) {
        cons_show("");
        int presence_colour = win_presence_colour(presence);
        win_save_vprint(console, '-', NULL, NO_EOL, presence_colour, "", "%s", jid);
        win_save_print(console, '-', NULL, NO_DATE, 0, "", ":");
    }
    if (name != NULL) {
        cons_show("Name    : %s", name);
    }
    if (version != NULL) {
        cons_show("Version : %s", version);
    }
    if (os != NULL) {
        cons_show("OS      : %s", os);
    }

    cons_alert();
}

static void
_cons_show_received_subs(void)
{
    GSList *received = presence_get_subscription_requests();
    if (received == NULL) {
        cons_show("No outstanding subscription requests.");
    } else {
        cons_show("Outstanding subscription requests from:",
            g_slist_length(received));
        while (received != NULL) {
            cons_show("  %s", received->data);
            received = g_slist_next(received);
        }
        g_slist_free_full(received, g_free);
    }

    cons_alert();
}

static void
_cons_show_sent_subs(void)
{
   if (roster_has_pending_subscriptions()) {
        GSList *contacts = roster_get_contacts();
        PContact contact = NULL;
        cons_show("Awaiting subscription responses from:");
        while (contacts != NULL) {
            contact = (PContact) contacts->data;
            if (p_contact_pending_out(contact)) {
                cons_show("  %s", p_contact_barejid(contact));
            }
            contacts = g_slist_next(contacts);
        }
    } else {
        cons_show("No pending requests sent.");
    }

    cons_alert();
}

static void
_cons_show_room_list(GSList *rooms, const char * const conference_node)
{
    ProfWin *console = wins_get_console();
    if ((rooms != NULL) && (g_slist_length(rooms) > 0)) {
        cons_show("Chat rooms at %s:", conference_node);
        while (rooms != NULL) {
            DiscoItem *room = rooms->data;
            win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "  %s", room->jid);
            if (room->name != NULL) {
                win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", (%s)", room->name);
            }
            win_save_newline(console);
            rooms = g_slist_next(rooms);
        }
    } else {
        cons_show("No chat rooms at %s", conference_node);
    }

    cons_alert();
}

static void
_cons_show_bookmarks(const GList *list)
{
    ProfWin *console = wins_get_console();

    if (list == NULL) {
        cons_show("");
        cons_show("No bookmarks found.");
    } else {
        cons_show("");
        cons_show("Bookmarks:");

        while (list != NULL) {
            Bookmark *item = list->data;

            int presence_colour = 0;

            if (muc_room_is_active(item->jid)) {
                presence_colour = COLOUR_ONLINE;
            }
            win_save_vprint(console, '-', NULL, NO_EOL, presence_colour, "", "  %s", item->jid);
            if (item->nick != NULL) {
                win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", "/%s", item->nick);
            }
            if (item->autojoin) {
                win_save_print(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " (autojoin)");
            }
            if (item->password != NULL) {
                win_save_print(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " (private)");
            }
            if (muc_room_is_active(item->jid)) {
                ProfWin *roomwin = wins_get_by_recipient(item->jid);
                if (roomwin != NULL) {
                    int num = wins_get_num(roomwin);
                    win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " (%d)", num);
                }
            }
            win_save_newline(console);
            list = g_list_next(list);
        }
    }
    cons_alert();
}

static void
_cons_show_disco_info(const char *jid, GSList *identities, GSList *features)
{
    if (((identities != NULL) && (g_slist_length(identities) > 0)) ||
        ((features != NULL) && (g_slist_length(features) > 0))) {
        cons_show("");
        cons_show("Service disovery info for %s", jid);

        if (identities != NULL) {
            cons_show("  Identities");
        }
        while (identities != NULL) {
            DiscoIdentity *identity = identities->data;  // anme trpe, cat
            GString *identity_str = g_string_new("    ");
            if (identity->name != NULL) {
                identity_str = g_string_append(identity_str, identity->name);
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->type != NULL) {
                identity_str = g_string_append(identity_str, identity->type);
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->category != NULL) {
                identity_str = g_string_append(identity_str, identity->category);
            }
            cons_show(identity_str->str);
            g_string_free(identity_str, FALSE);
            identities = g_slist_next(identities);
        }

        if (features != NULL) {
            cons_show("  Features:");
        }
        while (features != NULL) {
            cons_show("    %s", features->data);
            features = g_slist_next(features);
        }

        cons_alert();
    }
}

static void
_cons_show_disco_items(GSList *items, const char * const jid)
{
    ProfWin *console = wins_get_console();
    if ((items != NULL) && (g_slist_length(items) > 0)) {
        cons_show("");
        cons_show("Service discovery items for %s:", jid);
        while (items != NULL) {
            DiscoItem *item = items->data;
            win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "  %s", item->jid);
            if (item->name != NULL) {
                win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", (%s)", item->name);
            }
            win_save_vprint(console, '-', NULL, NO_DATE, 0, "", "");
            items = g_slist_next(items);
        }
    } else {
        cons_show("");
        cons_show("No service discovery items for %s", jid);
    }

    cons_alert();
}

static void
_cons_show_status(const char * const barejid)
{
    ProfWin *console = wins_get_console();
    PContact pcontact = roster_get_contact(barejid);

    if (pcontact != NULL) {
        win_show_contact(console, pcontact);
    } else {
        cons_show("No such contact \"%s\" in roster.", barejid);
    }

    cons_alert();
}

static void
_cons_show_room_invite(const char * const invitor, const char * const room,
    const char * const reason)
{
    char *display_from = NULL;
    PContact contact = roster_get_contact(invitor);
    if (contact != NULL) {
        if (p_contact_name(contact) != NULL) {
            display_from = strdup(p_contact_name(contact));
        } else {
            display_from = strdup(invitor);
        }
    } else {
        display_from = strdup(invitor);
    }

    cons_show("");
    cons_show("Chat room invite received:");
    cons_show("  From   : %s", display_from);
    cons_show("  Room   : %s", room);

    if (reason != NULL) {
        cons_show("  Message: %s", reason);
    }

    cons_show("Use /join or /decline");

    if (prefs_get_boolean(PREF_NOTIFY_INVITE)) {
        notify_invite(display_from, room, reason);
    }

    free(display_from);

    cons_alert();
}

static void
_cons_show_account_list(gchar **accounts)
{
    ProfWin *console = wins_get_console();
    int size = g_strv_length(accounts);
    if (size > 0) {
        cons_show("Accounts:");
        int i = 0;
        for (i = 0; i < size; i++) {
            if ((jabber_get_connection_status() == JABBER_CONNECTED) &&
                    (g_strcmp0(jabber_get_account_name(), accounts[i]) == 0)) {
                resource_presence_t presence = accounts_get_last_presence(accounts[i]);
                int presence_colour = win_presence_colour(string_from_resource_presence(presence));
                win_save_vprint(console, '-', NULL, 0, presence_colour, "", "%s", accounts[i]);
            } else {
                cons_show(accounts[i]);
            }
        }
        cons_show("");
    } else {
        cons_show("No accounts created yet.");
        cons_show("");
    }

    cons_alert();
}

static void
_cons_show_account(ProfAccount *account)
{
    ProfWin *console = wins_get_console();
    cons_show("");
    cons_show("Account %s:", account->name);
    if (account->enabled) {
        cons_show   ("enabled           : TRUE");
    } else {
        cons_show   ("enabled           : FALSE");
    }
    cons_show       ("jid               : %s", account->jid);
    if (account->password != NULL) {
        cons_show   ("password          : [redacted]");
    }
    if (account->resource != NULL) {
        cons_show   ("resource          : %s", account->resource);
    }
    if (account->server != NULL) {
        cons_show   ("server            : %s", account->server);
    }
    if (account->port != 0) {
        cons_show   ("port              : %d", account->port);
    }
    if (account->muc_service != NULL) {
        cons_show   ("muc service       : %s", account->muc_service);
    }
    if (account->muc_nick != NULL) {
        cons_show   ("muc nick          : %s", account->muc_nick);
    }
    if (account->last_presence != NULL) {
        cons_show   ("Last presence     : %s", account->last_presence);
    }
    if (account->login_presence != NULL) {
        cons_show   ("Login presence    : %s", account->login_presence);
    }

    if (account->otr_policy != NULL) {
        cons_show   ("OTR policy        : %s", account->otr_policy);
    }
    if (g_list_length(account->otr_manual) > 0) {
        GString *manual = g_string_new("OTR manual        : ");
        GList *curr = account->otr_manual;
        while (curr != NULL) {
            g_string_append(manual, curr->data);
            if (curr->next != NULL) {
                g_string_append(manual, ", ");
            }
            curr = curr->next;
        }
        cons_show(manual->str);
        g_string_free(manual, TRUE);
    }
    if (g_list_length(account->otr_opportunistic) > 0) {
        GString *opportunistic = g_string_new("OTR opportunistic : ");
        GList *curr = account->otr_opportunistic;
        while (curr != NULL) {
            g_string_append(opportunistic, curr->data);
            if (curr->next != NULL) {
                g_string_append(opportunistic, ", ");
            }
            curr = curr->next;
        }
        cons_show(opportunistic->str);
        g_string_free(opportunistic, TRUE);
    }
    if (g_list_length(account->otr_always) > 0) {
        GString *always = g_string_new("OTR always        : ");
        GList *curr = account->otr_always;
        while (curr != NULL) {
            g_string_append(always, curr->data);
            if (curr->next != NULL) {
                g_string_append(always, ", ");
            }
            curr = curr->next;
        }
        cons_show(always->str);
        g_string_free(always, TRUE);
    }

    cons_show       ("Priority          : chat:%d, online:%d, away:%d, xa:%d, dnd:%d",
        account->priority_chat, account->priority_online, account->priority_away,
        account->priority_xa, account->priority_dnd);

    if ((jabber_get_connection_status() == JABBER_CONNECTED) &&
            (g_strcmp0(jabber_get_account_name(), account->name) == 0)) {
        GList *resources = jabber_get_available_resources();
        GList *ordered_resources = NULL;

        if (resources != NULL) {
            win_save_println(console, "Resources:");

            // sort in order of availabiltiy
            while (resources != NULL) {
                Resource *resource = resources->data;
                ordered_resources = g_list_insert_sorted(ordered_resources,
                    resource, (GCompareFunc)resource_compare_availability);
                resources = g_list_next(resources);
            }
        }

        while (ordered_resources != NULL) {
            Resource *resource = ordered_resources->data;
            const char *resource_presence = string_from_resource_presence(resource->presence);
            int presence_colour = win_presence_colour(resource_presence);
            win_save_vprint(console, '-', NULL, NO_EOL, presence_colour, "", "  %s (%d), %s", resource->name, resource->priority, resource_presence);

            if (resource->status != NULL) {
                win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", resource->status);
            }
            win_save_vprint(console, '-', NULL, NO_DATE, 0, "", "");
            Jid *jidp = jid_create_from_bare_and_resource(account->jid, resource->name);
            Capabilities *caps = caps_lookup(jidp->fulljid);

            if (caps != NULL) {
                // show identity
                if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                    win_save_print(console, '-', NULL, NO_EOL, 0, "", "    Identity: ");
                    if (caps->name != NULL) {
                        win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->name);
                        if ((caps->category != NULL) || (caps->type != NULL)) {
                            win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                        }
                    }
                    if (caps->type != NULL) {
                        win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->type);
                        if (caps->category != NULL) {
                            win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                        }
                    }
                    if (caps->category != NULL) {
                        win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->category);
                    }
                    win_save_newline(console);
                }
                if (caps->software != NULL) {
                    win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "    Software: %s", caps->software);
                }
                if (caps->software_version != NULL) {
                    win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->software_version);
                }
                if ((caps->software != NULL) || (caps->software_version != NULL)) {
                    win_save_newline(console);
                }
                if (caps->os != NULL) {
                    win_save_vprint(console, '-', NULL, NO_EOL, 0, "", "    OS: %s", caps->os);
                }
                if (caps->os_version != NULL) {
                    win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->os_version);
                }
                if ((caps->os != NULL) || (caps->os_version != NULL)) {
                    win_save_newline(console);
                }
                caps_destroy(caps);
            }

            ordered_resources = g_list_next(ordered_resources);
        }
    }

    cons_alert();
}

static void
_cons_show_aliases(GList *aliases)
{
    if (aliases == NULL) {
        cons_show("No aliases configured.");
        return;
    }

    GList *curr = aliases;
    if (curr != NULL) {
        cons_show("Command aliases:");
    }
    while (curr != NULL) {
        ProfAlias *alias = curr->data;
        cons_show("  /%s -> %s", alias->name, alias->value);
        curr = g_list_next(curr);
    }
    cons_show("");
}

static void
_cons_theme_setting(void)
{
    char *theme = prefs_get_string(PREF_THEME);
    if (theme == NULL) {
        cons_show("Theme (/theme)                : default");
    } else {
        cons_show("Theme (/theme)                : %s", theme);
    }
    prefs_free_string(theme);
}

static void
_cons_beep_setting(void)
{
    if (prefs_get_boolean(PREF_BEEP))
        cons_show("Terminal beep (/beep)         : ON");
    else
        cons_show("Terminal beep (/beep)         : OFF");
}

static void
_cons_flash_setting(void)
{
    if (prefs_get_boolean(PREF_FLASH))
        cons_show("Terminal flash (/flash)       : ON");
    else
        cons_show("Terminal flash (/flash)       : OFF");
}

static void
_cons_splash_setting(void)
{
    if (prefs_get_boolean(PREF_SPLASH))
        cons_show("Splash screen (/splash)       : ON");
    else
        cons_show("Splash screen (/splash)       : OFF");
}

static void
_cons_autoconnect_setting(void)
{
    char *pref_connect_account = prefs_get_string(PREF_CONNECT_ACCOUNT);
    if (pref_connect_account != NULL)
        cons_show("Autoconnect (/autoconnect)      : %s", pref_connect_account);
    else
        cons_show("Autoconnect (/autoconnect)      : OFF");

    prefs_free_string(pref_connect_account);
}

static void
_cons_vercheck_setting(void)
{
    if (prefs_get_boolean(PREF_VERCHECK))
        cons_show("Version checking (/vercheck)  : ON");
    else
        cons_show("Version checking (/vercheck)  : OFF");
}

static void
_cons_mouse_setting(void)
{
    if (prefs_get_boolean(PREF_MOUSE))
        cons_show("Mouse handling (/mouse)       : ON");
    else
        cons_show("Mouse handling (/mouse)       : OFF");
}

static void
_cons_statuses_setting(void)
{
    char *console = prefs_get_string(PREF_STATUSES_CONSOLE);
    char *chat = prefs_get_string(PREF_STATUSES_CHAT);
    char *muc = prefs_get_string(PREF_STATUSES_MUC);

    cons_show("Console statuses (/statuses)  : %s", console);
    cons_show("Chat statuses (/statuses)     : %s", chat);
    cons_show("MUC statuses (/statuses)      : %s", muc);

    prefs_free_string(console);
    prefs_free_string(chat);
    prefs_free_string(muc);
}

static void
_cons_titlebar_setting(void)
{
    if (prefs_get_boolean(PREF_TITLEBAR)) {
        cons_show("Titlebar display (/titlebar)  : ON");
    } else {
        cons_show("Titlebar display (/titlebar)  : OFF");
    }
}

static void
_cons_show_ui_prefs(void)
{
    cons_show("UI preferences:");
    cons_show("");
    cons_theme_setting();
    cons_beep_setting();
    cons_flash_setting();
    cons_splash_setting();
    cons_vercheck_setting();
    cons_mouse_setting();
    cons_statuses_setting();
    cons_titlebar_setting();

    cons_alert();
}

static void
_cons_notify_setting(void)
{
    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE))
        cons_show("Messages (/notify message)          : ON");
    else
        cons_show("Messages (/notify message)          : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE_CURRENT))
        cons_show("Messages current (/notify message)  : ON");
    else
        cons_show("Messages current (/notify message)  : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE_TEXT))
        cons_show("Messages text (/notify message)     : ON");
    else
        cons_show("Messages text (/notify message)     : OFF");

    char *room_setting = prefs_get_string(PREF_NOTIFY_ROOM);
    if (g_strcmp0(room_setting, "on") == 0) {
    cons_show    ("Room messages (/notify room)        : ON");
    } else if (g_strcmp0(room_setting, "off") == 0) {
    cons_show    ("Room messages (/notify room)        : OFF");
    } else {
    cons_show    ("Room messages (/notify room)        : %s", room_setting);
    }
    prefs_free_string(room_setting);

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_CURRENT))
        cons_show("Room current (/notify room)         : ON");
    else
        cons_show("Room current (/notify room)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT))
        cons_show("Room text (/notify room)            : ON");
    else
        cons_show("Room text (/notify room)            : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_TYPING))
        cons_show("Composing (/notify typing)          : ON");
    else
        cons_show("Composing (/notify typing)          : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_TYPING_CURRENT))
        cons_show("Composing current (/notify typing)  : ON");
    else
        cons_show("Composing current (/notify typing)  : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_INVITE))
        cons_show("Room invites (/notify invite)       : ON");
    else
        cons_show("Room invites (/notify invite)       : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_SUB))
        cons_show("Subscription requests (/notify sub) : ON");
    else
        cons_show("Subscription requests (/notify sub) : OFF");

    gint remind_period = prefs_get_notify_remind();
    if (remind_period == 0) {
        cons_show("Reminder period (/notify remind)    : OFF");
    } else if (remind_period == 1) {
        cons_show("Reminder period (/notify remind)    : 1 second");
    } else {
        cons_show("Reminder period (/notify remind)    : %d seconds", remind_period);
    }
}

static void
_cons_show_desktop_prefs(void)
{
    cons_show("Desktop notification preferences:");
    cons_show("");
    cons_notify_setting();

    cons_alert();
}

static void
_cons_states_setting(void)
{
    if (prefs_get_boolean(PREF_STATES))
        cons_show("Send chat states (/states) : ON");
    else
        cons_show("Send chat states (/states) : OFF");
}

static void
_cons_outtype_setting(void)
{
    if (prefs_get_boolean(PREF_OUTTYPE))
        cons_show("Send composing (/outtype)  : ON");
    else
        cons_show("Send composing (/outtype)  : OFF");
}

static void
_cons_intype_setting(void)
{
    if (prefs_get_boolean(PREF_INTYPE))
        cons_show("Show typing (/intype)      : ON");
    else
        cons_show("Show typing (/intype)      : OFF");
}

static void
_cons_gone_setting(void)
{
    gint gone_time = prefs_get_gone();
    if (gone_time == 0) {
        cons_show("Leave conversation (/gone) : OFF");
    } else if (gone_time == 1) {
        cons_show("Leave conversation (/gone) : 1 minute");
    } else {
        cons_show("Leave conversation (/gone) : %d minutes", gone_time);
    }
}

static void
_cons_history_setting(void)
{
    if (prefs_get_boolean(PREF_HISTORY))
        cons_show("Chat history (/history)    : ON");
    else
        cons_show("Chat history (/history)    : OFF");
}

static void
_cons_show_chat_prefs(void)
{
    cons_show("Chat preferences:");
    cons_show("");
    cons_states_setting();
    cons_outtype_setting();
    cons_intype_setting();
    cons_gone_setting();
    cons_history_setting();

    cons_alert();
}

static void
_cons_log_setting(void)
{
    cons_show("Log file location           : %s", get_log_file_location());
    cons_show("Max log size (/log maxsize) : %d bytes", prefs_get_max_log_size());

    if (prefs_get_boolean(PREF_LOG_ROTATE))
        cons_show("Log rotation (/log rotate)  : ON");
    else
        cons_show("Log rotation (/log rotate)  : OFF");

    if (prefs_get_boolean(PREF_LOG_SHARED))
        cons_show("Shared log (/log shared)    : ON");
    else
        cons_show("Shared log (/log shared)    : OFF");
}

static void
_cons_chlog_setting(void)
{
    if (prefs_get_boolean(PREF_CHLOG))
        cons_show("Chat logging (/chlog)       : ON");
    else
        cons_show("Chat logging (/chlog)       : OFF");
}

static void
_cons_grlog_setting(void)
{
    if (prefs_get_boolean(PREF_GRLOG))
        cons_show("Groupchat logging (/grlog)  : ON");
    else
        cons_show("Groupchat logging (/grlog)  : OFF");
}

static void
_cons_show_log_prefs(void)
{
    cons_show("Logging preferences:");
    cons_show("");
    cons_log_setting();
    cons_chlog_setting();
    cons_grlog_setting();

    cons_alert();
}

static void
_cons_autoaway_setting(void)
{
    char *pref_autoaway_mode = prefs_get_string(PREF_AUTOAWAY_MODE);
    if (strcmp(pref_autoaway_mode, "off") == 0) {
        cons_show("Autoaway (/autoaway mode)            : OFF");
    } else {
        cons_show("Autoaway (/autoaway mode)            : %s", pref_autoaway_mode);
    }
    prefs_free_string(pref_autoaway_mode);

    cons_show("Autoaway minutes (/autoaway time)    : %d minutes", prefs_get_autoaway_time());

    char *pref_autoaway_message = prefs_get_string(PREF_AUTOAWAY_MESSAGE);
    if ((pref_autoaway_message == NULL) ||
            (strcmp(pref_autoaway_message, "") == 0)) {
        cons_show("Autoaway message (/autoaway message) : OFF");
    } else {
        cons_show("Autoaway message (/autoaway message) : \"%s\"", pref_autoaway_message);
    }
    prefs_free_string(pref_autoaway_message);

    if (prefs_get_boolean(PREF_AUTOAWAY_CHECK)) {
        cons_show("Autoaway check (/autoaway check)     : ON");
    } else {
        cons_show("Autoaway check (/autoaway check)     : OFF");
    }
}

static void
_cons_show_presence_prefs(void)
{
    cons_show("Presence preferences:");
    cons_show("");
    cons_autoaway_setting();

    cons_alert();
}

static void
_cons_reconnect_setting(void)
{
    gint reconnect_interval = prefs_get_reconnect();
    if (reconnect_interval == 0) {
        cons_show("Reconnect interval (/reconnect) : OFF");
    } else if (reconnect_interval == 1) {
        cons_show("Reconnect interval (/reconnect) : 1 second");
    } else {
        cons_show("Reconnect interval (/reconnect) : %d seconds", reconnect_interval);
    }
}

static void
_cons_autoping_setting(void)
{
    gint autoping_interval = prefs_get_autoping();
    if (autoping_interval == 0) {
        cons_show("Autoping interval (/autoping)   : OFF");
    } else if (autoping_interval == 1) {
        cons_show("Autoping interval (/autoping)   : 1 second");
    } else {
        cons_show("Autoping interval (/autoping)   : %d seconds", autoping_interval);
    }
}

static void
_cons_priority_setting(void)
{
    gint priority = prefs_get_priority();
    cons_show("Priority (/priority) : %d", priority);
}

static void
_cons_show_connection_prefs(void)
{
    cons_show("Connection preferences:");
    cons_show("");
    cons_reconnect_setting();
    cons_autoping_setting();
    cons_autoconnect_setting();

    cons_alert();
}

static void
_cons_show_otr_prefs(void)
{
    cons_show("OTR preferences:");
    cons_show("");

    char *policy_value = prefs_get_string(PREF_OTR_POLICY);
    cons_show("OTR policy (/otr policy) : %s", policy_value);
    prefs_free_string(policy_value);

    if (prefs_get_boolean(PREF_OTR_WARN)) {
        cons_show("Warn non-OTR (/otr warn) : ON");
    } else {
        cons_show("Warn non-OTR (/otr warn) : OFF");
    }

    char *log_value = prefs_get_string(PREF_OTR_LOG);
    if (strcmp(log_value, "on") == 0) {
        cons_show("OTR logging (/otr log)   : ON");
    } else if (strcmp(log_value, "off") == 0) {
        cons_show("OTR logging (/otr log)   : OFF");
    } else {
        cons_show("OTR logging (/otr log)   : Redacted");
    }
    prefs_free_string(log_value);

    cons_alert();
}

static void
_cons_show_themes(GSList *themes)
{
    cons_show("");

    if (themes == NULL) {
        cons_show("No available themes.");
    } else {
        cons_show("Available themes:");
        while (themes != NULL) {
            cons_show(themes->data);
            themes = g_slist_next(themes);
        }
    }

    cons_alert();
}

static void
_cons_prefs(void)
{
    cons_show("");
    cons_show_ui_prefs();
    cons_show("");
    cons_show_desktop_prefs();
    cons_show("");
    cons_show_chat_prefs();
    cons_show("");
    cons_show_log_prefs();
    cons_show("");
    cons_show_presence_prefs();
    cons_show("");
    cons_show_connection_prefs();
    cons_show("");
    cons_show_otr_prefs();
    cons_show("");

    cons_alert();
}

static void
_cons_help(void)
{
    cons_show("");
    cons_show("Choose a help option:");
    cons_show("");
    cons_show("/help commands   - List all commands.");
    cons_show("/help basic      - List basic commands for getting started.");
    cons_show("/help chatting   - List chat commands.");
    cons_show("/help groupchat  - List groupchat commands.");
    cons_show("/help presence   - List commands to change presence.");
    cons_show("/help roster     - List commands for manipulating your roster.");
    cons_show("/help service    - List service discovery commands.");
    cons_show("/help settings   - List commands for changing settings.");
    cons_show("/help other      - Other commands.");
    cons_show("/help navigation - How to navigate around Profanity.");
    cons_show("/help [command]  - Detailed help on a specific command.");
    cons_show("");

    cons_alert();
}

static void
_cons_navigation_help(void)
{
    cons_show("");
    cons_show("Navigation:");
    cons_show("");
    cons_show("Alt-1 (F1)               : This console window.");
    cons_show("Alt-2..Alt-0 (F2..F10)   : Chat windows.");
    cons_show("Alt-LEFT                 : Previous chat window");
    cons_show("Alt-RIGHT                : Next chat window");
    cons_show("UP, DOWN                 : Navigate input history.");
    cons_show("LEFT, RIGHT, HOME, END   : Edit current input.");
    cons_show("CTRL-LEFT, CTRL-RIGHT    : Jump word in input.");
    cons_show("ESC                      : Clear current input.");
    cons_show("TAB                      : Autocomplete.");
    cons_show("PAGE UP, PAGE DOWN       : Page the main window.");
    cons_show("");

    cons_alert();
}

static void
_cons_show_roster_group(const char * const group, GSList *list)
{
    cons_show("");

    if (list != NULL) {
        cons_show("%s:", group);
    } else {
        cons_show("No group named %s exists.", group);
    }

    _show_roster_contacts(list, FALSE);

    cons_alert();
}

static void
_cons_show_roster(GSList *list)
{
    cons_show("");
    cons_show("Roster: jid (nick) - subscription - groups");

    _show_roster_contacts(list, TRUE);

    cons_alert();
}

static void
_cons_show_contact_online(PContact contact, Resource *resource, GDateTime *last_activity)
{
    const char *show = string_from_resource_presence(resource->presence);
    char *display_str = p_contact_create_display_string(contact, resource->name);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, show, resource->status, last_activity,
        "++", "online");

    free(display_str);
}

static void
_cons_show_contact_offline(PContact contact, char *resource, char *status)
{
    char *display_str = p_contact_create_display_string(contact, resource);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, "offline", status, NULL, "--",
        "offline");
    free(display_str);
}

static void
_cons_show_contacts(GSList *list)
{
    ProfWin *console = wins_get_console();
    GSList *curr = list;

    while(curr) {
        PContact contact = curr->data;
        if ((strcmp(p_contact_subscription(contact), "to") == 0) ||
            (strcmp(p_contact_subscription(contact), "both") == 0)) {
            win_show_contact(console, contact);
        }
        curr = g_slist_next(curr);
    }
    cons_alert();
}

static void
_cons_alert(void)
{
    if (ui_current_win_type() != WIN_CONSOLE) {
        status_bar_new(1);
    }
}

static void
_cons_splash_logo(void)
{
    ProfWin *console = wins_get_console();
    win_save_println(console, "Welcome to");

    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "                   ___            _           ");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "                  / __)          (_)_         ");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", " ____   ____ ___ | |__ ____ ____  _| |_ _   _ ");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "|_|                                    (____/ ");
    win_save_print(console, '-', NULL, 0, COLOUR_SPLASH, "", "");

    if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
        win_save_vprint(console, '-', NULL, 0, 0, "", "Version %sdev.%s.%s", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
        win_save_vprint(console, '-', NULL, 0, 0, "", "Version %sdev", PACKAGE_VERSION);
#endif
    } else {
        win_save_vprint(console, '-', NULL, 0, 0, "", "Version %s", PACKAGE_VERSION);
    }
}

void
_show_roster_contacts(GSList *list, gboolean show_groups)
{
    ProfWin *console = wins_get_console();
    GSList *curr = list;
    while(curr) {

        PContact contact = curr->data;
        GString *title = g_string_new("  ");
        title = g_string_append(title, p_contact_barejid(contact));
        if (p_contact_name(contact) != NULL) {
            title = g_string_append(title, " (");
            title = g_string_append(title, p_contact_name(contact));
            title = g_string_append(title, ")");
        }

        const char *presence = p_contact_presence(contact);
        int presence_colour;
        if (p_contact_subscribed(contact)) {
            presence_colour = win_presence_colour(presence);
        } else {
            presence_colour = win_presence_colour("offline");
        }
        win_save_vprint(console, '-', NULL, NO_EOL, presence_colour, "", title->str);

        g_string_free(title, TRUE);

        win_save_print(console, '-', NULL, NO_DATE | NO_EOL, 0, "", " - ");
        GString *sub = g_string_new("");
        sub = g_string_append(sub, p_contact_subscription(contact));
        if (p_contact_pending_out(contact)) {
            sub = g_string_append(sub, ", request sent");
        }
        if (presence_sub_request_exists(p_contact_barejid(contact))) {
            sub = g_string_append(sub, ", request received");
        }
        if (p_contact_subscribed(contact)) {
            presence_colour = COLOUR_SUBSCRIBED;
        } else {
            presence_colour = COLOUR_UNSUBSCRIBED;
        }

        if (show_groups) {
            win_save_vprint(console, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", "%s", sub->str);
        } else {
            win_save_vprint(console, '-', NULL, NO_DATE, presence_colour, "", "%s", sub->str);
        }

        g_string_free(sub, TRUE);

        if (show_groups) {
            GSList *groups = p_contact_groups(contact);
            if (groups != NULL) {
                GString *groups_str = g_string_new(" - ");
                while (groups != NULL) {
                    g_string_append(groups_str, groups->data);
                    if (g_slist_next(groups) != NULL) {
                        g_string_append(groups_str, ", ");
                    }
                    groups = g_slist_next(groups);
                }
                win_save_vprint(console, '-', NULL, NO_DATE, 0, "", "%s", groups_str->str);
                g_string_free(groups_str, TRUE);
            } else {
                 win_save_print(console, '-', NULL, NO_DATE, 0, "", " ");
            }
        }

        curr = g_slist_next(curr);
    }
}

void
console_init_module(void)
{
    cons_show_time = _cons_show_time;
    cons_show_word = _cons_show_word;
    cons_debug = _cons_debug;
    cons_show = _cons_show;
    cons_show_error = _cons_show_error;
    cons_show_typing = _cons_show_typing;
    cons_show_incoming_message = _cons_show_incoming_message;
    cons_about = _cons_about;
    cons_check_version = _cons_check_version;
    cons_show_login_success = _cons_show_login_success;
    cons_show_wins = _cons_show_wins;
    cons_show_room_invites = _cons_show_room_invites;
    cons_show_info = _cons_show_info;
    cons_show_caps = _cons_show_caps;
    cons_show_software_version = _cons_show_software_version;
    cons_show_received_subs = _cons_show_received_subs;
    cons_show_sent_subs = _cons_show_sent_subs;
    cons_show_room_list = _cons_show_room_list;
    cons_show_bookmarks = _cons_show_bookmarks;
    cons_show_disco_info = _cons_show_disco_info;
    cons_show_disco_items = _cons_show_disco_items;
    cons_show_status = _cons_show_status;
    cons_show_room_invite = _cons_show_room_invite;
    cons_show_account_list = _cons_show_account_list;
    cons_show_account = _cons_show_account;
    cons_theme_setting = _cons_theme_setting;
    cons_beep_setting = _cons_beep_setting;
    cons_flash_setting = _cons_flash_setting;
    cons_splash_setting = _cons_splash_setting;
    cons_autoconnect_setting = _cons_autoconnect_setting;
    cons_vercheck_setting = _cons_vercheck_setting;
    cons_mouse_setting = _cons_mouse_setting;
    cons_statuses_setting = _cons_statuses_setting;
    cons_titlebar_setting = _cons_titlebar_setting;
    cons_show_ui_prefs = _cons_show_ui_prefs;
    cons_notify_setting = _cons_notify_setting;
    cons_show_desktop_prefs = _cons_show_desktop_prefs;
    cons_states_setting = _cons_states_setting;
    cons_outtype_setting = _cons_outtype_setting;
    cons_intype_setting = _cons_intype_setting;
    cons_gone_setting = _cons_gone_setting;
    cons_history_setting = _cons_history_setting;
    cons_show_chat_prefs = _cons_show_chat_prefs;
    cons_log_setting = _cons_log_setting;
    cons_chlog_setting = _cons_chlog_setting;
    cons_grlog_setting = _cons_grlog_setting;
    cons_show_log_prefs = _cons_show_log_prefs;
    cons_autoaway_setting = _cons_autoaway_setting;
    cons_show_presence_prefs = _cons_show_presence_prefs;
    cons_show_otr_prefs = _cons_show_otr_prefs;
    cons_reconnect_setting = _cons_reconnect_setting;
    cons_autoping_setting = _cons_autoping_setting;
    cons_priority_setting = _cons_priority_setting;
    cons_show_connection_prefs = _cons_show_connection_prefs;
    cons_show_themes = _cons_show_themes;
    cons_prefs = _cons_prefs;
    cons_help = _cons_help;
    cons_navigation_help = _cons_navigation_help;
    cons_show_roster_group = _cons_show_roster_group;
    cons_show_roster = _cons_show_roster;
    cons_show_contacts = _cons_show_contacts;
    cons_alert = _cons_alert;
    cons_show_contact_online = _cons_show_contact_online;
    cons_show_contact_offline = _cons_show_contact_offline;
    cons_show_aliases = _cons_show_aliases;
}
