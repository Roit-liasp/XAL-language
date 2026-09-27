CC ?= cc
CFLAGS ?= -O2 -std=c11 -Iinclude -Wall -Wextra
OUT ?= build

-include $(OUT)/config.mk

XOS = $(OUT)/xos_$(XOS_BACKEND).o
LIBXC = $(OUT)/xc.o
LIBXCORE = $(OUT)/xcore.o

XALС_SRC = src/xalc/lex.c src/xalc/codegen.c src/xalc/decl.c src/xalc/image.c src/xalc/main.c
XVM_SRC = src/xvm/value.c src/xvm/thread.c src/xvm/interp.c src/xvm/natives.c src/xvm/main.c
NEST_SRC = src/xvm/value.c src/xvm/thread.c src/xvm/interp.c src/xvm/natives.c

BINS = $(OUT)/xalc $(OUT)/xvm

.PHONY: all clean test bare config nested

all: $(BINS)

$(OUT)/config.mk: | $(OUT)/.stamp
	sh conf.sh $(OUT)

config:
	sh conf.sh $(OUT)

$(OUT)/.stamp:
	@mkdir -p $(OUT)
	@touch $(OUT)/.stamp

$(XOS): src/libxos/xos_$(XOS_BACKEND).c include/xos.h | $(OUT)/.stamp
	$(CC) $(CFLAGS) $(XOS_CAPS) -c src/libxos/xos_$(XOS_BACKEND).c -o $(XOS)

$(LIBXC): src/libxc/xc.c include/xc.h include/xos.h | $(OUT)/.stamp
	$(CC) $(CFLAGS) $(XOS_CAPS) -c src/libxc/xc.c -o $(LIBXC)

$(LIBXCORE): src/libxcore/xcore.c include/xcore.h | $(OUT)/.stamp
	$(CC) $(CFLAGS) $(XOS_CAPS) -c src/libxcore/xcore.c -o $(LIBXCORE)

$(OUT)/xalc: $(XAL_SRC) src/xalc/xal.h include/xcore.h $(LIBXC) $(LIBXCORE) $(XOS)
	$(CC) $(CFLAGS) $(XOS_CAPS) $(LDFLAGS) $(XALС_SRC) $(LIBXCORE) $(LIBXC) $(XOS) $(XOS_LIBS) -o $(OUT)/xalc

$(OUT)/xvm: $(XVM_SRC) src/xvm/xvm.h include/xcore.h $(LIBXC) $(LIBXCORE) $(XOS)
	$(CC) $(CFLAGS) $(XOS_CAPS) $(LDFLAGS) $(XVM_SRC) $(LIBXCORE) $(LIBXC) $(XOS) $(XOS_LIBS) -o $(OUT)/xvm

test: $(BINS) nested
	@sh tests/run.sh $(OUT)
	@sh tests/run_xb.sh $(OUT)
	@sh tests/proc.sh $(OUT)

nested: $(OUT)/nested
	@$(OUT)/xalc -o $(OUT)/nest_parent.xb tests/nest_parent.xa
	@$(OUT)/xalc -o $(OUT)/nest_child.xb tests/nest_child.xa
	@$(OUT)/nested $(OUT)/nest_parent.xb $(OUT)/nest_child.xb

$(OUT)/nested: tests/nested.c $(NEST_SRC) src/xvm/xvm.h include/xcore.h $(LIBXC) $(LIBXCORE) $(XOS) | $(OUT)/.stamp
	$(CC) $(CFLAGS) -Isrc/xvm $(XOS_CAPS) $(LDFLAGS) tests/nested.c $(NEST_SRC) $(LIBXCORE) $(LIBXC) $(XOS) $(XOS_LIBS) -o $(OUT)/nested

bare: $(OUT)/xalc
	$(OUT)/xalc -o $(OUT)/bare_prog.xb tests/bare_prog.xa
	sh tests/embed.sh $(OUT)/bare_prog.xb $(OUT)/bare_prog_xb.c
	$(CC) $(CFLAGS) -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -static \
	  -DXVM_FREESTANDING -DXOS_HAS_FS=0 -DXOS_HAS_DIR=0 -DXOS_HAS_ENV=0 -DXOS_HAS_PROC=0 \
	  -DXOS_HAS_TERM=0 -DXOS_HAS_THREADS=0 -DXOS_HAS_CWD=0 \
	  tests/bare_start.S src/libxos/xos_bare.c src/libxc/xc.c src/libxcore/xcore.c \
	  $(XVM_SRC) tests/bare_demo.c $(OUT)/bare_prog_xb.c -o $(OUT)/xvm_bare_demo

clean:
	rm -rf $(OUT)
