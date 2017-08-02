# Fast Forum Backend – TODO

## Persistence

- [X] Return flag if import was successful or not
- [X] Stop application if import unsuccessful
- [X] Implement IDirectWriteRepository for populating the memory repository directly without validations
- [X] Use IDirectWriteRepository for importing events read from persistent storage

## Logging

- [ ] Add logging to more scenarios
- [ ] Add samples for configuring logging

## Configuration

- [X] Read configuration entries from file on application startup
- [ ] Add endpoint for changing configuration entries via HTTP and persisting them to disk

## Authorization

- [X] Implement simple authorization scheme for all actions
- [ ] Implement commands for adjusting authorization rights
- [ ] Implement views for returning authorization information
- [ ] Implement flood protection/throttling for actions

## HTTP

- [X] Reject zipped or chunked request bodies
- [ ] Implement HTTP security features (e.g. [OWASP](https://www.owasp.org/index.php/Main_Page) recommendations)

## Authentication 

- [ ] Implement authentication as a different service with OpenID Connect
- [ ] Add context entry for current authentication
- [X] Link users to authentication 

## Attachments

- [ ] Implement attachments as a different service
- [ ] Implement storage of attachments to cloud blob storage
- [ ] Implement option to require validation of attachments before availability for download

## Tests

- [ ] Add unit tests for the HTTP pipeline
- [ ] Add integration tests for persistence
- [ ] Setup continuous integration/testing

## Deployment

- [ ] Create docker container
