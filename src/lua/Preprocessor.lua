local oop, lpeg, fs, log, MacroDefined, MacroBuiltin =
    require "oop",
    require "lpeg",
    require "fs",
    require "log",
    require "Preprocessor.MacroDefined",
    require "Preprocessor.MacroBuiltin"

local pegLoCase, pegHiCase =
    lpeg.R"az", lpeg.R"AZ"
local pegAlpha, pegDigit =
    pegLoCase + pegHiCase, lpeg.R"09"
local pegAlnum, pegSpace =
    pegAlpha + pegDigit, lpeg.S" \t"
local pegEOF, pegPadding =
    -lpeg.P(1), pegSpace ^ 0

---@param peg Pattern
---@return Pattern
local function pegfBounds(peg)
    return lpeg.Cp() * peg * lpeg.Cp()
end

---@type Pattern
local pegID =
    (pegAlpha + "_") * ((pegAlnum + "_") ^ 0)

---@type Pattern
local pegParseCommandName =
    pegPadding *
    ("#" * lpeg.C(pegID)) * lpeg.Cp()

---@type Pattern
local pegParseCommandBody =
    pegPadding *
    lpeg.C(lpeg.P(1) ^ 1)

---@type Pattern
local pegStringLiteral =
    "\"" * (("\\" * lpeg.P(1)) + (lpeg.P(1) - "\"")) ^ 0 * "\""

---@type Pattern
local pegCharLiteral =
    "\'" * (("\\" * lpeg.P(1)) + (lpeg.P(1) - "\'")) * "\'"

---@diagnostic disable missing-fields
---@type Pattern
local pegExprList = lpeg.P{
    "root_list",
    root_list   = lpeg.C(lpeg.V"expr") * ("," * lpeg.C(lpeg.V"expr")) ^ 0,
    expr_list   = lpeg.V"expr" * ("," * lpeg.V"expr") ^ 0,
    expr        = lpeg.V"expr_char" ^ 0 * ((lpeg.V"round_brkt" + lpeg.V"square_brkt" + lpeg.V"curly_brkt" + pegStringLiteral + pegCharLiteral) * lpeg.V"expr") ^ -1,
    expr_char   = pegAlnum + pegSpace + lpeg.S"+-*/^%!~&|.:$@_",
    round_brkt  = "(" * lpeg.V"expr_list" * ")",
    square_brkt = "[" * lpeg.V"expr_list" * "]",
    curly_brkt  = "{" * lpeg.V"expr_list" * "}"
}

---@type Pattern
local pegNameList = lpeg.P{
    "name_list",
    name        = lpeg.C(pegID) * pegPadding,
    separator   = "," * pegPadding,
    name_list   = pegPadding * lpeg.V"name" * (lpeg.V"separator" * lpeg.V"name") ^ 0 * pegPadding
}

---@type Pattern
local pegParseMacro = lpeg.P{
    "first_macro",
    text        = (lpeg.P(1) - "$" - "\"" - "\'") ^ 0,
    literal     = pegStringLiteral + pegCharLiteral,
    macro       = "$" * lpeg.C(pegID) * lpeg.Ct(("(" * pegExprList * ")") ^ -1),
    first_macro = lpeg.V"text" * (lpeg.V"literal" * lpeg.V"text") ^ 0 * pegfBounds(lpeg.V"macro")
}

---@type Pattern
local pegParseLineComment = lpeg.P{
    "first_comment",
    text            = (lpeg.P(1) - "//" - "\"" - "\'") ^ 0,
    literal         = pegStringLiteral + pegCharLiteral,
    comment         = lpeg.Cp() * "//",
    first_comment   = lpeg.V"text" * (lpeg.V"literal" * lpeg.V"text") ^ 0 * lpeg.V"comment"
}
---@diagnostic enable missing-fields

---@type Pattern
local pegCmdDefine =
    lpeg.C(pegID) *
    lpeg.Ct(("(" * pegNameList * ")") ^ -1) *
    pegSpace * lpeg.C(lpeg.P(1) ^ 1)

---@type Pattern
local pegCmdUndef =
    lpeg.C(pegID) * pegPadding * pegEOF

---@alias Command fun(self: Preprocessor, strCmdBody: string)

---@class Preprocessor
---@field private tblMacrosGlobal       table<string, IMacro>
---@field private bDisabledSource       boolean
---@field private strFile               string
---@field private nLine                 integer
--- static fields:
---@field private tblCommands           table<string, Command>
local Preprocessor = oop.setAsClass({
        tblCommands = {}
    })

---@private
---@return string
function Preprocessor:MacroLine()
    return tostring(self.nLine)
end

---@private
---@return string
function Preprocessor:MacroFile()
    return ("\"%s\""):format(self.strFile)
end

---@return Preprocessor
function Preprocessor.New()
    local obj   = setmetatable({
                    tblMacrosGlobal = {},
                    bDisabledSource = false,
                    strFile         = nil,
                    nLine           = nil
                }, Preprocessor)

    obj.tblMacrosGlobal["__index"] =
        obj.tblMacrosGlobal
    obj.tblMacrosGlobal["LINE"] =
        MacroBuiltin.New(obj, Preprocessor.MacroLine)
    obj.tblMacrosGlobal["FILE"] =
        MacroBuiltin.New(obj, Preprocessor.MacroFile)

    return obj
