#pragma once

#include "boot_image.h"
#include "golden_trace.h"

// Pins the hardware helper and the CRT tables. Does not execute them and does not
// invent the unmapped FPR bytes. `applying` only changes the report: the semantic
// replacement is performed by the caller after this returns.
void IsolateHardwareFrontier(const BootImage& image, const PreEntryOracle& oracle, bool applying);
