local oop, log, IMacro, MacroExpansion =
    require "oop",
    require "log",
    require "Preprocessor.IMacro",
    require "Preprocessor.MacroExpansion"

---@class MacroDefined : IMacro
---@field private strBody       string
---@field private tblParamNames string[]
---@field public  New           fun(strMacroBody: string, ... : string) : MacroDefined
local MacroDefined = oop.newClass(IMacro)

---@param strMacroBody string
---@param ... string
function MacroDefined:__init(strMacroBody, ...)
    self.strBody        = strMacroBody
    self.tblParamNames  = {...}
end

---@param strMacroName  string
---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@param tblParams string[]
---@return string
function MacroDefined:Expand(strMacroName, objFileInfo, tblMacros, tblParams)
    local tblParamNames =
        self.tblParamNames
    local nParamNameCount, nParamValCount =
        #tblParamNames, #tblParams
    
    objFileInfo:Assert(nParamNameCount == nParamValCount,
        "%s macro param mismatch -- expected %i, got %i",
        strMacroName, nParamNameCount, nParamValCount)
    
    ---@type MacroLookupTable
    local tblMacrosLocal = setmetatable({}, tblMacros)
    tblMacrosLocal["__index"] =
        tblMacrosLocal

    for i = 1, nParamNameCount do
        local strParamName = tblParamNames[i]
        log.assert(not tblMacros[strParamName],
            "macro parameter shadows existing macro: %s", strParamName)

        tblMacrosLocal[strParamName] =
            MacroDefined.New(tblParams[i])
    end

    local macro_expansion =
        MacroExpansion.New(objFileInfo, tblMacrosLocal)

    return macro_expansion:ExpandMacros(self.strBody)
end

return MacroDefined
