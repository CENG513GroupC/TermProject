import socket
import time
import IN

try:
    soket = socket.socket ( socket.AF_INET , socket.SOCK_DGRAM , 0 )
    soket.setsockopt(socket.SOL_SOCKET, IN.SO_BINDTODEVICE, "tun0")

    soket.bind(( "" , 5000 ))
    # soket.connect ( ( "192.169.4.5" , 80 ) )
    localtime = time.localtime(time.time())
    for i in range(1):
        soket.sendto("deneme : " + str(localtime[3]) + ":" + str(localtime[4]) +":"+ str(localtime[5]), ( "10.0.0.2" , 6080 ));


    soket.close()
except:
    print "Error"

exit()
