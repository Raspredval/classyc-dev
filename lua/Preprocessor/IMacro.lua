local oop = require "oop"

---@class IMacro
local IMacro = oop.newClass()

---@return string
function IMacro:Body()
    error("invalid abstract method call", 2)
end

---@param i integer
---@return string?
function IMacro:ParamName(i)
    return nil
end

---@return integer
function IMacro:ParamCount()
    return 0
end

return IMacro