// -*- mode: c++ -*-

#include <stdint.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <fcntl.h>

#include "sail.h"
#include "rts.h"

#include "riscv_platform_impl.h"
#include "riscv_sail.h"

#include "Vtop_exp_ahb.h"
#include <verilated.h>

#include "riscv_codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

  void klee_make_symbolic(void *addr, size_t nbytes, const char *name);
  void klee_assume(uintptr_t condition);


#ifdef __cplusplus
}
#endif

const char *RV64ISA = "RV64IMAC";
const char *RV32ISA = "RV32IMAC";

/* Selected CSRs from riscv-isa-sim/riscv/encoding.h */
#define CSR_STVEC 0x105
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143

#define CSR_MSTATUS 0x300
#define CSR_MISA 0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MTVAL 0x343
#define CSR_MIP 0x344

enum {
  OPT_TRACE_OUTPUT = 1000,
  OPT_ENABLE_WRITABLE_FIOM,
  OPT_PMP_COUNT,
  OPT_PMP_GRAIN,
  OPT_ENABLE_SVINVAL,
  OPT_ENABLE_ZCB,
};

FILE *trace_log = NULL;
bool config_print_instr = true;
bool config_print_reg = true;
bool config_print_mem_access = true;
bool config_print_platform = true;
bool config_print_rvfi = false;

char *sig_file = NULL;
uint64_t mem_sig_start = 0;
uint64_t mem_sig_end = 0;
int signature_granularity = 4;

struct timeval init_start, init_end, run_end;
int total_insns = 0;
int insn_limit = 0;


Vtop_exp_ahb *top;

enum HTRANS { IDLE = 0b00, BUSY = 0b01, NONSEQ = 0b10, SEQ = 0b11 };

enum HSIZE { BYTE = 0b00, HALFWORD = 0b01, WORD = 0b10 };

const uint32_t SCR1_SIM_PRINT_ADDR = 0xF0000000;
const uint32_t SCR1_SIM_EXIT_ADDR = 0x000000F8;
const uint32_t ADDR_TRAP_DEFAULT = 0x000001C0;

// memory management for rtl (taken from the sail library)

struct memory_buffer scr1_memory;

void rtl_write_mem_byte(uint32_t address, uint8_t byte)
{
  // TODO: assert address?
  scr1_memory.buffer[address] = byte;
  scr1_memory.mask[address] = true;
}

uint32_t rtl_read_mem_byte(uint32_t address)
{
  // TODO: assert address?
  return scr1_memory.buffer[address];
}

void rtl_write_mem(uint32_t address, uint32_t value, uint8_t size) {
  switch (size) {
  case BYTE: {
    rtl_write_mem_byte(address, (uint8_t) value);
    break;
  }
  case HALFWORD: {
    rtl_write_mem_byte(address, (uint8_t) value);
    rtl_write_mem_byte(address + 1, (uint8_t) (value >> 8));
    break;
  }
  case WORD: {
    rtl_write_mem_byte(address, (uint8_t) value);
    rtl_write_mem_byte(address + 1, (uint8_t) (value >> 8));
    rtl_write_mem_byte(address + 2, (uint8_t) (value >> 16));
    rtl_write_mem_byte(address + 3, (uint8_t) (value >> 24));
    break;
  }
  }
}

uint32_t rtl_read_mem(uint32_t address, uint8_t size)
{
  uint32_t ret = 0;
  switch (size) {
  case BYTE: {
    ret = rtl_read_mem_byte(address);
    break;
  }
  case HALFWORD: {
    ret = (rtl_read_mem_byte(address + 1) << 8) + rtl_read_mem_byte(address);
    break;
  }
  case WORD: {
    ret = (((((rtl_read_mem_byte(address + 3) << 8)
              + rtl_read_mem_byte(address + 2))
             << 8)
            + rtl_read_mem_byte(address + 1))
           << 8)
        + rtl_read_mem_byte(address);
    break;
  }
  }
  return ret;
}

