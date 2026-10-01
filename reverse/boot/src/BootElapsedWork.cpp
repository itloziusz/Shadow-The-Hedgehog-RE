#include "shadow/boot/BootElapsedWork.hpp"

#include <array>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace shadow::boot {
namespace {
// These are semantic unit annotations, not an opcode dispatch table. Each work
// constant was independently decoded from the pinned private PPCTables source.
// Most operations cost1; mtspr (including mtlr/mtctr) costs2, sync/mtfsf cost3.
// No physical latency claim follows. Exact words retain the same 65 unit hashes
// as agent_native_work_42.py; original paths and loops come from NativeBi2.
struct UnitDefinition {
    const char* name;
    const char* raw_sha256;
    std::uint32_t source_work;
    std::vector<BootWorkWordGate> words;
};

const std::array<UnitDefinition, 65> kUnits{{
    {"entry_register_call", "59dfaa7682e58022a901952c50794939c38c61df47f59e8873b22d386bd42bc6", 1u, {
        {0x80003154u, 0x4800015Du}
    }},
    {"register_seed", "080cce80236b961d2416b90e72e86f1fd7382441219b83c1ad8f39e2d2c14ff8", 36u, {
        {0x800032B0u, 0x38000000u},
        {0x800032B4u, 0x38600000u},
        {0x800032B8u, 0x38800000u},
        {0x800032BCu, 0x38A00000u},
        {0x800032C0u, 0x38C00000u},
        {0x800032C4u, 0x38E00000u},
        {0x800032C8u, 0x39000000u},
        {0x800032CCu, 0x39200000u},
        {0x800032D0u, 0x39400000u},
        {0x800032D4u, 0x39600000u},
        {0x800032D8u, 0x39800000u},
        {0x800032DCu, 0x39C00000u},
        {0x800032E0u, 0x39E00000u},
        {0x800032E4u, 0x3A000000u},
        {0x800032E8u, 0x3A200000u},
        {0x800032ECu, 0x3A400000u},
        {0x800032F0u, 0x3A600000u},
        {0x800032F4u, 0x3A800000u},
        {0x800032F8u, 0x3AA00000u},
        {0x800032FCu, 0x3AC00000u},
        {0x80003300u, 0x3AE00000u},
        {0x80003304u, 0x3B000000u},
        {0x80003308u, 0x3B200000u},
        {0x8000330Cu, 0x3B400000u},
        {0x80003310u, 0x3B600000u},
        {0x80003314u, 0x3B800000u},
        {0x80003318u, 0x3BA00000u},
        {0x8000331Cu, 0x3BC00000u},
        {0x80003320u, 0x3BE00000u},
        {0x80003324u, 0x3C208060u},
        {0x80003328u, 0x6021C5F0u},
        {0x8000332Cu, 0x3C40805Fu},
        {0x80003330u, 0x6042A780u},
        {0x80003334u, 0x3DA0805Eu},
        {0x80003338u, 0x61ADC500u},
        {0x8000333Cu, 0x4E800020u}
    }},
    {"hardware_call", "64c0f326c78bf1ba1a7adbd8c1706e6b5ddf3d5d08adc25b63d2c92bb6eec320", 1u, {
        {0x80003158u, 0x480002A9u}
    }},
    {"hardware_prepare", "1336630bee4fd6296eb9719c77f41348327c6e198f215cd21367ad97a5521d56", 5u, {
        {0x80003400u, 0x7C0000A6u},
        {0x80003404u, 0x60002000u},
        {0x80003408u, 0x7C000124u},
        {0x8000340Cu, 0x7FE802A6u},
        {0x80003410u, 0x4836E305u}
    }},
    {"paired_frame", "64b31467eaf48c4b1e1eb76b249a43e9fd7c53dee0c656c2efd412e445c62bd6", 4u, {
        {0x80371714u, 0x7C0802A6u},
        {0x80371718u, 0x90010004u},
        {0x8037171Cu, 0x9421FFF8u},
        {0x80371720u, 0x4BFFF489u}
    }},
    {"hid2_read", "538276da3dbf3e36ab5ac5901064516ccb758d4c90802e435ff30338e8064ec8", 2u, {
        {0x80370BA8u, 0x7C78E2A6u},
        {0x80370BACu, 0x4E800020u}
    }},
    {"paired_enable_call", "96bf3c5c766e90ca7dc3a030dffb8c216b4c1aa3950f13872734eda25a5fced8", 2u, {
        {0x80371724u, 0x6463A000u},
        {0x80371728u, 0x4BFFF489u}
    }},
    {"hid2_write", "07ef0919a3cc3b9ed78d07471a6dc1c9ff557281af98b0fcd72ceea9d3bc3a7d", 3u, {
        {0x80370BB0u, 0x7C78E3A6u},
        {0x80370BB4u, 0x4E800020u}
    }},
    {"icfi_call", "fa36a1b6ecaff17818cf74295f13f4b08ad2af459b8185bd8b4217d9d76d3ec3", 1u, {
        {0x8037172Cu, 0x48000EC9u}
    }},
    {"icfi_leaf", "726ac395cef5a7b5fac2d58256f031669dba8cccca226959c59d93a4f45c7a78", 5u, {
        {0x803725F4u, 0x7C70FAA6u},
        {0x803725F8u, 0x60630800u},
        {0x803725FCu, 0x7C70FBA6u},
        {0x80372600u, 0x4E800020u}
    }},
    {"sync_gqr_return", "d5ff6a2d2741219f332d55f85e121f37208fc89e9e790995d85d5a9421dabbdc", 25u, {
        {0x80371730u, 0x7C0004ACu},
        {0x80371734u, 0x38600000u},
        {0x80371738u, 0x7C70E3A6u},
        {0x8037173Cu, 0x7C71E3A6u},
        {0x80371740u, 0x7C72E3A6u},
        {0x80371744u, 0x7C73E3A6u},
        {0x80371748u, 0x7C74E3A6u},
        {0x8037174Cu, 0x7C75E3A6u},
        {0x80371750u, 0x7C76E3A6u},
        {0x80371754u, 0x7C77E3A6u},
        {0x80371758u, 0x8001000Cu},
        {0x8037175Cu, 0x38210008u},
        {0x80371760u, 0x7C0803A6u},
        {0x80371764u, 0x4E800020u}
    }},
    {"fpr_call", "c7f914389f3ca0d408cda9e64769ed256d22ea0152fc2cff8203ef1673402879", 1u, {
        {0x80003414u, 0x4836D8C9u}
    }},
    {"fpr_seed", "a87830dc9f95f94f3803106aab7b2b9e39a3662ba84dc05444981b1765e8bf62", 76u, {
        {0x80370CDCu, 0x7C6000A6u},
        {0x80370CE0u, 0x60632000u},
        {0x80370CE4u, 0x7C600124u},
        {0x80370CE8u, 0x7C78E2A6u},
        {0x80370CECu, 0x54631FFFu},
        {0x80370CF0u, 0x4182008Cu},
        {0x80370CF4u, 0x3C60805Fu},
        {0x80370CF8u, 0x38631F38u},
        {0x80370CFCu, 0xE0030000u},
        {0x80370D00u, 0x10200090u},
        {0x80370D04u, 0x10400090u},
        {0x80370D08u, 0x10600090u},
        {0x80370D0Cu, 0x10800090u},
        {0x80370D10u, 0x10A00090u},
        {0x80370D14u, 0x10C00090u},
        {0x80370D18u, 0x10E00090u},
        {0x80370D1Cu, 0x11000090u},
        {0x80370D20u, 0x11200090u},
        {0x80370D24u, 0x11400090u},
        {0x80370D28u, 0x11600090u},
        {0x80370D2Cu, 0x11800090u},
        {0x80370D30u, 0x11A00090u},
        {0x80370D34u, 0x11C00090u},
        {0x80370D38u, 0x11E00090u},
        {0x80370D3Cu, 0x12000090u},
        {0x80370D40u, 0x12200090u},
        {0x80370D44u, 0x12400090u},
        {0x80370D48u, 0x12600090u},
        {0x80370D4Cu, 0x12800090u},
        {0x80370D50u, 0x12A00090u},
        {0x80370D54u, 0x12C00090u},
        {0x80370D58u, 0x12E00090u},
        {0x80370D5Cu, 0x13000090u},
        {0x80370D60u, 0x13200090u},
        {0x80370D64u, 0x13400090u},
        {0x80370D68u, 0x13600090u},
        {0x80370D6Cu, 0x13800090u},
        {0x80370D70u, 0x13A00090u},
        {0x80370D74u, 0x13C00090u},
        {0x80370D78u, 0x13E00090u},
        {0x80370D7Cu, 0xC80D5A30u},
        {0x80370D80u, 0xFC200090u},
        {0x80370D84u, 0xFC400090u},
        {0x80370D88u, 0xFC600090u},
        {0x80370D8Cu, 0xFC800090u},
        {0x80370D90u, 0xFCA00090u},
        {0x80370D94u, 0xFCC00090u},
        {0x80370D98u, 0xFCE00090u},
        {0x80370D9Cu, 0xFD000090u},
        {0x80370DA0u, 0xFD200090u},
        {0x80370DA4u, 0xFD400090u},
        {0x80370DA8u, 0xFD600090u},
        {0x80370DACu, 0xFD800090u},
        {0x80370DB0u, 0xFDA00090u},
        {0x80370DB4u, 0xFDC00090u},
        {0x80370DB8u, 0xFDE00090u},
        {0x80370DBCu, 0xFE000090u},
        {0x80370DC0u, 0xFE200090u},
        {0x80370DC4u, 0xFE400090u},
        {0x80370DC8u, 0xFE600090u},
        {0x80370DCCu, 0xFE800090u},
        {0x80370DD0u, 0xFEA00090u},
        {0x80370DD4u, 0xFEC00090u},
        {0x80370DD8u, 0xFEE00090u},
        {0x80370DDCu, 0xFF000090u},
        {0x80370DE0u, 0xFF200090u},
        {0x80370DE4u, 0xFF400090u},
        {0x80370DE8u, 0xFF600090u},
        {0x80370DECu, 0xFF800090u},
        {0x80370DF0u, 0xFFA00090u},
        {0x80370DF4u, 0xFFC00090u},
        {0x80370DF8u, 0xFFE00090u},
        {0x80370DFCu, 0xFDFE058Eu},
        {0x80370E00u, 0x4E800020u}
    }},
    {"cache_call", "49757a558411e9d0e1998043075268a5973aae76b7c242fceb8e0eb88e393e25", 1u, {
        {0x80003418u, 0x4836F421u}
    }},
    {"cache_first_check", "dc8f305c21f5617f12f4a682bd1ba0c96c19620e46fa50a16c6c22c46c05319d", 11u, {
        {0x80372838u, 0x7C0802A6u},
        {0x8037283Cu, 0x90010004u},
        {0x80372840u, 0x9421FFF0u},
        {0x80372844u, 0x93E1000Cu},
        {0x80372848u, 0x93C10008u},
        {0x8037284Cu, 0x3C608056u},
        {0x80372850u, 0x3BE31380u},
        {0x80372854u, 0x4BFFE299u},
        {0x80372858u, 0x54600420u},
        {0x8037285Cu, 0x28000000u},
        {0x80372860u, 0x40820014u}
    }},
    {"hid0_read", "89c28f50635cd43ea9809356818d885e02d3eea737bd84a96d8440f7db1cf3b2", 2u, {
        {0x80370AECu, 0x7C70FAA6u},
        {0x80370AF0u, 0x4E800020u}
    }},
    {"cache_second_check", "a26938bf019c55b06cad90b87582b08c7ada023e62476fc33c0a076ea2f3fe38", 4u, {
        {0x80372874u, 0x4BFFE279u},
        {0x80372878u, 0x54600462u},
        {0x8037287Cu, 0x28000000u},
        {0x80372880u, 0x40820014u}
    }},
    {"l2_check", "758aebf9853b02d22d7bf8b8e3b467e556255f539188f73d419f4d49109c7da8", 4u, {
        {0x80372894u, 0x4BFFE269u},
        {0x80372898u, 0x54600000u},
        {0x8037289Cu, 0x28000000u},
        {0x803728A0u, 0x40820058u}
    }},
    {"l2_read", "80df99292ca9f1efd069f2ceba44d7e61b5250969e22def37d2735866b3b24dc", 2u, {
        {0x80370AFCu, 0x7C79FAA6u},
        {0x80370B00u, 0x4E800020u}
    }},
    {"l2_write", "bbf276c614e8c25a8cbe1a74562d8762d61a2e441633ad7439477c08e8f520bb", 3u, {
        {0x80370B04u, 0x7C79FBA6u},
        {0x80370B08u, 0x4E800020u}
    }},
    {"msr_read", "127fd69db6263e3c7870f5e43dd22bc5fdcc3f873eca4b7982470662c10792f3", 2u, {
        {0x80370ADCu, 0x7C6000A6u},
        {0x80370AE0u, 0x4E800020u}
    }},
    {"msr_write", "9cb567e86575ed12fb1dce90a43229a5f91396b2afe0c1d2fe636692bde3262e", 2u, {
        {0x80370AE4u, 0x7C600124u},
        {0x80370AE8u, 0x4E800020u}
    }},
    {"l2_disabled_caller", "4c5497352add1ef13ad3a8c8e0d1aae00ba36bea692bd97cadd97b835db16cc7", 29u, {
        {0x803728A4u, 0x4BFFE239u},
        {0x803728A8u, 0x7C7E1B78u},
        {0x803728ACu, 0x7C0004ACu},
        {0x803728B0u, 0x38600030u},
        {0x803728B4u, 0x4BFFE231u},
        {0x803728B8u, 0x7C0004ACu},
        {0x803728BCu, 0x7C0004ACu},
        {0x803728C0u, 0x4BFFE23Du},
        {0x803728C4u, 0x5463007Eu},
        {0x803728C8u, 0x4BFFE23Du},
        {0x803728CCu, 0x7C0004ACu},
        {0x803728D0u, 0x4BFFFD71u},
        {0x803728D4u, 0x7FC3F378u},
        {0x803728D8u, 0x4BFFE20Du},
        {0x803728DCu, 0x4BFFE221u},
        {0x803728E0u, 0x64608000u},
        {0x803728E4u, 0x540302D2u},
        {0x803728E8u, 0x4BFFE21Du},
        {0x803728ECu, 0x387F01E4u},
        {0x803728F0u, 0x4CC63182u},
        {0x803728F4u, 0x4BFFE399u}
    }},
    {"l2_invalidate_body", "3dd29d7356631e1b4f4a50a30a399685c7098f3153462c7c9eca4bb649ab639c", 40u, {
        {0x80372640u, 0x7C0802A6u},
        {0x80372644u, 0x90010004u},
        {0x80372648u, 0x9421FFF0u},
        {0x8037264Cu, 0x93E1000Cu},
        {0x80372650u, 0x7C0004ACu},
        {0x80372654u, 0x4BFFE4A9u},
        {0x80372658u, 0x5463007Eu},
        {0x8037265Cu, 0x4BFFE4A9u},
        {0x80372660u, 0x7C0004ACu},
        {0x80372664u, 0x4BFFE499u},
        {0x80372668u, 0x64630020u},
        {0x8037266Cu, 0x4BFFE499u},
        {0x80372670u, 0x48000004u},
        {0x80372674u, 0x48000004u},
        {0x80372678u, 0x4BFFE485u},
        {0x8037267Cu, 0x546007FEu},
        {0x80372680u, 0x28000000u},
        {0x80372684u, 0x4082FFF4u},
        {0x80372688u, 0x4BFFE475u},
        {0x8037268Cu, 0x546302D2u},
        {0x80372690u, 0x4BFFE475u},
        {0x80372694u, 0x48000004u},
        {0x80372698u, 0x3C608056u},
        {0x8037269Cu, 0x3BE31380u},
        {0x803726A0u, 0x48000004u},
        {0x803726A4u, 0x48000010u},
        {0x803726B4u, 0x4BFFE449u},
        {0x803726B8u, 0x546007FEu},
        {0x803726BCu, 0x28000000u},
        {0x803726C0u, 0x4082FFE8u},
        {0x803726C4u, 0x80010014u},
        {0x803726C8u, 0x83E1000Cu},
        {0x803726CCu, 0x38210010u},
        {0x803726D0u, 0x7C0803A6u},
        {0x803726D4u, 0x4E800020u}
    }},
    {"logger", "7b60fd3dacba227c48ddee506b81ac01c1ff72937779634177e80df526705606", 12u, {
        {0x80370C8Cu, 0x9421FF90u},
        {0x80370C90u, 0x40860024u},
        {0x80370CB4u, 0x90610008u},
        {0x80370CB8u, 0x9081000Cu},
        {0x80370CBCu, 0x90A10010u},
        {0x80370CC0u, 0x90C10014u},
        {0x80370CC4u, 0x90E10018u},
        {0x80370CC8u, 0x9101001Cu},
        {0x80370CCCu, 0x91210020u},
        {0x80370CD0u, 0x91410024u},
        {0x80370CD4u, 0x38210070u},
        {0x80370CD8u, 0x4E800020u}
    }},
    {"handler_arguments", "f21b177befefed67e1dc1850ef12b44fee5b83bf4188494f320685a1e13ed2eb", 3u, {
        {0x803728F8u, 0x3C608037u},
        {0x803728FCu, 0x388326D8u},
        {0x80372900u, 0x38600001u}
    }},
    {"handler_call", "328675fd6070c2368aedc74bf6523bd6de1f680103a5cc2e36f0e7b1b4ac88f7", 1u, {
        {0x80372904u, 0x48000A75u}
    }},
    {"handler_frame", "17311e91da4030c001b1fc1c93427219dd3a4d1644db9385911d74b6d728fe62", 10u, {
        {0x80373378u, 0x7C0802A6u},
        {0x8037337Cu, 0x90010004u},
        {0x80373380u, 0x9421FFD0u},
        {0x80373384u, 0x93E1002Cu},
        {0x80373388u, 0x93C10028u},
        {0x8037338Cu, 0x93A10024u},
        {0x80373390u, 0x3BA30000u},
        {0x80373394u, 0x93810020u},
        {0x80373398u, 0x3B840000u},
        {0x8037339Cu, 0x48002D81u}
    }},
    {"disable_ee", "560fee0651ade315c86f3ad757f0201c19230e39d14d5d1a15f5e9c19704c23d", 5u, {
        {0x8037611Cu, 0x7C6000A6u},
        {0x80376120u, 0x5464045Eu},
        {0x80376124u, 0x7C800124u},
        {0x80376128u, 0x54638FFEu},
        {0x8037612Cu, 0x4E800020u}
    }},
    {"handler_select", "0d1bfdf9fac76c8df9856088f9ef088f106f629aa9306cd60bd93150edd096ec", 10u, {
        {0x803733A0u, 0x3C808058u},
        {0x803733A4u, 0x57A513BAu},
        {0x803733A8u, 0x38046CB0u},
        {0x803733ACu, 0x57A6043Eu},
        {0x803733B0u, 0x7C802A14u},
        {0x803733B4u, 0x83C40000u},
        {0x803733B8u, 0x28060010u},
        {0x803733BCu, 0x7C7D1B78u},
        {0x803733C0u, 0x93840000u},
        {0x803733C4u, 0x408201A0u}
    }},
    {"handler_restore_call", "af9850af65d6982e6f4b068c9f43f20483eccd852543ebb45b1e1045f4a44187", 2u, {
        {0x80373564u, 0x7FA3EB78u},
        {0x80373568u, 0x48002BDDu}
    }},
    {"restore_ee_zero", "c22938c14f71361babb6ad11cb376efe3485e8c0bb823b50516c96b12d8e691d", 7u, {
        {0x80376144u, 0x2C030000u},
        {0x80376148u, 0x7C8000A6u},
        {0x8037614Cu, 0x4182000Cu},
        {0x80376158u, 0x5485045Eu},
        {0x8037615Cu, 0x7CA00124u},
        {0x80376160u, 0x54838FFEu},
        {0x80376164u, 0x4E800020u}
    }},
    {"handler_return", "f8b3dd8305a6444eec475bc913f1cb51e15f26a43787f875b907c8af2b11ed64", 10u, {
        {0x8037356Cu, 0x7FC3F378u},
        {0x80373570u, 0x80010034u},
        {0x80373574u, 0x83E1002Cu},
        {0x80373578u, 0x83C10028u},
        {0x8037357Cu, 0x83A10024u},
        {0x80373580u, 0x83810020u},
        {0x80373584u, 0x38210030u},
        {0x80373588u, 0x7C0803A6u},
        {0x8037358Cu, 0x4E800020u}
    }},
    {"handler_logger_call", "fa98abf7c292d0024769575c5c379e1f0d4979d997bd405ba3041527fc92c4ec", 3u, {
        {0x80372908u, 0x387F01FCu},
        {0x8037290Cu, 0x4CC63182u},
        {0x80372910u, 0x4BFFE37Du}
    }},
    {"cache_return", "c95f09ee3b5723674bc14796da664dd37ecf0be50323f1d4737301860c1cfa80", 7u, {
        {0x80372914u, 0x80010014u},
        {0x80372918u, 0x83E1000Cu},
        {0x8037291Cu, 0x83C10008u},
        {0x80372920u, 0x38210010u},
        {0x80372924u, 0x7C0803A6u},
        {0x80372928u, 0x4E800020u}
    }},
    {"hardware_return", "10477fb8244402bae830912f1380922a78952ba45a97e9bcd2f0a2c25df2005c", 3u, {
        {0x8000341Cu, 0x7FE803A6u},
        {0x80003420u, 0x4E800020u}
    }},
    {"sentinel_walker_call", "125f80a2fe77a4bf82b891fd72a706e92c3b7c866f4d8b0034d7b19b8a824115", 5u, {
        {0x8000315Cu, 0x3800FFFFu},
        {0x80003160u, 0x9421FFF8u},
        {0x80003164u, 0x90010004u},
        {0x80003168u, 0x90010000u},
        {0x8000316Cu, 0x480001D5u}
    }},
    {"descriptor_frame", "27c71cbac1b66b3fab8dcef2dccad1a94eb0ac8b0c760d91d7964c334aa610a2", 11u, {
        {0x80003340u, 0x7C0802A6u},
        {0x80003344u, 0x90010004u},
        {0x80003348u, 0x9421FFE8u},
        {0x8000334Cu, 0x93E10014u},
        {0x80003350u, 0x93C10010u},
        {0x80003354u, 0x93A1000Cu},
        {0x80003358u, 0x3C608000u},
        {0x8000335Cu, 0x38035544u},
        {0x80003360u, 0x7C1D0378u},
        {0x80003364u, 0x48000004u},
        {0x80003368u, 0x48000004u}
    }},
    {"identity_copy_descriptor", "14d92ac9022d01a9acb83b14e9d8786e2543b103b23b613ae5ecd4e9c330616a", 10u, {
        {0x8000336Cu, 0x83DD0008u},
        {0x80003370u, 0x281E0000u},
        {0x80003374u, 0x41820038u},
        {0x80003378u, 0x809D0000u},
        {0x8000337Cu, 0x83FD0004u},
        {0x80003380u, 0x41820024u},
        {0x80003384u, 0x7C1F2040u},
        {0x80003388u, 0x4182001Cu},
        {0x800033A4u, 0x3BBD000Cu},
        {0x800033A8u, 0x4BFFFFC4u}
    }},
    {"copy_terminator", "adcfd477eae5470c0a24d8e71ea9f849f21ea91e6d67fce68a6509a34ad55298", 3u, {
        {0x8000336Cu, 0x83DD0008u},
        {0x80003370u, 0x281E0000u},
        {0x80003374u, 0x41820038u}
    }},
    {"zero_table_prepare", "7c62adbe869405aa26b93bd7dfa7f934f335c737423af35e3492008e1c115c62", 5u, {
        {0x800033ACu, 0x3C608000u},
        {0x800033B0u, 0x380355C8u},
        {0x800033B4u, 0x7C1D0378u},
        {0x800033B8u, 0x48000004u},
        {0x800033BCu, 0x48000004u}
    }},
    {"zero_descriptor", "5431bd03b356603768b9465c5ac0c8a2237d9fd9993824d0ba97bcbed33ca59e", 9u, {
        {0x800033C0u, 0x80BD0004u},
        {0x800033C4u, 0x28050000u},
        {0x800033C8u, 0x4182001Cu},
        {0x800033CCu, 0x807D0000u},
        {0x800033D0u, 0x4182000Cu},
        {0x800033D4u, 0x38800000u},
        {0x800033D8u, 0x48002035u},
        {0x800033DCu, 0x3BBD0008u},
        {0x800033E0u, 0x4BFFFFE0u}
    }},
    {"zero_wrapper", "f2cb1c36598116f182c1a6b4ff64422951fb8794553ca7091e42e5b1c7ab38de", 13u, {
        {0x8000540Cu, 0x9421FFF0u},
        {0x80005410u, 0x7C0802A6u},
        {0x80005414u, 0x90010014u},
        {0x80005418u, 0x93E1000Cu},
        {0x8000541Cu, 0x7C7F1B78u},
        {0x80005420u, 0x4800001Du},
        {0x80005424u, 0x80010014u},
        {0x80005428u, 0x7FE3FB78u},
        {0x8000542Cu, 0x83E1000Cu},
        {0x80005430u, 0x7C0803A6u},
        {0x80005434u, 0x38210010u},
        {0x80005438u, 0x4E800020u}
    }},
    {"zero_leaf_fixed", "388df1804af960d5c58fbe13ec8a989c47abdfcd6ddccc931e10453d20338f59", 19u, {
        {0x8000543Cu, 0x28050020u},
        {0x80005440u, 0x5484063Eu},
        {0x80005444u, 0x38C3FFFFu},
        {0x80005448u, 0x7C872378u},
        {0x8000544Cu, 0x41800090u},
        {0x80005450u, 0x7CC030F8u},
        {0x80005454u, 0x540307BFu},
        {0x80005458u, 0x41820014u},
        {0x8000546Cu, 0x28070000u},
        {0x80005470u, 0x4182001Cu},
        {0x8000548Cu, 0x54A3D97Fu},
        {0x80005490u, 0x3886FFFDu},
        {0x80005494u, 0x4182002Cu},
        {0x800054C0u, 0x54A3F77Fu},
        {0x800054C4u, 0x41820010u},
        {0x800054D4u, 0x38C40003u},
        {0x800054D8u, 0x54A507BEu},
        {0x800054DCu, 0x28050000u},
        {0x800054E0u, 0x4D820020u}
    }},
    {"zero_group", "2740c30348dc595eee7067715a6a61b7361d6d2e26c1da415b896e8ea15dad69", 10u, {
        {0x80005498u, 0x90E40004u},
        {0x8000549Cu, 0x3463FFFFu},
        {0x800054A0u, 0x90E40008u},
        {0x800054A4u, 0x90E4000Cu},
        {0x800054A8u, 0x90E40010u},
        {0x800054ACu, 0x90E40014u},
        {0x800054B0u, 0x90E40018u},
        {0x800054B4u, 0x90E4001Cu},
        {0x800054B8u, 0x94E40020u},
        {0x800054BCu, 0x4082FFDCu}
    }},
    {"zero_remaining_word", "72360f2cbfd03c9f80bf5209f3a12f3e40dcf9e88d8e1f3ce2f3f60c6a5fb3a8", 3u, {
        {0x800054C8u, 0x3463FFFFu},
        {0x800054CCu, 0x94E40004u},
        {0x800054D0u, 0x4082FFF8u}
    }},
    {"zero_terminator", "db67d0dc0415d6a444a6c583c4803ff7a40e5807e192eaf30adfa662c52d70d3", 3u, {
        {0x800033C0u, 0x80BD0004u},
        {0x800033C4u, 0x28050000u},
        {0x800033C8u, 0x4182001Cu}
    }},
    {"descriptor_return", "bcc0bc319c9e8a203ed96f04ecdd240e1ff3cc366597d2e934bc7193cccdc617", 8u, {
        {0x800033E4u, 0x8001001Cu},
        {0x800033E8u, 0x83E10014u},
        {0x800033ECu, 0x83C10010u},
        {0x800033F0u, 0x83A1000Cu},
        {0x800033F4u, 0x38210018u},
        {0x800033F8u, 0x7C0803A6u},
        {0x800033FCu, 0x4E800020u}
    }},
    {"crt_entry_tail", "63de7677bea1f499f4873728ff23758d30e7105b34d41c03b0cb2ab1f9451b2b", 6u, {
        {0x80003170u, 0x38000000u},
        {0x80003174u, 0x3CC08000u},
        {0x80003178u, 0x38C60044u},
        {0x8000317Cu, 0x90060000u},
        {0x80003180u, 0x3CC08000u},
        {0x80003184u, 0x38C600F4u}
    }},
    {"bi2_pointer_nonzero", "c28b8de45f708b70a77c7ac1215baa744beec6d0e03ef262acd71bcc7fab001d", 5u, {
        {0x80003188u, 0x80C60000u},
        {0x8000318Cu, 0x28060000u},
        {0x80003190u, 0x4182000Cu},
        {0x80003194u, 0x80E6000Cu},
        {0x80003198u, 0x48000024u}
    }},
    {"bi2_debug_ordinary", "2d8b0bddb967568412b18f4cbc4635921262fba58c2e5521ec55ad2b05176051", 8u, {
        {0x800031BCu, 0x38A00000u},
        {0x800031C0u, 0x28070002u},
        {0x800031C4u, 0x41820024u},
        {0x800031C8u, 0x28070003u},
        {0x800031CCu, 0x38A00001u},
        {0x800031D0u, 0x41820018u},
        {0x800031D4u, 0x28070004u},
        {0x800031D8u, 0x40820020u}
    }},
    {"bi2_debug4_call_return", "466cbb7c93fc550e9638194dd3420f46de9eb113865be53810aee62c5014dd66", 3u, {
        {0x800031DCu, 0x38A00002u},
        {0x800031E0u, 0x4BFFFF61u},
        {0x800031E4u, 0x48000014u}
    }},
    {"bi2_debug4_leaf", "4371be2d493f25717c1eda105babfa5349c1a4a5df7ea5a0306fd66ec500c987", 3u, {
        {0x80003140u, 0x38000001u},
        {0x80003144u, 0x980D5AF0u},
        {0x80003148u, 0x4E800020u}
    }},
    {"bi2_offset_check", "28793ac10df819cf92ede9e782c644541d4cae3af884f1b070af39f27822cebd", 8u, {
        {0x800031F8u, 0x3CC08000u},
        {0x800031FCu, 0x38C600F4u},
        {0x80003200u, 0x80A60000u},
        {0x80003204u, 0x28050000u},
        {0x80003208u, 0x41A20050u},
        {0x8000320Cu, 0x80C50008u},
        {0x80003210u, 0x28060000u},
        {0x80003214u, 0x41A20044u}
    }},
    {"bi2_no_array", "ed9ee64962d532f8b25ed65e0675ee04a8ea1d113c3790a19492785a1d9444fd", 2u, {
        {0x80003258u, 0x39C00000u},
        {0x8000325Cu, 0x39E00000u}
    }},
    {"bi2_count_check", "54aa64d30d9050c53ac1ed6e3231e6b361440187f086777124e1cb08c05e38b9", 4u, {
        {0x80003218u, 0x7CC53214u},
        {0x8000321Cu, 0x81C60000u},
        {0x80003220u, 0x280E0000u},
        {0x80003224u, 0x41820034u}
    }},
    {"bi2_array_prepare", "9a9638b030ae207fe5851d317f3109c19ccf78c576263c70c63e47671fefb57d", 3u, {
        {0x80003228u, 0x39E60004u},
        {0x8000322Cu, 0x7DC903A6u}
    }},
    {"bi2_relocate_item", "780597d22ee2f18fae86a7df5a35a06b43e72e6564ae19ee023cf6278f878ade", 5u, {
        {0x80003230u, 0x38C60004u},
        {0x80003234u, 0x80E60000u},
        {0x80003238u, 0x7CE72A14u},
        {0x8000323Cu, 0x90E60000u},
        {0x80003240u, 0x4200FFF0u}
    }},
    {"bi2_array_publish", "b270bcd3d8a71a3605b866f1b083385e389c859be6cc5a12e3a8278170853766", 5u, {
        {0x80003244u, 0x3CA08000u},
        {0x80003248u, 0x38A50034u},
        {0x8000324Cu, 0x55E70034u},
        {0x80003250u, 0x90E50000u},
        {0x80003254u, 0x4800000Cu}
    }},
    {"metadata_call", "d4e6b44034b1a26aaceb681c9a0233b11a0a41af65dde84c6ff31f2e3df5bcb4", 1u, {
        {0x80003260u, 0x4836D991u}
    }},
    {"metadata_leaf", "97ae2593a4737c5b60a622917800251428202ac3898e018256e965bdd4cc2357", 10u, {
        {0x80370BF0u, 0x3C808000u},
        {0x80370BF4u, 0x38040040u},
        {0x80370BF8u, 0x3C608037u},
        {0x80370BFCu, 0x900D5A18u},
        {0x80370C00u, 0x38630C60u},
        {0x80370C04u, 0x3C038000u},
        {0x80370C08u, 0x90040048u},
        {0x80370C0Cu, 0x38000001u},
        {0x80370C10u, 0x900D5A1Cu},
        {0x80370C14u, 0x4E800020u}
    }},
    {"os_call", "324f1899c46bde622f203ce8e8d03fd62802d1d59171fcafc1daa0070fdea857", 1u, {
        {0x80003264u, 0x4836DC05u}
    }},
    {"os_first_guard_frame", "a472740be340eb3962b9e6395f7e88906f84bf480769173d29a474d20edf45d1", 16u, {
        {0x80370E68u, 0x7C0802A6u},
        {0x80370E6Cu, 0x90010004u},
        {0x80370E70u, 0x9421FFE8u},
        {0x80370E74u, 0x93E10014u},
        {0x80370E78u, 0x93C10010u},
        {0x80370E7Cu, 0x93A1000Cu},
        {0x80370E80u, 0x800D5A40u},
        {0x80370E84u, 0x3C608058u},
        {0x80370E88u, 0x3BE36C40u},
        {0x80370E8Cu, 0x2C000000u},
        {0x80370E90u, 0x3C608056u},
        {0x80370E94u, 0x3BC310F8u},
        {0x80370E98u, 0x40820494u},
        {0x80370E9Cu, 0x38000001u},
        {0x80370EA0u, 0x900D5A40u},
        {0x80370EA4u, 0x480087A5u}
    }},
    {"clock_frame_call_disable", "ac27ff0f124921e68fe2de4bad55f9f5a544afc3ee03e04ba5303b191cd70f75", 7u, {
        {0x80379648u, 0x7C0802A6u},
        {0x8037964Cu, 0x90010004u},
        {0x80379650u, 0x9421FFE0u},
        {0x80379654u, 0x93E1001Cu},
        {0x80379658u, 0x93C10018u},
        {0x8037965Cu, 0x93A10014u},
        {0x80379660u, 0x4BFFCABDu}
    }},
    {"clock_sample_call", "973cf7041436c83651ab25940be720c40c758a533a6612811367a1eca07b62cb", 2u, {
        {0x80379664u, 0x7C7F1B78u},
        {0x80379668u, 0x4BFFFFC1u}
    }}
}};

constexpr std::array<std::array<std::uint32_t, 3>, 11> kCopies{{
    {{0x80003100u, 0x80003100u, 0x24E8u}},
    {{0x80005600u, 0x80005600u, 0x1F08u}},
    {{0x80007520u, 0x80007520u, 0x1814u}},
    {{0x80008D40u, 0x80008D40u, 0x4A1F08u}},
    {{0x804AAC60u, 0x804AAC60u, 0x46Cu}},
    {{0x804AB0E0u, 0x804AB0E0u, 0xCu}},
    {{0x804AB100u, 0x804AB100u, 0x72418u}},
    {{0x8051D520u, 0x8051D520u, 0x528C8u}},
    {{0x805E4500u, 0x805E4500u, 0xAB20u}},
    {{0x805F2780u, 0x805F2780u, 0x9DB8u}},
    {{0x0u, 0x0u, 0x0u}}
}};
constexpr std::array<std::array<std::uint32_t, 2>, 4> kZeros{{
    {{0x8056FE00u, 0x74700u}},
    {{0x805EF020u, 0x375Cu}},
    {{0x805FC540u, 0xACu}},
    {{0x0u, 0x0u}}
}};

std::uint32_t InputWord(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if ((offset & 3u) || offset + 4u > bytes.size())
        throw std::runtime_error("work input reads unknown BI2 bytes");
    std::uint32_t word = 0;
    for (unsigned byte = 0; byte < 4; ++byte)
        word = (word << 8u) | bytes[offset + byte];
    return word;
}

const UnitDefinition& Unit(std::string_view name) {
    for (const auto& unit : kUnits) if (name == unit.name) return unit;
    throw std::runtime_error("undefined native semantic work unit");
}

}  // namespace

