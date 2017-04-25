#include "PersistenceBlob.h"

using namespace Forum::Persistence;

void Forum::Persistence::freeBlob(char* buffer)
{
    if (buffer)
    {
        delete[] buffer;
    }
}

Blob Blob::create(size_t size)
{
    return
    {
        std::shared_ptr<char>(new char[size], freeBlob),
        size
    };
}
