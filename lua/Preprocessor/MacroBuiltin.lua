local oop, IMacro =
    require "oop",
    require "Preprocessor.IMacro"

---@class MacroBuiltin : IMacro
---@field private objPreprocessor Preprocessor
---@field private fnGetBody fun(obj: Preprocessor) : string
local MacroBuiltin = oop.newClass(IMacro)

---@param objPreprocessor Preprocessor
---@param fnGetBody fun(obj: Preprocessor) : string
---@return MacroBuiltin
function MacroBuiltin.New(objPreprocessor, fnGetBody)
    local obj   = setmetatable({
                    objPreprocessor = objPreprocessor,
                    fnGetBody = fnGetBody
                }, MacroBuiltin)

    return obj
end

---@return string
function MacroBuiltin:Body()
    return self.fnGetBody(self.objPreprocessor)
end

return MacroBuiltin
