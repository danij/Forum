# Fast Forum Backend – API Documentation

The forum software uses UTF-8 for encoding text. 
Unless otherwise specified, the request parameters need to be valid UTF-8 strings (without BOM).

Parameters provided as part of the URL, unless otherwise specified, are mandatory.

Parameters provided as query strings are optional.

> Note: URLs and query strings are case insensitive.

## Authentication

_Coming soon_

## Users

### GET /users

Returns a page of registered users.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastseen`, `threadcount`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /users/online

Returns the users that have recently accessed the application.

### GET /users/id/`userid`

Returns a user, searching by id.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### GET /users/name/`name`

Returns a user, searching by name.

`name` – the name of the user to find.

### GET /users/search/`query`

Returns the index of the first user whose name is greater or equal to the search string, when ordering by name in ascending order.

`query` – the name of the user to search for.

### POST /users

Creates a new user.

Request body – the name of the new user.

### DELETE /users/`userid`

Deletes a user.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /users/name/`userid`

Updates a user's name.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new name.

### PUT /users/info/`userid`

Updates a user's info.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new info.

### PUT /users/title/`userid`

Updates a user's title.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new title.

### PUT /users/signature/`userid`

Updates a user's signature.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new signature.

### GET /users/logo/`userid`

Returns a user's logo (binary).

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /users/logo/`userid`

Updates a user's logo.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new logo in binary format.

### DELETE /users/logo/`userid`

Deletes a user's logo.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### GET /users/votehistory/`userid`

Returns a user's vote history.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### GET /users/subscribed/thread/`threadid`

Returns all users subscribed to a discussion thread. The sort order is unspecified.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

## Discussion Threads

### GET /threads

Returns a page of discussion threads.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastupdated`, `latestmessagecreated`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /threads/id/`threadid`

Returns a discussion thread with a page of it's messages, searched by id. 

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### GET /threads/multiple/`threadid1`,`threadid2`,...

Returns up to a page of discussion threads, searched by id. 

`threadidX` – unique identifiers of the discussion threads to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

The results are returned in the same order as they were searched. Entries that have not been found or that the user does not have access to are returned as null.

### GET /threads/search/`query`

Returns the index of the first discussion thread whose name is greater or equal to the search string, when ordering by name in ascending order.

`query` – the name of the discussion thread to search for.

### GET /threads/user/`userid`

Returns a page of discussion threads of created by a user.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastupdated`, `latestmessagecreated`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /threads/subscribed/user/`userid`

Returns a page of discussion threads to which a user is subscribed.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastupdated`, `latestmessagecreated`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /threads/tag/`tagid`

Returns a page of discussion threads which have a specific tag assigned.

