#include "EntityUser.h"

using namespace Forum::Entities;

User::ChangeNotification User::changeNotifications_;
const std::set<DiscussionThreadMessagePtr> User::emptyVotedMessages_;
const MessageCommentCollection User::emptyMessageComments_;
