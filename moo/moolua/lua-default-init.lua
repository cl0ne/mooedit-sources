local _g = getfenv(0)

do
  local status, result
  status, result = pcall(require, "_moo.builtin")
  if status then
    result._inject(_g)
  else
    print(result)
  end
end

local moo = require("moo")
_g.app = moo.App.instance()
_g.editor = _g.app.get_editor()
