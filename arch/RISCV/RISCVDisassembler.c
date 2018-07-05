//===-- RISCVDisassembler.cpp - Disassembler for RISCV --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Rodrigo Cortes Porto <porto703@gmail.com>, 2018 */

#ifdef CAPSTONE_HAS_RISCV

#include <stdio.h>		// DEBUG
#include <stdlib.h>
#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"
#include "../../MathExtras.h"

#define GET_SUBTARGETINFO_ENUM
#include "RISCVGenSubtargetInfo.inc"

static uint64_t
getFeatureBits (int mode)
{
	// support everything
	return (uint64_t) - 1;
}

// Forward declare these because the autogenerated code will reference them.
// Definitions are further down.
static DecodeStatus DecodeGPRRegisterClass (MCInst * Inst, uint64_t RegNo,
					    uint64_t Address,
					    const void *Decoder);
static DecodeStatus DecodeFPR32RegisterClass (MCInst * Inst, uint64_t RegNo,
					      uint64_t Address,
					      const void *Decoder);
static DecodeStatus DecodeFPR64RegisterClass (MCInst * Inst, uint64_t RegNo,
					      uint64_t Address,
					      const void *Decoder);
static DecodeStatus decodeUImmOperand (MCInst * Inst, uint64_t Imm,
				       int64_t Address, const void *Decoder,
				       unsigned N);
static DecodeStatus decodeSImmOperand (MCInst * Inst, uint64_t Imm,
				       int64_t Address, const void *Decoder,
				       unsigned N);
static DecodeStatus decodeSImmOperandAndLsl1 (MCInst * Inst, uint64_t Imm,
					      int64_t Address,
					      const void *Decoder,
					      unsigned N);

#include "RISCVGenDisassemblerTables.inc"

#define GET_REGINFO_ENUM
#include "RISCVGenRegisterInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RISCVGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "RISCVGenInstrInfo.inc"

void
RISCV_init (MCRegisterInfo * MRI)
{
	/*InitMCRegisterInfo(RISCVRegDesc, 97, RA, PC,
	   RISCVMCRegisterClasses, 3,
	   RISCVRegUnitRoots,
	   64,
	   RISCVRegDiffLists,
	   RISCVLaneMaskLists,
	   RISCVRegStrings,
	   RISCVRegClassStrings,
	   RISCVSubRegIdxLists,
	   2,
	   RISCVSubRegIdxRanges,
	   RISCVRegEncodingTable);
	 */

	MCRegisterInfo_InitMCRegisterInfo (MRI, RISCVRegDesc, 97,
					   0, 0,
					   RISCVMCRegisterClasses, 3,
					   0, 0,
					   RISCVRegDiffLists,
					   0, RISCVSubRegIdxLists, 2, 0);
}

static DecodeStatus
RISCVDisassembler_getInstruction (int mode, MCInst * MI,
				  const uint8_t * code, size_t code_len,
				  uint64_t * Size, uint64_t Address,
				  MCRegisterInfo * MRI)
{
	/*MCInst &MI, uint64_t &Size,
	   ArrayRef<uint8_t> Bytes,
	   uint64_t Address,
	   raw_ostream &OS,
	   raw_ostream &CS) const { */

	// TODO: although assuming 4-byte instructions is sufficient for RV32 and
	// RV64, this will need modification when supporting the compressed
	// instruction set extension (RVC) which uses 16-bit instructions. Other
	// instruction set extensions have the option of defining instructions up to
	// 176 bits wide.
	*Size = 4;
	if (code_len < 4)
	  {
		  *Size = 0;
		  return MCDisassembler_Fail;
	  }

	if (MI->flat_insn->detail)
	  {
		  memset (MI->flat_insn->detail, 0, sizeof (cs_detail));
	  }

	// Get the four bytes of the instruction.
	//Encoded as little endian 32 bits.
	uint32_t Inst = (code[0] << 0) |
		(code[1] << 8) | (code[2] << 16) | (code[3] << 24);

	return decodeInstruction (DecoderTable32, MI, Inst, Address, MRI,
				  mode);
}


bool
RISCV_getInstruction (csh ud, const uint8_t * code, size_t code_len,
		      MCInst * instr, uint64_t * size, uint64_t address,
		      void *info)
{
	cs_struct *handle = (cs_struct *) (uintptr_t) ud;

	DecodeStatus status =
		RISCVDisassembler_getInstruction (handle->mode, instr,
						  code, code_len,
						  size, address,
						  (MCRegisterInfo *) info);


	return status == MCDisassembler_Success;
}

static const unsigned GPRDecoderTable[] = {
	RISCV_X0, RISCV_X1, RISCV_X2, RISCV_X3,
	RISCV_X4, RISCV_X5, RISCV_X6, RISCV_X7,
	RISCV_X8, RISCV_X9, RISCV_X10, RISCV_X11,
	RISCV_X12, RISCV_X13, RISCV_X14, RISCV_X15,
	RISCV_X16, RISCV_X17, RISCV_X18, RISCV_X19,
	RISCV_X20, RISCV_X21, RISCV_X22, RISCV_X23,
	RISCV_X24, RISCV_X25, RISCV_X26, RISCV_X27,
	RISCV_X28, RISCV_X29, RISCV_X30, RISCV_X31
};

