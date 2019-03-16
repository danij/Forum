# Running Fast Forum via Docker

The forum can also be run using [Docker](https://www.docker.com/) containers.

## Step 0 – Building the Image

An image definition has been prepared. It is rather fat, encapsulating all the application services and dependencies
in a single container (for ease of use).  

The image can be built using a single command:

    docker build -t example/fastforum:v1RC4 .

## Step 1 – Setting up Folders

The forum container is designed to be immutable, relying on docker volumes to store all the data and logs on the host.

After deciding on where the data will be stored, create the following directories.  

    mkdir insert_folder/forum
    mkdir insert_folder/forum/config
    mkdir insert_folder/forum/data
    mkdir insert_folder/forum/logs
    mkdir insert_folder/forum/start
    mkdir insert_folder/forum/temp
    mkdir insert_folder/forum/www

## Step 2 – Bootstrapping

The forum container contains a bootstrap script that sets up the forum to use the directories created 
in the previous step.

This script will also preconfigure the forum to use a hostname of your choosing.

     docker run \
     -v insert_folder/forum/data:/forum/data \
     -v insert_folder/forum/logs:/forum/logs \
     -v insert_folder/forum/start:/forum/start \
     -v insert_folder/forum/temp:/forum/temp \
     -v insert_folder/forum/www:/var/www \
     -v insert_folder/forum/config:/forum/config \
     example/fastforum:v1RC4 \
     /forum/repos/Forum/docker/bootstrap-global.sh forum.tld

## Step 3 – Configuration

After bootstrapping, the `insert_folder/forum/config` and `insert_folder/forum/start` folders on the host should contain
the initial configuration and startup data. 

A low of parameters can be adjusted, some of them mandatory before being able to successfully use the software.

### Step 3.1 – Server Certificate

The forum is designed to use HTTPS and thus requires a certificate as well as a private key.

These need to be saved under `insert_folder/forum/config/https` as `forum.crt` and `forum.key`. 

### Step 3.2 – SMTP

The authentication method envolving custom user accounts needs to be able to send emails. 
As such the `EMAIL_` variables under `insert_folder/forum/start/Forum.Auth.sh` need to be adjusted.  

### Step 3.3 – Google reCAPTCHA

The forum relies on [Google reCAPTCHA](https://developers.google.com/recaptcha/) to prevent robots 
from automatically registering.

In order to use this feature, an account is needed and the keys need to be configured:
* The public key `reCAPTCHASiteKey` under `insert_folder/forum/config/Forum.Webclient/config.js`
* The private key `RECAPTCHA_SECRET_KEY` under `insert_folder/forum/start/Forum.Auth.sh`

### Step 3.4 – User Info

Various pieces of information that will be displayed to users, such as _terms of service_ and a _privacy policy_ need 
be adjusted under `insert_folder/forum/Forum.WebClient/doc`. 

## Step 4 – Running

Once the configuration is complete, the forum can be run from the container:

    docker run \
    -p 80:80 -p 443:443 \
    -v insert_folder/forum/config:/forum/config \
    -v insert_folder/forum/data:/forum/data \
    -v insert_folder/forum/logs:/forum/logs \
    -v insert_folder/forum/start:/forum/start \
    -v insert_folder/forum/temp:/forum/temp \
    -v insert_folder/forum/www:/var/www \
    example/fastforum:v1RC4

The first user to be created will receive the maximum privileges.