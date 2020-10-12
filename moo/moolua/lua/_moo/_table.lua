-- -%- indent-width:2 -%- --

local _G = _G
local table, math = table, math
local pairs, ipairs, select, unpack, require =
      pairs, ipairs, select, unpack, require

module('_moo._table')

function count(t, x)
  local c = 0
  for _, v in pairs(t) do
    if x == v then
      c = c + 1
    end
  end
  return c
end

function index(t, x, start, stop)
  start = start or 1
  stop = stop or #t
  for i = start, stop do
    if t[i] == x then
      return i
    end
  end
  return 0
end

function reverse(t)
  local len = #t
  for i = 1, math.floor(len / 2) do
    local tmp = t[i]
    t[i] = t[len - i + 1]
    t[len - i + 1] = tmp
  end
end
