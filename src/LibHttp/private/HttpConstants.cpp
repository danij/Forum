/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "HttpConstants.h"
#include "HttpStringHelpers.h"

#include <cstddef>
#include <cstdint>

using namespace Http;

static HttpStringView statusCodes[] =
{
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, { "Continue", 8 }, { "Switching Protocols", 19 }, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, { "OK", 2 }, { "Created", 7 }, { "Accepted", 8 },
    { "Non-Authoritative Information", 29 }, { "No Content", 10 }, { "Reset Content", 13 }, { "Partial Content", 15 },
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, { "Multiple Choices", 16 }, { "Moved Permanently", 17 }, { "Found", 5 }, { "See Other", 9 },
    { "Not Modified", 12 }, { "Use Proxy", 9 }, {}, { "Temporary Redirect", 18 }, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    { "Bad Request", 11 }, { "Unauthorized", 12 }, { "Payment Required", 16 }, { "Forbidden", 9 }, { "Not Found", 9 },
    { "Method Not Allowed", 18 }, { "Not Acceptable", 14 }, { "Proxy Authentication Required", 29 },
    { "Request Timeout", 15 }, { "Conflict", 8 }, { "Gone", 4 }, { "Length Required", 15 },
    { "Precondition Failed", 19 }, { "Payload Too Large", 17 }, { "URI Too Long", 12 },
    { "Unsupported Media Type", 22 }, { "Range Not Satisfiable", 21 }, { "Expectation Failed", 18 }, {}, {}, {}, {},
    {}, {}, {}, {}, { "Upgrade Required", 16 }, {}, { "Precondition Required", 21 }, { "Too Many Requests", 17 }, {},
    { "Request Header Fields Too Large", 31 }, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    { "Internal Server Error", 21 }, { "Not Implemented", 15 }, { "Bad Gateway", 11 }, { "Service Unavailable", 19 },
    { "Gateway Timeout", 15 }, { "HTTP Version Not Supported", 26 }, {}, {}, {}, {}, {},
    { "Network Authentication Required", 31 }
};

HttpStringView Http::getStatusCodeString(HttpStatusCode code)
{
    static_assert(std::size(statusCodes) >= (HTTP_STATUS_CODES_COUNT - 1),
                  "statusCodes array is not big enough");

    HttpStringView result;
    if (code > 0 && code < HttpStatusCode::HTTP_STATUS_CODES_COUNT)
    {
        result = statusCodes[code];
    }
    return result.size() ? result : HttpStringView("Unknown", 7);
}

static int hashHeaderName(const char* text, size_t size)
{
    constexpr int maxHeaderNameSize = 22;
    static int coefficients[] = { 1, 2, 42, 307 };
    return
    (
        CharToLower[static_cast<uint8_t>(text[0])]        * coefficients[0] +
        CharToLower[static_cast<uint8_t>(text[size - 1])] * coefficients[1] +
        (size % maxHeaderNameSize)                        * coefficients[2]
    ) % coefficients[3];
}

#define HEADER_COMPARER(hash, string, result) \
    static Request::HttpHeader match_##hash(const char* value, size_t size) \
    { \
        return (matchStringUpperOrLower(value, size, string)) ? result : Request::HttpHeader::UnknownHeader; \
    }

