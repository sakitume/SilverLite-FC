#!/usr/bin/env python

import sys

try:
    import serial
except:
    print "Please install PySerial: pip install PySerial"
    sys.exit(-1)

import serial.tools.list_ports
import struct
import time

from signal import signal, SIGINT

def handler(signal_received, frame):
    print 'SIGINT or CTRL-C detected. Will try to exit.'
    sys.exit(-1)

if sys.platform.startswith('win'):
    import msvcrt
    def getch():
        if msvcrt.kbhit():
            return msvcrt.getch()
else:
    # See: https://stackoverflow.com/questions/13207678/whats-the-simplest-way-of-detecting-keyboard-input-in-python-from-the-terminal
    import termios, fcntl, os, termios
    def getch():
        fd = sys.stdin.fileno()

        oldterm = termios.tcgetattr(fd)
        newattr = termios.tcgetattr(fd)
        newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
        termios.tcsetattr(fd, termios.TCSANOW, newattr)

        oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

        c = None

        try:
            c = sys.stdin.read(1)
        except IOError: pass

        termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
        fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

        return c

def handlePacket(cmdID, packet):
    if cmdID == 0xFF:
        print str(packet),
    elif cmdID == 0x01: # For when RX is SilverLite SPI 
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
    elif cmdID == 0x02: # For when RX is IBUS
        if len(packet) >= 10:
            loopTime, osdTime, pps, pktHit, bCRC = struct.unpack("<hhhhh", packet)
            print "looptime: ", loopTime
            print "osdTime: ", osdTime
            print "pps: ", pps, "hit: ", pktHit
            print "bcrc: ", bCRC
            print
    elif cmdID == 0x03: # Debug logging of USART reception, 32bytes at a time
        if len(packet) == 12:
            channels = struct.unpack("<" + ('h'*6), packet)
            for chan in channels:
                print chan,
            print "\n"
        elif len(packet) == 16:  # 4 floats rx[] (roll, pitch, yaw, throttle)
            channels = struct.unpack("<" + ('f'*4), packet)
            for chan in channels:
                print chan,
            print "\n"
        elif len(packet) == 28:
            channels = struct.unpack("<" + ('h'*14), packet)
            for chan in channels:
                print chan,
            print "\n"
        else:    
            for b in packet:
                print hex(b),
            print "\n"
    elif cmdID == 0x04: # Debug logging of RPM_TELEMETRY_DEBUG data
        if len(packet) == 8:
            debugData = struct.unpack("<" + ('h'*4), packet)
            for data  in debugData:
                print data,
            print "\n"
    elif cmdID == 0x05: # Debug logging for Test apps
        if len(packet) >= 2:
            numShorts = len(packet) / 2
            debugData = struct.unpack("<" + ('h'*numShorts), packet)
            for data in debugData:
                print data,
            print "\n"
    else:
        print "Unknown packet type: ", hex(cmdID), len(packet)


    pass

def checkForInput(ser):
    # Check for keyboard input
    ch = getch()
    if ch == 'r':
        if ser:
            print 'Sending reset command'
            ser.write(ch)
    elif ch == 'b':
        if ser:
            print 'Sending bind command'
            ser.write(ch)

def main():
    signal(SIGINT, handler)

    while True:
        # Call getch() so that Ctrl-C can be used to kill this script
        # should we be in a tight loop trying to open a port that isn't available
        getch()

        try:
            port = None
            # Search available com ports for the VCP 
            # Note: Look at arch/stm32f4-usb.h to see how VID and PID are defined
            for p in serial.tools.list_ports.comports():
                if p.vid == 0x483 and p.pid == 0x5740:
                    port = p.device
                    break
            if port:
                ser = serial.Serial(port, baudrate=115200, timeout=0)
                print "\n\nFlight controller found on", port, "\n"
            else:
                # VCP not found, do nothing for a tiny bit before retrying
                time.sleep(0.1)
                continue
        except:
            time.sleep(0.1)
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


