require('munit')

window = editor.get_active_window()
window.set_property('visible', false)
tassert(window.get_property('visible') == false)
window.set_property('visible', true)
tassert(window.get_property('visible') == true)
