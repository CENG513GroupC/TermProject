import socket
import time
import IN


print "Saif"
try:
    soket = socket.socket ( socket.AF_INET , socket.SOCK_DGRAM , 0 )
    soket.setsockopt(socket.SOL_SOCKET, IN.SO_BINDTODEVICE, "tun0")

    #soket.bind(( "192.169.4.4" , 3696 ))
    # soket.connect ( ( "192.169.4.5" , 80 ) )

    soket.sendto("denemelik\0", ( "10.0.0.3" , 2048 ));


    soket.close()
except:
    print "Error"

exit()