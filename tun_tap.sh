openvpn --mktun --dev tun0
openvpn --mktun --dev tun1
ip link set tun0 up
ip link set tun1 up
ip addr add 10.0.0.1/24 dev tun0
ip addr add 10.0.0.2/24 dev tun1

