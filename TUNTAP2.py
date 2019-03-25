import socket
import time
import IN


try:
    soket = socket.socket ( socket.AF_INET , socket.SOCK_DGRAM , 0 )
    soket.setsockopt(socket.SOL_SOCKET, IN.SO_BINDTODEVICE, "tun1")

    soket.bind(( "" , 6080 ))

    # soket.re("tun1\0", ( "10.0.0.23" , 2048 ));
    message, address = soket.recvfrom(10)
    print "Message : ", message
    print "Address : ", address


    soket.close()
except:
    print "Error"
