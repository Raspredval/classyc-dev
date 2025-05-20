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

---@type Pattern
local pegParseLocalFile =
    "\"" * lpeg.C((lpeg.P(1) - "\"") ^ 0) * "\"" * pegPadding * pegEOF

---@type Pattern
local pegParseSystemFile =
    "<" * lpeg.C((lpeg.P(1) - ">") ^ 0) * ">" * pegPadding * pegEOF

---@alias Command fun(self: Preprocessor, strCmdBody: string)

---@alias MacroInfo table<string, IMacro>

---@class FileInfo
---@field public strName    string
---@field public nLine      integer
---@field public fData      file*

---@class Preprocessor
---@field private tblMacrosGlobal       MacroInfo
---@field private tblInputFiles         FileInfo[]
---@field private tblAlreadyRequired    table<string, boolean>
---@field private objOutputFile         FileInfo?
---@field private bDisabledSource       boolean
--- static fields:
---@field private tblCommands           table<string, Command>
local Preprocessor = oop.setAsClass({
        tblCommands = {}
    })

---@private
---@return string
function Preprocessor:MacroLine()
    local cur_file  = self:CurInputFile()
    return tostring(cur_file.nLine)
end

---@private
---@return string
function Preprocessor:MacroFile()
    local cur_file  = self:CurInputFile()
    return ("\"%s\""):format(cur_file.strName)
end

---@return Preprocessor
function Preprocessor.New()
    local obj   = setmetatable({
                    tblMacrosGlobal     = {},
                    tblInputFiles       = {},
                    tblAlreadyRequired  = {},
                    objOutputFile       = nil,
                    bDisabledSource     = false
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
    self:OpenOutputFile(strOutputFile)
    self:RecursiveParsing(strInputFile)
    self:CloseOutputFile()
end

---@private
---@param strFilename string
function Preprocessor:OpenOutputFile(strFilename)
    local fOutputFile   = log.assert(io.open(strFilename, "w"),
                            "failed to open output file: %s", strFilename)
    self.objOutputFile  = {
        strName = strFilename,
        nLine   = 1,
        fData   = fOutputFile
    }
end

function Preprocessor:CloseOutputFile()
    assert(self.objOutputFile,
        "no output file present")
    self.objOutputFile.fData:close()
    self.objOutputFile = nil
end

---@private
---@param strFilename string
function Preprocessor:PushInputFile(strFilename)
    local strRealPath   = log.assert(fs.realpath(strFilename),
                            "failed to get real path from file: %s", strFilename)

    local fInputFile    = log.assert(io.open(strRealPath, "r"),
                            "failed to open input file: %s", strFilename)

    local nFileCount    = #self.tblInputFiles
    self.tblInputFiles[nFileCount + 1] = {
        strName = strRealPath,
        nLine   = 1,
        fData   = fInputFile
    }

    self:OutputFile().fData:write(
        "#FILE_BEG ", strRealPath, "\n")
end

---@private
function Preprocessor:PopInputFile()
    local nFileCount    = #self.tblInputFiles
    if nFileCount > 0 then
        self:OutputFile().fData:write(
            "#FILE_END ", self.tblInputFiles[nFileCount].strName, "\n")
        self.tblInputFiles[nFileCount].fData:close()
        self.tblInputFiles[nFileCount] = nil
    end
end

---@private
---@return FileInfo
function Preprocessor:CurInputFile()
    local nFileCount    = #self.tblInputFiles
    log.assert(nFileCount > 0,
        "no source file info present")

    return self.tblInputFiles[nFileCount]
end

---@private
---@return FileInfo
function Preprocessor:OutputFile()
    return self.objOutputFile
end

---@private
---@param strInputFile string
function Preprocessor:RecursiveParsing(strInputFile)
    self:PushInputFile(strInputFile)
    
    local input_file, output_file =
        self:CurInputFile(), self:OutputFile()

    for strLine in input_file.fData:lines() do
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
            output_file.fData:write(
                self:ExpandMacros(
                    self:RemoveComments(strLine),
                    self.tblMacrosGlobal))
            output_file.fData:write("\n")
        end

        output_file.nLine =
            output_file.nLine + 1
        input_file.nLine =
            input_file.nLine + 1
    end

    self:PopInputFile()
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

    self:OutputFile().fData:write("\n")
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

    self:OutputFile().fData:write("\n")
end

---@private
---@param strLocalFile string
---@return string
function Preprocessor:GetLocalFilePath(strLocalFile)
    local input_file    = self:CurInputFile()
    local strDirname    = log.assert(fs.dirname(input_file.strName),
                            "failed to get directory name of the source file: %s",
                            input_file.strName)
    local strFilename   = ("%s/%s"):format(strDirname, strLocalFile)
    
    log.assert(fs.access(strFilename),
        "file doesn't exist: %s", strLocalFile)

    return strFilename
end

---@private
---@param strCmdBody string
function Preprocessor:CmdRequire(strCmdBody)
    local strLocalFile, strSystemFile =
        pegParseLocalFile:match(strCmdBody),
        pegParseSystemFile:match(strCmdBody)
    
    local strFilename   = self:GetLocalFilePath(
                            log.assert(strLocalFile or strSystemFile,
                                "expected file name, got %s", strCmdBody))
    local strRealPath   = log.assert(fs.realpath(strFilename),
                            "failed to get real path from file: %s",
                            strFilename)

    if not self.tblAlreadyRequired[strRealPath] then
        self.tblAlreadyRequired[strRealPath] = true
        self:RecursiveParsing(strFilename)
    end
end

function Preprocessor:CmdInsert(strCmdBody)
    local strLocalFile, strSystemFile =
        pegParseLocalFile:match(strCmdBody),
        pegParseSystemFile:match(strCmdBody)
    
    local strFilename   = self:GetLocalFilePath(
                            log.assert(strLocalFile or strSystemFile,
                                "expected file name, got %s", strCmdBody))
    local strRealPath   = log.assert(fs.realpath(strFilename),
                            "failed to get real path from file: %s",
                            strFilename)

    self:RecursiveParsing(strRealPath)
end

Preprocessor.tblCommands["define"]  =
    Preprocessor.CmdDefine
Preprocessor.tblCommands["undef"]   =
    Preprocessor.CmdUndef
Preprocessor.tblCommands["require"] =
    Preprocessor.CmdRequire
Preprocessor.tblCommands["insert"]  =
    Preprocessor.CmdInsert

return Preprocessor