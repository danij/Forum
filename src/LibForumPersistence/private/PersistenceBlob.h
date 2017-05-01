#pragma once

#include <cstddef>
#include <cstdint>

namespace Forum
{
    namespace Persistence
    {
        struct Blob
        {
            Blob();
            explicit Blob(size_t size);
            ~Blob() = default;
            Blob(const Blob&) = default;
            Blob(Blob&&) = default;
            Blob& operator=(const Blob&) = default;
            Blob& operator=(Blob&&) = default;
            
            char* buffer; //storing raw pointer so that Blob can be placed in a boost lockfree queue
            size_t size;

            static void free(Blob& blob);
        };
    }
}
