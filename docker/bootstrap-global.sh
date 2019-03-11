#!/bin/bash

HOSTNAME="$1"

function exit_if_not_empty {
    if [ -d "$1" ]; then
        if [ "$(find "$1" -mindepth 1 -print -quit 2>/dev/null)" ]; then
            echo "$1 is not empty"
            exit 1
        fi
    fi
}

exit_if_not_empty /forum/config
exit_if_not_empty /forum/data
exit_if_not_empty /forum/logs
exit_if_not_empty /forum/start
exit_if_not_empty /forum/temp

mkdir /forum/config
mkdir /forum/data
mkdir /forum/logs
mkdir /forum/start
mkdir /forum/temp
mkdir /forum/www

chmod +x /forum/repos/Forum/docker/bootstrap.sh
/bin/bash /forum/repos/Forum/docker/bootstrap.sh "$HOSTNAME"
chmod +x /forum/repos/Forum.Auth/docker/bootstrap.sh
/bin/bash /forum/repos/Forum.Auth/docker/bootstrap.sh "$HOSTNAME"
chmod +x /forum/repos/Forum.Attachments/docker/bootstrap.sh
/bin/bash /forum/repos/Forum.Attachments/docker/bootstrap.sh "$HOSTNAME"
chmod +x /forum/repos/Forum.Search/docker/bootstrap.sh
/bin/bash /forum/repos/Forum.Search/docker/bootstrap.sh "$HOSTNAME"
chmod +x /forum/repos/Forum.WebClient/docker/bootstrap.sh
/bin/bash /forum/repos/Forum.WebClient/docker/bootstrap.sh "$HOSTNAME"

cp /forum/repos/Forum/docker/start.sh /forum/start/start.sh
chmod +x /forum/start/start.sh
