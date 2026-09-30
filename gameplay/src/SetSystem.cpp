// Native reconstruction of the SET system. Each function names its PPC origin.
#include "shadow/gameplay/SetSystem.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

namespace shadow::gameplay {

namespace {
uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
float rdf(const uint8_t* p) { uint32_t u = rd32(p); float f; std::memcpy(&f, &u, 4); return f; }

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}
}  // namespace

SetData::SetData() {
    catalog_ = OriginalSetCatalog();
    factories_.resize(catalog_.size());
    Reset();
}

// 0x800CAF38: memset records (0x800 * 0x2C), reset slots, slot[i].rec = &rec[i].
void SetData::Reset() {
    records_.assign(kMaxRecords, SetRecord{});
    miscStore_.clear();
    count_ = 0;
    alive_ = 0;
    for (int i = 0; i < kMaxRecords; ++i) {
        slots_[i] = SetSlot{};
        slots_[i].rec = &records_[i];
    }
    linkHeads_.fill(nullptr);
}

// 0x800CB044: u32 0; u32 count; count x {u32 id; u32 mask[2]} (little-endian on disc)
bool SetData::LoadSetId(const std::string& path) {
    std::vector<uint8_t> b;
    if (!readFile(path, b) || b.size() < 8) return false;
    uint32_t n = rd32(&b[4]);
    if (8 + (size_t)n * 12 > b.size()) return false;
    setId_.resize(n);
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t* e = &b[8 + i * 12];
        setId_[i] = {rd32(e), {rd32(e + 4), rd32(e + 8)}};
    }
    return true;
}

// 0x800CA7F0: bit (31 - stage%32) of mask[stage/32] enables the type in this stage.
void SetData::MarkUsedTypes(int stageIndex) {
    for (auto& d : catalog_) d.usedInStage = false;
    const uint32_t bit = 1u << (31 - (stageIndex % 32));
    const int word = stageIndex / 32;
    for (const auto& e : setId_) {
        if (!(e.mask[word & 1] & bit)) continue;
        for (auto& d : catalog_)
            if (d.id == (uint16_t)e.id) { d.usedInStage = true; break; }
    }
}

// 0x800CB1A4 + 0x800CB0EC
bool SetData::LoadFile(const std::string& path, uint32_t orMask, uint32_t andNotMask) {
    std::vector<uint8_t> b;
    if (!readFile(path, b)) return false;
    if (b.size() < 12 || std::memcmp(b.data(), "sky2", 4) != 0) {
        log.push_back("bad SET magic: " + path);
        return false;
    }
    const uint32_t n = rd32(&b[4]), miscSize = rd32(&b[8]);
    const size_t recEnd = 12 + (size_t)n * 0x2C;
    if (recEnd + miscSize > b.size() || count_ + (int)n > kMaxRecords) {
        log.push_back("SET file truncated/overflow: " + path);
        return false;
    }
    miscStore_.emplace_back(b.begin() + recEnd, b.begin() + recEnd + miscSize);
    const uint8_t* misc = miscStore_.back().data();
    size_t miscOff = 0;
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t* p = &b[12 + i * 0x2C];
        SetRecord& r = records_[count_ + i];
        r.pos = {rdf(p), rdf(p + 4), rdf(p + 8)};
        r.rotDeg = {rdf(p + 12), rdf(p + 16), rdf(p + 20)};
        r.flags = rd32(p + 0x1C);
        r.runtimeFlags = r.flags;                          // rec+0x18 = rec+0x1C
        r.id = rd16(p + 0x20);
        r.link = p[0x22];
        r.range = p[0x23];
        r.miscLen = rd32(p + 0x24);
        r.misc = (miscOff + r.miscLen <= miscSize) ? misc + miscOff : nullptr;
        miscOff += r.miscLen;
        // fn_800CB0EC source-file masks on both flag words
        r.runtimeFlags = (r.runtimeFlags | orMask) & ~andNotMask;
        r.flags = (r.flags | orMask) & ~andNotMask;
    }
    count_ += (int)n;
    return true;
}

