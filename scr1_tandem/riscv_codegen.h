/*
 * Licensed to the .NET Foundation under one or more agreements.
 * The .NET Foundation licenses this file to you under the MIT license.
 */

#include <stdint.h>
#include <stddef.h>

enum {
	RISCV_X0   = 0,
	RISCV_X1   = 1,
	RISCV_X2   = 2,
	RISCV_X3   = 3,
	RISCV_X4   = 4,
	RISCV_X5   = 5,
	RISCV_X6   = 6,
	RISCV_X7   = 7,
	RISCV_X8   = 8,
	RISCV_X9   = 9,
	RISCV_X10  = 10,
	RISCV_X11  = 11,
	RISCV_X12  = 12,
	RISCV_X13  = 13,
	RISCV_X14  = 14,
	RISCV_X15  = 15,
	RISCV_X16  = 16,
	RISCV_X17  = 17,
	RISCV_X18  = 18,
	RISCV_X19  = 19,
	RISCV_X20  = 20,
	RISCV_X21  = 21,
	RISCV_X22  = 22,
	RISCV_X23  = 23,
	RISCV_X24  = 24,
	RISCV_X25  = 25,
	RISCV_X26  = 26,
	RISCV_X27  = 27,
	RISCV_X28  = 28,
	RISCV_X29  = 29,
	RISCV_X30  = 30,
	RISCV_X31  = 31,

	RISCV_ZERO = RISCV_X0,

	// Argument and return registers.

	RISCV_A0   = RISCV_X10,
	RISCV_A1   = RISCV_X11,
	RISCV_A2   = RISCV_X12,
	RISCV_A3   = RISCV_X13,
	RISCV_A4   = RISCV_X14,
	RISCV_A5   = RISCV_X15,
	RISCV_A6   = RISCV_X16,
	RISCV_A7   = RISCV_X17,

	// Callee-saved registers.

	RISCV_S0   = RISCV_X8,
	RISCV_S1   = RISCV_X9,
	RISCV_S2   = RISCV_X18,
	RISCV_S3   = RISCV_X19,
	RISCV_S4   = RISCV_X20,
	RISCV_S5   = RISCV_X21,
	RISCV_S6   = RISCV_X22,
	RISCV_S7   = RISCV_X23,
	RISCV_S8   = RISCV_X24,
	RISCV_S9   = RISCV_X25,
	RISCV_S10  = RISCV_X26,
	RISCV_S11  = RISCV_X27,

	// Temporary registers.

	RISCV_T0   = RISCV_X5,
	RISCV_T1   = RISCV_X6,
	RISCV_T2   = RISCV_X7,
	RISCV_T3   = RISCV_X28,
	RISCV_T4   = RISCV_X29,
	RISCV_T5   = RISCV_X30,
	RISCV_T6   = RISCV_X31,

	// Call stack registers.

	RISCV_SP   = RISCV_X2, // Stack pointer.
	RISCV_RA   = RISCV_X1, // Return address (AKA link register).
	RISCV_FP   = RISCV_S0, // Frame pointer (AKA base pointer).

	// ABI implementation registers.

	RISCV_GP   = RISCV_X3,
	RISCV_TP   = RISCV_X4,
};

#define RISCV_N_GREGS  (32)
#define RISCV_N_GAREGS (8)
#define RISCV_N_GSREGS (12)
#define RISCV_N_GTREGS (7)

enum {
	RISCV_F0   = 0,
	RISCV_F1   = 1,
	RISCV_F2   = 2,
	RISCV_F3   = 3,
	RISCV_F4   = 4,
	RISCV_F5   = 5,
	RISCV_F6   = 6,
	RISCV_F7   = 7,
	RISCV_F8   = 8,
	RISCV_F9   = 9,
	RISCV_F10  = 10,
	RISCV_F11  = 11,
	RISCV_F12  = 12,
	RISCV_F13  = 13,
	RISCV_F14  = 14,
	RISCV_F15  = 15,
	RISCV_F16  = 16,
	RISCV_F17  = 17,
	RISCV_F18  = 18,
	RISCV_F19  = 19,
	RISCV_F20  = 20,
	RISCV_F21  = 21,
	RISCV_F22  = 22,
	RISCV_F23  = 23,
	RISCV_F24  = 24,
	RISCV_F25  = 25,
	RISCV_F26  = 26,
	RISCV_F27  = 27,
	RISCV_F28  = 28,
	RISCV_F29  = 29,
	RISCV_F30  = 30,
	RISCV_F31  = 31,

	// Argument and return registers.

	RISCV_FA0  = RISCV_F10,
	RISCV_FA1  = RISCV_F11,
	RISCV_FA2  = RISCV_F12,
	RISCV_FA3  = RISCV_F13,
	RISCV_FA4  = RISCV_F14,
	RISCV_FA5  = RISCV_F15,
	RISCV_FA6  = RISCV_F16,
	RISCV_FA7  = RISCV_F17,

	// Callee-saved registers.

