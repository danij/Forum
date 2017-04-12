#include "HttpConstants.h"
#include "StringHelpers.h"

#include <cstddef>
#include <cstdint>

using namespace Http;

static constexpr uint8_t charLower[]=
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

static StringView statusCodes[] = 
{
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Continue", 8 },{ "Switching Protocols", 19 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "OK", 2 },{ "Created", 7 },{ "Accepted", 8 },{ "Non-Authoritative Information", 29 },
    { "No Content", 10 },{ "Reset Content", 13 },{ "Partial Content", 15 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Multiple Choices", 16 },{ "Moved Permanently", 17 },{ "Found", 5 },{ "See Other", 9 },
    { "Not Modified", 12 },{ "Use Proxy", 9 },{ "Unknown", 7 },{ "Temporary Redirect", 18 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Bad Request", 11 },{ "Unauthorized", 12 },{ "Payment Required", 16 },{ "Forbidden", 9 },
    { "Not Found", 9 },{ "Method Not Allowed", 18 },{ "Not Acceptable", 14 },{ "Proxy Authentication Required", 29 },
    { "Request Timeout", 15 },{ "Conflict", 8 },{ "Gone", 4 },{ "Length Required", 15 },{ "Precondition Failed", 19 },
    { "Payload Too Large", 17 },{ "URI Too Long", 12 },{ "Unsupported Media Type", 22 },{ "Range Not Satisfiable", 21 },
    { "Expectation Failed", 18 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Upgrade Required", 16 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },{ "Unknown", 7 },
    { "Unknown", 7 },{ "Internal Server Error", 21 },{ "Not Implemented", 15 },{ "Bad Gateway", 11 },
    { "Service Unavailable", 19 },{ "Gateway Timeout", 15 },{ "HTTP Version Not Supported", 26 },

};

StringView Http::getStatusCodeString(HttpStatusCode code)
{
    if (code > 0 && code < HttpStatusCode::HTTP_STATUS_CODES_COUNT)
    {
        return statusCodes[code];
    }
    return statusCodes[0];
}

static int hashHeaderName(const char* text, size_t size)
{
    constexpr int maxHeaderNameSize = 22;
    static int coefficients[] = { 1, 2, 42, 307 };
    return 
    (
        charLower[static_cast<uint8_t>(text[0])]        * coefficients[0] +
        charLower[static_cast<uint8_t>(text[size - 1])] * coefficients[1] +
        (size % maxHeaderNameSize)                      * coefficients[2]
    ) % coefficients[3];
}

template<size_t Size>
static constexpr size_t getExpectedSize(const char (&value)[Size])
{
    return (Size - 1) / 2;
}

#define HEADER_COMPARER(hash, string, result) \
    static HttpHeader match_##hash(const char* value, size_t size) \
    { \
        return ((size == getExpectedSize(string)) && matchStringUpperOrLower(value, string)) ? result : HttpHeader::UnknownHeader; \
    }

HEADER_COMPARER(5, "wWaArRnNiInNgG", HttpHeader::Warning)
HEADER_COMPARER(8, "aAcCcCeEpPtT--lLaAnNgGuUaAgGeE", HttpHeader::Accept_Language)
HEADER_COMPARER(11, "eExXpPiIrReEsS", HttpHeader::Expires)
HEADER_COMPARER(12, "aAcCcCeEpPtT--eEnNcCoOdDiInNgG", HttpHeader::Accept_Encoding)
HEADER_COMPARER(18, "xX--hHtTtTpP--mMeEtThHoOdD--oOvVeErRrRiIdDeE", HttpHeader::X_Http_Method_Override)
HEADER_COMPARER(22, "rReEfFeErReErR", HttpHeader::Referer)
HEADER_COMPARER(24, "tTrRaAiIlLeErR", HttpHeader::Trailer)
HEADER_COMPARER(29, "iIfF--rRaAnNgGeE", HttpHeader::If_Range)
HEADER_COMPARER(35, "iIfF--mMaAtTcChH", HttpHeader::If_Match)
HEADER_COMPARER(41, "fFrRoOnNtT--eEnNdD--hHtTtTpPsS", HttpHeader::Front_End_Https)
HEADER_COMPARER(50, "lLoOcCaAtTiIoOnN", HttpHeader::Location)
HEADER_COMPARER(52, "cCoOnNtTeEnNtT--lLaAnNgGuUaAgGeE", HttpHeader::Content_Language)
HEADER_COMPARER(53, "cCoOnNtTeEnNtT--mMdD55", HttpHeader::Content_MD5)
HEADER_COMPARER(56, "cCoOnNtTeEnNtT--eEnNcCoOdDiInNgG", HttpHeader::Content_Encoding)
HEADER_COMPARER(57, "xX--fFoOrRwWaArRdDeEdD--fFoOrR", HttpHeader::X_Forwarded_For)
HEADER_COMPARER(70, "cCoOnNtTeEnNtT--lLoOcCaAtTiIoOnN", HttpHeader::Content_Location)
HEADER_COMPARER(71, "xX--cCoOrRrReElLaAtTiIoOnN--iIdD", HttpHeader::X_Correlation_ID)
HEADER_COMPARER(72, "wWwWwW--aAuUtThHeEnNtTiIcCaAtTeE", HttpHeader::WWW_Authenticate)
HEADER_COMPARER(79, "xX--rReEqQuUeEsStTeEdD--wWiItThH", HttpHeader::X_Requested_With)
HEADER_COMPARER(83, "pPrRoOxXyY--cCoOnNnNeEcCtTiIoOnN", HttpHeader::Proxy_Connection)
HEADER_COMPARER(95, "tTeE", HttpHeader::TE)
HEADER_COMPARER(100, "iIfF--mMoOdDiIfFiIeEdD--sSiInNcCeE", HttpHeader::If_Modified_Since)
HEADER_COMPARER(103, "xX--fFoOrRwWaArRdDeEdD--hHoOsStT", HttpHeader::X_Forwarded_Host)
HEADER_COMPARER(115, "tTrRaAnNsSfFeErR--eEnNcCoOdDiInNgG", HttpHeader::Transfer_Encoding)
HEADER_COMPARER(118, "aAgGeE", HttpHeader::Age)
HEADER_COMPARER(125, "cCoOnNnNeEcCtTiIoOnN", HttpHeader::Connection)
HEADER_COMPARER(131, "vViIaA", HttpHeader::Via)
HEADER_COMPARER(135, "xX--fFoOrRwWaArRdDeEdD--pPrRoOtToO", HttpHeader::X_Forwarded_Proto)
HEADER_COMPARER(149, "pPrRoOxXyY--aAuUtThHeEnNtTiIcCaAtTeE", HttpHeader::Proxy_Authenticate)
HEADER_COMPARER(151, "dDnNtT", HttpHeader::DNT)
HEADER_COMPARER(155, "uUsSeErR--aAgGeEnNtT", HttpHeader::User_Agent)
HEADER_COMPARER(163, "dDaAtTeE", HttpHeader::Date)
HEADER_COMPARER(168, "eEtTaAgG", HttpHeader::ETag)
HEADER_COMPARER(181, "fFrRoOmM", HttpHeader::From)
HEADER_COMPARER(184, "iIfF--uUnNmMoOdDiIfFiIeEdD--sSiInNcCeE", HttpHeader::If_Unmodified_Since)
HEADER_COMPARER(190, "rReEtTrRyY--aAfFtTeErR", HttpHeader::Retry_After)
HEADER_COMPARER(191, "cCoOnNtTeEnNtT--tTyYpPeE", HttpHeader::Content_Type)
HEADER_COMPARER(197, "hHoOsStT", HttpHeader::Host)
HEADER_COMPARER(209, "pPrRoOxXyY--aAuUtThHoOrRiIzZaAtTiIoOnN", HttpHeader::Proxy_Authorization)
HEADER_COMPARER(210, "xX--rReEqQuUeEsStT--iIdD", HttpHeader::X_Request_ID)
HEADER_COMPARER(219, "rRaAnNgGeE", HttpHeader::Range)
HEADER_COMPARER(221, "vVaArRyY", HttpHeader::Vary)
HEADER_COMPARER(229, "mMaAxX--fFoOrRwWaArRdDsS", HttpHeader::Max_Forwards)
HEADER_COMPARER(230, "xX--cCsSrRfF--tToOkKeEnN", HttpHeader::X_Csrf_Token)
HEADER_COMPARER(233, "cCoOnNtTeEnNtT--rRaAnNgGeE", HttpHeader::Content_Range)
HEADER_COMPARER(238, "aAlLlLoOwW", HttpHeader::Allow)
HEADER_COMPARER(240, "lLaAsStT--mMoOdDiIfFiIeEdD", HttpHeader::Last_Modified)
HEADER_COMPARER(245, "iIfF--nNoOnNeE--mMaAtTcChH", HttpHeader::If_None_Match)
HEADER_COMPARER(247, "cCaAcChHeE--cCoOnNtTrRoOlL", HttpHeader::Cache_Control)
HEADER_COMPARER(249, "aAuUtThHoOrRiIzZaAtTiIoOnN", HttpHeader::Authorization)
HEADER_COMPARER(251, "pPrRaAgGmMaA", HttpHeader::Pragma)
HEADER_COMPARER(254, "xX--wWaApP--pPrRoOfFiIlLeE", HttpHeader::X_Wap_Profile)
HEADER_COMPARER(259, "aAcCcCeEpPtT--rRaAnNgGeEsS", HttpHeader::Accept_Ranges)
HEADER_COMPARER(273, "xX--uUiIdDhH", HttpHeader::X_UIDH)
HEADER_COMPARER(274, "aAcCcCeEpPtT", HttpHeader::Accept)
HEADER_COMPARER(278, "eExXpPeEcCtT", HttpHeader::Expect)
HEADER_COMPARER(281, "cCoOnNtTeEnNtT--lLeEnNgGtThH", HttpHeader::Content_Length)
HEADER_COMPARER(288, "sSeErRvVeErR", HttpHeader::Server)
HEADER_COMPARER(294, "xX--aAtTtT--dDeEvViIcCeEiIdD", HttpHeader::X_ATT_DeviceId)
HEADER_COMPARER(303, "aAcCcCeEpPtT--cChHaArRsSeEtT", HttpHeader::Accept_Charset)
HEADER_COMPARER(306, "uUpPgGrRaAdDeE", HttpHeader::Upgrade)

typedef HttpHeader (*MatchFunction)(const char* value, size_t size);

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
    match_240, nullptr, nullptr, nullptr, nullptr, match_245, nullptr, match_247, nullptr, match_249,
    nullptr, match_251, nullptr, nullptr, match_254, nullptr, nullptr, nullptr, nullptr, match_259,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, match_273, match_274, nullptr, nullptr, nullptr, match_278, nullptr,
    nullptr, match_281, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, match_288, nullptr,
    nullptr, nullptr, nullptr, nullptr, match_294, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, match_303, nullptr, nullptr, match_306, 
};

HttpHeader Http::matchHttpHeader(const char* headerName, size_t size)
{
    auto result = HttpHeader::UnknownHeader;
    if (size > 0)
    {
        auto hashFn = matchFunctionTable[hashHeaderName(headerName, size)];
        if (hashFn)
        {
            result = hashFn(headerName, size);
        }
    }

    return result;
}