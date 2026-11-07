#pragma once
#include "ProjectFormat.h"
namespace sct::services::testing {
enum class PacketField {Group,EffectBrush,FlowKind,ConnectorLabel};
inline void SealPacket(std::vector<BYTE>& bytes){
    if(bytes.size()<16)throw std::runtime_error("Fixture packet too short");uint32_t crc=~0u;
    for(size_t i=0;i<bytes.size()-4;++i){crc^=bytes[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xEDB88320u&uint32_t(-int(crc&1)));}crc=~crc;memcpy(bytes.data()+bytes.size()-4,&crc,4);
}
inline void PatchPacketInt(std::vector<BYTE>& bytes,size_t at,int32_t value){if(bytes.size()<4||at>bytes.size()-4||bytes.size()-at<8)throw std::runtime_error("Fixture patch outside payload");memcpy(bytes.data()+at,&value,4);SealPacket(bytes);}
inline size_t LastPacketField(const std::vector<BYTE>& bytes,PacketField field){
    if(bytes.size()<20||memcmp(bytes.data(),"SCTPROJ\0",8))throw std::runtime_error("Fixture packet header");size_t at=8;
    auto skip=[&](size_t n){if(at>bytes.size()-4||n>bytes.size()-4-at)throw std::runtime_error("Fixture layout truncated");at+=n;};
    auto integer=[&]{int32_t n;const size_t old=at;skip(4);memcpy(&n,bytes.data()+old,4);return n;};
    auto counted=[&](size_t size,int maximum){const auto n=integer();if(n<0||n>maximum)throw std::runtime_error("Fixture layout count");skip(size_t(n)*size);};
    const int version=integer();if(version<1||version>14)throw std::runtime_error("Fixture layout version");counted(1,512*1024*1024);const int count=integer();if(count<1||count>10000)throw std::runtime_error("Fixture object count");size_t found=SIZE_MAX;
    for(int i=0;i<count;++i){
        skip(4+sizeof(RECT));counted(sizeof(POINT),1000000);counted(2,65536);counted(2,65536);skip(9*4);if(version>=2)skip(sizeof(POINT));counted(1,512*1024*1024);
        if(version>=3)skip(4*4);if(version>=4)skip(4);if(version>=5)skip(4);if(version>=7){counted(2,80);skip(4);}
        if(version>=6){if(i==count-1&&field==PacketField::Group)found=at;skip(8);}
        if(version>=8){skip(sizeof(RECT));const int runs=integer();if(runs<0||runs>512)throw std::runtime_error("Fixture run count");for(int j=0;j<runs;++j){skip(8);counted(2,128);skip(12);}}
        if(version>=9)skip(7*4);if(version>=10)skip(11*4);if(version>=11)skip(4);if(version>=12)skip(4*4);
        if(version>=13){if(i==count-1&&field==PacketField::EffectBrush)found=at;skip(4);}
        if(version>=14){if(i==count-1&&field==PacketField::FlowKind)found=at;skip(4);if(i==count-1&&field==PacketField::ConnectorLabel)found=at;skip(4);}
    }
    if(at!=bytes.size()-4||found==SIZE_MAX)throw std::runtime_error("Fixture field unavailable or trailing layout");return found;
}
inline void PatchLastPacketInt(std::vector<BYTE>& bytes,PacketField field,int32_t value){PatchPacketInt(bytes,LastPacketField(bytes,field),value);}
inline void MakeUnsupportedVersion(std::vector<BYTE>& bytes){PatchPacketInt(bytes,8,CurrentProjectVersion+1);}
}
