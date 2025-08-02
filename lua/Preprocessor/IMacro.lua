local oop, lpeg, PEG =
    require "oop",
    require "lpeg",
    require "CommonPatterns"

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

---@alias MacroLookupTable table<string, IMacro>

---@class IMacro : oop.object
local IMacro = oop.newClass()

---@param strMacroName  string
---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@param tblParams     string[]
---@return string
function IMacro:Expand(strMacroName, objFileInfo, tblMacros, tblParams)
    return ""
end

---@param strChunk      string
---@param objFileInfo   FileInfo
---@param tblMacros     MacroLookupTable
---@return string
function IMacro.ExpandAllMacros(strChunk, objFileInfo, tblMacros)
    while true do
        local nMacroBegin, strMacroName, tblMacroParams, nMacroEnd =
            pegParseMacro:match(strChunk)

        objFileInfo:Assert(not PEG.IsStringEOF,
            "missing a closing string literal quote (\")")

        if not strMacroName then
            return strChunk
        else
            local objMacro  = objFileInfo:Assert(tblMacros[strMacroName],
                                "undefined macro: %s", strMacroName)
            objFileInfo:Assert(objMacro:IsDerivedFrom(IMacro),
                "'%s' macro name is reserved and cannot be used")

            strChunk = ("%s%s%s"):format(
                strChunk:sub(1, nMacroBegin - 1),
                objMacro:Expand(strMacroName, objFileInfo, tblMacros, tblMacroParams),
                strChunk:sub(nMacroEnd))
        end
    end
end

return IMacro