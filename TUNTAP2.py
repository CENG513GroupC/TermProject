
import random
from socket import *

# Create a UDP socket
# Notice the use of SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)
# serverSocket.setsockopt(socket.SOL_SOCKET, 25, str("mytun1" + '\0').encode('utf-8'))

import socket
import IN

#sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serverSocket.setsockopt(socket.SOL_SOCKET, IN.SO_BINDTODEVICE, "tun1")

rawSocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
rawSocket.bind(('127.0.0.1',0))


receivedPacket = rawSocket.recv(2048)
print "Received : ", receivedPacket





exit()
# Assign IP address and port number to socket
serverSocket.bind(('0.0.0.0', 2048))

while True:
    # Generate random number in the range of 0 to 10
    print "Waiting for message : "

    # Receive the client packet along with the address it is coming from
    message, address = serverSocket.recvfrom(1024)
    print "Message : ", message
    print "Address : ", address

    # Capitalize the message from the client
    message = message.upper()

    # If rand is less is than 4, we consider the packet lost and do notrespond


    # Otherwise, the server responds
