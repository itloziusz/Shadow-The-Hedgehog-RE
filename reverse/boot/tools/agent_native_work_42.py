#!/usr/bin/env python3
"""Finite original-operation work annotations for the established native prefix.

This does not execute PPC, take a host timestamp, ingest a frontier count, or
admit a native clock. Inputs select already proved semantic paths. The only
instruction decoding is read-only metadata checking for the static proof units.
Optional traces/captures are compared after independently producing work.
"""
from __future__ import annotations
import argparse
from collections import Counter
import copy
import hashlib
import json
from pathlib import Path
import re
import struct

DOL_SHA = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"
TABLE_SHA = "4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771"
COPY_DESCRIPTORS = (
    (0x80003100, 0x80003100, 0x24E8), (0x80005600, 0x80005600, 0x1F08),
    (0x80007520, 0x80007520, 0x1814), (0x80008D40, 0x80008D40, 0x4A1F08),
    (0x804AAC60, 0x804AAC60, 0x46C), (0x804AB0E0, 0x804AB0E0, 0xC),
    (0x804AB100, 0x804AB100, 0x72418), (0x8051D520, 0x8051D520, 0x528C8),
    (0x805E4500, 0x805E4500, 0xAB20), (0x805F2780, 0x805F2780, 0x9DB8),
    (0, 0, 0),
)
ZERO_DESCRIPTORS = ((0x8056FE00, 0x74700), (0x805EF020, 0x375C), (0x805FC540, 0xAC), (0, 0))


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Dol:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        require(sha(self.data) == DOL_SHA, "original DOL identity mismatch")
        h = lambda off: struct.unpack_from(">I", self.data, off)[0]
        self.sections = []
        for ob, ab, sb, count in ((0, 0x48, 0x90, 7), (0x1c, 0x64, 0xac, 11)):
            for i in range(count):
                off, base, size = h(ob+4*i), h(ab+4*i), h(sb+4*i)
                if size:
                    require(off+size <= len(self.data), "section exceeds original DOL")
                    self.sections.append((base, off, size))

    def word(self, address):
        offsets = [off+address-base for base, off, size in self.sections if base <= address and address+4 <= base+size]
        require(not address & 3 and len(offsets) == 1, "unproved static DOL word")
        return struct.unpack_from(">I", self.data, offsets[0])[0]


def span(first, last):
    return tuple(range(first, last+4, 4))


# These are reached operations belonging to fixed semantic units, not a guest
# PC program. Calls/returns and conditionals are not interpreted. Untaken paths
# are separately gated or declined according to the connected native contract.
UNITS = {
    "entry_register_call": (0x80003154,),
    "register_seed": span(0x800032B0, 0x8000333C),
    "hardware_call": (0x80003158,),
    "hardware_prepare": span(0x80003400, 0x80003410),
    "paired_frame": span(0x80371714, 0x80371720),
    "hid2_read": span(0x80370BA8, 0x80370BAC),
    "paired_enable_call": span(0x80371724, 0x80371728),
    "hid2_write": span(0x80370BB0, 0x80370BB4),
    "icfi_call": (0x8037172C,),
    "icfi_leaf": span(0x803725F4, 0x80372600),
    "sync_gqr_return": span(0x80371730, 0x80371764),
    "fpr_call": (0x80003414,),
    "fpr_seed": span(0x80370CDC, 0x80370E00),
    "cache_call": (0x80003418,),
    "cache_first_check": span(0x80372838, 0x80372860),
    "hid0_read": span(0x80370AEC, 0x80370AF0),
    "cache_second_check": span(0x80372874, 0x80372880),
    "l2_check": span(0x80372894, 0x803728A0),
    "l2_read": span(0x80370AFC, 0x80370B00),
    "l2_write": span(0x80370B04, 0x80370B08),
    "msr_read": span(0x80370ADC, 0x80370AE0),
    "msr_write": span(0x80370AE4, 0x80370AE8),
    "l2_disabled_caller": span(0x803728A4, 0x803728F4),
    "l2_invalidate_body": span(0x80372640, 0x803726A4)+span(0x803726B4, 0x803726D4),
    "logger": span(0x80370C8C, 0x80370C90)+span(0x80370CB4, 0x80370CD8),
    "handler_arguments": span(0x803728F8, 0x80372900),
    "handler_call": (0x80372904,),
    "handler_frame": span(0x80373378, 0x8037339C),
    "disable_ee": span(0x8037611C, 0x8037612C),
    "handler_select": span(0x803733A0, 0x803733C4),
    "handler_restore_call": span(0x80373564, 0x80373568),
    "restore_ee_zero": span(0x80376144, 0x8037614C)+span(0x80376158, 0x80376164),
    "handler_return": span(0x8037356C, 0x8037358C),
    "handler_logger_call": span(0x80372908, 0x80372910),
    "cache_return": span(0x80372914, 0x80372928),
    "hardware_return": span(0x8000341C, 0x80003420),
    "sentinel_walker_call": span(0x8000315C, 0x8000316C),
    "descriptor_frame": span(0x80003340, 0x80003368),
    "identity_copy_descriptor": span(0x8000336C, 0x80003388)+span(0x800033A4, 0x800033A8),
    "copy_terminator": span(0x8000336C, 0x80003374),
    "zero_table_prepare": span(0x800033AC, 0x800033BC),
    "zero_descriptor": span(0x800033C0, 0x800033D8)+span(0x800033DC, 0x800033E0),
    "zero_wrapper": span(0x8000540C, 0x80005438),
    "zero_leaf_fixed": span(0x8000543C, 0x80005458)+span(0x8000546C, 0x80005470)+span(0x8000548C, 0x80005494)+span(0x800054C0, 0x800054C4)+span(0x800054D4, 0x800054E0),
    "zero_group": span(0x80005498, 0x800054BC),
    "zero_remaining_word": span(0x800054C8, 0x800054D0),
    "zero_terminator": span(0x800033C0, 0x800033C8),
    "descriptor_return": span(0x800033E4, 0x800033FC),
    "crt_entry_tail": span(0x80003170, 0x80003184),
    "bi2_pointer_nonzero": span(0x80003188, 0x80003198),
    "bi2_debug_ordinary": span(0x800031BC, 0x800031D8),
    "bi2_debug4_call_return": span(0x800031DC, 0x800031E4),
    "bi2_debug4_leaf": span(0x80003140, 0x80003148),
    "bi2_offset_check": span(0x800031F8, 0x80003214),
    "bi2_no_array": span(0x80003258, 0x8000325C),
    "bi2_count_check": span(0x80003218, 0x80003224),
    "bi2_array_prepare": span(0x80003228, 0x8000322C),
    "bi2_relocate_item": span(0x80003230, 0x80003240),
    "bi2_array_publish": span(0x80003244, 0x80003254),
    "metadata_call": (0x80003260,),
    "metadata_leaf": span(0x80370BF0, 0x80370C14),
    "os_call": (0x80003264,),
    "os_first_guard_frame": span(0x80370E68, 0x80370EA4),
    "clock_frame_call_disable": span(0x80379648, 0x80379660),
    "clock_sample_call": span(0x80379664, 0x80379668),
}

