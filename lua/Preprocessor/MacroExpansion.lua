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
    macro_name  = PEG.ID + PEG.Digit ^ 1,
    macro       = "$" * lpeg.C(lpeg.V"macro_name") * lpeg.Ct((("(" * PEG.ExprList * ")") ^ -1)),
    first_macro = lpeg.V"text" * (lpeg.V"literal" * lpeg.V"text") ^ 0 * PEG.Bounds(lpeg.V"macro")
}

---@diagnostic enable missing-fields

local MacroExpansion = {}

---@alias MacroLookupTable table<string, IMacro>

---@param strChunk      string
---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@return string
function MacroExpansion.ExpandAllMacros(strChunk, objFileInfo, tblMacros)
    while true do
        local nMacroBegin, strMacroName, tblMacroParams, nMacroEnd =
            pegParseMacro:match(strChunk)

        log.assert(not PEG.IsStringEOF,
            "missing closing quotes '\"'")

        if not strMacroName then
            return strChunk
        else
            local objMacro  = log.assert(tblMacros[strMacroName],
                                "undefined macro: %s", strMacroName)
            log.assert(objMacro:IsDerivedFrom(IMacro),
                "'%s' macro name is reserved and cannot be used")

            strChunk = ("%s%s%s"):format(
                strChunk:sub(1, nMacroBegin - 1),
                objMacro:Expand(strMacroName, objFileInfo, tblMacros, tblMacroParams),
                strChunk:sub(nMacroEnd))
        end
    end
end

return MacroExpansion