	RISCV_FS0  = RISCV_F8,
	RISCV_FS1  = RISCV_F9,
	RISCV_FS2  = RISCV_F18,
	RISCV_FS3  = RISCV_F19,
	RISCV_FS4  = RISCV_F20,
	RISCV_FS5  = RISCV_F21,
	RISCV_FS6  = RISCV_F22,
	RISCV_FS7  = RISCV_F23,
	RISCV_FS8  = RISCV_F24,
	RISCV_FS9  = RISCV_F25,
	RISCV_FS10 = RISCV_F26,
	RISCV_FS11 = RISCV_F27,

	// Temporary registers.

	RISCV_FT0  = RISCV_F0,
	RISCV_FT1  = RISCV_F1,
	RISCV_FT2  = RISCV_F2,
	RISCV_FT3  = RISCV_F3,
	RISCV_FT4  = RISCV_F4,
	RISCV_FT5  = RISCV_F5,
	RISCV_FT6  = RISCV_F6,
	RISCV_FT7  = RISCV_F7,
	RISCV_FT8  = RISCV_F28,
	RISCV_FT9  = RISCV_F29,
	RISCV_FT10 = RISCV_F30,
	RISCV_FT11 = RISCV_F31,
};

#define RISCV_N_FREGS  (32)
#define RISCV_N_FAREGS (8)
#define RISCV_N_FSREGS (12)
#define RISCV_N_FTREGS (12)

enum {
	// Floating point.

	RISCV_CSR_FFLAGS   = 0x001, // Accrued exceptions.
	RISCV_CSR_FRM      = 0x002, // Rounding mode.
	RISCV_CSR_FCSR     = 0x003, // Combination of FFLAGS and FRM.

	// Counters and timers.

	RISCV_CSR_CYCLE    = 0xc00, // Cycle counter.
	RISCV_CSR_TIME     = 0xc01, // Wall clock time.
	RISCV_CSR_INSTRET  = 0xc02, // Instruction counter.

#ifdef TARGET_RISCV32
	RISCV_CSR_CYCLEH   = 0xc80, // Upper 32 bits of CYCLE.
	RISCV_CSR_TIMEH    = 0xc81, // Upper 32 bits of TIME.
	RISCV_CSR_INSTRETH = 0xc82, // Upper 32 bits of INSTRET.
#endif
};

enum {
	RISCV_FENCE_NONE = 0b0000,

	RISCV_FENCE_W    = 0b0001, // Memory writes.
	RISCV_FENCE_R    = 0b0010, // Memory reads.
	RISCV_FENCE_O    = 0b0100, // Device outputs.
	RISCV_FENCE_I    = 0b1000, // Device inputs.

	RISCV_FENCE_MEM  = RISCV_FENCE_W | RISCV_FENCE_R,
	RISCV_FENCE_DEV  = RISCV_FENCE_O | RISCV_FENCE_I,
	RISCV_FENCE_ALL  = RISCV_FENCE_DEV | RISCV_FENCE_MEM,
};

enum {
	RISCV_ORDER_NONE = 0b00,

	RISCV_ORDER_RL   = 0b01, // Release semantics.
	RISCV_ORDER_AQ   = 0b10, // Acquire semantics.

	RISCV_ORDER_ALL  = RISCV_ORDER_RL | RISCV_ORDER_AQ,
};

enum {
	RISCV_ROUND_NE = 0b000, // Round to nearest (ties to even).
	RISCV_ROUND_TZ = 0b001, // Round towards zero.
	RISCV_ROUND_DN = 0b010, // Round down (towards negative infinity).
	RISCV_ROUND_UP = 0b011, // Round up (towards positive infinity).
	RISCV_ROUND_MM = 0b100, // Round to nearest (ties to max magnitude).
	RISCV_ROUND_DY = 0b111, // Use current rounding mode in the FRM CSR.
};

enum {
	RISCV_FCLASS_NINF = 0b1 << 0, // Negative infinity.
	RISCV_FCLASS_NN = 0b1 << 1,   // Negative normal.
	RISCV_FCLASS_ND = 0b1 << 2,   // Negative denormal.
	RISCV_FCLASS_NZ = 0b1 << 3,   // Negative zero.
	RISCV_FCLASS_PZ = 0b1 << 4,   // Positive zero.
	RISCV_FCLASS_PD = 0b1 << 5,   // Positive denormal.
	RISCV_FCLASS_PN = 0b1 << 6,   // Positive normal.
	RISCV_FCLASS_PINF = 0b1 << 7, // Positive infinity.
	RISCV_FCLASS_SNAN = 0b1 << 8, // Signalling NaN.
	RISCV_FCLASS_QNAN = 0b1 << 9, // Quiet NaN.
	RISCV_FCLASS_INF = RISCV_FCLASS_NINF | RISCV_FCLASS_PINF,
	RISCV_FCLASS_NAN = RISCV_FCLASS_SNAN | RISCV_FCLASS_QNAN,
};

#define _riscv_emit(insn) (uint32_t)insn

#define RISCV_BITS(value, start, count) (((value) >> (start)) & ((1 << (count)) - 1))
#define RISCV_SIGN(value) (-(((value) >> (sizeof (uint32_) * 8 - 1)) & 1))

// Encode an imemdiate for use in an instruction.

#define RISCV_ENCODE_I_IMM(imm) \
	(RISCV_BITS ((imm), 0, 12) << 20)
