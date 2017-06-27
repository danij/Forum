#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace Forum
{
    namespace Entities
    {
        class EntityCollection;
        class User;
        class DiscussionThread;
        class DiscussionThreadMessage;
        class DiscussionTag;
        class DiscussionCategory;
        class MessageComment;

        namespace Private
        {
            EntityCollection& getGlobalEntityCollection();
            void setGlobalEntityCollection(EntityCollection* collection);
            
            template<typename T>
            T& getEntityFromGlobalCollection(size_t index)
            {
                throw std::runtime_error("Should no be called");
            }

            template<>
            User& getEntityFromGlobalCollection<User>(size_t index);

            template<>
            DiscussionThread& getEntityFromGlobalCollection<DiscussionThread>(size_t index);

            template<>
            DiscussionThreadMessage& getEntityFromGlobalCollection<DiscussionThreadMessage>(size_t index);

            template<>
            DiscussionTag& getEntityFromGlobalCollection<DiscussionTag>(size_t index);

            template<>
            DiscussionCategory& getEntityFromGlobalCollection<DiscussionCategory>(size_t index);

            template<>
            MessageComment& getEntityFromGlobalCollection<MessageComment>(size_t index);
        }
        /**
         * Stores a pointer to an entity as an index into a collection
         * The collection will be part of a singleton EntityCollection
         */
        template<typename T>
        class EntityPointer final
        {
        public:
            typedef int32_t IndexType;
            
            static constexpr IndexType Invalid = -1;
            
            EntityPointer() : index_(Invalid)
            {}
            
            explicit EntityPointer(IndexType index) : index_(index)
            {}

            EntityPointer(nullptr_t) : EntityPointer()
            {}

            EntityPointer(const EntityPointer&) = default;
            EntityPointer(EntityPointer&&) = default;

            EntityPointer& operator=(const EntityPointer&) = default;
            EntityPointer& operator=(EntityPointer&&) = default;

            operator bool() const
            {
                return Invalid != index_;
            }

            const T& operator*() const
            {
                return *this;
            }

            T& operator*()
            {
                if ( ! *this) throw std::runtime_error("Invalid EntityPointer dereferenced");
                return Private::getEntityFromGlobalCollection<T>(index_);
            }

            const T* operator->() const
            {
                return &(*this);
            }

            T* operator->()
            {
                return &(*this);
            }
            
            bool operator==(EntityPointer other) const
            {
                return index_ == other.index_;
            }

            bool operator!=(EntityPointer other) const
            {
                return index_ != other.index_;
            }

            bool operator<(EntityPointer other) const
            {
                return index_ < other.index_;
            }

            bool operator<=(EntityPointer other) const
            {
                return index_ <= other.index_;
            }

            bool operator>(EntityPointer other) const
            {
                return index_ > other.index_;
            }

            bool operator>=(EntityPointer other) const
            {
                return index_ >= other.index_;
            }

        private:
            IndexType index_;
        };
    }
}
