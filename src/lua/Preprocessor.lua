local oop, lpeg, PEG, fs, log, FileInfo, MacroDefined, MacroBuiltin, MacroExpansion =
    require "oop",
    require "lpeg",
    require "CommonPatterns",
    require "fs",
    require "log",
    require "FileInfo",
    require "Preprocessor.MacroDefined",
    require "Preprocessor.MacroBuiltin",
    require "Preprocessor.MacroExpansion"

---@type Pattern
local pegParseCommandName =
    PEG.Padding *
    ("#" * lpeg.C(PEG.ID)) * lpeg.Cp()

---@type Pattern
local pegParseCommandBody =
    PEG.Padding *
    lpeg.C(lpeg.P(1) ^ 1)

---@diagnostic disable missing-fields

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
    lpeg.C(PEG.ID) *
    lpeg.Ct(("(" * PEG.NameList * ")") ^ -1) *
    PEG.Space * lpeg.C(lpeg.P(1) ^ 1)

---@type Pattern
local pegCmdUndef =
    lpeg.C(PEG.ID) * PEG.Padding * PEG.EOF

---@type Pattern
local pegParseLocalFile =
    "\"" * lpeg.C((lpeg.P(1) - "\"") ^ 0) * "\"" * PEG.Padding * PEG.EOF

---@type Pattern
local pegParseSystemFile =
    "<" * lpeg.C((lpeg.P(1) - ">") ^ 0) * ">" * PEG.Padding * PEG.EOF

---@alias Command fun(self: Preprocessor, strCmdBody: string)

---@class Preprocessor
---@field private tblMacrosGlobal       MacroLookupTable
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
    return tostring(cur_file:Line())
end

---@private
---@return string
function Preprocessor:MacroFile()
    local cur_file  = self:CurInputFile()
    return ("\"%s\""):format(cur_file:Name())
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
    self.objOutputFile  = FileInfo.New(strFilename, "w")
end

function Preprocessor:CloseOutputFile()
    self.objOutputFile = nil
end

---@private
---@param strFilename string
function Preprocessor:PushInputFile(strFilename)
    local strRealPath   = log.assert(fs.realpath(strFilename),
                            "failed to get real path from file: %s", strFilename)

    local nFileCount    = #self.tblInputFiles
    self.tblInputFiles[nFileCount + 1] =
        FileInfo.New(strRealPath, "r")

    self:OutputFile():File():write(
        "#FILE_BEG ", strRealPath, "\n")
end

---@private
function Preprocessor:PopInputFile()
    local nFileCount    = #self.tblInputFiles
    if nFileCount > 0 then
        self:OutputFile():File():write(
            "#FILE_END ", self.tblInputFiles[nFileCount]:Name(), "\n")
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
    
    local input, output =
        self:CurInputFile(), self:OutputFile()

    for strLine in input:File():lines() do
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
            local macro_expansion =
                MacroExpansion.New(self:CurInputFile(), self.tblMacrosGlobal)

            output:File():write(
                macro_expansion:ExpandMacros(
                    self:RemoveComments(strLine)),
                "\n")
        end

        output:IncrLine()
        input:IncrLine()
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

    self:OutputFile():File():write("\n")
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

    self:OutputFile():File():write("\n")
end

---@private
---@param strLocalFile string
---@return string
function Preprocessor:GetLocalFilePath(strLocalFile)
    local input_file    = self:CurInputFile()
    local strDirname    = log.assert(fs.dirname(input_file:Name()),
                            "failed to get directory name of the source file: %s",
                            input_file:Name())
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