constexpr size_t MAX_INSTRS = 20;
constexpr size_t INSTR_BYTE_WIDTH = 4;

unsigned char instr_memory[MAX_INSTRS * INSTR_BYTE_WIDTH];

constexpr size_t INSTR_START = 0x200;
constexpr size_t DATA_START = INSTR_START + MAX_INSTRS * INSTR_BYTE_WIDTH;
constexpr size_t DATA_SIZE = 0x500;

enum CSR : uint32_t {
  MSTATUS,
  MIE,
  MTVEC,

  MSCRATCH,
  MEPC,
  MCAUSE,
  MTVAL,
  MIP,

  CSR_LAST = MIP,
};
const size_t CSR_SIZE = CSR_LAST + 1;

uint32_t csr_to_code(CSR csr) {
  switch (csr) {
  case MSTATUS: return 0x300;
  case MIE: return 0x304;
  case MTVEC: return 0x305;
  case MSCRATCH: return 0x340;
  case MEPC: return 0x341;
  case MCAUSE: return 0x342;
  case MTVAL: return 0x343;
  case MIP: return 0x344;
  }
}

const char *csr_to_name(CSR csr) {
  switch (csr) {
  case MSTATUS: return "MSTATUS";
  case MIE: return "MIE";
  case MTVEC: return "MTVEC";
  case MSCRATCH: return "MSCRATCH";
  case MEPC: return "MEPC";
  case MCAUSE: return "MCAUSE";
  case MTVAL: return "MTVAL";
  case MIP: return "MIP";
  }
}

struct state {
  uint32_t GPRs[32];
  uint32_t CSRs[CSR_SIZE];
};

enum class SCR1_INST : uint8_t {
  LUI,
  AUIPC,

  JAL,
  JALR,

  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,

  LB,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,

  ADDI,
  SLTI,
  SLTIU,
  XORI,
  ORI,
  ANDI,
  ADD,
  SUB,
  SLL,
  SLT,
  SLTU,
  XOR,
  SRL,
  SRA,
  OR,
  AND,
  FENCE_I,
  ECALL,
  EBREAK,

  CSRRW,
  CSRRS,
  CSRRC,
  CSRRWI,
  CSRRSI,
  CSRRCI,

  MUL,
  MULH,
  MULHSU,
  MULHU,
  DIV,
  DIVU,
  REM,
  REMU,
};

