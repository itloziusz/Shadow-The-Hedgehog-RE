#include "golden_trace.h"

#include "boot_image.h"
#include "native_boot_manifest.h"

#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string_view>

namespace {

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw BootError("cannot open oracle file");
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

// Validate the entire JSON document before consuming evidence. Only root
// members can supply oracle fields; metadata objects cannot impersonate them.
class OracleDocument {
public:
    explicit OracleDocument(const std::string& source) : text(source) {
        Object(0);
        Ws();
        if (cursor != text.size()) Fail();
    }

    bool Has(std::string_view key) const { return members.contains(std::string(key)); }
    std::size_t Position(std::string_view key) const {
        const auto found = members.find(std::string(key));
        if (found == members.end()) {
            throw BootError("oracle is missing \"" + std::string(key) + "\"");
        }
        return found->second;
    }

    const std::string& text;

private:
    std::size_t cursor = 0;
    std::map<std::string, std::size_t> members;
    [[noreturn]] static void Fail() { throw BootError("malformed oracle JSON"); }
    void Ws() {
        while (cursor < text.size() &&
               (text[cursor] == ' ' || text[cursor] == '\t' ||
                text[cursor] == '\r' || text[cursor] == '\n')) ++cursor;
    }
    bool Take(char value) {
        Ws();
        if (cursor == text.size() || text[cursor] != value) return false;
        ++cursor;
        return true;
    }
    void Need(char value) { if (!Take(value)) Fail(); }
    unsigned Hex4() {
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            if (cursor == text.size()) Fail();
            const char c = text[cursor++];
            unsigned digit;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
            else { Fail(); }
            value = value * 16u + digit;
        }
        return value;
    }
    static void Utf8(std::string& result, unsigned value) {
        if (value < 0x80u) result += static_cast<char>(value);
        else if (value < 0x800u) {
            result += static_cast<char>(0xC0u | (value >> 6));
            result += static_cast<char>(0x80u | (value & 63u));
        } else if (value < 0x10000u) {
            result += static_cast<char>(0xE0u | (value >> 12));
            result += static_cast<char>(0x80u | ((value >> 6) & 63u));
            result += static_cast<char>(0x80u | (value & 63u));
        } else {
            result += static_cast<char>(0xF0u | (value >> 18));
            result += static_cast<char>(0x80u | ((value >> 12) & 63u));
            result += static_cast<char>(0x80u | ((value >> 6) & 63u));
            result += static_cast<char>(0x80u | (value & 63u));
        }
    }
    std::string String() {
        Need('"');
        std::string result;
        while (cursor < text.size()) {
            const unsigned char c = static_cast<unsigned char>(text[cursor++]);
            if (c == '"') return result;
            if (c < 0x20u) Fail();
            if (c >= 0x80u) {
                unsigned count;
                unsigned value;
                unsigned minimum;
                if (c >= 0xC2u && c <= 0xDFu) { count = 1; value = c & 31u; minimum = 0x80u; }
                else if (c >= 0xE0u && c <= 0xEFu) { count = 2; value = c & 15u; minimum = 0x800u; }
                else if (c >= 0xF0u && c <= 0xF4u) { count = 3; value = c & 7u; minimum = 0x10000u; }
                else { Fail(); }
                for (unsigned i = 0; i < count; ++i) {
                    if (cursor == text.size()) Fail();
                    const auto next = static_cast<unsigned char>(text[cursor++]);
                    if ((next & 0xC0u) != 0x80u) Fail();
                    value = (value << 6) | (next & 63u);
                }
                if (value < minimum || value > 0x10FFFFu || (value >= 0xD800u && value <= 0xDFFFu)) Fail();
                Utf8(result, value);
                continue;
            }
            if (c != '\\') { result += static_cast<char>(c); continue; }
            if (cursor == text.size()) Fail();
            switch (text[cursor++]) {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u': {
                unsigned value = Hex4();
                if (value >= 0xD800u && value <= 0xDBFFu) {
                    if (cursor + 2 > text.size() || text[cursor] != '\\' || text[cursor + 1] != 'u') Fail();
                    cursor += 2;
                    const unsigned low = Hex4();
                    if (low < 0xDC00u || low > 0xDFFFu) Fail();
                    value = 0x10000u + ((value - 0xD800u) << 10) + low - 0xDC00u;
                } else if (value >= 0xDC00u && value <= 0xDFFFu) { Fail(); }
                Utf8(result, value);
                break;
            }
            default: Fail();
            }
        }
        Fail();
    }
    void Object(unsigned depth) {
        Need('{');
        std::set<std::string> keys;
        if (Take('}')) return;
        do {
            const std::string key = String();
            if (!keys.insert(key).second) throw BootError("oracle repeats a JSON key");
            if (depth == 0) members.emplace(key, cursor);
            Need(':');
            Value(depth + 1);
            if (Take('}')) return;
        } while (Take(','));
        Fail();
    }
    void Digits() {
        const std::size_t first = cursor;
        while (cursor < text.size() && text[cursor] >= '0' && text[cursor] <= '9') ++cursor;
        if (cursor == first) Fail();
    }
    void Value(unsigned depth) {
        if (depth > 64) throw BootError("oracle JSON nesting limit exceeded");
        Ws();
        if (cursor == text.size()) Fail();
        if (text[cursor] == '{') { Object(depth); return; }
        if (text[cursor] == '"') { (void)String(); return; }
        if (Take('[')) {
            if (Take(']')) return;
            do {
                Value(depth + 1);
                if (Take(']')) return;
            } while (Take(','));
            Fail();
        }
        for (const std::string_view literal : {"true", "false", "null"}) {
            if (text.compare(cursor, literal.size(), literal) == 0) {
                cursor += literal.size();
                return; // The enclosing container must supply a delimiter.
            }
        }
        if (text[cursor] == '-') ++cursor;
        if (cursor < text.size() && text[cursor] == '0') ++cursor;
        else Digits();
        if (cursor < text.size() && text[cursor] == '.') { ++cursor; Digits(); }
        if (cursor < text.size() && (text[cursor] == 'e' || text[cursor] == 'E')) {
            ++cursor;
            if (cursor < text.size() && (text[cursor] == '+' || text[cursor] == '-')) ++cursor;
            Digits();
        }
    }
};

