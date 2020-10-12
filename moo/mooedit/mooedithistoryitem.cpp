#include "mooedithistoryitem.h"
#include <stdlib.h>

#define KEY_ENCODING "encoding"
#define KEY_LINE "line"

void
_moo_edit_history_item_set_encoding (MooHistoryItem *item,
                                     const char     *encoding)
{
    g_return_if_fail (item != NULL);
    moo_history_item_set (item, KEY_ENCODING, encoding);
}

void
_moo_edit_history_item_set_line (MooHistoryItem *item,
                                 int             line)
{
    char *value = NULL;

    g_return_if_fail (item != NULL);

    if (line >= 0)
        value = g_strdup_printf ("%d", line + 1);

    moo_history_item_set (item, KEY_LINE, value);
    g_free (value);
}

const char *
_moo_edit_history_item_get_encoding (MooHistoryItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    return moo_history_item_get (item, KEY_ENCODING);
}

int
_moo_edit_history_item_get_line (MooHistoryItem *item)
{
    const char *strval;

    g_return_val_if_fail (item != NULL, -1);

    strval = moo_history_item_get (item, KEY_LINE);

    if (strval && strval[0])
        return strtol (strval, NULL, 10) - 1;
    else
        return -1;
}