uint32_t get_symbolic_inst(uint32_t address) {
  return riscv_ebreak();
  // SCR1_INST inst_type;
  // klee_make_symbolic(&inst_type, sizeof(SCR1_INST), "inst_type");

  // klee_assume(inst_type == SCR1_INST::ADDI ||
  //             inst_type == SCR1_INST::LW ||
  //             inst_type == SCR1_INST::SW);

  // klee_assume(inst_type == SCR1_INST::LUI ||
  //             inst_type == SCR1_INST::AUIPC ||
  //             inst_type == SCR1_INST::JAL ||
  //             inst_type == SCR1_INST::JALR ||
  //             inst_type == SCR1_INST::BEQ ||
  //             inst_type == SCR1_INST::BNE ||
  //             inst_type == SCR1_INST::BLT ||
  //             inst_type == SCR1_INST::BGE ||
  //             inst_type == SCR1_INST::BLTU ||
  //             inst_type == SCR1_INST::BGEU ||
  //             inst_type == SCR1_INST::LB ||
  //             inst_type == SCR1_INST::LH ||
  //             inst_type == SCR1_INST::LW ||
  //             inst_type == SCR1_INST::LBU ||
  //             inst_type == SCR1_INST::LHU ||
  //             inst_type == SCR1_INST::SB ||
  //             inst_type == SCR1_INST::SH ||
  //             inst_type == SCR1_INST::SW ||
  //             inst_type == SCR1_INST::ADDI ||
  //             inst_type == SCR1_INST::SLTI ||
  //             inst_type == SCR1_INST::SLTIU ||
  //             inst_type == SCR1_INST::XORI ||
  //             inst_type == SCR1_INST::ORI ||
  //             inst_type == SCR1_INST::ANDI ||
  //             inst_type == SCR1_INST::ADD ||
  //             inst_type == SCR1_INST::SUB ||
  //             inst_type == SCR1_INST::SLL ||
  //             inst_type == SCR1_INST::SLT ||
  //             inst_type == SCR1_INST::SLTU ||
  //             inst_type == SCR1_INST::XOR ||
  //             inst_type == SCR1_INST::SRL ||
  //             inst_type == SCR1_INST::SRA ||
  //             inst_type == SCR1_INST::OR ||
  //             inst_type == SCR1_INST::AND ||
  //             inst_type == SCR1_INST::FENCE_I ||
  //             inst_type == SCR1_INST::ECALL ||
  //             inst_type == SCR1_INST::EBREAK ||
  //             inst_type == SCR1_INST::MUL ||
  //             inst_type == SCR1_INST::MULH ||
  //             inst_type == SCR1_INST::MULHSU ||
  //             inst_type == SCR1_INST::MULHU ||
  //             inst_type == SCR1_INST::DIV ||
  //             inst_type == SCR1_INST::DIVU ||
  //             inst_type == SCR1_INST::REM ||
  //             inst_type == SCR1_INST::REMU);

  // switch (inst_type) {
  // case SCR1_INST::LUI: {
  //   uint8_t rd, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_lui(rd, imm);
  // }
  // case SCR1_INST::AUIPC: {
  //   uint8_t rd, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_auipc(rd, imm);
  // }
  // case SCR1_INST::JAL: {
  //   uint8_t rd, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_jal(rd, imm);
  // }
  // case SCR1_INST::JALR: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_jalr(rd, rs1, imm);
  // }
  // case SCR1_INST::BEQ: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_beq(rs1, rs2, imm);
  // }
  // case SCR1_INST::BNE: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_bne(rs1, rs2, imm);
  // }
  // case SCR1_INST::BLT: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_blt(rs1, rs2, imm);
  // }
  // case SCR1_INST::BGE: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_bge(rs1, rs2, imm);
  // }
  // case SCR1_INST::BLTU: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_bltu(rs1, rs2, imm);
  // }
  // case SCR1_INST::BGEU: {
  //   uint8_t rs1, rs2, imm;
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_bgeu(rs1, rs2, imm);
  // }
  // case SCR1_INST::LB: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_lb(rd, rs1, imm);
  // }
  // case SCR1_INST::LH: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_lh(rd, rs1, imm);
  // }
  // case SCR1_INST::LW: {
  //   uint8_t rd, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_sw(rd, RISCV_X0, imm);
  // }
  // case SCR1_INST::LBU: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_lbu(rd, rs1, imm);
  // }
  // case SCR1_INST::LHU: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_lhu(rd, rs1, imm);
  // }
  // case SCR1_INST::SB: {
  //   uint8_t rs2, rs1, imm;
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_sb(rs2, rs1, imm);
  // }
  // case SCR1_INST::SH: {
  //   uint8_t rs2, rs1, imm;
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_sh(rs2, rs1, imm);
  // }
  // case SCR1_INST::SW: {
  //   uint8_t rs2, imm;
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "rs2");
  //   klee_assume(rs2 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_sw(rs2, RISCV_X0, imm);
  // }
  // case SCR1_INST::ADDI: {
  //   uint8_t rd, rs, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs, sizeof(uint8_t), "rs");
  //   klee_assume(rs < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_addi(rd, rs, imm);
  // }
  // case SCR1_INST::SLTI: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_slti(rd, rs1, imm);
  // }
  // case SCR1_INST::SLTIU: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_sltiu(rd, rs1, imm);
  // }
  // case SCR1_INST::XORI: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_xori(rd, rs1, imm);
  // }
  // case SCR1_INST::ORI: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_ori(rd, rs1, imm);
  // }
  // case SCR1_INST::ANDI: {
  //   uint8_t rd, rs1, imm;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&imm, sizeof(uint8_t), "imm");
  //   return riscv_andi(rd, rs1, imm);
  // }
  // case SCR1_INST::ADD: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_add(rd, rs1, rs2);
  // }
  // case SCR1_INST::SUB: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_sub(rd, rs1, rs2);
  // }
  // case SCR1_INST::SLL: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_sll(rd, rs1, rs2);
  // }
  // case SCR1_INST::SLT: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_slt(rd, rs1, rs2);
  // }
  // case SCR1_INST::SLTU: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_sltu(rd, rs1, rs2);
  // }
  // case SCR1_INST::XOR: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_xor(rd, rs1, rs2);
  // }
  // case SCR1_INST::SRL: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_srl(rd, rs1, rs2);
  // }
  // case SCR1_INST::SRA: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_sra(rd, rs1, rs2);
  // }
  // case SCR1_INST::OR: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_or(rd, rs1, rs2);
  // }
  // case SCR1_INST::AND: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_and(rd, rs1, rs2);
  // }
  // case SCR1_INST::FENCE_I: {
  //   return riscv_fence_i();
  // }
  // case SCR1_INST::ECALL: {
  //   return riscv_ecall();
  // }
  // case SCR1_INST::EBREAK: {
  //   return riscv_ebreak();
  // }
  // case SCR1_INST::MUL: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_mul(rd, rs1, rs2);
  // }
  // case SCR1_INST::MULH: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_mulh(rd, rs1, rs2);
  // }
  // case SCR1_INST::MULHSU: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_mulhsu(rd, rs1, rs2);
  // }
  // case SCR1_INST::MULHU: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_mulhu(rd, rs1, rs2);
  // }
  // case SCR1_INST::DIV: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_div(rd, rs1, rs2);
  // }
  // case SCR1_INST::DIVU: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_divu(rd, rs1, rs2);
  // }
  // case SCR1_INST::REM: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_rem(rd, rs1, rs2);
  // }
  // case SCR1_INST::REMU: {
  //   uint8_t rd, rs1, rs2;
  //   klee_make_symbolic(&rd, sizeof(uint8_t), "rd");
  //   klee_assume(rd < 32);
  //   klee_make_symbolic(&rs1, sizeof(uint8_t), "rs1");
  //   klee_assume(rs1 < 32);
  //   klee_make_symbolic(&rs2, sizeof(uint8_t), "imm");
  //   klee_assume(rs2 < 32);
  //   return riscv_remu(rd, rs1, rs2);
  // }
  // default: {
  //   exit(1);
  // }
  // }
}