void SkipWs(const std::string& text, std::size_t& cursor) {
    while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
}

std::string ParseString(const std::string& text, std::size_t cursor) {
    SkipWs(text, cursor);
    if (cursor >= text.size() || text[cursor] != ':') {
        throw BootError("oracle key is not followed by a value");
    }
    ++cursor;
    SkipWs(text, cursor);
    if (cursor >= text.size() || text[cursor] != '"') {
        throw BootError("oracle value is not a string");
    }
    ++cursor;
    std::string value;
    while (cursor < text.size() && text[cursor] != '"') {
        if (text[cursor] == '\\') {
            throw BootError("oracle strings in this schema are plain");
        }
        value.push_back(text[cursor]);
        ++cursor;
    }
    if (cursor >= text.size() || text[cursor] != '"') {
        throw BootError("unterminated oracle string");
    }
    return value;
}

bool ParseBool(const std::string& text, std::size_t cursor) {
    SkipWs(text, cursor);
    if (cursor >= text.size() || text[cursor] != ':') {
        throw BootError("oracle key is not followed by a value");
    }
    ++cursor;
    SkipWs(text, cursor);
    if (text.compare(cursor, 4, "true") == 0) {
        return true;
    }
    if (text.compare(cursor, 5, "false") == 0) {
        return false;
    }
    throw BootError("oracle bool is not true or false");
}

std::string RequiredString(const OracleDocument& document, std::string_view key) {
    return ParseString(document.text, document.Position(key));
}

bool IsHex(std::string_view text, std::size_t count) {
    if (text.size() != count) {
        return false;
    }
    for (const char character : text) {
        if (!std::isxdigit(static_cast<unsigned char>(character))) {
            return false;
        }
    }
    return true;
}

}  // namespace

