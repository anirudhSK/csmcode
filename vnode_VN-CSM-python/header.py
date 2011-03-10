#leader election state machine state id's
#should be moved to join.h
#define DEAD -2 		#a node that is powered off
#define UNKNOWN -1		#a node that enters a region but doesn't know its leader status
#define REQUESTED 0		#a node that starts leader election
#define LEADER 1		#a node that claims to be the leader
#define NON_LEADER 2		#a node that claims to be the non-leader
#define UNSTABLE 3		#a node that missed at least one heartbeat message, TODO Is this really used in our case ? 
#define PENDING 4		#requested from nearby VN 
#define OLD_LEADER 5		#old leader

#leader election message types (struct hdr_join), for agent join
#define LEADER_REQUEST 0#request for leadership
#define LEADER_REPLY 1	#reply for leadership request
#define HEART_BEAT 2	#heartbeat message used to claim leadership
#define LEADER_LEFT 3	#the current leader has left
#define HEART_BEAT_ACK 4 #response from secondary leader
#define LEADER_LEFT_ACK 5 #ack for leader left msg
#define LEADER_ELECT 6	#leadership election start
#define LEADER_REQUEST_REMOTE 7	#leadership request from remote region
#define LEADER_ACK_REMOTE 8	#leadership ack from remote region
#define LEADER_ACK_ACK 9	#leadership ack_ack

#answer types for LEADER_REPLY messages
#define CONSENT 1 #ok
#define DISSENT 2 #need negotiation
#define NOWAY 0 #rejection since the receiver is a leader

#INTERNAL loopback messages
#define NEWLEADER 99 #from the join agent to the vns/vnc agent, notifying the change of leader status to leader
#define NEWREGION 88 #from the join agent to the vns/vnc agent, notifying the change of region
#define NONLEADER 77 #from the join agent to the vns/vnc agent, notifying the change of leader status to non-leader
#define RG_UPDATE 66 #from the join agent to the vns agent, notifying the change of neighbor regions' activeness
#define ST_SYNCED 55 #from the vns agent to the join agent, notifying that the local node has synchronized its state with the leader
#define BCKLEADER 44 #secondary leader
#define ST_VER 56 #from the vns agent to the join agent, notifying the version #
#define OLDLEADER 33 #

#VNS message types, struct header_vns
#define SYN_REQUEST 0
#define SYN_ACK 1

#Application message types, type 0, 1, 99, 88, 77, 66 reserved for vns layer messages
#define REQUEST 2
#define OFFER 3
#define ACQUIRE 4
#define ACK 5
#define RENEW 6
#define RACK 7

#message class, sub types in vns messages.
#This is for convenience, actually the internal loopback messages, vns messages and application messages should use
#separate message formats and data structures. However, it will be hard to tell from the
#same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
#define INTERNAL_MESSAGE 0 			#internal loopback messsages, shall be limited at vnlayer
#define SYNC_MESSAGE 1 				#sync_request and sync_ack messages, shall be limited at vnlayer
#define SERVER_MESSAGE 2 			#server messages sent directly from the servers, this set of messages will be used in synchronization
#define FORWARDED_SERVER_MESSAGE 3 	#forwarded server messages, needed for emptying sending queues of non-leader nodes, no need for synchronization
#define FORWARDED_CLIENT_MESSAGE 4 	#forwarded client messages, needed for emptying sending queues of non-leader nodes, no need for synchronization
#define CLIENT_MESSAGE 5			#messages sent directly from clients, this set of messages will be ignored by the vns layer packet processing
##define SERVER_MESSAGE_AODV 6		#special message class designed for AODV/VN protocol. server messages will be sent to client port and
#define PARKING_REQUEST 6
#define PARKING_REPLY 7
#define PARKING_ACK 8
#define PARKING_REQUEST_RESEND 9

#transmission type
#define FLOOD 0
#define UNICAST 1

#forwardingType
#define LOCAL 0			#local forwarding, last hop
#define INTER_REGION 1	#inter-region forwarding

#sending type
#define SEND 0
#define FORWARD 1
#define SENDLOOPBACK 2 #no effect on bandwidth

#neighbor status
#define ACTIVE 1
#define INACTIVE 0
#define INVALID -1

#define PGEN_DMSG 333 #PGEN_DMSG