uint32_t init_instrs[] = {
  riscv_csrrwi(RISCV_X0, csr_to_code(MTVEC), 0),
};

size_t init_instr_index = 0;
constexpr size_t INIT_INSTR_SIZE = sizeof(init_instrs) / sizeof(init_instrs[0]);

uint32_t read_materialize_inst(uint32_t address) {
  if (!scr1_memory.mask[address] && !scr1_memory.mask[address + 1] && !scr1_memory.mask[address + 2] && !scr1_memory.mask[address + 3]) {
    uint32_t inst = 0;

    if (init_instr_index < INIT_INSTR_SIZE) {
      inst = init_instrs[init_instr_index];
      ++init_instr_index;
    } else {
      inst = get_symbolic_inst(address);
    }

    rtl_write_mem(address, inst, WORD);

    write_mem(address, rtl_read_mem_byte(address));
    write_mem(address + 1, rtl_read_mem_byte(address + 1));
    write_mem(address + 2, rtl_read_mem_byte(address + 2));
    write_mem(address + 3, rtl_read_mem_byte(address + 3));
  } else {
    // Think this through
    exit(1);
  }

  return rtl_read_mem(address, WORD);
}

size_t constexpr GPR_DUMP_START = 0x500;
size_t constexpr CSR_DUMP_START = 0x600;

