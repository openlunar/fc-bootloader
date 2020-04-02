#!/usr/bin/env python

# Copyright (c) 2015 Freescale Semiconductor, Inc.
# Copyright 2016-2017 NXP
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import struct
from erpc.codec import (MessageType, MessageInfo, Codec, CodecError)

class MoonCodec(Codec):
	## Version of this codec.
	MOON_CODEC_VERSION = 0

	def start_write_message(self, msgInfo):
		header = (self.MOON_CODEC_VERSION & 0x3)	\
			| ((msgInfo.service & 0x1F) << 2)		\
			| ((msgInfo.request & 0x3F) << 7)		\
			| ((msgInfo.sequence & 0x1F) << 13)		\
			| ((msgInfo.type.value & 0x3) << 18)	\
			| ((msgInfo.protocol & 0xF) << 20)

		self.write_uint8(header & 0xFF)
		self.write_uint8((header >> 8) & 0xFF)
		self.write_uint8((header >> 16) & 0xFF)

	def _write(self, fmt, value):
		self._buffer += struct.pack(fmt, value)
		self._cursor += struct.calcsize(fmt)

	def write_bool(self, value):
		self._write('<?', value)

	def write_int8(self, value):
		self._write('<b', value)

	def write_int16(self, value):
		self._write('<h', value)

	def write_int32(self, value):
		self._write('<i', value)

	def write_int64(self, value):
		self._write('<q', value)

	def write_uint8(self, value):
		self._write('<B', value)

	def write_uint16(self, value):
		self._write('<H', value)

	def write_uint32(self, value):
		self._write('<I', value)

	def write_uint64(self, value):
		self._write('<Q', value)

	def write_float(self, value):
		self._write('<f', value)

	def write_double(self, value):
		self._write('<d', value)

	def write_string(self, value):
		self.write_binary(value.encode())

	def write_binary(self, value):
		self.write_uint32(len(value))
		self._buffer += value

	def start_write_list(self, length):
		# self.write_uint32(length)
		if length < 256:
			self.write_uint8(length)
		elif length < 65536:
			self.write_uint16(length)
		else:
			self.write_uint32(length)

	def start_write_union(self, discriminator):
		self.write_uint32(discriminator)

	def write_null_flag(self, flag):
		self.write_uint8(1 if flag else 0)

	##
	# @return 4-tuple of msgType, service, request, sequence.
	def start_read_message(self):
		header = self.read_uint8()
		header = header | (self.read_uint8() << 8)
		header = header | (self.read_uint8() << 16)
		# print(hex(header))
		# print(header.to_bytes(3,'little').hex())

		version = (header & 0x3)

		if version != self.MOON_CODEC_VERSION:
			raise CodecError("unsupported codec version %d" % version)

		service = ((header >> 2) & 0x1F)
		request = ((header >> 7) & 0x3F)
		sequence = ((header >> 13) & 0x1F)
		msgType = MessageType((header >> 18) & 0x3)
		protocol = ((header >> 20) & 0xF)

		return MessageInfo(type=msgType, service=service, request=request, sequence=sequence, protocol=protocol)

	def _read(self, fmt):
		result = struct.unpack_from(fmt, self._buffer, self._cursor)
		self._cursor += struct.calcsize(fmt)
		return result[0]

	def read_bool(self):
		return self._read('<?')

	def read_int8(self):
		return self._read('<b')

	def read_int16(self):
		return self._read('<h')

	def read_int32(self):
		return self._read('<i')

	def read_int64(self):
		return self._read('<q')

	def read_uint8(self):
		return self._read('<B')

	def read_uint16(self):
		return self._read('<H')

	def read_uint32(self):
		return self._read('<I')

	def read_uint64(self):
		return self._read('<Q')

	def read_float(self):
		return self._read('<f')

	def read_double(self):
		return self._read('<d')

	def read_string(self):
		return self.read_binary().decode()

	def read_binary(self):
		length = self.read_uint32()
		data = self._buffer[self._cursor:self._cursor+length]
		self._cursor += length
		return data

	##
	# @return Int of list length.
	# TODO: Since the index can be of variable length we need to indicate which size to read...
	def start_read_list(self):
		return self.read_uint32()

	##
	# @return Int of union discriminator.
	def start_read_union(self):
		return self.read_int32()

	def read_null_flag(self):
		return self.read_uint8()
