/*
 *   mooapp-ipc.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mooapp-ipc.h"
#include "mooappinput.h"
#include "mooutils-debug.h"
#include "mooutils-misc.h"
#include "moolist.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define VERSION_STRING "0001"
#define VERSION_LEN 4
#define STAMP_LEN 8
#define APP_ID_LEN 8
#define CLIENT_ID_LEN 4
#define MAX_ID_LEN 9999

MOO_DEBUG_INIT (ipc, FALSE)

static struct {
    GHashTable *clients; /* id -> GSList* of ClientInfo */
    GQuark ids_quark;
    char *app_id;
    guint stamp;
    GHashTable *stamp_hash; /* app_id -> guint */
} ipc_data;

typedef struct {
    GObject *object;
    MooDataCallback callback;
    guint ref_count : 31;
    guint dead : 1;
} ClientInfo;

MOO_DEFINE_SLIST(ClientInfoList, client_info_list, ClientInfo)

static ClientInfo *
client_info_new (GObject        *obj,
                 MooDataCallback callback)
{
    ClientInfo *ci = g_slice_new (ClientInfo);
    ci->object = obj;
    ci->callback = callback;
    ci->ref_count = 1;
    ci->dead = FALSE;
    return ci;
}

static void
client_info_ref (ClientInfo *ci)
{
    ci->ref_count++;
}

static void
client_info_unref (ClientInfo *ci)
{
    g_return_if_fail (ci != NULL && ci->ref_count != 0);
    if (!--ci->ref_count)
        g_slice_free (ClientInfo, ci);
}

static char *
generate_id (void)
{
    struct timeval tv;
    int rand_int;

    gettimeofday (&tv, NULL);
    rand_int = g_random_int_range (1, 10000);

    return g_strdup_printf ("%04d%04d", (int)(tv.tv_sec % 10000), rand_int);
}

static void
init_clients_table (void)
{
    if (!ipc_data.clients)
    {
        ipc_data.clients = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        ipc_data.ids_quark = g_quark_from_static_string ("moo-ipc-client-id-list");
        ipc_data.stamp_hash = g_hash_table_new (g_str_hash, g_str_equal);
        ipc_data.app_id = generate_id ();
    }
}

static void
unregister_client (const char *id,
                   GObject    *obj)
{
    ClientInfo *ci = NULL;
    ClientInfoList *client_list, *l;

    moo_dmsg ("%s: removing <%p, %s>", G_STRFUNC, (gpointer) obj, id);

    client_list = (ClientInfoList*) g_hash_table_lookup (ipc_data.clients, id);
    for (ci = NULL, l = client_list; !ci && l; l = l->next)
    {
        ci = l->data;
        if (ci->object != obj)
            ci = NULL;
    }

    if (!ci)
    {
        g_critical ("could not find <%p, %s>", (gpointer) obj, id);
    }
    else
    {
        client_list = client_info_list_delete_link (client_list, l);
        if (client_list)
            g_hash_table_insert (ipc_data.clients, g_strdup (id), client_list);
        else
            g_hash_table_remove (ipc_data.clients, id);

        ci->dead = TRUE;
        client_info_unref (ci);
    }
}

static void
client_died (GSList  *ids,
             GObject *dead_obj)
{
    moo_dmsg ("%s: object %p died", G_STRFUNC, (gpointer) dead_obj);

    while (ids)
    {
        char *id = (char*) ids->data;

        unregister_client (id, dead_obj);

        g_free (id);
        ids = g_slist_delete_link (ids, ids);
    }
}

void
moo_ipc_register_client (GObject        *object,
                         const char     *id,
                         MooDataCallback callback)
{
    GSList *ids;
    ClientInfoList *client_list;
    ClientInfo *info;

    g_return_if_fail (G_IS_OBJECT (object));
    g_return_if_fail (id != NULL);

    if (strlen (id) > MAX_ID_LEN)
    {
        g_critical ("%s: id '%s' is too long", G_STRFUNC, id);
        return;
    }

    init_clients_table ();

    ids = (GSList*) g_object_get_qdata (object, ipc_data.ids_quark);
    if (g_slist_find_custom (ids, id, (GCompareFunc) strcmp))
    {
        g_critical ("%s: <%p, %s> already registered",
                    G_STRFUNC, (gpointer) object, id);
        return;
    }

    if (ids)
        g_object_weak_unref (object, (GWeakNotify) client_died, ids);
    ids = g_slist_prepend (ids, g_strdup (id));
    g_object_weak_ref (object, (GWeakNotify) client_died, ids);
    g_object_set_qdata (object, ipc_data.ids_quark, ids);

    info = client_info_new (object, callback);

    client_list = (ClientInfoList*) g_hash_table_lookup (ipc_data.clients, id);
    client_list = client_info_list_prepend (client_list, info);
    g_hash_table_insert (ipc_data.clients, g_strdup (id), client_list);
}

void
moo_ipc_unregister_client (GObject    *object,
                           const char *id)
{
    GSList *ids, *l;

    g_return_if_fail (G_IS_OBJECT (object));
    g_return_if_fail (id != NULL);

    init_clients_table ();

    ids = (GSList*) g_object_get_qdata (object, ipc_data.ids_quark);
    if (!(l = g_slist_find_custom (ids, id, (GCompareFunc) strcmp)))
    {
        g_critical ("%s: <%p, %s> not registered",
                    G_STRFUNC, (gpointer) object, id);
        return;
    }

    g_free (l->data);
    g_object_weak_unref (object, (GWeakNotify) client_died, ids);
    ids = g_slist_delete_link (ids, l);
    if (ids)
        g_object_weak_ref (object, (GWeakNotify) client_died, ids);
    g_object_set_qdata (object, ipc_data.ids_quark, ids);

    unregister_client (id, object);
}