class hdr_pgen:
	#int type; #packet type
	#double send_time; #sending time
	#int src; #source address
	#int dst; #destination address
	#int seq; #sequence number
	
	def __init__(self):
		return
	
	def toString(self):
		return "%d,%.4f,%d,%d,%d" % (self.type, self.send_time, self.src, self.dst, self.seq)

#common message header, message types
#define JOIN_MSG	1 #JOIN agent messages, subtype LEADER_REQUEST, LEADER_REPLY, HEART_BEAT, LEADER_LEFT
#define SYNC_MSG	2 #Synchronization messages, subtype SYN_REQUEST, SYN_ACK
#define LEADER_MSG	3 #loopback messages, leadership info, subtype NEWLEADER, NONLEADER
#define REGION_MSG	4 #loopback messages, region info, subtype NEWREGION
#define APPL_MSG	5 #Application messages, to be passed on to the application layer, subtypes: INTERNAL_MESSAGE,



#
# Common Message Header for virtual node system.Including the vns layer synchonization message,
# internal loopback messages from the join agent and application messages defined by the user.
# This is for convenience, actually the loopback messages, vns messages and application messages should use
# separate message formats and data structures. However, it will be hard to tell from the
# same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
# The message_layer field is used to tell apart the three type of messages and the messages that need to be used
# for synchronization.
#
class hdr_vncommon:
	#int type;			#Top level message type:
	#int subtype; 		#Subtype
	#double send_time;	#sending time
 	#int regionX;		#source region X
 	#int regionY;		#source region Y
 	#int src;			#source node
 	#int dst;			#destination node
 	#int send_type;		#sending type. for the VNLayer to generate debugging info
 	#unsigned int hash;	#this is to store the state hash.

	def __init__(self):
		return
		
	def toString(self):
		return "%d,%d,%.4f,(%d.%d),%d,%d" % (self.type, self.subtype, self.send_time, self.regionX, self.regionY, self.src, self.dst)



#
#	Message format of the vns messages. Including the vns layer synchonization message,
#  internal loopback messages from the join agent and application messages defined by the user.
#  This is for convenience, actually the loopback messages, vns messages and application messages should use
#  separate message formats and data structures. However, it will be hard to tell from the
#  same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
#  The message_layer field is used to tell apart the three type of messages and the messages that need to be used
#  for synchronization.
#
class hdr_vns:
#	int message_class; 	#needed by consistency manager to classify messages
						#INTERNAL_MESSAGE 0, SYNC_MESSAGE 1, SERVER_MESSAGE 2, FORWARDED_SERVER_MESSAGE 3, FORWARDED_CLIENT_MESSAGE 4, CLIENT_MESSAGE 5
#	int type;			#message type: 0 request, 1 reply,

#	double send_time;	#sending time
#
#	int regionX;		#source region X
#	int regionY;		#source region Y
#	int client_id;		#the node id of the client
#	int server_regionX;	#the region id X of the server
#	int server_regionY;	#the region id Y of the server
#	int seq;			#request sequence number, created from the requestor
#	int addr;			#address given, last two bytes of a 4 byte IP address
#	int transmissionType; 	# broadcast or unicast 0 FLOOD, non-negative integ    er: unicast
#	int hop_count; 			# the count of the hops the packet has travelled t    hrough.
#	int source_route_len; 	# length of the source route
#	int src_route[MAX_ROUTE_LENGTH][2]; # the buffer for the source route
#	double lease_time; 		# the time deadline that a response for the messag    e is expected
							# used to time out stale messages
#	int forwardingType; 	# LOCAL message 0 or INTER_REGION message 1
#	int sendingType; 		# SEND 0, FORWARD 1, SENDLOOPBACK 2
	
	def __init__(self):
		return

	# toString() method used to display the message content
	# used by the logging functions.
	# Be aware the memory used by the string need to be released after logging is done.
	def toString(self):
		str = ""
		sroute = ""
		tmp = ""
		for i in range(self.source_route_len):
			if(i+1==self.source_route_len):
				tmp = "%d.%d" % (self.src_route[i][0], self.src_route[i][1])
			else:
				tmp = "%d.%d|" % (self.src_route[i][0], self.src_route[i][1])
			sroute += tmp
		str += "%d,%.4f,(%d.%d),%d,(%d.%d),%d,%d,%d,%d,%d,%s,%.4f,%d,%d,C:%d" % (self.type, self.send_time, self.regionX, self.regionY, self.client_id, self.server_regionX, self.server_regionY, self.seq, self.addr, self.transmissionType, self.hop_count, self.source_route_len, sroute, self.lease_time, self.forwardingType, self.sendingType, self.message_class)
		return str


	
