local oop = {}

---@param tbl table
---@param baseClass table?
---@return table
function oop.setAsClass(tbl, baseClass)
    tbl.__index = tbl
    setmetatable(tbl, baseClass)
    return tbl
end

---@param baseClass table?
---@return table
function oop.newClass(baseClass)
    local class = {}
    oop.setAsClass(class, baseClass)
    return class
end

---@param obj table
---@param class table
---@return boolean
function oop.isRelatedTo(obj, class)
    local mt = getmetatable(obj)
    if mt then
        if mt ~= class then
            return oop.isRelatedTo(mt, class)
        else
            return true
        end
    else
        return false
    end
end

return oop