#define RISCV_ENCODE_S_IMM(imm) \
	((RISCV_BITS ((imm), 0, 5) << 7) | (RISCV_BITS ((imm), 5, 7) << 25))
#define RISCV_ENCODE_B_IMM(imm) \
	((RISCV_BITS ((imm), 11, 1) << 7) | (RISCV_BITS ((imm), 1, 4) << 8) | \
	 (RISCV_BITS ((imm), 5, 6) << 25) | (RISCV_BITS ((imm), 12, 1) << 31))
#define RISCV_ENCODE_U_IMM(imm) \
	(RISCV_BITS ((imm), 0, 20) << 12)
#define RISCV_ENCODE_J_IMM(imm) \
	((RISCV_BITS ((imm), 1, 10) << 21) | (RISCV_BITS ((imm), 11, 1) << 20) | \
	 (RISCV_BITS ((imm), 12, 8) << 12) | (RISCV_BITS ((imm), 20, 1) << 31))

// Decode an immediate from an instruction.

#define RISCV_DECODE_I_IMM(ins) \
	((RISCV_BITS ((ins), 20, 12) << 0) | (RISCV_SIGN ((ins)) << 12))
#define RISCV_DECODE_S_IMM(ins) \
	((RISCV_BITS ((ins), 7, 5) << 0) | (RISCV_BITS ((ins), 25, 7) << 5) | \
	 (RISCV_SIGN ((ins)) << 12))
#define RISCV_DECODE_B_IMM(ins) \
	((RISCV_BITS ((ins), 8, 4) << 1) | (RISCV_BITS ((ins), 25, 6) << 5) | \
	 (RISCV_BITS ((ins), 7, 1) << 11) | (RISCV_SIGN((ins)) << 12))
#define RISCV_DECODE_U_IMM(ins) \
	(RISCV_BITS ((ins), 12, 20) << 0)
#define RISCV_DECODE_J_IMM(ins) \
	((RISCV_BITS ((ins), 21, 10) << 1) | (RISCV_BITS ((ins), 20, 1) << 11) | \
	 (RISCV_BITS ((ins), 12, 8) << 12) | (RISCV_SIGN ((ins)) << 20))

// Check a value for validity as an immediate.
#define RISCV_VALID_IMM32(value) (((int32_t)value) == (value))
#define RISCV_VALID_I_IMM(value) \
	(RISCV_DECODE_I_IMM (RISCV_ENCODE_I_IMM ((value))) == (value))
#define RISCV_VALID_S_IMM(value) \
	(RISCV_DECODE_S_IMM (RISCV_ENCODE_S_IMM ((value))) == (value))
#define RISCV_VALID_B_IMM(value) \
	(RISCV_DECODE_B_IMM (RISCV_ENCODE_B_IMM ((value))) == (value))
#define RISCV_VALID_U_IMM(value) \
	(RISCV_DECODE_U_IMM (RISCV_ENCODE_U_IMM ((value))) == (value))
#define RISCV_VALID_J_IMM(value) \
	(RISCV_DECODE_J_IMM (RISCV_ENCODE_J_IMM ((value))) == (value))

// Check various values for validity in an instruction.

#define RISCV_VALID_REG(value) \
	(RISCV_BITS ((value), 0, 5) == (value))
#define RISCV_VALID_CSR(value) \
	(RISCV_BITS ((value), 0, 12) == (value))
#define RISCV_VALID_IS_AMOUNT(value) \
	(RISCV_BITS ((value), 0, 5) == (value))
#define RISCV_VALID_LS_AMOUNT(value) \
	(RISCV_BITS ((value), 0, 6) == (value))
#define RISCV_VALID_FENCE(value) \
	(RISCV_BITS ((value), 0, 4) == (value))
#define RISCV_VALID_ORDERING(value) \
	(RISCV_BITS ((value), 0, 2) == (value))

/*
 * The R-type encoding is used for a variety of instructions that operate on
 * registers only, such as most integer instructions, atomic instructions, and
 * some floating point instructions.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] rs2
 * [25..31] funct7
 */

#define _riscv_r_op(opcode, funct3, funct7, rd, rs1, rs2) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             ((rs2) << 20) | \
		             ((funct7) << 25)) \

/*
 * The R4-type encoding is used for floating point fused multiply-add
 * instructions.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] rs2
 * [25..26] funct2
 * [27..31] rs3
 */

#define _riscv_r4_op(opcode, funct3, funct2, rd, rs1, rs2, rs3) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
	                 ((rs1) << 15) | \
 	                 ((rs2) << 20) | \
		             ((funct2) << 25) | \
		             ((rs3) << 27)) \

/*
 * The I-type encoding is used for a variety of instructions, such as JALR,
 * loads, and most integer instructions that operate on immediates.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..31] imm[0..11]
 */

#define _riscv_i_op(opcode, funct3, rd, rs1, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             (RISCV_ENCODE_I_IMM ((int32_t) (imm)))) \

/*
 * This is a specialization of the I-type encoding used for shifts by immediate
 * values. The shift amount and right shift type are encoded into separate
 * parts of the imm field.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] shamt
 * [25..31] rstype
 */

#define _riscv_is_op(opcode, funct3, rstype, rd, rs1, shamt) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             (RISCV_BITS ((shamt), 0, 5) << 20) | \
		             ((rstype) << 25)) \