PreEntryOracle ParsePreEntryOracle(const std::string& text) {
    const OracleDocument document(text);
    if (RequiredString(document, "revision") != NativeBootManifest::revision) {
        throw BootError("oracle revision is not GUPP8P");
    }
    if (RequiredString(document, "checkpoint") != "pre_dol_entry") {
        throw BootError("oracle checkpoint is not pre_dol_entry");
    }
    if (RequiredString(document, "evidence") != "DOLPHIN_ORACLE") {
        throw BootError("oracle evidence class is not DOLPHIN_ORACLE");
    }
    if (ParseBool(text, document.Position("applied_by_native_boot"))) {
        throw BootError("oracle claims the native boot already applied external state");
    }

    PreEntryOracle oracle;
    oracle.present = true;
    oracle.pc = RequiredString(document, "pc");
    oracle.memory_0x805f1f30 = RequiredString(document, "memory_0x805F1F30");
    oracle.memory_0x805f1f38 = RequiredString(document, "memory_0x805F1F38");
    if (oracle.pc != "0x80003154") {
        throw BootError("oracle PC is not the DOL entry");
    }
    if (!IsHex(oracle.memory_0x805f1f30, 16) || !IsHex(oracle.memory_0x805f1f38, 16)) {
        throw BootError("oracle memory fields are not 8-byte hex");
    }

    if (document.Has("msr")) {
        oracle.msr = RequiredString(document, "msr");
    }
    if (document.Has("hid0")) {
        oracle.hid0 = RequiredString(document, "hid0");
    }
    if (document.Has("fpscr")) {
        oracle.fpscr = RequiredString(document, "fpscr");
    }
    if (document.Has("entry_instruction_hex")) {
        oracle.entry_instruction_hex = RequiredString(document, "entry_instruction_hex");
        if (oracle.entry_instruction_hex != "4800015d" &&
            oracle.entry_instruction_hex != "4800015D") {
            throw BootError("oracle entry instruction is not the pinned bl");
        }
    }
    bool any_lowmem = false;
    for (const std::string_view key : {"lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044",
                                      "lowmem_0x800000F4", "lowmem_0x800030E4", "lowmem_0x800030E6",
                                      "bi2_bytes"}) {
        any_lowmem = any_lowmem || document.Has(key);
    }
    if (any_lowmem) {
        oracle.lowmem_0x80000030 = RequiredString(document, "lowmem_0x80000030");
        oracle.lowmem_0x80000034 = RequiredString(document, "lowmem_0x80000034");
        oracle.lowmem_0x80000044 = RequiredString(document, "lowmem_0x80000044");
        oracle.lowmem_0x800000f4 = RequiredString(document, "lowmem_0x800000F4");
        oracle.lowmem_0x800030e4 = RequiredString(document, "lowmem_0x800030E4");
        oracle.lowmem_0x800030e6 = RequiredString(document, "lowmem_0x800030E6");
        oracle.bi2_bytes = RequiredString(document, "bi2_bytes");
        oracle.has_lowmem = true;
    }
    return oracle;
}

PreEntryOracle LoadPreEntryOracle(const std::filesystem::path& path) {
    return ParsePreEntryOracle(ReadFile(path));
}

Constructor0Oracle ParseConstructor0Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_1_executed"))) {
        throw BootError("constructor oracle identity/completion mismatch");
    }
    const auto hex = [](std::string value) -> GuestWord32 {
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, 8)) throw BootError("constructor observation is not a word");
        return static_cast<GuestWord32>(std::stoul(value, nullptr, 16));
    };
    const auto observation = [&doc, &hex](const std::string& prefix, bool bases) {
        FragmentObservation item;
        item.id = hex(RequiredString(doc, prefix + "_id"));
        const std::string slot = RequiredString(doc, prefix + "_slot");
        if (!IsHex(slot, 24)) throw BootError("constructor slot is not three words");
        for (std::size_t i = 0; i < 3; ++i) item.slot[i] = hex(slot.substr(i * 8, 8));
        if (bases) {
            item.r2 = hex(RequiredString(doc, prefix + "_r2"));
            item.r13 = hex(RequiredString(doc, prefix + "_r13"));
        }
        return item;
    };
    Constructor0Oracle oracle;
    const std::array<std::string, 9> input_keys{
        "memory_0x805F1F30", "memory_0x805F1F38", "lowmem_0x80000030", "lowmem_0x80000034",
        "lowmem_0x80000044", "lowmem_0x800000F4", "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < input_keys.size(); ++i)
        oracle.startup_inputs[i] = RequiredString(doc, "input_" + input_keys[i]);
    const std::array<std::string, 4> labels{"dol_entry", "after_crt", "before_walker", "constructor_0_entry"};
    for (std::size_t i = 0; i < labels.size(); ++i) oracle.boundaries[i] = observation(labels[i], true);
    oracle.final_state = observation("final", false);
    oracle.final_pc = hex(RequiredString(doc, "final_pc"));
    oracle.final_cursor = hex(RequiredString(doc, "final_cursor"));
    oracle.final_next = hex(RequiredString(doc, "final_next"));
    oracle.final_r1 = hex(RequiredString(doc, "final_r1"));
    oracle.final_lr = hex(RequiredString(doc, "final_lr"));
    oracle.present = true;
    return oracle;
}

