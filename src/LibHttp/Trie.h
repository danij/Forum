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

#include <array>
#include <cassert>
#include <cctype>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace Http
{
    template<typename Key>
    struct GetSingleKey
    {
        static constexpr size_t NrOfResults = 1;

        std::array<Key, NrOfResults> operator()(Key value) const
        {
            return {{ value }};
        }
    };
    
    struct GetAsciiUpperLowerKey
    {
        static constexpr size_t NrOfResults = 2;

        std::array<char, NrOfResults> operator()(const char value) const
        {
            return 
            {{
                static_cast<char>(std::tolower(static_cast<unsigned char>(value))),
                static_cast<char>(std::toupper(static_cast<unsigned char>(value)))
            }};
        }
    };

    template<typename Key, typename T, 
             typename GetKeyAlternatives = GetSingleKey<Key>, size_t NrOfEqKeys = GetKeyAlternatives::NrOfResults>
    class ImmutableTrie final
    {
    public:
        ImmutableTrie() : nodes_(std::make_unique<Node[]>(1))
        {}

        template<typename KeyCollection>
        ImmutableTrie(std::initializer_list<std::pair<KeyCollection, T>> values)
            : ImmutableTrie(values.begin(), values.end())
        {}

        template<typename It>
        ImmutableTrie(It begin, It end)
        {
            using KeyCollection = typename std::iterator_traits<It>::value_type::first_type;
            using KeyType = typename KeyCollection::value_type;

            static_assert(std::is_same<Key, KeyType>::value);

            initialize(begin, end);
        }

        ~ImmutableTrie() = default;

        ImmutableTrie(const ImmutableTrie&) = delete;
        ImmutableTrie(ImmutableTrie&&) = default;
        ImmutableTrie& operator=(const ImmutableTrie&) = delete;
        ImmutableTrie& operator=(ImmutableTrie&&) = default;

        template<typename KeyCollection>
        const T* find(const KeyCollection& keyCollection) const
        {
            return find(std::cbegin(keyCollection), std::cend(keyCollection));
        }

        template<typename KeyIt>
        const T* find(KeyIt begin, KeyIt end) const;

        auto size() const
        {
            return values_.size();
        }

    private:
        std::vector<T> values_;

        struct Node
        {
            std::array<Key, NrOfEqKeys> keys{};
            size_t nrOfChildNodes{};
            Node** children{};
            const T* value{};
        };
        std::unique_ptr<Node[]> nodes_;
        std::unique_ptr<Node*[]> childNodeArrays_;
        size_t nrOfNodes_{};
        size_t nodesCurrentlyAdded_{};
        size_t childNodesCurrentlyAdded_{};

        template<typename It>
        void initialize(It begin, It end);

        template<typename It>
        size_t countRequiredNumberedOfNodes(It begin, It end);

        template<typename KeyIt>
        size_t countRequiredNumberedOfNodes(const std::vector<std::tuple<KeyIt, KeyIt>>& keyPairs);

        template<typename KeyIt>
        void addKeyLevel(Node* parentNode, const std::vector<std::tuple<KeyIt, KeyIt, const T&>>& keyValuePairs);

        const T* addValueAndGetPtr(const T& value);
        Node** getChildNodeArray(size_t nrOfChildNodes);
    };

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    template <typename It>
    void ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::initialize(It begin, It end)
    {
        values_.reserve(std::distance(begin, end));

        nrOfNodes_ = countRequiredNumberedOfNodes(begin, end);
        nodes_ = std::make_unique<Node[]>(nrOfNodes_);
        childNodeArrays_ = std::make_unique<Node*[]>(nrOfNodes_);

        using KeyCollection = typename std::iterator_traits<It>::value_type::first_type;
        using KeyIt = typename KeyCollection::const_iterator;

        std::vector<std::tuple<KeyIt, KeyIt, const T&>> keyValuePairs;

        for (auto it = begin; it != end; ++it)
        {
            const auto& [key, value] = *it;
            keyValuePairs.emplace_back(std::cbegin(key), std::cend(key), value);
        }
        addKeyLevel(nodes_.get(), keyValuePairs);
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    template <typename It>
    size_t ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::countRequiredNumberedOfNodes(It begin, It end)
    {
        using KeyCollection = typename std::iterator_traits<It>::value_type::first_type;
        using KeyIt = typename KeyCollection::const_iterator;

        std::vector<std::tuple<KeyIt, KeyIt>> keyPairs;

        for (auto it = begin; it != end; ++it)
        {
            const auto&[key, value] = *it;
            keyPairs.emplace_back(std::cbegin(key), std::cend(key));
        }
        return countRequiredNumberedOfNodes(keyPairs);
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    template <typename KeyIt>
    size_t ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::countRequiredNumberedOfNodes(
        const std::vector<std::tuple<KeyIt, KeyIt>>& keyPairs)
    {
        size_t result = 1u;
        GetKeyAlternatives keyGetter{};

        std::map<std::array<Key, NrOfEqKeys>, std::vector<std::tuple<KeyIt, KeyIt>>> sameKeys;

        for (const auto&[keyBegin, keyEnd] : keyPairs)
        {
            if (keyBegin != keyEnd)
            {
                sameKeys[keyGetter(*keyBegin)].emplace_back(std::next(keyBegin), keyEnd);
            }
        }

        for (const auto&[keys, subKeyValuePairs] : sameKeys)
        {
            result += countRequiredNumberedOfNodes(subKeyValuePairs);
        }

        return result;
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    template <typename KeyIt>
    void ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::addKeyLevel(Node* parentNode,
            const std::vector<std::tuple<KeyIt, KeyIt, const T&>>& keyValuePairs)
    {
        GetKeyAlternatives keyGetter{};

        std::map<std::array<Key, NrOfEqKeys>, std::vector<std::tuple<KeyIt, KeyIt, const T&>>> sameKeys;

        for (const auto& [keyBegin, keyEnd, value] : keyValuePairs)
        {
            if (keyBegin == keyEnd)
            {
                parentNode->value = addValueAndGetPtr(value);
            }
            else
            {
                sameKeys[keyGetter(*keyBegin)].emplace_back(std::next(keyBegin), keyEnd, value);
            }
        }

        if ( ! sameKeys.empty())
        {
            parentNode->children = getChildNodeArray(sameKeys.size());

            for (const auto& [keys, subKeyValuePairs] : sameKeys)
            {
                nodesCurrentlyAdded_ += 1;
                assert(nodesCurrentlyAdded_ < nrOfNodes_);
                Node* childNode = nodes_.get() + nodesCurrentlyAdded_;
                childNode->keys = keys;
                parentNode->children[parentNode->nrOfChildNodes++] = childNode;

                addKeyLevel(childNode, subKeyValuePairs);
            }
        }
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    const T* ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::addValueAndGetPtr(const T& value)
    {
        values_.emplace_back(value);
        return values_.data() + (values_.size() - 1);
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    typename ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::Node** 
            ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::getChildNodeArray(const size_t nrOfChildNodes)
    {
        auto result = childNodeArrays_.get() + childNodesCurrentlyAdded_;
        childNodesCurrentlyAdded_ += nrOfChildNodes;
        assert(childNodesCurrentlyAdded_ < nrOfNodes_);
        return result;
    }

    template <typename Key, typename T, typename GetKeyAlternatives, size_t NrOfEqKeys>
    template <typename KeyIt>
    const T* ImmutableTrie<Key, T, GetKeyAlternatives, NrOfEqKeys>::find(KeyIt begin, KeyIt end) const
    {
        const Node* currentNode = nodes_.get();
        auto it = begin;

        for (; it != end; ++it)
        {
            const Node* matchedChildNode{};

            for (size_t i = 0; i < currentNode->nrOfChildNodes; ++i)
            {
                const Node* childNode = currentNode->children[i];
                if (std::find(childNode->keys.cbegin(), childNode->keys.cend(), *it) != childNode->keys.cend())
                {
                    matchedChildNode = childNode;
                    break;
                }
            }
            if ( ! matchedChildNode) break;
            currentNode = matchedChildNode;
        }

        bool matchedAllKeyParts = it == end;
        return matchedAllKeyParts ? currentNode->value : nullptr;
    }

    template<typename T>
    using ImmutableAsciiCaseInsensitiveTrie = ImmutableTrie<char, T, GetAsciiUpperLowerKey>;
}
