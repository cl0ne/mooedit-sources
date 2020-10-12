/*
 *   mooi18n.h
 *
 *   Copyright (C) 2004-2016 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
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

#pragma once

#include <config.h>
#include <mooglib/moo-glib.h>

G_BEGIN_DECLS

#define moo_dgettext(d,s) D_(d,s)

const char *moo_gettext (const char *string) G_GNUC_FORMAT (1);
const char *moo_pgettext (const char *msgctxtid, gsize msgidoffset) G_GNUC_FORMAT (1);
const char *moo_pgettext2 (const char *context, const char *msgctxtid) G_GNUC_FORMAT (2);
const char *moo_dpgettext (const char *domain, const char *msgctxtid, gsize msgidoffset) G_GNUC_FORMAT (2);
const char *_moo_gsv_gettext (const char *string) G_GNUC_FORMAT (1);
char *_moo_gsv_dgettext (const char *domain, const char *string) G_GNUC_FORMAT (2);

G_END_DECLS

#ifdef ENABLE_NLS

#include <libintl.h>
#include <string.h>

#define _(String) moo_gettext (String)
#define N_(String) (String)
#define D_(String,Domain) dgettext (Domain, String)
#define C_(Context,String) moo_pgettext (Context "\004" String, strlen (Context) + 1)
#define NC_(Context,String) (String)
#define DC_(Context,String,Domain) moo_dpgettext (Domain, Context "\004" String, strlen (Context) + 1)

 // On windows the following produces wrong results when there is no translation, because the
 // arguments to g_strip_context() are two different character buffers (with identical contents),
 // i.e. it generates and uses two different buffers for the same string literal. There is probably
 // a compiler flag to avoid that duplication, but it's safer to just fix the macros instead.
 //#define Q_(String) g_strip_context ((String), moo_gettext (String))
inline static const char* Q_(const char* msgid)
{
	return g_strip_context(msgid, _(msgid));
}

//#define QD_(String,Domain) g_strip_context ((String), D_ (String, Domain))
inline static const char* QD_(const char* msgid, const char* domain)
{
	return g_strip_context(msgid, D_(msgid, domain));
}

#else /* !ENABLE_NLS */

#undef textdomain
#undef gettext
#undef dgettext
#undef dcgettext
#undef ngettext
#undef bindtextdomain
#undef bind_textdomain_codeset

// a function to make sure that g_strip_context parameters really
// point to the same char buffer
G_INLINE_FUNC const char*
_moo_strip_context (const char* s)
{
    return g_strip_context (s, s);
}

#define _(String) (String)
#define N_(String) (String)
#define Q_(String) _moo_strip_context (String)
#define D_(String,Domain) (String)
#define QD_(String,Domain) _moo_strip_context (String)

#define C_(Context,String) (String)
#define NC_(Context,String) (String)
#define DC_(Context,String,Domain) (String)

#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define ngettext(str,str_pl,n) ((n) > 1 ? (str_pl) : (str))
#define dngettext(dom,str,str_pl,n) ((n) > 1 ? (str_pl) : (str))
#define bindtextdomain(Domain,Directory) (Domain)
#define bind_textdomain_codeset(Domain,Codeset) (Codeset)

#endif /* !ENABLE_NLS */
