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
#include "Trie.h"

#include <cstddef>
#include <cstdint>
#include <string>

using namespace Http;
using namespace std::literals::string_literals;

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

HttpStringView Http::getStatusCodeString(const HttpStatusCode code)
{
    static_assert(std::size(statusCodes) >= (HTTP_STATUS_CODES_COUNT - 1),
                  "statusCodes array is not big enough");

    HttpStringView result;
    if (code > 0 && code < HttpStatusCode::HTTP_STATUS_CODES_COUNT)
    {
        result = statusCodes[code];
    }
    return ! result.empty() ? result : HttpStringView("Unknown", 7);
}

static const ImmutableAsciiCaseInsensitiveTrie<Request::HttpHeader> httpHeaders
{
    std::make_pair("accept-charset"s, Request::HttpHeader::Accept_Charset),
    std::make_pair("accept-encoding"s, Request::HttpHeader::Accept_Encoding),
    std::make_pair("accept-language"s, Request::HttpHeader::Accept_Language),
    std::make_pair("accept-ranges"s, Request::HttpHeader::Accept_Ranges),
    std::make_pair("accept"s, Request::HttpHeader::Accept),
    std::make_pair("age"s, Request::HttpHeader::Age),
    std::make_pair("allow"s, Request::HttpHeader::Allow),
    std::make_pair("authorization"s, Request::HttpHeader::Authorization),
    std::make_pair("cache-control"s, Request::HttpHeader::Cache_Control),
    std::make_pair("connection"s, Request::HttpHeader::Connection),
    std::make_pair("content-encoding"s, Request::HttpHeader::Content_Encoding),
    std::make_pair("content-language"s, Request::HttpHeader::Content_Language),
    std::make_pair("content-length"s, Request::HttpHeader::Content_Length),
    std::make_pair("content-location"s, Request::HttpHeader::Content_Location),
    std::make_pair("content-md5"s, Request::HttpHeader::Content_MD5),
    std::make_pair("content-range"s, Request::HttpHeader::Content_Range),
    std::make_pair("content-type"s, Request::HttpHeader::Content_Type),
    std::make_pair("cookie"s, Request::HttpHeader::Cookie),
    std::make_pair("date"s, Request::HttpHeader::Date),
    std::make_pair("dnt"s, Request::HttpHeader::DNT),
    std::make_pair("etag"s, Request::HttpHeader::ETag),
    std::make_pair("expect"s, Request::HttpHeader::Expect),
    std::make_pair("expiresS"s, Request::HttpHeader::Expires),
    std::make_pair("from"s, Request::HttpHeader::From),
    std::make_pair("front-end-https"s, Request::HttpHeader::Front_End_Https),
    std::make_pair("host"s, Request::HttpHeader::Host),
    std::make_pair("if-match"s, Request::HttpHeader::If_Match),
    std::make_pair("if-modified-since"s, Request::HttpHeader::If_Modified_Since),
    std::make_pair("if-none-match"s, Request::HttpHeader::If_None_Match),
    std::make_pair("if-range"s, Request::HttpHeader::If_Range),
    std::make_pair("if-unmodified-since"s, Request::HttpHeader::If_Unmodified_Since),
    std::make_pair("last-modified"s, Request::HttpHeader::Last_Modified),
    std::make_pair("location"s, Request::HttpHeader::Location),
    std::make_pair("max-forwards"s, Request::HttpHeader::Max_Forwards),
    std::make_pair("pragma"s, Request::HttpHeader::Pragma),
    std::make_pair("proxy-authenticate"s, Request::HttpHeader::Proxy_Authenticate),
    std::make_pair("proxy-authorization"s, Request::HttpHeader::Proxy_Authorization),
    std::make_pair("proxy-connection"s, Request::HttpHeader::Proxy_Connection),
    std::make_pair("range"s, Request::HttpHeader::Range),
    std::make_pair("referer"s, Request::HttpHeader::Referer),
    std::make_pair("retry-after"s, Request::HttpHeader::Retry_After),
    std::make_pair("server"s, Request::HttpHeader::Server),
    std::make_pair("te"s, Request::HttpHeader::TE),
    std::make_pair("trailer"s, Request::HttpHeader::Trailer),
    std::make_pair("transfer-encoding"s, Request::HttpHeader::Transfer_Encoding),
    std::make_pair("upgrade"s, Request::HttpHeader::Upgrade),
    std::make_pair("user-agent"s, Request::HttpHeader::User_Agent),
    std::make_pair("vary"s, Request::HttpHeader::Vary),
    std::make_pair("via"s, Request::HttpHeader::Via),
    std::make_pair("warning"s, Request::HttpHeader::Warning),
    std::make_pair("www-authenticate"s, Request::HttpHeader::WWW_Authenticate),
    std::make_pair("x-att-deviceid"s, Request::HttpHeader::X_ATT_DeviceId),
    std::make_pair("x-correlation-id"s, Request::HttpHeader::X_Correlation_ID),
    std::make_pair("x-csrf-token"s, Request::HttpHeader::X_Csrf_Token),
    std::make_pair("x-forwarded-for"s, Request::HttpHeader::X_Forwarded_For),
    std::make_pair("x-forwarded-host"s, Request::HttpHeader::X_Forwarded_Host),
    std::make_pair("x-forwarded-proto"s, Request::HttpHeader::X_Forwarded_Proto),
    std::make_pair("x-http-method-override"s, Request::HttpHeader::X_Http_Method_Override),
    std::make_pair("x-request-id"s, Request::HttpHeader::X_Request_ID),
    std::make_pair("x-requested-with"s, Request::HttpHeader::X_Requested_With),
    std::make_pair("x-uidh"s, Request::HttpHeader::X_UIDH),
    std::make_pair("x-wap-profile"s, Request::HttpHeader::X_Wap_Profile)
};

Request::HttpHeader Http::Request::matchHttpHeader(const HttpStringView header)
{
    const auto result = httpHeaders.find(header);

    return result ? *result : HttpHeader::UnknownHeader;
}