// 0x800CBC54
bool SetData::LoadStage(const std::string& root, int stage, bool hardMode, int stageIndex) {
    char buf[64];
    Reset();
    if (!LoadSetId(root + "/setid.bin")) log.push_back("setid.bin missing");
    MarkUsedTypes(stageIndex);
    std::snprintf(buf, sizeof buf, "/stg%04d/stg%04d_cmn.dat", stage, stage);
    bool ok = LoadFile(root + buf);
    if (hardMode) {
        std::snprintf(buf, sizeof buf, "/stg%04d/stg%04d_hrd.dat", stage, stage);
        LoadFile(root + buf, kSetSrcHrd, 0xB000);
    } else {
        std::snprintf(buf, sizeof buf, "/stg%04d/stg%04d_nrm.dat", stage, stage);
        LoadFile(root + buf, kSetSrcNrm, 0xD000);
    }
    std::snprintf(buf, sizeof buf, "/stg%04d/stg%04d_ds1.dat", stage, stage);
    LoadFile(root + buf, kSetSrcDs1, 0x7000);
    // fn_800CA780 (resource hooks) is engine-side; BuildSlots = fn_800CA6B4
    BuildSlots();
    return ok;
}

// 0x800CA6B4: descIndex = index of catalog entry with matching id, else -1.
void SetData::BuildSlots() {
    for (auto& s : slots_) {
        s.descIndex = -1;
        for (size_t i = 0; i < catalog_.size(); ++i)
            if (catalog_[i].id == s.rec->id) { s.descIndex = (int32_t)i; break; }
    }
}

// 0x800CA970: flags 0x80800 bypass; else |pos - player|^2 <= (range^2) * 10000
bool SetData::InRange(const SetSlot& s, const Vec3& pl) {
    const SetRecord& r = *s.rec;
    if (r.runtimeFlags & (kSetAlwaysInRange | kSetNoFadeRange)) return true;
    const float dx = r.pos.x - pl.x, dy = r.pos.y - pl.y, dz = r.pos.z - pl.z;
    const float lim = 10000.0f * (float)(r.range * r.range);
    return dx * dx + dy * dy + dz * dz <= lim;
}

// 0x800CA8D8 (2P version calls it once per player: 0x800CABC8/0x800CABE0)
bool SetData::TrySpawn(SetSlot& s, const Vec3* players, int n) {
    SetRecord& r = *s.rec;
    // rlwinm. bit30 (0x2 alive) and bit14 (0x20000 blocked) at 0x800CA8F8/0x800CA900
    if (r.runtimeFlags & (kSetAlive | kSetBlockCreate)) return false;
    for (int i = 0; i < n; ++i) {
        if (!InRange(s, players[i])) continue;
        r.runtimeFlags &= ~(kSetAlwaysInRange | kSetRearm);   // & 0xFFF7FFBF
        const SetCreateFn& fn = factories_[s.descIndex];
        if (fn) {
            fn(s);
        } else {
            // NATIVE POLICY (not original): no native implementation for this type yet.
            // Fail closed: log it and mark the slot alive so it is not retried every frame.
            log.push_back(std::string("no factory for ") + catalog_[s.descIndex].name);
            r.runtimeFlags |= kSetAlive;   // behave like an attached object so we don't retry every frame
        }
        return true;
    }
    return false;
}

// 0x800CAE24: rebuild intrusive list of enabled slots with this link id.
void SetData::RebuildLinkList(uint8_t link) {
    linkHeads_[link] = nullptr;
    for (auto& s : slots_) {
        if (!(s.rec->runtimeFlags & kSetEnabled) || s.rec->link != link) continue;
        SetSlot*& head = linkHeads_[link];
        if (!head) {
            head = &s;
            s.prev = &s;
            s.next = nullptr;
        } else {
            SetSlot* tail = head->prev;
            tail->next = &s;
            s.prev = tail;
            s.next = nullptr;
            head->prev = &s;
        }
    }
}