const std::vector<BootWorkWordGate>& BootElapsedWorkRawGatesResearch() {
    static const auto gates = [] {
        std::map<std::uint32_t, std::uint32_t> by_address;
        const auto add = [&](std::uint32_t address, std::uint32_t word) {
            const auto found = by_address.find(address);
            if (found != by_address.end() && found->second != word)
                throw std::runtime_error("inconsistent static semantic unit gate");
            by_address[address] = word;
        };
        for (const auto& unit : kUnits)
            for (const auto& gate : unit.words) add(gate.address, gate.word);
        for (unsigned item = 0; item < kCopies.size(); ++item)
            for (unsigned field = 0; field < 3; ++field)
                add(0x80005544u + 12u * item + 4u * field, kCopies[item][field]);
        for (unsigned item = 0; item < kZeros.size(); ++item)
            for (unsigned field = 0; field < 2; ++field)
                add(0x800055C8u + 8u * item + 4u * field, kZeros[item][field]);
        add(0x80379628u, 0x7C6D42E6u);
        std::vector<BootWorkWordGate> result;
        for (const auto& entry : by_address) result.push_back({entry.first, entry.second});
        return result;
    }();
    return gates;
}

BootElapsedWork ProduceBootElapsedWorkResearch(const BootImage& image, const NativeBi2Inputs& input) {
    for (const auto& gate : BootElapsedWorkRawGatesResearch())
        if (image.ReadWord(gate.address) != gate.word)
            throw std::runtime_error("native semantic work raw word changed");
    BootElapsedWork run;
    // This unchanged native execution proves path conditions, completion,
    // unknown-byte discipline, original stores/aliases and first-clock entry.
    // It contains finite C++ semantic loops, never a guest CPU interpreter.
    run.prefix = RunImmutableNativeBi2Prefix(image, input);
    if (run.prefix.checkpoints.empty() ||
        run.prefix.checkpoints.back().boot.boot.native.state.machine.cpu.pc != 0x80379628u)
        throw std::runtime_error("native semantic work lacks the fully proved first-clock path");
    run.stop_pc = 0x80379628u;
    const auto add = [&](std::string_view name, std::uint64_t repetitions = 1u) {
        const auto& unit = Unit(name);
        const auto operations = repetitions * unit.words.size();
        const auto work = repetitions * unit.source_work;
        run.ledger.push_back({unit.name, unit.raw_sha256, repetitions, operations, work});
        run.original_operations += operations;
        run.source_operation_work += work;
    };
    for (const auto name : {"entry_register_call", "register_seed", "hardware_call", "hardware_prepare",
                           "paired_frame", "hid2_read", "paired_enable_call", "hid2_write", "icfi_call",
                           "icfi_leaf", "sync_gqr_return", "fpr_call", "fpr_seed", "cache_call", "cache_first_check"})
        add(name);
    add("hid0_read"); add("cache_second_check"); add("hid0_read"); add("l2_check"); add("l2_read");
    if (!(input.crt.l2.l2cr & 0x80000000u)) {
        add("l2_disabled_caller"); add("msr_read"); add("msr_write", 2); add("l2_read", 2); add("l2_write", 2);
        add("l2_invalidate_body"); add("l2_read", 5); add("l2_write", 3); add("logger");
    }
    add("handler_arguments");
    for (const auto name : {"handler_call", "handler_frame", "disable_ee", "handler_select", "handler_restore_call",
                           "restore_ee_zero", "handler_return", "handler_logger_call", "logger", "cache_return",
                           "hardware_return", "sentinel_walker_call", "descriptor_frame"})
        add(name);
    // Copies are source==destination in the pinned original descriptors. Their
    // loaded source/destination comparison skips the copy/cache helpers.
    unsigned copies = 0;
    for (unsigned item = 0; item < kCopies.size(); ++item) {
        const auto source = image.ReadWord(0x80005544u + 12u * item);
        const auto destination = image.ReadWord(0x80005548u + 12u * item);
        const auto size = image.ReadWord(0x8000554Cu + 12u * item);
        if (!size) break;
        if (source != destination) throw std::runtime_error("unimplemented CRT copy work path");
        ++copies;
    }
    add("identity_copy_descriptor", copies); add("copy_terminator"); add("zero_table_prepare");
    for (unsigned item = 0; item < kZeros.size(); ++item) {
        const auto address = image.ReadWord(0x800055C8u + 8u * item);
        const auto size = image.ReadWord(0x800055CCu + 8u * item);
        if (!size) break;
        if ((address & 3u) || (size & 3u) || size < 32u)
            throw std::runtime_error("unimplemented zero alignment/short/byte work path");
        add("zero_descriptor"); add("zero_wrapper"); add("zero_leaf_fixed");
        add("zero_group", size >> 5u); add("zero_remaining_word", (size >> 2u) & 7u);
    }
    for (const auto name : {"zero_terminator", "descriptor_return", "crt_entry_tail",
                           "bi2_pointer_nonzero", "bi2_debug_ordinary"}) add(name);
    // Input bytes are immutable here. Native execution may already have
    // relocated an overlapping debug word; the original earlier read is used.
    const auto debug = InputWord(input.loaded_bi2, 12);
    if (debug == 4u) { add("bi2_debug4_call_return"); add("bi2_debug4_leaf"); }
    const auto offset = InputWord(input.loaded_bi2, 8);
    const auto count = offset ? InputWord(input.loaded_bi2, offset) : 0u;
    add("bi2_offset_check");
    if (offset) add("bi2_count_check");
    if (count) {
        add("bi2_array_prepare"); add("bi2_relocate_item", count); add("bi2_array_publish");
    } else add("bi2_no_array");
    for (const auto name : {"metadata_call", "metadata_leaf", "os_call", "os_first_guard_frame",
                           "clock_frame_call_disable", "disable_ee", "clock_sample_call"}) add(name);
    return run;
}

}  // namespace shadow::boot
