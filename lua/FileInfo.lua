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

function FileInfo:DecrLine()
    self.nLine = self.nLine - 1
end

function FileInfo:IncrLine()
    self.nLine = self.nLine + 1
end

function FileInfo:File()
    return self.fFile
end

---@alias LoggingFunction fun(strfmt: string, ...)

---@private
---@param fnLogging LoggingFunction
---@param strfmt string
---@param ... any
function FileInfo:loggingInsertCurParsePos(fnLogging, strfmt, ...)
    local strFilepath = self.strName
    if #strFilepath > 30 then
        strFilepath = ("...%s"):format(strFilepath:sub(#strFilepath - 30))
    end

    fnLogging("[file: %s][line %i] " .. strfmt,
        strFilepath, self.nLine, ...)
end

---@param strfmt string
---@param ... any
function FileInfo:Print(strfmt, ...)
    self:loggingInsertCurParsePos(
        log.print, strfmt, ...)
end

---@param strfmt string
---@param ... any
function FileInfo:Error(strfmt, ...)
    self:loggingInsertCurParsePos(
        log.error, strfmt, ...)
end

---@generic T
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function FileInfo:Assert(v, strfmt, ...)
    if not v then
        self:loggingInsertCurParsePos(
            log.error, strfmt, ...)
    end

    return v
end

return FileInfo