state run_scr1() {
  uint8_t dmem_htrans[2] = {IDLE, IDLE};
  uint32_t dmem_haddr[2];
  uint8_t dmem_hsize[2];
  uint32_t dmem_hwdata[2];
  uint8_t dmem_hwrite[2];

  size_t predump_nops_fed = 0;
  const size_t PREDUMP_NOPS = 5;

  size_t read_insts = 0;

  bool start_dump = false;
  size_t gpr_count = RISCV_X0;
  size_t csr_count = MSTATUS;

  bool csr_flush_to_mem = false;

  state state;

  size_t cycle_count = 0;
  while (true) {
    if (top->clk) {
      cycle_count++;
      if (cycle_count == 1000) {
        fprintf(stderr, "Cycle count reached\n");
        exit(1);
      }
      // imem read
      if (top->imem_htrans == NONSEQ) {
        // fprintf(stderr, "requesting address %u\n", top->imem_haddr);
        if (start_dump) {
          // Insert some NOP instructions before dumping registers so that we clear the instruction queue
          if (predump_nops_fed < PREDUMP_NOPS) {
            // fprintf(stderr, "inserting predump NOP\n");
            top->imem_hrdata = riscv_addi(RISCV_X0, RISCV_X0, 0);
            ++predump_nops_fed;
          } else if (gpr_count <= RISCV_X31) {
            top->imem_hrdata = riscv_sw(gpr_count, RISCV_X0, GPR_DUMP_START + gpr_count * 4);
            // fprintf(stderr, "dump register %u instruction fed\n", register_dump);
            ++gpr_count;
            predump_nops_fed = 3;
          } else if (csr_count <= CSR_LAST) {
            CSR reg = (CSR)csr_count;
            if (!csr_flush_to_mem) {
              top->imem_hrdata = riscv_csrrs(RISCV_X1, csr_to_code(reg), RISCV_X0);
            } else {
              top->imem_hrdata = riscv_sw(RISCV_X1, RISCV_X0, CSR_DUMP_START + csr_count * 4);
              // fprintf(stderr, "dump cs register %s instruction fed\n", CSR_NAME_MAPPING.at(reg));
              ++csr_count;
              predump_nops_fed = 3;
            }
            csr_flush_to_mem = !csr_flush_to_mem;
          } else {
            // fprintf(stderr, "inserting NOP\n");
            top->imem_hrdata = riscv_addi(RISCV_X0, RISCV_X0, 0);
          }
        } else {
          top->imem_hrdata = read_materialize_inst(top->imem_haddr);
          // fprintf(stderr, "read imem: %u at address %u\n", top->imem_hrdata, top->imem_haddr);
        }
        top->imem_hresp = 0;
        top->imem_hready = 1;
        read_insts++;
        if (read_insts == insn_limit) {
          start_dump = true;
        }
      } else {
        assert(top->imem_htrans == IDLE);
        top->imem_hresp = 0;
        top->imem_hready = 1;
      }

      dmem_htrans[0] = dmem_htrans[1];
      dmem_haddr[0] = dmem_haddr[1];
      dmem_hsize[0] = dmem_hsize[1];
      dmem_hwdata[0] = dmem_hwdata[1];
      dmem_hwrite[0] = dmem_hwrite[1];

      dmem_htrans[1] = top->dmem_htrans;
      dmem_haddr[1] = top->dmem_haddr;
      dmem_hsize[1] = top->dmem_hsize;
      dmem_hwdata[1] = top->dmem_hwdata;
      dmem_hwrite[1] = top->dmem_hwrite;

      // dmem read and write
      if (dmem_htrans[1] == NONSEQ && dmem_hwrite[1] == 0) {
        top->dmem_hrdata = rtl_read_mem(dmem_haddr[1], dmem_hsize[1]);
        // fprintf(stderr, "read dmem: %u at address %u\n", top->dmem_hrdata, dmem_haddr[1]);
        top->dmem_hresp = 0;
        top->dmem_hready = 1;
      } else if (dmem_htrans[0] == NONSEQ && dmem_hwrite[0] == 1) {
        if (dmem_haddr[0] >= GPR_DUMP_START && dmem_haddr[0] < CSR_DUMP_START) {
          uint32_t reg = (dmem_haddr[0] - GPR_DUMP_START) / 4;
          // fprintf(stderr, "dumping general purpose register %u\n", reg);
          state.GPRs[reg] = dmem_hwdata[1];
        } else if (dmem_haddr[0] >= CSR_DUMP_START) {
          CSR reg = (CSR)((dmem_haddr[0] - CSR_DUMP_START) / 4);
          // fprintf(stderr, "dumping register %s\n", csr_to_name(reg));
          state.CSRs[reg] = dmem_hwdata[1];
          if (reg == CSR_LAST) {
            break;
          }
        } else {
          rtl_write_mem(dmem_haddr[0], dmem_hwdata[1], dmem_hsize[0]);
          // fprintf(stderr, "wrote dmem: %u to address %u\n", dmem_hwdata[1],
          //         dmem_haddr[0]);
        }
        top->dmem_hresp = 0;
        top->dmem_hready = 1;
      } else {
        top->dmem_hresp = 0;
        top->dmem_hready = 1;
      }
    }
    top->clk = !top->clk;
    top->eval();
  }
  return state;
}

