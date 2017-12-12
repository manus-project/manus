#!/usr/bin/env python
import sys
import os
import time
import traceback
import signal

from subprocess import call

from netifaces import interfaces, ifaddresses, AF_INET

def ip4_addresses():
    ip_list = []
    for interface in interfaces():
        for link in ifaddresses(interface)[AF_INET]:
            ip_list.append(link['addr'])
    return ip_list

def add_port_redirect(src, dst):
    # Separate rule for localhost
    call(["iptables", "-t", "nat", "-A", "OUTPUT", "-p", "tcp", "-o", "lo", "--dport", str(dst), "-j", "REDIRECT", "--to", str(src)])
    call(["iptables", "-t", "nat", "-A", "PREROUTING", "-p", "tcp", "--dport", str(dst), "-j", "REDIRECT", "--to", str(src)])

def del_port_redirect(src, dst):
    # Separate rule for localhost
    call(["iptables", "-t", "nat", "-D", "OUTPUT", "-p", "tcp", "-o", "lo", "--dport", str(dst), "-j", "REDIRECT", "--to", str(src)])
    call(["iptables", "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "--dport", str(dst), "-j", "REDIRECT", "--to", str(src)])

#iptables -t nat -A OUTPUT -p tcp --dport 80 -j REDIRECT --to 8080
if __name__ == '__main__':

    portmap = {}

    for p in sys.argv[1:]:
        (src, dst) = p.split(":")
        portmap[int(src)] = int(dst)

    def shutdown_handler(signum, frame):
        for src, dst in portmap.items():
            print("Deleting port mapping %d:%d" % (src, dst))
            del_port_redirect(src, dst)
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown_handler)
 
    for src, dst in portmap.items():
        print("Adding port mapping %d:%d" % (src, dst))
        add_port_redirect(src, dst)

    try:
        signal.pause()
    except KeyboardInterrupt:
        pass
    finally:
        pass

    shutdown_handler(0, None)