HEADER_COMPARER(5, "warningWARNING", Request::HttpHeader::Warning)
HEADER_COMPARER(8, "accept-languageACCEPT-LANGUAGE", Request::HttpHeader::Accept_Language)
HEADER_COMPARER(11, "expiresEXPIRES", Request::HttpHeader::Expires)
HEADER_COMPARER(12, "accept-encodingACCEPT-ENCODING", Request::HttpHeader::Accept_Encoding)
HEADER_COMPARER(18, "x-http-method-overrideX-HTTP-METHOD-OVERRIDE", Request::HttpHeader::X_Http_Method_Override)
HEADER_COMPARER(22, "refererREFERER", Request::HttpHeader::Referer)
HEADER_COMPARER(24, "trailerTRAILER", Request::HttpHeader::Trailer)
HEADER_COMPARER(29, "if-rangeIF-RANGE", Request::HttpHeader::If_Range)
HEADER_COMPARER(35, "if-matchIF-MATCH", Request::HttpHeader::If_Match)
HEADER_COMPARER(41, "front-end-httpsFRONT-END-HTTPS", Request::HttpHeader::Front_End_Https)
HEADER_COMPARER(50, "locationLOCATION", Request::HttpHeader::Location)
HEADER_COMPARER(52, "content-languageCONTENT-LANGUAGE", Request::HttpHeader::Content_Language)
HEADER_COMPARER(53, "content-md5CONTENT-MD5", Request::HttpHeader::Content_MD5)
HEADER_COMPARER(56, "content-encodingCONTENT-ENCODING", Request::HttpHeader::Content_Encoding)
HEADER_COMPARER(57, "x-forwarded-forX-FORWARDED-FOR", Request::HttpHeader::X_Forwarded_For)
HEADER_COMPARER(70, "content-locationCONTENT-LOCATION", Request::HttpHeader::Content_Location)
HEADER_COMPARER(71, "x-correlation-idX-CORRELATION-ID", Request::HttpHeader::X_Correlation_ID)
HEADER_COMPARER(72, "www-authenticateWWW-AUTHENTICATE", Request::HttpHeader::WWW_Authenticate)
HEADER_COMPARER(79, "x-requested-withX-REQUESTED-WITH", Request::HttpHeader::X_Requested_With)
HEADER_COMPARER(83, "proxy-connectionPROXY-CONNECTION", Request::HttpHeader::Proxy_Connection)
HEADER_COMPARER(95, "teTE", Request::HttpHeader::TE)
HEADER_COMPARER(100, "if-modified-sinceIF-MODIFIED-SINCE", Request::HttpHeader::If_Modified_Since)
HEADER_COMPARER(103, "x-forwarded-hostX-FORWARDED-HOST", Request::HttpHeader::X_Forwarded_Host)
HEADER_COMPARER(115, "transfer-encodingTRANSFER-ENCODING", Request::HttpHeader::Transfer_Encoding)
HEADER_COMPARER(118, "ageAGE", Request::HttpHeader::Age)
HEADER_COMPARER(125, "connectionCONNECTION", Request::HttpHeader::Connection)
HEADER_COMPARER(131, "viaVIA", Request::HttpHeader::Via)
HEADER_COMPARER(135, "x-forwarded-protoX-FORWARDED-PROTO", Request::HttpHeader::X_Forwarded_Proto)
HEADER_COMPARER(149, "proxy-authenticatePROXY-AUTHENTICATE", Request::HttpHeader::Proxy_Authenticate)
HEADER_COMPARER(151, "dntDNT", Request::HttpHeader::DNT)
HEADER_COMPARER(155, "user-agentUSER-AGENT", Request::HttpHeader::User_Agent)
HEADER_COMPARER(163, "dateDATE", Request::HttpHeader::Date)
HEADER_COMPARER(168, "etagETAG", Request::HttpHeader::ETag)
HEADER_COMPARER(181, "fromFROM", Request::HttpHeader::From)
HEADER_COMPARER(184, "if-unmodified-sinceIF-UNMODIFIED-SINCE", Request::HttpHeader::If_Unmodified_Since)
HEADER_COMPARER(190, "retry-afterRETRY-AFTER", Request::HttpHeader::Retry_After)
HEADER_COMPARER(191, "content-typeCONTENT-TYPE", Request::HttpHeader::Content_Type)
HEADER_COMPARER(197, "hostHOST", Request::HttpHeader::Host)
HEADER_COMPARER(209, "proxy-authorizationPROXY-AUTHORIZATION", Request::HttpHeader::Proxy_Authorization)
HEADER_COMPARER(210, "x-request-idX-REQUEST-ID", Request::HttpHeader::X_Request_ID)
HEADER_COMPARER(219, "rangeRANGE", Request::HttpHeader::Range)
HEADER_COMPARER(221, "varyVARY", Request::HttpHeader::Vary)
HEADER_COMPARER(229, "max-forwardsMAX-FORWARDS", Request::HttpHeader::Max_Forwards)
HEADER_COMPARER(230, "x-csrf-tokenX-CSRF-TOKEN", Request::HttpHeader::X_Csrf_Token)
HEADER_COMPARER(233, "content-rangeCONTENT-RANGE", Request::HttpHeader::Content_Range)
HEADER_COMPARER(238, "allowALLOW", Request::HttpHeader::Allow)
HEADER_COMPARER(240, "last-modifiedLAST-MODIFIED", Request::HttpHeader::Last_Modified)
HEADER_COMPARER(245, "if-none-matchIF-NONE-MATCH", Request::HttpHeader::If_None_Match)
HEADER_COMPARER(246, "cookieCOOKIE", Request::HttpHeader::Cookie)
HEADER_COMPARER(247, "cache-controlCACHE-CONTROL", Request::HttpHeader::Cache_Control)
HEADER_COMPARER(249, "authorizationAUTHORIZATION", Request::HttpHeader::Authorization)
HEADER_COMPARER(251, "pragmaPRAGMA", Request::HttpHeader::Pragma)
HEADER_COMPARER(254, "x-wap-profileX-WAP-PROFILE", Request::HttpHeader::X_Wap_Profile)
HEADER_COMPARER(259, "accept-rangesACCEPT-RANGES", Request::HttpHeader::Accept_Ranges)
HEADER_COMPARER(273, "x-uidhX-UIDH", Request::HttpHeader::X_UIDH)
HEADER_COMPARER(274, "acceptACCEPT", Request::HttpHeader::Accept)
HEADER_COMPARER(278, "expectEXPECT", Request::HttpHeader::Expect)
HEADER_COMPARER(281, "content-lengthCONTENT-LENGTH", Request::HttpHeader::Content_Length)
HEADER_COMPARER(288, "serverSERVER", Request::HttpHeader::Server)
HEADER_COMPARER(294, "x-att-deviceidX-ATT-DEVICEID", Request::HttpHeader::X_ATT_DeviceId)
HEADER_COMPARER(303, "accept-charsetACCEPT-CHARSET", Request::HttpHeader::Accept_Charset)
HEADER_COMPARER(306, "upgradeUPGRADE", Request::HttpHeader::Upgrade)

