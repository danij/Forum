/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Helpers;
using namespace Forum::Repository;

BOOST_AUTO_TEST_CASE( Version_is_successfully_returned )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::SHOW_VERSION);
    BOOST_REQUIRE_EQUAL(Forum::VERSION, versionObj.get<std::string>("version"));
}

BOOST_AUTO_TEST_CASE( Executing_a_command_beyond_the_range_of_available_commands_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          std::get<1>(handlerToObjAndStatus(handler, static_cast<Forum::Commands::Command>(-1))));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          std::get<1>(handlerToObjAndStatus(handler, static_cast<Forum::Commands::Command>(0xFFFFFF))));
}
