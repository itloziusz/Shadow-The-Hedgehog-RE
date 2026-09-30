#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using s8 = std::int8_t;
using s16 = std::int16_t;

struct Reader {
    const std::vector<u8>& d;
    std::size_t p = 0;
    bool can(std::size_t n) const { return p + n <= d.size(); }
    u8 u8v() { return d[p++]; }
    u16 be16() { u16 v = (u16(d[p]) << 8) | d[p+1]; p += 2; return v; }
    u32 be32() { u32 v=(u32(d[p])<<24)|(u32(d[p+1])<<16)|(u32(d[p+2])<<8)|d[p+3]; p+=4; return v; }
    float bef32() { u32 v = be32(); return std::bit_cast<float>(v); }
};

static unsigned bits(u32 v, unsigned pos, unsigned width) {
    return (v >> pos) & ((width == 32) ? 0xffffffffu : ((1u << width) - 1u));
}

struct VAT { u32 a=0,b=0,c=0; };
struct CPState {
    u32 vcd_lo=0, vcd_hi=0;
    bool vcd_lo_known=false, vcd_hi_known=false;
    std::array<VAT,8> vat{};
    std::array<std::array<bool,3>,8> vat_known{};
    std::array<u32,16> array_base{};
    std::array<u32,16> array_stride{};
};

enum class VMode : unsigned { None=0, Direct=1, Index8=2, Index16=3 };
static VMode mode(unsigned x) { return static_cast<VMode>(x & 3); }

static std::optional<unsigned> elem_size(unsigned fmt) {
    if (fmt <= 1) return 1;
    if (fmt <= 3) return 2;
    if (fmt == 4) return 4;
    // 5..7 are not valid public GX component formats. Fail closed instead of guessing.
    return std::nullopt;
}
static std::optional<unsigned> color_size(unsigned fmt) {
    static constexpr unsigned s[6] = {2,3,4,2,3,4};
    if (fmt < 6) return s[fmt];
    return std::nullopt;
}
static unsigned index_size(VMode m) { return m==VMode::Index8 ? 1u : m==VMode::Index16 ? 2u : 0u; }

static std::pair<unsigned,unsigned> tex_elem_fmt(const VAT& v, int i) {
    switch (i) {
        case 0: return {bits(v.a,21,1),bits(v.a,22,3)};
        case 1: return {bits(v.b,0,1),bits(v.b,1,3)};
        case 2: return {bits(v.b,9,1),bits(v.b,10,3)};
        case 3: return {bits(v.b,18,1),bits(v.b,19,3)};
        case 4: return {bits(v.b,27,1),bits(v.b,28,3)};
        case 5: return {bits(v.c,5,1),bits(v.c,6,3)};
        case 6: return {bits(v.c,14,1),bits(v.c,15,3)};
        case 7: return {bits(v.c,23,1),bits(v.c,24,3)};
    }
    return {0,0};
}

