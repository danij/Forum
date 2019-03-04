#!/bin/bash
HOSTNAME="$1"

mkdir /forum/config/Forum
mkdir /forum/data/events
mkdir /forum/data/search_updates
mkdir /forum/logs/Forum

cp /forum/repos/Forum/forum_settings-example.json /forum/config/Forum/forum_settings.json
sed -i 's#log.settings#/forum/config/Forum/log.settings#' /forum/config/Forum/forum_settings.json
sed -i 's#/mnt/forum/input#/forum/data/events#' /forum/config/Forum/forum_settings.json
sed -i 's#/mnt/forum/output#/forum/data/events#' /forum/config/Forum/forum_settings.json
sed -i 's#ForumSearchUpdatePlugin.so#/forum/repos/Forum-RelWithDebInfo/plugins/ForumSearchUpdatePlugin/libForumSearchUpdatePlugin.so#' /forum/config/Forum/forum_settings.json
sed -i 's#search_updates-%1%.json#/forum/data/search_updates/search_updates-%1%.json#' /forum/config/Forum/forum_settings.json
sed -i "s#dani.forum#$HOSTNAME#" /forum/config/Forum/forum_settings.json

cp /forum/repos/Forum/example-log.settings /forum/config/Forum/log.settings
sed -i 's#file_%Y%m%d.log#/forum/logs/Forum/%Y%m%d.log#' /forum/config/Forum/log.settings
