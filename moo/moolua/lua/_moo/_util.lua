-- -%- indent-width:2 -%- --

local _G = _G
local type, require, pairs =
      type, require, pairs

module('_moo._util')

function inject_symbols(src, dest)
  if type(src) == 'string' then
    src = require(src)
  end
  for k, v in pairs(src) do
    if type(k) == 'string' then
      if #k ~= 0 and k:sub(1,1) ~= '_' then
        dest[k] = v
      end
    end
  end
end
