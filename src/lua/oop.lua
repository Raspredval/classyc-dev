local oop = {}

---@class oop.object
---@field __init    fun(self: oop.object, ...)?
---@field New       fun(...) : oop.object
oop.object = {}
oop.object.__index = oop.object

---@param class table
---@param baseClass oop.object?
---@return oop.object
function oop.newClassFrom(class, baseClass)
    class.__index =
        setmetatable(class, baseClass or oop.object)
    ---@cast class oop.object

    function class:New(...)
        assert(class.__init,
            "creating an instance of abstract class")
        local obj =
            setmetatable({}, class)
        obj:__init(...)
        return obj
    end

    return class
end

---@param baseClass oop.object?
---@return oop.object
function oop.newClass(baseClass)
    local class = {}
    oop.newClassFrom(class, baseClass)
    return class
end

---@param class oop.object
---@return boolean
function oop.object:IsDerivedFrom(class)
    ---@type oop.object?
    local mt =
        getmetatable(self)

    if mt then
        if mt ~= class then
            return mt:IsDerivedFrom(class)
        else
            return true
        end
    else
        return false
    end
end

return oop