static std::optional<unsigned> vertex_size(const CPState& cp, unsigned vi) {
    if (!cp.vcd_lo_known || !cp.vcd_hi_known) return std::nullopt;
    const unsigned idx = vi & 7;
    const VAT& v=cp.vat[idx];
    unsigned n = std::popcount(cp.vcd_lo & 0x1ffu); // matrix indices are direct u8
    VMode pos=mode(bits(cp.vcd_lo,9,2));
    if (pos==VMode::Direct) {
        if (!cp.vat_known[idx][0]) return std::nullopt;
        auto es=elem_size(bits(v.a,1,3)); if(!es) return std::nullopt;
        n += (bits(v.a,0,1)?3u:2u)*(*es);
    } else n += index_size(pos);

    VMode normal=mode(bits(cp.vcd_lo,11,2));
    if (normal==VMode::Direct) {
        if (!cp.vat_known[idx][0]) return std::nullopt;
        auto es=elem_size(bits(v.a,10,3)); if(!es) return std::nullopt;
        n += (bits(v.a,9,1)?9u:3u)*(*es);
    } else if (normal!=VMode::None) {
        if (!cp.vat_known[idx][0]) return std::nullopt;
        unsigned k=index_size(normal);
        if (bits(v.a,9,1) && bits(v.a,31,1)) k*=3;
        n += k;
    }

    for (int ci=0;ci<2;ci++) {
        VMode m=mode(bits(cp.vcd_lo,13+2*ci,2));
        if(m==VMode::Direct){ if(!cp.vat_known[idx][0]) return std::nullopt; auto cs=color_size(bits(v.a,14+4*ci,3)); if(!cs)return std::nullopt; n+=*cs; }
        else n+=index_size(m);
    }
    for(int ti=0;ti<8;ti++){
        VMode m=mode(bits(cp.vcd_hi,2*ti,2));
        if(m==VMode::Direct){ unsigned group=(ti==0)?0u:(ti<=4?1u:2u); if(!cp.vat_known[idx][group]) return std::nullopt; auto [e,f]=tex_elem_fmt(v,ti); auto es=elem_size(f); if(!es)return std::nullopt; n+=(e?2u:1u)*(*es); }
        else n+=index_size(m);
    }
    return n;
}

static const char* prim_name(u8 op) {
    switch(op & 0x78) {
        case 0x00: return "GX_QUADS";
        case 0x08: return "GX_QUADS2";
        case 0x10: return "GX_TRIANGLES";
        case 0x18: return "GX_TRIANGLESTRIP";
        case 0x20: return "GX_TRIANGLEFAN";
        case 0x28: return "GX_LINES";
        case 0x30: return "GX_LINESTRIP";
        case 0x38: return "GX_POINTS";
    }
    return "GX_UNKNOWN_PRIMITIVE";
}
static const char* scalar_suffix(unsigned fmt) {
    static constexpr const char* n[8]={"u8","s8","u16","s16","f32","rawf32_5","rawf32_6","rawf32_7"};
    return n[fmt&7];
}

static void emit_scalar_values(Reader& r, unsigned fmt, unsigned count, std::ostream& o) {
    for(unsigned i=0;i<count;i++){
        if(i) o << ", ";
        if(fmt==0) o << unsigned(r.u8v());
        else if(fmt==1) o << int(static_cast<s8>(r.u8v()));
        else if(fmt==2) o << r.be16();
        else if(fmt==3) o << static_cast<s16>(r.be16());
        else { float f=r.bef32(); o << std::setprecision(9) << std::showpoint << f << "f" << std::noshowpoint; }
    }
}
static void emit_index(Reader& r, VMode m, const std::string& name, std::ostream& o) {
    if(m==VMode::Index8) o << name << "1x8(" << unsigned(r.u8v()) << ");\n";
    else if(m==VMode::Index16) o << name << "1x16(" << r.be16() << ");\n";
}

