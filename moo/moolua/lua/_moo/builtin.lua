-- -%- indent-width:2 -%- --

local _G = _G
local math, table, string =
      math, table, string
local pairs, ipairs, select, unpack, require, type, error =
      pairs, ipairs, select, unpack, require, type, error

module('_moo.builtin')

function _inject(dest)
  local util = require('_moo._util')
  util.inject_symbols('_moo._table', table)
  util.inject_symbols('_moo._string', string)
  util.inject_symbols('_moo.builtin', dest)
  dest.os = require('_moo.os')
  dest.os.path = require('_moo.path')
end

abs = math.abs
sort = table.sort

function all(t, pred)
  local pred = pred
  if pred == nil then
    pred = function(x) return x; end
  end
  for k, v in pairs(t) do
    if not pred(v, k) then
      return false
    end
  end
  return true
end

function any(t, pred)
  local pred = pred
  if pred == nil then
    pred = function(x) return x; end
  end
  for k, v in pairs(t) do
    if pred(v, k) then
      return true
    end
  end
  return false
end

function filter(func, t)
  local t_new = {}
  for k, v in pairs(t) do
    if func(v, k) then
      t_new[k] = v
    end
  end
  return t_new
end

function map(func, t)
  local t_new = {}
  for k, v in pairs(t) do
    t_new[k] = func(v, k)
  end
  return t_new
end

function range(arg1, arg2, arg3)
  local start, stop, step
  if arg2 then
    start, stop, step = arg1, arg2, arg3
    if not step then
      step = 1
    end
  else
    start, stop, step = 1, arg1, 1
  end
  local t = {}
  for v = start, stop, step do
    table.insert(t, v)
  end
  return t
end

-- note, it loses nil arguments
local function pack(...)
  local list = {}
  for i = 1, select('#', ...) do
    local arg = select(i, ...)
    table.insert(list, arg)
  end
  return list
end

function max(...)
  if select('#', ...) ~= 1 then
    return max(pack(...))
  end
  local val
  for _, v in pairs(select(1, ...)) do
    if val == nil or v > val then
      val = v
    end
  end
  return val
end

function min(...)
  if select('#', ...) ~= 1 then
    return min(pack(...))
  end
  local val
  for _, v in pairs(select(1, ...)) do
    if val == nil or v < val then
      val = v
    end
  end
  return val
end

function reversed(t)
  local rt = {}
  local len = #t
  for i, v in ipairs(t) do
    rt[len - i + 1] = v
  end
  return rt
end

function sorted(t)
  local st = {}
  for _, v in pairs(t) do
    table.insert(st, v)
  end
  table.sort(st)
  return st
end

function slice(t, start, stop)
  local len = #t
  start = start or 1
  stop = stop or len
  if start < 0 then
    start = len + start + 1
  end
  if stop < 0 then
    stop = len + stop + 1
  end
  if type(t) == 'table' then
    local ret = {}
    for i = start, stop do
      table.insert(ret, t[i])
    end
    return ret
  else
    error("can't slice object of type " .. type(t))
  end
end
