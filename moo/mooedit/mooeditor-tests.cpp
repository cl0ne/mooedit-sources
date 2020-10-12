#include "mooedit/mooeditor-tests.h"
#include "mooedit/mooeditor-impl.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/moohistorymgr.h"
#include "moocpp/fileutils.h"

static struct {
    gstr working_dir;
    gstr encodings_dir;
} test_data;

#ifdef __WIN32__
#define LE "\r\n"
#else
#define LE "\n"
#endif

#define TT1 "blah blah blah"
#define TT2 "blah blah blah" LE "blah blah blah"
#define TT3 LE LE LE LE
#define TT4 "lala\nlala\n"
#define TT5 "lala\r\nlala\r\n"
#define TT6 "lala\rlala\r"

static void
check_contents (const gstr& filename,
                const char *expected)
{
    char *contents;
    GError *error = NULL;

    if (!g_file_get_contents (filename.get(), &contents, NULL, &error))
    {
        TEST_FAILED_MSG ("could not load file '%s': %s",
                         filename.get(), error->message);
        g_error_free (error);
        return;
    }

    TEST_ASSERT_STR_EQ (contents, expected);

    g_free (contents);
}

static void
test_basic (void)
{
    MooEditor *editor;
    MooEdit *doc, *doc2;
    GtkTextBuffer *buffer;
    MooOpenInfo *info;

    editor = moo_editor_instance ();
    gstr filename = g::build_filename(test_data.working_dir, "test.txt");
    info = moo_open_info_new(filename.get(), NULL, -1, MOO_OPEN_FLAGS_NONE);
    doc = moo_editor_new_file (editor, info, NULL, NULL);

    TEST_ASSERT (doc != NULL);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, "");

    buffer = moo_edit_get_buffer (doc);

    gtk_text_buffer_set_text (buffer, TT1, -1);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT1);

    gtk_text_buffer_set_text (buffer, TT2, -1);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT2);

    gtk_text_buffer_set_text (buffer, TT2 LE, -1);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT2 LE);

    gtk_text_buffer_set_text (buffer, TT3, -1);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT3);

    doc2 = moo_editor_open_path (editor, filename.get(), NULL, -1, NULL);
    TEST_ASSERT (doc2 == doc);

    TEST_ASSERT (moo_edit_close (doc));
    TEST_ASSERT (moo_editor_get_doc (editor, filename.get()) == NULL);

    g_file_set_contents (filename.get(), TT4, -1, NULL);
    doc = moo_editor_open_path (editor, filename.get(), NULL, -1, NULL);
    TEST_ASSERT (doc != NULL);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT4);
    TEST_ASSERT (moo_edit_close (doc));

    g_file_set_contents (filename.get(), TT5, -1, NULL);
    doc = moo_editor_open_path (editor, filename.get(), NULL, -1, NULL);
    TEST_ASSERT (doc != NULL);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT5);
    TEST_ASSERT (moo_edit_close (doc));

    g_file_set_contents (filename.get(), TT6, -1, NULL);
    doc = moo_editor_open_path (editor, filename.get(), NULL, -1, NULL);
    TEST_ASSERT (doc != NULL);
    TEST_ASSERT (moo_edit_save (doc, NULL));
    check_contents (filename, TT6);
    TEST_ASSERT (moo_edit_close (doc));

    g_object_unref (info);
}

#define TEST_ASSERT_SAME_FILE_CONTENT(filename1, filename2)             \
{                                                                       \
    char *contents1__ = NULL;                                           \
    char *contents2__ = NULL;                                           \
    g_file_get_contents (filename1, &contents1__, NULL, NULL);          \
    g_file_get_contents (filename2, &contents2__, NULL, NULL);          \
    if (!contents1__)                                                   \
        moo_test_assert_msg (FALSE, __FILE__, __LINE__,                 \
                             "could not open file %s",                  \
                             filename1);                                \
    if (!contents2__)                                                   \
        moo_test_assert_msg (FALSE, __FILE__, __LINE__,                 \
                             "could not open file %s",                  \
                             filename2);                                \
    if (contents1__ && contents2__)                                     \
    {                                                                   \
        gboolean equal = strcmp (contents1__, contents2__) == 0;        \
        TEST_ASSERT_MSG (equal, "contents of %s and %s differ",         \
                         filename1, filename2);                         \
    }                                                                   \
    g_free (contents2__);                                               \
    g_free (contents1__);                                               \
}