static bool emit_vertex(Reader& r, const CPState& cp, unsigned vi, std::ostream& o) {
    const VAT& v=cp.vat[vi&7];
    static constexpr const char* mtx_attrs[9]={"PNMTXIDX","TEX0MTXIDX","TEX1MTXIDX","TEX2MTXIDX","TEX3MTXIDX","TEX4MTXIDX","TEX5MTXIDX","TEX6MTXIDX","TEX7MTXIDX"};
    for(int i=0;i<9;i++) if(cp.vcd_lo&(1u<<i)){
        if(!r.can(1))return false;
        o<<"GX_MatrixIndex1x8("<<unsigned(r.u8v())<<"); /* "<<mtx_attrs[i]<<" */\n";
    }

    VMode pos=mode(bits(cp.vcd_lo,9,2));
    if(pos==VMode::Direct){ unsigned elems=bits(v.a,0,1)?3:2, fmt=bits(v.a,1,3); auto es=elem_size(fmt); if(!es||!r.can(elems*(*es)))return false; o<<"GX_Position"<<elems<<scalar_suffix(fmt)<<"("; emit_scalar_values(r,fmt,elems,o); o<<");\n"; }
    else if(pos!=VMode::None){ if(!r.can(index_size(pos)))return false; emit_index(r,pos,"GX_Position",o); }

    VMode nm=mode(bits(cp.vcd_lo,11,2));
    if(nm==VMode::Direct){ unsigned vecs=bits(v.a,9,1)?3:1, fmt=bits(v.a,10,3); auto es=elem_size(fmt); if(!es||!r.can(vecs*3*(*es)))return false; if(vecs==1){ o<<"GX_Normal3"<<scalar_suffix(fmt)<<"("; emit_scalar_values(r,fmt,3,o); o<<");\n";} else { o<<"GX_NBT9"<<scalar_suffix(fmt)<<"("; emit_scalar_values(r,fmt,9,o); o<<"); /* lossless pseudo-helper */\n"; } }
    else if(nm!=VMode::None){ unsigned cnt=(bits(v.a,9,1)&&bits(v.a,31,1))?3:1; for(unsigned j=0;j<cnt;j++){ if(!r.can(index_size(nm)))return false; emit_index(r,nm,cnt==1?"GX_Normal":"GX_NBTIndex",o);} }

    for(int ci=0;ci<2;ci++){
        VMode cm=mode(bits(cp.vcd_lo,13+2*ci,2)); std::string prefix="GX_Color";
        if(cm==VMode::Direct){ unsigned cf=bits(v.a,14+4*ci,3); auto cs=color_size(cf); if(!cs||!r.can(*cs))return false;
            if(cf==0||cf==3){u16 x=r.be16();o<<prefix<<"1u16(0x"<<std::hex<<std::setw(4)<<std::setfill('0')<<x<<std::dec<<");\n";}
            else if(cf==1){u8 a=r.u8v(),b=r.u8v(),c=r.u8v();o<<prefix<<"3u8("<<unsigned(a)<<", "<<unsigned(b)<<", "<<unsigned(c)<<");\n";}
            else if(cf==2||cf==5){u32 x=r.be32();o<<prefix<<"1u32(0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<x<<std::dec<<");\n";}
            else {u32 x=(u32(r.u8v())<<16)|(u32(r.u8v())<<8)|r.u8v();o<<prefix<<"Raw24(0x"<<std::hex<<std::setw(6)<<std::setfill('0')<<x<<std::dec<<");\n";}
        } else if(cm!=VMode::None){if(!r.can(index_size(cm)))return false;emit_index(r,cm,prefix,o);}
    }

    for(int ti=0;ti<8;ti++){
        VMode tm=mode(bits(cp.vcd_hi,2*ti,2));
        if(tm==VMode::None)continue;
        std::string prefix = "GX_TexCoord";
        if(tm==VMode::Direct){auto[e,f]=tex_elem_fmt(v,ti);unsigned elems=e?2:1;auto es=elem_size(f);if(!es||!r.can(elems*(*es)))return false;o<<prefix<<elems<<scalar_suffix(f)<<"(";emit_scalar_values(r,f,elems,o);o<<");\n";}
        else {if(!r.can(index_size(tm)))return false;emit_index(r,tm,prefix,o);}
    }
    return true;
}

static void load_cp(CPState& cp, u8 reg, u32 val) {
    switch(reg & 0xf0){
        case 0x50: cp.vcd_lo=val; cp.vcd_lo_known=true; break;
        case 0x60: cp.vcd_hi=val; cp.vcd_hi_known=true; break;
        case 0x70: cp.vat[reg&7].a=val; cp.vat_known[reg&7][0]=true; break;
        case 0x80: cp.vat[reg&7].b=val; cp.vat_known[reg&7][1]=true; break;
        case 0x90: cp.vat[reg&7].c=val; cp.vat_known[reg&7][2]=true; break;
        case 0xa0: cp.array_base[reg&0xf]=val; break;
        case 0xb0: cp.array_stride[reg&0xf]=val; break;
    }
}

