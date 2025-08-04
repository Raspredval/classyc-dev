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
    error("Error: " .. strfmt:format(...), 0)
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

return log