void
moo_ipc_send (GObject    *sender,
              const char *id,
              const char *data,
              gssize      len)
{
    GString *header;
    gsize id_len;

    g_return_if_fail (!sender || G_IS_OBJECT (sender));
    g_return_if_fail (id != NULL);
    g_return_if_fail (data != NULL);
    g_return_if_fail (ipc_data.app_id != NULL);

    if (len < 0)
        len = strlen (data);

    id_len = strlen (id);
    g_return_if_fail (id_len != 0 && id_len < MAX_ID_LEN);

    header = g_string_new (VERSION_STRING);
    g_string_append (header, ipc_data.app_id);
    g_string_append_printf (header, "%08x", ++ipc_data.stamp);

    g_string_append_printf (header, "%04x", (unsigned) id_len);
    g_string_append (header, id);

    _moo_app_input_broadcast (header->str, data, len);

    g_string_free (header, TRUE);
}

static void
dispatch (const char *id,
          const char *data,
          gsize       len)
{
    ClientInfoList *clients;

    moo_dmsg ("%s: got data for id '%s': %.*s",
              G_STRFUNC, id, (int) len, data);

    clients = (ClientInfoList*) g_hash_table_lookup (ipc_data.clients, id);

    if (!clients)
    {
        moo_dmsg ("%s: no clients registered for id '%s'", G_STRFUNC, id);
        return;
    }

    clients = client_info_list_copy_links (clients);
    client_info_list_foreach (clients, (ClientInfoListFunc) client_info_ref, NULL);
    while (clients)
    {
        ClientInfo *ci = clients->data;

        if (!ci->dead)
        {
            moo_dmsg ("%s: feeding data to <%p>", G_STRFUNC, (gpointer) ci->object);
            ci->callback (ci->object, data, len);
        }

        client_info_unref (ci);
        clients = client_info_list_delete_link (clients, clients);
    }
}

static gboolean
get_uint (const char *data,
          guint       len,
          guint      *dest)
{
    char *string, *end;
    guint64 val;
    gboolean result = FALSE;
    mgw_errno_t err;

    string = g_strndup (data, len);
    val = mgw_ascii_strtoull (string, &end, 16, &err);

    if (!mgw_errno_is_set (err) && !end[0] && val <= G_MAXUINT)
    {
        *dest = (guint)val;
        result = TRUE;
    }

    g_free (string);
    return result;
}

static gboolean
check_stamp (const char *app_id,
             guint       stamp)
{
    gpointer old_val;

    old_val = g_hash_table_lookup (ipc_data.stamp_hash, app_id);

    if (old_val)
    {
        guint old_stamp = GPOINTER_TO_UINT (old_val);

        moo_dmsg ("%s: new stamp %u, old stamp %u",
                  G_STRFUNC, stamp, old_stamp);

        if (old_stamp >= stamp)
            return FALSE;

        g_hash_table_insert (ipc_data.stamp_hash, (char*) app_id,
                             GUINT_TO_POINTER (stamp));
    }
    else
    {
        g_hash_table_insert (ipc_data.stamp_hash, g_strdup (app_id),
                             GUINT_TO_POINTER (stamp));
    }

    return TRUE;
}

void
_moo_ipc_dispatch (const char *data,
                   gsize       len)
{
    guint stamp;
    guint id_len;
    char *id = NULL, *app_id = NULL;

    g_return_if_fail (len >= VERSION_LEN + STAMP_LEN + APP_ID_LEN + CLIENT_ID_LEN);

    if (!ipc_data.app_id)
    {
        moo_dmsg ("%s: no clients", G_STRFUNC);
        goto out;
    }

    if (strncmp (data, VERSION_STRING, VERSION_LEN) != 0)
    {
        moo_dmsg ("%s: wrong version %.4s", G_STRFUNC, data);
        goto out;
    }

    data += VERSION_LEN;
    len -= VERSION_LEN;

    if (strncmp (data, ipc_data.app_id, APP_ID_LEN) == 0)
    {
        moo_dmsg ("%s: got message from itself", G_STRFUNC);
        goto out;
    }

    app_id = g_strndup (data, APP_ID_LEN);
    data += APP_ID_LEN;
    len -= APP_ID_LEN;

    if (!get_uint (data, STAMP_LEN, &stamp))
    {
        moo_dmsg ("%s: invalid stamp '%.8s'", G_STRFUNC, data);
        goto out;
    }

    if (!check_stamp (app_id, stamp))
        goto out;

    data += STAMP_LEN;
    len -= STAMP_LEN;

    if (!get_uint (data, CLIENT_ID_LEN, &id_len))
    {
        moo_dmsg ("%s: invalid id length '%.4s'", G_STRFUNC, data);
        goto out;
    }

    data += CLIENT_ID_LEN;
    len -= CLIENT_ID_LEN;

    if (len < id_len)
    {
        moo_dmsg ("%s: id is too short: '%.*s' (expected %u)",
                  G_STRFUNC, (int) len, data, id_len);
        goto out;
    }

    id = g_strndup (data, id_len);
    data += id_len;
    len -= id_len;

    dispatch (id, data, len);

out:
    g_free (id);
    g_free (app_id);
}
