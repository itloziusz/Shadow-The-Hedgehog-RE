#include "semantic_helpers.h"

std::array<GuestWord32,2> InitializeWordPair(const BootImage& image,
    GuestAddress32 helper,GuestAddress32 object,std::array<GuestWord32,2> inputs,
    GuestAddress32 return_pc,std::vector<Constructor1Event>& events) {
    image.RequirePalFixtureDigest();
    if((helper!=0x80045FB8u && helper!=0x80045FACu) || object%4!=0 || object>0xFFFFFFF7u)
        throw BootError("unvalidated word-pair helper/extent");
    // Guard the complete helper contract (base/source roles, width, offsets and
    // return). Values remain caller parameters; no expected output is installed.
    for(unsigned i=0;i<2;++i) {
        const auto pc=helper+4*i,word=image.ReadWord(pc);
        if(word>>26!=36 || ((word>>21)&31u)!=4+i || ((word>>16)&31u)!=3 ||
            (word&65535u)!=4*i)throw BootError("word-pair semantic contract changed");
    }
    if(image.ReadWord(helper+8)!=0x4E800020u)throw BootError("word-pair return changed");
    for(unsigned i=0;i<2;++i)
        events.push_back({"write",helper+4*i,object+4*i,4,Constructor1Hex(inputs[i],4)});
    events.push_back({"return",helper+8,return_pc,0,Constructor1Hex(0,4)});
    return inputs;
}
