/*
 *   moo-test-macros.h
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

#ifndef MOO_TEST_MACROS_H
#define MOO_TEST_MACROS_H

#include "moo-test-utils.h"
#include <mooglib/moo-glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

G_BEGIN_DECLS


G_GNUC_UNUSED static gboolean
TEST_STR_EQ (const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return s1 == s2;
    else
        return strcmp (s1, s2) == 0;
}

G_GNUC_UNUSED static gboolean
TEST_STRV_EQ (char **ar1, char **ar2)
{
    if (!ar1 || !ar2)
        return ar1 == ar2;

    while (*ar1 && *ar2)
        if (strcmp (*ar1++, *ar2++) != 0)
            return FALSE;

    return *ar1 == *ar2;
}

static char *
test_string_stack_add__ (char *string)
{
#define STACK_LEN 100
    static char *stack[STACK_LEN];
    static guint ptr = STACK_LEN;

    if (ptr == STACK_LEN)
        ptr = 0;

    g_free (stack[ptr]);
    stack[ptr++] = string;

    return string;
#undef STACK_LEN
}

G_GNUC_UNUSED static const char *
TEST_FMT_STR (const char *s)
{
    GString *buf;

    if (!s)
        return "<null>";

    buf = g_string_new ("\"");

    while (*s)
    {
        char c = *s++;

        switch (c)
        {
            case '\r':
                g_string_append (buf, "\\r");
                break;
            case '\n':
                g_string_append (buf, "\\n");
                break;
            case '\t':
                g_string_append (buf, "\\t");
                break;
            case '\\':
                g_string_append (buf, "\\\\");
                break;
            case '"':
                g_string_append (buf, "\\\"");
                break;
            default:
                if (g_ascii_isprint (c))
                    g_string_append_c (buf, c);
                else
                    g_string_append_printf (buf, "\\x%02x", (guchar)c);
                break;
        }
    }

    g_string_append (buf, "\"");
    return test_string_stack_add__ (g_string_free (buf, FALSE));
}

G_GNUC_UNUSED static const char *
TEST_FMT_STRV (char **array)
{
    GString *s;

    if (!array)
        return "<null>";

    s = g_string_new ("{");

    while (*array)
    {
        if (s->len > 1)
            g_string_append (s, ", ");
        g_string_append_printf (s, "\"%s\"", *array++);
    }

    g_string_append (s, "}");
    return test_string_stack_add__ (g_string_free (s, FALSE));
}

#define TEST_FMT_INT(a)     test_string_stack_add__ (g_strdup_printf ("%d", (int) a))
#define TEST_FMT_UINT(a)    test_string_stack_add__ (g_strdup_printf ("%u", (guint) a))
#define TEST_FMT_DBL(a)     test_string_stack_add__ (g_strdup_printf ("%f", (double) a))

#define TEST_G_ASSERT(expr)                     \
G_STMT_START {                                  \
    if (G_UNLIKELY (!(expr)))                   \
    {                                           \
        char lstr[32];                          \
        char *s;                                \
        g_snprintf (lstr, 32, "%d", __LINE__);  \
        s = g_strconcat ("ERROR:(", __FILE__,   \
                         ":", lstr, "):",       \
                         G_STRFUNC, ": ",       \
                         "assertion failed: (", \
                         #expr, ")",            \
                         (char*) NULL);         \
        g_printerr ("**\n** %s\n", s);          \
        g_free (s);                             \
        moo_abort();                            \
    }                                           \
} G_STMT_END

G_GNUC_UNUSED static struct TestWarningsInfo {
    int count;
    int line;
    char *file;
    char *msg;
} *test_warnings_info;

static void
test_log_handler (const gchar    *log_domain,
                  GLogLevelFlags  log_level,
                  const gchar    *message,
                  gpointer        data)
{
    TEST_G_ASSERT (data == test_warnings_info);

    if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING))
    {
        test_warnings_info->count -= 1;

        if (test_warnings_info->count < 0)
            g_log_default_handler (log_domain, log_level, message, NULL);
    }
}

G_GNUC_PRINTF(4, 0) static void
TEST_EXPECT_WARNINGV_ (int         howmany,
                       int         line,
                       const char *file,
                       const char *fmt,
                       va_list     args)
{
    TEST_G_ASSERT (test_warnings_info == NULL);
    TEST_G_ASSERT (howmany >= 0);

    test_warnings_info = g_new0 (struct TestWarningsInfo, 1);
    test_warnings_info->count = howmany;
    test_warnings_info->line = line;
    test_warnings_info->file = g_strdup (file);
    test_warnings_info->msg = g_strdup_vprintf (fmt, args);

    g_log_set_default_handler (test_log_handler, test_warnings_info);
    g_log_set_handler (NULL, (GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                       test_log_handler, test_warnings_info);
    g_log_set_handler (G_LOG_DOMAIN, (GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                       test_log_handler, test_warnings_info);
}

G_GNUC_PRINTF(4, 5) G_GNUC_UNUSED static void
TEST_EXPECT_WARNING_ (int         howmany,
                      int         line,
                      const char *file,
                      const char *fmt,
                      ...)
{
    va_list args;
    va_start (args, fmt);
    TEST_EXPECT_WARNINGV_ (howmany, line, file, fmt, args);
    va_end (args);
}

G_GNUC_UNUSED static void
TEST_CHECK_WARNING (void)
{
#ifndef __WIN32__
    TEST_G_ASSERT (test_warnings_info != NULL);

    moo_test_assert_msg (test_warnings_info->count == 0,
                         test_warnings_info->file,
                         test_warnings_info->line,
                         "%s: %d %s warning(s)",
                         test_warnings_info->msg ? test_warnings_info->msg : "",
                         ABS (test_warnings_info->count),
                         test_warnings_info->count < 0 ?
                         "unexpected" : "missing");

    g_free (test_warnings_info->msg);
    g_free (test_warnings_info->file);
    g_free (test_warnings_info);
    test_warnings_info = NULL;

    g_log_set_default_handler (g_log_default_handler, NULL);
    g_log_set_handler (NULL, (GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                       g_log_default_handler, test_warnings_info);
    g_log_set_handler (G_LOG_DOMAIN, (GLogLevelFlags) (G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                       g_log_default_handler, test_warnings_info);
#endif // __WIN32__
}

/*
 * Suppress warnings when GCC is in -pedantic mode and not -std=c99
 */
