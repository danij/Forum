#pragma once

/**
 * To be used when declaring an interface.
 * Move constructor/assignment operator is not implicitly declared if a destructor has been declared.
 * Declaring the move constructor then also requires the declaration of the default and copy constructors
 */
#define DECLARE_INTERFACE_MANDATORY(TypeName) \
    TypeName() = default; \
    virtual ~TypeName() = default; \
    TypeName(const TypeName&) = default; \
    TypeName(TypeName&&) = default; \
    TypeName& operator=(const TypeName&) = default; \
    TypeName& operator=(TypeName&&) = default;

#define DECLARE_INTERFACE_MANDATORY_NO_COPY(TypeName) \
    TypeName() = default; \
    virtual ~TypeName() = default; \
    TypeName(TypeName&&) = default; \
    TypeName& operator=(TypeName&&) = default;
