#pragma once

/**
 * To be used when declaring an interface.
 * Move constructor/assignment operator is not implicitly declared if a destructor has been declared.
 */
#define DECLARE_INTERFACE_MANDATORY(TypeName) \
    TypeName() = default; \
    virtual ~TypeName() = default; \
    TypeName(const TypeName&) = default; \
    TypeName(TypeName&&) = default; \
    TypeName& operator=(const TypeName&) = default; \
    TypeName& operator=(TypeName&&) = default;
