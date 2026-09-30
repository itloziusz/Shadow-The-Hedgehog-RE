#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT/build"

python3 "$ROOT/python/make_sample_fifo.py" "$ROOT/build/sample_fifo.bin"
python3 "$ROOT/python/make_sample_ppc.py" "$ROOT/build/sample_ppc.bin"
printf '\x90\x00\x01' > "$ROOT/build/no_state_primitive.bin"

# Build the C++ decoder with two independent host compilers when available.
g++ -std=c++20 -O2 -Wall -Wextra -Werror "$ROOT/cpp/gx_fifo_decoder.cpp" -o "$ROOT/build/gx_fifo_decoder_gcc"
if command -v clang++ >/dev/null 2>&1; then
  clang++ -std=c++20 -O2 -Wall -Wextra -Werror "$ROOT/cpp/gx_fifo_decoder.cpp" -o "$ROOT/build/gx_fifo_decoder_clang"
fi

go build -o "$ROOT/build/ppc_wgpipe_scan" "$ROOT/go/ppc_wgpipe_scan.go"
go vet "$ROOT/go/ppc_wgpipe_scan.go"

"$ROOT/build/gx_fifo_decoder_gcc" "$ROOT/build/sample_fifo.bin" > "$ROOT/build/fifo.out"
if [[ -x "$ROOT/build/gx_fifo_decoder_clang" ]]; then
  "$ROOT/build/gx_fifo_decoder_clang" "$ROOT/build/sample_fifo.bin" > "$ROOT/build/fifo.clang.out"
  cmp "$ROOT/build/fifo.out" "$ROOT/build/fifo.clang.out"
fi
"$ROOT/build/ppc_wgpipe_scan" -base 0x80001000 "$ROOT/build/sample_ppc.bin" > "$ROOT/build/ppc.out"

grep -q 'GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3)' "$ROOT/build/fifo.out"
grep -q 'GX_Position3f32' "$ROOT/build/fifo.out"
grep -q 'GX_Color1u32(0xff0000ff)' "$ROOT/build/fifo.out"
grep -q 'GX_TexCoord2f32' "$ROOT/build/fifo.out"
grep -q '0x80001008.*WGPIPE.*width=8' "$ROOT/build/ppc.out"
grep -q '0x80001010.*WGPIPE.*width=16' "$ROOT/build/ppc.out"
[[ "$(grep -c 'WGPIPE' "$ROOT/build/ppc.out")" -eq 2 ]]

# Fail-closed: a primitive without known VCD/VAT state must not be decoded heuristically.
if "$ROOT/build/gx_fifo_decoder_gcc" "$ROOT/build/no_state_primitive.bin" > "$ROOT/build/fail_closed.out" 2>&1; then
  echo 'FAIL: primitive with unknown VCD/VAT was accepted' >&2
  exit 1
fi
grep -q 'vertex size unknown' "$ROOT/build/fail_closed.out"

echo 'PASS: GCC C++ decoder + Clang cross-check + Go scanner + Python vectors + fail-closed negatives'
