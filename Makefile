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

# Firmware Configuration
target       := bootloader
builddir     := build
tgt_bindir   := bin

# The directories for these two files need to exist before they're generated!
config_file     := ${builddir}/.config
config_hdr_file := ${builddir}/src/include/config.h

-include ${config_file}

CROSSCOMPILER := arm-none-eabi-

# These all need to be prepended with path to FreeRTOS source

link_script := ${builddir}/link.ld

tgt_srcs := src/main.c
tgt_srcs += src/common/sll.c
tgt_srcs += src/common/crc.c
tgt_srcs += src/common/printf.c
tgt_srcs += src/common/console.c
tgt_srcs += src/common/system.c

tgt_srcs += src/common/moon/server.c
tgt_srcs += src/common/moon/codec.c
tgt_srcs += src/common/moon/transport_usart.c

tgt_srcs += src/common/services/bootloader.c

tgt_srcs += src/common/moon/generated/services.c
tgt_srcs += src/common/moon/generated/service_bootloader.c

# Sources to be included based on target architecture (e.g. ARM)
tgt_srcs += src/arch/arm/vector.c

# Unified drivers
tgt_srcs += src/drivers/x71_usart.c
tgt_srcs += src/drivers/flash.c
# tgt_srcs += src/drivers/x71_flash.c

# Sources to be included based on target processor (e.g. SAMV71)
tgt_srcs-${CONFIG_SOC_SERIES_SAMRH71} += src/drivers/rh71_flash.c
# tgt_srcs-${CONFIG_SOC_SERIES_SAMRH71} += src/drivers/rh71_usart.c
tgt_srcs-${CONFIG_SOC_SERIES_SAMRH71} += src/drivers/rh71_watchdog.c

tgt_srcs-${CONFIG_SOC_SERIES_SAMV71} += src/drivers/v71_flash.c
# tgt_srcs-${CONFIG_SOC_SERIES_SAMV71} += src/drivers/v71_usart.c
tgt_srcs-${CONFIG_SOC_SERIES_SAMV71} += src/drivers/v71_watchdog.c

# Add CONFIG sources to main sources list
tgt_srcs += ${tgt_srcs-y}

# NOTE: Need to add rule to generate '${builddir}/src/include/generated' (depdir dependency for rule that generates the contents of this folder)

tgt_cppflags := -g
tgt_cppflags += -Wall -Wextra
tgt_cppflags += -O3
# tgt_cppflags += -O0
# tgt_cppflags += -Os
tgt_cppflags += -I src/include -I $(dir ${config_hdr_file})
# Architecture include path
tgt_cppflags += -I src/arch/arm/include
# tgt_cppflags += -fno-builtin
tgt_cppflags += -ffreestanding
tgt_cppflags += -fno-builtin-memcpy -fno-builtin-memset
# tgt_cppflags += -O0
# tgt_cppflags += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16
tgt_cppflags += -mthumb -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv5-sp-d16
tgt_cppflags += -DPRINTF_INCLUDE_CONFIG_H
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
tgt_ldflags  += -mthumb -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv5-sp-d16
tgt_ldlibs   :=
# tgt_ldlibs   += -lgcc
# tgt_ldlibs   += -lc
asm_flags    = -Wa,-adhln=$(addsuffix .s,$(basename $@))

# Configure tooling
AS      := ${CROSSCOMPILER}as
CC      := ${CROSSCOMPILER}gcc
CPP     := ${CROSSCOMPILER}cpp
CXX     := ${CROSSCOMPILER}g++
# LINKER  := ${CROSSCOMPILER}ld
# LINKER  := ${CROSSCOMPILER}g++
LINKER  := ${CROSSCOMPILER}gcc
OBJCOPY := ${CROSSCOMPILER}objcopy
OBJDUMP := ${CROSSCOMPILER}objdump

PYTHON := python3

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

${link_script}: tools/atsamx71_bl.ld ${config_hdr_file}
	${quiet_cpp}$(strip ${CPP} -E -P -x assembler-with-cpp ${CPPFLAGS} ${tgt_cppflags} $< -o $@)

# Linker script base uses config.h
# tools/atsamv71q21_bl.ld: src/include/generated/config.h
${config_hdr_file}:

# Add dependency of the final ELF on the generated linker script
${tgtdir}/${tgt_elf}: ${link_script}

# DEVELOPMENT

# # Try to get a list of config files and make sure they exist
# # Could use this...
# # %_defconfig:board_defconfig := $(filter %_defconfig,${MAKECMDGOALS})
# # firstword used in case someone supplies two *_defconfig goals (which is not supported)
# board_defconfig := $(firstword $(filter %_defconfig,${MAKECMDGOALS}))
# # Could convert the operation below into a function that provides the functionality "if more than one list item"; I wrote it to warn if there is more than one *_defconfig file.
# # $(ifneq ,$(word 2,$(filter %_defconfig,${MAKECMDGOALS})),$(error "Do not supply more than one *_defconfig file"))
# # board_defconfig := $(filter %_defconfig,${MAKECMDGOALS})
# board := 
# test_var := $(wildcard boards/*/${board_defconfig})

# Ok, so u-boot supports something like "make valid_defconfig valid_defconfig"
# I don't know why you would want to do it but if you're dumb enough to do it, go for it
# board_defconfig := $(firstword $(filter %_defconfig,${MAKECMDGOALS}))
# Also, for now, we're going to pass the buck to the Python script to validate that the file exists and spit out an error if it doesn't.
# If I wanted to create a list of defconfig paths, validate them, and still use them in the pattern rule then I could create a list of lists (one for each defconfig) and then use a make find command to get the correct list based on the "$@" variable

# Here is where we check that these files exist...however, that's not how I'm setting things up right now; I don't have a persistent way of tracking the board defconfig between builds. TBD.
# $(foreach c,${board_defconfig},\
# 	$(ifeq ,$(wildcard ${c})),$(error "File ${c} doesn't exist"))

depdirs += $(dir ${config_hdr_file})

# Expected pattern of: <board-name>_defconfig
# 	echo ${board_defconfig}
# 	${PYTHON} tools/kconfig.py Kconfig ${board_defconfig}
%_defconfig: | $(dir ${config_hdr_file})
	${PYTHON} tools/kconfig.py Kconfig ${config_file} ${config_hdr_file} src/boards/$*/$@

# DEVELOPMENT


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
