#include "PersistenceBlob.h"

using namespace Forum::Persistence;

Blob::Blob() : buffer(nullptr), size(0)
{
}

Blob::Blob(size_t size) : buffer(new char[size]), size(size)
{
}

void Blob::free(Blob& blob)
{
    if (blob.buffer)
    {
        delete[] blob.buffer;
    }
}
