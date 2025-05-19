local oop, IMacro =
    require "oop",
    require "Preprocessor.IMacro"

---@class MacroDefined : IMacro
---@field private strBody string
local MacroDefined = oop.newClass(IMacro)

---@param strMacroBody string
---@param ... string
---@return MacroDefined
function MacroDefined.New(strMacroBody, ...)
    local obj   = setmetatable({
                    strBody = strMacroBody,
                    ...
                }, MacroDefined)

    return obj
end

---@return string
function MacroDefined:Body()
    return self.strBody
end

---@param i integer
---@return string?
function MacroDefined:ParamName(i)
    return self[i]
end

---@return integer
function MacroDefined:ParamCount()
    return #self
end

return MacroDefined
