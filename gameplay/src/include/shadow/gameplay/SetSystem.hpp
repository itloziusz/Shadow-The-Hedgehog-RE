// Native reconstruction of the SET (stage object placement) system of
// Shadow the Hedgehog (GC). Evidence: gameplay/STAGE_GAMEPLAY_DATA_FORMATS.md.
//
// Scope: file parsing (little-endian "sky2" layouts, setid.bin), per-stage
// type enabling, the per-frame spawn scan, link groups and the slot API that
// objects use. Object *behaviour* is not here — creation is delegated to a
// factory registered per object id (unregistered ids fail closed: logged, not faked).
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace shadow::gameplay {

struct Vec3 { float x = 0, y = 0, z = 0; };

// SetRecord+0x18 runtime flag bits (see data-formats doc for confidence per bit).
enum SetFlag : uint32_t {
    kSetEnabled        = 0x00000001,
    kSetAlive          = 0x00000002,
    kSetDespawnRequest = 0x00000004,
    kSetFlag8          = 0x00000008,  // set by objects (GunSoldier at spawn); meaning UNKNOWN
    kSetRearm          = 0x00000040,
    kSetNoFadeRange    = 0x00000800,
    kSetSrcCmn         = 0x00001000,
    kSetSrcNrm         = 0x00002000,
    kSetSrcHrd         = 0x00004000,
    kSetSrcDs1         = 0x00008000,
    kSetRespawnOutOfRange = 0x00010000,
    kSetBlockCreate    = 0x00020000,
    kSetAlwaysInRange  = 0x00080000,
    kSetPendingEnable  = 0x00800000,
    kSetParamSizeError = 0x20000000,
};

// 0x2C-byte on-disc record, byte-swapped with "ffffffiisccii" (fn_800CB1A4).
struct SetRecord {
    Vec3     pos;             // +0x00
    Vec3     rotDeg;          // +0x0C degrees
    uint32_t runtimeFlags;    // +0x18
    uint32_t flags;           // +0x1C authored
    uint16_t id;              // +0x20
    uint8_t  link;            // +0x22
    uint8_t  range;           // +0x23 activation radius / 100
    uint32_t miscLen;         // +0x24
    const uint8_t* misc;      // +0x28 (runtime pointer)
};

struct SetSlot {              // 0x20 bytes in the original
    SetRecord* rec = nullptr; // +0x00
    SetSlot*   prev = nullptr;// +0x04 link-group list
    SetSlot*   next = nullptr;// +0x08
    void*      object = nullptr; // +0x0C attached instance (SetSlot_Attach)
    int32_t    descIndex = -1;// +0x10 index into the catalog, -1 = unknown id
    uint32_t   field14 = 0;   // +0x14 (fn_800C9D18) meaning UNKNOWN
};

// Type codes named by the DOL's own "Sample" object (id 0x2586) params: "Sint32","Uint32","Hex","Single".
enum class SetParamType : uint32_t { None = 0, Sint32 = 1, Uint32 = 2, Hex = 3, Single = 4 };

struct SetParamDesc {         // 0x28-byte editor schema entry in the DOL
    SetParamType type;
    const char*  name;
    int32_t      iDefault, iMin, iMax;
    float        fDefault, fMin, fMax;
};

using SetCreateFn = std::function<void*(SetSlot&)>;

struct SetObjDesc {           // 0x24-byte descriptor (table 0x8052C1A0)
    const char* name;
    uint16_t    id;
    uint8_t     category;     // +0x1A (1 gadget / 2 enemy / 4 ?) LIKELY
    uint32_t    origDescAddr; // address of the descriptor in main.dol (evidence)
    uint32_t    origCreateHook;
    const SetParamDesc* params;
    int         paramCount;
    bool        usedInStage = false;   // +0x1B bit0
};

// Generated from main.dol by tools/gen_catalog_cpp.py
const std::vector<SetObjDesc>& OriginalSetCatalog();

class SetData {
public:
    static constexpr int kMaxRecords = 0x800;

    SetData();
    // SetData_Reset 0x800CAF38
    void Reset();
    // SetData_LoadSetId 0x800CB044 — per-stage type masks
    bool LoadSetId(const std::string& path);
    // SetData_MarkUsedTypes 0x800CA7F0
    void MarkUsedTypes(int stageIndex);
    // SetData_LoadFile 0x800CB1A4 / SetData_LoadFileWithSourceMask 0x800CB0EC
    bool LoadFile(const std::string& path, uint32_t orMask = 0, uint32_t andNotMask = 0);
    // SetData_LoadStage 0x800CBC54 — whole stage (cmn, nrm|hrd, ds1)
    bool LoadStage(const std::string& filesRoot, int stageNumber, bool hardMode, int stageIndex);
    // SetData_BuildSlots 0x800CA6B4
    void BuildSlots();

    // Per-frame scan: SetData_ScanSlots1P 0x800CAC08 / 2P 0x800CAA2C.
    // NOTE: the original passes CAMERA unit positions (fn_80010244(fn_80009548(), i)), not players.
    void Scan(const Vec3* players, int playerCount);
    // SetSlot_InRange 0x800CA970
    static bool InRange(const SetSlot& s, const Vec3& player);
    // SetSlot_TrySpawn 0x800CA8D8
    bool TrySpawn(SetSlot& s, const Vec3* players, int playerCount);
    // SetData_RebuildLinkList 0x800CAE24
    void RebuildLinkList(uint8_t link);
    // 0x800C9EEC -> 0x800C9F1C -> 0x800C9FD0: normal enemy death.
    // Clears enabled/alive, preserves flag 0x8, detaches and unlinks the slot.
    void MarkKilledAndDetach(SetSlot& slot);

    void RegisterFactory(uint16_t id, SetCreateFn fn);

    int recordCount() const { return count_; }
    int aliveCount() const { return alive_; }
    std::vector<SetRecord>& records() { return records_; }
    std::array<SetSlot, kMaxRecords>& slots() { return slots_; }
    SetSlot* linkHead(uint8_t link) { return linkHeads_[link]; }
    uint32_t suppressBits = 0;   // +0x428; bit0 hides objects without kSetSrcDs1(0x8000)
    std::vector<std::string> log;   // fail-closed diagnostics

private:
    int count_ = 0;                                  // +0x000
    int alive_ = 0;                                  // +0x008
    struct SetIdEntry { uint32_t id; uint32_t mask[2]; };
    std::vector<SetIdEntry> setId_;                  // +0x00C/+0x010
    std::vector<SetObjDesc> catalog_;                // +0x014 (copy, flags mutable)
    std::array<SetSlot, kMaxRecords> slots_{};       // +0x018
    std::array<SetSlot*, 256> linkHeads_{};          // +0x01C
    std::vector<SetRecord> records_;                 // +0x41C
    std::vector<std::vector<uint8_t>> miscStore_;    // +0x420 buffer
    std::vector<SetCreateFn> factories_;             // indexed by catalog index
};

// Slot API used by objects (addresses of the original helpers)
Vec3 SetSlot_GetPosition(const SetSlot& s);                        // 0x800CA2FC
Vec3 SetSlot_GetRotationRad(const SetSlot& s);                     // 0x800CA2BC
bool SetSlot_GetParams(SetSlot& s, void* dst, uint32_t size);      // 0x800CA1A4
void SetSlot_Attach(SetSlot& s, void* obj);                        // 0x800CA0B8
void SetSlot_SetFlag8(SetSlot& s);                                 // 0x800C9C44
bool SetSlot_ShouldDespawn(const SetSlot& s, bool globalDespawnBit); // 0x800C9E4C

}  // namespace shadow::gameplay
