
# Memory map of RH71 for easier examination in GDB

# Ideally this file is generated from a JSON or other document of the memory / register layout.
# I want it to have the proper register map per peripheral plus bitfield definition
# This is why I want to build Python support into GDB

# https://ftp.gnu.org/old-gnu/Manuals/gdb/html_node/gdb_188.html#SEC193

# I want to rebuild arm-gdb with python API support
# Ideally I'd rebuild the Atmel arm-gdb
# https://sourceware.org/gdb/onlinedocs/gdb/Python-API.html

set $INTFLASH = 0x10000000

set $HEFC = 0x40004000

set $PIO = 0x40008000
set $PMC = 0x4000C000
set $FLEXCOM0 = 0x40010000
set $FLEXCOM1 = 0x40014000

set $HEMC = 0x40080000

set $CHIPID = 0x40100000

set $SYSC = 0x40100200

set $RSTC = $SYSC + 0x00
set $SUPC = $SYSC + 0x10
set $RTT = $SYSC + 0x30
set $WDT = $SYSC + 0x50
set $RTC = $SYSC + 0x60
