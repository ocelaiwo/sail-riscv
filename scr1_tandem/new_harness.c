#include <cstdint>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <fcntl.h>

#include "elf.h"
#include "sail.h"
#include "rts.h"

#include "riscv_platform.h"
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

constexpr size_t MEMORY_LEN = 0x500;
unsigned char memory[MEMORY_LEN];

constexpr size_t FETCH_START = 0x200;
constexpr size_t FETCH_END = 0x250;

struct state {
  uint32_t GPRs[32];
};

void run_scr1() {
  bool do_write = false;
  uint32_t haddr;
  uint32_t hwdata;
  uint8_t hsize;

  size_t read_insts = 0;
  size_t unwind_count = 0;

  while (true) {
    auto clk_posedge = top->clk;

    if (read_insts == insn_limit) {
      unwind_count++;
      if (unwind_count == 4) {
        break;
      }
    }

    if (top->clk) {
      top->imem_hrdata = 0;

      switch (top->imem_htrans) {
      case IDLE: {
        top->imem_hresp = 0;
        top->imem_hready = 1;
        break;
      }
      case NONSEQ: {
        if (read_insts == insn_limit) {
          top->imem_hresp = 0;
          top->imem_hready = 0;
        } else {
          read_insts++;
          top->imem_hrdata = rtl_read_mem(top->imem_haddr, top->imem_hsize);
          haddr = top->imem_haddr;
          hsize = top->imem_hsize;
          top->imem_hready = 1;
          top->imem_hresp = 0;
        }
        break;
      }
      default: {
        exit(1);
      }
      }
    }

    if (top->clk) {
      if (do_write) {
        do_write = false;
        if (haddr == 0x80001000) {
          printf("SUCCESS\n");
          break; // Break from outer loop
        } else if (haddr == SCR1_SIM_PRINT_ADDR) {
          printf("%c", top->dmem_hwdata);
        } else {
          /* memwrite(haddr, top->dmem_hwdata, hsize); */
          rtl_write_mem(haddr, top->dmem_hwdata, hsize);
        }
      }

      switch (top->dmem_htrans) {
      case IDLE: {
        top->dmem_hresp = 0;
        top->dmem_hready = 1;
        break;
      }
      case NONSEQ: {
        if (!top->dmem_hwrite) {
          /* top->dmem_hrdata = memread(top->dmem_haddr, top->dmem_hsize); */
          top->dmem_hrdata = rtl_read_mem(top->dmem_haddr, top->dmem_hsize);
          top->dmem_hresp = 0;
          top->dmem_hready = 1;
        } else {
          do_write = true;
          haddr = top->dmem_haddr;
          hsize = top->dmem_hsize;
          top->dmem_hresp = 0;
          top->dmem_hready = 1;
        }
        break;
      }
      default: {
        exit(1);
      }
      }
    }

    top->clk = !top->clk;
    top->eval();
  }
}

