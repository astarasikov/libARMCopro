#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define BIT(x) (1 << (x))

enum {
	B0001 = 1,
	B0111 = 7,
	B1110 = 14,
	B1111 = 15,
};

/******************************************************************************
 * Accessors to manipulate bitfields
 *****************************************************************************/

/* It would be nice to use functions to explicitely specify types,
 * but it is impossible to use functions to initialize static constants
 */

#define Pack(_Field, val) ((val & (Mask_ ##_Field)) << (ShiftWidth_ ##_Field))
#define Extract(_Field, opcode) ((opcode >> (ShiftWidth_ ##_Field)) & (Mask_ ##_Field))

#define MaskInReg(_Field) ((Mask_ ##_Field) << (ShiftWidth_ ##_Field))

/******************************************************************************
 * MCR/MRC definitions
 *****************************************************************************/

/*
 * MCR/MRC format:
 * 31   27   23   19   15
 * YYYY 1110 YYYL YYYY RRRR 1111 YYY1 YYYY
 * COND YYYY OP1L CR_N R__D 1111 OP21 CR_M
 */
enum {
	MaskMcrMrc_24_28 = B1110,
	MaskMcrMrc_8_11 = B1111,
	MaskMcrMrc = (MaskMcrMrc_24_28 << 24) | (MaskMcrMrc_8_11 << 8),

	ShiftWidth_Op1 = 21,
	ShiftWidth_Ldop = 20,
	ShiftWidth_CRn = 16,
	ShiftWidth_Rd = 12,
	ShiftWidth_Cp = 8,
	ShiftWidth_Op2 = 5,
	ShiftWidth_CRm = 0,

	Mask_1bit = B0001,
	Mask_3bits = B0111,
	Mask_4bits = B1111,

	Mask_Op1 = Mask_3bits,
	Mask_Ldop = Mask_1bit,
	Mask_CRn = Mask_4bits,
	Mask_Rd = Mask_4bits,
	Mask_Cp = Mask_4bits,
	Mask_Op2 = Mask_3bits,
	Mask_CRm = Mask_4bits,

	Mask_Op1_CRm_Op2_CRn = MaskInReg(Op1) | MaskInReg(CRm) | MaskInReg(Op2) | MaskInReg(CRn),
};

static bool is_mcr_or_mrc(uint32_t opcode)
{
	return MaskMcrMrc == (opcode & MaskMcrMrc);
}

/******************************************************************************
 * Coprocessor Register Descriptions
 *****************************************************************************/

enum ArmIsaRevision {
	ArmIsaCortexA9 = BIT(0),
	ArmIsaCortexA15 = BIT(1),
	ArmIsaCortexR4 = BIT(2),
};

#define RDESC_ALL(_mask, _name, _comment) \
	{ .mask = _mask, .name = _name, .comment = _comment, .isaMask = 0xffffffff }

#define RDESC_A15(_mask, _name, _comment) \
	{ .mask = _mask, .name = _name, .comment = _comment, .isaMask = ArmIsaCortexA15 }

#define RDESC_CR4(_mask, _name, _comment) \
	{ .mask = _mask, .name = _name, .comment = _comment, .isaMask = ArmIsaCortexR4 }

#define STRING_UNWRAP(str) ((NULL == str) ? "Unknown" : str)