/*
 * A further specialization of the I-type encoding used for shifts by immediate
 * values in RV64I. The shift amount field is larger.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..25] shamt
 * [26..31] rstype
 */

#define _riscv_ls_op(opcode, funct3, rstype, rd, rs1, shamt) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             (RISCV_BITS ((shamt), 0, 6) << 20) | \
		             ((rstype) << 26)) \

/*
 * This is a specialization of the I-type encoding used for accessing control
 * and status registers.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1/zimm
 * [20..31] csr
 */

#define _riscv_ic_op(opcode, funct3, rd, csr, rs1) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             (RISCV_BITS ((csr), 0, 12) << 20)) \

/*
 * The S-type encoding is used for stores with signed offsets.
 *
 * [0....6] opcode
 * [7...11] imm[0..4]
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] rs2
 * [25..31] imm[5..11]
 */

#define _riscv_s_op(opcode, funct3, rs2, rs1, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             ((rs2) << 20) | \
		             (RISCV_ENCODE_S_IMM ((int32_t) (imm)))) \

/*
 * The B-type encoding is used for conditional branches with signed offsets.
 *
 * [0....6] opcode
 * [7....7] imm[11]
 * [8...11] imm[1..4]
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] rs2
 * [25..30] imm[5..10]
 * [31..31] imm[12]
 */

#define _riscv_b_op(opcode, funct3, rs1, rs2, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             ((rs2) << 20) | \
		             (RISCV_ENCODE_B_IMM ((int32_t) (imm)))) \

/*
 * The U-type encoding is used for LUI and AUIPC only, i.e. for instructions
 * that create 32-bit values from 20-bit immediates.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..31] imm[12..31]
 */

#define _riscv_u_op(opcode, rd, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             (RISCV_ENCODE_U_IMM ((uint32_t) (imm)))) \

/*
 * The J-type encoding is used exclusively for JAL.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..19] imm[12..19]
 * [20..20] imm[11]
 * [21..30] imm[1..10]
 * [31..31] imm[20]
 */

#define _riscv_j_op(opcode, rd, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             (RISCV_ENCODE_J_IMM ((int32_t) (imm)))) \

/*
 * Fence instructions have a peculiar encoding that isn't quite like any of the
 * other formal encoding categories.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..23] succ
 * [24..27] pred
 * [28..31] imm[0..3]
 */

#define _riscv_f_op(opcode, funct3, rd, rs1, pred, succ, imm) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             (RISCV_BITS ((succ), 0, 4) << 20) | \
		             (RISCV_BITS ((pred), 0, 4) << 24) | \
		             (RISCV_BITS ((uint32_t) (imm), 0, 4) << 28)) \

/*
 * Atomic instructions have a peculiar encoding that isn't quite like any of
 * the other formal encoding categories.
 *
 * [0....6] opcode
 * [7...11] rd
 * [12..14] funct3
 * [15..19] rs1
 * [20..24] rs2
 * [25..26] ordering
 * [27..31] funct5
 */

#define _riscv_a_op(opcode, funct3, funct5, ordering, rd, rs2, rs1) \
		_riscv_emit (((opcode) << 0) | \
		             ((rd) << 7) | \
		             ((funct3) << 12) | \
		             ((rs1) << 15) | \
		             ((rs2) << 20) | \
		             (RISCV_BITS ((ordering), 0, 2) << 25) | \
		             ((funct5) << 27)) \

/*
 * NOTE: When you add new codegen macros or change existing ones, you must
 * expand riscv-codegen-test.c to cover them, and update riscv-codegen.exp32
 * and riscv-codegen.exp64 as needed.
 */

// RV32I

