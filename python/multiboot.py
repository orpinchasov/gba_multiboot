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
    if type(bytes) is long:
        bytes = int(bytes)
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
            
        #sleep(0.0625)
        
    r = transmission(s, 0x6102)
    
    print 'Done\nSending Header Data......'
    
    for i in xrange(0x60):
        transmission(s, data[i * 2] + data[i * 2 + 1])
        
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
    
    print('hh:', hex(hh), 'enc:', hex(enc))
    
    return hh, enc
    
def send_main_data_2(s, data, hh, encrypt_seed):
    ClientLength = len(data)
    Client = [unpack('b', b)[0] for b in data]
    
    RData = transmission(s, ( (ClientLength-0xC0) >> 2 ) - 0x34 )
                                                    
    print 'str5=%x\n' % (RData,)

    var_8 = RData
    var_C = 0x0FFF8
    
    client_pos = 0x0C0
    
    still_sending = 0x2

    send_data16_high = 0

    while True:
        send_data16 = send_data16_high
        
        if (client_pos & 0x02) ==  0:
            data32 = Client[client_pos] + (Client[client_pos+1] << 8) \
                   + (Client[client_pos+2]<<0x10) + (Client[client_pos+3] <<0x18)
                   
            CRCTemp = data32
            
            for bit in xrange(32):
                var_30 = var_C^CRCTemp
                
            var_C = var_C >> 1
            CRCTemp = CRCTemp >> 1
            if (var_30&0x01):
                var_C = var_C^0x0A517
        
            encrypt_seed = (encrypt_seed * 0x6F646573)+1
                
            send_data16 = (encrypt_seed ^ data32) ^ ( (~(client_pos+0x2000000)+1) ^ 0x6465646F)
                         
            send_data16_high = send_data16 >> 16
                                                  
            send_data16 = send_data16 & 0x0FFFF
        
        while True:
            print 'here'
            if (client_pos != 0x0C0):
                if (RData != ((client_pos-2)&0x0FFFF)):
                    print 'Transmision error\n'
                    exit(1)             
             
            print 'about to transmit'
            RData = transmission(s, send_data16)
            print '%x:%x ' % (RData,send_data16)
         
            if still_sending:
                client_pos = client_pos + 2
                if ((still_sending==2) and (client_pos!=ClientLength)):
                    break
                send_data16 = 0x65
                still_sending -= 1
            else:
                break
        
        if not still_sending:
            break

    
    while (RData != 0x0075):
        RData = transmission(s, send_data16)

    send_data16 = 0x0066
                                            
    RData = transmission(s, send_data16)

    data32 = ((((RData&0xFF00)+var_8)<<8)|0xFFFF0000)+var_1
    for bit in xrange(32):
        var_30 = var_C ^ data32
        var_C  = var_C>>1
        data32 = data32>>1
        if (var_30 & 0x01):
            var_C = var_C ^ 0x0A517
    
    RData = xfer(s, var_C)
        
    print '[[[%x:%x]]]\n' % (RData, var_C)
    
    if (var_C != RData):
        print 'Transmision error: CRC Bad!.\n'
        return 1
    else:
        print 'CRC Ok - Transmision Done.\n'
        
    return 0
    
def send_main_data(s, data, hh, enc):
    filesize_long = ((len(data) - 0xc0) >> 2) - 0x34
    filesize_32 = (len(data) - 0xc0) / 4
    
    print 'filesize_long:', filesize_long
    print 'filesize_32:', filesize_32
    
    program = [unpack('b', b)[0] for b in data]
    
    r = transmission(s, filesize_long)
    
    rr = (r >> 8) & 0xff
    
    var_C = 0xFFF8
    
    percent = 0
    
    print 'Sending Main Data........'
    for i in xrange(filesize_32):
        # 'pos' skips header
        # realign pos pointer, only for encoding calculation (see above)
        pos = 0xc0 + i * 4
        
        data32 = program[pos] + (program[pos + 1] << 8) + \
                 (program[pos + 2] << 0x10) + (program[pos + 3] << 0x18)

        CRC = data32
        for bit in xrange(32):
            var_30 = var_C ^ CRC
            var_C = var_C >> 1
            CRC = CRC >> 1
            if (var_30 & 0x01):
                # Adapted to multiplayer transfer
                var_C = var_C ^ 0x0A517

        enc = (enc * 0x6F646573) + 1
        # NOTE: The following has been adapted to multiboot instead of
        # normal mode.
        data32 = (enc ^ data32) ^ (((~(pos + 0x2000000)) + 1) ^ 0x6465646F)

        # NOTE: We need to send in two times:
        transmission(s, int(data32 & 0xffff))
        transmission(s, int((data32 >> 0x16) & 0xffff))

        #if (percent!=(((i+1)*100)/filesize_32)):
        #    percent=((i+1)*100)/filesize_32
        #    print '%3d%%' % (percent,)

    print 'Done\nCRC Check................'
    
    timeout = 0
    r = transmission(s, 0x6500)
    while r != 0x7500 and timeout < 1000:
        r = transmission(s, 0x6500)
        timeout += 1
        if timeout == 1000:
            print 'Timeout:'

    transmission(s, 0x6600)

    CRC = ((rr << 8) | 0x0ffff0000) + hh
    for bit in xrange(32):
        var_30 = var_C ^ CRC
        var_C  = var_C >> 1
        CRC = CRC >> 1
        if var_30 & 0x01:
            var_C = var_C ^ 0x0A517
            
    CRC = transmission(s, var_C)

    if CRC == var_C:
        print 'Passed\n\n'
    else:
        print 'CRC check failed!\n\n'


def main():
    s = open_serial()
    data = open(sys.argv[1], 'rb').read()
    
    padding = (16 - (len(data) % 16)) % 16
    data = data + '\x00' * padding
    
    # TODO: This is to faciliate debugging
    #data = data[:0x1a0]
    
    hh, enc = initialize(s, data)
    send_main_data(s, data, hh, enc)


if __name__ == '__main__':
    main()
