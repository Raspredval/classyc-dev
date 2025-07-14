local oop, log, lpeg, PEG, FileInfo, IMacro =
    require "oop",
    require "log",
    require "lpeg",
    require "CommonPatterns",
    require "FileInfo",
    require "Preprocessor.IMacro"

---@diagnostic disable missing-fields

---@type Pattern
local pegParseMacro = lpeg.P{
    "first_macro",
    text        = (lpeg.P(1) - "$" - "\"" - "\'") ^ 0,
    literal     = PEG.StringLiteral + PEG.CharLiteral,
    macro       = "$" * lpeg.C(PEG.ID) * lpeg.Ct((("(" * PEG.ExprList * ")") ^ -1)),
    first_macro = lpeg.V"text" * (lpeg.V"literal" * lpeg.V"text") ^ 0 * PEG.Bounds(lpeg.V"macro")
}

---@diagnostic enable missing-fields

---@alias MacroLookupTable table<string, IMacro>

---@class MacroExpansion : oop.object
---@field private objFileInfo       FileInfo
---@field private tblMacros         MacroLookupTable
---@field public  New               fun(objFileInfo: FileInfo, tblMacros: MacroLookupTable) : MacroExpansion
local MacroExpansion = oop.newClass()

---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
function MacroExpansion:__init(objFileInfo, tblMacros)
    self.objFileInfo =
        objFileInfo
    self.tblMacros =
        tblMacros
end

---@param strChunk string
---@return string
function MacroExpansion:ExpandMacros(strChunk)
    while true do
        local nMacroBegin, strMacroName, tblMacroParams, nMacroEnd =
            pegParseMacro:match(strChunk)

        log.assert(not PEG.IsStringEOF,
            "missing closing quotes '\"'")

        if not strMacroName then
            return strChunk
        else
            local objMacro  = log.assert(self.tblMacros[strMacroName],
                                "undefined macro: %s", strMacroName)
            log.assert(objMacro:IsDerivedFrom(IMacro),
                "'%s' macro name is reserved and cannot be used")

            strChunk = ("%s%s%s"):format(
                strChunk:sub(1, nMacroBegin - 1),
                objMacro:Expand(strMacroName, self.objFileInfo, self.tblMacros, tblMacroParams),
                strChunk:sub(nMacroEnd))
        end
    end
end

return MacroExpansion

