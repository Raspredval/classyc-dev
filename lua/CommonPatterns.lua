local lpeg = require "lpeg"

local PEG = {}

PEG.LoCase, PEG.HiCase =
    lpeg.R"az", lpeg.R"AZ"
PEG.Alpha, PEG.Digit =
    PEG.LoCase + PEG.HiCase, lpeg.R"09"
PEG.Alnum, PEG.Space =
    PEG.Alpha + PEG.Digit, lpeg.S" \t"
PEG.EOF, PEG.Padding =
    -lpeg.P(1), PEG.Space ^ 0

---@param pegPattern Pattern
---@return Pattern
function PEG.Bounds(pegPattern)
    return lpeg.Cp() * pegPattern * lpeg.Cp()
end

---@type Pattern
PEG.DoubleUnderscore =
    lpeg.P"__"

---@type Pattern
PEG.ID =
    (PEG.Alpha + "_") * ((PEG.Alnum + "_") ^ 0)

PEG.IsStringEOF = false

local pegCatchStringEOF =
    lpeg.Cmt(PEG.EOF, function() PEG.IsStringEOF = true; return false end)
local pegResetStringEOF =
    lpeg.Cmt(lpeg.P"\"", function() PEG.IsStringEOF = false; return true end)

---@type Pattern
PEG.StringLiteral =
    "\"" *
    (("\\" * lpeg.P(1)) + (lpeg.P(1) - "\"")) ^ 0 *
    (pegCatchStringEOF + pegResetStringEOF)

---@type Pattern
PEG.CharLiteral =
    "\'" * (("\\" * lpeg.P(1)) + (lpeg.P(1) - "\'")) * "\'"

---@type Pattern
PEG.StringLiteralContents =
    "\"" *
    lpeg.C((("\\" * lpeg.P(1)) + (lpeg.P(1) - "\"")) ^ 0) *
    (pegCatchStringEOF + pegResetStringEOF)

---@diagnostic disable missing-fields

---@param strExprListItem string
local function FilterEmptyExprListItems(strExprListItem)
    if #strExprListItem > 0 then
        return strExprListItem
    end
end

---@type Pattern
PEG.ExprList = lpeg.P{
    "root_list",
    root_list   = PEG.Padding * (lpeg.V"expr" / FilterEmptyExprListItems) * ("," * PEG.Padding * (lpeg.V"expr" / FilterEmptyExprListItems)) ^ 0,
    expr_list   = lpeg.V"expr" * ("," * lpeg.V"expr") ^ 0,
    expr        = lpeg.V"expr_text" * ((
        lpeg.V"round_brkt" +
        lpeg.V"square_brkt" +
        lpeg.V"curly_brkt" +
        PEG.StringLiteral + PEG.CharLiteral
    ) * lpeg.V"expr") ^ -1,
    expr_text   = (PEG.Alnum + PEG.Space + lpeg.S"+-*/^%!~&|.:$@_<=>") ^ 0,
    round_brkt  = "(" * lpeg.V"expr_list" * ")",
    square_brkt = "[" * lpeg.V"expr_list" * "]",
    curly_brkt  = "{" * lpeg.V"expr_list" * "}"
}

---@type Pattern
PEG.IDList = lpeg.P{
    "name_list",
    name        = lpeg.C(PEG.ID) * PEG.Padding,
    separator   = "," * PEG.Padding,
    name_list   = PEG.Padding * lpeg.V"name" * (lpeg.V"separator" * lpeg.V"name") ^ 0 * PEG.Padding
}

---@diagnostic enable missing-fields

return PEG