#define riscv_lui(rd, imm)                      _riscv_u_op  (0b0110111, (rd), (imm))
#define riscv_auipc(rd, imm)                    _riscv_u_op  (0b0010111, (rd), (imm))
#define riscv_jal(rd, imm)                      _riscv_j_op  (0b1101111, (rd), (imm))
#define riscv_jalr(rd, rs1, imm)                _riscv_i_op  (0b1100111, 0b000, (rd), (rs1), (imm))
#define riscv_beq(rs1, rs2, imm)                _riscv_b_op  (0b1100011, 0b000, (rs1), (rs2), (imm))
#define riscv_bne(rs1, rs2, imm)                _riscv_b_op  (0b1100011, 0b001, (rs1), (rs2), (imm))
#define riscv_blt(rs1, rs2, imm)                _riscv_b_op  (0b1100011, 0b100, (rs1), (rs2), (imm))
#define riscv_bge(rs1, rs2, imm)                _riscv_b_op  (0b1100011, 0b101, (rs1), (rs2), (imm))
#define riscv_bltu(rs1, rs2, imm)               _riscv_b_op  (0b1100011, 0b110, (rs1), (rs2), (imm))
#define riscv_bgeu(rs1, rs2, imm)               _riscv_b_op  (0b1100011, 0b111, (rs1), (rs2), (imm))
#define riscv_lb(rd, rs1, imm)                  _riscv_i_op  (0b0000011, 0b000, (rd), (rs1), (imm))
#define riscv_lh(rd, rs1, imm)                  _riscv_i_op  (0b0000011, 0b001, (rd), (rs1), (imm))
#define riscv_lw(rd, rs1, imm)                  _riscv_i_op  (0b0000011, 0b010, (rd), (rs1), (imm))
#define riscv_lbu(rd, rs1, imm)                 _riscv_i_op  (0b0000011, 0b100, (rd), (rs1), (imm))
#define riscv_lhu(rd, rs1, imm)                 _riscv_i_op  (0b0000011, 0b101, (rd), (rs1), (imm))
#define riscv_sb(rs2, rs1, imm)                 _riscv_s_op  (0b0100011, 0b000, (rs2), (rs1), (imm))
#define riscv_sh(rs2, rs1, imm)                 _riscv_s_op  (0b0100011, 0b001, (rs2), (rs1), (imm))
#define riscv_sw(rs2, rs1, imm)                 _riscv_s_op  (0b0100011, 0b010, (rs2), (rs1), (imm))
#define riscv_addi(rd, rs1, imm)                _riscv_i_op  (0b0010011, 0b000, (rd), (rs1), (imm))
#define riscv_slti(rd, rs1, imm)                _riscv_i_op  (0b0010011, 0b010, (rd), (rs1), (imm))
#define riscv_sltiu(rd, rs1, imm)               _riscv_i_op  (0b0010011, 0b011, (rd), (rs1), (imm))
#define riscv_xori(rd, rs1, imm)                _riscv_i_op  (0b0010011, 0b100, (rd), (rs1), (imm))
#define riscv_ori(rd, rs1, imm)                 _riscv_i_op  (0b0010011, 0b110, (rd), (rs1), (imm))
#define riscv_andi(rd, rs1, imm)                _riscv_i_op  (0b0010011, 0b111, (rd), (rs1), (imm))
#ifdef TARGET_RISCV32
#define riscv_slli(rd, rs1, shamt)              _riscv_is_op (0b0010011, 0b001, 0b0000000, (rd), (rs1), (shamt))
#define riscv_srli(rd, rs1, shamt)              _riscv_is_op (0b0010011, 0b101, 0b0000000, (rd), (rs1), (shamt))
#define riscv_srai(rd, rs1, shamt)              _riscv_is_op (0b0010011, 0b101, 0b0100000, (rd), (rs1), (shamt))
#endif
#define riscv_add(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b000, 0b0000000, (rd), (rs1), (rs2))
#define riscv_sub(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b000, 0b0100000, (rd), (rs1), (rs2))
#define riscv_sll(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b001, 0b0000000, (rd), (rs1), (rs2))
#define riscv_slt(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b010, 0b0000000, (rd), (rs1), (rs2))
#define riscv_sltu(rd, rs1, rs2)                _riscv_r_op  (0b0110011, 0b011, 0b0000000, (rd), (rs1), (rs2))
#define riscv_xor(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b100, 0b0000000, (rd), (rs1), (rs2))
#define riscv_srl(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b101, 0b0000000, (rd), (rs1), (rs2))
#define riscv_sra(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b101, 0b0100000, (rd), (rs1), (rs2))
#define riscv_or(rd, rs1, rs2)                  _riscv_r_op  (0b0110011, 0b110, 0b0000000, (rd), (rs1), (rs2))
#define riscv_and(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b111, 0b0000000, (rd), (rs1), (rs2))
#define riscv_fence(pred, succ)                 _riscv_f_op  (0b0001111, 0b000, 0b00000, 0b00000, (pred), (succ), 0b0000)
#define riscv_fence_i()                         _riscv_f_op  (0b0001111, 0b001, 0b00000, 0b00000, 0b0000, 0b0000, 0b0000)
#define riscv_ecall()                           _riscv_i_op  (0b1110011, 0b000, 0b00000, 0b00000, 0b000000000000)
#define riscv_ebreak()                          _riscv_i_op  (0b1110011, 0b000, 0b00000, 0b00000, 0b000000000001)
#define riscv_csrrw(rd, csr, rs1)               _riscv_ic_op (0b1110011, 0b001, (rd), (csr), (rs1))
#define riscv_csrrs(rd, csr, rs1)               _riscv_ic_op (0b1110011, 0b010, (rd), (csr), (rs1))
#define riscv_csrrc(rd, csr, rs1)               _riscv_ic_op (0b1110011, 0b011, (rd), (csr), (rs1))
#define riscv_csrrwi(rd, csr, imm)              _riscv_ic_op (0b1110011, 0b101, (rd), (csr), (imm))
#define riscv_csrrsi(rd, csr, imm)              _riscv_ic_op (0b1110011, 0b110, (rd), (csr), (imm))
#define riscv_csrrci(rd, csr, imm)              _riscv_ic_op (0b1110011, 0b111, (rd), (csr), (imm))

// RV64I

