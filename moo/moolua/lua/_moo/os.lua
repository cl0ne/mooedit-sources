-- -%- indent-width:2 -%- --

local _G = _G
local math, table, package = math, table, package
local require, setmetatable = require, setmetatable
local _os = os
local lfs = require('lfs')
local _moo_utils = require("_moo_utils")

module('_moo.os')

if package.config:sub(1,1) == '/' then
  name = 'posix'
else
  name = 'nt'
end

os = _os
execute = os.execute
system = _moo_utils._execute
clock = os.clock
date = os.date
difftime = os.difftime
getenv = os.getenv
remove = os.remove
rename = os.rename
time = os.time
tmpname = os.tmpname

environ = {}
setmetatable(environ, { __index = function(t, v) return getenv(v) end })

chdir = lfs.chdir
getcwd = lfs.currentdir

access = _moo_utils._access
F_OK = _moo_utils.F_OK
R_OK = _moo_utils.R_OK
W_OK = _moo_utils.W_OK
X_OK = _moo_utils.X_OK
