#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shadow::gc {

// AFS archive. Magic "AFS\0", then a little-endian count and offset/size pairs.
// The table is bounded by the file size. There is no fixed entry cap.
struct AfsEntry {
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
};

struct AfsArchive {
    std::uint32_t count = 0;
    std::vector<AfsEntry> entries;
};

AfsArchive parse_afs(std::span<const std::uint8_t> bytes);

// ADX starts with big-endian 0x8000. The u16 at +2 is an offset that lands on the
// CRI marker in every sampled file. The following bytes are stable across those
// files (encoding 3, block 18, sample bits 4, channels 2, rate 48000) and are
// named from that stable layout. Sample decoding is not implemented.
struct AdxHeader {
    std::uint16_t copyright_offset = 0;
    std::uint8_t encoding = 0;
    std::uint8_t block_size = 0;
    std::uint8_t sample_bit_depth = 0;
    std::uint8_t channel_count = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t total_samples = 0;
    std::uint16_t highpass_frequency = 0;
    std::string copyright;
};

AdxHeader parse_adx(std::span<const std::uint8_t> bytes);

// MPEG program-stream pack header used by the .sfd cutscenes. Demux is unresolved.
struct SofdecFile {
    std::uint64_t size = 0;
};

SofdecFile parse_sofdec(std::span<const std::uint8_t> bytes);

// setid.bin: u32 0, u32 count, then count records of three little-endian u32s.
// 8 + count * 12 equals the file size. The three words are not named further.
struct SetIdRecord {
    std::uint32_t word0 = 0;
    std::uint32_t word1 = 0;
    std::uint32_t word2 = 0;
};

struct SetIdTable {
    std::vector<SetIdRecord> records;
};

SetIdTable parse_setid(std::span<const std::uint8_t> bytes);

// Files whose first big-endian u32 equals the file length: CCL, PTB, BIN, PTP.
// Only that size word is treated as known. The following words are kept raw.
struct SizedBlob {
    std::uint32_t declared_size = 0;
    std::vector<std::uint32_t> header_words;
    std::vector<std::uint8_t> bytes;
};

bool looks_sized_blob(std::span<const std::uint8_t> bytes);
SizedBlob parse_sized_blob(std::span<const std::uint8_t> bytes);

struct EffectFile {
    std::uint32_t version = 0;
    std::string name;
    std::vector<std::uint8_t> bytes;
};

bool looks_effect(std::span<const std::uint8_t> bytes);
EffectFile parse_effect(std::span<const std::uint8_t> bytes);

// METRICS1 text. The body after the signature is kept as text, not a guessed glyph table.
struct MetricsText {
    std::string text;
};

MetricsText parse_metrics(std::span<const std::uint8_t> bytes);

// CPAF container. The fourcc is known; the interior chunk grammar was not traced,
// so the payload is retained whole.
struct CsdContainer {
    std::vector<std::uint8_t> bytes;
};

CsdContainer parse_csd(std::span<const std::uint8_t> bytes);

struct RawAsset {
    std::vector<std::uint8_t> bytes;
};

}  // namespace shadow::gc
