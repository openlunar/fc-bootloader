
# TODO: Probably worth a good read of (https://mesonbuild.com/Reference-manual.html#custom_target) to make sure I'm using everything right. Also might be useful to use 'depfile'
# TODO: Look into how others use python; this comes from (https://mesonbuild.com/Generating-sources.html)


project('OLF Bootloader',
	['c'],
	default_options : [
		'c_std=c11', 'cpp_std=c++11',
		'buildtype=debugoptimized',
		# 'werror=true'
	],
	license: 'MIT',
	meson_version: '>=0.52.0', # NOTE: Still a WIP, I bump it up whenever I use a feature that warns about my required version versus what I have set
	version: '0'
)

# -- Programs -- #
prog_python = import('python').find_installation('python3')
prog_env = find_program('env')
pykconfig = find_program('tools/kconfig.py')
prog_genconfig = find_program('genconfig')
objcopy = find_program('objcopy')
# meson already calls the C++ compile 'cpp', so 'pp' is pre-processor
pp = find_program('pp')

# -- Configuration -- #
kconfig = import('unstable-kconfig')

# meson config

# So if I switch to 'src/boards/<arch>/<board-name>' directory structure that I plan to, I'm going to need to get the path to the board to meson; I could set 'arch' as a config variable or I could add a 'board_path' as a variable. This second option might be better because it would allow out-of-tree board directories (I think).
# arch = get_option('arch')
# board_path = get_option('board_path')
board = get_option('board')

# board_defconfig = files('src/boards/samv71_xult_ek/samv71_xult_ek_defconfig')
board_defconfig = files('src/boards/' / board / board + '_defconfig')

# Kconfig

top_kconfig = files('Kconfig')

# This runs every time meson is regenerated (i.e. this file is re-evaluated); so the kconfig.py program needs to take this into account and only apply the list of additional commands if '.config' doesn't exist *or* if the input files have changed (e.g. defconfig was updated (?), proj.conf was updated)
# 
# TODO: Add a meson configuration option that is additional Kconfig snippets (e.g. proj.conf); this way the 'input' list to configure_file is fixed for a build.
# 
# TODO: The script below can cache a hash of the files and decide when an update is actually required. 'genconfig' (used by config.h) is also smart and only updates the header if it's actually going to be different (preventing additional builds).
#
# NOTE: So this works but it blows away changes from menuconfig...
config_in = configure_file(
	input : [ board_defconfig ],
	output : '.config',
	# Do I need 'prog_python' in front of pykconfig (?)
	command : [ prog_env, 'srctree=' + meson.source_root(), prog_python, pykconfig, top_kconfig, '@OUTPUT@', '@INPUT@' ]
)

config = kconfig.load( config_in )

ssmod = import('sourceset')

ss = ssmod.source_set()

# Unconditional files (at least for now)
ss.add( files(
	'src/main.c',
	'src/common/sll.c',
	'src/common/crc.c',
	'src/common/printf.c', # TODO: Make this and console.c CONFIG dependence
	'src/common/console.c',
	'src/common/system.c',

	# Platform independent drive code
	'src/drivers/flash.c',

	# Could split these off into "moon sources"; probably will, it'll probably become a sort of sub-project
	'src/common/moon/server.c',
	'src/common/moon/codec.c',
	'src/common/moon/transport_usart.c',
	'src/common/services/bootloader.c',
	'src/common/moon/generated/services.c',
	'src/common/moon/generated/service_bootloader.c',
))

ss.add( when: 'CONFIG_ARM', if_true: files('src/arch/arm/vector.c') )

# TODO: Way to include x71_* drivers when either V71 or RH71 is selected

ss.add( when: 'CONFIG_SOC_SERIES_SAMV71', if_true: files(
	'src/drivers/x71_usart.c',
	'src/drivers/v71_flash.c',
	'src/drivers/v71_watchdog.c'
))

ss.add( when: 'CONFIG_SOC_SERIES_SAMRH71', if_true: files(
	'src/drivers/x71_usart.c',
	'src/drivers/rh71_flash.c',
	'src/drivers/rh71_watchdog.c'
))

# NOTE: I think I can use subdir() to achieve the effect I want with source-inclusion. I'll be able to create files() variables like "arch_files" and "arch_incdirs" and use them at higher-level build steps.

incdirs = include_directories(
	'src/include',
	'src/arch/arm/include'
)

# [ cppflags ] Apply to C and C++ targets
# ??? -fno-builtin-memcpy -fno-builtin-memset
# -ffunction-sections -fdata-sections
# 
# [ ldflags ]
# XXX -z max-page-size=0x1000
# 
# [ arch_flags ]
# -mthumb -mcpu=cortex-m7 -mfloat-abi=softfp -mfpu=fpv5-sp-d16

config_h = custom_target(
	'gen-config-header',
	output : 'config.h',
	input : [ config_in ],
	command : [ prog_env, 'srctree=' + meson.source_root(), prog_genconfig ]
)

link_script = custom_target(
	'gen-linker-script',
	input : 'tools/atsamx71_bl.ld',
	output : 'link.ld',
	command : [ pp, '-E', '-P', '-x', 'assembler-with-cpp', '-I', meson.build_root() , '@INPUT@', '-o', '@OUTPUT@' ],
	depends : [ config_h ]
)

# Architecture independent C flags
c_args = [ '-Wall', '-Wextra', '-ffreestanding', '-DPRINTF_INCLUDE_CONFIG_H' ]
c_link_args = [ '-T', link_script.full_path(), '-nostdlib' ]

# TODO: Have one name, e.g. 'bootloader' and all generated files use it.
basename = 'bootloader'
elf_name = basename + '.elf'
bin_name = basename + '.bin'

ssconfig = ss.apply(config, strict: false) # 'strict: false' prevents errors when a config doesn't exist in the dictionary (as is the case for unset Kconfig options)

tgt_elf = executable(
	elf_name,
	sources: [ ssconfig.sources(), link_script, config_h ],
	dependencies: ssconfig.dependencies(),
	include_directories: incdirs,
	c_args : c_args,
	link_args : c_link_args,
	implicit_include_directories: false # I think
)

# Probably want this set up so that if tgt_elf is built then this is built
custom_target(
	bin_name,
	build_by_default : true,
	input : tgt_elf,
	# output : bin_name,
	output : '@BASENAME@.bin',
	command : [objcopy, '-O', 'binary', '-j', '.text', '-j', '.relocate',
		'@INPUT@', '@OUTPUT@',
	]
)

# Runs from an 'unspecified' directory (i.e. don't count on it being run in a specific directory)
# There's no way to set environment variables in a run_target right now (https://github.com/mesonbuild/meson/issues/2723), so use 'env' to pass in KCONFIG_CONFIG (existing config)
run_target(
	'menuconfig',
	command: [ prog_env, 'KCONFIG_CONFIG=' + meson.build_root() / '.config', 'menuconfig', meson.source_root() / 'Kconfig' ]
)

run_target(
	'oldconfig',
	command: [ prog_env, 'KCONFIG_CONFIG=' + meson.build_root() / '.config', 'oldconfig', meson.source_root() / 'Kconfig' ]
)