static struct RegDesc {
	uint32_t mask;
	char *name;
	char *comment;
	uint32_t isaMask;
} RegDescs[] = {
	/**
	 * C0
	 */
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "MIDR", "Main ID Register"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "CTR", "Cache Type Register"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "TCMTR", "TCM Type Register"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 3), "TLBTR", "TLB Type Register"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 5), "MPIDR", "Multiprocessor Affinity Register"),
	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 6), "REVIDR", "Revision ID Register"),

	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "ID_PFR0", "Processor Feature Register 0"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 1), "ID_PFR1", "Processor Feature Register 1"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 2), "ID_DFR0", "Debug Feature Register 0"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 3), "ID_AFR0", "Auiliary Feature Register 0"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 4), "ID_MMFR0", "Memory Model Feature Register 0"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 5), "ID_MMFR1", "Memory Model Feature Register 1"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 6), "ID_MMFR2", "Memory Model Feature Register 2"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 7), "ID_MMFR3", "Memory Model Feature Register 3"),

	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 0), "ID_ISAR0", "Instruction Set Attributes Register 0"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 1), "ID_ISAR1", "Instruction Set Attributes Register 1"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 2), "ID_ISAR2", "Instruction Set Attributes Register 2"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 3), "ID_ISAR3", "Instruction Set Attributes Register 3"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 4), "ID_ISAR4", "Instruction Set Attributes Register 4"),
	RDESC_ALL(Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 5), "ID_ISAR5", "Instruction Set Attributes Register 5"),

	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 1) | Pack(CRm, 2) | Pack(Op2, 0), "CCSIDR", "Current Cache Size ID"),
	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 1) | Pack(CRm, 2) | Pack(Op2, 1), "CLIDR", "Current Cache Level ID"),
	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 1) | Pack(CRm, 2) | Pack(Op2, 7), "AIDR", NULL),
	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 2) | Pack(CRm, 0) | Pack(Op2, 0), "CSSELR", "Cache Size Selection"),

	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 0), "VPIDR", NULL),
	RDESC_A15(Pack(CRn, 0) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 5), "VMPIDR", NULL),

	/**
	 * C1
	 */
	RDESC_ALL(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "SCTLR", "System Control Register"),
	RDESC_ALL(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "ACTLR", "Auxiliary Control Register"),
	RDESC_ALL(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "CPACR", "Coprocessor Access Control Register"),

	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "SCR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 1), "SDER", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 2), "NSACR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 3), "VCR", NULL),

	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 0), "HSCTLR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 1), "HACTLR", NULL),

	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 0), "HCR", "Hypervisor Control Register"),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 1), "HDCR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 2), "HCPTR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 3), "HSTR", NULL),
	RDESC_A15(Pack(CRn, 1) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 7), "HACR", "Hypervisor AUX Control Register"),

	/**
	 * C2
	 */
	RDESC_A15(Pack(CRn, 2) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "TTBR0", "Translation Table Base Register 0"),
	RDESC_A15(Pack(CRn, 2) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "TTBR1", "Translation Table Base Register 1"),
	RDESC_A15(Pack(CRn, 2) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "TTBCR", "Translation Table Base Control Register"),

	RDESC_A15(Pack(CRn, 2) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 2), "HTCR", NULL),
	RDESC_A15(Pack(CRn, 2) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 2), "VTCR", NULL),

	/**
	 * C3
	 */
	RDESC_A15(Pack(CRn, 3) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "DACR", NULL),

	/**
	 * C5
	 */
	RDESC_ALL(Pack(CRn, 5) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "DFSR", "Data Fault Status Register"),
	RDESC_ALL(Pack(CRn, 5) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "IFSR", "Instruction Fault Status Register"),

	RDESC_A15(Pack(CRn, 5) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "ADFSR", "Auxiliary Data Fault Status Register"),
	RDESC_A15(Pack(CRn, 5) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 1), "AIFSR", "Auxiliary Instruction Fault Status Register"),

	RDESC_A15(Pack(CRn, 5) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 0), "HADFSR", "Hypervisor Auxiliary Data Fault Status Register"),
	RDESC_A15(Pack(CRn, 5) | Pack(Op1, 4) | Pack(CRm, 1) | Pack(Op2, 0), "HAIFSR", "Hypervisor Auxiliary Instruction Fault Status Register"),
	RDESC_A15(Pack(CRn, 5) | Pack(Op1, 4) | Pack(CRm, 2) | Pack(Op2, 0), "HSR", NULL),

	/**
	 * C6
	 */
	RDESC_ALL(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "DFAR", "Data Fault Address Register"),
	RDESC_ALL(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "IFAR", "Instruction Fault Address Register"),

	RDESC_A15(Pack(CRn, 6) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 0), "HDFAR", "Hypervisor Data Fault Address Register"),
	RDESC_A15(Pack(CRn, 6) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 2), "HIFAR", "Hypervisor Instruction Fault Address Register"),
	RDESC_A15(Pack(CRn, 6) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 4), "HPFAR", NULL),

	RDESC_CR4(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "MPU BAR", "MPU Base Address Register"),
	RDESC_CR4(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 2), "MPU RSE", "MPU Region Size and Enable"),
	RDESC_CR4(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 4), "MPU RAC", "MPU Region Access Control"),

	RDESC_CR4(Pack(CRn, 6) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 0), "MPU RN", "MPU Memory Region Number"),

	/**
	 * C7
	 */
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 3), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 4), "NOP", NULL),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "ICIALLUIS", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 6), "BPIALLIS", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 7), "Reserved", NULL),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 4) | Pack(Op2, 0), "PAR", NULL),

	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 0), "ICIALLU", "Invalidate Instruction Cache"),
	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 1), "ICIMVAU", "Invalidate Instruction Cache by MVA"),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 2), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 3), "Reserved", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 4), "ISB", "Instruction Sync Barrier"),
	RDESC_CR4(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 4), "FlushPrefetch", "Flush Prefetch Buffer"),
	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 6), "BPIALL", "Invalidate Entire Branch Predictor Array"),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 6), "DCIMVAC", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 2), "DCISW", "Invalidate Data Cache by Set/Way"),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 0), "ATS1CPR", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 1), "ATS1CPW", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 2), "ATS1CUR", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 3), "ATS1CUW", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 4), "ATS1NSOPR", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 5), "ATS1NSOPW", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 6), "ATS1NSOUR", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 8) | Pack(Op2, 7), "ATS1NSOUW", NULL),

	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 10) | Pack(Op2, 1), "DCCVAC", "Clean Data Cache line by Virtual Address"),
	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 10) | Pack(Op2, 2), "DCCSW", "Clean Data Cache by Set/Way"),
	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 10) | Pack(Op2, 4), "DSB", "Data Sync Barrier"),
	RDESC_ALL(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 10) | Pack(Op2, 5), "DMB", "Data Memory Barrier"),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 11) | Pack(Op2, 1), "DCCVAU", "Clean Data cache by VA to PoU"),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 14) | Pack(Op2, 1), "DCCIMVAC", "Clean Data cache by MVA to PoU"),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 0) | Pack(CRm, 14) | Pack(Op2, 2), "DCCISW", NULL),

	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 4) | Pack(CRm, 8) | Pack(Op2, 0), "ATS1HR", NULL),
	RDESC_A15(Pack(CRn, 7) | Pack(Op1, 4) | Pack(CRm, 8) | Pack(Op2, 0), "ATS1HW", NULL),

	/**
	 * C8
	 */
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 0), "TLBIALLIS", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 1), "TLBIMVAIS", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 2), "TLBIASIDIS", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 3), "TLBIMVAAIS", NULL),

	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 0), "TLBIALL", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 1), "TLBIMVA", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 2), "TLBIASID", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 5) | Pack(Op2, 3), "TLBIMVAA", NULL),

	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 0), "TLBIALL", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 1), "TLBIMVA", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 2), "TLBIASID", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 6) | Pack(Op2, 3), "TLBIMVAA", NULL),

	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 7) | Pack(Op2, 0), "TLBIALL", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 7) | Pack(Op2, 1), "TLBIMVA", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 7) | Pack(Op2, 2), "TLBIASID", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 0) | Pack(CRm, 7) | Pack(Op2, 3), "TLBIMVAA", NULL),

	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 3) | Pack(Op2, 0), "TLBIALLHIS", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 3) | Pack(Op2, 1), "TLBIMVAHIS", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 3) | Pack(Op2, 4), "TLBIALLNSHIS", NULL),

	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 7) | Pack(Op2, 0), "TLBIALLH", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 7) | Pack(Op2, 1), "TLBIMVAH", NULL),
	RDESC_A15(Pack(CRn, 8) | Pack(Op1, 4) | Pack(CRm, 7) | Pack(Op2, 4), "TLBIALLNSNH", NULL),

	/**
	 * C9
	 */
	RDESC_A15(Pack(CRn, 9) | Pack(Op1, 1) | Pack(CRm, 0) | Pack(Op2, 2), "L2CTLR", NULL),
	RDESC_A15(Pack(CRn, 9) | Pack(Op1, 1) | Pack(CRm, 0) | Pack(Op2, 3), "L2ECTLR", NULL),

	/**
	 * C10
	 */
	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "TLB Lockdown", NULL),

	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 0), "PRRR/MAIR0", NULL),
	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 0) | Pack(CRm, 2) | Pack(Op2, 1), "NMRR/MAIR1", NULL),

	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 0), "AMAIR0", NULL),
	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 0) | Pack(CRm, 3) | Pack(Op2, 1), "AMAIR1", NULL),

	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 4) | Pack(CRm, 2) | Pack(Op2, 0), "HMAIR0", NULL),
	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 4) | Pack(CRm, 2) | Pack(Op2, 1), "HMAIR1", NULL),

	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 4) | Pack(CRm, 3) | Pack(Op2, 0), "HAMAIR0", NULL),
	RDESC_A15(Pack(CRn, 10) | Pack(Op1, 4) | Pack(CRm, 3) | Pack(Op2, 1), "HAMAIR1", NULL),

	/**
	 * C12
	 */
	RDESC_A15(Pack(CRn, 12) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "VBAR", NULL),
	RDESC_A15(Pack(CRn, 12) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "MVBAR", NULL),

	RDESC_A15(Pack(CRn, 12) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "ISR", NULL),
	RDESC_A15(Pack(CRn, 12) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 1), "VIR", NULL),

	RDESC_A15(Pack(CRn, 12) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 0), "HVBAR", NULL),

	/**
	 * C13
	 */
	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "FCSEIDR", "[deprecated] FSCE ID Register"),
	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 1), "CONTEXTIDR", "Context ID Register"),
	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 2), "TPIDRURW", "Software Thread ID Register"),
	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 3), "TPIDRURO", NULL),
	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 4), "TPIDRPRW", NULL),

	RDESC_A15(Pack(CRn, 13) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 2), "HTPIDR", NULL),

	/**
	 * C15
	 */
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0), "PCR", "Power Control Register"),
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 0) | Pack(CRm, 1) | Pack(Op2, 0), "NEONBR", "NEON Busy Register"),

	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 4) | Pack(CRm, 0) | Pack(Op2, 0), "CFGBA", "Configuration Base Address"),

	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 5) | Pack(CRm, 4) | Pack(Op2, 2), "PCR", "Select Lockdown TLB Entry for read"),
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 5) | Pack(CRm, 4) | Pack(Op2, 4), "PCR", "Select Lockdown TLB Entry for write"),
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 5) | Pack(CRm, 5) | Pack(Op2, 2), "PCR", "Main TLB VA register"),
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 5) | Pack(CRm, 6) | Pack(Op2, 2), "PCR", "Main TLB PA register"),
	RDESC_A15(Pack(CRn, 15) | Pack(Op1, 5) | Pack(CRm, 7) | Pack(Op2, 2), "PCR", "Main TLB Attribute register"),

	RDESC_CR4(Pack(CRn, 15) | Pack(Op1, 0) | Pack(CRm, 14) | Pack(Op2, 0), "CacheSizeOverride", "Cache Size Override"),
};

