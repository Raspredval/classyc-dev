local oop, log =
    require "oop",
    require "log"

---@class FileInfo : oop.object
---@field private strName   string
---@field private fFile     file*
---@field private nLine     integer
---@field public  New       fun(strFilename: string, file_mode: "r" | "w") : FileInfo
local FileInfo = oop.newClass()

---@param strFilename string
---@param file_mode "r" | "w"
function FileInfo:__init(strFilename, file_mode)
    self.strName    = strFilename
    self.fFile      = log.assert(io.open(strFilename, file_mode),
                        "failed to open file: %s", strFilename)
    self.nLine      = 1
end

function FileInfo:__gc()
    self.fFile:close()
end

---@return string
function FileInfo:Path()
    return self.strName
end

function FileInfo:Line()
    return self.nLine
end

function FileInfo:IncrLine()
    self.nLine = self.nLine + 1
end

function FileInfo:File()
    return self.fFile
end

---@param strfmt string
---@param ... any
function FileInfo:Print(strfmt, ...)
    log.print("[file: %s][line %i] " .. strfmt,
        self.strName, self.nLine, ...)
end

---@param strfmt string
---@param ... any
function FileInfo:Error(strfmt, ...)
    error(("[file: %s][line: %i] Error: " .. strfmt):
        format(self.strName, self.nLine, ...), 0)
end

---@generic T
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function FileInfo:Assert(v, strfmt, ...)
    if not v then
        error(("[file: %s][line: %i] Error: " .. strfmt):
            format(self.strName, self.nLine, ...), 0)
    end

    return v
end

return FileInfo