#ifdef TARGET_RISCV64
#define riscv_lwu(rd, rs1, imm)                 _riscv_i_op  (0b0000011, 0b110, (rd), (rs1), (imm))
#define riscv_ld(rd, rs1, imm)                  _riscv_i_op  (0b0000011, 0b011, (rd), (rs1), (imm))
#define riscv_sd(rs2, rs1, imm)                 _riscv_s_op  (0b0100011, 0b011, (rs2), (rs1), (imm))
#define riscv_slli(rd, rs1, shamt)              _riscv_ls_op (0b0010011, 0b001, 0b000000, (rd), (rs1), (shamt))
#define riscv_srli(rd, rs1, shamt)              _riscv_ls_op (0b0010011, 0b101, 0b000000, (rd), (rs1), (shamt))
#define riscv_srai(rd, rs1, shamt)              _riscv_ls_op (0b0010011, 0b101, 0b010000, (rd), (rs1), (shamt))
#define riscv_addiw(rd, rs1, imm)               _riscv_i_op  (0b0011011, 0b000, (rd), (rs1), (imm))
#define riscv_slliw(rd, rs1, shamt)             _riscv_is_op (0b0011011, 0b001, 0b0000000, (rd), (rs1), (shamt))
#define riscv_srliw(rd, rs1, shamt)             _riscv_is_op (0b0011011, 0b101, 0b0000000, (rd), (rs1), (shamt))
#define riscv_sraiw(rd, rs1, shamt)             _riscv_is_op (0b0011011, 0b101, 0b0100000, (rd), (rs1), (shamt))
#define riscv_addw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b000, 0b0000000, (rd), (rs1), (rs2))
#define riscv_subw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b000, 0b0100000, (rd), (rs1), (rs2))
#define riscv_sllw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b001, 0b0000000, (rd), (rs1), (rs2))
#define riscv_srlw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b101, 0b0000000, (rd), (rs1), (rs2))
#define riscv_sraw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b101, 0b0100000, (rd), (rs1), (rs2))
#endif

// RV32M

#define riscv_mul(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b000, 0b0000001, (rd), (rs1), (rs2))
#define riscv_mulh(rd, rs1, rs2)                _riscv_r_op  (0b0110011, 0b001, 0b0000001, (rd), (rs1), (rs2))
#define riscv_mulhsu(rd, rs1, rs2)              _riscv_r_op  (0b0110011, 0b010, 0b0000001, (rd), (rs1), (rs2))
#define riscv_mulhu(rd, rs1, rs2)               _riscv_r_op  (0b0110011, 0b011, 0b0000001, (rd), (rs1), (rs2))
#define riscv_div(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b100, 0b0000001, (rd), (rs1), (rs2))
#define riscv_divu(rd, rs1, rs2)                _riscv_r_op  (0b0110011, 0b101, 0b0000001, (rd), (rs1), (rs2))
#define riscv_rem(rd, rs1, rs2)                 _riscv_r_op  (0b0110011, 0b110, 0b0000001, (rd), (rs1), (rs2))
#define riscv_remu(rd, rs1, rs2)                _riscv_r_op  (0b0110011, 0b111, 0b0000001, (rd), (rs1), (rs2))

// RV64M

#ifdef TARGET_RISCV64
#define riscv_mulw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b000, 0b0000001, (rd), (rs1), (rs2))
#define riscv_divw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b100, 0b0000001, (rd), (rs1), (rs2))
#define riscv_divuw(rd, rs1, rs2)               _riscv_r_op  (0b0111011, 0b101, 0b0000001, (rd), (rs1), (rs2))
#define riscv_remw(rd, rs1, rs2)                _riscv_r_op  (0b0111011, 0b110, 0b0000001, (rd), (rs1), (rs2))
#define riscv_remuw(rd, rs1, rs2)               _riscv_r_op  (0b0111011, 0b111, 0b0000001, (rd), (rs1), (rs2))
#endif

// RV32A

#define riscv_lr_w(ordering, rd, rs1)           _riscv_a_op  (0b0101111, 0b010, 0b00010, (ordering), (rd), 0b00000, (rs1))
#define riscv_sc_w(ordering, rd, rs2, rs1)      _riscv_a_op  (0b0101111, 0b010, 0b00011, (ordering), (rd), (rs2), (rs1))
#define riscv_amoswap_w(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b010, 0b00001, (ordering), (rd), (rs2), (rs1))
#define riscv_amoadd_w(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b010, 0b00000, (ordering), (rd), (rs2), (rs1))
#define riscv_amoxor_w(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b010, 0b00100, (ordering), (rd), (rs2), (rs1))
#define riscv_amoand_w(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b010, 0b01100, (ordering), (rd), (rs2), (rs1))
#define riscv_amoor_w(ordering, rd, rs2, rs1)   _riscv_a_op  (0b0101111, 0b010, 0b01000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomin_w(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b010, 0b10000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomax_w(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b010, 0b10100, (ordering), (rd), (rs2), (rs1))
#define riscv_amominu_w(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b010, 0b11000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomaxu_w(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b010, 0b11100, (ordering), (rd), (rs2), (rs1))

// RV64A

