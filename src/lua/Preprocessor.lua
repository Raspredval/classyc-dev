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
    literal         = PEG.StringLiteral + PEG.CharLiteral,
    comment         = lpeg.Cp() * "//",
    first_comment   = lpeg.V"text" * (lpeg.V"literal" * lpeg.V"text") ^ 0 * lpeg.V"comment"
}

---@diagnostic enable missing-fields

---@type Pattern
local pegCmdDefine =
    lpeg.C(PEG.ID) *
    lpeg.Ct(("(" * PEG.IDList * ")") ^ -1) *
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
---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function Preprocessor:MacroCurLine(strMacroName, objFileInfo, tblMacros, tblParams)
    local nParamNameCount   = #tblParams
    objFileInfo:Assert(nParamNameCount == 0,
        "%s macro param mismatch -- expected 0, got %i",
        strMacroName, nParamNameCount)

    return tostring(objFileInfo:Line())
end

---@private
---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function Preprocessor:MacroCurFilePath(strMacroName, objFileInfo, tblMacros, tblParams)
    local nParamNameCount   = #tblParams
    objFileInfo:Assert(nParamNameCount == 0,
        "%s macro param mismatch -- expected 0, got %i",
        strMacroName, nParamNameCount)

    local strFilePath   = objFileInfo:Path()

    return ("\"%s\""):format(strFilePath)
end

---@private
---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function Preprocessor:MacroCurFileName(strMacroName, objFileInfo, tblMacros, tblParams)
    local nParamNameCount   = #tblParams
    objFileInfo:Assert(nParamNameCount == 0,
        "%s macro param mismatch -- expected %0, got %i",
        strMacroName, nParamNameCount)

    local strFilePath   = objFileInfo:Path()
    local strBaseName   = log.assert(fs.basename(strFilePath),
                            "failed to get file base name: %s",
                            strFilePath)

    return ("\"%s\""):format(strBaseName)
end

---@private
---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function Preprocessor:MacroCurFileDir(strMacroName, objFileInfo, tblMacros, tblParams)
    local tblMacroParams    = #tblParams
    local nParamNameCount   = #tblMacroParams
    objFileInfo:Assert(nParamNameCount == 0,
        "%s macro param mismatch -- expected 0, got %i",
        strMacroName, nParamNameCount)

    local strFilePath   = objFileInfo:Path()
    local strDirName    = log.assert(fs.dirname(strFilePath),
                            "failed to get file dir name: %s",
                            strFilePath)

    return ("\"%s\""):format(strDirName)
end

---@private
---@param strMacroName string
---@param objFileInfo FileInfo
---@param tblMacros MacroLookupTable
---@param tblParams string[]
---@return string
function Preprocessor:MacroDefined(strMacroName, objFileInfo, tblMacros, tblParams)
    local tblMacroParams    = #tblParams
    local nParamNameCount   = #tblMacroParams
    objFileInfo:Assert(nParamNameCount == 1,
        "%s macro param mismatch -- expected 1, got %i",
        strMacroName, nParamNameCount)

    local strParamMacroName  = tblParams[1]
    objFileInfo.Assert((PEG.ID * PEG.EOF):match(strParamMacroName),
        "% macro invalid param -- expected an identifier, got %s",
        strMacroName, strParamMacroName)

    if tblMacros[strParamMacroName] then
        return "1"
    else
        return "0"
    end
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
    obj.tblMacrosGlobal["CUR_LINE"] =
        MacroBuiltin.New(obj, Preprocessor.MacroCurLine)
    obj.tblMacrosGlobal["CUR_FILE_PATH"] =
        MacroBuiltin.New(obj, Preprocessor.MacroCurFilePath)
    obj.tblMacrosGlobal["CUR_FILE_NAME"] =
        MacroBuiltin.New(obj, Preprocessor.MacroCurFileName)
    obj.tblMacrosGlobal["CUR_FILE_DIR"] =
        MacroBuiltin.New(obj, Preprocessor.MacroCurFileDir)
    local objMacroDefined =
        MacroBuiltin.New(obj, Preprocessor.MacroDefined)
    obj.tblMacrosGlobal["DEFINED"] =
        objMacroDefined
    obj.tblMacrosGlobal["defined"] =
        objMacroDefined

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
            "#FILE_END ", self.tblInputFiles[nFileCount]:Path(), "\n")
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

    log.assert(not PEG.IsStringEOF,
        "missing closing quotes '\"'")

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
    local strDirname    = log.assert(fs.dirname(input_file:Path()),
                            "failed to get directory name of the source file: %s",
                            input_file:Path())
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