#pragma once
#include "riff.h"
#include <array>
namespace producer::app {
using ParamGuid=std::array<std::uint8_t,16>;
extern const ParamGuid param_time_music,param_time_reference;
struct ParamObject {ParamGuid timeFormat{};std::uint32_t pchannel=0,stage=0x2300,buffer=0;ParamGuid objectClass{};std::uint32_t index=0;};
struct ParamCurve {std::int32_t start=0,end=0;float startValue=0,endValue=0;std::uint32_t type=1,flags=0;};
struct ParamParameter {std::uint32_t index=0;std::vector<ParamCurve> curves;};
struct ParamControlObject {ParamObject address;std::vector<ParamParameter> parameters;};
bool valid_param_object(const ParamObject&);
bool valid_param_curve(const ParamCurve&);
std::vector<ParamControlObject> param_control_objects(const Chunk&);
Chunk param_control_track();
bool add_param_object(Chunk&,const ParamObject&);
bool edit_param_object(Chunk&,size_t,const ParamObject&);
bool delete_param_object(Chunk&,size_t);
bool add_param_parameter(Chunk&,size_t,std::uint32_t);
bool delete_param_parameter(Chunk&,size_t,size_t);
bool add_param_curve(Chunk&,size_t,size_t,const ParamCurve&,size_t* =nullptr);
bool edit_param_curve(Chunk&,size_t,size_t,size_t,const ParamCurve&,size_t* =nullptr);
bool delete_param_curve(Chunk&,size_t,size_t,size_t);
Bytes copy_param_curve(const Chunk&,size_t,size_t,size_t);
bool paste_param_curve(Chunk&,size_t,size_t,const Bytes&,std::int32_t,size_t* =nullptr);
}
