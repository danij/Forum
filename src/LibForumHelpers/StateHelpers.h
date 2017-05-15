#pragma once

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

namespace Forum
{
    namespace Helpers
    {
        struct BoolTemporaryChanger final : private boost::noncopyable
        {
            BoolTemporaryChanger(bool& toChange, bool newValue) noexcept : toChange_(toChange)
            {
                oldValue_ = toChange;
                toChange = newValue;
            }
            ~BoolTemporaryChanger() noexcept
            {
                toChange_ = oldValue_;
            }
        private:
            bool& toChange_;
            bool oldValue_;
        };

        /**
         * Changes a boost::optional and reverts it to boost::none only if it didn't have a value to start with
         */
        template<typename T>
        struct OptionalRevertToNoneChanger final : private boost::noncopyable
        {
            OptionalRevertToNoneChanger(boost::optional<T>& optional, T value) : optional_(optional)
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
                    optional_ = boost::none;
                }
            }

        private:
            boost::optional<T>& optional_;
            bool revertToNone_;
        };
    }
}