static void DecodeMrcAndPrint(uint32_t opcode)
{
	if (!is_mcr_or_mrc(opcode)) {
		return;
	}

	printf("%s, %d, %d, r%d, cr%d, cr%d, {%d}\n",
			Extract(Ldop, opcode) ? "MRC" : "MCR",
			Extract(Cp, opcode),
			Extract(Op1, opcode),
			Extract(Rd, opcode),
			Extract(CRn, opcode),
			Extract(CRm, opcode),
			Extract(Op2, opcode));

	printf("%s, CRn=%d Op1=%d CRm=%d Op2=%d Rd=%d CP=%d\n",
			Extract(Ldop, opcode) ? "MRC" : "MCR",
			Extract(CRn, opcode),
			Extract(Op1, opcode),
			Extract(CRm, opcode),
			Extract(Op2, opcode),
			Extract(Rd, opcode),
			Extract(Cp, opcode));

	uint32_t opcodeMatchMask = opcode & Mask_Op1_CRm_Op2_CRn;
	for (size_t i = 0; i < ARRAY_SIZE(RegDescs); i++)
	{
		if (opcodeMatchMask == RegDescs[i].mask) {
			printf("[%s] : %s\n",
					STRING_UNWRAP(RegDescs[i].name),
					STRING_UNWRAP(RegDescs[i].comment));
		}
	}
}

