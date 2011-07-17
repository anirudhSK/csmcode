#! /usr/bin/python

# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="anirudh"
__date__ ="$Feb 20, 2011 2:43:44 AM$"

import pcap
import sys
import string
import time
import socket
import struct

protocols={socket.IPPROTO_TCP:'tcp',
           socket.IPPROTO_UDP:'udp',
           socket.IPPROTO_ICMP:'icmp'}

def decode_ip_packet(s):
  d={}
  d['version']=(ord(s[0]) & 0xf0) >> 4
  d['header_len']=ord(s[0]) & 0x0f
  d['tos']=ord(s[1])
  d['total_len']=socket.ntohs(struct.unpack('H',s[2:4])[0])
  d['id']=socket.ntohs(struct.unpack('H',s[4:6])[0])
  d['flags']=(ord(s[6]) & 0xe0) >> 5
  d['fragment_offset']=socket.ntohs(struct.unpack('H',s[6:8])[0] & 0x1f)
  d['ttl']=ord(s[8])
  d['protocol']=ord(s[9])
  d['checksum']=socket.ntohs(struct.unpack('H',s[10:12])[0])
  d['source_address']=pcap.ntoa(struct.unpack('i',s[12:16])[0])
  d['destination_address']=pcap.ntoa(struct.unpack('i',s[16:20])[0])
  if d['header_len']>5:
    d['options']=s[20:4*(d['header_len']-5)]
  else:
    d['options']=None
  d['data']=s[4*d['header_len']:]
  return d


def dumphex(s):
  bytes = map(lambda x: '%.2x' % x, map(ord, s))
  for i in xrange(0,len(bytes)/16):
    print '    %s' % string.join(bytes[i*16:(i+1)*16],' ')
  print '    %s' % string.join(bytes[(i+1)*16:],' ')


def print_packet(pktlen, data, timestamp):
  if not data:
    return

  if data[12:14]=='\x08\x00':
    decoded=decode_ip_packet(data[14:])
    # useful for debugging 
#    print decoded
#    print '\n%s.%f %s > %s' % (time.strftime('%H:%M',
#                                           time.localtime(timestamp)),
#                             timestamp % 60,
#                             decoded['source_address'],
#                             decoded['destination_address'])
#    for key in ['version', 'header_len', 'tos', 'total_len', 'id',
#                'flags', 'fragment_offset', 'ttl']:
#      print '  %s: %d' % (key, decoded[key])
#    print '  protocol: %s' % protocols[decoded['protocol']]
#    print '  header checksum: %d' % decoded['checksum']
    transportPayload=decoded['data']
    #print '  data:'
    #print decoded['data']
    # now decode within the data since it is UDP ditch the first 8 bytes ( which is port numbers plus length plus some other fields)
    # and print hecneforth
    print transportPayload[8:]
    # dumphex(decoded['data'])



if __name__ == "__main__":
    print "Hello World";
    pktCapture=pcap.pcapObject()
    pktCapture.open_live("eth0",1024,False,100)
    pktCapture.loop(-1,print_packet)
