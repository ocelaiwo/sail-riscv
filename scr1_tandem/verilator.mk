tandem_dir := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
root_dir := $(abspath $(tandem_dir)/..)
scr1_dir := $(root_dir)/scr1
build_dir := $(tandem_dir)/build

rtl_src_dir := $(scr1_dir)/src
rtl_core_files ?= core.files
rtl_top_files ?= ahb_top.files
rtl_tb_files ?= ahb_tb.files
rtl_inc_dir ?= $(rtl_src_dir)/includes
rtl_inc_tb_dir ?= $(rtl_src_dir)/tb
top_module ?= top_exp_ahb

rtl_core_list := $(addprefix $(rtl_src_dir)/,$(shell cat $(rtl_src_dir)/$(rtl_core_files)))
rtl_top_list := $(addprefix $(rtl_src_dir)/,$(shell cat $(rtl_src_dir)/$(rtl_top_files)))
rtl_tb_list := $(addprefix $(rtl_src_dir)/,$(shell cat $(rtl_src_dir)/$(rtl_tb_files)))
sv_list := $(rtl_core_list) $(rtl_top_list) $(rtl_tb_list)

FLAGS=-I.  -MMD -I/usr/share/verilator/include -I/usr/share/verilator/include/vltstd -DVM_COVERAGE=0 -DVM_SC=0 -faligned-new -fcf-protection=none -Wno-bool-operation -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-shadow -std=gnu++17 -I$(HOME)/.opam/default/share/sail/lib/ -I$(root_dir)/c_emulator -c

ifdef KLEE_LIBCXX
	FLAGS += -nostdinc++ -I $(KLEE_LIBCXX)
endif

.PHONY: verilate

all: $(build_dir)/verilated.a

verilate: riscv_exp.c $(sv_list)
	mkdir -p $(build_dir); \
	cd $(build_dir); \
	verilator \
	-cc \
	-sv \
	+1800-2017ext+sv \
	-Wno-fatal \
	--top-module $(top_module) \
	-DSCR1_TRGT_SIMULATION \
	--clk clk \
	--exe $(tandem_dir)/riscv_exp.c \
	--no-threads    \
    --no-timing     \
	--fno-merge-const-pool -x-assign fast -x-initial fast --noassert \
	--Mdir . \
	-I$(rtl_inc_dir) \
	-I$(rtl_inc_tb_dir) \
	-I$(HOME)/.opam/default/share/sail/lib/ \
	$(SIM_BUILD_OPTS) \
	$(sv_list); \

$(build_dir)/verilated.a: verilate $(tandem_dir)/riscv_exp.c
	cd $(build_dir); \
	$(CXX) $(FLAGS) -o riscv_exp.o $(tandem_dir)/riscv_exp.c; \
	$(CXX) $(FLAGS) -o verilated.o /usr/share/verilator/include/verilated.cpp; \
	$(CXX) $(FLAGS) -o verilated_dpi.o /usr/share/verilator/include/verilated_dpi.cpp; \
	$(CXX) $(FLAGS) *.cpp; \
	ar -rcs verilated.a *.o