/*
 * decode Rd
 * while Rd is not trashed -> decode
 *
 */

/******************************************************************************
 * ARM MRC/MCR decoding: testing routines
 ****************************************************************************/

/******************************************************************************
 * ARM MRC/MCR decoding:
 ****************************************************************************/

/**
 * Decode some hard-coded values. This is just a placeholder for development.
 */

static inline void DemoSomeMcrs(void)
{
	static uint32_t test_mcrs[] = {
		Pack(CRn, 0) | Pack(Op1, 0) | Pack(CRm, 0) | Pack(Op2, 0) | Pack(Rd, 5) | Pack(Cp, 15) | MaskMcrMrc,
		0xee011f10,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee061f12,
		0xee062f11,
		0xee064f91,
		0xee063f51,
		0xee011f10,
		0xee010f10,
		0xee000e15,
	};
	for (size_t i = 0; i < ARRAY_SIZE(test_mcrs); i++) {
		printf("Decoding: %08x\n", test_mcrs[i]);
		DecodeMrcAndPrint(test_mcrs[i]);
	}
}

static inline void RunFullTest(void)
{
	for (uint32_t i = 0; i <= 0xffffffff; i++) {
		DecodeMrcAndPrint(i);
	}
}

static inline void RunStdinDecoder(void)
{
	while (1) {
		enum {
			BUF_SIZE = 64,
		};
		char buf[BUF_SIZE];
		memset(buf, 0, BUF_SIZE);
		uint32_t val = 0;

		if (!fgets(buf, BUF_SIZE, stdin)) {
			break;
		}
		val = strtol(buf, NULL, 16);
		DecodeMrcAndPrint(val);
	};
}