static void
test_encodings_1 (const char *name,
                  const char *working_dir)
{
    const char *dot;
    MooEditor *editor;
    MooEdit *doc;

    gstr encoding;
    if ((dot = strchr(name, '.')))
        encoding.steal(g_strndup (name, dot - name));
    else
        encoding.copy(name);

#ifdef MOO_OS_WIN32
    if (encoding == "UTF-16" || encoding == "UCS-4")
        return;
#endif

    gstr filename = g::build_filename (test_data.encodings_dir, name);
    gstr filename2 = g::build_filename (working_dir, name);

    editor = moo_editor_instance ();
    doc = moo_editor_open_path (editor, filename.get(), encoding.get(), -1, NULL);
    TEST_ASSERT_MSG (doc != NULL,
                     "file %s, encoding %s",
                     TEST_FMT_STR (filename.get()),
                     TEST_FMT_STR (encoding.get()));

    if (doc)
    {
        MooSaveInfo *info = moo_save_info_new (filename2.get(), NULL);
        TEST_ASSERT (moo_edit_save_as (doc, info, NULL));
        TEST_ASSERT_SAME_FILE_CONTENT (filename2.get(), filename.get());
        TEST_ASSERT (moo_edit_close (doc));
        g_object_unref (info);
    }
}

static void
test_encodings (void)
{
    GDir *dir;
    const char *name;
    mgw_errno_t err;

    dir = g_dir_open (test_data.encodings_dir.get(), 0, NULL);

    if (!dir)
    {
        g_critical ("could not open encodings dir");
        TEST_ASSERT (FALSE);
        return;
    }

    gstr working_dir = g::build_filename (test_data.working_dir, "encodings");
    _moo_mkdir_with_parents (working_dir.get(), &err);

    while ((name = g_dir_read_name (dir)))
        test_encodings_1 (name, working_dir.get());

    g_dir_close (dir);
}

static void
test_types (void)
{
    TEST_ASSERT (g_type_is_a (MOO_TYPE_EDIT_CONFIG_SOURCE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_EDIT_CONFIG_SOURCE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_SAVE_RESPONSE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_EDIT_STATE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_EDIT_STATUS, G_TYPE_FLAGS));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_LINE_END_TYPE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_TEXT_SELECTION_TYPE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_TEXT_SEARCH_FLAGS, G_TYPE_FLAGS));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_FIND_FLAGS, G_TYPE_FLAGS));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_DRAW_WS_FLAGS, G_TYPE_FLAGS));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_ACTION_CHECK_TYPE, G_TYPE_ENUM));
    TEST_ASSERT (g_type_is_a (MOO_TYPE_TEXT_CURSOR, G_TYPE_ENUM));
}

static gboolean
test_suite_init (G_GNUC_UNUSED gpointer data)
{
    mgw_errno_t err;

    test_data.working_dir = g::build_filename (moo_test_get_working_dir (), "editor-work");
    test_data.encodings_dir = g::build_filename (moo_test_get_data_dir (), "encodings");

    if (_moo_mkdir_with_parents (test_data.working_dir.get(), &err) != 0)
    {
        g_critical ("could not create directory '%s': %s",
                    test_data.working_dir.get(),
                    mgw_strerror (err));
        test_data.working_dir.clear();
        return FALSE;
    }

    return TRUE;
}

static void
test_suite_cleanup (G_GNUC_UNUSED gpointer data)
{
    char *recent_file;
    MooEditor *editor;
    GError *error = NULL;

    if (!_moo_remove_dir (test_data.working_dir.get(), TRUE, &error))
    {
        g_critical ("could not remove directory '%s': %s",
                    test_data.working_dir.get(), error->message);
        g_error_free (error);
        error = NULL;
    }

    test_data.working_dir = NULL;
    test_data.encodings_dir = NULL;

    editor = moo_editor_instance ();
//     moo_editor_close_all (editor, FALSE, FALSE);
    recent_file = _moo_history_mgr_get_filename (_moo_editor_get_history_mgr (editor));

    if (!g_file_test (recent_file, G_FILE_TEST_EXISTS))
        g_critical ("recent file %s does not exist", recent_file);

//     cache_dir = moo_get_user_cache_dir ();
//     if (!_moo_remove_dir (cache_dir, TRUE, &error))
//     {
//         g_critical ("could not remove directory '%s': %s",
//                     cache_dir, error->message);
//         g_error_free (error);
//         error = NULL;
//     }

    g_free (recent_file);
}

void
moo_test_editor (void)
{
    MooTestSuite& suite = moo_test_suite_new ("Editor",
                                              "Editor tests",
                                              test_suite_init,
                                              test_suite_cleanup,
                                              NULL);
    moo_test_suite_add_test (suite, "basic", "basic editor functionality", (MooTestFunc) test_basic, NULL);
    moo_test_suite_add_test (suite, "encodings", "character encoding handling", (MooTestFunc) test_encodings, NULL);
    moo_test_suite_add_test (suite, "types", "sanity checks for GObject types", (MooTestFunc) test_types, NULL);
}
