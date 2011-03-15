#!/bin/python

from header import *
from vnserver import *
from vnclient import *
from parkingserver import *
from parkingclient import *
from join import *
#import time


# START TESTS
# construct test joinagent
node1_ja = JoinAgent(1)
node1_ja.regionX_ = 1
node1_ja.regionY_ = 0
node1_ja.seq = 1
node1_ja.m_version = 1
print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'



print "RESET"
node1_ja.status_reset()
node1_ja.setNeighbors()

print "CHECKING LEADER STATUS"
node1_ja.check_old_leader_status()
node1_ja.check_leader_status()
print "NOW IN REGION %d,%d" % (node1_ja.regionX_, node1_ja.regionY_)

print "FORCING REGION CHANGE from 1,0 to 0,0"
node1_ja.regionX_ = 1
node1_ja.regionY_ = 0
node1_ja.check_location()
print "NOW IN REGION %d,%d" % (node1_ja.regionX_, node1_ja.regionY_)

print "STATIONARY..."
node1_ja.check_location()
print "NOW IN REGION %d,%d" % (node1_ja.regionX_, node1_ja.regionY_)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'




# construct test packet
pkt3 = Packet()
pkt3.vnhdr.type = JOIN_MSG
pkt3.vnhdr.subtype = ""
pkt3.vnhdr.src = 1337
pkt3.vnhdr.dst = -1
pkt3.vnhdr.send_time = time.time()
pkt3.join_hdr.type = HEART_BEAT
pkt3.join_hdr.regionX = 1
pkt3.join_hdr.regionY = 1
pkt3.join_hdr.src = 1337
pkt3.join_hdr.dst = -1
pkt3.join_hdr.send_time = time.time()
pkt3.payload = "PAYLOAD HERE"

# test join.recv() path
send(pkt3, 0)
node1_ja.recv(pkt3)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'




# construct test packet
pkt5 = Packet()
pkt5.vnhdr.type = JOIN_MSG
pkt5.vnhdr.subtype = ""
pkt5.vnhdr.src = 1337
pkt5.vnhdr.dst = -1
pkt5.vnhdr.send_time = time.time()
pkt5.join_hdr.type = LEADER_ELECT
pkt5.join_hdr.regionX = 0
pkt5.join_hdr.regionY = 0
pkt5.join_hdr.old_x = 0
pkt5.join_hdr.old_y = 0
pkt5.join_hdr.src = 1337
pkt5.join_hdr.dst = -1
pkt5.join_hdr.seq = 1
pkt5.join_hdr.version = 1
pkt5.join_hdr.send_time = time.time()
pkt5.payload = "PAYLOAD HERE"

# test join.recv() path
send(pkt5, 0)
node1_ja.recv(pkt5)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'



# construct test packet
pkt6 = Packet()
pkt6.vnhdr.type = JOIN_MSG
pkt6.vnhdr.subtype = ""
pkt6.vnhdr.src = 1337
pkt6.vnhdr.dst = -1
pkt6.vnhdr.send_time = time.time()
pkt6.join_hdr.type = LEADER_ACK_REMOTE
pkt6.join_hdr.regionX = 0
pkt6.join_hdr.regionY = 0
pkt6.join_hdr.old_x = 0
pkt6.join_hdr.old_y = 0
pkt6.join_hdr.src = 1337
pkt6.join_hdr.dst = 1
pkt6.join_hdr.seq = 1
pkt6.join_hdr.version = 1
pkt6.join_hdr.send_time = time.time()
pkt6.payload = "PAYLOAD HERE"

# test join.recv() path
send(pkt6, 0)
node1_ja.recv(pkt6)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'




# construct test packet
pkt7 = Packet()
pkt7.vnhdr.type = JOIN_MSG
pkt7.vnhdr.subtype = ""
pkt7.vnhdr.src = 1337
pkt7.vnhdr.dst = -1
pkt7.vnhdr.send_time = time.time()
pkt7.join_hdr.type = LEADER_ACK_ACK
pkt7.join_hdr.regionX = 0
pkt7.join_hdr.regionY = 0
pkt7.join_hdr.old_x = 0
pkt7.join_hdr.old_y = 0
pkt7.join_hdr.src = 1337
pkt7.join_hdr.dst = 1
pkt7.join_hdr.seq = 1
pkt7.join_hdr.version = 1
pkt7.join_hdr.send_time = time.time()
pkt7.payload = "PAYLOAD HERE"


# test join.recv() path
send(pkt7, 0)
node1_ja.recv(pkt7)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'



# construct test packet
pkt4 = Packet()
pkt4.vnhdr.type = JOIN_MSG
pkt4.vnhdr.subtype = ""
pkt4.vnhdr.src = 1337
pkt4.vnhdr.dst = -1
pkt4.vnhdr.send_time = time.time()
pkt4.join_hdr.type = LEADER_REPLY
pkt4.join_hdr.regionX = 0
pkt4.join_hdr.regionY = 0
pkt4.join_hdr.src = 1337
pkt4.join_hdr.dst = 1
pkt4.join_hdr.seq = 1
pkt4.join_hdr.send_time = time.time()
pkt4.payload = "PAYLOAD HERE"


# test join.recv() path
send(pkt4, 0)
node1_ja.recv(pkt4)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'





node1_ja.status_reset()
print '################# STATUS RESET ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'


# construct test packet
pkt1 = Packet()
pkt1.vnhdr.type = SYNC_MSG
pkt1.vnhdr.subtype = ST_VER
pkt1.vnhdr.src = 1337
pkt1.vnhdr.dst = 1
pkt1.vnhdr.send_time = time.time()
pkt1.join_hdr.type = ""
pkt1.payload = ""


# test join.recv() path
send(pkt1, 0)
node1_ja.recv(pkt1)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'



# construct test packet
pkt2 = Packet()
pkt2.vnhdr.type = SYNC_MSG
pkt2.vnhdr.subtype = ST_SYNCED
pkt2.vnhdr.src = 1337
pkt2.vnhdr.dst = 1
pkt2.vnhdr.send_time = time.time()
pkt2.join_hdr.type = ""
pkt2.payload = ""

# test join.recv() path
send(pkt2, 0)
node1_ja.recv(pkt2)

print '################# STATUS ####################'
print '### node1_ja.state_synced_: ' + str(node1_ja.state_synced_)
print '### node1_ja.leader_start_: ' + str(node1_ja.leader_start_)
print '### node1_ja.leader_status_: ' + str(node1_ja.leader_status_)
print '\n\n'






# START TESTS
# construct parking server joinagent
node1_parkingserver = ParkingServerAgent()
node1_parkingserver.reset_states()
node1_parkingserver.server_init()
node1_parkingserver.handle_packet(pkt1)
node1_parkingserver.handle_packet(pkt2)
node1_parkingserver.handle_packet(pkt3)
node1_parkingserver.handle_packet(pkt4)
node1_parkingserver.handle_packet(pkt5)
node1_parkingserver.handle_packet(pkt6)
node1_parkingserver.check_resending_status()
node1_parkingserver.check_parking_status()

#node1_parkingserver.getState()
#node1_parkingserver.getStateSize()
#node1_parkingserver.saveState(pkt1)



node1_parkingclient = ParkingClientAgent()
#node1_parkingclient.parking_timeout(0)
#node1_parkingclient.resending_timeout(0)
#node1_parkingclient.reset_states()
#node1_parkingclient.server_init()