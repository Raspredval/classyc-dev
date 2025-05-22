local oop, log =
    require "oop",
    require "log"

---@class FileInfo
---@field public strName    string
---@field public fData      file*
---@field public nLine      integer
local FileInfo = oop.newClass()

---@param strFilename string
---@param fData file*
---@return FileInfo
function FileInfo.New(strFilename, fData)
    local obj   = setmetatable({
                    strName = strFilename,
                    fData   = fData,
                    nLine   = 1
                }, FileInfo)

    return obj
end

function FileInfo:Error(strfmt, ...)
    error(("[%s:%i] Error: "):format(self.strName, self.nLine) ..
        strfmt:format(...), 0)
end

---@generic T
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function FileInfo:Assert(v, strfmt, ...)
    if not v then
        error(("[%s:%i] Assertion failed"):format(self.strName, self.nLine) ..
            strfmt:format(...), 0)
    end

    return v
end

return FileInfo