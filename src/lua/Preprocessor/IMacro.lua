local oop, log =
    require "oop",
    require "log"

---@class IMacro
local IMacro = oop.newClass()

---@param ... string
---@return string
function IMacro:Expand(...)
    return ""
end

return IMacro