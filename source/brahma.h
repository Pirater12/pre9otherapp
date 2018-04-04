#pragma once

#include "exploitdata.h"
#include "utils.h"

Result brahma_init (void);
u32 brahma_exit (void);
Result load_arm9_payload_offset (char *filename, u32 offset, u32 max_psize);
void redirect_codeflow (u32 *dst_addr, u32 *src_addr);
s32 map_arm9_payload (void);
s32 map_arm11_payload (void);
void exploit_arm9_race_condition (void);
s32 get_exploit_data (exploit_data *data);
s32 firm_reboot (bool is_n3ds);

#define ARM_JUMPOUT 0xE51FF004 // LDR PC, [PC, -#04]
#define ARM_RET     0xE12FFF1E // BX LR
#define ARM_NOP     0xE1A00000 // NOP

extern void *arm11_start;
extern void *arm11_end;
extern void *arm9_start;
extern void *arm9_end;
