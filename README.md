## About ##

|            |                            |
|------------|----------------------------|
| Title:     | Flight Computer Bootloader |
| Author:    | OpenLunar Foundation       |
| Date:      | 2020-01-16                 |
| Copyright: | Copyright © 2020 OpenLunar |
| Version:   | 0                          |

[![C/C++ CI](https://github.com/openlunar/fc-bootloader/workflows/C/C++%20CI/badge.svg)](https://github.com/openlunar/fc-bootloader/actions?query=workflow%3A%22C%2FC%2B%2B+CI%22)

Flight Computer bootloader developed for the [SAMRH71](https://www.microchip.com/wwwproducts/en/SAMRH71) and [SAMV71](https://www.microchip.com/wwwproducts/en/ATSAMV71Q21).

## Build

### Dependencies

*Note: Currently the development environment is Linux only.*

- Python 3
- TODO: ARM tool chain
- make
- opneocd

### Configuration

To support multiple targets and various configurations (development / debug / production) this project uses Kconfig. To configure the build for a specific board:

```bash
$ make <board>_defconfig
```

Currently supported targets:

- [Atmel SAMRH71](https://www.microchip.com/wwwproducts/en/SAMRH71)
	+ `samrh71_ek_defconfig` : [SAMRH71F20-EK](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/SAMRH71F20-EK)
- [Atmel SAMV71Q21](https://www.microchip.com/wwwproducts/en/ATSAMV71Q21)
	+ `samv71_xult_ek_defconfig` : [SAM V71 Xplained Ultra](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/ATSAMV71-XULT)

### Compile

```bash
$ make
```

This will generate `bin/bootloader.elf`.

### Deploy

The method of loading the binary onto the target depends on the specific board and debug connection. Below are examples for the two evaluation boards.

#### SAMV71 Xplained Ultra

This example uses `gdb` to load code via `openocd` over the eDBG interface.

```bash
# In one terminal (tab)
$ openocd -f interface/cmsis-dap.cfg -f board/atmel_samv71_xplained_ultra.cfg

# In another terminal (tab)
$ arm-none-eabi-gdb bin/bootloader.elf
(gdb) tar ext :3333
(gdb) load
(gdb) quit
```

#### SAMRH71 EK

...

### Debug

...

## Project Structure

...

## Contributing

...

## The MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## README TODO

- RH71 deploy example
	+ Supporting the Atmel ICE
	+ Custom openocd config file
- Debug notes
	+ Supporting the Atmel ICE with V71 (selecting USB device)
- Fill out sections: Project Structure, Contributing, ...

---

Old config (removing this soon).

## Requirements

* Python 3
* Usb or debugger connection to either the ATSAMV71 Xplained board, or the ATSAMV71RH Eval board

## Setup

```bash
pip3 install tools/requirements.txt
sudo apt-get install gcc-arm-none-eabi
sudo apt install openocd gdb-multiarch
```

## How do I use it?

git and make


## Configuration


### Compiler

something arm eabi some version


### Makefile

clean
	make clean

config file for ATSAMV71 Xplained board
	make samv71_xult_ek_defconfig
or
config file for ATSAMV71RH Eval board
	make samrh71_ek_defconfig

build:
	make

start openocd server thing:
	openocd -f interface/cmsis-dap.cfg -f board/atmel_samv71_xplained_ultra.cfg

separate terminal:
	gdb-multiarch bin/bootloader.elf

separate terminal:
	#here goes python terminal startup
