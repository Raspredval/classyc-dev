local oop, log =
    require "oop",
    require "log"

---@class IMacro
local IMacro = oop.newClass()

---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@param ... string
---@return string
function IMacro:Expand(objFileInfo, tblMacros, ...)
    return ""
end

return IMacro