/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

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

#include <utility>

namespace Forum::Helpers
{
    template<typename... Args>
    class CallbackWrapper final
    {
    public:
        typedef void* StateType;
        typedef void (*FnType)(StateType, Args...);

        CallbackWrapper() : state_{ nullptr }, callback_{ nullptr }
        { }

        CallbackWrapper(const FnType callback) : state_{ nullptr }, callback_{ callback }
        { }

        ~CallbackWrapper() = default;
        CallbackWrapper(const CallbackWrapper&) = default;
        CallbackWrapper(CallbackWrapper&&) = default;
        CallbackWrapper& operator=(const CallbackWrapper&) = default;
        CallbackWrapper& operator=(CallbackWrapper&&) = default;

        CallbackWrapper& operator=(const FnType callback)
        {
            callback_ = callback;
            return *this;
        }

        void operator()(Args&& ... arguments)
        {
            if (callback_)
            {
                callback_(state_, std::forward<Args>(arguments)...);
            }
        }

        const StateType& state() const
        {
            return state_;
        }

        StateType& state()
        {
            return state_;
        }

    private:
        StateType state_;
        FnType callback_;
    };
}