# Per-unit SHA256 of addressBE32 || original wordBE32, in declared order.
# Filled from the pinned original bytes once, then immutable proof gates.
UNIT_PINS = {
    "entry_register_call": "59dfaa7682e58022a901952c50794939c38c61df47f59e8873b22d386bd42bc6",
    "register_seed": "080cce80236b961d2416b90e72e86f1fd7382441219b83c1ad8f39e2d2c14ff8",
    "hardware_call": "64c0f326c78bf1ba1a7adbd8c1706e6b5ddf3d5d08adc25b63d2c92bb6eec320",
    "hardware_prepare": "1336630bee4fd6296eb9719c77f41348327c6e198f215cd21367ad97a5521d56",
    "paired_frame": "64b31467eaf48c4b1e1eb76b249a43e9fd7c53dee0c656c2efd412e445c62bd6",
    "hid2_read": "538276da3dbf3e36ab5ac5901064516ccb758d4c90802e435ff30338e8064ec8",
    "paired_enable_call": "96bf3c5c766e90ca7dc3a030dffb8c216b4c1aa3950f13872734eda25a5fced8",
    "hid2_write": "07ef0919a3cc3b9ed78d07471a6dc1c9ff557281af98b0fcd72ceea9d3bc3a7d",
    "icfi_call": "fa36a1b6ecaff17818cf74295f13f4b08ad2af459b8185bd8b4217d9d76d3ec3",
    "icfi_leaf": "726ac395cef5a7b5fac2d58256f031669dba8cccca226959c59d93a4f45c7a78",
    "sync_gqr_return": "d5ff6a2d2741219f332d55f85e121f37208fc89e9e790995d85d5a9421dabbdc",
    "fpr_call": "c7f914389f3ca0d408cda9e64769ed256d22ea0152fc2cff8203ef1673402879",
    "fpr_seed": "a87830dc9f95f94f3803106aab7b2b9e39a3662ba84dc05444981b1765e8bf62",
    "cache_call": "49757a558411e9d0e1998043075268a5973aae76b7c242fceb8e0eb88e393e25",
    "cache_first_check": "dc8f305c21f5617f12f4a682bd1ba0c96c19620e46fa50a16c6c22c46c05319d",
    "hid0_read": "89c28f50635cd43ea9809356818d885e02d3eea737bd84a96d8440f7db1cf3b2",
    "cache_second_check": "a26938bf019c55b06cad90b87582b08c7ada023e62476fc33c0a076ea2f3fe38",
    "l2_check": "758aebf9853b02d22d7bf8b8e3b467e556255f539188f73d419f4d49109c7da8",
    "l2_read": "80df99292ca9f1efd069f2ceba44d7e61b5250969e22def37d2735866b3b24dc",
    "l2_write": "bbf276c614e8c25a8cbe1a74562d8762d61a2e441633ad7439477c08e8f520bb",
    "msr_read": "127fd69db6263e3c7870f5e43dd22bc5fdcc3f873eca4b7982470662c10792f3",
    "msr_write": "9cb567e86575ed12fb1dce90a43229a5f91396b2afe0c1d2fe636692bde3262e",
    "l2_disabled_caller": "4c5497352add1ef13ad3a8c8e0d1aae00ba36bea692bd97cadd97b835db16cc7",
    "l2_invalidate_body": "3dd29d7356631e1b4f4a50a30a399685c7098f3153462c7c9eca4bb649ab639c",
    "logger": "7b60fd3dacba227c48ddee506b81ac01c1ff72937779634177e80df526705606",
    "handler_arguments": "f21b177befefed67e1dc1850ef12b44fee5b83bf4188494f320685a1e13ed2eb",
    "handler_call": "328675fd6070c2368aedc74bf6523bd6de1f680103a5cc2e36f0e7b1b4ac88f7",
    "handler_frame": "17311e91da4030c001b1fc1c93427219dd3a4d1644db9385911d74b6d728fe62",
    "disable_ee": "560fee0651ade315c86f3ad757f0201c19230e39d14d5d1a15f5e9c19704c23d",
    "handler_select": "0d1bfdf9fac76c8df9856088f9ef088f106f629aa9306cd60bd93150edd096ec",
    "handler_restore_call": "af9850af65d6982e6f4b068c9f43f20483eccd852543ebb45b1e1045f4a44187",
    "restore_ee_zero": "c22938c14f71361babb6ad11cb376efe3485e8c0bb823b50516c96b12d8e691d",
    "handler_return": "f8b3dd8305a6444eec475bc913f1cb51e15f26a43787f875b907c8af2b11ed64",
    "handler_logger_call": "fa98abf7c292d0024769575c5c379e1f0d4979d997bd405ba3041527fc92c4ec",
    "cache_return": "c95f09ee3b5723674bc14796da664dd37ecf0be50323f1d4737301860c1cfa80",
    "hardware_return": "10477fb8244402bae830912f1380922a78952ba45a97e9bcd2f0a2c25df2005c",
    "sentinel_walker_call": "125f80a2fe77a4bf82b891fd72a706e92c3b7c866f4d8b0034d7b19b8a824115",
    "descriptor_frame": "27c71cbac1b66b3fab8dcef2dccad1a94eb0ac8b0c760d91d7964c334aa610a2",
    "identity_copy_descriptor": "14d92ac9022d01a9acb83b14e9d8786e2543b103b23b613ae5ecd4e9c330616a",
    "copy_terminator": "adcfd477eae5470c0a24d8e71ea9f849f21ea91e6d67fce68a6509a34ad55298",
    "zero_table_prepare": "7c62adbe869405aa26b93bd7dfa7f934f335c737423af35e3492008e1c115c62",
    "zero_descriptor": "5431bd03b356603768b9465c5ac0c8a2237d9fd9993824d0ba97bcbed33ca59e",
    "zero_wrapper": "f2cb1c36598116f182c1a6b4ff64422951fb8794553ca7091e42e5b1c7ab38de",
    "zero_leaf_fixed": "388df1804af960d5c58fbe13ec8a989c47abdfcd6ddccc931e10453d20338f59",
    "zero_group": "2740c30348dc595eee7067715a6a61b7361d6d2e26c1da415b896e8ea15dad69",
    "zero_remaining_word": "72360f2cbfd03c9f80bf5209f3a12f3e40dcf9e88d8e1f3ce2f3f60c6a5fb3a8",
    "zero_terminator": "db67d0dc0415d6a444a6c583c4803ff7a40e5807e192eaf30adfa662c52d70d3",
    "descriptor_return": "bcc0bc319c9e8a203ed96f04ecdd240e1ff3cc366597d2e934bc7193cccdc617",
    "crt_entry_tail": "63de7677bea1f499f4873728ff23758d30e7105b34d41c03b0cb2ab1f9451b2b",
    "bi2_pointer_nonzero": "c28b8de45f708b70a77c7ac1215baa744beec6d0e03ef262acd71bcc7fab001d",
    "bi2_debug_ordinary": "2d8b0bddb967568412b18f4cbc4635921262fba58c2e5521ec55ad2b05176051",
    "bi2_debug4_call_return": "466cbb7c93fc550e9638194dd3420f46de9eb113865be53810aee62c5014dd66",
    "bi2_debug4_leaf": "4371be2d493f25717c1eda105babfa5349c1a4a5df7ea5a0306fd66ec500c987",
    "bi2_offset_check": "28793ac10df819cf92ede9e782c644541d4cae3af884f1b070af39f27822cebd",
    "bi2_no_array": "ed9ee64962d532f8b25ed65e0675ee04a8ea1d113c3790a19492785a1d9444fd",
    "bi2_count_check": "54aa64d30d9050c53ac1ed6e3231e6b361440187f086777124e1cb08c05e38b9",
    "bi2_array_prepare": "9a9638b030ae207fe5851d317f3109c19ccf78c576263c70c63e47671fefb57d",
    "bi2_relocate_item": "780597d22ee2f18fae86a7df5a35a06b43e72e6564ae19ee023cf6278f878ade",
    "bi2_array_publish": "b270bcd3d8a71a3605b866f1b083385e389c859be6cc5a12e3a8278170853766",
    "metadata_call": "d4e6b44034b1a26aaceb681c9a0233b11a0a41af65dde84c6ff31f2e3df5bcb4",
    "metadata_leaf": "97ae2593a4737c5b60a622917800251428202ac3898e018256e965bdd4cc2357",
    "os_call": "324f1899c46bde622f203ce8e8d03fd62802d1d59171fcafc1daa0070fdea857",
    "os_first_guard_frame": "a472740be340eb3962b9e6395f7e88906f84bf480769173d29a474d20edf45d1",
    "clock_frame_call_disable": "ac27ff0f124921e68fe2de4bad55f9f5a544afc3ee03e04ba5303b191cd70f75",
    "clock_sample_call": "973cf7041436c83651ab25940be720c40c758a533a6612811367a1eca07b62cb"
}


