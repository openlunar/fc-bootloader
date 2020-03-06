# Makefile preamble
MAKEFLAGS += --warn-undefined-variables
SHELL := bash
# .SHELLFLAGS := -eu -o pipefail -c
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.SUFFIXES:

# openocd -f interface/cmsis-dap.cfg -f board/atmel_samv71_xplained_ultra.cfg

include tools/framework.mk
# verbose := true

CPPFLAGS ?=
CFLAGS   ?=
CXXFLAGS ?=
LDFLAGS  ?=
LDLIBS   ?=

-include src/include/generated/config.mk

# Firmware Configuration
target       := bootloader
builddir     := build
tgt_bindir   := bin

ARMGNU := arm-none-eabi

# These all need to be prepended with path to FreeRTOS source

link_script := ${builddir}/link.ld

tgt_srcs := src/main.c
tgt_srcs += src/common/sll.c
tgt_srcs += src/common/crc.c
tgt_srcs += src/common/command.c

# Sources to be included based on target architecture (e.g. ARM)
tgt_srcs += src/arch/arm/vector.c

# Sources to be included based on target processor (e.g. SAMV71)
tgt_srcs-${CONFIG_SOC_SAMRH71} += src/drivers/rh71_flash.c
tgt_srcs-${CONFIG_SOC_SAMRH71} += src/drivers/rh71_usart.c
tgt_srcs-${CONFIG_SOC_SAMRH71} += src/drivers/rh71_watchdog.c

tgt_srcs-${CONFIG_SOC_SAMV71} += src/drivers/v71_flash.c
tgt_srcs-${CONFIG_SOC_SAMV71} += src/drivers/v71_usart.c
tgt_srcs-${CONFIG_SOC_SAMV71} += src/drivers/v71_watchdog.c

tgt_srcs += ${tgt_srcs-y}

# tgt_srcs-${CONFIG_SOC_RH71} += src/drivers/rh71_usart.c
# $(call tgt_src_ifdef, CONFIG_SOC_RH71, src/drivers/rh71_usart.c)
# # This maybe also has to be eval'd (?) I don't remember, if it does then this is going to be a hassle
# define tgt_src_ifdef
# if defined(${1})
#   tgt_srcs += ${2}
# endef

# Sources to be included based on target board (e.g. SAMRH71-EK)
# ...

tgt_cppflags := -g
tgt_cppflags += -Wall -Wextra
tgt_cppflags += -O3
# tgt_cppflags += -Os
tgt_cppflags += -I src/include -I src/include/generated
# Architecture include path
tgt_cppflags += -I src/arch/arm/include
# tgt_cppflags += -fno-builtin
tgt_cppflags += -ffreestanding
tgt_cppflags += -fno-builtin-memcpy -fno-builtin-memset
# tgt_cppflags += -O0
# tgt_cppflags += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16
# 
# !!!!!!!! NOTE: I think this is the wrong FPU !!!!!!!!
# 
tgt_cppflags  += -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv4-sp-d16
tgt_cflags   := 
tgt_ldflags  := -T ${link_script}
tgt_ldflags  += -nostdlib
tgt_ldflags  += -z max-page-size=0x1000
#  -L${ARMPATH}/lib/gcc/arm-none-eabi/7.2.1/
# tgt_ldflags  +=-Wl,--build-id=none
# tgt_ldflags  += --specs=nano.specs
# tgt_ldflags  += --specs=nosys.specs
# tgt_ldflags  += -L arm-none-eabi/lib/
# tgt_ldflags  += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16
# 
# !!!!!!!! NOTE: I think this is the wrong FPU !!!!!!!!
# 
tgt_ldflags  += -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv4-sp-d16
tgt_ldlibs   :=
# tgt_ldlibs   += -lgcc
# tgt_ldlibs   += -lc
asm_flags    = -Wa,-adhln=$(addsuffix .s,$(basename $@))

# Configure tooling
AS      := ${ARMGNU}-as
CC      := ${ARMGNU}-gcc
CPP     := ${ARMGNU}-cpp
CXX     := ${ARMGNU}-g++
# LINKER  := ${ARMGNU}-ld
# LINKER  := ${ARMGNU}-g++
LINKER  := ${ARMGNU}-gcc
OBJCOPY := ${ARMGNU}-objcopy
OBJDUMP := ${ARMGNU}-objdump

objdir     := ${builddir}
tgtdir     := ${tgt_bindir}

tgt_objs   := $(addprefix ${objdir}/,$(addsuffix .o,$(basename ${tgt_srcs})))

depdirs    := ${tgtdir} ${objdir}
depdirs    += $(dir ${tgt_objs})

# Add order-only prerequisite rules for depdirs
$(foreach dir,${depdirs},$(eval $(filter ${dir}%,${tgt_objs}): | ${dir}))

tgt := $(strip ${target})
tgt_elf := ${tgt}.elf

.PHONY: all
all: ${tgtdir}/${tgt_elf}

# Automatic variables: https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
# $@ : The file name of the target rule
# $< : The name of the first prerequisite

# ELF build rule
${tgtdir}/${tgt_elf}: ${tgt_objs} | ${tgtdir}
	${quiet_ld}$(strip ${LINKER} ${tgt_ldflags} ${tgt_objs} -o $@ ${tgt_ldlibs})
	${quiet_objdump}$(strip ${OBJDUMP} -d -x $@ > $@.list)

# C build rule
${objdir}/%.o : %.c
	${quiet_cc}$(strip ${CC} ${DEPFLAGS} ${asm_flags} ${tgt_cppflags} \
		${CPPFLAGS} ${tgt_cflags} ${CFLAGS} -c $< -o $@)
	${quiet_objdump}$(strip ${OBJDUMP} -D -S $@ > $@.list)
	${deppostcompile}

# Linker script is generated
#   -E  Stop after the preprocessing stage; do not run the compiler proper.  The output is in
#       the form of preprocessed source code, which is sent to the standard output.
#   -P  Inhibit generation of linemarkers in the output from the preprocessor.  This might be
#       useful when running the preprocessor on something that is not C code, and will be sent
#       to a program which might be confused by the linemarkers.
#   -x  Specify explicitly the language for hte following input files

${link_script}: tools/atsamv71q21_bl.ld src/include/generated/config.h
	${quiet_cpp}$(strip ${CPP} -E -P -x assembler-with-cpp ${CPPFLAGS} ${tgt_cppflags} $< -o $@)

# Linker script base uses config.h
# tools/atsamv71q21_bl.ld: src/include/generated/config.h
src/include/generated/config.h:

# Add dependency of the final ELF on the generated linker script
${tgtdir}/${tgt_elf}: ${link_script}

.PHONY: clean
clean:
	${RM} ${tgt_objs} ${tgt_elf} ${tgt_bin} ${link_script}

.PHONY: distclean
distclean:
	${RM} -rf ${depdirs}

print.%:
	@echo '$*=$($*)'
	@echo '  origin = $(origin $*)'
	@echo '  flavor = $(flavor $*)'
	@echo '   value = $(value  $*)'

# Create dependent directories
$(eval $(call ET_DEP_RULE,${depdirs}))

# Include automatically generated dependencies
-include ${tgt_objs:%.o=%.d}
