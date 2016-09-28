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


CommandHandler::CommandHandler(ReadRepositoryConstRef readRepository, WriteRepositoryRef writeRepository,
                               MetricsRepositoryRef metricsRepository)
        : readRepository_(readRepository), writeRepository_(writeRepository), metricsRepository_(metricsRepository)
{
    setHandler(SHOW_VERSION, version);
    setHandler(COUNT_USERS, countUsers);
    setHandler(ADD_USER, addNewUser);
    setHandler(GET_USERS, getUsers);
    setHandler(GET_USER_BY_NAME, getUserByName);
    setHandler(CHANGE_USER_NAME, changeUserName);
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

void CommandHandler::addNewUser(const std::vector<std::string>& parameters, std::ostream& output)
{
    StatusCode code;
    if (parameters.empty())
    {
        code = StatusCode::INVALID_PARAMETERS;
    }
    else
    {
        code = writeRepository_->addNewUser(parameters[0], output);
    }
    if (statusCodeNotWritten(code))
    {
        writeStatusCode(output, code);
    }
}

void CommandHandler::getUsers(const std::vector<std::string>& parameters, std::ostream& output)
{
    readRepository_->getUsers(output);
}

void CommandHandler::getUserByName(const std::vector<std::string>& parameters, std::ostream& output)
{
    if (parameters.empty())
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
    }
    else
    {
        readRepository_->getUserByName(parameters[0], output);
    }
}

void CommandHandler::changeUserName(const std::vector<std::string>& parameters, std::ostream& output)
{
    StatusCode code;
    if (parameters.size() != 2)
    {
        code = StatusCode::INVALID_PARAMETERS;
    }
    else
    {
        code = writeRepository_->changeUserName(parameters[0], parameters[1], output);
    }
    if (statusCodeNotWritten(code))
    {
        writeStatusCode(output, code);
    }
}
