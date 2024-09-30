// -*- mode: c++ -*-

#include <cstdint>
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

uint64_t rtl_mem_MASK = 0xFFFFFFul;

struct rtl_mem_block {
  uint64_t block_id;
  uint8_t *mem;
  struct rtl_mem_block *next;
};

struct rtl_mem_block *rtl_memory = NULL;

void rtl_write_mem_byte(uint32_t address, uint8_t byte)
{
  uint64_t mask = address & ~rtl_mem_MASK;
  uint64_t offset = address & rtl_mem_MASK;

  struct rtl_mem_block *current = rtl_memory;

  while (current != NULL) {
    if (current->block_id == mask) {
      current->mem[offset] = byte;
      return;
    } else {
      current = current->next;
    }
  }

  /*
   * If we couldn't find a block matching the mask, allocate a new
   * one, write the byte, and put it at the front of the block list.
   */
  struct rtl_mem_block *new_block = (struct rtl_mem_block *)malloc(sizeof(struct rtl_mem_block));
  new_block->block_id = mask;
  new_block->mem = (uint8_t *)calloc(rtl_mem_MASK + 1, sizeof(uint8_t));
  new_block->mem[offset] = byte;
  new_block->next = rtl_memory;
  rtl_memory = new_block;
}

uint32_t rtl_read_mem_byte(uint32_t address)
{
  uint64_t mask = address & ~rtl_mem_MASK;
  uint64_t offset = address & rtl_mem_MASK;

  struct rtl_mem_block *current = rtl_memory;

  while (current != NULL) {
    if (current->block_id == mask) {
      return (uint32_t) current->mem[offset];
    } else {
      current = current->next;
    }
  }

  return 0x00;
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

struct state {
  uint32_t GPRs[32];
};


state run_scr1() {
  uint8_t dmem_htrans[2] = {IDLE, IDLE};
  uint32_t dmem_haddr[2];
  uint8_t dmem_hsize[2];
  uint32_t dmem_hwdata[2];
  uint8_t dmem_hwrite[2];

  size_t read_insts = 0;
  int register_dump = RISCV_X0;
  state state;

  while (true) {
    if (top->clk) {
      // imem read
      if (top->imem_htrans == NONSEQ) {
        fprintf(stderr, "requesting address %u\n", top->imem_haddr);
        if (read_insts == insn_limit) {
          if (register_dump <= RISCV_X31) {
            top->imem_hrdata = riscv_sw(register_dump, RISCV_X0, register_dump * 4);
            fprintf(stderr, "dump register %u instruction fed\n", register_dump);
            ++register_dump;
          } else {
            // NOP
            fprintf(stderr, "inserting NOP\n");
            top->imem_hrdata = riscv_addi(RISCV_X0, RISCV_X0, 0);
          }
        } else {
          top->imem_hrdata = rtl_read_mem(top->imem_haddr, top->imem_hsize);
          fprintf(stderr, "read imem: %u at address %u\n", top->imem_hrdata, top->imem_haddr);
        }
        top->imem_hresp = 0;
        top->imem_hready = 1;
        if (read_insts != insn_limit) {
          read_insts++;
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
        fprintf(stderr, "read dmem: %u at address %u\n", top->dmem_hrdata, dmem_haddr[1]);
        top->dmem_hresp = 0;
        top->dmem_hready = 1;
      } else if (dmem_htrans[0] == NONSEQ && dmem_hwrite[0] == 1) {
        uint32_t reg = (dmem_haddr[0]) / 4;
        if (reg >= RISCV_X0 && reg <= RISCV_X31) {
          fprintf(stderr, "dumping register %u\n", reg);
          state.GPRs[reg] = dmem_hwdata[1];
          if (reg == RISCV_X31) {
            break;
          }
        } else {
          rtl_write_mem(dmem_haddr[0], dmem_hwdata[1], dmem_hsize[0]);
          fprintf(stderr, "wrote dmem: %u to address %u\n", dmem_hwdata[1],
                  dmem_haddr[0]);
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

bool checkGPRs(state scr1_state) {
  if (scr1_state.GPRs[1] != zx1) {
    return false;
  }
  if (scr1_state.GPRs[2] != zx2) {
    return false;
  }
  if (scr1_state.GPRs[3] != zx3) {
    return false;
  }
  if (scr1_state.GPRs[4] != zx4) {
    return false;
  }
  if (scr1_state.GPRs[5] != zx5) {
    return false;
  }
  if (scr1_state.GPRs[6] != zx6) {
    return false;
  }
  if (scr1_state.GPRs[7] != zx7) {
    return false;
  }
  if (scr1_state.GPRs[8] != zx8) {
    return false;
  }
  if (scr1_state.GPRs[9] != zx9) {
    return false;
  }
  if (scr1_state.GPRs[10] != zx10) {
    return false;
  }
  if (scr1_state.GPRs[11] != zx11) {
    return false;
  }
  if (scr1_state.GPRs[12] != zx12) {
    return false;
  }
  if (scr1_state.GPRs[13] != zx13) {
    return false;
  }
  if (scr1_state.GPRs[14] != zx14) {
    return false;
  }
  if (scr1_state.GPRs[15] != zx15) {
    return false;
  }
  if (scr1_state.GPRs[16] != zx16) {
    return false;
  }
  if (scr1_state.GPRs[17] != zx17) {
    return false;
  }
  if (scr1_state.GPRs[18] != zx18) {
    return false;
  }
  if (scr1_state.GPRs[19] != zx19) {
    return false;
  }
  if (scr1_state.GPRs[20] != zx20) {
    return false;
  }
  if (scr1_state.GPRs[21] != zx21) {
    return false;
  }
  if (scr1_state.GPRs[22] != zx22) {
    return false;
  }
  if (scr1_state.GPRs[23] != zx23) {
    return false;
  }
  if (scr1_state.GPRs[24] != zx24) {
    return false;
  }
  if (scr1_state.GPRs[25] != zx25) {
    return false;
  }
  if (scr1_state.GPRs[26] != zx26) {
    return false;
  }
  if (scr1_state.GPRs[27] != zx27) {
    return false;
  }
  if (scr1_state.GPRs[28] != zx28) {
    return false;
  }
  if (scr1_state.GPRs[29] != zx29) {
    return false;
  }
  if (scr1_state.GPRs[30] != zx30) {
    return false;
  }
  if (scr1_state.GPRs[31] != zx31) {
    return false;
  }
  return true;
}

int main(int argc, char **argv)
{
  insn_limit = 4;
  uint32_t *inst = (uint32_t *)instr_memory;
  *(inst) = riscv_addi(RISCV_X1, RISCV_X0, 11);
  *(inst + 1) = riscv_sw(RISCV_X1, RISCV_X0, DATA_START);
  *(inst + 2) = riscv_lw(RISCV_X2, RISCV_X0, DATA_START);
  *(inst + 3) = riscv_add(RISCV_X1, RISCV_X1, RISCV_X2);

  for (size_t i = 0; i < insn_limit * INSTR_BYTE_WIDTH; ++i) {
    write_mem(INSTR_START + i, instr_memory[i]);
    rtl_write_mem_byte(INSTR_START + i, instr_memory[i]);
  }

  // SCR1
  top = new Vtop_exp_ahb;
  state scr1_state = run_scr1();

  // SAIL
  trace_log = stdout;
  model_init();
  zinit_model(UNIT);
  rv_rom_base = INSTR_START;
  rv_rom_size = DATA_START - INSTR_START;
  rv_ram_base = DATA_START;
  rv_ram_size = DATA_START + 0x500;
  zPC = INSTR_START;
  run_sail();
  model_fini();

  if (checkGPRs(scr1_state)) {
    fprintf(stderr, "ALL OK\n");
  }

  return 0;
}