def table_cost_reader(path):
    data = Path(path).read_bytes()
    require(sha(data) == TABLE_SHA, "source table identity mismatch")
    text = data.decode()
    tables = {}
    for name in ("s_primary_table", "s_table4", "s_table19", "s_table31", "s_table63"):
        m = re.search(r"constexpr std::array<GekkoOPTemplate, \d+> "+name+r"\{\{(.*?)\}\};", text, re.S)
        require(m is not None, "missing source metadata table")
        rows = re.findall(r'\{(\d+),\s*"[^"]+",\s*OpType::\w+,\s*(\d+),', m[1])
        tables[name] = {int(op): int(cost) for op, cost in rows}
        require(len(tables[name]) == len(rows), "duplicate source table opcode")

    def cost(word):
        op = word >> 26
        table, key = (tables[f"s_table{op}"], (word >> 1) & 1023) if op in (4, 19, 31, 63) else (tables["s_primary_table"], op)
        require(key in table and table[key] > 0, "unknown annotation cost opcode")
        return table[key]
    return cost


def unit_digest(dol, pcs):
    return sha(b"".join(struct.pack(">II", pc, dol.word(pc)) for pc in pcs))


def proof_units(dol, cost_of):
    require(set(UNIT_PINS) == set(UNITS), "unsealed semantic work units")
    result = {}
    for name, pcs in UNITS.items():
        require(unit_digest(dol, pcs) == UNIT_PINS[name], "semantic work raw gate changed: "+name)
        result[name] = {"operations": len(pcs), "work": sum(cost_of(dol.word(pc)) for pc in pcs), "raw_sha256": UNIT_PINS[name]}
    require(dol.word(0x80379628) == 0x7C6D42E6, "first unconsumed clock word changed")
    return result


