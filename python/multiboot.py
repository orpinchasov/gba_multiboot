# Send images to the GBA using the multiboot protocol
# over multiplayer protocol.
# 
# The implementation is based on the original
# implementation in the XBOO cable.

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
    return unpack('<H', raw)[0]
    
    
def transmission(s, bytes):
    print 'Writing: %04x' % (bytes,)
    update_arduino(s, bytes)
    result = read(s)
    print 'Read: %04x' % (result,)
    
    return result


def open_serial():
    s = Serial(ARDUINO_PORT, 115200, timeout=1)
    print 'Sleeping for allowing serial to open...',
    sleep(2)

    return s


def initialize(s):
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
            
        sleep(0.0625)
        
    
    r = transmission(s, 0x6102)
    sleep(0.06)
    
    print 'Done\nSending Header Data......'
    
    for i in xrange(0x60):
        transmission(s, i)
        sleep(0.0625)
        
    transmission(s, 0x6202)
    sleep(0.0625)
    transmission(s, 0x6202)
    sleep(0.0625)
    
    for i in xrange(100):
        transmission(0x63f7)
        sleep(0.0625)


def main():
    s = open_serial()
    
    while True:
        initialize(s)


if __name__ == '__main__':
    main()
