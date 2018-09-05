# Installation Guide

The forum backend is composed of multiple services that run behind a reverse proxy.

## 1. Main Service

### Install Dependencies

A C++17 toolchain.

[International Components for Unicode](http://site.icu-project.org/)

[Boost C++ libraries](http://www.boost.org/) (at least version 1.66)

### Retrieve The Sources

The sources of the main backend service are located at [https://github.com/danij/forum](https://github.com/danij/forum).

### Compile

While in the sources folder run the following to create a release build with debug information:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../
    make

### Run Tests

The following tests (accessible from the build folder) can reveal potential problems with the build:

     test/ForumServiceTests/ForumServiceTests
     test/HttpTests/HttpTests
     test/MemoryRepositoryBenchmarks/MemoryRepositoryBenchmarks

### Configure

Using `forum_settings-example.json` as a template, create a configuration file for the service and adjust it to your needs.

Important settings:
* `listenIPAddress` and `authListenIPAddress` should be `127.0.0.1` (localhost) to prevent outside access 
(`authListenIPAddress` should not be exposed to outside clients as that would be a security risk)
* `trustIpFromXForwardedFor` should be `true` when using a reverse proxy
* `expectedOriginReferer` should contain the address of the site (with prefix, usually `https`) and without trailing `/`
* `persistence.inputFolder` should point to where the input data files will be read from
* `persistence.outputFolder` should point to where the output data files will be stored (can be the same as `inputFolder`)
* `logging.settingsFile` should point to the log settings file (this can be configured to log in various locations)
* `libraryPath` for _ForumSearchUpdatePlugin_ should point to the location of the library, with the appropriate extension
* `outputFileNameTemplate` should point to where the search update files should be stored.

### Run

Starting the service requires specifying the location of the configuration file, e.g.:

    src/ForumApp/ForumApp -c ~/Forum/config/forum_settings.json

For better security, run the service under a user with the minimum amount of privileges.

## 2. Authentication Service

### Install Dependencies

[Node.js](https://nodejs.org/en/)

[PostgreSQL](https://www.postgresql.org/) (only needed for custom accounts)

### Retrieve The Sources

The sources of the authentication service are located at [https://github.com/danij/Forum.Auth](https://github.com/danij/Forum.Auth).

### Configure

The configuration values are specified via environment variables.

    #!/bin/sh
    
    export AUTH_COOKIE_SIZE="32"
    export AUTH_SECONDS="600"
    export AUTH_REGISTER_URL="http://127.0.0.1:18081/"
    export DOUBLE_SUBMIT_COOKIE_SIZE="32"
    export PREFIX="while(1);"
    export TRUST_FORWARDED_IP="true"
    export EXPECTED_ORIGIN="https://host without trailing /"
    
    node bin/www > forum-auth.log

#### Custom Authentication

Custom authentication works by storing user accounts in a local database.

Create a new username & database in PostgreSQL and the following tables in that database:
    
    CREATE TABLE logins (
        id SERIAL PRIMARY KEY,
        email VARCHAR(128) UNIQUE NOT NULL,
        email_alphanumeric VARCHAR(128) UNIQUE NOT NULL,
        password VARCHAR(255) NOT NULL,
        password_details VARCHAR(255) NOT NULL,
        auth VARCHAR(128) UNIQUE NOT NULL,
        min_age_at_registration INT NOT NULL,
        created TIMESTAMPTZ DEFAULT now(),
        last_password_change TIMESTAMPTZ DEFAULT now(),
        enabled BOOLEAN NOT NULL DEFAULT false
    );
    
    CREATE TABLE login_confirmations (
        
        id VARCHAR(128) PRIMARY KEY,
        login_id INT NOT NULL REFERENCES logins(id) ON UPDATE CASCADE ON DELETE CASCADE,
        expires TIMESTAMPTZ NOT NULL
    );
    
    CREATE TABLE domain_blacklist (
        domain VARCHAR(128) PRIMARY KEY
    );
    
    CREATE TABLE reset_password_confirmations (
        
        id VARCHAR(128) PRIMARY KEY,
        login_id INT NOT NULL REFERENCES logins(id) ON UPDATE CASCADE ON DELETE CASCADE,
        expires TIMESTAMPTZ NOT NULL
    );

The following settings need also be specified to enable authentication:

    export PGHOST="127.0.0.1"
    export PGUSER="auth user"
    export PGPASSWORD="password"
    export PGDATABASE="auth database"
    export ENABLE_CUSTOM_AUTH="true"
    export PASSWORD_MIN_LENGTH="8"
    export EMAIL_SMTP="smtp server"
    export EMAIL_PORT="587"
    export EMAIL_SECURE="true"
    export EMAIL_USER="username"
    export EMAIL_PASSWORD="password"
    export EMAIL_FROM_NAME="Forum Account Registration"
    export EMAIL_FROM_EMAIL="email address"
    export REGISTER_MIN_AGE="18"
    export REGISTER_AUTH_SIZE="32"
    export REGISTER_CONFIRMATION_SIZE="32"
    export REGISTER_CONFIRMATION_TITLE="Forum Registration Confirmation"
    export REGISTER_CONFIRMATION_URL="https://host/auth/custom/confirm"
    export REGISTER_CONFIRMATION_REDIRECT_URL="https://host"
    export REGISTER_TIMEOUT_SECONDS="600"
    export REGISTER_CUSTOM_AUTH_THROTTLING="30" #only allow registration calls every n seconds/IP
    export LOGIN_CUSTOM_AUTH_THROTTLING="5" #only allow login calls every n seconds/IP
    export CHANGE_PASSWORD_CUSTOM_AUTH_THROTTLING="5" #only allow change password calls every n seconds/IP
    export RESET_PASSWORD_CUSTOM_AUTH_THROTTLING="5" #only allow reset password calls every n seconds/IP
    export RESET_PASSWORD_CONFIRMATION_TITLE="Forum Reset Password Confirmation"
    export RESET_PASSWORD_CONFIRMATION_URL="https://host/auth/custom/confirm_reset_password"
    export RESET_PASSWORD_TIMEOUT_SECONDS="600"
    export RESET_PASSWORD_NEW_PASSWORD_SIZE="16"
    
The confirmation email template can be adjusted at `register_confirmation_template.html`.

The reset password email template can be adjusted at `reset_password_confirmation_template.html`.

#### Authentication Using External Providers

User authentication can be performed via external providers.

To allow users to log in with Google accounts via OAuth 2, please register the application and configure the following:

    export GOOGLE_CLIENT_ID="client id"
    export GOOGLE_CLIENT_SECRET="client secret"

### Run

Execute the above created script under a user with the minimum amount of privileges 
(a different user than the one running other services).

## 3. Search Service

### Install Dependencies

[Node.js](https://nodejs.org/en/)

[PostgreSQL](https://www.postgresql.org/)

Create a new username & database in PostgreSQL and the following tables in that database:
    
    CREATE TABLE threads (
        id UUID PRIMARY KEY,
        name tsvector
    );
    CREATE INDEX idx_fts_threads ON threads USING gin(name);
    
    CREATE TABLE thread_messages (
        id UUID PRIMARY KEY,
        content tsvector
    );
    CREATE INDEX idx_fts_thread_messages ON thread_messages USING gin(content);

### Retrieve The Sources

The sources of the authentication service are located at [https://github.com/danij/Forum.Search](https://github.com/danij/Forum.Search).

### Configure

The configuration values are specified via environment variables.

    #!/bin/sh
    
    export PORT="8082"
    export PGHOST="127.0.0.1"
    export PGUSER="search user"
    export PGPASSWORD="password"
    export PGDATABASE="search database"
    
    node bin/www > forum-search.log

### Run

Execute the above created script under a user with the minimum amount of privileges 
(a different user than the one running other services).

## 4. Web Front End

### Install Dependencies

[Node.js](https://nodejs.org/en/)

### Retrieve The Sources

The sources of the authentication service are located at [https://github.com/danij/Forum.WebClient](https://github.com/danij/Forum.WebClient).

### Install NPM Dependencies

    npm install

### Build

The following command creates a `dist` folder which will contain the markup, media, scripts and stylesheets that need to be served.

    webpack --config webpack.production.js

### Configure

Edit `dist/config/config.js` and adjust the settings.

Edit `dist/doc/privacy.md` and add an appropriate Privacy Policy.

Edit `dist/dic/terms_of_service.md` and add appropriate Terms Of Service.

> Please make a backup of your configuration as running webpack again will override it with the defaults.

## 5. Putting it all together

### Install Dependencies

[Nginx](https://www.nginx.com/)

### Configure

Edit the `nginx` sites configuration (located, e.g. under `/etc/nginx/sites-available/default` or `/etc/nginx/nginx.conf`)

    add_header Content-Security-Policy "default-src 'self'; img-src 'self' data:; style-src 'self' 'unsafe-inline'; frame-src https://www.youtube.com/" always;
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Xss-Protection "1; mode=block" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header Referrer-Policy "same-origin" always;
    add_header Feature-Policy "geolocation 'none'; midi 'none'; notifications 'none'; push 'none'; sync-xhr 'none'; microphone 'none'; camera 'none'; magnetometer 'none'; gyroscope 'none'; speaker 'none'; vibrate 'none'; fullscreen 'none'; payment 'none';" always;

    location / {
        alias  location of dist/ folder (or copy its content to the default website root folder);
        index  index.html index.htm;
    
        rewrite ^/home.*$ /index.html last;
        rewrite ^/category.*$ /index.html last;
        rewrite ^/tag.*$ /index.html last;
        rewrite ^/thread.*$ /index.html last;
        rewrite ^/thread_message.*$ /index.html last;
        rewrite ^/user.*$ /index.html last;
        rewrite ^/documentation.*$ /index.html last;
    }
    
    location /api {
        rewrite /api(.*) /$1  break;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_pass http://127.0.0.1:8081;
    }
    
    location /auth {
        rewrite /auth(.*) $1  break;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_pass http://127.0.0.1:3000;
    }
    
    location /search {
        rewrite /search(.*) $1  break;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_pass http://127.0.0.1:8082;
    }

The port that nginx is listening on is the only one that needs to be exposed to users, the other internal ones should only be accesible from localhost.
