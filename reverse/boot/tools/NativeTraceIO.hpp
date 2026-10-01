#pragma once
#include "shadow/boot/NativeL2Prefix.hpp"
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <istream>

namespace shadow::boot::trace {
inline std::uint64_t Read(std::istream& input) {
    std::uint64_t word;
    if (!(input>>std::hex>>word)) throw std::runtime_error("incomplete explicit entry state");
    return word;
}
inline std::uint32_t Read32(std::istream& input) {
    const auto word=Read(input);
    if (word>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("32-bit field overflow");
    return static_cast<std::uint32_t>(word);
}
inline NativeL2Inputs ReadL2(std::istream& input) {
    NativeL2Inputs data{};auto& e=data.entry;
    e.msr=Read32(input);e.hid0=Read32(input);e.hid2=Read32(input);
    e.cr=Read32(input);e.xer=Read32(input);e.ctr=Read32(input);e.fpscr=Read32(input);
    for(auto& f:e.fpr)f.ps0=Read(input);
    for(auto& f:e.fpr)f.ps1=Read(input);
    for(auto& q:e.gqr)q=Read32(input);
    e.source_address=Read32(input);
    for(unsigned n=0;n<4;++n) {
        const auto word=Read32(input);
        for(unsigned b=0;b<4;++b)e.source_bytes[4*n+b]=static_cast<std::uint8_t>(word>>(24-8*b));
    }
    data.l2cr=Read32(input);return data;
}
inline void Print(const NativeBootCheckpoint& cp,std::uint32_t l2cr,const L2StackBytes* stack) {
    const auto& s=cp.state;
    std::cout<<std::setw(8)<<s.machine.cpu.pc<<' '<<std::setw(8)<<s.machine.msr
             <<' '<<std::setw(8)<<s.machine.cpu.lr<<' '<<std::setw(8)<<s.cr
             <<' '<<std::setw(8)<<s.xer<<' '<<std::setw(8)<<s.fpscr;
    for(auto r:s.machine.cpu.gpr)std::cout<<' '<<std::setw(8)<<r;
    for(auto f:s.fpr)std::cout<<' '<<std::setw(16)<<f.ps0;
    for(auto f:s.fpr)std::cout<<' '<<std::setw(16)<<f.ps1;
    std::cout<<' '<<std::setw(8)<<cp.ctr<<' '<<std::setw(8)<<cp.hid0<<' '<<std::setw(8)<<cp.hid2;
    for(auto q:cp.gqr)std::cout<<' '<<std::setw(8)<<q;
    std::cout<<' ';for(auto b:cp.paired_stack.bytes)std::cout<<std::setw(2)<<unsigned(b);
    std::cout<<' ';for(auto b:cp.paired_stack.valid)std::cout<<(b?'1':'0');
    std::cout<<' '<<std::setw(8)<<l2cr<<' ';
    if(stack) {
        for(auto b:stack->bytes)std::cout<<std::setw(2)<<unsigned(b);
        std::cout<<' ';for(auto b:stack->valid)std::cout<<(b?'1':'0');
    } else std::cout<<"- -";
}
}
