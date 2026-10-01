"""Gekko (PowerPC 750CL) instruction decoder.

Written because Capstone 5's paired-single mode mis-decodes `fcmpo` and stops
disassembling.  Produces structured operands suited for dataflow analysis:

    ins.m    mnemonic (simplified forms: li, lis, mr, blr, bctrl, beq, ...)
    ins.ops  tuple of operands:
             ('r', n)          GPR
             ('f', n)          FPR
             ('i', v)          immediate (signed where the ISA says so)
             ('m', disp, rA)   memory operand disp(rA)   (rA==0 -> literal 0)
             ('x', rA, rB)     indexed memory operand rA+rB
             ('t', addr)       branch target
             ('c', n)          condition register field crN
             ('s', n)          SPR number
    ins.rd / ins.ra / ins.rb raw fields, ins.imm decoded immediate
    ins.kind one of: 'load','store','branch','call','ret','alu','fp','ps','cmp','sys','other'

Unknown encodings decode to mnemonic '.word' (never guessed).
"""

class Ins:
    __slots__ = ('addr', 'w', 'm', 'ops', 'kind', 'rd', 'ra', 'rb', 'imm',
                 'target', 'size', 'fp', 'upd', 'lk', 'bo', 'bi')

    def __init__(self, addr, w):
        self.addr = addr
        self.w = w
        self.m = '.word'
        self.ops = ()
        self.kind = 'other'
        self.rd = (w >> 21) & 31
        self.ra = (w >> 16) & 31
        self.rb = (w >> 11) & 31
        self.imm = None
        self.target = None
        self.size = 0        # memory access width
        self.fp = False      # memory access to/from FPR
        self.upd = False     # update-form (rA written)
        self.lk = False
        self.bo = None
        self.bi = None

    def __repr__(self):
        return '%08X  %s' % (self.addr, self.text())

    def text(self):
        return (self.m + ' ' + ', '.join(fmt_op(o) for o in self.ops)).rstrip()


SPR_NAMES = {1: 'xer', 8: 'lr', 9: 'ctr', 18: 'dsisr', 19: 'dar', 22: 'dec',
             25: 'sdr1', 26: 'srr0', 27: 'srr1', 272: 'sprg0', 273: 'sprg1',
             274: 'sprg2', 275: 'sprg3', 268: 'tbl', 269: 'tbu', 287: 'pvr',
             912: 'gqr0', 913: 'gqr1', 914: 'gqr2', 915: 'gqr3', 916: 'gqr4',
             917: 'gqr5', 918: 'gqr6', 919: 'gqr7', 920: 'hid2', 921: 'wpar',
             922: 'dma_u', 923: 'dma_l', 1008: 'hid0', 1009: 'hid1',
             1017: 'l2cr', 1019: 'ictc', 1020: 'thrm1', 1021: 'thrm2',
             1022: 'thrm3', 936: 'ummcr0', 952: 'mmcr0', 956: 'mmcr1'}


def fmt_op(o):
    t = o[0]
    if t == 'r':
        return 'r%d' % o[1]
    if t == 'f':
        return 'f%d' % o[1]
    if t == 'i':
        v = o[1]
        return ('-0x%X' % -v) if v < 0 else ('0x%X' % v)
    if t == 'm':
        d = o[1]
        ds = ('-0x%X' % -d) if d < 0 else ('0x%X' % d)
        return '%s(r%d)' % (ds, o[2])
    if t == 'x':
        return 'r%d, r%d' % (o[1], o[2])
    if t == 't':
        return '0x%08X' % o[1]
    if t == 'c':
        return 'cr%d' % o[1]
    if t == 's':
        return SPR_NAMES.get(o[1], 'spr%d' % o[1])
    if t == 'b':
        return 'crb%d' % o[1]
    return str(o)


def sx16(v):
    return v - 0x10000 if v & 0x8000 else v


def sx12(v):
    return v - 0x1000 if v & 0x800 else v