def overlap(a, n, b, m):
    return a < b+m and b < a+n


def admitted_input(dol, initial):
    for key in ("msr", "hid0", "hid2", "l2cr", "fpscr"):
        require(type(initial.get(key)) is int and 0 <= initial[key] <= 0xFFFFFFFF, "unknown/nonword entry input: "+key)
    require(initial["msr"] & 0x4CF01 == 0, "native mode unresolved at80003400")
    require(initial["hid0"] & ~0x0011C064 == 0 and initial["hid0"] & 0xC000 == 0xC000, "native cache input unresolved at entry")
    require(initial["hid2"] & ~0xE0000000 == 0, "native HID2 DMA/locked/reserved owner unresolved")
    require(initial["l2cr"] & ~0xC0480000 == 0, "native pending/test/reserved L2 work unresolved at80372894")
    require(initial["fpscr"] & 0x800 == 0, "unadmitted reserved FPSCR at80370CDC")
    source = initial.get("fpr_source")
    require(isinstance(source, bytes) and len(source) == 16, "unknown FPR source owner at80370CF8")
    scalar, lane0, lane1 = struct.unpack(">QII", source)
    require((scalar >> 52) & 0x7FF != 0x7FF, "unadmitted scalar nonfinite at80370D7C")
    for lane in (lane0, lane1):
        exponent, fraction = (lane >> 23) & 0xFF, lane & 0x7FFFFF
        require(exponent != 0xFF and (exponent != 0 or fraction == 0), "unadmitted paired source at80370CF8")
    require(initial["old_handler"] is not None, "unknown handler owner at803733B4")
    require(initial["pointer"] is not None, "unknown BI2 pointer owner at80003188")
    require(type(initial["old_handler"]) is int and 0 <= initial["old_handler"] <= 0xFFFFFFFF, "nonword handler owner")
    require(type(initial["pointer"]) is int and 0 <= initial["pointer"] <= 0xFFFFFFFF, "nonword BI2 pointer owner")
    base = initial["pointer"]
    require(base != 0, "null BI2 requires unknown arena input at800031A4")
    require(base % 4 == 0 and 0x80000000 <= base and base+0x2000 <= 0x81800000, "BI2 mapping unproved")
    blob = initial["bi2"]
    require(len(blob) == 0x2000, "unknown or partial BI2 blob at80003194")
    for address, _, size in dol.sections:
        require(not overlap(base, len(blob), address, size), "BI2 aliases immutable DOL")
    for address, size in ((0x80000000, 0x3100), (0x8060C570, 0x90), (0x8056FE00, 0x74700), (0x805EF020, 0x375C), (0x805FC540, 0xAC)):
        require(not overlap(base, len(blob), address, size), "BI2 aliases closed prefix owner")
    w = lambda off: struct.unpack_from(">I", blob, off)[0]
    debug, offset = w(12), w(8)
    require(debug not in (2, 3), "unknown debug context transfer at800031F4")
    count = 0
    if offset:
        require(offset % 4 == 0 and offset+4 <= len(blob) and base+offset <= 0xFFFFFFFF, "BI2 offset read unknown at8000321C")
        count = w(offset)
        require(offset+4+4*count <= len(blob), "BI2 relocation source extent unknown at80003234")
    return {"l2_enabled": bool(initial["l2cr"] & 0x80000000), "debug": debug, "offset": offset, "count": count}


