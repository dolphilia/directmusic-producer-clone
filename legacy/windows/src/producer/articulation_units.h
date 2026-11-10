#pragma once
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
namespace producer::app {
enum class ArticulationUnit {Raw,Cents,Timecents,Centibels};
inline ArticulationUnit articulation_unit(std::uint16_t destination){
    switch(destination){
    case 1:case 0x501:return ArticulationUnit::Centibels;
    case 3:case 0x104:case 0x114:case 0x500:return ArticulationUnit::Cents;
    case 0x105:case 0x115:case 0x206:case 0x207:case 0x209:
    case 0x20b:case 0x20c:case 0x20d:case 0x30a:case 0x30b:
    case 0x30d:case 0x30f:case 0x310:return ArticulationUnit::Timecents;
    default:return ArticulationUnit::Raw;
    }
}
inline const wchar_t* articulation_unit_label(ArticulationUnit unit){
    switch(unit){case ArticulationUnit::Cents:return L"Scale (cents)";
    case ArticulationUnit::Timecents:return L"Scale (timecents)";
    case ArticulationUnit::Centibels:return L"Scale (centibels)";
    default:return L"Scale (signed raw DLS value)";}
}
// Exact terminating decimal: every signed 16.16 value round-trips, including
// INT32_MIN and INT32_MAX. No locale-dependent separator or floating point.
inline std::wstring format_articulation_scale(std::int32_t raw,bool units){
    if(!units)return std::to_wstring(raw);
    const auto signedValue=static_cast<std::int64_t>(raw);
    auto magnitude=static_cast<std::uint64_t>(signedValue<0?-signedValue:signedValue);
    auto result=(raw<0?L"-":L"")+std::to_wstring(magnitude/65536);
    auto remainder=magnitude%65536;
    if(remainder){result+=L'.';while(remainder){remainder*=10;result+=static_cast<wchar_t>(L'0'+remainder/65536);remainder%=65536;}}
    return result;
}
inline std::int32_t parse_articulation_scale(std::wstring_view text,bool units){
    const auto invalid=[](){throw std::runtime_error("Scale requires a decimal value in range; use a dot and at most 16 fractional digits");};
    if(text.empty())invalid();bool negative=false;size_t at=0;
    if(text[at]==L'-'||text[at]==L'+'){negative=text[at]==L'-';if(++at==text.size())invalid();}
    std::uint64_t whole=0,fraction=0;unsigned digits=0;bool dot=false,any=false;
    for(;at<text.size();++at){auto c=text[at];if(c==L'.'&&units&&!dot){dot=true;continue;}
        if(c<L'0'||c>L'9')invalid();any=true;
        if(dot){if(++digits>16)invalid();fraction=fraction*10+static_cast<unsigned>(c-L'0');}
        else{whole=whole*10+static_cast<unsigned>(c-L'0');if(whole>(units?32768u:2147483648u))invalid();}
    }
    if(!any||(dot&&!digits))invalid();
    std::uint64_t magnitude=whole;
    if(units){magnitude*=65536;std::uint64_t divisor=1;for(unsigned i=0;i<digits;++i)divisor*=5;
        // Cancel 2^digits from the decimal divisor before multiplying; all
        // intermediate values stay below 10^16, even for 16-digit fractions.
        auto numerator=fraction*(std::uint64_t{1}<<(16-digits));
        magnitude+=(numerator+divisor/2)/divisor;
    }
    const auto limit=negative?2147483648ULL:2147483647ULL;if(magnitude>limit)invalid();
    auto value=static_cast<std::int64_t>(magnitude);return static_cast<std::int32_t>(negative?-value:value);
}
}