R = lambda n: ('r', n)
F = lambda n: ('f', n)
I = lambda v: ('i', v)
C = lambda n: ('c', n)

# D-form loads/stores: op -> (mnemonic, size, fp, update, is_store)
DFORM_MEM = {
    32: ('lwz', 4, False, False, False), 33: ('lwzu', 4, False, True, False),
    34: ('lbz', 1, False, False, False), 35: ('lbzu', 1, False, True, False),
    36: ('stw', 4, False, False, True), 37: ('stwu', 4, False, True, True),
    38: ('stb', 1, False, False, True), 39: ('stbu', 1, False, True, True),
    40: ('lhz', 2, False, False, False), 41: ('lhzu', 2, False, True, False),
    42: ('lha', 2, False, False, False), 43: ('lhau', 2, False, True, False),
    44: ('sth', 2, False, False, True), 45: ('sthu', 2, False, True, True),
    46: ('lmw', 4, False, False, False), 47: ('stmw', 4, False, False, True),
    48: ('lfs', 4, True, False, False), 49: ('lfsu', 4, True, True, False),
    50: ('lfd', 8, True, False, False), 51: ('lfdu', 8, True, True, False),
    52: ('stfs', 4, True, False, True), 53: ('stfsu', 4, True, True, True),
    54: ('stfd', 8, True, False, True), 55: ('stfdu', 8, True, True, True),
}

# X-form (op 31) indexed loads/stores: xo -> (mnemonic, size, fp, update, store)
XFORM_MEM = {
    23: ('lwzx', 4, False, False, False), 55: ('lwzux', 4, False, True, False),
    87: ('lbzx', 1, False, False, False), 119: ('lbzux', 1, False, True, False),
    151: ('stwx', 4, False, False, True), 183: ('stwux', 4, False, True, True),
    215: ('stbx', 1, False, False, True), 247: ('stbux', 1, False, True, True),
    279: ('lhzx', 2, False, False, False), 311: ('lhzux', 2, False, True, False),
    343: ('lhax', 2, False, False, False), 375: ('lhaux', 2, False, True, False),
    407: ('sthx', 2, False, False, True), 439: ('sthux', 2, False, True, True),
    534: ('lwbrx', 4, False, False, False), 662: ('stwbrx', 4, False, False, True),
    790: ('lhbrx', 2, False, False, False), 918: ('sthbrx', 2, False, False, True),
    535: ('lfsx', 4, True, False, False), 567: ('lfsux', 4, True, True, False),
    599: ('lfdx', 8, True, False, False), 631: ('lfdux', 8, True, True, False),
    663: ('stfsx', 4, True, False, True), 695: ('stfsux', 4, True, True, True),
    727: ('stfdx', 8, True, False, True), 759: ('stfdux', 8, True, True, True),
    983: ('stfiwx', 4, True, False, True), 20: ('lwarx', 4, False, False, False),
    150: ('stwcx.', 4, False, False, True),
}

# op 31 XO-form arithmetic with OE bit (9-bit xo) -> (mnemonic, has_rB)
XO_ARITH = {266: ('add', 1), 10: ('addc', 1), 138: ('adde', 1),
            234: ('addme', 0), 202: ('addze', 0), 491: ('divw', 1),
            459: ('divwu', 1), 235: ('mullw', 1), 104: ('neg', 0),
            40: ('subf', 1), 8: ('subfc', 1), 136: ('subfe', 1),
            232: ('subfme', 0), 200: ('subfze', 0), 75: ('mulhw', 1),
            11: ('mulhwu', 1)}

# op 31 X-form logical (rA <- rS op rB)
X_LOGIC = {28: 'and', 60: 'andc', 124: 'nor', 284: 'eqv', 316: 'xor',
           412: 'orc', 444: 'or', 476: 'nand', 24: 'slw', 536: 'srw',
           792: 'sraw'}
