# Send images to the GBA using the multiboot protocol
# over multiplayer protocol.
# 
# The implementation is based on the original
# implementation in the XBOO cable.

import sys
from time import sleep
from struct import pack, unpack

from serial import Serial


ARDUINO_PORT = 'COM5'


def update_arduino(s, bytes):
    if type(bytes) is int:
        bytes = pack('<H', bytes)
    s.write(bytes)
    s.flush()


def read(s):
    raw = s.read(2)
    try:
        return unpack('<H', raw)[0]
    except Exception as e:
        print('raw:', raw.encode('hex'))
        return 0xffff
    
    
def transmission(s, bytes):
    update_arduino(s, bytes)
    result = read(s)
    if type(bytes) is int:
        print 'send/receive: %04x %04x' % (bytes, result)
    else:
        print 'send/receive: %s %04x' % (bytes.encode('hex'), result)
        
    sleep(0.01)
    
    return result


def open_serial():
    s = Serial(ARDUINO_PORT, 115200, timeout=1)
    print 'Sleeping for allowing serial to open...',
    sleep(2)

    return s


def initialize(s, data):
    print 'Waiting for first response...'
    
    response = False
    for i in xrange(240):
        for j in xrange(8):
            r = transmission(s, 0x6202)
            
            if (r == 0) or ((r & 0xff00) == 0x7200):
                response = True
                break
        
        if response:
            break

        sleep(0.065)
        
    if not response:
        print 'Timed out waiting for response!'
        exit(1)
            
    print 'Got response!'
    
    for i in xrange(47):
        r = transmission(s, 0x6202)
        if ((r & 0xff00) != 0x7200):
            sleep(0.0625)
            return
            
        #sleep(0.0625)
        
    r = transmission(s, 0x6102)
    
    print 'Done\nSending Header Data......'
    
    for i in xrange(0x60):
        transmission(s, data[i * 2] + data[i * 2 + 1])
        
    transmission(s, 0x6202)
    transmission(s, 0x6202)
    
    r = transmission(s, 0x63f7)
    while (r & 0xff00) != 0x7300:
        r = transmission(s, 0x63f7)
        
    # Get the encryption
    cc = r & 0xff
    hh = ((cc & 0xff) + 0x20f) & 0xff
    enc = ((cc & 0xff) << 8) | (0x0ffff00f7)

    transmission(s, 0x6400 | hh)

    sleep(0.06)


def main():
    s = open_serial()
    data = open(sys.argv[1], 'rb').read()
    
    padding = (16 - (len(data) % 16)) % 16
    data = data + '\x00' * padding
    
    while True:
        initialize(s, data)


if __name__ == '__main__':
    main()