void run_sail(void)
{
  bool stepped;

  /* initialize the step number */
  mach_int step_no = 0;
  int insn_cnt = 0;

  while (insn_limit == 0 || total_insns < insn_limit) {
    {
      /* run a Sail step */
      sail_int sail_step = CONVERT_OF(sail_int, mach_int)(step_no);
      stepped = zstep(sail_step);
    }
    if (stepped) {
      step_no++;
      insn_cnt++;
      total_insns++;
    }

    if (insn_cnt == rv_insns_per_tick) {
      insn_cnt = 0;
      ztick_clock(UNIT);
      ztick_platform(UNIT);
    }
  }
}

bool checkGPRs(const state &scr1_state) {
  if (scr1_state.GPRs[1] != zx1) {
    fprintf(stderr, "GPR 1 differs\n");
    return false;
  }
  if (scr1_state.GPRs[2] != zx2) {
    fprintf(stderr, "GPR 2 differs\n");
    return false;
  }
  if (scr1_state.GPRs[3] != zx3) {
    fprintf(stderr, "GPR 3 differs\n");
    return false;
  }
  if (scr1_state.GPRs[4] != zx4) {
    fprintf(stderr, "GPR 4 differs\n");
    return false;
  }
  if (scr1_state.GPRs[5] != zx5) {
    fprintf(stderr, "GPR 5 differs\n");
    return false;
  }
  if (scr1_state.GPRs[6] != zx6) {
    fprintf(stderr, "GPR 6 differs\n");
    return false;
  }
  if (scr1_state.GPRs[7] != zx7) {
    fprintf(stderr, "GPR 7 differs\n");
    return false;
  }
  if (scr1_state.GPRs[8] != zx8) {
    fprintf(stderr, "GPR 8 differs\n");
    return false;
  }
  if (scr1_state.GPRs[9] != zx9) {
    fprintf(stderr, "GPR 9 differs\n");
    return false;
  }
  if (scr1_state.GPRs[10] != zx10) {
    fprintf(stderr, "GPR 10 differs\n");
    return false;
  }
  if (scr1_state.GPRs[11] != zx11) {
    fprintf(stderr, "GPR 11 differs\n");
    return false;
  }
  if (scr1_state.GPRs[12] != zx12) {
    fprintf(stderr, "GPR 12 differs\n");
    return false;
  }
  if (scr1_state.GPRs[13] != zx13) {
    fprintf(stderr, "GPR 13 differs\n");
    return false;
  }
  if (scr1_state.GPRs[14] != zx14) {
    fprintf(stderr, "GPR 14 differs\n");
    return false;
  }
  if (scr1_state.GPRs[15] != zx15) {
    fprintf(stderr, "GPR 15 differs\n");
    return false;
  }
  if (scr1_state.GPRs[16] != zx16) {
    fprintf(stderr, "GPR 16 differs\n");
    return false;
  }
  if (scr1_state.GPRs[17] != zx17) {
    fprintf(stderr, "GPR 17 differs\n");
    return false;
  }
  if (scr1_state.GPRs[18] != zx18) {
    fprintf(stderr, "GPR 18 differs\n");
    return false;
  }
  if (scr1_state.GPRs[19] != zx19) {
    fprintf(stderr, "GPR 19 differs\n");
    return false;
  }
  if (scr1_state.GPRs[20] != zx20) {
    fprintf(stderr, "GPR 20 differs\n");
    return false;
  }
  if (scr1_state.GPRs[21] != zx21) {
    fprintf(stderr, "GPR 21 differs\n");
    return false;
  }
  if (scr1_state.GPRs[22] != zx22) {
    fprintf(stderr, "GPR 22 differs\n");
    return false;
  }
  if (scr1_state.GPRs[23] != zx23) {
    fprintf(stderr, "GPR 23 differs\n");
    return false;
  }
  if (scr1_state.GPRs[24] != zx24) {
    fprintf(stderr, "GPR 24 differs\n");
    return false;
  }
  if (scr1_state.GPRs[25] != zx25) {
    fprintf(stderr, "GPR 25 differs\n");
    return false;
  }
  if (scr1_state.GPRs[26] != zx26) {
    fprintf(stderr, "GPR 26 differs\n");
    return false;
  }
  if (scr1_state.GPRs[27] != zx27) {
    fprintf(stderr, "GPR 27 differs\n");
    return false;
  }
  if (scr1_state.GPRs[28] != zx28) {
    fprintf(stderr, "GPR 28 differs\n");
    return false;
  }
  if (scr1_state.GPRs[29] != zx29) {
    fprintf(stderr, "GPR 29 differs\n");
    return false;
  }
  if (scr1_state.GPRs[30] != zx30) {
    fprintf(stderr, "GPR 30 differs\n");
    return false;
  }
  if (scr1_state.GPRs[31] != zx31) {
    fprintf(stderr, "GPR 31 differs\n");
    return false;
  }
  return true;
}

