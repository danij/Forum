#include "CommandHandler.h"
#include "JsonWriter.h"
#include "OutputHelpers.h"
#include "Version.h"

using namespace Forum::Commands;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

#define setHandler(command, function) \
    handlers_[command] = std::bind(&CommandHandler::function, this, std::placeholders::_1, std::placeholders::_2);


CommandHandler::CommandHandler(ReadRepositoryRef readRepository, WriteRepositoryRef writeRepository,
                               MetricsRepositoryRef metricsRepository)
        : readRepository_(readRepository), writeRepository_(writeRepository), metricsRepository_(metricsRepository)
{
    setHandler(SHOW_VERSION, version);
    setHandler(COUNT_ENTITIES, countEntities);

    setHandler(ADD_USER, addNewUser);
    setHandler(GET_USERS_BY_NAME, getUsersByName);
    setHandler(GET_USERS_BY_CREATED_ASCENDING, getUsersByCreatedAscending);
    setHandler(GET_USERS_BY_CREATED_DESCENDING, getUsersByCreatedDescending);
    setHandler(GET_USERS_BY_LAST_SEEN, getUsersByLastSeen);
    setHandler(GET_USER_BY_ID, getUserById);
    setHandler(GET_USER_BY_NAME, getUserByName);
    setHandler(CHANGE_USER_NAME, changeUserName);
    setHandler(DELETE_USER, deleteUser);

    setHandler(ADD_DISCUSSION_THREAD, addNewDiscussionThread);
    setHandler(GET_DISCUSSION_THREADS_BY_NAME, getDiscussionThreadsByName);
    setHandler(GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING, getDiscussionThreadsByCreatedAscending);
    setHandler(GET_DISCUSSION_THREADS_BY_CREATED_DESCENDING, getDiscussionThreadsByCreatedDescending);
    setHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED_ASCENDING, getDiscussionThreadsByLastUpdatedAscending);
    setHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED_DESCENDING, getDiscussionThreadsByLastUpdatedDescending);
    setHandler(GET_DISCUSSION_THREAD_BY_ID, getDiscussionThreadById);
    setHandler(CHANGE_DISCUSSION_THREAD_NAME, changeDiscussionThreadName);
    setHandler(DELETE_DISCUSSION_THREAD, deleteDiscussionThread);

    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_NAME, getDiscussionThreadsOfUserByName);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_ASCENDING, getDiscussionThreadsOfUserByCreatedAscending);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_DESCENDING, getDiscussionThreadsOfUserByCreatedDescending);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_ASCENDING,
               getDiscussionThreadsOfUserByLastUpdatedAscending);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_DESCENDING,
               getDiscussionThreadsOfUserByLastUpdatedDescending);

    setHandler(ADD_DISCUSSION_THREAD_MESSAGE, addNewDiscussionThreadMessage);
    setHandler(DELETE_DISCUSSION_THREAD_MESSAGE, deleteDiscussionThreadMessage);
}

ReadRepositoryRef CommandHandler::getReadRepository()
{
    return readRepository_;
}

WriteRepositoryRef CommandHandler::getWriteRepository()
{
    return writeRepository_;
}

void CommandHandler::handle(Command command, const std::vector<std::string>& parameters, std::ostream& output)
{
    if (command >= 0 && command < LAST_COMMAND)
    {
        handlers_[command](parameters, output);
    }
}

void CommandHandler::version(const std::vector<std::string>& parameters, std::ostream& output)
{
    metricsRepository_->getVersion(output);
}

void CommandHandler::countEntities(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getEntitiesCount(output);
}

bool CommandHandler::checkNumberOfParameters(const std::vector<std::string>& parameters, std::ostream& output,
                                             size_t number)
{
    if (parameters.size() != number)
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
        return false;
    }
    return true;
}

void CommandHandler::addNewUser(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    writeRepository_->addNewUser(parameters[0], output);
}

void CommandHandler::getUsersByName(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsersByName(output);
}

void CommandHandler::getUsersByCreatedAscending(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsersByCreatedAscending(output);
}

void CommandHandler::getUsersByCreatedDescending(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsersByCreatedDescending(output);
}

void CommandHandler::getUsersByLastSeen(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsersByLastSeen(output);
}

void CommandHandler::getUserById(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getUserById(parameters[0], output);
}

void CommandHandler::getUserByName(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getUserByName(parameters[0], output);
}

void CommandHandler::changeUserName(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 2)) return;
    writeRepository_->changeUserName(parameters[0], parameters[1], output);
}

void CommandHandler::deleteUser(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    writeRepository_->deleteUser(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsByName(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getDiscussionThreadsByName(output);
}

void CommandHandler::getDiscussionThreadsByCreatedAscending(const std::vector<std::string>& parameters,
                                                            std::ostream& output)
{
    readRepository_->getDiscussionThreadsByCreatedAscending(output);
}

void CommandHandler::getDiscussionThreadsByCreatedDescending(const std::vector<std::string>& parameters,
                                                             std::ostream& output)
{
    readRepository_->getDiscussionThreadsByCreatedDescending(output);
}

void CommandHandler::getDiscussionThreadsByLastUpdatedAscending(const std::vector<std::string>& parameters,
                                                                std::ostream& output)
{
    readRepository_->getDiscussionThreadsByLastUpdatedAscending(output);
}

void CommandHandler::getDiscussionThreadsByLastUpdatedDescending(const std::vector<std::string>& parameters,
                                                                 std::ostream& output)
{
    readRepository_->getDiscussionThreadsByLastUpdatedDescending(output);
}

void CommandHandler::addNewDiscussionThread(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    writeRepository_->addNewDiscussionThread(parameters[0], output);
}

void CommandHandler::getDiscussionThreadById(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadById(parameters[0], output);
}

void CommandHandler::changeDiscussionThreadName(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 2)) return;
    writeRepository_->changeDiscussionThreadName(parameters[0], parameters[1], output);
}

void CommandHandler::deleteDiscussionThread(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    writeRepository_->deleteDiscussionThread(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByName(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByName(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByCreatedAscending(const std::vector<std::string>& parameters,
                                                                  std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByCreatedAscending(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByCreatedDescending(const std::vector<std::string>& parameters,
                                                                   std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByCreatedDescending(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByLastUpdatedAscending(const std::vector<std::string>& parameters,
                                                                      std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByLastUpdatedAscending(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByLastUpdatedDescending(const std::vector<std::string>& parameters,
                                                                       std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByLastUpdatedDescending(parameters[0], output);
}

void CommandHandler::addNewDiscussionThreadMessage(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 2)) return;
    writeRepository_->addNewDiscussionMessageInThread(parameters[0], parameters[1], output);
}

void CommandHandler::deleteDiscussionThreadMessage(const std::vector<std::string>& parameters, std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    writeRepository_->deleteDiscussionMessage(parameters[0], output);
}