#ifdef TARGET_RISCV64
#define riscv_lr_d(ordering, rd, rs1)           _riscv_a_op  (0b0101111, 0b011, 0b00010, (ordering), (rd), 0b00000, (rs1))
#define riscv_sc_d(ordering, rd, rs2, rs1)      _riscv_a_op  (0b0101111, 0b011, 0b00011, (ordering), (rd), (rs2), (rs1))
#define riscv_amoswap_d(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b011, 0b00001, (ordering), (rd), (rs2), (rs1))
#define riscv_amoadd_d(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b011, 0b00000, (ordering), (rd), (rs2), (rs1))
#define riscv_amoxor_d(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b011, 0b00100, (ordering), (rd), (rs2), (rs1))
#define riscv_amoand_d(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b011, 0b01100, (ordering), (rd), (rs2), (rs1))
#define riscv_amoor_d(ordering, rd, rs2, rs1)   _riscv_a_op  (0b0101111, 0b011, 0b01000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomin_d(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b011, 0b10000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomax_d(ordering, rd, rs2, rs1)  _riscv_a_op  (0b0101111, 0b011, 0b10100, (ordering), (rd), (rs2), (rs1))
#define riscv_amominu_d(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b011, 0b11000, (ordering), (rd), (rs2), (rs1))
#define riscv_amomaxu_d(ordering, rd, rs2, rs1) _riscv_a_op  (0b0101111, 0b011, 0b11100, (ordering), (rd), (rs2), (rs1))
#endif

// RV32F

#define riscv_flw(rd, rs1, imm)                 _riscv_i_op  (0b0000111, 0b010, (rd), (rs1), (imm))
#define riscv_fsw(rs2, rs1, imm)                _riscv_s_op  (0b0100111, 0b010, (rs2), (rs1), (imm))
#define riscv_fmadd_s(rm, rd, rs1, rs2, rs3)    _riscv_r4_op (0b1000011, (rm), 0b00, (rd), (rs1), (rs2), (rs3))
#define riscv_fmsub_s(rm, rd, rs1, rs2, rs3)    _riscv_r4_op (0b1000111, (rm), 0b00, (rd), (rs1), (rs2), (rs3))
#define riscv_fnmadd_s(rm, rd, rs1, rs2, rs3)   _riscv_r4_op (0b1001011, (rm), 0b00, (rd), (rs1), (rs2), (rs3))
#define riscv_fnmsub_s(rm, rd, rs1, rs2, rs3)   _riscv_r4_op (0b1001111, (rm), 0b00, (rd), (rs1), (rs2), (rs3))
#define riscv_fadd_s(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0000000, (rd), (rs1), (rs2))
#define riscv_fsub_s(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0000100, (rd), (rs1), (rs2))
#define riscv_fmul_s(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0001000, (rd), (rs1), (rs2))
#define riscv_fdiv_s(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0001100, (rd), (rs1), (rs2))
#define riscv_fsqrt_s(rm, rd, rs1)              _riscv_r_op  (0b1010011, (rm), 0b0101100, (rd), (rs1), 0b00000)
#define riscv_fsgnj_s(rd, rs1, rs2)             _riscv_r_op  (0b1010011, 0b000, 0b0010000, (rd), (rs1), (rs2))
#define riscv_fsgnjn_s(rd, rs1, rs2)            _riscv_r_op  (0b1010011, 0b001, 0b0010000, (rd), (rs1), (rs2))
#define riscv_fsgnjx_s(rd, rs1, rs2)            _riscv_r_op  (0b1010011, 0b010, 0b0010000, (rd), (rs1), (rs2))
#define riscv_fmin_s(rd, rs1, rs2)              _riscv_r_op  (0b1010011, 0b000, 0b0010100, (rd), (rs1), (rs2))
#define riscv_fmax_s(rd, rs1, rs2)              _riscv_r_op  (0b1010011, 0b001, 0b0010100, (rd), (rs1), (rs2))
#define riscv_fcvt_w_s(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b1100000, (rd), (rs1), 0b00000)
#define riscv_fcvt_wu_s(rm, rd, rs1)            _riscv_r_op  (0b1010011, (rm), 0b1100000, (rd), (rs1), 0b00001)
#define riscv_fmv_x_w(rd, rs1)                  _riscv_r_op  (0b1010011, 0b000, 0b1110000, (rd), (rs1), 0b00000)
#define riscv_feq_s(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b010, 0b1010000, (rd), (rs1), (rs2))
#define riscv_flt_s(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b001, 0b1010000, (rd), (rs1), (rs2))
#define riscv_fle_s(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b000, 0b1010000, (rd), (rs1), (rs2))
#define riscv_fclass_s(rd, rs1)                 _riscv_r_op  (0b1010011, 0b001, 0b1110000, (rd), (rs1), 0b00000)
#define riscv_fcvt_s_w(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b1101000, (rd), (rs1), 0b00000)
#define riscv_fcvt_s_wu(rm, rd, rs1)            _riscv_r_op  (0b1010011, (rm), 0b1101000, (rd), (rs1), 0b00001)
#define riscv_fmv_w_x(rd, rs1)                  _riscv_r_op  (0b1010011, 0b000, 0b1111000, (rd), (rs1), 0b00000)

// RV64F

#ifdef TARGET_RISCV64
#define riscv_fcvt_l_s(rm, rd, rs1)             _riscv_r_op (0b1010011, (rm), 0b1100000, (rd), (rs1), 0b00010)
#define riscv_fcvt_lu_s(rm, rd, rs1)            _riscv_r_op (0b1010011, (rm), 0b1100000, (rd), (rs1), 0b00011)
#define riscv_fcvt_s_l(rm, rd, rs1)             _riscv_r_op (0b1010011, (rm), 0b1101000, (rd), (rs1), 0b00010)
#define riscv_fcvt_s_lu(rm, rd, rs1)            _riscv_r_op (0b1010011, (rm), 0b1101000, (rd), (rs1), 0b00011)
#endif