def produce_work(dol, units, initial):
    path = admitted_input(dol, initial)
    # Actual immutable copy/zero descriptor data, not trace iteration counts.
    for n, expected in enumerate(COPY_DESCRIPTORS):
        require(tuple(dol.word(0x80005544+12*n+4*i) for i in range(3)) == expected, "copy descriptor raw gate changed")
    for n, expected in enumerate(ZERO_DESCRIPTORS):
        require(tuple(dol.word(0x800055C8+8*n+4*i) for i in range(2)) == expected, "zero descriptor raw gate changed")
    copies = 0
    for n in range(11):
        source, dest, size = [dol.word(0x80005544+12*n+4*i) for i in range(3)]
        if not size:
            require(n == 10 and source == dest == 0, "copy descriptor terminator changed")
            break
        require(source == dest, "unimplemented CRT copy/cache path at80003388")
        copies += 1
    zeros = []
    for n in range(4):
        address, size = dol.word(0x800055C8+8*n), dol.word(0x800055CC+8*n)
        if not size:
            require(n == 3 and address == 0, "zero descriptor terminator changed")
            break
        require(address % 4 == 0 and size % 4 == 0 and size >= 32, "unimplemented zero alignment/short/byte path")
        zeros.append((address, size))
    ledger = []
    def add(name, times=1, reason="fixed reached semantic unit"):
        require(times >= 0, "negative semantic work count")
        u = units[name]
        ledger.append({"unit": name, "times": times, "operations": u["operations"]*times, "work": u["work"]*times, "reason": reason})

    for name in ("entry_register_call", "register_seed", "hardware_call", "hardware_prepare", "paired_frame", "hid2_read",
                 "paired_enable_call", "hid2_write", "icfi_call", "icfi_leaf", "sync_gqr_return", "fpr_call", "fpr_seed", "cache_call", "cache_first_check"):
        add(name)
    add("hid0_read")
    add("cache_second_check")
    add("hid0_read")
    add("l2_check")
    add("l2_read")
    if not path["l2_enabled"]:
        add("l2_disabled_caller", reason="initial E=0; owned synchronous completion gives one clear visit at each poll")
        add("msr_read")
        add("msr_write", 2)
        add("l2_read", 2)
        add("l2_write", 2)
        add("l2_invalidate_body")
        add("l2_read", 5, "five statically identified helper reads; no retry")
        add("l2_write", 3, "three helper command stores")
        add("logger", reason="caller clears CR bit6; eight FPR saves skipped")
    add("handler_arguments")
    for name in ("handler_call", "handler_frame", "disable_ee", "handler_select", "handler_restore_call", "restore_ee_zero", "handler_return",
                 "handler_logger_call", "logger", "cache_return", "hardware_return", "sentinel_walker_call", "descriptor_frame"):
        add(name)
    add("identity_copy_descriptor", copies, "actual immutable source==destination descriptors")
    add("copy_terminator")
    add("zero_table_prepare")
    for address, size in zeros:
        reason = f"immutable zero descriptor{address:08X}/size{size:X}"
        add("zero_descriptor", reason=reason)
        add("zero_wrapper", reason=reason)
        add("zero_leaf_fixed", reason=reason)
        add("zero_group", size >> 5, reason)
        add("zero_remaining_word", (size >> 2) & 7, reason)
    for name in ("zero_terminator", "descriptor_return", "crt_entry_tail", "bi2_pointer_nonzero", "bi2_debug_ordinary"):
        add(name)
    if path["debug"] == 4:
        add("bi2_debug4_call_return", reason="already loaded opaque debug value equals4")
        add("bi2_debug4_leaf")
    add("bi2_offset_check")
    if path["offset"]:
        add("bi2_count_check", reason="owned nonzero offset")
    if path["count"]:
        add("bi2_array_prepare")
        add("bi2_relocate_item", path["count"], "count loaded before any same-owner relocation writes")
        add("bi2_array_publish")
    else:
        add("bi2_no_array", reason="zero offset or actual zero count")
    for name in ("metadata_call", "metadata_leaf", "os_call", "os_first_guard_frame", "clock_frame_call_disable", "disable_ee", "clock_sample_call"):
        add(name)
    return {"input_path": path, "copy_descriptors": copies, "zero_descriptors": len(zeros),
            "operations": sum(x["operations"] for x in ledger), "original_operation_work": sum(x["work"] for x in ledger),
            "ledger": ledger, "stop": "before80379628", "connected_clock_admitted": False,
            "contract": "Finite semantic work annotations; event/scheduler owners and source mode must be independently admitted before work becomes elapsed clock"}


