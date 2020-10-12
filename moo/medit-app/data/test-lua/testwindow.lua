require("munit")

editor.close_docs(editor.get_docs())
window = editor.get_active_window()
tassert(#window.get_docs() == 1)
tassert(window.get_docs()[1] == window.get_active_doc())
editor.new_doc(window)
tassert(#window.get_docs() == 2)
tassert(window.get_docs()[2] == window.get_active_doc())
tassert(window.close_all())
tassert(#window.get_docs() <= 1)

doc1 = window.get_active_doc()
if not doc1 then
  doc1 = editor.new_doc(window)
end

doc2 = editor.new_doc(window)
doc3 = editor.new_doc(window)
doc4 = editor.new_doc(window)

tab1 = doc1.get_tab()
tab2 = doc2.get_tab()
tab3 = doc3.get_tab()
tab4 = doc4.get_tab()

view1 = doc1.get_view()
view2 = doc2.get_view()
view3 = doc3.get_view()
view4 = doc4.get_view()

tassert(tab1.get_doc() == doc1)
tassert(tab1.get_active_view() == view1)
tassert(doc1.get_view() == view1)
tassert(doc1.get_tab() == tab1)
tassert(view1.get_tab() == tab1)
tassert(view1.get_doc() == doc1)

tassert(window.get_n_tabs() == 4)

tassert_eq(window.get_views(), {view1, view2, view3, view4})
tassert_eq(window.get_docs(), {doc1, doc2, doc3, doc4})
tassert_eq(window.get_tabs(), {tab1, tab2, tab3, tab4})

window.set_active_tab(tab3)
tassert(window.get_active_tab() == tab3)
tassert(window.get_active_doc() == doc3)
tassert(window.get_active_view() == view3)

window.set_active_doc(doc2)
tassert(window.get_active_tab() == tab2)
tassert(window.get_active_doc() == doc2)
tassert(window.get_active_view() == view2)

window.set_active_view(view1)
tassert(window.get_active_tab() == tab1)
tassert(window.get_active_doc() == doc1)
tassert(window.get_active_view() == view1)

tassert(view4.get_editor() == editor)
tassert(view4.get_window() == window)

tassert_eq(tab3.get_views(), {view3})
tassert_eq(tab3.get_window(), window)

editor.set_active_doc(doc2)
tassert(window.get_active_tab() == tab2)
tassert(window.get_active_doc() == doc2)
tassert(window.get_active_view() == view2)
tassert(editor.get_active_window() == window)
tassert(editor.get_active_doc() == doc2)
tassert(editor.get_active_view() == view2)

editor.set_active_view(view4)
tassert(window.get_active_tab() == tab4)
tassert(window.get_active_doc() == doc4)
tassert(window.get_active_view() == view4)
tassert(editor.get_active_window() == window)
tassert(editor.get_active_doc() == doc4)
tassert(editor.get_active_view() == view4)

tassert_eq(doc1.get_views(), {view1})
tassert_eq(doc1.get_window(), window)
tassert_eq(doc1.get_n_views(), 1)

tassert(editor.close_docs({doc1, doc2, doc3}))
tassert_eq(window.get_docs(), {doc4})
tassert_eq(editor.get_active_doc(), doc4)
