#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

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
            T* getEntityFromGlobalCollection(size_t index)
            {
                throw std::runtime_error("Should no be called");
            }

            template<>
            User* getEntityFromGlobalCollection<User>(size_t index);

            template<>
            DiscussionThread* getEntityFromGlobalCollection<DiscussionThread>(size_t index);

            template<>
            DiscussionThreadMessage* getEntityFromGlobalCollection<DiscussionThreadMessage>(size_t index);

            template<>
            DiscussionTag* getEntityFromGlobalCollection<DiscussionTag>(size_t index);

            template<>
            DiscussionCategory* getEntityFromGlobalCollection<DiscussionCategory>(size_t index);

            template<>
            MessageComment* getEntityFromGlobalCollection<MessageComment>(size_t index);
        }
        
        template<typename T>
        class StoresEntityPointer;

        /**
         * Stores a pointer to an entity as an index into a collection
         * The collection will be part of a singleton EntityCollection
         */
        template<typename T>
        class EntityPointer final
        {
        public:
            typedef int32_t IndexType;

            typedef T element_type;

            typedef typename std::add_const<T>::type ConstT;
            typedef typename std::add_lvalue_reference<T>::type RefT;
            typedef typename std::add_pointer<T>::type PtrT;
            typedef typename std::add_lvalue_reference<typename std::add_const<T>::type>::type RefToConstT;
            typedef typename std::add_pointer<typename std::add_const<T>::type>::type PtrToConstT;

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

            explicit operator EntityPointer<ConstT>() const
            {
                return EntityPointer<ConstT>(index_);
            }

            auto toConst() const
            {
                return operator EntityPointer<ConstT>();
            }

            PtrToConstT ptr() const
            {
                if ( ! operator bool()) return nullptr;
                return Private::getEntityFromGlobalCollection<typename std::remove_const<T>::type>(index_);
            }

            PtrT ptr()
            {
                if ( ! operator bool()) return nullptr;
                return Private::getEntityFromGlobalCollection<typename std::remove_const<T>::type>(index_);
            }

            RefToConstT operator*() const
            {
                assert(operator bool());
                return *ptr();
            }

            RefT operator*()
            {
                assert(operator bool());
                return *ptr();
            }

            PtrToConstT operator->() const
            {
                assert(operator bool());
                return ptr();
            }

            PtrT operator->()
            {
                assert(operator bool());
                return ptr();
            }
            
            operator PtrToConstT() const
            {
                return ptr();
            }

            operator PtrT()
            {
                return ptr();
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
        
        template<typename T>
        class StoresEntityPointer
        {
        public:
            auto pointer() const { return pointer_; }

            friend class EntityCollection;
        private:
            EntityPointer<T> pointer_;
        };
    }
}
