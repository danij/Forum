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

#pragma once

#include <optional>

#include <boost/noncopyable.hpp>

namespace Forum::Helpers
{
    template<typename T>
    struct TemporaryChanger final : private boost::noncopyable
    {
        TemporaryChanger(T& toChange, T newValue) noexcept : toChange_(toChange)
        {
            oldValue_ = toChange;
            toChange = newValue;
        }
        ~TemporaryChanger() noexcept
        {
            toChange_ = oldValue_;
        }
    private:
        T& toChange_;
        T oldValue_;
    };

    typedef TemporaryChanger<bool> BoolTemporaryChanger;

    /**
     * Changes a std::optional and reverts it to std::nullopt only if it didn't have a value to start with
     */
    template<typename T>
    struct OptionalRevertToNoneChanger final : private boost::noncopyable
    {
        OptionalRevertToNoneChanger(std::optional<T>& optional, T value) : optional_(optional)
        {
            if ( ! optional)
            {
                revertToNone_ = true;
                optional = value;
            }
            else
            {
                revertToNone_ = false;
            }
        }

        ~OptionalRevertToNoneChanger()
        {
            if (revertToNone_)
            {
                optional_ = std::nullopt;
            }
        }

    private:
        std::optional<T>& optional_;
        bool revertToNone_;
    };
}