#ifdef __GNUC__
#pragma GCC system_header
#endif

#define TEST_FAILED_MSG(format,...)                                 \
    moo_test_assert_msg (FALSE, __FILE__, __LINE__,                 \
                         format, __VA_ARGS__)

#define TEST_FAILED(msg)                                            \
    moo_test_assert_msg (FALSE, __FILE__, __LINE__,                 \
                         "%s", msg)

#define TEST_ASSERT_MSG(cond,format,...)                            \
G_STMT_START {                                                      \
    if (cond)                                                       \
        moo_test_assert_impl (TRUE, "", __FILE__, __LINE__);        \
    else                                                            \
        moo_test_assert_msg (FALSE, __FILE__, __LINE__,             \
                             format, __VA_ARGS__);                  \
} G_STMT_END

#define TEST_ASSERT(cond)                                           \
    moo_test_assert_impl (!!(cond), (char*) #cond,                  \
                          __FILE__, __LINE__)

#define TEST_ASSERT_CMP__(Type,actual,expected,cmp,fmt_arg,msg)     \
G_STMT_START {                                                      \
    Type actual__ = actual;                                         \
    Type expected__ = expected;                                     \
    gboolean passed__ = cmp (actual__, expected__);                 \
    TEST_ASSERT_MSG (passed__,                                      \
                     "%s: expected %s, got %s", msg,                \
                     fmt_arg (expected__),                          \
                     fmt_arg (actual__));                           \
} G_STMT_END

#define TEST_ASSERT_CMP(Type,actual,expected,cmp,cmp_sym,fmt_arg)   \
    TEST_ASSERT_CMP__ (Type, actual, expected, cmp, fmt_arg,        \
                       #actual " " #cmp_sym " " #expected)          \

#define TEST_ASSERT_CMP_MSG(Type,actual,expected,cmp,               \
                            fmt_arg,format,...)                     \
G_STMT_START {                                                      \
    char *msg__ = g_strdup_printf (format, __VA_ARGS__);            \
    TEST_ASSERT_CMP__ (Type, actual, expected, cmp, fmt_arg,        \
                       msg__);                                      \
    g_free (msg__);                                                 \
} G_STMT_END

#define TEST_ASSERT_STR_EQ(actual,expected)                         \
    TEST_ASSERT_CMP (const char *, actual, expected,                \
                     TEST_STR_EQ, =, TEST_FMT_STR)

#define TEST_ASSERT_STR_NEQ(actual,expected)                        \
    TEST_ASSERT_CMP (const char *, actual, expected,                \
                     TEST_STR_NEQ, !=, TEST_FMT_STR)

#define TEST_ASSERT_STR_EQ_MSG(actual,expected,format,...)          \
    TEST_ASSERT_CMP_MSG (const char *, actual, expected,            \
                         TEST_STR_EQ, TEST_FMT_STR,                 \
                         format, __VA_ARGS__)

#define TEST_ASSERT_STRV_EQ(actual,expected)                        \
    TEST_ASSERT_CMP (char**, actual, expected,                      \
                     TEST_STRV_EQ, =, TEST_FMT_STRV)

#define TEST_ASSERT_STRV_EQ_MSG(actual,expected,format,...)         \
    TEST_ASSERT_CMP_MSG (char**, actual, expected,                  \
                         TEST_STRV_EQ, TEST_FMT_STRV,               \
                         format, __VA_ARGS__)

#define TEST_ASSERT_INT_EQ(actual,expected)                         \
    TEST_ASSERT_CMP (int, actual, expected,                         \
                     TEST_CMP_EQ, =, TEST_FMT_INT)

#define TEST_ASSERT_DBL_EQ(actual,expected)                         \
    TEST_ASSERT_CMP (double, actual, expected,                      \
                     TEST_CMP_EQ, =, TEST_FMT_DBL)

#define TEST_CMP_EQ(a,b)    ((a) == (b))
#define TEST_CMP_NEQ(a,b)   ((a) != (b))
#define TEST_CMP_LT(a,b)    ((a) < (b))
#define TEST_CMP_LE(a,b)    ((a) <= (b))
#define TEST_CMP_GT(a,b)    ((a) > (b))
#define TEST_CMP_GE(a,b)    ((a) >= (b))

#define TEST_STR_NEQ(s1,s2)  (!TEST_STR_EQ ((s1), (s2)))
#define TEST_STRV_NEQ(s1,s2) (!TEST_STRV_EQ ((s1), (s2)))

#ifndef __WIN32__
#define TEST_EXPECT_WARNING(howmany,fmt,...)            \
    TEST_EXPECT_WARNING_ (howmany, __LINE__, __FILE__,  \
                          fmt, __VA_ARGS__)
#else // !__WIN32__
#define TEST_EXPECT_WARNING(howmany,fmt,...)
#endif // __WIN32__

G_END_DECLS

#endif /* MOO_TEST_MACROS_H */