X_UNARY = {26: 'cntlzw', 922: 'extsh', 954: 'extsb'}
X_CACHE = {54: 'dcbst', 86: 'dcbf', 246: 'dcbtst', 278: 'dcbt', 470: 'dcbi',
           982: 'icbi', 1014: 'dcbz'}
X_SYS0 = {598: 'sync', 854: 'eieio', 566: 'tlbsync'}

OP59 = {18: 'fdivs', 20: 'fsubs', 21: 'fadds', 22: 'fsqrts', 24: 'fres',
        25: 'fmuls', 28: 'fmsubs', 29: 'fmadds', 30: 'fnmsubs', 31: 'fnmadds'}
OP63_A = {18: 'fdiv', 20: 'fsub', 21: 'fadd', 22: 'fsqrt', 23: 'fsel',
          25: 'fmul', 26: 'frsqrte', 28: 'fmsub', 29: 'fmadd', 30: 'fnmsub',
          31: 'fnmadd'}
OP63_X = {12: 'frsp', 14: 'fctiw', 15: 'fctiwz', 40: 'fneg', 72: 'fmr',
          136: 'fnabs', 264: 'fabs'}
PS_A = {10: 'ps_sum0', 11: 'ps_sum1', 12: 'ps_muls0', 13: 'ps_muls1',
        14: 'ps_madds0', 15: 'ps_madds1', 18: 'ps_div', 20: 'ps_sub',
        21: 'ps_add', 23: 'ps_sel', 24: 'ps_res', 25: 'ps_mul',
        26: 'ps_rsqrte', 28: 'ps_msub', 29: 'ps_madd', 30: 'ps_nmsub',
        31: 'ps_nmadd'}
PS_X = {40: 'ps_neg', 72: 'ps_mr', 136: 'ps_nabs', 264: 'ps_abs',
        528: 'ps_merge00', 560: 'ps_merge01', 592: 'ps_merge10',
        624: 'ps_merge11'}

COND_T = {0: 'lt', 1: 'gt', 2: 'eq', 3: 'so'}
COND_F = {0: 'ge', 1: 'le', 2: 'ne', 3: 'ns'}


def _bc_name(bo, bi):
    """Simplified mnemonic stem for a conditional branch (None = raw bc)."""
    b = bo & 0x1E
    if (b & 0x14) == 0x14:
        return ''            # always
    if (b & 0x14) == 0x04:   # CR test only
        return 'b' + (COND_T if (b & 0x08) else COND_F)[bi & 3]
    if (b & 0x14) == 0x10:   # CTR only
        return 'bdz' if (b & 0x02) else 'bdnz'
    return None