`tagid` – a unique identifier of the discussion tag to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastupdated`, `latestmessagecreated`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /threads/category/`categoryid`

Returns a page of discussion threads which belong to a category.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|OrderBy|Query String|One of: `name`, `created`, `lastupdated`, `latestmessagecreated`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### POST /threads

Creates a new discussion thread.

Request body – the name of the new discussion thread.

### DELETE /threads/`threadid`

Deletes a discussion thread.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /threads/name/`threadid`

Updates a discussion thread's name.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new name.

### PUT /threads/pindisplayorder/`threadid`

Updates a discussion thread's display order when pinned.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new display order (integer).

### POST /threads/merge/`threadfromid`/`threadintoid`

Merges together two discussion threads.

`threadfromid` – a unique identifier of the discussion thread that will be merged, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`threadintoid` – a unique identifier of the discussion thread that will remain after the merge, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /threads/subscribe/`threadid`

Subscribes the current user to a discussion thread.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /threads/unsubscribe/`threadid`

Unsubscribes the current user from a discussion thread.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /threads/tag/`threadid`/`tagid`

Assigns a tag to a discussion thread.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`tagid` – a unique identifier of the discussion tag to add, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### DELETE /threads/tag/`threadid`/`tagid`

Removes a tag from a discussion thread.

`threadid` – a unique identifier of the discussion thread to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`tagid` – a unique identifier of the discussion tag to remove, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

## Discussion Thread Messages

### GET /thread_messages/multiple/`threadid1`,`threadid2`,...

Returns up to a page of discussion thread messages, searched by id. 

`threadidX` – unique identifiers of the discussion threads to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

The results are returned in the same order as they were searched. Entries that have not been found or that the user does not have access to are returned as null.

### GET /thread_messages/user/`userid`

Returns a page of discussion thread messages created by a specific user.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /thread_messages/latest

Returns a page of the most recently created discussion thread messages.

### GET /thread_messages/allcomments

Returns a page of discussion thread message comments, ordered by the creation date/time.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|
 
### GET /thread_messages/comments/`messageid`

Returns all comments associated to a specific discussion thread message, ordered by the creation date/time in descending order

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### GET /thread_messages/comments/user/`userid`

Returns a page of discussion thread message comments created by a specific user, ordered by the creation date/time.

`userid` – a unique identifier of the user to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|Page|Query String|Zero-based page number|0|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /thread_messages/rank/`messageid`

Returns the rank of a discussion thread message relative to it's parent thread. 

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /thread_messages/`threadid`

Creates a new discussion thread message.

`threadid` – a unique identifier of the discussion thread to which the message will belong to, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the message content.

### DELETE /thread_messages/`messageid`

Deletes a discussion thread message.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /thread_messages/content/`messageid`

Updates the content of a discussion thread message.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new content.

### POST /thread_messages/move/`messageid`/`threadid`

Moves a discussion thread message to a new thread.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`threadid` – a unique identifier of the discussion thread to which the message will be moved to, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /thread_messages/upvote/`messageid`

Records an up vote of a discussion thread message by the current user.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /thread_messages/downvote/`messageid`

Records a down vote of a discussion thread message by the current user.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /thread_messages/resetvote/`messageid`

Resets a vote of a discussion thread message cast by the current user.

`messageid` – a unique identifier of the discussion thread message to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /thread_messages/comment/`messageid`

Creates a comment for a specified discussion thread message.

`messageid` – a unique identifier of the discussion thread message to which the comment is addressed, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the content of the new comment.

### PUT /thread_messages/comment/solved/`commentid`

Updates a discussion thread message comment, marking it as solved.

`commentid` – a unique identifier of the discussion thread message comment to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

## Discussion Tags

### GET /tags

Returns all discussion tags.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|OrderBy|Query String|One of: `name`, `threadcount`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### POST /tags

Creates a new discussion tag.

Request body – the name of the new discussion tag.

### DELETE /tags/`tagid`

Deletes a discussion tag.

`tagid` – a unique identifier of the discussion tag to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /tags/name/`tagid`

Updates a discussion tag's name.

`tagid` – a unique identifier of the discussion tag to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new name.

### PUT /tags/uiblob/`tagid`

Updates a discussion tag's UI blob.

`tagid` – a unique identifier of the discussion tag to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new UI blob.

### POST /tags/merge/`tagfromid`/`tagintoid`

Merges together two discussion tags.

`tagfromid` – a unique identifier of the discussion tag that will be merged, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`tagintoid` – a unique identifier of the discussion tag that will remain after the merge, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

## Discussion Categories

### GET /categories

Returns all discussion categories.

|Parameter|Type|Description|Default|
|----|:----:|----|:----:|
|OrderBy|Query String|One of: `name`, `messagecount`|`name`|
|Sort|Query String|One of: `ascending`, `descending`|`ascending`|

### GET /categories/root

Returns all root discussion categories.

### GET /category/`categoryid`

Returns a discussion categories together with it's child categories.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### POST /categories/`parentcategoryid`

Creates a new discussion category.

Request body – the name of the new discussion category.

[OPTIONAL] `parentcategoryid` – a unique identifier of the parent discussion category, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### DELETE /categories/`categoryid`

Deletes a discussion category.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /categories/name/`categoryid`

Updates a discussion category's name.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new name.

### PUT /categories/description/`categoryid`

Updates a discussion category's description.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new description.

### PUT /categories/parent/`categoryid`

Updates a discussion category's parent.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – a unique identifier of the new discussion category parent, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### PUT /categories/displayorder/`categoryid`

Updates a discussion category's display order.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

Request body – the new display order (integer).

### POST /categories/tag/`categoryid`/`tagid`

Assigns a tag to a discussion category.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`tagid` – a unique identifier of the discussion tag to add, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

### DELETE /categories/tag/`categoryid`/`tagid`

Removes a tag from a discussion category.

`categoryid` – a unique identifier of the discussion category to find, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

`tagid` – a unique identifier of the discussion tag to remove, e.g. `00112233-4455-6677-8899-aabbccddeeff`.

## Authorization Privileges

### GET /privileges/required/thread_message

Returns the privileges levels required for actions relating to a specific discussion thread message.

### GET /privileges/assigned/thread_message

Returns the privileges levels assigned to a specific discussion thread message.

### GET /privileges/required/thread

Returns the privileges levels required for actions relating to a specific discussion thread.

### GET /privileges/assigned/thread

Returns the privileges levels assigned to a specific discussion thread.

### GET /privileges/required/tag

Returns the privileges levels required for actions relating to a specific discussion tag.

### GET /privileges/assigned/tag

Returns the privileges levels assigned to a specific discussion tag.

### GET /privileges/required/category

Returns the privileges levels required for actions relating to a specific discussion category.

### GET /privileges/assigned/category

Returns the privileges levels assigned to a specific discussion category.

### GET /privileges/forum_wide/current_user

Returns the forum-wide privileges levels assigned to the current user.

### GET /privileges/required/forum_wide

Returns the privileges levels required for forum-wide actions.

### GET /privileges/defaults/forum_wide

Returns forum-wide default privilege levels that are granted in specific circumstances. 

### GET /privileges/assigned/forum_wide

Returns all forum-wide assigned privilege levels.

### GET /privileges/assigned/user

Returns all privilege levels assigned to a specific user.

### POST /privileges/thread_message/required/thread_message

Updates the required level for a discussion thread message privilege on a discussion thread message. 

### POST /privileges/thread_message/required/thread

Updates the required level for a discussion thread message privilege on a discussion thread. 

### POST /privileges/thread/required/thread

Updates the required level for a discussion thread privilege on a discussion thread. 

### POST /privileges/thread_message/required/tag

Updates the required level for a discussion thread message privilege on a discussion tag. 

### POST /privileges/thread/required/tag

Updates the required level for a discussion thread privilege on a discussion tag. 

### POST /privileges/tag/required/tag

Updates the required level for a discussion tag privilege on a discussion tag. 

### POST /privileges/category/required/category

Updates the required level for a discussion category privilege on a discussion category. 

### POST /privileges/thread_message/required/forum_wide

Updates the forum-wide required level for a discussion thread message privilege. 

### POST /privileges/thread/required/forum_wide

Updates the forum-wide required level for a discussion thread privilege. 

### POST /privileges/tag/required/forum_wide

Updates the forum-wide required level for a discussion tag privilege. 

### POST /privileges/category/required/forum_wide

Updates the forum-wide required level for a discussion category privilege. 

### POST /privileges/forum_wide/required/forum_wide

Updates the forum-wide required level for a forum-wide privilege. 

### POST /privileges/forum_wide/defaults/forum_wide

Updates the forum-wide default privilege levels that are granted in specific circumstances.

### POST /privileges/thread_message/assign

Assigns a privilege level to a discussion thread message for the current user.

### POST /privileges/thread/assign

Assigns a privilege level to a discussion thread for the current user.

### POST /privileges/tag/assign

Assigns a privilege level to a discussion tag for the current user.

### POST /privileges/category/assign

Assigns a privilege level to a discussion category for the current user.

### POST /privileges/forum_wide/assign

Assigns a forum-wide privilege level for the current user.

## Metrics & Statistics

### GET /metrics/version

Returns the current application version.

### GET /statistics/entitycount

Returns the number of: users, discussion threads, discussion thread messages, discussion tags and discussion categories.
