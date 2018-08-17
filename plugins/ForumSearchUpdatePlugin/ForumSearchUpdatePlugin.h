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

#include "Plugin.h"
#include "Observers.h"
#include "SeparateThreadConsumer.h"
#include "JsonWriter.h"

#include <boost/thread/tss.hpp>

#include <atomic>
#include <ctime>
#include <cstdio>
#include <thread>

namespace Forum::Extensibility
{
    class ForumSearchUpdatePlugin final : public IPlugin,
            Helpers::SeparateThreadConsumer<ForumSearchUpdatePlugin, Helpers::SeparateThreadConsumerBlob>
    {
    public:
        friend class Helpers::SeparateThreadConsumer<ForumSearchUpdatePlugin, Helpers::SeparateThreadConsumerBlob>;
        friend struct CloseFileReminderCallback;

        explicit ForumSearchUpdatePlugin(PluginInput& input);

        StringView name() const noexcept override;
        StringView version() const noexcept override;

        void stop() override;

    private:
        void registerEvents();
        void onAddNewDiscussionThread(const Entities::DiscussionThread& thread);
        void onChangeDiscussionThread(const Entities::DiscussionThread& thread, 
                                      Entities::DiscussionThread::ChangeType changeType);
        void onDeleteDiscussionThread(const Entities::DiscussionThread& thread);

        void onAddNewDiscussionThreadMessage(const Entities::DiscussionThreadMessage& message);
        void onChangeDiscussionThreadMessage(const Entities::DiscussionThreadMessage& message, 
                                             Entities::DiscussionThreadMessage::ChangeType changeType);
        void onDeleteDiscussionThreadMessage(const Entities::DiscussionThreadMessage& message);


        void onFail(uint32_t failNr);
        void onThreadFinish();

        void consumeValues(Helpers::SeparateThreadConsumerBlob* values, size_t nrOfValues);
        void prepareFile();
        void closeFile();
        void onThreadWaitNoValues();

        template<typename Fn>
        void enqueueJson(Fn&& action);

        Repository::WriteEvents& writeEvents_;
        boost::signals2::connection onAddNewDiscussionThreadConnection_;
        boost::signals2::connection onChangeDiscussionThreadConnection_;
        boost::signals2::connection onDeleteDiscussionThreadConnection_;
        boost::signals2::connection onMergeDiscussionThreadsConnection_;
        boost::signals2::connection onAddNewDiscussionThreadMessageConnection_;
        boost::signals2::connection onChangeDiscussionThreadMessageConnection_;
        boost::signals2::connection onDeleteDiscussionThreadMessageConnection_;

        //only used by the consumer thread
        std::string destinationFileTemplate_;
        std::string currentFileName_;
        size_t elementsWritten_{};
        time_t refreshEverySeconds_{};
        time_t lastFileCreatedAt_{};
    };

    template <typename Fn>
    void ForumSearchUpdatePlugin::enqueueJson(Fn&& action)
    {
        static boost::thread_specific_ptr<Json::StringBuffer> outputBufferPtr;

        if ( ! outputBufferPtr.get())
        {
            outputBufferPtr.reset(new Json::StringBuffer{ 1 << 20 }); //1 MiByte buffer / thread initial and for each increment
        }
        auto& outputBuffer = *outputBufferPtr;

        outputBuffer.clear();

        Json::JsonWriter writer{ outputBuffer };
        action(writer);

        enqueue(Helpers::SeparateThreadConsumerBlob::allocateCopy(outputBuffer.view()));
    }

    extern "C" BOOST_SYMBOL_EXPORT void __cdecl loadPlugin(PluginInput* input, PluginPtr* output);
}
