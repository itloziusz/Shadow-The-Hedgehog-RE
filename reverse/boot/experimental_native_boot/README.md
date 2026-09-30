# Experimental native-boot research source

These 73 authored C++ files come from an earlier GameCube startup and native
boot investigation. They preserve constructor, CRT, SI, timing and hardware
contract experiments with original addresses and test probes. This is a
**reference-only research archive**, separate from the buildable native
gameplay, asset and resource modules.

The original experiment depended on a large generated recompiler corpus and
generated headers. Those products, local captures and binaries are excluded
from the public tree, so this directory is deliberately not a build target.
It does not establish a playable boot path or complete native platform layer.
The current source-level evidence is in `reverse/boot/` and the gameplay
handoff; promote individual behaviors only after independent address review
and focused tests.
