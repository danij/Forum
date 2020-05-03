#!/bin/bash 

su -c '/usr/lib/postgresql/12/bin/pg_ctl -D /forum/data/sqldb -l /forum/logs/sqldb/logfile start' postgres
service nginx start
service cron start

/forum/start/Forum.Auth.sh &
/forum/start/Forum.Attachments.sh &
/forum/start/Forum.Search.sh &

/forum/repos/Forum-RelWithDebInfo/src/ForumApp/ForumApp -c /forum/config/Forum/forum_settings.json
