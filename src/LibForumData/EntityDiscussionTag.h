#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollectionBase.h"

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct DiscussionCategory;

        /**
        * Stores a discussion tag that groups threads of similar discussions
        * Repositories are responsible for updating the relationships between this message and other entities
        * 
        * The tag manages the message count and also notifies any discussion categories when a thread is added or removed
        */
        struct DiscussionTag final : public Identifiable, public CreatedMixin, public LastUpdatedMixin<User>,
                                     public DiscussionThreadCollectionBase, public IndicateDeletionInProgress
        {
            const std::string&  name()           const { return name_; }
                  std::string&  name()                 { return name_; }
                                                 
            const std::string&  uiBlob()         const { return uiBlob_; }
                  std::string&  uiBlob()               { return uiBlob_; }
                                                
                  int_fast32_t  messageCount()   const { return messageCount_; }
                  int_fast32_t& messageCount()         { return messageCount_; }
                  
                  auto          categories()     const { return Helpers::toConst(categories_); }
                  auto&         categoriesWeak()       { return categories_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                UIBlob
            };

            DiscussionTag() : notifyChangeFn_(&DiscussionTag::emptyNotifyChange) {  }

            bool insertDiscussionThread(const DiscussionThreadRef& thread) override;
            DiscussionThreadRef deleteDiscussionThread(DiscussionThreadCollection::iterator iterator) override;

            bool addCategory(std::weak_ptr<DiscussionCategory> category)
            {
                return std::get<1>(categories_.insert(std::move(category)));
            }

            bool removeCategory(const std::weak_ptr<DiscussionCategory>& category)
            {
                return categories_.erase(category) > 0;
            }
            
            typedef std::function<void(DiscussionTag&)> NotifyChangeActionType;
            auto& notifyChange() { return notifyChangeFn_; }

        private:
            std::string name_;
            std::string uiBlob_;
            int_fast32_t messageCount_ = 0;
            std::set<std::weak_ptr<DiscussionCategory>, std::owner_less<std::weak_ptr<DiscussionCategory>>> categories_;
            NotifyChangeActionType notifyChangeFn_;

            static void emptyNotifyChange(DiscussionTag& tag) { }
        };

        typedef std::shared_ptr<DiscussionTag> DiscussionTagRef;
        typedef std::  weak_ptr<DiscussionTag> DiscussionTagWeakRef;
    }
}
