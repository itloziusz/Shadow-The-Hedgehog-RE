#include "contract_tests.h"

#include "boot_image.h"
#include "crt_semantics.h"
#include "golden_trace.h"
#include "hardware_frontier.h"
#include "hardware_semantics.h"
#include "low_memory_semantics.h"
#include "native_boot_manifest.h"
#include "runtime_route.h"

namespace {

std::size_t FixtureOffset(GuestAddress32 address) {
    for (const DolSectionSpec& section : NativeBootManifest::sections) {
        if (address >= section.address &&
            HostByteCount64{address} + 1u <= HostByteCount64{section.address} + section.size) {
            return static_cast<std::size_t>(section.file_offset + (address - section.address));
        }
    }
    throw BootError("contract address is outside the manifest");
}

void ExpectThrow(const char* name, const auto& action) {
    bool threw = false;
    try {
        action();
    } catch (const BootError&) {
        threw = true;
    }
    if (!threw) {
        throw BootError(name);
    }
}

const char* kAppliedOracle = R"({
"revision": "GUPP8P",
"checkpoint": "pre_dol_entry",
"evidence": "DOLPHIN_ORACLE",
"applied_by_native_boot": true,
"pc": "0x80003154",
"memory_0x805F1F30": "0000000000000000",
"memory_0x805F1F38": "0000000000000000"
})";

const char* kZeroOracle = R"({
"revision": "GUPP8P",
"checkpoint": "pre_dol_entry",
"evidence": "DOLPHIN_ORACLE",
"applied_by_native_boot": false,
"pc": "0x80003154",
"memory_0x805F1F30": "0000000000000000",
"memory_0x805F1F38": "0000000000000000"
})";

const char* kOneOracle = R"({
"revision": "GUPP8P",
"checkpoint": "pre_dol_entry",
"evidence": "DOLPHIN_ORACLE",
"applied_by_native_boot": false,
"pc": "0x80003154",
"memory_0x805F1F30": "0000000000000001",
"memory_0x805F1F38": "000000003f800000"
})";

}  // namespace

