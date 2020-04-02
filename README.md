## About ##

|            |                           |  
| ---------- | ------------------------- |  
| Title:     | Flight Computer Bootloader|  
| Author:    | OpenLunar Foundation       |  
| Date:      | 2020-01-16 |  
| Copyright: | Copyright © 2020 OpenLunar    |  
| Version:   | 0     |  

[![C/C++ CI](https://github.com/openlunar/fc-bootloader/workflows/C/C++%20CI/badge.svg)](https://github.com/openlunar/fc-bootloader/actions?query=workflow%3A%22C%2FC%2B%2B+CI%22)

## Introduction ##
Flight Computer Bootloader

## Requirements ##
* Python 3
* Usb or debugger connection to either the ATSAMV71 Xplained board, or the ATSAMV71RH Eval board

## Setup ##

	pip3 install tools/requirements.txt
	sudo apt-get install gcc-arm-none-eabi
	sudo apt install openocd gdb-multiarch


## How do I use it? ##

git and make


## Configuration ##


### Compiler ###

something arm eabi some version


### Makefile ###

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


## The MIT License ##

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