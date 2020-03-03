
import math
import msg
import serial
import struct
import time

# Binary file name (TODO: argument)
# binf_name = '../bin/bootloader.bin'
binf_name = '../bin/blink.bin'

with open(binf_name,'rb') as f:
	binf = f.read()

# Open serial port
ser = serial.Serial('/dev/serial/by-id/usb-Silicon_Labs_CP2103_USB_to_UART_Bridge_Controller_0001-if00-port0',19200,timeout=1)

# Instance of frame decoder
f = msg.FrameDecoder()

# START_PAGE = 0
START_PAGE = 32
PAGE_SIZE = 256
PAYLOAD_SIZE = 32

def send_page(page,page_num):
	# Use declared frame decoder and serial objects; use global page size
	global f, ser, PAGE_SIZE, PAYLOAD_SIZE

	r = range(0,PAGE_SIZE,PAYLOAD_SIZE)
	pkt = 0
	for addr in r:
		# Construct the packet
		# TODO: Use python pack utilities to serialize command structure
		m = msg.build_msg( struct.pack('>BBHB',1,2,addr,32) + page[addr:addr+PAYLOAD_SIZE] )
		pkt = pkt + 1
		
		# Attempt to send the packet until we get an ACK
		err_cnt = 0
		while True:
			if err_cnt > 5:
				print('Exceeded error count maximum (5)')
				return (-1)

			# Send the packet
			ser.write(m)
			
			# Read the response
			r = f.parse_stream( ser )
			#
			# CRC failed or invalid packet length
			if (r is None):
				print('< None >')
				err_cnt = err_cnt + 1
				time.sleep(0.1)
				continue

			# Response should be [system,command,response]
			# Just going to assume system and command match...
			if (r[2] != 0x00):
				print('< NACK >')
				err_cnt = err_cnt + 1
				time.sleep(0.1)
				continue
			
			print('Wrote {1} bytes to page_buffer @ {0:02x}'.format(addr,PAYLOAD_SIZE))
			break

	return 0

# Synchronize communication channel (send a ping and get a valid response)
ser.flushInput()
m = msg.build_msg( struct.pack('>BB',1,0) )
r = None
err_cnt = 0
while True:
	print('Pinging...')
	ser.write(m)
	r = f.parse_stream(ser)
	
	if (r is None) or (r[2] is b'\xFF'):
		err_cnt = err_cnt + 1
	else:
		break

	if err_cnt >=5:
		break

	time.sleep(0.1)

if (err_cnt >= 5):
	exit(1)

# exit(0)

# Calculate number of pages to write
binf_size = len(binf)
page_cnt = math.ceil(binf_size / PAGE_SIZE)

# NOTE: For now it's easier to just fill pages out to PAGE_SIZE with 0xFF and
# send that; in the future it should be implemented such that there is a
# "page_buffer_erase" function and only real data is written

# Erase the pages to be written
m = msg.build_msg( struct.pack('>BBHH',1,0xE3,START_PAGE,START_PAGE+(page_cnt-1)) )
ser.write(m)
r = f.parse_stream(ser)
if (r is None) or (r[2] is b'\xFF'):
	print('Failed to erase flash')
else:
	print('Flash erased successfully')

import hexdump as hd

# Write and commit pages one at a time
for p in range(0,page_cnt):
	print('Writing page {0} ({2}) of {1}'.format((p + 1),page_cnt,(p + START_PAGE)))
	# Calculate start and end indices
	start = p * PAGE_SIZE
	end = start + PAGE_SIZE
	if end >= binf_size:
		end = binf_size

	# Slice the binary file into a page
	page = binf[start:end]

	# Pad page out to PAGE_SIZE if necessary
	if len(page) < PAGE_SIZE:
		page = page + b'\xFF' * (PAGE_SIZE - len(page))

	print()
	hd.hexdump(page)
	print()

	# Send the page
	send_page( page, (p + START_PAGE) )

	# Calculate page CRC
	crc = msg.crc_32(page)

	# Commit page
	m = msg.build_msg( struct.pack('>BBHI',1,0xE2,(p + START_PAGE),crc) )
	ser.write(m)
	r = f.parse_stream(ser)
	if (r is None) or (r[2] is b'\xFF'):
		print('Failed to commit page to flash')
		exit(1)
	else:
		print('Page commit to flash successfully')
