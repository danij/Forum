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
    setHandler(COUNT_USERS, countUsers);
    setHandler(ADD_USER, addNewUser);
    setHandler(GET_USERS, getUsers);
    setHandler(GET_USER_BY_NAME, getUserByName);
    setHandler(CHANGE_USER_NAME, changeUserName);
    setHandler(DELETE_USER, deleteUser);
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

void CommandHandler::countUsers(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUserCount(output);
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

void CommandHandler::getUsers(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsers(output);
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
