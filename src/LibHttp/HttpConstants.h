#pragma once

#include <boost/utility/string_view.hpp>

namespace Http
{
    //Status codes extracted from https://tools.ietf.org/html/rfc7231
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
        Internal_Server_Error = 500,
        Not_Implemented = 501,
        Bad_Gateway = 502,
        Service_Unavailable = 503,
        Gateway_Timeout = 504,
        HTTP_Version_Not_Supported = 505
    };

    typedef boost::string_view StringView;

    inline StringView getStatusCodeString(HttpStatusCode code)
    {
        switch (code)
        {
        case Continue:
            return{ "Continue", 8 };
        case Switching_Protocols:
            return{ "Switching Protocols", 19 };
        case OK:
            return{ "OK", 2 };
        case Created:
            return{ "Created", 7 };
        case Accepted:
            return{ "Accepted", 8 };
        case Non_Authoritative_Information:
            return{ "Non-Authoritative Information", 29 };
        case No_Content:
            return{ "No Content", 10 };
        case Reset_Content:
            return{ "Reset Content", 13 };
        case Partial_Content:
            return{ "Partial Content", 15 };
        case Multiple_Choices:
            return{ "Multiple Choices", 16 };
        case Moved_Permanently:
            return{ "Moved Permanently", 17 };
        case Found:
            return{ "Found", 5 };
        case See_Other:
            return{ "See Other", 9 };
        case Not_Modified:
            return{ "Not Modified", 12 };
        case Use_Proxy:
            return{ "Use Proxy", 9 };
        case Temporary_Redirect:
            return{ "Temporary Redirect", 18 };
        case Bad_Request:
            return{ "Bad Request", 11 };
        case Unauthorized:
            return{ "Unauthorized", 12 };
        case Payment_Required:
            return{ "Payment Required", 16 };
        case Forbidden:
            return{ "Forbidden", 9 };
        case Not_Found:
            return{ "Not Found", 9 };
        case Method_Not_Allowed:
            return{ "Method Not Allowed", 18 };
        case Not_Acceptable:
            return{ "Not Acceptable", 14 };
        case Proxy_Authentication_Required:
            return{ "Proxy Authentication Required", 29 };
        case Request_Timeout:
            return{ "Request Timeout", 15 };
        case Conflict:
            return{ "Conflict", 8 };
        case Gone:
            return{ "Gone", 4 };
        case Length_Required:
            return{ "Length Required", 15 };
        case Precondition_Failed:
            return{ "Precondition Failed", 19 };
        case Payload_Too_Large:
            return{ "Payload Too Large", 17 };
        case URI_Too_Long:
            return{ "URI Too Long", 12 };
        case Unsupported_Media_Type:
            return{ "Unsupported Media Type", 22 };
        case Range_Not_Satisfiable:
            return{ "Range Not Satisfiable", 21 };
        case Expectation_Failed:
            return{ "Expectation Failed", 18 };
        case Upgrade_Required:
            return{ "Upgrade Required", 16 };
        case Internal_Server_Error:
            return{ "Internal Server Error", 21 };
        case Not_Implemented:
            return{ "Not Implemented", 15 };
        case Bad_Gateway:
            return{ "Bad Gateway", 11 };
        case Service_Unavailable:
            return{ "Service Unavailable", 19 };
        case Gateway_Timeout:
            return{ "Gateway Timeout", 15 };
        case HTTP_Version_Not_Supported:
            return{ "HTTP Version Not Supported", 26 };
        default:
            return{ "Unknown", 0 };
        }
    }

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