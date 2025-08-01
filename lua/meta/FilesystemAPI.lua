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

---@param path string
---@return Directory?
function fs.opendir(path) end

---@param path string
---@return boolean
function fs.rmdir(path) end

---@param path string
---@return boolean
function fs.rmfile(path) end

---@class Directory
local Directory = {}

---@alias DirectoryIterator fun() : string?

---@return DirectoryIterator
function Directory:readdir() end

---@param offset integer
function Directory:seekdir(offset) end

---@return integer
function Directory:telldir() end

function Directory:rewinddir() end

return fs
