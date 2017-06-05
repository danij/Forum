#include "StringHelpers.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <unicode/ucol.h>

using namespace Forum::Helpers;

struct LocaleCache final : private boost::noncopyable
{
public:
    static LocaleCache& instance()
    {
        std::call_once(initFlag_, []()
        {
            instance_ = std::unique_ptr<LocaleCache>(new LocaleCache);
        });
        return *instance_;
    }

    static void reset()
    {
        instance_.reset();
    }

    const UCollator* collator() const
    {
        return collator_.get();
    }

private:
    LocaleCache()
    {
        UErrorCode errorCode{};
        collator_.reset(ucol_open("en_US", &errorCode));
        
        if (U_FAILURE(errorCode))
        {
            throw std::runtime_error("Unable to load collation!");
        }

        ucol_setStrength(collator_.get(), UCollationStrength::UCOL_PRIMARY);
    }

    static std::once_flag initFlag_;
    static std::unique_ptr<LocaleCache> instance_;

    std::unique_ptr<UCollator, decltype(&ucol_close)> collator_ {nullptr, ucol_close};
};

std::once_flag LocaleCache::initFlag_;
std::unique_ptr<LocaleCache> LocaleCache::instance_;

static constexpr size_t MaxSortKeyGenerationUCharBufferSize = 65536;
static constexpr size_t MaxSortKeyGenerationDestinationBufferSize = 10 * MaxSortKeyGenerationUCharBufferSize;

static thread_local std::unique_ptr<UChar[]> SortKeyGenerationUCharBuffer(new UChar[MaxSortKeyGenerationUCharBufferSize]);
static thread_local std::unique_ptr<uint8_t[]> SortKeyGenerationDestinationBuffer(new uint8_t[MaxSortKeyGenerationDestinationBufferSize]);

static char EmptySortKeyNullTerminator = 0;

StringWithSortKey::StringWithSortKey() noexcept : stringLength_(0), sortKeyLength_(1)
{
}

StringWithSortKey::StringWithSortKey(StringView view) : StringWithSortKey()
{
    if (view.size() < 1)
    {
        return;
    }

    if (view.size() > (MaxSortKeyGenerationUCharBufferSize / 8))
    {
        throw new std::runtime_error("String for which a sort key is to be generated is too big");
    }

    stringLength_ = view.size();

    int32_t u16Written{};
    UErrorCode errorCode{};

    auto ucharBuffer = SortKeyGenerationUCharBuffer.get();
    std::unique_ptr<UChar[]> ucharBufferIfThreadLocalOneIsNotYetInitialized;

    if ( ! ucharBuffer)
    {
        ucharBufferIfThreadLocalOneIsNotYetInitialized.reset(new UChar[MaxSortKeyGenerationUCharBufferSize]);
        ucharBuffer = ucharBufferIfThreadLocalOneIsNotYetInitialized.get();
    }

    auto u16Chars = u_strFromUTF8Lenient(ucharBuffer, MaxSortKeyGenerationUCharBufferSize,
                                         &u16Written, view.data(), view.size(), &errorCode);
    if (U_FAILURE(errorCode))
    {
        //use string as sort key
        sortKeyLength_ = stringLength_ + 1;
        bytes_ = std::unique_ptr<char[]>(new char[stringLength_ + sortKeyLength_]);
        std::copy(view.begin(), view.end(), bytes_.get());
        std::copy(view.begin(), view.end(), bytes_.get() + stringLength_);
        bytes_[stringLength_ + sortKeyLength_ - 1] = 0;

        return;
    }

    auto sortKeyBuffer = SortKeyGenerationDestinationBuffer.get();
    std::unique_ptr<uint8_t[]> sortKeyBufferIfThreadLocalOneIsNotYetInitialized;

    if ( ! sortKeyBuffer)
    {
        sortKeyBufferIfThreadLocalOneIsNotYetInitialized.reset(new uint8_t[MaxSortKeyGenerationDestinationBufferSize]);
        sortKeyBuffer = sortKeyBufferIfThreadLocalOneIsNotYetInitialized.get();
    }

    sortKeyLength_ = ucol_getSortKey(LocaleCache::instance().collator(), u16Chars, u16Written, 
                                     sortKeyBuffer, MaxSortKeyGenerationDestinationBufferSize);

    bytes_.reset(new char[stringLength_ + sortKeyLength_]);
    std::copy(view.begin(), view.end(), bytes_.get());
    std::copy(sortKeyBuffer, sortKeyBuffer + sortKeyLength_, bytes_.get() + stringLength_);
}

StringWithSortKey::StringWithSortKey(const StringWithSortKey& other)
    : stringLength_(other.stringLength_), sortKeyLength_(other.sortKeyLength_)
{
    auto toCopy = stringLength_ + sortKeyLength_;
    bytes_ = std::unique_ptr<char[]>(new char[toCopy]);
    std::copy(other.bytes_.get(), other.bytes_.get() + toCopy, bytes_.get());
}

StringWithSortKey::StringWithSortKey(StringWithSortKey&& other) noexcept
{
    swap(*this, other);
}

StringWithSortKey& StringWithSortKey::operator=(const StringWithSortKey& other)
{
    StringWithSortKey copy(other);
    swap(*this, copy);
    return *this;
}

StringWithSortKey& StringWithSortKey::operator=(StringWithSortKey&& other) noexcept
{
    swap(*this, other);
    return *this;
}

bool StringWithSortKey::operator==(const StringWithSortKey& other) const
{
    return strcmp(sortKey().data(), other.sortKey().data()) == 0;
}

bool StringWithSortKey::operator<(const StringWithSortKey& other) const
{
    return strcmp(sortKey().data(), other.sortKey().data()) < 0;
}

bool StringWithSortKey::operator<=(const StringWithSortKey& other) const
{
    return ! (*this > other);
}

bool StringWithSortKey::operator>(const StringWithSortKey& other) const
{
    return strcmp(sortKey().data(), other.sortKey().data()) > 0;
}

bool StringWithSortKey::operator>=(const StringWithSortKey& other) const
{
    return ! (*this < other);
}

StringView StringWithSortKey::string() const
{
    if (stringLength_ < 1)
    {
        return StringView{};
    }
    return StringView(bytes_.get(), stringLength_);
}

StringView StringWithSortKey::sortKey() const
{
    if (sortKeyLength_ < 2)
    {
        return StringView(&EmptySortKeyNullTerminator, 1);
    }
    return StringView(bytes_.get() + stringLength_, sortKeyLength_);
}

void Forum::Helpers::cleanupStringHelpers()
{
    LocaleCache::reset();
}

void Forum::Helpers::swap(StringWithSortKey& first, StringWithSortKey& second) noexcept
{
    using std::swap;
    swap(first.bytes_, second.bytes_);
    swap(first.stringLength_, second.stringLength_);
    swap(first.sortKeyLength_, second.sortKeyLength_);
}

std::ostream& Forum::Helpers::operator<<(std::ostream& stream, const StringWithSortKey& string)
{
    auto view = string.string();
    stream.write(view.data(), view.size());
    return stream;
}
