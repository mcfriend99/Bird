#ifndef BLADE_DEBUG_H
#define BLADE_DEBUG_H

#include "blob.h"

void disassemble_blob(b_blob *blob, const char *name);

int disassemble_instruction(b_blob *blob, int offset);

#endif