// RV32D

#define riscv_fld(rd, rs1, imm)                 _riscv_i_op  (0b0000111, 0b011, (rd), (rs1), (imm))
#define riscv_fsd(rs2, rs1, imm)                _riscv_s_op  (0b0100111, 0b011, (rs2), (rs1), (imm))
#define riscv_fmadd_d(rm, rd, rs1, rs2, rs3)    _riscv_r4_op (0b1000011, (rm), 0b01, (rd), (rs1), (rs2), (rs3))
#define riscv_fmsub_d(rm, rd, rs1, rs2, rs3)    _riscv_r4_op (0b1000111, (rm), 0b01, (rd), (rs1), (rs2), (rs3))
#define riscv_fnmadd_d(rm, rd, rs1, rs2, rs3)   _riscv_r4_op (0b1001011, (rm), 0b01, (rd), (rs1), (rs2), (rs3))
#define riscv_fnmsub_d(rm, rd, rs1, rs2, rs3)   _riscv_r4_op (0b1001111, (rm), 0b01, (rd), (rs1), (rs2), (rs3))
#define riscv_fadd_d(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0000001, (rd), (rs1), (rs2))
#define riscv_fsub_d(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0000101, (rd), (rs1), (rs2))
#define riscv_fmul_d(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0001001, (rd), (rs1), (rs2))
#define riscv_fdiv_d(rm, rd, rs1, rs2)          _riscv_r_op  (0b1010011, (rm), 0b0001101, (rd), (rs1), (rs2))
#define riscv_fsqrt_d(rm, rd, rs1)              _riscv_r_op  (0b1010011, (rm), 0b0101101, (rd), (rs1), 0b00000)
#define riscv_fsgnj_d(rd, rs1, rs2)             _riscv_r_op  (0b1010011, 0b000, 0b0010001, (rd), (rs1), (rs2))
#define riscv_fsgnjn_d(rd, rs1, rs2)            _riscv_r_op  (0b1010011, 0b001, 0b0010001, (rd), (rs1), (rs2))
#define riscv_fsgnjx_d(rd, rs1, rs2)            _riscv_r_op  (0b1010011, 0b010, 0b0010001, (rd), (rs1), (rs2))
#define riscv_fmin_d(rd, rs1, rs2)              _riscv_r_op  (0b1010011, 0b000, 0b0010101, (rd), (rs1), (rs2))
#define riscv_fmax_d(rd, rs1, rs2)              _riscv_r_op  (0b1010011, 0b001, 0b0010101, (rd), (rs1), (rs2))
#define riscv_fcvt_s_d(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b0100000, (rd), (rs1), 0b00001)
#define riscv_fcvt_d_s(rd, rs1)                 _riscv_r_op  (0b1010011, 0b000, 0b0100001, (rd), (rs1), 0b00000)
#define riscv_feq_d(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b010, 0b1010001, (rd), (rs1), (rs2))
#define riscv_flt_d(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b001, 0b1010001, (rd), (rs1), (rs2))
#define riscv_fle_d(rd, rs1, rs2)               _riscv_r_op  (0b1010011, 0b000, 0b1010001, (rd), (rs1), (rs2))
#define riscv_fclass_d(rd, rs1)                 _riscv_r_op  (0b1010011, 0b001, 0b1110001, (rd), (rs1), 0b00000)
#define riscv_fcvt_w_d(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b1100001, (rd), (rs1), 0b00000)
#define riscv_fcvt_wu_d(rm, rd, rs1)            _riscv_r_op  (0b1010011, (rm), 0b1100001, (rd), (rs1), 0b00001)
#define riscv_fcvt_d_w(rd, rs1)                 _riscv_r_op  (0b1010011, 0b000, 0b1101001, (rd), (rs1), 0b00000)
#define riscv_fcvt_d_wu(rd, rs1)                _riscv_r_op  (0b1010011, 0b000, 0b1101001, (rd), (rs1), 0b00001)

// RV64D

#ifdef TARGET_RISCV64
#define riscv_fcvt_l_d(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b1100001, (rd), (rs1), 0b00010)
#define riscv_fcvt_lu_d(rm, rd, rs1)            _riscv_r_op  (0b1010011, (rm), 0b1100001, (rd), (rs1), 0b00011)
#define riscv_fmv_x_d(rd, rs1)                  _riscv_r_op  (0b1010011, 0b000, 0b1110001, (rd), (rs1), 0b00000)
#define riscv_fcvt_d_l(rm, rd, rs1)             _riscv_r_op  (0b1010011, (rm), 0b1101001, (rd), (rs1), 0b00010)
#define riscv_fcvt_d_lu(rm, rd, rs1)            _riscv_r_op  (0b1010011, (rm), 0b1101001, (rd), (rs1), 0b00011)
#define riscv_fmv_d_x(rd, rs1)                  _riscv_r_op  (0b1010011, 0b000, 0b1111001, (rd), (rs1), 0b00000)
#endif