end

---@param strInputFile string
---@param strOutputFile string
function Preprocessor:Preprocess(strInputFile, strOutputFile)
    log.assert(fs.access(strInputFile),
        "file doesn't exist: %s", strInputFile)

    local fInput, fOutput =
        io.open(strInputFile, "r"),
        io.open(strOutputFile, "w")

    fInput  = log.assert(fInput,
        "failed to open input file: %s", strInputFile)
    fOutput = log.assert(fOutput,
        "failed to open output file: %s", strOutputFile)

    self.strFile    = log.assert(fs.basename(strInputFile),
                        "failed to get base filename from %s", strInputFile)
    self.nLine      = 1

    for strLine in fInput:lines() do
        local strCmdName, nCmdNameEnd =
            pegParseCommandName:match(strLine)
        if strCmdName and nCmdNameEnd then
            local fnCommand =
                self.tblCommands[strCmdName];
            log.assert(fnCommand,
                "invalid command name: %s", strCmdName)

            local strCmdBody =
                pegParseCommandBody:match(strLine, nCmdNameEnd) or ""
            fnCommand(self, strCmdBody)
        else
            fOutput:write(
                self:ExpandMacros(
                    self:RemoveComments(strLine),
                    self.tblMacrosGlobal))
        end

        fOutput:write("\n")
        self.nLine = self.nLine + 1;
    end

    fInput:close()
    fOutput:close()
end

---@private
---@param strLine string
---@return string
function Preprocessor:RemoveComments(strLine)
    local nCommentBegin =
        pegParseLineComment:match(strLine)

    if nCommentBegin then
        return strLine:sub(1, nCommentBegin - 1)
    else
        return strLine
    end
end

---@private
---@param strChunk string
---@param tblMacros table<string, IMacro>
---@return string
function Preprocessor:ExpandMacros(strChunk, tblMacros)
    while true do
        local nMacroBegin, strMacroName, tblMacroParams, nMacroEnd =
            pegParseMacro:match(strChunk)
        if strMacroName then
            log.assert((-lpeg.P"__"):match(strMacroName),
                "macro and parameter names beginning with '__' are reserved and cannot be used")

            local objMacro =
                tblMacros[strMacroName]
            log.assert(objMacro,
                "undefined macro: %s", strMacroName)
            
            local nExpectedParams, nRealParams =
                objMacro:ParamCount(), #tblMacroParams
            log.assert(nExpectedParams == nRealParams,
                "invalid macro parameter count: expected %i, got %i",
                nExpectedParams, nRealParams)

            local tblMacrosLocal =
                setmetatable({}, tblMacros)
            tblMacrosLocal["__index"] =
                tblMacrosLocal

            for i = 1, nRealParams do
                local strParamName = log.assert(objMacro:ParamName(i),
                    "failed to get param name #%i", i)
                local strParamBody = log.assert(tblMacroParams[i],
                    "failed to get param body #%i", i)

                tblMacrosLocal[strParamName] =
                    MacroDefined.New(strParamBody)
            end

            strChunk = ("%s%s%s"):format(
                strChunk:sub(1, nMacroBegin - 1),
                self:ExpandMacros(objMacro:Body(), tblMacrosLocal),
                strChunk:sub(nMacroEnd))
        else
            return strChunk
        end
    end
end

---@private
---@param strCmdBody string
function Preprocessor:CmdDefine(strCmdBody)
    local strMacroName, tblParamList, strMacroBody =
        pegCmdDefine:match(strCmdBody)
    
    log.assert(strMacroName and tblParamList and strMacroBody,
        "invalid macro definition")

    log.assert((-lpeg.P"__"):match(strMacroName),
        "macro and parameter names beginning with '__' are reserved and cannot be used")
    for i, strParamName in ipairs(tblParamList) do
        log.assert((-lpeg.P"__"):match(strParamName),
            "macro and parameter names beginning with '__' are reserved and cannot be used")
    end

    log.assert(not self.tblMacrosGlobal[strMacroName],
        "macro already exists: %s", strMacroName)
    self.tblMacrosGlobal[strMacroName] =
        MacroDefined.New(strMacroBody, unpack(tblParamList))
end

---@private
---@param strCmdBody string
function Preprocessor:CmdUndef(strCmdBody)
    local strMacroName =
        pegCmdUndef:match(strCmdBody)
    log.assert(strMacroName,
        "invalid macro name, must be a valid identifier")

    log.assert(not oop.isRelatedTo(self.tblMacrosGlobal[strMacroName], MacroBuiltin),
        "undefining builtin macro", strMacroName)
    self.tblMacrosGlobal[strMacroName] = nil
end

Preprocessor.tblCommands["define"] =
    Preprocessor.CmdDefine
Preprocessor.tblCommands["undef"] =
    Preprocessor.CmdUndef

return Preprocessor