typedef Request::HttpHeader (*MatchFunction)(const char* value, size_t size);

static MatchFunction matchFunctionTable[] =
{
    nullptr, nullptr, nullptr, nullptr, nullptr, match_5, nullptr, nullptr, match_8, nullptr,
    nullptr, match_11, match_12, nullptr, nullptr, nullptr, nullptr, nullptr, match_18, nullptr,
    nullptr, nullptr, match_22, nullptr, match_24, nullptr, nullptr, nullptr, nullptr, match_29,
    nullptr, nullptr, nullptr, nullptr, nullptr, match_35, nullptr, nullptr, nullptr, nullptr,
    nullptr, match_41, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    match_50, nullptr, match_52, match_53, nullptr, nullptr, match_56, match_57, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    match_70, match_71, match_72, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_79,
    nullptr, nullptr, nullptr, match_83, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, match_95, nullptr, nullptr, nullptr, nullptr,
    match_100, nullptr, nullptr, match_103, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, match_115, nullptr, nullptr, match_118, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, match_125, nullptr, nullptr, nullptr, nullptr,
    nullptr, match_131, nullptr, nullptr, nullptr, match_135, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_149,
    nullptr, match_151, nullptr, nullptr, nullptr, match_155, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, match_163, nullptr, nullptr, nullptr, nullptr, match_168, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, match_181, nullptr, nullptr, match_184, nullptr, nullptr, nullptr, nullptr, nullptr,
    match_190, match_191, nullptr, nullptr, nullptr, nullptr, nullptr, match_197, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_209,
    match_210, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_219,
    nullptr, match_221, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_229,
    match_230, nullptr, nullptr, match_233, nullptr, nullptr, nullptr, nullptr, match_238, nullptr,
    match_240, nullptr, nullptr, nullptr, nullptr, match_245, match_246, match_247, nullptr, match_249,
    nullptr, match_251, nullptr, nullptr, match_254, nullptr, nullptr, nullptr, nullptr, match_259,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, match_273, match_274, nullptr, nullptr, nullptr, match_278, nullptr,
    nullptr, match_281, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_288, nullptr,
    nullptr, nullptr, nullptr, nullptr, match_294, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, match_303, nullptr, nullptr, match_306,
};

Request::HttpHeader Http::Request::matchHttpHeader(const char* headerName, size_t size)
{
    auto result = HttpHeader::UnknownHeader;
    if (size > 0)
    {
        const auto hashFn = matchFunctionTable[hashHeaderName(headerName, size)];
        if (hashFn)
        {
            result = hashFn(headerName, size);
        }
    }

    return result;
}
