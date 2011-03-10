#! /usr/bin/python

# To change this template, choose Tools | Templates
# and open the template in the editor.

# Code that works on the VMs that allows the  VMs to broadcast messages to each other.

__author__="anirudh"
__date__ ="$Feb 20, 2011 2:52:57 AM$"
import libnet
from libnet.constants import *
import time
import sys

if __name__ == "__main__":
    print "Hello World";
    l=libnet.context(LINK,"eth0")
    l.build_udp(sp=1345,payload="UNDERSTAND_LEADER_REQUEST",dp=1345)
    l.autobuild_ipv4(dst=l.name2addr4("10.0.0.0"),prot=17)
    l.autobuild_ethernet(dst=l.hex_aton("ff:ef:ff:ff:ff:ff"),type=2048)
    burstSize=sys.argv[1]

    print "Burst size is "+ burstSize
    if(int(burstSize)<=10) :
	sleepInterval=(float(1)/float(burstSize))
    else :
	sleepInterval=0.1;

    print "sleep interval is " + str(sleepInterval)
    print "Burst size per second is "+burstSize
    if(int(burstSize)>=10) :
	packetsPerInterval=int(float(burstSize)/float(10))	# send multiple packets per interval 
    else :
	packetsPerInterval=1;					# 1 packet per interval at any rate 

    count=0
    print packetsPerInterval    
    now= time.time()
    numberOfIntervals=int(float(burstSize)/float(packetsPerInterval))
    for i in range (0,int(numberOfIntervals)):
	   time.sleep(sleepInterval)
	   for i in range (0,packetsPerInterval):
		l.write()
#		count=count+1
#	   print l.stats() use this to figure out the size of the packet. 
    then =time.time()
    print 'Now is ' + str(now) + ' then is '+ str(then)
    print 'Count is ' + str(count )