static inline void RunObjdumpDecoder(void)
{
	while (1) {
		enum {
			BUF_SIZE = 64,
		};
		char buf[BUF_SIZE];
		memset(buf, 0, BUF_SIZE);
		uint32_t val = 0;

		if (!fgets(buf, BUF_SIZE, stdin)) {
			break;
		}

		/* Echo objdump output to screen */
		printf("%s", buf);
		buf[BUF_SIZE - 1] = 0;

		/**
		 * GNU objdump output usually has the following format:
		 * OFFSET: OPCODE
		 * where OFFSET is an arbitrary-sized 32-bit hex integer and
		 * the colon is followed by an arbitrary number of whitespace
		 */

		char *opcodeStr = buf;
		for (size_t i = 0; (i < 9) && (i < BUF_SIZE); i++) {
			if (isxdigit(buf[i]) || isspace(buf[i])) {
				opcodeStr++;
			}
		}
		if (*opcodeStr != ':') {
			continue;
		}
		opcodeStr++;

		val = strtol(opcodeStr, NULL, 16);
		DecodeMrcAndPrint(val);
	};
}

int main(int argc, char **argv) {
	if ((2 == argc) && (!strcmp(argv[1], "fulltest"))) {
		RunFullTest();
		exit(0);
	}
	if ((2 == argc) && (!strcmp(argv[1], "stdin"))) {
		RunStdinDecoder();
		exit(0);
	}
	if ((2 == argc) && (!strcmp(argv[1], "objdump"))) {
		RunObjdumpDecoder();
		exit(0);
	}
	DemoSomeMcrs();
}