void RunFixtureContractTests(const std::vector<std::uint8_t>& dol) {
    ExpectThrow("CRT ran before the FPR source was consumed", [&dol] {
        const BootImage image = LoadValidatedFixture(dol);
        HardwareSemantics fake;
        fake.pc = 0x8000315Cu;
        fake.r1 = NativeBootManifest::stack_base;
        (void)InitializeNativeCrt(image, fake);
    });
    ExpectThrow("hardware semantics require the external FPR image", [] {
        (void)InitializeNativeHardwareSemantics(PreEntryOracle{});
    });
    ExpectThrow("tampered hardware helper was accepted", [&dol] {
        auto copy = dol;
        copy.at(FixtureOffset(0x80003400u)) ^= 0x01u;
        const BootImage image = LoadValidatedFixture(copy);
        IsolateHardwareFrontier(image, PreEntryOracle{}, false);
    });
    ExpectThrow("tampered CRT copy entry was accepted", [&dol] {
        auto copy = dol;
        copy.at(FixtureOffset(NativeBootManifest::copy_table.raw)) ^= 0x01u;
        const BootImage image = LoadValidatedFixture(copy);
        IsolateHardwareFrontier(image, PreEntryOracle{}, false);
    });
    ExpectThrow("malformed oracle was accepted", [] { (void)ParsePreEntryOracle("{"); });
    ExpectThrow("truncated oracle was accepted", [] {
        std::string text = kZeroOracle;
        text.pop_back();
        (void)ParsePreEntryOracle(text);
    });
    ExpectThrow("invalid boolean suffix was accepted", [] {
        std::string text = kZeroOracle;
        text.replace(text.find("false"), 5, "falseNOT_JSON");
        (void)ParsePreEntryOracle(text);
    });
    ExpectThrow("nested fields supplied the top-level oracle", [] {
        (void)ParsePreEntryOracle(std::string("{\"nested\":") + kZeroOracle + "}");
    });
    for (const std::string suffix : {",", " garbage", "{}"}) {
        ExpectThrow("trailing JSON content was accepted", [&suffix] {
            (void)ParsePreEntryOracle(std::string(kZeroOracle) + suffix);
        });
    }
    for (const std::string member : {
             R"("x": [1,])", R"("x": 01)", R"("x": 1.)", R"("x": 1e+)",
             R"("x": "\q")", R"("x": "\uD800")", R"("x": "\uDC00")",
             R"("revi\u0073ion": "GUPP8P")", R"("revision": "GUPP8P")",
             R"("lowmem_0x80000030": "0x00000000")"}) {
        ExpectThrow("malformed or partial oracle member was accepted", [&member] {
            std::string text = kZeroOracle;
            text.insert(text.rfind('}'), "," + member);
            (void)ParsePreEntryOracle(text);
        });
    }
    {
        std::string text = kZeroOracle;
        text.insert(text.rfind('}'), R"(,"metadata":{"pc":"unrelated","revision":"metadata"},
            "values":[null,true,false,-12.5e+2,{"escaped":"a\\b\n\uD83D\uDE00"}])");
        if (ParsePreEntryOracle(text).pc != "0x80003154") {
            throw BootError("valid metadata changed the top-level oracle");
        }
    }
    ExpectThrow("excessively nested oracle was accepted", [] {
        std::string text = kZeroOracle;
        text.insert(text.rfind('}'), ",\"nested\":" + std::string(65, '[') + "0" + std::string(65, ']'));
        (void)ParsePreEntryOracle(text);
    });
    for (const std::string invalid : {std::string(1, '\x01'), std::string("\xC0\xAF"),
                                      std::string("\xED\xA0\x80"), std::string("\xF4\x90\x80\x80")}) {
        ExpectThrow("invalid string encoding was accepted", [&invalid] {
            std::string text = kZeroOracle;
            text.insert(text.rfind('}'), ",\"metadata\":\"" + invalid + "\"");
            (void)ParsePreEntryOracle(text);
        });
    }
    ExpectThrow("oracle claims the native boot already applied external state", [] {
        (void)ParsePreEntryOracle(kAppliedOracle);
    });
    {
        const BootImage image = LoadValidatedFixture(dol);
        bool threw = false;
        try {
            (void)image.ReadWord(NativeBootManifest::fpr_source);
        } catch (const BootError&) {
            threw = true;
        }
        if (!threw) {
            throw BootError("unmapped FPR source was read as zero");
        }
    }
    {
        const HardwareSemantics zeros =
            InitializeNativeHardwareSemantics(ParsePreEntryOracle(kZeroOracle));
        if (zeros.fpr_binary64 != 0u || zeros.fpscr != 0u || !zeros.paired_temporary_survives ||
            zeros.pc != 0x8000315Cu) {
            throw BootError("zero FPR image did not become the +0 semantic state");
        }
        const HardwareSemantics one =
            InitializeNativeHardwareSemantics(ParsePreEntryOracle(kOneOracle));
        if (one.fpr_binary64 != 1u || one.fpscr != 1u || !one.paired_temporary_survives ||
            one.paired_lane1_binary32 != 0x3F800000u) {
            throw BootError("FPR semantic image was not derived from the oracle bytes");
        }
    }
    const char* retail = R"({
"revision": "GUPP8P",
"checkpoint": "pre_dol_entry",
"evidence": "DOLPHIN_ORACLE",
"applied_by_native_boot": false,
"pc": "0x80003154",
"memory_0x805F1F30": "0000000000000000",
"memory_0x805F1F38": "0000000000000000",
"lowmem_0x80000030": "0x00000000",
"lowmem_0x80000034": "0x817E74E0",
"lowmem_0x80000044": "0x00000000",
"lowmem_0x800000F4": "0x817E54E0",
"lowmem_0x800030E4": "0x00000000",
"lowmem_0x800030E6": "0x00000000",
"bi2_bytes": "00000000018000000000000000000000000000000000000000000002000000010000000100000000000000000000000000000000000000000000000000000000"
})";
    {
        const PreEntryOracle parsed = ParsePreEntryOracle(kZeroOracle);
        if (parsed.has_lowmem) {
            throw BootError("FPR-only oracle was treated as a low-memory capture");
        }
        const BootImage image = LoadValidatedFixture(dol);
        CrtSemantics crt;
        crt.pc = 0x80003170u;
        const LowMemorySemantics low = ResolveLowMemoryPrelude(image, crt, ParsePreEntryOracle(retail));
        const RuntimeRoute route = ResolveRuntimeRoute(image, low);
        // Arena low is the SECOND OSSetArenaLo (0x80370FC8): arenaLo word 0 and
        // BI2 debug flag 0 < 2 select ALIGN32(0x8060C5F0) = 0x8060C600. The
        // earlier expectation 0x8060E600 encoded only the first store and was
        // wrong (reference reads 0x8060C600 at 0x8037221C; ledger 2026-09-28).
        if (route.guest_ram_allocated || route.constructors_executed || route.os_body_executed ||
            route.constructor_count != 282 || route.first_constructor != 0x803A2520u ||
            route.arena_low != 0x8060C600u || route.arena_high != 0x817E74E0u ||
            route.pc != 0x8000329Cu) {
            throw BootError("retail route did not match the regenerated constructor boundary");
        }
    }
    for (const GuestAddress32 pc : {0x80370FBCu, 0x80370FB0u, 0x80370FA8u, 0x80370F14u}) {
        ExpectThrow("tampered second arena-low selection was accepted", [&dol, retail, pc] {
            auto copy = dol;
            copy.at(FixtureOffset(pc + 3u)) ^= 0x04u;
            const BootImage image = LoadValidatedFixture(copy);
            CrtSemantics crt;
            crt.pc = 0x80003170u;
            const LowMemorySemantics low = ResolveLowMemoryPrelude(image, crt, ParsePreEntryOracle(retail));
            (void)ResolveRuntimeRoute(image, low);
        });
    }
    ExpectThrow("tampered low-memory prelude was accepted", [&dol, retail] {
        auto copy = dol;
        copy.at(FixtureOffset(0x8000317Cu)) ^= 0x01u;
        const BootImage image = LoadValidatedFixture(copy);
        CrtSemantics crt;
        crt.pc = 0x80003170u;
        (void)ResolveLowMemoryPrelude(image, crt, ParsePreEntryOracle(retail));
    });
    ExpectThrow("non-retail BI2 debug flag was accepted", [&dol, retail] {
        std::string text = retail;
        const std::string needle = "0000000000000000000000000000000200000001";
        const auto at = text.find(needle);
        if (at == std::string::npos) {
            throw BootError("retail fixture string lost its BI2 image");
        }
        text.replace(at, 8, "00000001");
        const BootImage image = LoadValidatedFixture(dol);
        CrtSemantics crt;
        crt.pc = 0x80003170u;
        (void)ResolveLowMemoryPrelude(image, crt, ParsePreEntryOracle(text));
    });
}
