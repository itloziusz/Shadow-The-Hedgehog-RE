package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"os"
)

type Known struct {
	valid bool
	v     uint32
}

func sext16(x uint16) int32          { return int32(int16(x)) }
func add32(a uint32, b int32) uint32 { return uint32(int64(uint64(a)) + int64(b)) }

func main() {
	base := flag.Uint64("base", 0x80000000, "virtual base address of first word")
	flag.Parse()
	if flag.NArg() != 1 {
		fmt.Fprintln(os.Stderr, "usage: ppc_wgpipe_scan [-base 0x80000000] code.bin")
		os.Exit(2)
	}
	data, err := os.ReadFile(flag.Arg(0))
	if err != nil {
		panic(err)
	}
	if len(data)%4 != 0 {
		fmt.Fprintln(os.Stderr, "warning: trailing non-word bytes ignored")
	}
	var r [32]Known
	// EABI fixed bases are deliberately not guessed. Seed them only from actual instructions/dataflow.
	for off := 0; off+4 <= len(data); off += 4 {
		pc := uint32(*base) + uint32(off)
		w := binary.BigEndian.Uint32(data[off : off+4])
		op := w >> 26
		rs := int((w >> 21) & 31)
		ra := int((w >> 16) & 31)
		rb := int((w >> 11) & 31)
		imm := uint16(w)
		handled := false
		switch op {
		case 14: // addi / li
			if ra == 0 {
				r[rs] = Known{true, uint32(sext16(imm))}
			} else if r[ra].valid {
				r[rs] = Known{true, add32(r[ra].v, sext16(imm))}
			} else {
				r[rs] = Known{}
			}
			handled = true
		case 15: // addis / lis
			delta := int32(sext16(imm)) << 16
			if ra == 0 {
				r[rs] = Known{true, uint32(delta)}
			} else if r[ra].valid {
				r[rs] = Known{true, add32(r[ra].v, delta)}
			} else {
				r[rs] = Known{}
			}
			handled = true
		case 24: // ori rA,rS,UIMM
			if r[rs].valid {
				r[ra] = Known{true, r[rs].v | uint32(imm)}
			} else {
				r[ra] = Known{}
			}
			handled = true
		case 25: // oris
			if r[rs].valid {
				r[ra] = Known{true, r[rs].v | (uint32(imm) << 16)}
			} else {
				r[ra] = Known{}
			}
			handled = true
		case 36, 37, 38, 39, 44, 45, 52, 53, 54, 55: // D-form stores
			names := map[uint32]string{36: "stw", 37: "stwu", 38: "stb", 39: "stbu", 44: "sth", 45: "sthu", 52: "stfs", 53: "stfsu", 54: "stfd", 55: "stfdu"}
			widths := map[uint32]int{36: 32, 37: 32, 38: 8, 39: 8, 44: 16, 45: 16, 52: 32, 53: 32, 54: 64, 55: 64}
			if ra != 0 && r[ra].valid {
				ea := add32(r[ra].v, sext16(imm))
				if ea == 0xCC008000 {
					srcKind := "r"
					if op >= 52 {
						srcKind = "f"
					}
					val := "unknown"
					if srcKind == "r" && r[rs].valid {
						val = fmt.Sprintf("0x%08X", r[rs].v)
					}
					fmt.Printf("0x%08X  %-5s -> WGPIPE  width=%d  src=%s%d  value=%s\n", pc, names[op], widths[op], srcKind, rs, val)
				}
				if op == 37 || op == 39 || op == 45 || op == 53 || op == 55 {
					r[ra] = Known{true, ea}
				}
			} else if op == 37 || op == 39 || op == 45 || op == 53 || op == 55 {
				r[ra] = Known{}
			}
			handled = true
		case 18: // b / bl
			lk := (w & 1) != 0
			if lk {
				for i := 0; i <= 12; i++ {
					if i != 1 && i != 2 {
						r[i] = Known{}
					}
				}
			} else {
				for i := range r {
					r[i] = Known{}
				}
			}
			handled = true
		case 16, 19: // conditional/control transfer: merge unknown -> reset conservative state
			for i := range r {
				r[i] = Known{}
			}
			handled = true
		case 48, 50: // lfs/lfd: writes FPR only; GPR base untouched
			handled = true
		case 59, 63: // floating/paired arithmetic families: no GPR constant invalidation here
			handled = true
		}
		if op == 31 {
			xo := (w >> 1) & 0x3ff
			if xo == 444 { // or rA,rS,rB; mr when rS==rB
				if r[rs].valid && r[rb].valid {
					r[ra] = Known{true, r[rs].v | r[rb].v}
				} else {
					r[ra] = Known{}
				}
				handled = true
			}
		}
		if !handled {
			// Fail closed. Unknown PPC may modify address-bearing state; throw away all inferred constants.
			for i := range r {
				r[i] = Known{}
			}
		}
	}
}