Constructor0Oracle LoadConstructor0Oracle(const std::filesystem::path& path) {
    return ParseConstructor0Oracle(ReadFile(path));
}

Constructor1Oracle ParseConstructor1Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_2_executed")))
        throw BootError("constructor 1 oracle identity/completion mismatch");
    Constructor1Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 1 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc", "lr", "cr", "fpscr", "msr", "ctr", "xer",
            "context_address", "fpu_handler", "fragment_id", "r0", "r1", "r2", "r3", "r4",
            "r5", "r12", "r13", "r31"}) field(prefix + key, 8);
        for (const std::string key : {"f0", "f1", "f2", "f31"}) field(prefix + key, 16);
        field(prefix + "vectors", 72); field(prefix + "angles", 16);
        field(prefix + "low_context", 64); field(prefix + "context", 0x590);
        field(prefix + "fragment_slot", 24);
    }
    field("paired_spill", 16);
    field("fault_xer", 8);
    for (const std::string address : {"805F27AC", "805F27B8", "805F27BC", "805F27C0",
                                     "805F27C4", "805F27C8", "805FBDB0"})
        field("constant_0x" + address, 8);
    result.present = true;
    return result;
}

Constructor1Oracle LoadConstructor1Oracle(const std::filesystem::path& path) {
    return ParseConstructor1Oracle(ReadFile(path));
}

Constructor2Oracle ParseConstructor2Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_3_executed")))
        throw BootError("constructor 2 oracle identity/completion mismatch");
    Constructor2Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 2 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 24); field(prefix + "destination", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    for (const std::string label : {"after_crt", "before_walker", "constructor_2_entry"})
        field(label + "_source", 24);
    result.present = true;
    return result;
}

Constructor2Oracle LoadConstructor2Oracle(const std::filesystem::path& path) {
    return ParseConstructor2Oracle(ReadFile(path));
}

Constructor3Oracle ParseConstructor3Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_4_executed")))
        throw BootError("constructor 3 oracle identity/completion mismatch");
    Constructor3Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 3 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 16); field(prefix + "destination", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    for (const std::string label : {"after_crt", "before_walker", "constructor_3_entry"})
        field(label + "_source", 16);
    result.present = true;
    return result;
}

Constructor3Oracle LoadConstructor3Oracle(const std::filesystem::path& path) {
    return ParseConstructor3Oracle(ReadFile(path));
}

Constructor4Oracle ParseConstructor4Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_5_executed")))
        throw BootError("constructor 4 oracle identity/completion mismatch");
    Constructor4Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 4 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 8); field(prefix + "destination", 16);
        field(prefix + "node", 24); field(prefix + "c3_vector", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    for (const std::string label : {"after_crt", "before_walker", "constructor_4_entry"})
        field(label + "_source", 8);
    result.present = true;
    return result;
}

Constructor4Oracle LoadConstructor4Oracle(const std::filesystem::path& path) {
    return ParseConstructor4Oracle(ReadFile(path));
}

Constructor5Oracle ParseConstructor5Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_6_executed")))
        throw BootError("constructor 5 oracle identity/completion mismatch");
    Constructor5Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 5 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 8); field(prefix + "destination", 8);
        field(prefix + "c4_object", 16); field(prefix + "neighbors", 16);
        field(prefix + "node", 24); field(prefix + "c3_vector", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    result.present = true;
    return result;
}

