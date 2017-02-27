#pragma once

#include <memory>

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

#define DECLARE_ABSTRACT_MANDATORY(TypeName) DECLARE_INTERFACE_MANDATORY(TypeName)

#define DECLARE_ABSTRACT_MANDATORY_NO_COPY(TypeName) DECLARE_INTERFACE_MANDATORY_NO_COPY(TypeName)


namespace Forum
{
    namespace Helpers
    {
        template<typename T>
        bool ownerEqual(const std::weak_ptr<T>& first, const std::weak_ptr<T>& second)
        {
            return ! first.owner_before(second) && ! second.owner_before(first);
        }
    }
}