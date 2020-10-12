-- -%- indent-width:2 -%- --

local _G = _G
local math, table = math, table
local pairs, ipairs, select, unpack, require =
      pairs, ipairs, select, unpack, require
local os = require('_moo.os')
local lfs = require('lfs')

module('_moo.path')

function exists(path)
  return lfs.attributes(path) ~= nil
end

local function is_sep(c)
  if os.name == 'nt' then
    return c == '\\' or c == '/'
  else
    return c == '/'
  end
end

function splitext(path)
  for i = #path, 2, -1 do
    local c = path:sub(i, i)
    if c == '.' then
      return path:sub(1, i - 1), path:sub(i)
    elseif is_sep(c) then
      break
    end
  end
  return path, ''
end
