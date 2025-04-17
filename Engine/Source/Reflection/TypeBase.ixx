export module Reflection:TypeBase;

import Common;

export struct Field
{
    std::string_view TypeName;
    std::string_view FieldName;

    Field(std::string_view typeName, std::string_view fieldName) : TypeName(typeName), FieldName(fieldName) {}
};

export class Reference;