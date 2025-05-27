local oop, IMacro =
    require "oop",
    require "Preprocessor.IMacro"

---@alias OnExpandFunc fun(self: Preprocessor, strMacroName: string, objFileInfo: FileInfo, tblMacros: MacroLookupTable, ... : string) : string

---@class MacroBuiltin : IMacro
---@field private objPreprocessor Preprocessor
---@field private fnOnExpand OnExpandFunc
local MacroBuiltin = oop.newClass(IMacro)

---@param objPreprocessor Preprocessor
---@param fnOnExpand OnExpandFunc
---@return MacroBuiltin
function MacroBuiltin.New(objPreprocessor, fnOnExpand)
    local obj   = setmetatable({
                    objPreprocessor = objPreprocessor,
                    fnOnExpand      = fnOnExpand
                }, MacroBuiltin)

    return obj
end

---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param ... string
---@return string
function MacroBuiltin:Expand(strMacroName, objFileInfo, tblMacros, ...)
    return self.fnOnExpand(self.objPreprocessor, strMacroName, objFileInfo, tblMacros, ...)
end

return MacroBuiltin
