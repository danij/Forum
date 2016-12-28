#include "CommandHandler.h"

#include "OutputHelpers.h"

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
    setHandler(GET_USERS_BY_CREATED, getUsersByCreated);
    setHandler(GET_USERS_BY_LAST_SEEN, getUsersByLastSeen);
    setHandler(GET_USER_BY_ID, getUserById);
    setHandler(GET_USER_BY_NAME, getUserByName);
    setHandler(CHANGE_USER_NAME, changeUserName);
    setHandler(DELETE_USER, deleteUser);

    setHandler(ADD_DISCUSSION_THREAD, addNewDiscussionThread);
    setHandler(GET_DISCUSSION_THREADS_BY_NAME, getDiscussionThreadsByName);
    setHandler(GET_DISCUSSION_THREADS_BY_CREATED, getDiscussionThreadsByCreated);
    setHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED, getDiscussionThreadsByLastUpdated);
    setHandler(GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT, getDiscussionThreadsByMessageCount);
    setHandler(GET_DISCUSSION_THREAD_BY_ID, getDiscussionThreadById);
    setHandler(CHANGE_DISCUSSION_THREAD_NAME, changeDiscussionThreadName);
    setHandler(DELETE_DISCUSSION_THREAD, deleteDiscussionThread);

    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_NAME, getDiscussionThreadsOfUserByName);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, getDiscussionThreadsOfUserByCreated);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, getDiscussionThreadsOfUserByLastUpdated);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT, getDiscussionThreadsOfUserByMessageCount);

    setHandler(ADD_DISCUSSION_THREAD_MESSAGE, addNewDiscussionThreadMessage);
    setHandler(DELETE_DISCUSSION_THREAD_MESSAGE, deleteDiscussionThreadMessage);

    setHandler(GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, getDiscussionThreadMessagesOfUserByCreated);
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

void CommandHandler::getUsersByCreated(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsersByCreated(output);
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

void CommandHandler::getDiscussionThreadsByCreated(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getDiscussionThreadsByCreated(output);
}

void CommandHandler::getDiscussionThreadsByLastUpdated(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getDiscussionThreadsByLastUpdated(output);
}

void CommandHandler::getDiscussionThreadsByMessageCount(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getDiscussionThreadsByMessageCount(output);
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

void CommandHandler::getDiscussionThreadsOfUserByCreated(const std::vector<std::string>& parameters, 
                                                         std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByCreated(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByLastUpdated(const std::vector<std::string>& parameters,
                                                                      std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByLastUpdated(parameters[0], output);
}

void CommandHandler::getDiscussionThreadsOfUserByMessageCount(const std::vector<std::string>& parameters,
                                                              std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadsOfUserByMessageCount(parameters[0], output);
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

void CommandHandler::getDiscussionThreadMessagesOfUserByCreated(const std::vector<std::string>& parameters,
                                                                std::ostream& output)
{
    if ( ! checkNumberOfParameters(parameters, output, 1)) return;
    readRepository_->getDiscussionThreadMessagesOfUserByCreated(parameters[0], output);
}
