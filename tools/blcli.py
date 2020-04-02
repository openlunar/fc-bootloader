import erpc
import bootloader
import argparse
import zlib
import math
import hexdump as hd
import time

appId_mapping = {
    1: bootloader.common.AppId.APP_1,
    2: bootloader.common.AppId.APP_2
}

bootAction_mapping = {
    1: bootloader.common.BootAction.BOOT_APP_1,
    2: bootloader.common.BootAction.BOOT_APP_2
}

board_configs = {
    'v71': {
        'page_size': 512,
        'baud_rate': 38400,
        'timeout': 1
    },
    'rh71': {
        'page_size': 256,
        'baud_rate': 19200,
        'timeout': 1
    }
}

def open_device(device, baud_rate, timeout, **kwargs):
    print("Do a open device: " + str(device))
    # transport = bootloader.moon_transport.SerialTransport('loop://',38400,timeout=1)
    try:
        transport = bootloader.moon_transport.SerialTransport(device, baud_rate, timeout=timeout)
        clientManager = erpc.client.ClientManager(transport, bootloader.moon_codec.MoonCodec)
        bl_client = bootloader.client.BootloaderClient(clientManager)
    except:
        print("Failed to open device")
        raise

    return bl_client

def send_page(client, page, page_num, payload_size, **kwargs):
    # Use declared frame decoder and serial objects; use global page size
    PAYLOAD_SIZE = payload_size

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
            
            print('Wrote {1} bytes to page_buffer @ {0:02x}'.format(addr, (end-start)))
            break

    return 0


def main(args):
    print("do main stuff with these args: " + str(args))
    # do argument checking here

    bl_client = open_device(**vars(args))

    try:
        print( bl_client.bl_ping() )
    except:
        print('Failed to ping, pre load')
        raise

    # -- Flash the blinky program -- #
    with open(args.write, 'rb') as f:
        binf = f.read()

    # Calculate number of pages to write
    binf_size = len(binf)
    print('binf_size = ' + str(binf_size))
    page_cnt = math.ceil(binf_size / args.page_size)

    # Erase APP_1
    try:
        bl_client.bl_eraseApp( appId_mapping[args.app] )
        print('Flash erased successfully')
    except:
        print('Failed to erase flash')
        raise

    f = open(args.write, 'rb')
    chunk = f.read(args.page_size)

    # Write and commit pages one at a time
    for p in range(0, page_cnt):
        # Erase the page buffer
        bl_client.bl_erasePageBuffer()

        print('Writing page {0} ({2}) of {1}'.format((p + 1), page_cnt, p))
        # Calculate start and end indices
        start = p * args.page_size
        end = start + args.page_size
        if end >= binf_size:
            end = binf_size

        # Slice the binary file into a page
        page = binf[start:end]

        # DEBUG: Print the page contents
        print()
        hd.hexdump(page)
        print()

        # Send the page
        r = send_page( bl_client, page, p, **vars(args) )
        
        # Ignoring return for now
        print(r)

        # Pad page out to args.page_size if necessary (for CRC calculation)
        if len(page) < args.page_size:
            page = page + b'\xFF' * (args.page_size - len(page))

        # Calculate page CRC
        crc = bootloader.moon_transport.crc_32(page)

        # Write the page
        try:
            print('Writing page {0} with crc {1:X}'.format(p, crc))
            r = bl_client.bl_writePage( appId_mapping[args.app], p, crc )
            print(r)
        except:
            print('Comm failure while writing page')
            raise

        if r == 0:
            print('Page written to flash successfully')
        else:
            raise Exception('Page write failure')

    if args.do_boot:
        try:
            bl_client.bl_setBootAction( bootAction_mapping[args.app] )
            bl_client.bl_boot()
        except:
            print('Failed to boot')
            raise
    else:
        try:
            print( bl_client.bl_ping() )
        except:
            print('Failed to ping, post load')
            raise

    exit(0)

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Does commandline interfacing for the bootloader')
    parser.add_argument('-d', '--device', dest='device', default='/dev/serial/by-id/usb-Atmel_Corp._EDBG_CMSIS-DAP_ATML2407131800003232-if01',
                        help='Which device to interface with, ex /dev/serial/by-id/...')
    parser.add_argument('-w', '--write', dest='write', default='../bin/blink.bin',
                        help='File to write, ex /path/to/rickroll.bin')
    parser.add_argument('-a', '--app', dest='app', type=int, default=1, choices=[1, 2],
                        help='Application id of what to boot, ex 1 = APP_1, 2 = APP_2, etc.')
    parser.add_argument('-pls', '--payload-size', dest='payload_size', default=32, type=int,
                        help='Size of payload/chunk to write pages by in bytes, 32 seems to be a magical number here')
    parser.add_argument('--no-boot', dest='do_boot', action='store_false',
                        help='Don\'t boot the application after loading it')

    bc = parser.add_argument_group('board configs', 'choose board config from the following. defaults to v71.').add_mutually_exclusive_group()
    bc.add_argument('-v71', '--v71', action='store_const', dest='board', const='v71')
    bc.add_argument('-rh71', '--rh71', action='store_const', dest='board', const='rh71')

    bco = parser.add_argument_group('board config overrides')
    bco.add_argument('-ps', '--page-size', dest='page_size', type=int,
                        help='''Size of page in bytes. 
                        NOTE: defaults for V71 is {0} bytes, RH71 is {1} bytes.'''\
                        .format(board_configs['v71']['page_size'], 
                            board_configs['rh71']['page_size']))
    bco.add_argument('-br', '--baud', dest='baud_rate', type=int,
                        help='''Baud rate to use the serial device at in bits/second, 
                        ex 19200 = 19,200 bits/second = 19200 baud.
                        NOTE: defaults for V71 is {0} baud, RH71 is {1} baud.'''\
                        .format(board_configs['v71']['baud_rate'], 
                            board_configs['rh71']['baud_rate']))
    bco.add_argument('-t', '--timeout', dest='timeout', type=int,
                        help='''Timeout in seconds of the serial interface. 
                        NOTE: defaults for V71 is {0} seconds, RH71 is {1} seconds.'''\
                        .format(board_configs['v71']['timeout'], 
                            board_configs['rh71']['timeout']))


    args = parser.parse_args()
    # if no board config is supplied, default to v71
    args.board = args.board or 'v71'

    # always use board overrides if supplied
    args.page_size = args.page_size or (board_configs[args.board]['page_size']) 
    args.baud_rate = args.baud_rate or (board_configs[args.board]['baud_rate'])
    args.timeout = args.timeout or (board_configs[args.board]['timeout'])

    main(args)