Constructor5Oracle LoadConstructor5Oracle(const std::filesystem::path& path) {
    return ParseConstructor5Oracle(ReadFile(path));
}

Constructor6Oracle ParseConstructor6Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_7_executed")))
        throw BootError("constructor 6 oracle identity/completion mismatch");
    Constructor6Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 6 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 8); field(prefix + "destination", 32);
        field(prefix + "c5_bytes", 8); field(prefix + "c4_node", 24);
        field(prefix + "c4_object", 16); field(prefix + "c5_neighbors", 16);
        field(prefix + "node", 24); field(prefix + "c3_vector", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    result.present = true;
    return result;
}

Constructor6Oracle LoadConstructor6Oracle(const std::filesystem::path& path) {
    return ParseConstructor6Oracle(ReadFile(path));
}

Constructor7Oracle ParseConstructor7Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_8_executed")))
        throw BootError("constructor 7 oracle identity/completion mismatch");
    Constructor7Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 7 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 8); field(prefix + "destination", 32);
        field(prefix + "c5_bytes", 8); field(prefix + "c4_node", 24);
        field(prefix + "c4_object", 16); field(prefix + "c5_neighbors", 16);
        field(prefix + "node", 96);
        field(prefix + "c6_array", 32); field(prefix + "c6_node", 24); field(prefix + "c3_vector", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    result.present = true;
    return result;
}

Constructor7Oracle LoadConstructor7Oracle(const std::filesystem::path& path) {
    return ParseConstructor7Oracle(ReadFile(path));
}

Constructor8Oracle ParseConstructor8Oracle(const std::string& text) {
    const OracleDocument doc(text);
    if (RequiredString(doc, "evidence") != "DOLPHIN_ORACLE" ||
        RequiredString(doc, "dol_sha256") != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af" ||
        ParseBool(text, doc.Position("applied_by_native_boot")) ||
        !ParseBool(text, doc.Position("complete")) ||
        ParseBool(text, doc.Position("constructor_9_executed")))
        throw BootError("constructor 8 oracle identity/completion mismatch");
    Constructor8Oracle result;
    const std::array<std::string, 9> keys{"memory_0x805F1F30", "memory_0x805F1F38",
        "lowmem_0x80000030", "lowmem_0x80000034", "lowmem_0x80000044", "lowmem_0x800000F4",
        "lowmem_0x800030E4", "lowmem_0x800030E6", "bi2_bytes"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        result.startup_inputs[i] = RequiredString(doc, "input_" + keys[i]);
    const auto field = [&](const std::string& name, std::size_t count) {
        auto value = RequiredString(doc, name);
        if (value.starts_with("0x")) value.erase(0, 2);
        if (!IsHex(value, count)) throw BootError("invalid constructor 8 field: " + name);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        result.fields[name] = value;
    };
    for (const std::string prefix : {"entry_", "final_"}) {
        for (const std::string key : {"pc","lr","cr","fpscr","msr","ctr","xer","fragment_id","owner"})
            field(prefix + key, 8);
        for (unsigned i = 0; i < 32; ++i) {
            field(prefix + "r" + std::to_string(i), 8);
            field(prefix + "f" + std::to_string(i), 16);
        }
        field(prefix + "source", 8); field(prefix + "destination", 32);
        field(prefix + "c5_bytes", 8); field(prefix + "c4_node", 24);
        field(prefix + "c4_object", 16); field(prefix + "c5_neighbors", 16);
        field(prefix + "c7_nodes", 96); field(prefix + "c7_objects", 32);
        field(prefix + "c6_array", 32); field(prefix + "c6_node", 24); field(prefix + "c3_vector", 24);
        field(prefix + "c2_vectors", 48);
        field(prefix + "prior_vectors", 72); field(prefix + "prior_angles", 16);
        field(prefix + "fragment_slot", 24); field(prefix + "context", 0x590);
    }
    result.present = true;
    return result;
}

Constructor8Oracle LoadConstructor8Oracle(const std::filesystem::path& path) {
    return ParseConstructor8Oracle(ReadFile(path));
}
