#pragma once

#include "HttpConstants.h"
#include <cstddef>

namespace Http
{
    /**
     * Constructs a minimal HTTP response for the specified status code
     * @param code Status code which will be written both as a number and as a description
     * @param majorVersion Major version sent in the reply
     * @param minorVersion Minor version sent in the reply
     * @param buffer A large enough buffer (no bounds are checked)
     * @returns The number of bytes written
     */
    size_t buildSimpleResponseFromStatusCode(HttpStatusCode code, int_fast8_t majorVersion, int_fast8_t minorVersion,
                                             char* buffer);
}