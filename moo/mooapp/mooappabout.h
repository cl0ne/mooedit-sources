/*
 *   mooapp/mooappabout.h
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

#ifndef MOO_APP_ABOUT_H
#define MOO_APP_ABOUT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#include <mooglib/moo-glib.h>
#include <errno.h>
#include <gtk/gtk.h>

#ifdef __WIN32__

static char *
get_system_name (void)
{
    OSVERSIONINFOEXW ver;

    memset (&ver, 0, sizeof (ver));
    ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFOW);

    if (!GetVersionExW ((OSVERSIONINFOW*) &ver))
        return g_strdup ("Windows");

    switch (ver.dwMajorVersion)
    {
        case 4: /* Windows NT 4.0, Windows Me, Windows 98, or Windows 95 */
            switch (ver.dwMinorVersion)
            {
                case 0: /* Windows NT 4.0 or Windows95 */
                    if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
                        return g_strdup ("Windows 95");
                    else
                        return g_strdup ("Windows NT 4.0");

                case 10:
                    return g_strdup ("Windows 98");

                case 90:
                    return g_strdup ("Windows 98");
            }

            break;

        case 5: /* Windows Server 2003 R2, Windows Server 2003, Windows XP, or Windows 2000 */
            switch (ver.dwMinorVersion)
            {
                case 0:
                    return g_strdup ("Windows 2000");
                case 1:
                    return g_strdup ("Windows XP");
                case 2:
                    return g_strdup ("Windows Server 2003");
            }

            break;

        case 6:
            memset (&ver, 0, sizeof (ver));
            ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEXW);

            if (!GetVersionExW ((OSVERSIONINFOW*) &ver) || ver.wProductType == VER_NT_WORKSTATION)
            {
                switch (ver.dwMinorVersion)
                {
                    case 0:
                        return g_strdup ("Windows Vista");
                    case 1:
                        return g_strdup ("Windows 7");
                    case 2:
                        return g_strdup ("Windows 8");
                }
            }
            else
            {
                switch (ver.dwMinorVersion)
                {
                    case 0:
                        return g_strdup ("Windows Server 2008");
                    case 1:
                        return g_strdup ("Windows Server 2008 R2");
                    case 2:
                        return g_strdup ("Windows Server 2012");
                }
            }

            break;
    }

    return g_strdup ("Windows");
}

#elif defined(HAVE_SYS_UTSNAME_H)

static char *
get_system_name (void)
{
    struct utsname name;

    if (uname (&name) != 0)
    {
        MGW_ERROR_IF_NOT_SHARED_LIBC
        mgw_errno_t err = { errno };
        g_critical ("%s", mgw_strerror (err));
        return g_strdup ("unknown");
    }

    return g_strdup_printf ("%s %s (%s), %s", name.sysname,
                            name.release, name.version, name.machine);
}

#else

static char *
get_system_name (void)
{
    char *string;

    if (g_spawn_command_line_sync ("uname -s -r -v -m", &string, NULL, NULL, NULL))
        return string;
    else
        return g_strdup ("unknown");
}

#endif

#endif /* MOO_APP_ABOUT_H */

