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

---@param path string
---@return FileStats?
function fs.stat(path) end

---@class FileStats
local FileStats = {}

---@return integer
function FileStats:getDeviceID() end

---@return integer
function FileStats:getFileID() end

---@return integer
function FileStats:getOwnerID() end

---@return integer
function FileStats:getGroupID() end

---@return string?
function FileStats:getOwnerName() end

---@return string?
function FileStats:getGroupName() end

---@return integer
function FileStats:getLastAccessTime() end

---@return integer
function FileStats:getLastModificationTime() end

---@return integer
function FileStats:getLastStatusChangeTime() end

---@return integer
function FileStats:getFileSize() end

---@return "file" | "directory" | "pipe" | "symbolic link" | "socket" | "block special" | "char special" | "unknown"
function FileStats:getFileType() end


---return values: read bit, write bit, execute bit
---@return boolean, boolean, boolean
function FileStats:getOwnerPermissions() end

---return values: read bit, write bit, execute bit
---@return boolean, boolean, boolean
function FileStats:getGroupPermissions() end

---return values: read bit, write bit, execute bit
---@return boolean, boolean, boolean
function FileStats:getOtherPermissions() end

return fs
