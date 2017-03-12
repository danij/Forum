#pragma once

namespace Forum
{
    namespace Helpers
    {
        struct BoolTemporaryChanger
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
    }
}