void SetData::MarkKilledAndDetach(SetSlot& s) {
    // SetSlot_MarkKilledAndDetach 0x800C9EEC; DisableAndDetach 0x800C9F1C;
    // SetSlot_Detach 0x800C9FD0. The original unlinks this slot from its
    // link group; rebuilding that one group gives the same list membership.
    // The native SetData owns stage misc buffers. The original's separate
    // dynamic-slot flag 0x40000000 cleanup is outside this stage-owned path.
    SetRecord& r = *s.rec;
    r.runtimeFlags |= kSetFlag8;
    r.runtimeFlags &= ~(kSetEnabled | kSetAlive | kSetDespawnRequest);
    s.object = nullptr;
    s.prev = s.next = nullptr;
    RebuildLinkList(r.link);
}

// 0x800CAC08 (1 player) / 0x800CAA2C (2 players).
void SetData::Scan(const Vec3* players, int n) {
    alive_ = 0;
    for (auto& s : slots_) {
        SetRecord& r = *s.rec;
        if ((suppressBits & 1) && !(r.runtimeFlags & kSetSrcDs1)) continue;
        if (r.runtimeFlags & kSetAlive) { ++alive_; continue; }
        const uint32_t f = r.runtimeFlags;
        if (!(f & kSetEnabled)) {
            if (!(r.flags & kSetEnabled)) continue;          // authored disabled
            if (f & kSetRearm) {
                r.runtimeFlags = (r.flags | (f & 0x208) | kSetEnabled) & ~kSetRearm;
                RebuildLinkList(r.link);
            } else if (f & kSetPendingEnable) {
                r.runtimeFlags = (r.flags | kSetEnabled) & 0xFFFDFFBFu;
                RebuildLinkList(r.link);
            } else if (f & kSetRespawnOutOfRange) {
                bool anyIn = false;
                for (int i = 0; i < n; ++i) anyIn |= InRange(s, players[i]);
                if (!anyIn) r.runtimeFlags |= kSetRearm;
            }
        } else if (s.descIndex >= 0) {
            TrySpawn(s, players, n);
        }
    }
}

void SetData::RegisterFactory(uint16_t id, SetCreateFn fn) {
    for (size_t i = 0; i < catalog_.size(); ++i)
        if (catalog_[i].id == id) factories_[i] = fn;
}

// ---------------------------------------------------------------------------
Vec3 SetSlot_GetPosition(const SetSlot& s) { return s.rec->pos; }          // 0x800CA2FC

Vec3 SetSlot_GetRotationRad(const SetSlot& s) {                            // 0x800CA2BC
    const float k = 3.14159265f / 180.0f;
    return {s.rec->rotDeg.x * k, s.rec->rotDeg.y * k, s.rec->rotDeg.z * k};
}

bool SetSlot_GetParams(SetSlot& s, void* dst, uint32_t size) {             // 0x800CA1A4
    SetRecord& r = *s.rec;
    if (size != r.miscLen || !r.misc) {
        r.runtimeFlags |= kSetParamSizeError;
        return false;
    }
    r.runtimeFlags &= ~kSetParamSizeError;
    // The original memcpy's big-endian words (misc block pre-swapped by
    // fn_80043D34); natively the block is little-endian == host order on x86.
    std::memcpy(dst, r.misc, size);
    return true;
}

void SetSlot_Attach(SetSlot& s, void* obj) {                               // 0x800CA0B8
    s.rec->runtimeFlags = (s.rec->runtimeFlags | kSetAlive) & ~kSetDespawnRequest;
    s.object = obj;
}

void SetSlot_SetFlag8(SetSlot& s) { s.rec->runtimeFlags |= kSetFlag8; }  // 0x800C9C44

bool SetSlot_ShouldDespawn(const SetSlot& s, bool globalBit) {             // 0x800C9E4C
    if (globalBit && !(s.rec->runtimeFlags & kSetSrcDs1)) return true;
    return (s.rec->runtimeFlags & kSetDespawnRequest) != 0;
}

}  // namespace shadow::gameplay
