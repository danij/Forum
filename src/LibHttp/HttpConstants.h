#pragma once

#include <boost/utility/string_view.hpp>

namespace Http
{

#ifdef DELETE
#undef DELETE
#endif

    namespace Buffer
    {
        /**
        * Each request needs at least one buffer; the request header must fit into one buffer to avoid fragmentation
        */
        static constexpr size_t ReadBufferSize = 4096;
        /**
        * The body of a request can occupy at most this amount of buffers
        */
        static constexpr size_t MaximumBuffersForRequestBody = 100;
        /**
        * The maximum size of a request body
        */
        static constexpr size_t MaxRequestBodyLength = ReadBufferSize * MaximumBuffersForRequestBody;
        /**
        * The response can occupy at most this amount of buffers
        */
        static constexpr size_t MaximumBuffersForResponse = 256;
        /**
        * Each response can request multiple buffers
        */
        static constexpr size_t WriteBufferSize = 8192;
    }

    enum class HttpVerb
    {
        UNKNOWN = 0,
        GET,
        POST,
        PUT,
        PATCH,
        DELETE,

        HTTP_VERBS_COUNT
    };

    //Status codes extracted from https://tools.ietf.org/html/rfc7231 and https://tools.ietf.org/html/rfc6585
    enum HttpStatusCode
    {
        UnknownStatusCode = 0,
        Continue = 100,
        Switching_Protocols = 101,
        OK = 200,
        Created = 201,
        Accepted = 202,
        Non_Authoritative_Information = 203,
        No_Content = 204,
        Reset_Content = 205,
        Partial_Content = 206,
        Multiple_Choices = 300,
        Moved_Permanently = 301,
        Found = 302,
        See_Other = 303,
        Not_Modified = 304,
        Use_Proxy = 305,
        Temporary_Redirect = 307,
        Bad_Request = 400,
        Unauthorized = 401,
        Payment_Required = 402,
        Forbidden = 403,
        Not_Found = 404,
        Method_Not_Allowed = 405,
        Not_Acceptable = 406,
        Proxy_Authentication_Required = 407,
        Request_Timeout = 408,
        Conflict = 409,
        Gone = 410,
        Length_Required = 411,
        Precondition_Failed = 412,
        Payload_Too_Large = 413,
        URI_Too_Long = 414,
        Unsupported_Media_Type = 415,
        Range_Not_Satisfiable = 416,
        Expectation_Failed = 417,
        Upgrade_Required = 426,
        Precondition_Required = 428,
        Too_Many_Requests = 429,
        Request_Header_Fields_Too_Large = 431,
        Internal_Server_Error = 500,
        Not_Implemented = 501,
        Bad_Gateway = 502,
        Service_Unavailable = 503,
        Gateway_Timeout = 504,
        HTTP_Version_Not_Supported = 505,
        Network_Authentication_Required = 511,

        HTTP_STATUS_CODES_COUNT
    };

    typedef boost::string_view StringView;

    StringView getStatusCodeString(HttpStatusCode code);

    namespace Request
    {
        //Standard and common non-standard headers extracted from 
        //https://www.w3.org/Protocols/rfc2616/rfc2616.html and https://en.wikipedia.org/wiki/List_of_HTTP_header_fields
        enum HttpHeader
        {
            UnknownHeader = 0,

            Accept,
            Accept_Charset,
            Accept_Encoding,
            Accept_Language,
            Accept_Ranges,
            Age,
            Allow,
            Authorization,
            Cache_Control,
            Connection,
            Content_Encoding,
            Content_Language,
            Content_Length,
            Content_Location,
            Content_MD5,
            Content_Range,
            Content_Type,
            Date,
            DNT,
            ETag,
            Expect,
            Expires,
            From,
            Front_End_Https,
            Host,
            If_Match,
            If_Modified_Since,
            If_None_Match,
            If_Range,
            If_Unmodified_Since,
            Last_Modified,
            Location,
            Max_Forwards,
            Pragma,
            Proxy_Authenticate,
            Proxy_Authorization,
            Proxy_Connection,
            Range,
            Referer,
            Retry_After,
            Server,
            TE,
            Trailer,
            Transfer_Encoding,
            Upgrade,
            User_Agent,
            Vary,
            Via,
            Warning,
            WWW_Authenticate,
            X_ATT_DeviceId,
            X_Correlation_ID,
            X_Csrf_Token,
            X_Forwarded_For,
            X_Forwarded_Host,
            X_Forwarded_Proto,
            X_Http_Method_Override,
            X_Request_ID,
            X_Requested_With,
            X_UIDH,
            X_Wap_Profile,

            HTTP_HEADERS_COUNT
        };

        HttpHeader matchHttpHeader(const char* headerName, size_t size);
    }
}