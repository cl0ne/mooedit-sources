require("munit")

local ascii = require("unicode.ascii")

local strings = {
  { "", "", "", 0, 0 },
  { "a", "A", "a", 1, 1 },
  { "ABC", "ABC", "abc", 3, 3 },
  { "AbC", "ABC", "abc", 3, 3 },
  { "\208\176\208\177\209\134", "\208\144\208\145\208\166", "\208\176\208\177\209\134", 6, 3 },
  { "\049\050\051\208\176\208\177\209\134", "\049\050\051\208\144\208\145\208\166", "\049\050\051\208\176\208\177\209\134", 9, 6 },
  { "\208\156\208\176\208\188\208\176", "\208\156\208\144\208\156\208\144", "\208\188\208\176\208\188\208\176", 8, 4 },
  { "\208\159\208\176\208\191\208\176", "\208\159\208\144\208\159\208\144", "\208\191\208\176\208\191\208\176", 8, 4 },
  { "Мама", "МАМА", "мама", 8, 4 },
}

local test_ustring_one = function(case)
  s = case[1]
  upper = case[2]
  lower = case[3]
  byte_len = case[4]
  char_len = case[5]

  tassert(string.upper(s) == upper, '%q:upper() is %q, expected %q', s, string.upper(s), upper)
  tassert(string.lower(s) == lower, '%q:lower() is %q, expected %q', s, string.lower(s), lower)

  tassert(string.len(s) == char_len, 'utf8.len(%q) is %d, expected %d', s, string.len(s), char_len)
  tassert(ascii.len(s) == byte_len, '%q:len() is %d, expected %d', s, ascii.len(s), byte_len)
  --tassert(#s == s:len(), '#%q != %q:len()', s, s)
end

for index, case in pairs(strings) do
  test_ustring_one(case)
end
