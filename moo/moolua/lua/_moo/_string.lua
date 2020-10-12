-- -%- indent-width:2 -%- --

local _G = _G
local table = table
local pairs, ipairs, select, unpack, require =
      pairs, ipairs, select, unpack, require

module('_moo._string')

function startswith(s, prfx)
  return s:sub(1, #prfx) == prfx
end

function endswith(s, sfx)
  if #sfx == 0 then
    return true
  else
    return s:sub(-#sfx) == sfx
  end
end