static bool decode(const std::vector<u8>& data, std::ostream& o) {
    Reader r{data}; CPState cp; bool ok=true;
    while(r.p<data.size()){
        std::size_t at=r.p; u8 op=r.u8v();
        o << "/* FIFO+0x" << std::hex << at << std::dec << " */ ";
        if(op==0x00){o<<"GX_NOP();\n";continue;}
        if(op==0x08){if(!r.can(5)){o<<"/* truncated CP */\n";return false;}u8 reg=r.u8v();u32 v=r.be32();load_cp(cp,reg,v);o<<"GX_LoadCPReg(0x"<<std::hex<<unsigned(reg)<<", 0x"<<std::setw(8)<<std::setfill('0')<<v<<std::dec<<");\n";continue;}
        if(op==0x61){if(!r.can(4)){o<<"/* truncated BP */\n";return false;}u32 x=r.be32();o<<"GX_LoadBPReg(0x"<<std::hex<<unsigned(x>>24)<<", 0x"<<std::setw(6)<<std::setfill('0')<<(x&0xffffff)<<std::dec<<");\n";continue;}
        if(op==0x10){if(!r.can(4)){o<<"/* truncated XF */\n";return false;}u32 h=r.be32();unsigned count=((h>>16)&0xf)+1;u16 addr=h&0xffff;if(!r.can(count*4)){o<<"/* truncated XF payload */\n";return false;}o<<"GX_LoadXFRegsRaw(0x"<<std::hex<<addr<<std::dec<<", {";for(unsigned i=0;i<count;i++){if(i)o<<", ";o<<"0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<r.be32()<<std::dec;}o<<"});\n";continue;}
        if(op==0x40){if(!r.can(8)){o<<"/* truncated DL */\n";return false;}u32 a=r.be32(),s=r.be32();o<<"GX_CallDisplayListRaw(0x"<<std::hex<<a<<std::dec<<", "<<s<<");\n";continue;}
        if(op==0x48){o<<"GX_InvalidateVtxCache();\n";continue;}
        if(op==0x20||op==0x28||op==0x30||op==0x38){if(!r.can(4)){o<<"/* truncated indexed XF */\n";return false;}u32 x=r.be32();unsigned index=x>>16,addr=x&0xfff,size=((x>>12)&0xf)+1;o<<"GX_LoadIndexedXF("<<unsigned((op-0x20)/8)<<", "<<index<<", 0x"<<std::hex<<addr<<std::dec<<", "<<size<<");\n";continue;}
        if(op>=0x80&&op<=0xbf){if(!r.can(2)){o<<"/* truncated primitive */\n";return false;}u16 count=r.be16();unsigned vi=op&7;auto vs=vertex_size(cp,vi);o<<"GX_Begin("<<prim_name(op)<<", GX_VTXFMT"<<vi<<", "<<count<<");";if(vs)o<<" /* "<<*vs<<" bytes/vertex */";else o<<" /* vertex size unknown */";o<<"\n";if(!vs){return false;}for(unsigned i=0;i<count;i++){o<<"/* vertex "<<i<<" */\n";if(!emit_vertex(r,cp,vi,o)){o<<"/* truncated/invalid vertex */\n";return false;}}o<<"GX_End();\n";continue;}
        o<<"GX_UnknownOpcode(0x"<<std::hex<<unsigned(op)<<std::dec<<"); /* stop: fail-closed */\n";ok=false;break;
    }
    return ok;
}

int main(int argc,char**argv){
    if(argc!=2){std::cerr<<"usage: gx_fifo_decoder <fifo.bin>\n";return 2;}
    std::ifstream f(argv[1],std::ios::binary);if(!f){std::cerr<<"cannot open input\n";return 2;}
    std::vector<u8>d((std::istreambuf_iterator<char>(f)),{});
    return decode(d,std::cout)?0:1;
}
