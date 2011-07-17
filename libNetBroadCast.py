#! /usr/bin/python

# To change this template, choose Tools | Templates
# and open the template in the editor.

# Code that works on the VMs that allows the  VMs to broadcast messages to each other.

__author__="anirudh"
__date__ ="$Feb 20, 2011 2:52:57 AM$"
import libnet
from libnet.constants import *

if __name__ == "__main__":
    print "Hello World";
    l=libnet.context(LINK,"eth0")
    l.build_udp(sp=1345,payload="LEADER_REQUEST",dp=1345)
    l.autobuild_ipv4(dst=l.name2addr4("10.0.0.0"),prot=17)
    l.autobuild_ethernet(dst=l.hex_aton("ff:ef:ff:ff:ff:ff"),type=2048)
    for i in range (1,20):
	    l.write()
