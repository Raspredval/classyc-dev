---@meta fs

local fs = {}

---@return string?
function fs.getcwd() end

---@param path string
---@return boolean
function fs.chdir(path) end

---@param path string
---@return string?
function fs.realpath(path) end

---@param path string
---@return string?
function fs.basename(path) end

---@param path string
---@return string?
function fs.dirname(path) end

---@param path string
---@param mode string?
---@return boolean
function fs.access(path, mode) end

return fs
