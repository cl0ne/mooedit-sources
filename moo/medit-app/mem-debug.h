/*
 *   mem-debug.h
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

#ifndef MOO_MEM_DEBUG_H
#define MOO_MEM_DEBUG_H

#undef DEBUG_MEMORY

#ifndef DEBUG_MEMORY
static void
init_mem_stuff (void)
{
}
#else
#define ALIGN 8
#include <libxml/xmlmemory.h>

static gpointer
my_malloc (gsize n_bytes)
{
    char *mem = malloc (n_bytes + ALIGN);
    return mem ? mem + ALIGN : NULL;
}

static gpointer
my_realloc (gpointer mem,
	    gsize    n_bytes)
{
    if (mem)
    {
        char *new_mem = realloc ((char*) mem - ALIGN, n_bytes + ALIGN);
        return new_mem ? new_mem + ALIGN : NULL;
    }
    else
    {
        return my_malloc (n_bytes);
    }
}

static char *
my_strdup (const char *s)
{
    if (s)
    {
        char *new_s = my_malloc (strlen (s) + 1);
	if (new_s)
            strcpy (new_s, s);
        return new_s;
    }
    else
    {
        return NULL;
    }
}

static void
my_free (gpointer mem)
{
    if (mem)
        free ((char*) mem - ALIGN);
}

static void
init_mem_stuff (void)
{
    GMemVTable mem_table = {
        my_malloc,
        my_realloc,
        my_free,
        NULL, NULL, NULL
    };

    if (1)
    {
        g_mem_set_vtable (&mem_table);
        g_slice_set_config (G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);
    }
    else
    {
        xmlMemSetup (my_free, my_malloc, my_realloc, my_strdup);
    }
}
#endif

#endif /* MOO_MEM_DEBUG_H */
