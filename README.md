# Fast Forum Backend

Cross-platform backend software for providing a discussion forum. 

## Feature Overview

The backend supports the following:

* Users that create content
* Discussion threads composed of messages
* Discussion tags for classifying discussion threads by the subject their subject
* Hierarchical discussion categories for grouping together threads that share the same tags
* Access control (_not yet implemented_) 

Text content is stored as UTF-8 and is compared using Unicode awareness whenever necessary 
(e.g. when checking if a user name is already taken).

## Architecture

### Service

The backend will provide a RESTful API over HTTP/1.1 for retrieving and updating entities, represented using JSON. 
Supporting HATEOAS is not among the goals of the application.
  
Serving static files will not be among the responsibilities of the backend service, but of dedicated HTTPDs. 
These can then also be set up to forward API requests to the service via reverse proxy.

### Persistence

What sets this backend apart from similar software is the way it handles persisting the data.
 
Instead of interrogating a relational database for each request, the backend stores all the entities in memory. 
Using efficient data structures then enables constant or logarithmic times for looking up the required information in 
order to construct each reply.

The code is designed to handle multiple requests at the same time, using a multiple readers/single-writer lock.
 
Apart from storing entities in memory, they will also be persisted to a more durable storage. Observers will pick up
events and asynchronously store them into an event store on disk. When the application is started, all events are read
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
