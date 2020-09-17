
import os
import struct

from manus_starter.privileged import Handler, ConfigPublisher

def get_ip_address(ifname):
    try:
        import socket
        import fcntl
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(s.fileno(), 0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15].encode('utf-8'))
        )[20:24])
    except IOError:
        return ""


class NetworkHandler(Handler):

    def __init__(self, client):

        self._ethernet_interface = os.environ.get("MANUS_ETHERNET_INTERFACE", None)
        self._wireless_interface = os.environ.get("MANUS_WIRELESS_INTERFACE", None)

        self._ethernet_ip = ConfigPublisher(client, "_network.ethernet")
        self._wireless_ip = ConfigPublisher(client, "_network.wireless")


    def handle(self, arguments):
 
        try:

           if arguments[0] == "connect":
               ssid = arguments[1]
               passphrase = arguments[2]

           elif arguments[0] == "hotspot":
               ssid = arguments[1]
               passphrase = arguments[2]

        finally:
            return False

    def run(self):

        if self._ethernet_interface is not None:
            address = get_ip_address(self._ethernet_interface)
            self._ethernet_ip(address)

        if self._wireless_interface is not None:
            address = get_ip_address(self._wireless_interface)
            self._wireless_ip(address)


# /etc/hostapt.cfg
HOSTAPD_CFG="""interface={{interface}}
driver=nl80211
ssid={{name}}
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
"""

# /etc/defaults/hostapt  DAEMON_CONF="/etc/hostapd/hostapt.conf"

# /etc/network/interfaces
INTERFACES_CFG="""auto {{interface}}
iface {{interface}} inet static
  address 192.168.254.1
  netmask 255.255.255.0
"""

# /etc/dnsmasq.conf
DNSMASQ_CFG="""interface={{interface}}
  dhcp-range=192.168.254.2,192.168.254.20,255.255.255.0,24h
"""

def run_hotspot(arguments):
    call(["sudo", "systemctl", "stop", "hostapd"])
    call(["sudo", "systemctl", "stop", "dnsmasq"])







    call(["sudo", "systemctl", "start", "hostapd"])
    call(["sudo", "systemctl", "start", "dnsmasq"])