def decode(w, addr):
    ins = Ins(addr, w)
    op = w >> 26
    rd, ra, rb = ins.rd, ins.ra, ins.rb
    simm = sx16(w & 0xFFFF)
    uimm = w & 0xFFFF

    if op in DFORM_MEM:
        m, size, fp, upd, st = DFORM_MEM[op]
        ins.m = m
        ins.imm = simm
        ins.ops = ((F(rd) if fp else R(rd)), ('m', simm, ra))
        ins.kind = 'store' if st else 'load'
        ins.size, ins.fp, ins.upd = size, fp, upd
        return ins

    if op == 14:
        ins.imm = simm
        ins.kind = 'alu'
        if ra == 0:
            ins.m, ins.ops = 'li', (R(rd), I(simm))
        else:
            ins.m, ins.ops = 'addi', (R(rd), R(ra), I(simm))
        return ins
    if op == 15:
        ins.imm = simm
        ins.kind = 'alu'
        if ra == 0:
            ins.m, ins.ops = 'lis', (R(rd), I(simm))
        else:
            ins.m, ins.ops = 'addis', (R(rd), R(ra), I(simm))
        return ins
    if op in (24, 25, 26, 27, 28, 29):
        names = {24: 'ori', 25: 'oris', 26: 'xori', 27: 'xoris', 28: 'andi.', 29: 'andis.'}
        ins.imm = uimm
        ins.kind = 'alu'
        if op == 24 and w == 0x60000000:
            ins.m = 'nop'
            return ins
        ins.m = names[op]
        ins.ops = (R(ra), R(rd), I(uimm))   # rA <- rS op imm
        return ins
    if op in (7, 8, 12, 13):
        names = {7: 'mulli', 8: 'subfic', 12: 'addic', 13: 'addic.'}
        ins.m, ins.imm, ins.kind = names[op], simm, 'alu'
        ins.ops = (R(rd), R(ra), I(simm))
        return ins
    if op in (10, 11):
        crf = rd >> 2
        if op == 11:
            ins.m, ins.imm = 'cmpwi', simm
        else:
            ins.m, ins.imm = 'cmplwi', uimm
        ins.kind = 'cmp'
        ins.ops = (C(crf), R(ra), I(ins.imm))
        return ins
    if op in (20, 21, 23):
        sh, mb, me = rb, (w >> 6) & 31, (w >> 1) & 31
        rc = '.' if (w & 1) else ''
        ins.kind = 'alu'
        if op == 21:
            ins.m = 'rlwinm' + rc
            ins.ops = (R(ra), R(rd), I(sh), I(mb), I(me))
            # common simplified forms
            if sh == 0 and me == 31:
                ins.m, ins.ops = 'clrlwi' + rc, (R(ra), R(rd), I(mb))
            elif mb == 0 and me == 31 - sh:
                ins.m, ins.ops = 'slwi' + rc, (R(ra), R(rd), I(sh))
            elif me == 31 and sh == (32 - mb) % 32 and mb != 0:
                ins.m, ins.ops = 'srwi' + rc, (R(ra), R(rd), I(mb))
        elif op == 20:
            ins.m = 'rlwimi' + rc
            ins.ops = (R(ra), R(rd), I(sh), I(mb), I(me))
        else:
            ins.m = 'rlwnm' + rc
            ins.ops = (R(ra), R(rd), R(rb), I(mb), I(me))
        return ins
    if op == 18:
        li = w & 0x03FFFFFC
        if li & 0x02000000:
            li -= 0x04000000
        aa, lk = (w >> 1) & 1, w & 1
        tgt = (li if aa else addr + li) & 0xFFFFFFFF
        ins.target = tgt
        ins.lk = bool(lk)
        ins.m = 'bl' if lk else 'b'
        if aa:
            ins.m += 'a'
        ins.ops = (('t', tgt),)
        ins.kind = 'call' if lk else 'branch'
        return ins
    if op == 16:
        bo, bi = rd, ra
        bd = sx16(w & 0xFFFC)
        aa, lk = (w >> 1) & 1, w & 1
        tgt = (bd if aa else addr + bd) & 0xFFFFFFFF
        ins.target, ins.lk, ins.bo, ins.bi = tgt, bool(lk), bo, bi
        stem = _bc_name(bo, bi)
        if stem is None:
            ins.m = 'bc' + ('l' if lk else '')
            ins.ops = (I(bo), I(bi), ('t', tgt))
        else:
            ins.m = (stem or 'b') + ('l' if lk else '')
            crf = bi >> 2
            if stem and stem not in ('bdnz', 'bdz') and crf != 0:
                ins.ops = (C(crf), ('t', tgt))
            else:
                ins.ops = (('t', tgt),)
        ins.kind = 'call' if lk else 'branch'
        return ins
    if op == 19:
        xo = (w >> 1) & 0x3FF
        lk = w & 1
        if xo in (16, 528):
            bo, bi = rd, ra
            ins.bo, ins.bi, ins.lk = bo, bi, bool(lk)
            reg = 'lr' if xo == 16 else 'ctr'
            stem = _bc_name(bo, bi)
            if stem is None:
                ins.m = ('bclr' if xo == 16 else 'bcctr') + ('l' if lk else '')
                ins.ops = (I(bo), I(bi))
            else:
                ins.m = (stem or 'b') + reg + ('l' if lk else '')
                if stem and (bi >> 2):
                    ins.ops = (C(bi >> 2),)
            if lk:
                ins.kind = 'call'
            elif xo == 16:
                ins.kind = 'ret'
            else:
                ins.kind = 'branch'
            return ins
        crops = {257: 'crand', 449: 'cror', 193: 'crxor', 225: 'crnand',
                 33: 'crnor', 289: 'creqv', 129: 'crandc', 417: 'crorc'}
        if xo in crops:
            ins.m = crops[xo]
            ins.ops = (('b', rd), ('b', ra), ('b', rb))
            ins.kind = 'other'
            return ins
        if xo == 0:
            ins.m, ins.ops = 'mcrf', (C(rd >> 2), C(ra >> 2))
            return ins
        if xo == 150:
            ins.m, ins.kind = 'isync', 'sys'
            return ins
        if xo == 50:
            ins.m, ins.kind = 'rfi', 'ret'
            return ins
        return ins
    if op == 17:
        if w == 0x44000002:
            ins.m, ins.kind = 'sc', 'sys'
        return ins
    if op == 3:
        ins.m, ins.ops, ins.kind = 'twi', (I(rd), R(ra), I(simm)), 'sys'
        return ins
    if op == 31:
        xo = (w >> 1) & 0x3FF
        rc = '.' if (w & 1) else ''
        if xo in XFORM_MEM:
            m, size, fp, upd, st = XFORM_MEM[xo]
            ins.m = m
            ins.ops = ((F(rd) if fp else R(rd)), ('x', ra, rb))
            ins.kind = 'store' if st else 'load'
            ins.size, ins.fp, ins.upd = size, fp, upd
            return ins
        xo9 = xo & 0x1FF
        if xo9 in XO_ARITH and not (xo9 in (75, 11) and xo & 0x200):
            m, hasb = XO_ARITH[xo9]
            oe = 'o' if (xo & 0x200) else ''
            ins.m = m + oe + rc
            ins.ops = (R(rd), R(ra), R(rb)) if hasb else (R(rd), R(ra))
            ins.kind = 'alu'
            if m == 'subf' and not oe:
                pass
            return ins
        if xo in X_LOGIC:
            m = X_LOGIC[xo]
            ins.kind = 'alu'
            if m == 'or' and rd == rb:
                ins.m, ins.ops = 'mr' + rc, (R(ra), R(rd))
            elif m == 'nor' and rd == rb:
                ins.m, ins.ops = 'not' + rc, (R(ra), R(rd))
            else:
                ins.m, ins.ops = m + rc, (R(ra), R(rd), R(rb))
            return ins
        if xo in X_UNARY:
            ins.m, ins.ops, ins.kind = X_UNARY[xo] + rc, (R(ra), R(rd)), 'alu'
            return ins
        if xo == 824:
            ins.m, ins.ops, ins.kind = 'srawi' + rc, (R(ra), R(rd), I(rb)), 'alu'
            ins.imm = rb
            return ins
        if xo in (0, 32):
            ins.m = 'cmpw' if xo == 0 else 'cmplw'
            ins.ops, ins.kind = (C(rd >> 2), R(ra), R(rb)), 'cmp'
            return ins
        if xo in (339, 467, 371):
            spr = ((w >> 16) & 31) | (((w >> 11) & 31) << 5)
            if xo == 371 and (spr not in (268, 269) or w & 1):
                # Gekko mftb has exactly TBL/TBU selectors and reserved Rc=0.
                # Invalid selectors must not silently become a low TB read.
                return ins
            ins.imm = spr
            if xo == 339:
                ins.m = {8: 'mflr', 9: 'mfctr', 1: 'mfxer'}.get(spr, 'mfspr')
                ins.ops = (R(rd),) if spr in (8, 9, 1) else (R(rd), ('s', spr))
            elif xo == 467:
                ins.m = {8: 'mtlr', 9: 'mtctr', 1: 'mtxer'}.get(spr, 'mtspr')
                ins.ops = (R(rd),) if spr in (8, 9, 1) else (('s', spr), R(rd))
            else:
                ins.m = 'mftbu' if spr == 269 else 'mftb'
                ins.ops = (R(rd),)
            ins.kind = 'sys' if spr not in (8, 9, 1) else 'alu'
            return ins
        if xo == 19:
            ins.m, ins.ops = 'mfcr', (R(rd),)
            return ins
        if xo == 144:
            ins.m, ins.ops = 'mtcrf', (I((w >> 12) & 0xFF), R(rd))
            return ins
        if xo == 83:
            ins.m, ins.ops, ins.kind = 'mfmsr', (R(rd),), 'sys'
            return ins
        if xo == 146:
            ins.m, ins.ops, ins.kind = 'mtmsr', (R(rd),), 'sys'
            return ins
        if xo in X_CACHE:
            ins.m, ins.ops, ins.kind = X_CACHE[xo], (('x', ra, rb),), 'sys'
            return ins
        if xo in X_SYS0:
            ins.m, ins.kind = X_SYS0[xo], 'sys'
            return ins
        if xo == 4:
            ins.m, ins.ops, ins.kind = 'tw', (I(rd), R(ra), R(rb)), 'sys'
            return ins
        if xo == 512:
            ins.m, ins.ops = 'mcrxr', (C(rd >> 2),)
            return ins
        if xo in (595, 659, 210, 242, 306):
            ins.m, ins.kind = {595: 'mfsr', 659: 'mfsrin', 210: 'mtsr',
                               242: 'mtsrin', 306: 'tlbie'}[xo], 'sys'
            ins.ops = (R(rd), I(ra & 15))
            return ins
        if xo in (597, 725):
            ins.m = 'lswi' if xo == 597 else 'stswi'
            ins.ops = (R(rd), R(ra), I(rb))
            ins.kind = 'load' if xo == 597 else 'store'
            return ins
        if xo in (310, 438):
            ins.m = 'eciwx' if xo == 310 else 'ecowx'
            ins.ops = (R(rd), ('x', ra, rb))
            return ins
        return ins
    if op == 59:
        xo = (w >> 1) & 31
        rc = '.' if (w & 1) else ''
        frc = (w >> 6) & 31
        if xo in OP59:
            m = OP59[xo]
            ins.m = m + rc
            ins.kind = 'fp'
            if m in ('fdivs', 'fsubs', 'fadds'):
                ins.ops = (F(rd), F(ra), F(rb))
            elif m == 'fmuls':
                ins.ops = (F(rd), F(ra), F(frc))
            elif m in ('fres', 'fsqrts'):
                ins.ops = (F(rd), F(rb))
            else:
                ins.ops = (F(rd), F(ra), F(frc), F(rb))
        return ins
    if op == 63:
        xo5 = (w >> 1) & 31
        xo = (w >> 1) & 0x3FF
        rc = '.' if (w & 1) else ''
        frc = (w >> 6) & 31
        ins.kind = 'fp'
        if xo5 in OP63_A and xo5 >= 18:
            m = OP63_A[xo5]
            ins.m = m + rc
            if m in ('fdiv', 'fsub', 'fadd'):
                ins.ops = (F(rd), F(ra), F(rb))
            elif m == 'fmul':
                ins.ops = (F(rd), F(ra), F(frc))
            elif m in ('frsqrte', 'fsqrt'):
                ins.ops = (F(rd), F(rb))
            else:
                ins.ops = (F(rd), F(ra), F(frc), F(rb))
            return ins
        if xo in OP63_X:
            ins.m, ins.ops = OP63_X[xo] + rc, (F(rd), F(rb))
            return ins
        if xo in (0, 32):
            ins.m = 'fcmpu' if xo == 0 else 'fcmpo'
            ins.ops, ins.kind = (C(rd >> 2), F(ra), F(rb)), 'cmp'
            return ins
        if xo == 583:
            ins.m, ins.ops = 'mffs' + rc, (F(rd),)
            return ins
        if xo == 711:
            ins.m, ins.ops = 'mtfsf' + rc, (I((w >> 17) & 0xFF), F(rb))
            return ins
        if xo in (38, 70):
            ins.m, ins.ops = ('mtfsb1' if xo == 38 else 'mtfsb0') + rc, (I(rd),)
            return ins
        if xo == 64:
            ins.m, ins.ops = 'mcrfs', (C(rd >> 2), C(ra >> 2))
            return ins
        if xo == 134:
            ins.m, ins.ops = 'mtfsfi' + rc, (C(rd >> 2), I((w >> 12) & 15))
            return ins
        ins.kind = 'other'
        return ins
    if op in (56, 57, 60, 61):
        wbit = (w >> 15) & 1
        qi = (w >> 12) & 7
        d = sx12(w & 0xFFF)
        ins.m = {56: 'psq_l', 57: 'psq_lu', 60: 'psq_st', 61: 'psq_stu'}[op]
        ins.ops = (F(rd), ('m', d, ra), I(wbit), I(qi))
        ins.imm = d
        ins.kind = 'store' if op >= 60 else 'load'
        ins.size = 4 if wbit else 8
        ins.fp = True
        ins.upd = op in (57, 61)
        return ins
    if op == 4:
        xo5 = (w >> 1) & 31
        xo6 = (w >> 1) & 63
        xo = (w >> 1) & 0x3FF
        rc = '.' if (w & 1) else ''
        frc = (w >> 6) & 31
        ins.kind = 'ps'
        if xo6 in (6, 7, 38, 39):
            wbit = (w >> 10) & 1
            qi = (w >> 7) & 7
            ins.m = {6: 'psq_lx', 7: 'psq_stx', 38: 'psq_lux', 39: 'psq_stux'}[xo6]
            ins.ops = (F(rd), ('x', ra, rb), I(wbit), I(qi))
            ins.kind = 'store' if xo6 in (7, 39) else 'load'
            ins.size = 4 if wbit else 8
            ins.fp = True
            ins.upd = xo6 in (38, 39)
            return ins
        if xo5 in PS_A:
            m = PS_A[xo5]
            ins.m = m + rc
            if m in ('ps_div', 'ps_sub', 'ps_add'):
                ins.ops = (F(rd), F(ra), F(rb))
            elif m in ('ps_mul', 'ps_muls0', 'ps_muls1'):
                ins.ops = (F(rd), F(ra), F(frc))
            elif m in ('ps_res', 'ps_rsqrte'):
                ins.ops = (F(rd), F(rb))
            else:
                ins.ops = (F(rd), F(ra), F(frc), F(rb))
            return ins
        if xo in PS_X:
            m = PS_X[xo]
            ins.m = m + rc
            if m.startswith('ps_merge'):
                ins.ops = (F(rd), F(ra), F(rb))
            else:
                ins.ops = (F(rd), F(rb))
            return ins
        if xo in (0, 32, 64, 96):
            ins.m = {0: 'ps_cmpu0', 32: 'ps_cmpo0', 64: 'ps_cmpu1', 96: 'ps_cmpo1'}[xo]
            ins.ops, ins.kind = (C(rd >> 2), F(ra), F(rb)), 'cmp'
            return ins
        if xo == 1014:
            ins.m, ins.ops, ins.kind = 'dcbz_l', (('x', ra, rb),), 'sys'
            return ins
        ins.kind = 'other'
        return ins
    return ins


def decode_range(dol, start, end):
    """Decode [start,end) returning list of Ins."""
    raw = dol.read(start, end - start)
    out = []
    import struct
    words = struct.unpack('>%dI' % ((end - start) // 4), raw)
    a = start
    for w in words:
        out.append(decode(w, a))
        a += 4
    return out


if __name__ == '__main__':
    import sys
    from dol import get_dol
    d = get_dol()
    a = int(sys.argv[1], 16)
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 32
    for ins in decode_range(d, a, a + 4 * n):
        print(ins)
