#pragma once
#ifndef TYPE_BASE_H
#define TYPE_BASE_H
#include <cstdint>
#include <Common/MacroHelpers.hpp>

// Welcome to the horrible land of macroland where your eyes
//  will bleed but incredible power will be bestowed upon who reads this anomaly

#ifdef EDITOR
#define DRAW_EDITOR bool DrawEditorInfo() override;
#else
#define DRAW_EDITOR
#endif

#define SCLASS(_name, _parent) \
    static_assert(Reflection::IsReflected<_name>, "No reflection symbols detected for type " #_name ". The current project generated module is probably missing or you need to compile again.\n" ); \
    using Super = _parent; \
protected : \
    constexpr bool IsOfType(Type targetType) const override \
    { return (targetType == Type::GetType<_name>() || Super::IsOfType(targetType)); } \
public : \
    constexpr Type GetType() const override { return Type::GetType<_name>(); } \
    constexpr StaticString GetTypeName() const override { return Type::GetTypeName<_name>(); } \
    template<class T> friend class Class;  friend class Class<_name>; \
    EXPAND(DRAW_EDITOR) \
protected: \
[[nodiscard]] Base* VirtualCopy() const override { return InternalCopy<_name>(); }; \
private : static_assert(true, "") // requires semicolon

// For non root base types that includes non-agnostic type reflection
#define SSTRUCT(_name, _parent) \
static_assert(Reflection::IsReflected<_name>, "No reflection symbols detected for type " #_name ". The current project generated module is probably missing or you need to compile again.\n" ); \
private: \
    using Super = _parent; \
protected : \
constexpr bool IsOfType(Type targetType) const override \
{ return (targetType == Type::GetType<_name>() || Super::IsOfType(targetType)); } \
public : \
    constexpr Type GetType() const override { return Type::GetType<_name>(); } \
    constexpr StaticString GetTypeName() const override { return Type::GetTypeName<_name>(); } \
    template<class T> friend class Class; friend class Class<_name>; \
    EXPAND(DRAW_EDITOR) \
protected: \
    [[nodiscard]] Base* VirtualCopy() const override { return InternalCopy<_name>(); }; \
public :  static_assert(true, "") // requires semicolon

#define COMPONENT_DEFAULT_CONSTRUCTOR(_name) \
    _name() {}  static_assert(true, "") // requires semicolon

#define EDITOR_ONLY() static_assert(true, "") // requires semicolon

#define MANUALLY_DEFINED_EDITOR() static_assert(true, "") // requires semicolon

#endif // TYPE_BASE_H
