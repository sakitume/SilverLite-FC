#!/usr/bin/env python
import serial
import sys
import msvcrt
import struct

def handlePacket(cmdID, packet):
    if cmdID == 0xFF:
        print str(packet),
    elif cmdID == 0x01:
        if len(packet) >= 12:
            loopTime, pps, pktHit, hwCRC, bCRC, osdTime = struct.unpack("<hhhhhh", packet)
            print "looptime: ", loopTime
            print "osdTime: ", osdTime
            print "pps: ", pps, "hit: ", pktHit
            print "hcrc: ", hwCRC, "bcrc: ", bCRC
            print
        elif len(packet) >= 10:
            loopTime, pps, pktHit, hwCRC, bCRC = struct.unpack("<hhhhh", packet)
            print "looptime: ", loopTime
            print "pps: ", pps, "hit: ", pktHit
            print "hcrc: ", hwCRC, "bcrc: ", bCRC
            print

        elif len(packet) >= 2:
            print struct.unpack("<h", packet)
    else:
        print "Unknown packet type: ", hex(cmdID), len(packet)


    pass

def checkForInput(ser):
    # Check for keyboard input
    if msvcrt.kbhit():
        ch = msvcrt.getch()
        if ch == 'r':
            print 'Sending reset command'
            ser.write(ch)

def main():
    port = "com19"
    if len(sys.argv) > 1:
        if sys.argv[1].lower().startswith("com"):
            port = sys.argv[1]

    while True:
        # Call kbhit() so that Ctrl-C can be used to kill this script
        # should we be in a tight loop trying to open a port that isn't available
        msvcrt.kbhit()

        try:
            ser = serial.Serial(port, baudrate=115200, timeout=0)
            print "\n\nNew Serial\n"
        except:
            continue

        inputStream = bytearray()
        while True:
            try:
                data = ser.read(1024)
            except:
                break

            checkForInput(ser)

            if data:
                inputStream += bytearray(data)
                while len(inputStream):
                        
                    # Scan until flag found, then discard everything preceeding it
                    for i in xrange(len(inputStream)):
                        if inputStream[i] == 0x7E:
                            if i > 0:
                                # Discard everything preceeding the start flag
                                inputStream = inputStream[i:]
                            break

                    # if flag not available
                    if inputStream[0] != 0x7E:
                        break

                    # if enough data to retrieve the payload length
                    if len(inputStream) >= 3:
                        payloadLen = inputStream[2]
                        # if enough data to retrieve checksum and end flag
                        if len(inputStream) >= (payloadLen + 6): # 3 for packet header, 3 for packet trailer
                            if inputStream[5 + payloadLen] != 0x7E:
                                # End flag not present, not a valid frame
                                # Move past the byte we thought was a start flag but actually wasn't and try again
                                print "End flag not found"
                                inputStream = inputStream[1:]
                                continue
                            else:
                                # test checksum
                                checksum = inputStream[payloadLen+3] + (inputStream[payloadLen+4] << 8)
                                sum = 0
                                for i in xrange(payloadLen+3):
                                    sum += inputStream[i]
                                if checksum != sum:
                                    # Checksum didn't match
                                    # Move past the byte we thought was a start flag but actually wasn't and try again
                                    inputStream = inputStream[1:]
                                    print "Checksum invalid"
                                    continue
                                
                                # Packet was good, now we should handle it
                                handlePacket(inputStream[1], inputStream[3:payloadLen+3])

                                # Advance past the packet and continue in case there's more in the input stream
                                inputStream = inputStream[payloadLen+6:]
                                continue
                        else:
                            # Need more data, break outta here
                            break
                    else:
                        # Need more data, break outta here
                        break

#            
main()


