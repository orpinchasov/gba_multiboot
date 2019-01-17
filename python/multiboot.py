import time

from serial import Serial

ARDUINO_PORT = 'COM5'
MULTIBOOT_PROTOCOL = ['\xff\xff', '\x00\x00', '\x72\x02']


def update_arduino(s, bytes):
    print 'Writing:', bytes.encode('hex')
    s.write(bytes[::-1])
    s.flush()


def read(s):
    return s.read(2)


def open_serial():
    s = Serial(ARDUINO_PORT, 115200, timeout=1)
    print 'Sleeping for allowing serial to open...',
    time.sleep(2)

    return s


def main():
    s = open_serial()

    while True:
        update_arduino(s, MULTIBOOT_PROTOCOL[0])
        if read(s) == '6200'.decode('hex'):
            update_arduino(s, MULTIBOOT_PROTOCOL[1])
            read(s)

        for i in xrange(15):
            update_arduino(s, MULTIBOOT_PROTOCOL[2])
            print read(s).encode('hex')

        update_arduino(s, MULTIBOOT_PROTOCOL[2])
        print read(s).encode('hex')

        break


if __name__ == '__main__':
    main()
