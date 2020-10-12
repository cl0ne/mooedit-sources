/*
 *   mooi18n.c
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

#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-tests.h"
#include <mooglib/moo-glib.h>


/**
 * moo_dgettext: (moo.lua 0)
 *
 * @domain: (type const-utf8):
 * @string: (type const-utf8):
 *
 * Returns: (type const-utf8):
 **/

const char *moo_gettext (const char *string) G_GNUC_FORMAT (1);


#ifdef ENABLE_NLS
static void
init_gettext (void)
{
    static gboolean been_here = FALSE;

    if (!been_here)
    {
        been_here = TRUE;
        bindtextdomain (GETTEXT_PACKAGE, moo_get_locale_dir ());
        bindtextdomain (GETTEXT_PACKAGE "-gsv", moo_get_locale_dir ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        bind_textdomain_codeset (GETTEXT_PACKAGE "-gsv", "UTF-8");
#endif
    }
}
#endif /* ENABLE_NLS */

/**
 * moo_gettext: (moo.lua 0)
 *
 * @string: (type const-utf8)
 *
 * Returns: (type const-utf8)
 **/

const char *
moo_gettext (const char *string)
{
#ifdef ENABLE_NLS
    g_return_val_if_fail (string != NULL, NULL);
    init_gettext ();
    return dgettext (GETTEXT_PACKAGE, string);
#else /* !ENABLE_NLS */
    return string;
#endif /* !ENABLE_NLS */
}

const char *
moo_pgettext (const char *msgctxtid, G_GNUC_UNUSED gsize msgidoffset)
{
#ifdef ENABLE_NLS
    g_return_val_if_fail (msgctxtid != NULL, NULL);
    init_gettext ();
    return moo_dpgettext (GETTEXT_PACKAGE, msgctxtid, msgidoffset);
#else /* !ENABLE_NLS */
    return msgctxtid;
#endif /* !ENABLE_NLS */
}

const char *
moo_pgettext2 (G_GNUC_UNUSED const char *context, const char *msgctxtid)
{
#ifdef ENABLE_NLS
    char *tmp;
    const char *translation;

    g_return_val_if_fail (msgctxtid != NULL, NULL);
    init_gettext ();

    tmp = g_strjoin (context, "\004", msgctxtid, nullptr);
    translation = dgettext (GETTEXT_PACKAGE, tmp);

    if (translation == tmp)
        translation = msgctxtid;

    g_free (tmp);
    return translation;
#else /* !ENABLE_NLS */
    return msgctxtid;
#endif /* !ENABLE_NLS */
}

const char *
moo_dpgettext (const char *domain, const char *msgctxtid, gsize msgidoffset)
{
    g_return_val_if_fail (domain != NULL, NULL);
    g_return_val_if_fail (msgctxtid != NULL, NULL);
#ifdef ENABLE_NLS
    return g_dpgettext (GETTEXT_PACKAGE, msgctxtid, msgidoffset);
#else
    return msgctxtid + msgidoffset;
#endif
}

const char *
_moo_gsv_gettext (const char *string)
{
#ifdef ENABLE_NLS
    g_return_val_if_fail (string != NULL, NULL);
    init_gettext ();
    return dgettext (GETTEXT_PACKAGE "-gsv", string);
#else /* !ENABLE_NLS */
    return string;
#endif /* !ENABLE_NLS */
}

char *
_moo_gsv_dgettext (G_GNUC_UNUSED const char *domain, const char *string)
{
#ifdef ENABLE_NLS
    gchar *tmp;
    const gchar *translated;

    g_return_val_if_fail (string != NULL, NULL);

    init_gettext ();

    if (domain == NULL)
        return g_strdup (_moo_gsv_gettext (string));

    translated = dgettext (domain, string);

    if (strcmp (translated, string) == 0)
        return g_strdup (_moo_gsv_gettext (string));

    if (g_utf8_validate (translated, -1, NULL))
        return g_strdup (translated);

    tmp = g_locale_to_utf8 (translated, -1, NULL, NULL, NULL);

    if (tmp == NULL)
        return g_strdup (string);
    else
        return tmp;
#else
    return g_strdup (string);
#endif /* !ENABLE_NLS */
}


static void
test_mooi18n (void)
{
#ifdef ENABLE_NLS
    const char *locale_dir;
    char *po_file, *po_file2;

    locale_dir = moo_get_locale_dir ();
    po_file = g_build_filename (locale_dir, "ru", "LC_MESSAGES", GETTEXT_PACKAGE ".mo", nullptr);
    po_file2 = g_build_filename (locale_dir, "ru", "LC_MESSAGES", GETTEXT_PACKAGE "-gsv.mo", nullptr);

    TEST_ASSERT_MSG (g_file_test (po_file, G_FILE_TEST_EXISTS), "mo file '%s' does not exist", po_file);
    TEST_ASSERT_MSG (g_file_test (po_file2, G_FILE_TEST_EXISTS), "mo file '%s' does not exist", po_file2);

    g_free (po_file2);
    g_free (po_file);
#endif
}

void
moo_test_i18n (MooTestOptions opts)
{
    if (opts & MOO_TEST_INSTALLED)
    {
        MooTestSuite& suite = moo_test_suite_new ("mooi18n", "mooutils/mooi18n.c", NULL, NULL, NULL);

        moo_test_suite_add_test (suite, "mooi18n", "test of mooi18n",
                                 (MooTestFunc) test_mooi18n, NULL);
    }
}
