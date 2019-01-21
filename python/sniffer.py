from time import sleep
from struct import pack, unpack, error

from serial import Serial


ARDUINO_PORT = 'COM5'


def read(s):
    try:
        raw = s.read(4)
        return unpack('<I', raw)[0]
    except error:
        return None


def open_serial():
    s = Serial(ARDUINO_PORT, 115200, timeout=1)
    print 'Sleeping for allowing serial to open...',
    sleep(2)

    return s


def main():
    s = open_serial()
    
    while True:
        result = read(s)
        if result is None:
            continue
        print 'Read: %08x' % (result,)


if __name__ == '__main__':
    main()
