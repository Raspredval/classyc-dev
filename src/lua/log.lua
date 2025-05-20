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

---@generic T
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function log.assert(v, strfmt, ...)
    if not v then
        log.error(strfmt, ...)
    end

    return v
end

---@param strSourceFile string
---@param nSourceLine integer
---@param strfmt string
---@param ... any
function log.source_error(strSourceFile, nSourceLine, strfmt, ...)
    log.error(("[%s:%i] Error: %s") :
        format(strSourceFile, nSourceLine, strfmt), ...)
end

---@generic T
---@param strSourceFile string
---@param nSourceLine integer
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function log.source_assert(strSourceFile, nSourceLine, v, strfmt, ...)
    if not v then
        log.source_error(strSourceFile, nSourceLine, strfmt, ...)
    end

    return v
end

---@param strfmt string
---@param ... any
function log.internal_error(strfmt, ...)
    log.error(("%s\n Internal error: %s") :
        format(debug.traceback("traceback", 2), strfmt))
end

---@generic T
---@param v T?
---@param strfmt string
---@param ... any
---@return T
function log.internal_assert(v, strfmt, ...)
    if not v then
        log.internal_error(strfmt, ...)
    end

    return v
end

return log