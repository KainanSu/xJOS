#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#
OBJDIR := obj

# Run 'make V=1' to turn on verbose commands, or 'make V=0' to turn them off.
ifeq ($(V),1)
override V =
endif
ifeq ($(V),0)
override V = @
endif

-include conf/env.mk

LABSETUP ?= ./

TOP = .

# try to generate a unique GDB port
GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)

CC	:= $(GCCPREFIX)gcc -pipe
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm

# Native commands
NCC	:= gcc $(CC_VER) -pipe
NATIVE_CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -I$(TOP) -MD -Wall
TAR	:= gtar
PERL	:= perl

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS := $(CFLAGS) -fno-builtin -I$(TOP) -MD
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -static
CFLAGS += -std=gnu99
CFLAGS += -Wall -Wno-format -Wno-unused -gstabs 
CFLAGS += -fno-tree-ch -mcpu=cortex-a7 
# CFLAGS += -ffreestanding
# division support

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

LDFLAGS :=
# LDFLAGS := -m elf_arm

# Linker flags for JOS user programs
ULDFLAGS := -T user/user.ld

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=

# Make sure that 'all' is the first target
all:

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o $(OBJDIR)/boot/%.o $(OBJDIR)/kern/%.o \
	   $(OBJDIR)/lib/%.o $(OBJDIR)/fs/%.o $(OBJDIR)/net/%.o \
	   $(OBJDIR)/user/%.o

KERN_CFLAGS := $(CFLAGS) -DJOS_KERNEL
USER_CFLAGS := $(CFLAGS) -DJOS_USER

# Update .vars.X if variable X has changed since the last make run.
#
# Rules that use variable X should depend on $(OBJDIR)/.vars.X.  If
# the variable's value has changed, this will update the vars file and
# force a rebuild of the rule that depends on it.
$(OBJDIR)/.vars.%: FORCE
	$(V)echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PRECIOUS: $(OBJDIR)/.vars.%
.PHONY: FORCE

# Include Makefrags for subdirectories
include kern/Makefrag
include lib/Makefrag
include user/Makefrag
include fs/Makefrag

# QEMUOPTS = -kernel $(OBJDIR)/kern/kernel -cpu arm1176 -m 256 -M raspi2 -serial stdio -gdb tcp::$(GDBPORT)
QEMUOPTS = -kernel $(OBJDIR)/kern/kernel -M mcimx6ul-evk -m 512M -no-reboot -serial mon:stdio -gdb tcp::$(GDBPORT) -sd /home/kn/learn_linux/learn_arm/xJOS/obj/fs/fs.img

# -sd /home/kn/learn_linux/learn_arm/xJOS/obj/clean-fs.img
# -drive file=/home/kn/learn_linux/learn_arm/xJOS/obj/clean-fs.img,format=raw,id=mysdcard -device sd-card,drive=mysdcard

IMAGES = $(OBJDIR)/kern/kernel
IMAGES += $(OBJDIR)/fs/fs.img

gdb:
	arm-none-eabi-gdb -x .gdbinit

pre-qemu: .gdbinit

qemu: $(IMAGES) pre-qemu
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) $(QEMUOPTS) -S

qemu-nox: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS)

qemu-nox-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS) -S
# For deleting the build
clean:
	rm -rf $(OBJDIR) jos.in qemu.log

realclean: clean
	rm -rf lab$(LAB).tar.gz \
		jos.out $(wildcard jos.out.*) \
		qemu.pcap $(wildcard qemu.pcap.*) \
		myapi.key

distclean: realclean
	rm -rf conf/gcc.mk

grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
	  (echo "'make clean' failed.  HINT: Do you have another running instance of JOS?" && exit 1)
	./grade-lab$(LAB) $(GRADEFLAGS)

tarball: handin-check
	git archive --format=tar HEAD | gzip > lab$(LAB)-handin.tar.gz

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

always:
	@:

.PHONY: all always \
	handin git-handin tarball tarball-pref clean realclean distclean grade handin-prep handin-check