bool checkCSRs(const state &scr1_state) {
  bool ret = true;
  // if (scr1_state.CSRs[MSTATUS] != zreadCSR(csr_to_code(MSTATUS))) {
  //   fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
  //           csr_to_name(MSTATUS), scr1_state.CSRs[MSTATUS],
  //           zreadCSR(csr_to_code(MSTATUS)));
  //   ret = false;
  // }
  if (scr1_state.CSRs[MIE] != zreadCSR(csr_to_code(MIE))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MIE), scr1_state.CSRs[MIE],
    //         zreadCSR(csr_to_code(MIE)));
    ret = false;
  }
  if (scr1_state.CSRs[MTVEC] != zreadCSR(csr_to_code(MTVEC))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MTVEC), scr1_state.CSRs[MTVEC],
    //         zreadCSR(csr_to_code(MTVEC)));
    ret = false;
  }
  if (scr1_state.CSRs[MSCRATCH] != zreadCSR(csr_to_code(MSCRATCH))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MSCRATCH), scr1_state.CSRs[MSCRATCH],
    //         zreadCSR(csr_to_code(MSCRATCH)));
    ret = false;
  }
  if (scr1_state.CSRs[MEPC] != zreadCSR(csr_to_code(MEPC))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MEPC), scr1_state.CSRs[MEPC],
    //         zreadCSR(csr_to_code(MEPC)));
    ret = false;
  }
  if (scr1_state.CSRs[MCAUSE] != zreadCSR(csr_to_code(MCAUSE))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MCAUSE), scr1_state.CSRs[MCAUSE],
    //         zreadCSR(csr_to_code(MCAUSE)));
    ret = false;
  }
  if (scr1_state.CSRs[MTVAL] != zreadCSR(csr_to_code(MTVAL))) {
    // fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
    //         csr_to_name(MTVAL), scr1_state.CSRs[MTVAL],
    //         zreadCSR(csr_to_code(MTVAL)));
    ret = false;
  }
  // if (scr1_state.CSRs[MIP] != zreadCSR(csr_to_code(MIP))) {
  //   fprintf(stderr, "%s is different: %u in SCR1 and %lu in SAIL\n",
  //           csr_to_name(MIP), scr1_state.CSRs[MIP],
  //           zreadCSR(csr_to_code(MIP)));
  //   ret = false;
  // }
  return ret;
}

