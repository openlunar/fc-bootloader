
import bootloader
import erpc
import math
import hexdump as hd

# transport = bootloader.moon_transport.SerialTransport('loop://',38400,timeout=1)

# transport = bootloader.moon_transport.SerialTransport('/dev/serial/by-id/usb-Atmel_Corp._EDBG_CMSIS-DAP_ATML2407131800002578-if01',38400,timeout=1)
# TODO: The transport layer doesn't handle timeouts right now; it just shits itself
transport = bootloader.moon_transport.SerialTransport('/dev/serial/by-id/usb-Silicon_Labs_CP2103_USB_to_UART_Bridge_Controller_0001-if00-port0',19200) #,timeout=1
clientManager = erpc.client.ClientManager(transport, bootloader.moon_codec.MoonCodec)

bl_client = bootloader.client.BootloaderClient(clientManager)

print( bl_client.bl_ping() )
# print( bl_client.bl_writePageBuffer( 0x0000, [0x01,0x02,0x03,0x04]) )
# print( bl_client.bl_erasePageBuffer() )
# print( bl_client.bl_eraseApp( bootloader.common.AppId.APP_1 ) )
# print( bl_client.bl_writePage( 0x0000, 0x12345678 ) )
# print( bl_client.bl_writePage( 0x1000, 0x12345678 ) )
# print( bl_client.bl_setBootAction( bootloader.common.BootAction.BOOT_APP_1 ) )

# print( bl_client.bl_boot() )

# exit(0)

# ---
# binf_name = '../bin/blink.bin'
# binf_name = '../bin/rhblink.bin'
# binf_name = '../bin/bootloader_short.bin'

# PAGE_SIZE = 512 # V71
PAGE_SIZE = 256 # RH71

# binf_name = '../bin/app1.bin'
# PARTITION = bootloader.common.AppId.APP_1
# BOOT_ACTION = bootloader.common.BootAction.BOOT_APP_1

binf_name = '../bin/app2.bin'
PARTITION = bootloader.common.AppId.APP_2
BOOT_ACTION = bootloader.common.BootAction.BOOT_APP_2

# binf_name = '../bin/bootloader.bin'
# PARTITION = bootloader.common.AppId.BOOTLOADER
# BOOT_ACTION = bootloader.common.BootAction.BOOT_BOOTLOADER

def send_page(client,page,page_num):
	# Use declared frame decoder and serial objects; use global page size
	PAYLOAD_SIZE = 32

	r = range(0,len(page),PAYLOAD_SIZE)
	for addr in r:
		# Get chunk of page
		start = addr
		end = addr + PAYLOAD_SIZE
		if end >= len(page):
			end = len(page)

		chunk = page[start:end]

		# print()
		# hd.hexdump(chunk)
		# print()
		# input('- Press enter to continue -')

		# Attempt to send the packet until we get an ACK
		err_cnt = 0
		while True:
			if err_cnt > 5:
				print('Exceeded error count maximum (5)')
				return (-1)

			try:
				r = client.bl_writePageBuffer( addr, chunk )
			except:
				print('Error during transmission')
				err_cnt = err_cnt + 1
				time.sleep(0.1)
				continue

			# Response should be [system,command,response]
			# Just going to assume system and command match...
			if (r != 0x00):
				print('Error during page write')
				err_cnt = err_cnt + 1
				time.sleep(0.1)
				continue
			
			print('Wrote {1} bytes to page_buffer @ {0:02x}'.format(addr,(end-start)))
			break

	return 0

# -- Flash the blinky program -- #
with open(binf_name,'rb') as f:
	binf = f.read()

# Calculate number of pages to write
binf_size = len(binf)
print('binf_size = ' + str(binf_size))
page_cnt = math.ceil(binf_size / PAGE_SIZE)

# Erase APP_1
try:
	bl_client.bl_eraseApp( PARTITION )
	print('Flash erased successfully')
except:
	print('Failed to erase flash')
	raise

# Write and commit pages one at a time
for p in range(0,page_cnt):
	# Erase the page buffer
	bl_client.bl_erasePageBuffer()

	print('Writing page {0} ({2}) of {1}'.format((p + 1),page_cnt,p))
	# Calculate start and end indices
	start = p * PAGE_SIZE
	end = start + PAGE_SIZE
	if end >= binf_size:
		end = binf_size

	# Slice the binary file into a page
	page = binf[start:end]

	# DEBUG: Print the page contents
	print()
	hd.hexdump(page)
	print()

	# Send the page
	r = send_page( bl_client, page, p )
	# Ignoring return for now
	print(r)

	# Pad page out to PAGE_SIZE if necessary (for CRC calculation)
	if len(page) < PAGE_SIZE:
		page = page + b'\xFF' * (PAGE_SIZE - len(page))

	# Calculate page CRC
	crc = bootloader.moon_transport.crc_32(page)

	# Write the page
	try:
		print('Writing page {0} with crc {1:X}'.format(p, crc))
		r = bl_client.bl_writePage(PARTITION, p, crc )
		print(r)
	except:
		print('Comm failure while writing page')

	if r == 0:
		print('Page written to flash successfully')
	else:
		raise Exception('Page write failure')

print( bl_client.bl_setBootAction( BOOT_ACTION ) )
print( bl_client.bl_boot() )