static DecodeStatus
DecodeGPRRegisterClass (MCInst * Inst, uint64_t RegNo,
			uint64_t Address, const void *Decoder)
{
	if (RegNo > sizeof (GPRDecoderTable))
		return MCDisassembler_Fail;

	// We must define our own mapping from RegNo to register identifier.
	// Accessing index RegNo in the register class will work in the case that
	// registers were added in ascending order, but not in general.
	unsigned Reg = GPRDecoderTable[RegNo];
	//Inst.addOperand(MCOperand::createReg(Reg));
	MCOperand_CreateReg0 (Inst, Reg);
	return MCDisassembler_Success;
}

static const unsigned FPR32DecoderTable[] = {
	RISCV_F0_32, RISCV_F1_32, RISCV_F2_32, RISCV_F3_32,
	RISCV_F4_32, RISCV_F5_32, RISCV_F6_32, RISCV_F7_32,
	RISCV_F8_32, RISCV_F9_32, RISCV_F10_32, RISCV_F11_32,
	RISCV_F12_32, RISCV_F13_32, RISCV_F14_32, RISCV_F15_32,
	RISCV_F16_32, RISCV_F17_32, RISCV_F18_32, RISCV_F19_32,
	RISCV_F20_32, RISCV_F21_32, RISCV_F22_32, RISCV_F23_32,
	RISCV_F24_32, RISCV_F25_32, RISCV_F26_32, RISCV_F27_32,
	RISCV_F28_32, RISCV_F29_32, RISCV_F30_32, RISCV_F31_32
};

static DecodeStatus
DecodeFPR32RegisterClass (MCInst * Inst, uint64_t RegNo,
			  uint64_t Address, const void *Decoder)
{
	if (RegNo > sizeof (FPR32DecoderTable))
		return MCDisassembler_Fail;

	// We must define our own mapping from RegNo to register identifier.
	// Accessing index RegNo in the register class will work in the case that
	// registers were added in ascending order, but not in general.
	unsigned Reg = FPR32DecoderTable[RegNo];
	MCOperand_CreateReg0 (Inst, Reg);
	return MCDisassembler_Success;
}

static const unsigned FPR64DecoderTable[] = {
	RISCV_F0_64, RISCV_F1_64, RISCV_F2_64, RISCV_F3_64,
	RISCV_F4_64, RISCV_F5_64, RISCV_F6_64, RISCV_F7_64,
	RISCV_F8_64, RISCV_F9_64, RISCV_F10_64, RISCV_F11_64,
	RISCV_F12_64, RISCV_F13_64, RISCV_F14_64, RISCV_F15_64,
	RISCV_F16_64, RISCV_F17_64, RISCV_F18_64, RISCV_F19_64,
	RISCV_F20_64, RISCV_F21_64, RISCV_F22_64, RISCV_F23_64,
	RISCV_F24_64, RISCV_F25_64, RISCV_F26_64, RISCV_F27_64,
	RISCV_F28_64, RISCV_F29_64, RISCV_F30_64, RISCV_F31_64
};

static DecodeStatus
DecodeFPR64RegisterClass (MCInst * Inst, uint64_t RegNo,
			  uint64_t Address, const void *Decoder)
{
	if (RegNo > sizeof (FPR64DecoderTable))
		return MCDisassembler_Fail;

	// We must define our own mapping from RegNo to register identifier.
	// Accessing index RegNo in the register class will work in the case that
	// registers were added in ascending order, but not in general.
	unsigned Reg = FPR64DecoderTable[RegNo];
	MCOperand_CreateReg0 (Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus
decodeUImmOperand (MCInst * Inst, uint64_t Imm,
		   int64_t Address, const void *Decoder, unsigned N)
{
	//Inst.addOperand(MCOperand::createImm(Imm));
	MCOperand_CreateImm0 (Inst, Imm);
	return MCDisassembler_Success;
}

static DecodeStatus
decodeSImmOperand (MCInst * Inst, uint64_t Imm,
		   int64_t Address, const void *Decoder, unsigned N)
{
	// Sign-extend the number in the bottom N bits of Imm
	//Inst.addOperand(MCOperand::createImm(SignExtend64<N>(Imm)));
	MCOperand_CreateImm0 (Inst, SignExtend64 (Imm, N));
	return MCDisassembler_Success;
}

static DecodeStatus
decodeSImmOperandAndLsl1 (MCInst * Inst, uint64_t Imm,
			  int64_t Address, const void *Decoder, unsigned N)
{
	// Sign-extend the number in the bottom N bits of Imm after accounting for
	// the fact that the N bit immediate is stored in N-1 bits (the LSB is
	// always zero)
	//Inst.addOperand(MCOperand::createImm(SignExtend64<N>(Imm << 1)));
	MCOperand_CreateImm0 (Inst, SignExtend64 (Imm << 1, N));
	return MCDisassembler_Success;
}

#endif