def synthetic_initial(l2cr=0, debug=0, offset=0, count=0):
    blob = bytearray(0x2000)
    struct.pack_into(">II", blob, 8, offset, debug)
    if offset:
        require(offset % 4 == 0 and offset+4 <= len(blob), "synthetic input offset invalid")
        struct.pack_into(">I", blob, offset, count)
    return {"msr": 0x2032, "hid0": 0x11C064, "hid2": 0xE0000000, "l2cr": l2cr,
            "fpscr": 0, "fpr_source": bytes(16), "old_handler": 0, "pointer": 0x817E54E0, "bi2": bytes(blob)}


def adversarial_checks(dol, units, cost_of):
    """Raw mutations and input falsifications; no reference totals available."""
    rejected = 0
    def must_decline(action):
        nonlocal rejected
        try:
            action()
        except (ValueError, TypeError, KeyError):
            rejected += 1
        else:
            raise ValueError("mandatory native work falsification unexpectedly admitted")
    class ChangedWord:
        def __init__(self, address):
            self.address, self.sections = address, dol.sections
        def word(self, address):
            return dol.word(address) ^ (1 if address == self.address else 0)
    for pcs in UNITS.values():
        must_decline(lambda pcs=pcs: proof_units(ChangedWord(pcs[0]), cost_of))
    must_decline(lambda: proof_units(ChangedWord(0x80379628), cost_of))
    for pc in range(0x80005544, 0x800055E8, 4):
        must_decline(lambda pc=pc: produce_work(ChangedWord(pc), units, synthetic_initial()))
    for key, bad in (("msr", None), ("msr", 0x8000), ("msr", 0x40000), ("msr", 1),
                     ("hid0", 0), ("hid0", 0x11C464), ("hid2", 1), ("hid2", 0xE1000000),
                     ("l2cr", 1), ("l2cr", 0x200000), ("l2cr", 0x01000000),
                     ("old_handler", None), ("pointer", None), ("pointer", 0), ("pointer", 0x817E54E1),
                     ("pointer", 0x817FF000), ("pointer", 0x80003100), ("pointer", 0x8056FE00),
                     ("pointer", 0x8060C570), ("pointer", 0x80000000), ("bi2", b""), ("bi2", bytes(0x1FFC)),
                     ("fpscr", 0x800), ("fpr_source", bytes(15)),
                     ("fpr_source", struct.pack(">QII", 0x7FF0000000000000, 0, 0)),
                     ("fpr_source", struct.pack(">QII", 0, 1, 0)),
                     ("fpr_source", struct.pack(">QII", 0, 0, 0x7F800000))):
        value = synthetic_initial()
        value[key] = bad
        must_decline(lambda value=value: produce_work(dol, units, value))
    for debug in (2, 3):
        must_decline(lambda debug=debug: produce_work(dol, units, synthetic_initial(debug=debug)))
    for offset, count in ((1, 0), (0x2000, 0), (0xFFFFFFFC, 0), (0x1FFC, 1), (0x100, 0xFFFFFFFF)):
        value = synthetic_initial()
        blob = bytearray(value["bi2"])
        struct.pack_into(">I", blob, 8, offset)
        if offset < 0x2000 and not offset & 3:
            struct.pack_into(">I", blob, offset, count)
        value["bi2"] = bytes(blob)
        must_decline(lambda value=value: produce_work(dol, units, value))
    checked = 0
    for passive in (0, 0x40000000, 0x00400000, 0x00080000, 0x40480000):
        for enabled in (0, 0x80000000):
            for debug in (0, 1, 4, 0xFFFFFFFF):
                for offset, count in ((0, 0), (0x100, 0), (0x100, 1), (0x100, 7), (0x100, 1983)):
                    result = produce_work(dol, units, synthetic_initial(passive|enabled, debug, offset, count))
                    extra = 0 if offset == 0 else 4 if count == 0 else 10+5*count
                    extra_operations = 0 if offset == 0 else 4 if count == 0 else 9+5*count
                    expected = 153636+494+(116 if enabled == 0 else 0)+(6 if debug == 4 else 0)+extra
                    require(result["original_operation_work"] == expected, "independent closed form disagrees with semantic unit sum")
                    require(result["operations"] == 154108+(98 if enabled == 0 else 0)+(6 if debug == 4 else 0)+extra_operations,
                            "finite operation cardinality disagrees with semantic unit sum")
                    checked += 1
    # Metadata and count may alias. Predictions use bytes at their original
    # consumption, never the requested command-line debug/count labels.
    alias = produce_work(dol, units, synthetic_initial(0x80000000, debug=0, offset=12, count=4))
    require(alias["input_path"]["debug"] == 4 and alias["input_path"]["count"] == 4
            and alias["original_operation_work"] == 154130+6+10+20, "same-owner BI2 alias lost")
    return {"raw_and_input_mutations_rejected": rejected, "closed_form_paths_checked": checked,
            "same_owner_alias_checks": 1, "reference_or_output_totals_used": False}


