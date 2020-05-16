# from gpiozero import LED
import socket
import time
import argparse
import sys


def getOptions(args):
    parser = argparse.ArgumentParser(description='Parse Raw Socket Configuration')
    parser.add_argument('-i', '--interface', help="WiFi Interface")
    parser.add_argument('-x', '--mode', help="tx or rxq")
    parser.add_argument('-m', '--message', help="message")
    parser.add_argument('-n', '--interval', help="interval between each message")
    options = parser.parse_args(args)
    return options


def protocol_to_ethertype(protocol):
    return chr((protocol & 0xFF00) >> 8) + chr(protocol & 0x00FF)


def send_msg(sock, msg):
    payload = dest + mac + ethertype + msg
    sock.send(payload)


def receive_msg(sock):
    sock.recv(1024)


def main(options):
    protocol_hex = 0xEEFA
    interface = options.interface
    ethertype = protocol_to_ethertype(protocol_hex)
    protocol = socket.htons(protocol_hex) # or 0
    # led = LED(17)

    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, protocol)
    sock.bind((interface, 0))

    if options.mode == "tx":
        while True:
            # led.on()
            send_msg(sock, option.message)
            # led.off()
            # time.sleep(option.interval)
    else:
        while True:
            receive_msg(sock, led)
            # led.on()
            # time.sleep(0.001)
            # led.off()


if __name__ == '__main__':
    options = getOptions(sys.argv[1:])
    main(options)

