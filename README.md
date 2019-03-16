# Fast Forum Backend 

Cross-platform backend software for providing a discussion forum. 

Demo: http://fastforumdemo.westeurope.cloudapp.azure.com/

## Feature Overview

The backend supports the following:

* Users that create content
* Discussion threads composed of messages
* Discussion tags for classifying discussion threads
* Hierarchical discussion categories for grouping together threads that share the same tags
* Comments for discussion messages
* Private messages between users
* Fine grained, hierarchical access control which allows granting or revoking specific privileges to any individual user  
* Event based persistence of data, enabling incremental backups and going back to any point in time
* External authentication providers for stronger security and a better user experience.

Text content is normalized and stored using `UTF-8` encoding. When necessary it is compared using `en_US` collation 
with primary strength (e.g. when checking if a user name is already taken).

## Architecture

[The following document presents the architecture decisions in detail](doc/Architecture&Details.md)

### Service

The backend provides a REST API over HTTP/1.1 for retrieving and updating entities, represented using JSON. 
Supporting HATEOAS is not among the goals of the application.
  
Serving static files will not be among the responsibilities of the backend service, but of dedicated HTTPDs. 
These can then also be set up to forward API requests to the service via reverse proxy.

[API Documentation](doc/API.md)

### Persistence

What sets this backend apart from similar software is the way it handles persisting the data.
 
Instead of interrogating a relational database for each request, the backend stores all the entities in memory. 
Using efficient data structures then enables constant or logarithmic times for looking up the required information in 
order to construct each reply.

The code is designed to handle multiple requests at the same time, using a multiple readers/single-writer lock.
 
Apart from storing entities in memory, they will also be persisted to a more durable storage. Observers will pick up
events and asynchronously store them into an event store on disk. In case of a crash, the events that have not reached 
the disk will be lost. 

When the application is started, all events are read
from disk and replied to fill the repositories.

## Development Info

### Coding Style

* 4 spaces for indentation
* Java style but with brackets on new line
* Max 120 characters/line
* Space inside an initializer list (e.g. `{ 1, 2, 3 }` instead of `{1, 2, 3}`)

### Library Dependencies

* [Boost C++ libraries](http://www.boost.org/)
* [International Components for Unicode](http://site.icu-project.org/)

### Building

    mkdir build
    cd build
    cmake ../
    make

### Running Tests

    ctest --verbose

### Installing

A more detailed installation guide is available at [Installation Guide](doc/Install.md).

### Container

The forum can also be run from [Docker](https://www.docker.com/) containers: [Running Fast Forum via Docker](docker/README.md)