def initial_from_capture(path):
    capture = json.loads(Path(path).read_text())
    initial = capture["checkpoints"][0]
    require(initial["pc"].lower() == "80003154", "capture lacks true prefix inputs")
    return capture, {k: int(initial[k], 16) for k in ("msr", "hid0", "hid2", "l2cr")} | {
        "fpscr": int(initial["fpscr"], 16), "fpr_source": bytes.fromhex(initial["fpr_source"]),
        "old_handler": int(initial["handler_slot"], 16), "pointer": int(initial["bi2_pointer"], 16), "bi2": bytes.fromhex(initial["bi2_blob"])}


def reference_falsification(capture_path, capture, result, dol, cost_of):
    # This validation is strictly after the producer has finished. No observed
    # operation list/cycle count selects a producer unit, path or loop count.
    trace = Path(capture_path).parent/capture["instruction_trace"]
    require(sha(trace.read_bytes()) == capture["instruction_trace_sha256"], "reference stream changed")
    region = capture["region_extension"]
    require(region["sampler_mode"] == "continuous" and region["midchain_writes"] is False and region["clock_plan"] == [],
            "reference is not unforced original continuous scheduling")
    require(int(capture["checkpoints"][0]["clock_producer"]["exceptions"], 16) == 0,
            "reference entry pending exception owner unresolved")
    require(result["stop"] == "before80379628" and result["connected_clock_admitted"] is False,
            "work result claims an unproved clock frontier")
    expected = Counter()
    for item in result["ledger"]:
        pcs = UNITS[item["unit"]]
        require(item["operations"] == len(pcs)*item["times"]
                and item["work"] == sum(cost_of(dol.word(pc)) for pc in pcs)*item["times"],
                "reported semantic unit is inconsistent with raw proof")
        for pc in pcs:
            expected[pc] += item["times"]
    actual = Counter()
    count = cost = 0
    source_entry = int(capture["checkpoints"][0]["clock_producer"]["cycles"], 16)
    phase, before, entered, last_exit = "hook", None, None, None
    for sequence, line in enumerate(trace.open(), 1):
        p = line.split()
        require(len(p) == 15, "reference stream malformed")
        require(int(p[0]) == sequence and p[1] == phase and p[12] == "3f800000" and int(p[14], 16) == 0,
                "reference ordering/overclock/pending exception changed")
        if p[1] == "hook" and p[2].lower() == "80379628":
            require(last_exit is not None and last_exit[2] == "80379668" and last_exit[5] == "00000001"
                    and last_exit[6] == "80379628", "first-clock caller block not retired")
            observed_elapsed = int(p[8], 16)-source_entry
            break
        if phase == "hook":
            require(p[2] == p[6] and int(p[5], 16) == 0, "active HLE hook or bad original PC")
            before, phase = p, "enter"
        if p[1] == "enter":
            pc, word, work = int(p[2], 16), int(p[3], 16), int(p[4], 16)
            require(p[2] == before[2] and p[8:15] == before[8:15] and int(p[5], 16) == 0,
                    "active HLE replacement or altered enter state")
            require(dol.word(pc) == word and cost_of(word) == work, "reference original word/cost changed")
            actual[pc] += 1
            count += 1
            cost += work
            entered, phase = p, "exit"
        elif p[1] == "exit":
            require(p[2:5] == entered[2:5] and p[8:13] == entered[8:13] and int(p[5], 16) in (0, 1),
                    "reference exit identity/timing changed")
            last_exit, phase = p, "hook"
    else:
        raise ValueError("reference lacks first-clock boundary")
    require(actual == expected and result["operations"] == count and result["original_operation_work"] == cost == observed_elapsed,
            "finite work prediction falsified by independent source output")
    return {"observed_operations": count, "observed_cost": cost, "observed_elapsed": observed_elapsed,
            "matched_original_pc_multiplicities": len(actual), "used_only_after_prediction": True}