state read_scr1_state() {
  state state;
  bool do_write = false;
  uint32_t haddr;
  uint32_t hwdata;
  uint8_t hsize;

  for (int i = RISCV_X0; i <= RISCV_X31; ++i) {
    bool written = false;
    while (true) {
      auto clk_posedge = top->clk;

      if (top->clk) {
        top->imem_hrdata = 0;

        switch (top->imem_htrans) {
        case IDLE: {
          top->imem_hresp = 0;
          top->imem_hready = 1;
          break;
        }
        case NONSEQ: {
          if (written) {
            top->imem_hready = 0;
          } else {
            /* top->imem_hrdata = memread(top->imem_haddr, top->imem_hsize); */
            top->imem_hrdata = riscv_sw(i, RISCV_X0, 0);
            top->imem_hready = 1;
            top->imem_hresp = 0;
            written = true;
          }
          break;
        }
        default: {
          exit(1);
        }
        }
      }

      if (top->clk) {

        if (do_write) {
          do_write = false;
          if (haddr == 0x0) {
            state.GPRs[i] = top->dmem_hwdata;
            break;
          } else {
            assert(0);
          }
        }

        switch (top->dmem_htrans) {
        case IDLE: {
          top->dmem_hresp = 0;
          top->dmem_hready = 1;
          break;
        }
        case NONSEQ: {
          if (!top->dmem_hwrite) {
            /* top->dmem_hrdata = memread(top->dmem_haddr, top->dmem_hsize); */
            top->dmem_hrdata = rtl_read_mem(top->dmem_haddr, top->dmem_hsize);
            top->dmem_hresp = 0;
            top->dmem_hready = 1;
          } else {
            do_write = true;
            haddr = top->dmem_haddr;
            hsize = top->dmem_hsize;
            top->dmem_hresp = 0;
            top->dmem_hready = 1;
          }
          break;
        }
        default: {
          exit(1);
        }
        }
      }

      top->clk = !top->clk;
      top->eval();
    }
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

void checkGPRs(state scr1_state) {
  assert(scr1_state.GPRs[1] = zx1);
  assert(scr1_state.GPRs[2] = zx2);
  assert(scr1_state.GPRs[3] = zx3);
  assert(scr1_state.GPRs[4] = zx4);
  assert(scr1_state.GPRs[5] = zx5);
  assert(scr1_state.GPRs[6] = zx6);
  assert(scr1_state.GPRs[7] = zx7);
  assert(scr1_state.GPRs[8] = zx8);
  assert(scr1_state.GPRs[9] = zx9);
  assert(scr1_state.GPRs[10] = zx10);
  assert(scr1_state.GPRs[11] = zx11);
  assert(scr1_state.GPRs[12] = zx12);
  assert(scr1_state.GPRs[13] = zx13);
  assert(scr1_state.GPRs[14] = zx14);
  assert(scr1_state.GPRs[15] = zx15);
  assert(scr1_state.GPRs[16] = zx16);
  assert(scr1_state.GPRs[17] = zx17);
  assert(scr1_state.GPRs[18] = zx18);
  assert(scr1_state.GPRs[19] = zx19);
  assert(scr1_state.GPRs[20] = zx20);
  assert(scr1_state.GPRs[21] = zx21);
  assert(scr1_state.GPRs[22] = zx22);
  assert(scr1_state.GPRs[23] = zx23);
  assert(scr1_state.GPRs[24] = zx24);
  assert(scr1_state.GPRs[25] = zx25);
  assert(scr1_state.GPRs[26] = zx26);
  assert(scr1_state.GPRs[27] = zx27);
  assert(scr1_state.GPRs[28] = zx28);
  assert(scr1_state.GPRs[29] = zx29);
  assert(scr1_state.GPRs[30] = zx30);
  assert(scr1_state.GPRs[31] = zx31);
}

int main(int argc, char **argv)
{
  insn_limit = 1;
  uint32_t *inst = (uint32_t *)memory;
  *inst = riscv_lui(RISCV_X1, 77);

  /* for (size_t i = 0; i < insn_limit; ++i) { */
  /*   for (size_t j = 0; j < 4; ++j) { */
  /*     unsigned char byte; */
  /*     klee_make_symbolic(&byte, sizeof(byte), "byte"); */
  /*     memory[j + (i * 4)] = byte; */
  /*   } */
  /* } */

  for (size_t i = 0; i < MEMORY_LEN; ++i) {
    write_mem(FETCH_START + i, memory[i]);
    rtl_write_mem_byte(FETCH_START + i, memory[i]);
  }

  // SCR1
  top = new Vtop_exp_ahb;
  run_scr1();
  state scr1_state = read_scr1_state();

  // SAIL
  model_init();
  zinit_model(UNIT);
  rv_ram_base = FETCH_START;
  rv_ram_size = FETCH_END - FETCH_START;
  zPC = FETCH_START;
  run_sail();
  model_fini();

  checkGPRs(scr1_state);

  return 0;
}
