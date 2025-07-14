local oop, IMacro =
    require "oop",
    require "Preprocessor.IMacro"

---@alias OnExpandFunc fun(self: Preprocessor, strMacroName: string, objFileInfo: FileInfo, tblMacros: MacroLookupTable, tblParams : string[]) : string

---@class MacroBuiltin : IMacro
---@field private objPreprocessor   Preprocessor
---@field private fnOnExpand        OnExpandFunc
---@field public  New               fun(objPreprocessor: Preprocessor, fnOnExpand: OnExpandFunc) : MacroBuiltin
local MacroBuiltin = oop.newClass(IMacro)

---@param objPreprocessor Preprocessor
---@param fnOnExpand OnExpandFunc
function MacroBuiltin:__init(objPreprocessor, fnOnExpand)
    self.objPreprocessor    = objPreprocessor
    self.fnOnExpand         = fnOnExpand
end

---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function MacroBuiltin:Expand(strMacroName, objFileInfo, tblMacros, tblParams)
    return self.fnOnExpand(self.objPreprocessor, strMacroName, objFileInfo, tblMacros, tblParams)
end

return MacroBuiltin