def reference_mutation_checks(capture_path, capture, result, dol, cost_of):
    rejected = 0
    def decline(c, r):
        nonlocal rejected
        try:
            reference_falsification(capture_path, c, r, dol, cost_of)
        except (ValueError, KeyError):
            rejected += 1
        else:
            raise ValueError("mutated work/reference output admitted")
    for key in ("operations", "original_operation_work"):
        changed = copy.deepcopy(result)
        changed[key] += 1
        decline(capture, changed)
    for key in ("times", "operations", "work"):
        changed = copy.deepcopy(result)
        changed["ledger"][0][key] += 1
        decline(capture, changed)
    changed = copy.deepcopy(result)
    changed["stop"] = "after80379628"
    decline(capture, changed)
    changed = copy.deepcopy(result)
    changed["connected_clock_admitted"] = True
    decline(capture, changed)
    changed = copy.deepcopy(capture)
    changed["checkpoints"][0]["clock_producer"]["cycles"] = f"{int(changed['checkpoints'][0]['clock_producer']['cycles'], 16)+1:016X}"
    decline(changed, result)
    changed = copy.deepcopy(capture)
    changed["checkpoints"][0]["clock_producer"]["exceptions"] = "00000001"
    decline(changed, result)
    for key, value in (("sampler_mode", "single_step"), ("midchain_writes", True), ("clock_plan", [{"raw_tb": "0000000000000000"}])):
        changed = copy.deepcopy(capture)
        changed["region_extension"][key] = value
        decline(changed, result)
    return rejected


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dol", type=Path, required=True, help="read-only original PAL DOL")
    p.add_argument("--source-tables", type=Path, required=True, help="matching read-only PPCTables.cpp")
    p.add_argument("--capture", type=Path, action="append", default=[])
    p.add_argument("--l2cr", type=lambda s:int(s, 16), default=0)
    p.add_argument("--debug", type=lambda s:int(s, 16), default=0)
    p.add_argument("--offset", type=lambda s:int(s, 16), default=0)
    p.add_argument("--count", type=lambda s:int(s, 16), default=0)
    p.add_argument("--print-gates", action="store_true", help="one-time static gate generation; does not produce/admit work")
    p.add_argument("--self-test", action="store_true", help="run independent raw/input mutations without a trace")
    p.add_argument("--compact", action="store_true", help="omit per-unit and per-run ledgers from display")
    args = p.parse_args()
    dol = Dol(args.dol)
    if args.print_gates:
        print(json.dumps({name:unit_digest(dol, pcs) for name, pcs in UNITS.items()}, indent=2))
        return
    cost_of = table_cost_reader(args.source_tables)
    units = proof_units(dol, cost_of)
    runs = []
    reference_rejections = 0
    for cap_path in args.capture:
        capture, initial = initial_from_capture(cap_path)
        result = produce_work(dol, units, initial)
        result["reference_falsification"] = reference_falsification(cap_path, capture, result, dol, cost_of)
        if args.self_test:
            reference_rejections += reference_mutation_checks(cap_path, capture, result, dol, cost_of)
        runs.append(result)
    if not args.capture:
        initial = synthetic_initial(args.l2cr, args.debug, args.offset, args.count)
        runs.append(produce_work(dol, units, initial))
    checks = adversarial_checks(dol, units, cost_of) if args.self_test else None
    if checks is not None:
        checks["reference_mutations_rejected_after_prediction"] = reference_rejections
    if args.compact:
        runs = [{k:v for k,v in run.items() if k != "ledger"} for run in runs]
    print(json.dumps({"scope":"independent finite semantic work candidate; not clock admission",
                      "proof_units": len(units) if args.compact else units, "runs":runs, "checks": checks}, indent=2))


if __name__ == "__main__":
    main()
