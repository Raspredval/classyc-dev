local log = {}

---@param strfmt string
---@param ... any
function log.print(strfmt, ...)
    io.stdout:write(strfmt:format(...))
end

function log.println(strfmt, ...)
    io.stdout:write(strfmt:format(...), "\n")
end

---@param strfmt string
---@param ... any
function log.error(strfmt, ...)
    error(strfmt:format(...), 0)
end

function log.assert(bCond, strfmt, ...)
    if not bCond then
        log.error(strfmt, ...)
    end
end

---@param strSourceFile string
---@param nSourceLine integer
---@param strfmt string
---@param ... any
function log.source_error(strSourceFile, nSourceLine, strfmt, ...)
    log.error(("[%s:%i] Error: %s") :
        format(strSourceFile, nSourceLine, strfmt), ...)
end

---@param strSourceFile string
---@param nSourceLine integer
---@param bCond boolean
---@param strfmt string
---@param ... any
function log.source_assert(strSourceFile, nSourceLine, bCond, strfmt, ...)
    if not bCond then
        log.source_error(strSourceFile, nSourceLine, strfmt, ...)
    end
end

---@param strfmt string
---@param ... any
function log.internal_error(strfmt, ...)
    log.error(("%s\n Internal error: %s") :
        format(debug.traceback("traceback", 2), strfmt))
end

---@param bCond boolean
---@param strfmt string
---@param ... any
function log.internal_assert(bCond, strfmt, ...)
    if not bCond then
        log.internal_error(strfmt, ...)
    end
end

return log