#
# Message format of the leader election messages.
#
class hdr_join:
#	int type;		#message type: 0 request, 1 reply, heartbeat 2,
#	double send_time;	#sending time
#	double ldr_start;	# when the sender became a leader or got its state synchronized
#	double time_to_leave; ##time to leave the current region
#	int src;		#the node id of the sender
#	int dst;		#the node id of the receiver
#	int regionX;	#source region X
#	int regionY;	#source region Y
#	int seq;		#sequence number of request from the requestor
#	int answer;		#the answer of reply, 0 no, 1 yes, 2 conflict
#	int version;		#version of the leader. NIKET	
#	int old_x;		#old_x of the leader. NIKET	
#	int old_y;		#old_y of the leader. NIKET	
#	int parking_spots;
	def __init__(self):
		return
	
	def toString(self):
		return "%d,%.4f,%.4f,(%d.%d),%d,%d,%d,%d,%.4f" % (self.type, self.send_time, self.ldr_start, self.regionX, self.regionY, self.src, self.dst, self.seq, self.answer, self.time_to_leave)



##AODV over VNLayer message types
#define RREQ 1
#define RREP 2
#define RERR 3
#define DMSG 10
#define DACK 11
#define CLCK 22

#
# Message format of the AODV messages.
#
class hdr_vnaodv:
#	int type;		#message type: 0 request, 1 reply, heartbeat 2,
#	double send_time;	#sending time
#	int regionX;	#source region X
#	int regionY;	#source region Y
#	int rreqRegionX; #region launching the RREQ
#	int rreqRegionY; #region lannching the RREQ
#	int nextX;		#next hop region id X
#	int nextY;		#next hop region id Y
#	int destid;		#final destination node's id
#	int destseq;	#route sequence number for the destination id
#	int origid;		#originator of a route request
#	int origseq;	#the route sequence number of the originator
#	int hopcount;	#the number of hops the message has traversed
#	int ttl;		#the number of hops the message can still be forwarded
#	double lifetime;#the life time of the route being sent
#	int rreqseq;	#RREQ sequence number
#	int forwardingType; #local or inter-region message
#	int size;		#the size of the payload
	def __init__(self):
		return
	
	def toString(self):
		return "%d,%.4f,(%d.%d),(%d.%d),(%d.%d),%d,%d,%d,%d,%d,%d,%d,%.4f,%d" % (self.type, self.send_time, self.regionX, self.regionY, self.rreqRegionX, self.rreqRegionY, self.nextX, self.nextY, self.destid, self.destseq, self.origid, self.origseq, self.hopcount, self.ttl, self.rreqseq, self.lifetime, self.forwardingType)
		


#define UPDATE 14   #route update
#define QUERY 15	#explict route request

#
# Message format of the parking messages.
#
class hdr_vnparking:
#	int type;		#message type: 0 data, 1 route update, 2,
#	int srcX;	#source region X
#	int srcY;	#source region Y
#	int destX;	#destination region X
#	int destY;	#destination region Y
#	int nextX;	#next hop region X
#	int nextY;	#next hop region Y
#	int isWrite;
#	int isSuccess;
#	double m_send_time; 
#	int m_token;
#	int m_count;
#	int destid;
#	int origid;		#originator of a route request
#	int size;		#the size of the payload, which is the routing information updates
	
	def __init__(self):
		return
	
	def toString(self):
		return "%d,%.4f,(%d.%d),(%d.%d),%d,%d, %d" % (self.type, self.m_send_time, self.srcX, self.srcY, self.destX, self.destY, self.destid, self.origid, self.isWrite)



##SNSR network over VNLayer message types
#define BEACON 1
#define REPORT 2
#define INFORM 3
#define WAKEUP 4

#
# Message format of sensor network messages.
#
class hdr_vnsnsr:
#	int type;		#message type
#	double send_time;	#sending time
#	int sender; #sender id
#	int seq;
#	int regionX;	#source region X
#	int regionY;	#source region Y
#	float X; #x coordinate
#	float Y; #y coordinate
#	float distance; #the distance between the target and the sensor. used in REPORT message.
	
	def __init__(self):
		return
	
	def toString(self):
		return "%d,%.4f,%d,%d,(%d.%d),(%f.%f),%f" % (self.type, self.send_time, self.sender, self.seq, self.regionX, self.regionY, self.X, self.Y, self.distance)
