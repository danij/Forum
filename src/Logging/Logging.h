/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#pragma once

#define BOOST_LOG_DYN_LINK 1

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/trivial.hpp>

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(forumLogger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)

#define FORUM_LOG_TRACE   BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::trace)
#define FORUM_LOG_DEBUG   BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::debug)
#define FORUM_LOG_INFO    BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::info)
#define FORUM_LOG_WARNING BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::warning)
#define FORUM_LOG_ERROR   BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::error)
#define FORUM_LOG_FATAL   BOOST_LOG_SEV(forumLogger::get(), boost::log::trivial::severity_level::fatal)