void print_CSRs(const state &scr1_state) {
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MIE),
         scr1_state.CSRs[MIE], zreadCSR(csr_to_code(MIE)));
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MTVEC),
         scr1_state.CSRs[MTVEC], zreadCSR(csr_to_code(MTVEC)));
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MSCRATCH),
         scr1_state.CSRs[MSCRATCH], zreadCSR(csr_to_code(MSCRATCH)));
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MEPC),
         scr1_state.CSRs[MEPC], zreadCSR(csr_to_code(MEPC)));
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MCAUSE),
         scr1_state.CSRs[MCAUSE], zreadCSR(csr_to_code(MCAUSE)));
  printf("%s: %u in SCR1 and %lu in SAIL\n", csr_to_name(MTVAL),
         scr1_state.CSRs[MTVAL], zreadCSR(csr_to_code(MTVAL)));
}

bool checkMemory() {
  assert(sail_memory.size == scr1_memory.size);
  for (size_t i = 0; i < sail_memory.size; ++i) {
    if (read_mem(i) != rtl_read_mem_byte(i)) {
      return false;
    }
  }
  return true;
}

int main(int argc, char **argv)
{
  insn_limit = INIT_INSTR_SIZE + 1;

  // INIT MEMORY BUFFERS
  sail_memory.buffer = new uint8_t[0x500];
  std::memset(sail_memory.buffer, 0x0, 0x500);
  sail_memory.mask = new bool[0x500];
  std::memset(sail_memory.mask, 0x0, 0x500);
  sail_memory.size = 0x500;

  scr1_memory.buffer = new uint8_t[0x500];
  std::memset(scr1_memory.buffer, 0x0, 0x500);
  scr1_memory.mask = new bool[0x500];
  std::memset(scr1_memory.mask, 0x0, 0x500);
  scr1_memory.size = 0x500;

  // SCR1
  top = new Vtop_exp_ahb;
  state scr1_state = run_scr1();

  // SAIL
  trace_log = stdout;
  model_init();
  zinit_model(UNIT);
  rv_rom_base = 0x0;
  rv_rom_size = 0x500;
  rv_ram_base = 0x0;
  rv_ram_size = 0x500;
  zPC = INSTR_START;
  run_sail();
  model_fini();

  // assert(checkGPRs(scr1_state));
  // assert(checkCSRs(scr1_state));
  // assert(checkMemory());

  if (checkGPRs(scr1_state)) {
    fprintf(stderr, "GPRs OK\n");
  } else {
    fprintf(stderr, "GPRs differ!\n");
  }

  if (checkCSRs(scr1_state)) {
    fprintf(stderr, "CSRs OK\n");
  } else {
    fprintf(stderr, "CSRs differ!\n");
  }

  print_CSRs(scr1_state);

  if (checkMemory()) {
    fprintf(stderr, "Memory OK\n");
  } else {
    fprintf(stderr, "Memory differs!\n");
  }

  return 0;
}
