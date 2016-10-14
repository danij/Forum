#include "MemoryRepository.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Repository;

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>())
{
}

void MemoryRepository::addObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::addObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::removeObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}

void MemoryRepository::removeObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}
