#include <glib-object.h>
#include "moo-lua-api-util.h"

#undef g_object_connect
#undef g_object_connect_after
#undef g_object_disconnect
#undef g_object_signal_handler_block
#undef g_object_signal_handler_unblock

#define g_object_connect(obj,sig,closure) (moo_signal_connect_closure (obj, sig, closure, FALSE))
#define g_object_connect_after(obj,sig,closure) (moo_signal_connect_closure (obj, sig, closure, TRUE))

#define g_object_disconnect g_signal_handler_disconnect
#define g_object_signal_handler_block g_signal_handler_block
#define g_object_signal_handler_unblock g_signal_handler_unblock
