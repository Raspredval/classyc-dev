local oop, log =
    require "oop",
    require "log"

---@class IMacro
local IMacro = oop.newClass()

---@param strMacroName  string
---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@param tblParams string[]
---@return string
function IMacro:Expand(strMacroName, objFileInfo, tblMacros, tblParams)
    return ""
end

return IMacro