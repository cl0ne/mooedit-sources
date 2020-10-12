require("munit")

function to_list(t)
  local l = {}
  for k, v in pairs(t) do
    table.insert(l, v)
  end
  return l
end

function list_equal(t1, t2)
  t1 = to_list(t1)
  t2 = to_list(t2)
  if #t1 ~= #t2 then
    return false
  end
  for i, v1 in ipairs(t1) do
    if v1 ~= t2[i] then
      return false
    end
  end
  return true
end

function table_equal(t1, t2)
  for k, v in pairs(t1) do
    v2 = t2[k]
    if v2 ~= v1 then
      if type(v2) ~= 'table' or type(v1) ~= 'table' or not table_equal(v1, v2) then
        return false
      end
    end
  end
  for k, _ in pairs(t2) do
    if not t1[k] then
      return false
    end
  end
  return true
end

function test_builtin()
  tassert(abs(-1) == abs(1))
  tassert(abs(-1) == 1)

  local t1 = {3, 2, 1}
  sort(t1)
  tassert(list_equal(t1, {1, 2, 3}))

  tassert(all({}))
  tassert(all({1, 2, 3}))
  tassert(not all({1, 2, 3, false}))

  tassert(not any({}))
  tassert(any({1, 2, 3}))
  tassert(any({1, false, 3}))
  tassert(not any({false, false}))

  local function id(x) return x end
  local function _not(x) return not x end
  tassert(list_equal(filter(id, {}), {}))
  tassert(list_equal(filter(id, {1}), {1}))
  tassert(list_equal(filter(id, {1, false}), {1}))
  tassert(list_equal(filter(_not, {}), {}))
  tassert(list_equal(filter(_not, {1}), {}))
  tassert(list_equal(filter(_not, {1, false}), {false}))

  tassert(list_equal(map(function(x) return x+3 end, {}), {}))
  tassert(list_equal(map(function(x) return x+3 end, {1, 2, 3}), {4, 5, 6}))

  tassert(list_equal(range(0), {}))
  tassert(list_equal(range(1), {1}))
  tassert(list_equal(range(3), {1, 2, 3}))
  tassert(list_equal(range(0, 3), {0, 1, 2, 3}))
  tassert(list_equal(range(1, 3), {1, 2, 3}))
  tassert(list_equal(range(2, 3), {2, 3}))
  tassert(list_equal(range(3, 3), {3}))
  tassert(list_equal(range(1, 3, 2), {1, 3}))
  tassert(list_equal(range(1, 3, 3), {1}))

  tassert(max(1, 2) == 2)
  tassert(max(2, 1) == 2)
  tassert(max(1, 2, 3) == 3)
  tassert(max(1, 3, 2) == 3)
  tassert(max({}) == nil)
  tassert(max({1}) == 1)
  tassert(max({1, 2}) == 2)
  tassert(max({2, 1}) == 2)
  tassert(max({1, 2, 3}) == 3)
  tassert(max({1, 3, 2}) == 3)

  tassert(min(1, 2) == 1)
  tassert(min(2, 1) == 1)
  tassert(min(1, 2, 3) == 1)
  tassert(min(1, 3, 2) == 1)
  tassert(min({}) == nil)
  tassert(min({1}) == 1)
  tassert(min({1, 2}) == 1)
  tassert(min({2, 1}) == 1)
  tassert(min({1, 2, 3}) == 1)
  tassert(min({1, 3, 2}) == 1)

  tassert(list_equal(reversed({}), {}))
  tassert(list_equal(reversed({2}), {2}))
  tassert(list_equal(reversed({2,3,4,3}), {3,4,3,2}))

  local t2 = {3, 2, 1}
  tassert(list_equal(sorted(t2), {1, 2, 3}))
  tassert(not list_equal(sorted(t2), t2))
  tassert(list_equal(sorted({2,3,2,3,2,4}), {2,2,2,3,3,4}))

  local t = {1, 2, 3, 4, 5, 6}
  tassert(list_equal(slice(t), t))
  tassert(list_equal(slice(t, 1), t))
  tassert(list_equal(slice(t, -6), t))
  tassert(list_equal(slice(t, 6), {6}))
  tassert(list_equal(slice(t, -1), {6}))
  tassert(list_equal(slice(t, -3), {4,5,6}))
  tassert(list_equal(slice(t, 2), {2,3,4,5,6}))
  tassert(list_equal(slice(t, 1, 0), {}))
  tassert(list_equal(slice(t, 1, 1), {1}))
  tassert(list_equal(slice(t, 2, 3), {2, 3}))
  tassert(list_equal(slice(t, -6, 6), t))
  tassert(list_equal(slice(t, -6, -6), {1}))
  tassert(list_equal(slice(t, -1, -1), {6}))
  tassert(list_equal(slice(t, 2, -2), {2,3,4,5}))
end

function test_table()
  tassert(table.count({1,2,1,2,3}, 1) == 2)
  tassert(table.count({1,2,1,2,3}, 4) == 0)
  tassert(table.index({1,2,1,2,3}, 1) == 1)
  tassert(table.index({1,2,1,2,3}, 1, 2) == 3)
  tassert(table.index({1,2,1,2,3}, 1, 2, 3) == 3)
  tassert(table.index({1,2,1,2,3}, 1, 4, 5) == 0)
  tassert(table.index({1,2,1,2,3}, 3) == 5)
  tassert(table.index({1,2,1,2,3}, 4) == 0)
  tassert(table.index({}, 4) == 0)

  local t = {1,2,3,2}
  table.reverse(t)
  tassert(list_equal(t, {2,3,2,1}))
  t = {}
  table.reverse(t)
  tassert(list_equal(t, {}))
  local t = {1,2,3,4,2}
  table.reverse(t)
  tassert(list_equal(t, {2,4,3,2,1}))
end

function test_string()
  local s
  s = 'abcde'
  tassert(s:startswith(''))
  tassert(s:startswith('a'))
  tassert(s:startswith('abc'))
  tassert(s:startswith(s))
  tassert(not s:startswith('b'))
  tassert(s:endswith(''))
  tassert(s:endswith('e'))
  tassert(s:endswith('cde'))
  tassert(s:endswith(s))
  tassert(not s:endswith('b'))
end

test_builtin()
test_table()
test_string()
