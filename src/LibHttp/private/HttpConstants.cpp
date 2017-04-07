#include "HttpConstants.h"
#include "StringMatching.h"

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

static HttpHeader match_5(const char* value, size_t size)
{
    return ((size == 7) && matchStringUpperOrLower(value, "wWaArRnNiInNgG")) ? HttpHeader::Warning : HttpHeader::UnknownHeader;
}
static HttpHeader match_8(const char* value, size_t size)
{
    return ((size == 15) && matchStringUpperOrLower(value, "aAcCcCeEpPtT--lLaAnNgGuUaAgGeE")) ? HttpHeader::Accept_Language : HttpHeader::UnknownHeader;
}
static HttpHeader match_11(const char* value, size_t size)
{
    return ((size == 7) && matchStringUpperOrLower(value, "eExXpPiIrReEsS")) ? HttpHeader::Expires : HttpHeader::UnknownHeader;
}
static HttpHeader match_12(const char* value, size_t size)
{
    return ((size == 15) && matchStringUpperOrLower(value, "aAcCcCeEpPtT--eEnNcCoOdDiInNgG")) ? HttpHeader::Accept_Encoding : HttpHeader::UnknownHeader;
}
static HttpHeader match_18(const char* value, size_t size)
{
    return ((size == 22) && matchStringUpperOrLower(value, "xX--hHtTtTpP--mMeEtThHoOdD--oOvVeErRrRiIdDeE")) ? HttpHeader::X_Http_Method_Override : HttpHeader::UnknownHeader;
}
static HttpHeader match_22(const char* value, size_t size)
{
    return ((size == 7) && matchStringUpperOrLower(value, "rReEfFeErReErR")) ? HttpHeader::Referer : HttpHeader::UnknownHeader;
}
static HttpHeader match_24(const char* value, size_t size)
{
    return ((size == 7) && matchStringUpperOrLower(value, "tTrRaAiIlLeErR")) ? HttpHeader::Trailer : HttpHeader::UnknownHeader;
}
static HttpHeader match_29(const char* value, size_t size)
{
    return ((size == 8) && matchStringUpperOrLower(value, "iIfF--rRaAnNgGeE")) ? HttpHeader::If_Range : HttpHeader::UnknownHeader;
}
static HttpHeader match_35(const char* value, size_t size)
{
    return ((size == 8) && matchStringUpperOrLower(value, "iIfF--mMaAtTcChH")) ? HttpHeader::If_Match : HttpHeader::UnknownHeader;
}
static HttpHeader match_41(const char* value, size_t size)
{
    return ((size == 15) && matchStringUpperOrLower(value, "fFrRoOnNtT--eEnNdD--hHtTtTpPsS")) ? HttpHeader::Front_End_Https : HttpHeader::UnknownHeader;
}
static HttpHeader match_50(const char* value, size_t size)
{
    return ((size == 8) && matchStringUpperOrLower(value, "lLoOcCaAtTiIoOnN")) ? HttpHeader::Location : HttpHeader::UnknownHeader;
}
static HttpHeader match_52(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--lLaAnNgGuUaAgGeE")) ? HttpHeader::Content_Language : HttpHeader::UnknownHeader;
}
static HttpHeader match_53(const char* value, size_t size)
{
    return ((size == 11) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--mMdD55")) ? HttpHeader::Content_MD5 : HttpHeader::UnknownHeader;
}
static HttpHeader match_56(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--eEnNcCoOdDiInNgG")) ? HttpHeader::Content_Encoding : HttpHeader::UnknownHeader;
}
static HttpHeader match_57(const char* value, size_t size)
{
    return ((size == 15) && matchStringUpperOrLower(value, "xX--fFoOrRwWaArRdDeEdD--fFoOrR")) ? HttpHeader::X_Forwarded_For : HttpHeader::UnknownHeader;
}
static HttpHeader match_70(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--lLoOcCaAtTiIoOnN")) ? HttpHeader::Content_Location : HttpHeader::UnknownHeader;
}
static HttpHeader match_71(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "xX--cCoOrRrReElLaAtTiIoOnN--iIdD")) ? HttpHeader::X_Correlation_ID : HttpHeader::UnknownHeader;
}
static HttpHeader match_72(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "wWwWwW--aAuUtThHeEnNtTiIcCaAtTeE")) ? HttpHeader::WWW_Authenticate : HttpHeader::UnknownHeader;
}
static HttpHeader match_79(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "xX--rReEqQuUeEsStTeEdD--wWiItThH")) ? HttpHeader::X_Requested_With : HttpHeader::UnknownHeader;
}
static HttpHeader match_83(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "pPrRoOxXyY--cCoOnNnNeEcCtTiIoOnN")) ? HttpHeader::Proxy_Connection : HttpHeader::UnknownHeader;
}
static HttpHeader match_95(const char* value, size_t size)
{
    return ((size == 2) && matchStringUpperOrLower(value, "tTeE")) ? HttpHeader::TE : HttpHeader::UnknownHeader;
}
static HttpHeader match_100(const char* value, size_t size)
{
    return ((size == 17) && matchStringUpperOrLower(value, "iIfF--mMoOdDiIfFiIeEdD--sSiInNcCeE")) ? HttpHeader::If_Modified_Since : HttpHeader::UnknownHeader;
}
static HttpHeader match_103(const char* value, size_t size)
{
    return ((size == 16) && matchStringUpperOrLower(value, "xX--fFoOrRwWaArRdDeEdD--hHoOsStT")) ? HttpHeader::X_Forwarded_Host : HttpHeader::UnknownHeader;
}
static HttpHeader match_115(const char* value, size_t size)
{
    return ((size == 17) && matchStringUpperOrLower(value, "tTrRaAnNsSfFeErR--eEnNcCoOdDiInNgG")) ? HttpHeader::Transfer_Encoding : HttpHeader::UnknownHeader;
}
static HttpHeader match_118(const char* value, size_t size)
{
    return ((size == 3) && matchStringUpperOrLower(value, "aAgGeE")) ? HttpHeader::Age : HttpHeader::UnknownHeader;
}
static HttpHeader match_125(const char* value, size_t size)
{
    return ((size == 10) && matchStringUpperOrLower(value, "cCoOnNnNeEcCtTiIoOnN")) ? HttpHeader::Connection : HttpHeader::UnknownHeader;
}
static HttpHeader match_131(const char* value, size_t size)
{
    return ((size == 3) && matchStringUpperOrLower(value, "vViIaA")) ? HttpHeader::Via : HttpHeader::UnknownHeader;
}
static HttpHeader match_135(const char* value, size_t size)
{
    return ((size == 17) && matchStringUpperOrLower(value, "xX--fFoOrRwWaArRdDeEdD--pPrRoOtToO")) ? HttpHeader::X_Forwarded_Proto : HttpHeader::UnknownHeader;
}
static HttpHeader match_149(const char* value, size_t size)
{
    return ((size == 18) && matchStringUpperOrLower(value, "pPrRoOxXyY--aAuUtThHeEnNtTiIcCaAtTeE")) ? HttpHeader::Proxy_Authenticate : HttpHeader::UnknownHeader;
}
static HttpHeader match_151(const char* value, size_t size)
{
    return ((size == 3) && matchStringUpperOrLower(value, "dDnNtT")) ? HttpHeader::DNT : HttpHeader::UnknownHeader;
}
static HttpHeader match_155(const char* value, size_t size)
{
    return ((size == 10) && matchStringUpperOrLower(value, "uUsSeErR--aAgGeEnNtT")) ? HttpHeader::User_Agent : HttpHeader::UnknownHeader;
}
static HttpHeader match_163(const char* value, size_t size)
{
    return ((size == 4) && matchStringUpperOrLower(value, "dDaAtTeE")) ? HttpHeader::Date : HttpHeader::UnknownHeader;
}
static HttpHeader match_168(const char* value, size_t size)
{
    return ((size == 4) && matchStringUpperOrLower(value, "eEtTaAgG")) ? HttpHeader::ETag : HttpHeader::UnknownHeader;
}
static HttpHeader match_181(const char* value, size_t size)
{
    return ((size == 4) && matchStringUpperOrLower(value, "fFrRoOmM")) ? HttpHeader::From : HttpHeader::UnknownHeader;
}
static HttpHeader match_184(const char* value, size_t size)
{
    return ((size == 19) && matchStringUpperOrLower(value, "iIfF--uUnNmMoOdDiIfFiIeEdD--sSiInNcCeE")) ? HttpHeader::If_Unmodified_Since : HttpHeader::UnknownHeader;
}
static HttpHeader match_190(const char* value, size_t size)
{
    return ((size == 11) && matchStringUpperOrLower(value, "rReEtTrRyY--aAfFtTeErR")) ? HttpHeader::Retry_After : HttpHeader::UnknownHeader;
}
static HttpHeader match_191(const char* value, size_t size)
{
    return ((size == 12) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--tTyYpPeE")) ? HttpHeader::Content_Type : HttpHeader::UnknownHeader;
}
static HttpHeader match_197(const char* value, size_t size)
{
    return ((size == 4) && matchStringUpperOrLower(value, "hHoOsStT")) ? HttpHeader::Host : HttpHeader::UnknownHeader;
}
static HttpHeader match_209(const char* value, size_t size)
{
    return ((size == 19) && matchStringUpperOrLower(value, "pPrRoOxXyY--aAuUtThHoOrRiIzZaAtTiIoOnN")) ? HttpHeader::Proxy_Authorization : HttpHeader::UnknownHeader;
}
static HttpHeader match_210(const char* value, size_t size)
{
    return ((size == 12) && matchStringUpperOrLower(value, "xX--rReEqQuUeEsStT--iIdD")) ? HttpHeader::X_Request_ID : HttpHeader::UnknownHeader;
}
static HttpHeader match_219(const char* value, size_t size)
{
    return ((size == 5) && matchStringUpperOrLower(value, "rRaAnNgGeE")) ? HttpHeader::Range : HttpHeader::UnknownHeader;
}
static HttpHeader match_221(const char* value, size_t size)
{
    return ((size == 4) && matchStringUpperOrLower(value, "vVaArRyY")) ? HttpHeader::Vary : HttpHeader::UnknownHeader;
}
static HttpHeader match_229(const char* value, size_t size)
{
    return ((size == 12) && matchStringUpperOrLower(value, "mMaAxX--fFoOrRwWaArRdDsS")) ? HttpHeader::Max_Forwards : HttpHeader::UnknownHeader;
}
static HttpHeader match_230(const char* value, size_t size)
{
    return ((size == 12) && matchStringUpperOrLower(value, "xX--cCsSrRfF--tToOkKeEnN")) ? HttpHeader::X_Csrf_Token : HttpHeader::UnknownHeader;
}
static HttpHeader match_233(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--rRaAnNgGeE")) ? HttpHeader::Content_Range : HttpHeader::UnknownHeader;
}
static HttpHeader match_238(const char* value, size_t size)
{
    return ((size == 5) && matchStringUpperOrLower(value, "aAlLlLoOwW")) ? HttpHeader::Allow : HttpHeader::UnknownHeader;
}
static HttpHeader match_240(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "lLaAsStT--mMoOdDiIfFiIeEdD")) ? HttpHeader::Last_Modified : HttpHeader::UnknownHeader;
}
static HttpHeader match_245(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "iIfF--nNoOnNeE--mMaAtTcChH")) ? HttpHeader::If_None_Match : HttpHeader::UnknownHeader;
}
static HttpHeader match_247(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "cCaAcChHeE--cCoOnNtTrRoOlL")) ? HttpHeader::Cache_Control : HttpHeader::UnknownHeader;
}
static HttpHeader match_249(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "aAuUtThHoOrRiIzZaAtTiIoOnN")) ? HttpHeader::Authorization : HttpHeader::UnknownHeader;
}
static HttpHeader match_251(const char* value, size_t size)
{
    return ((size == 6) && matchStringUpperOrLower(value, "pPrRaAgGmMaA")) ? HttpHeader::Pragma : HttpHeader::UnknownHeader;
}
static HttpHeader match_254(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "xX--wWaApP--pPrRoOfFiIlLeE")) ? HttpHeader::X_Wap_Profile : HttpHeader::UnknownHeader;
}
static HttpHeader match_259(const char* value, size_t size)
{
    return ((size == 13) && matchStringUpperOrLower(value, "aAcCcCeEpPtT--rRaAnNgGeEsS")) ? HttpHeader::Accept_Ranges : HttpHeader::UnknownHeader;
}
static HttpHeader match_273(const char* value, size_t size)
{
    return ((size == 6) && matchStringUpperOrLower(value, "xX--uUiIdDhH")) ? HttpHeader::X_UIDH : HttpHeader::UnknownHeader;
}
static HttpHeader match_274(const char* value, size_t size)
{
    return ((size == 6) && matchStringUpperOrLower(value, "aAcCcCeEpPtT")) ? HttpHeader::Accept : HttpHeader::UnknownHeader;
}
static HttpHeader match_278(const char* value, size_t size)
{
    return ((size == 6) && matchStringUpperOrLower(value, "eExXpPeEcCtT")) ? HttpHeader::Expect : HttpHeader::UnknownHeader;
}
static HttpHeader match_281(const char* value, size_t size)
{
    return ((size == 14) && matchStringUpperOrLower(value, "cCoOnNtTeEnNtT--lLeEnNgGtThH")) ? HttpHeader::Content_Length : HttpHeader::UnknownHeader;
}
static HttpHeader match_288(const char* value, size_t size)
{
    return ((size == 6) && matchStringUpperOrLower(value, "sSeErRvVeErR")) ? HttpHeader::Server : HttpHeader::UnknownHeader;
}
static HttpHeader match_294(const char* value, size_t size)
{
    return ((size == 14) && matchStringUpperOrLower(value, "xX--aAtTtT--dDeEvViIcCeEiIdD")) ? HttpHeader::X_ATT_DeviceId : HttpHeader::UnknownHeader;
}
static HttpHeader match_303(const char* value, size_t size)
{
    return ((size == 14) && matchStringUpperOrLower(value, "aAcCcCeEpPtT--cChHaArRsSeEtT")) ? HttpHeader::Accept_Charset : HttpHeader::UnknownHeader;
}
static HttpHeader match_306(const char* value, size_t size)
{
    return ((size == 7) && matchStringUpperOrLower(value, "uUpPgGrRaAdDeE")) ? HttpHeader::Upgrade : HttpHeader::UnknownHeader;
}

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