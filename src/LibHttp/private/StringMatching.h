#pragma once

#include <cstddef>

namespace Http
{
    /**
    * Matches a string against another one, optionally ignoring case
    * 
    * @pre source must point to (AgainstSize - 1) / 2 characters
    * @param source String to be searched
    * @param against A string where each character appears as both upper and lower case (e.g. HhEeLlLlO  WwOoRrLlDd)
    */
    template<size_t AgainstSize>
    bool matchStringUpperOrLower(const char* source, const char (&against)[AgainstSize])
    {
        char result = 0;
        auto size = (AgainstSize - 1) / 2; //-1 because AgainstSize also contains terminating null character
        for (size_t iSource = 0, iAgainst = 0; iSource < size; ++iSource, iAgainst += 2)
        {
            result |= (source[iSource] ^ against[iAgainst]) & (source[iSource] ^ against[iAgainst + 1]);
        }
        return result == 0;
    }
}