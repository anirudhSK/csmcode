#!/bin/python
from log_parking import *
from header import *
from threading import Timer
from recurring_timer import RecurringTimer
import time


### DEFINE ###
NEW_NODE = 0
MOVING = 1
STOPPED = 2
NUM_NEIGHBORS = 8 #for square regions, the maximum number of neighbors is 8
MAX_APPS = 10			 #number of apps ports that the agent report region and leadership status to
						  #[Might want to put this in TCL script later.]
CODE_JOIN = "JOIN"		#code for leader election related events
CODE_MOVE = "MOVE"		#code for node motion related events
CODE_INFO = "INFO"		#code for other events



class oldLeaderData:
	def __init__(self, ver, ox, oy, spots):
		self.latest_version = ver
		self.old_x = ox
		self.old_y = oy
		self.parking_spots = spots
		self.retries = 0
		self.is_valid = True
		self.leader_ack = False	
		self.leader_ack_ack = False
		self.new_leader = -1


class JoinAgent:
	def __init__(self, id, slowInterval_, interval_, maxX_, maxY_, rows_, columns_, ):
		# bind
		#bind_offset(&hdr_vns::offset_);
		#bind_offset(&hdr_vncommon::offset_);
		#bind_offset(&hdr_join::offset_);
#		self.port_number_ = 
		self.maxX_ = maxX_
		self.maxY_ = maxY_
		self.columns_ = columns_
		self.rows_ = rows_
		self.nodeID_ = id
		self.regionX_ = 0
		self.regionY_ = 0
		self.seq_ = 0
		self.status_ = UNKNOWN
		self.leader_ = UNKNOWN
		self.leader_status_ = UNKNOWN
		self.beat_period_ = 2
#		self.max_delay_ = 
		self.claim_period_ = 1
#		self.beat_miss_limit_ = 
		self.interval_ = interval_
		self.slowInterval_ = slowInterval_
#		self.zeroDistance_ = 
#		self.packetSize_ = 
		# end binds
		
		self.leader_start_ = UNKNOWN
		
		
		# modified by GPS routines
		self.x = 0
		self.y = 0


#TODO what initial time to start out with? do we need to start at app start, or wait until reschedule event? # Timers
		self.join_timer_ = RecurringTimer(3, self.check_location) # location check timeout
		self.leader_req_timer_ = RecurringTimer(3, self.check_leader_status) # leader request timeout
		self.old_leader_timer = RecurringTimer(3, self.check_old_leader_status)
		
		# Copied from Agent/Join constructor
		self.beat_misses_ = 0
		self.num_apps = 0
		
		self.state_synced_ = False # state unknown
		self.time_to_leave_ = time.time() + 5 # what to set this to?
		self.old_leader_retries = 0

		# srand(time(NULL)+nodeID_) #TODO rand seed
		
#TODO shmid, logfile
		
		# NIKET - added for leader left
		self.old_leaders = []
		
		# neighbor status
		self.neighbors = [[0]*2 for x in xrange(NUM_NEIGHBORS)] #the list of neighbor region ids
		self.neighbor_flags = [0]*NUM_NEIGHBORS
		self.neighbor_flags = [0]*NUM_NEIGHBORS #which of the 8 neighbors can be used for routing
		self.neighbor_timers = [0]*NUM_NEIGHBORS #use the heartbeat messages to timeout inactive neighbors
		
		return
	
	# The method processing every packet received by the agent
	# void JoinAgent::recv(Packet* pkt, Handler*)
	def recv(self, pkt):
		print 'JoinAgent.recv'
		
		vnhdr = pkt.vnhdr
		
		if(vnhdr.type == SYNC_MSG): # this must be a loopback message from the VNS thread 
			if(vnhdr.subtype == ST_SYNCED): # Received a synchronization message 
				self.state_synced_ = True
				self.leader_start_ = vnhdr.send_time
			elif (vnhdr.subtype == ST_VER): # version synchronization message Do we really need loopbacks, can't we just have shared vairables , that are set by one thread and read by another.
				self.m_version = vnhdr.src # Just reusing the src field of vnhdr for sending version;
		
		elif(vnhdr.type == JOIN_MSG): # not a loopback message, but rather received from another node.
			hdr = pkt.join_hdr
			
			if(hdr.regionX != self.regionX_ or hdr.regionY != self.regionY_): # not the same region, Ask Niket: what is this?
				if(hdr.type == HEART_BEAT and hdr.dst == -1 and abs(hdr.regionX - self.regionX_) <= 1 and abs(hdr.regionY - self.regionY_) <= 1):
					self.copy_loopback(pkt, 17920) # what is this again ? 17920 is the port , HEART_BEAT is not used in Niket's code from what he says. Niket says it might be useful for geographic routing . Check for HEART BEAT elsewhere too
			
			# incoming packet has to be either destined for the node or broadcast, the source has to be in the same region.
			if((hdr.dst == self.nodeID_ or hdr.dst == -1) and hdr.regionX == self.regionX_ and hdr.regionY == self.regionY_):
#				log(JOIN, RECV, hdr.toString());
				if (hdr.type == LEADER_REQUEST): # got a request message, this is received by a node already in the region when a new guy comes in 
					if(self.leader_status_ == LEADER): #I am a leader already, absolutely say NO to  the other guy 
						self.send_unicast(LEADER_REPLY, hdr.src, CONSENT, hdr.seq) # CONSENT or DISSENT  ?? Ask Niket.
						# This is telling the new node, ok you can come and join the region or no there are too many here already. This was never implemented though from what Niket told me.   
				elif(hdr.type == LEADER_REPLY): # I got a reply message, this message is unicast, so it can be heard only by the requestor ie if I sent a LEADER_REQUEST 
					if(self.leader_status_ == REQUESTED and hdr.dst == self.nodeID_ and hdr.seq== self.seq_): #the messages is for my last request, that is I sent a request, Niket:  where is REQUESTED set
						self.leader_status_ = UNKNOWN  # back to being a lame non leader, because my LEADER_REQUEST was turned down by a LEADER_REPLY , when will a LEADER_REQUEST ever not get turned down ? 
#						if(hdr.answer == CONSENT) #Participate in the VN as non-leader
#						{
#							leader_ = hdr.src;
#							self.leader_status_ = NON_LEADER; #REQUESTED
#							send_loopback(NONLEADER);
#						}
#						else if(hdr.answer == DISSENT) #Do not participate in the VN
#						{
#							# Do nothing. 
#						}
			
			if(hdr.type == LEADER_ELECT and (hdr.dst == -1) and (hdr.old_x == self.regionX_) and (hdr.old_y == self.regionY_)): # A just exited leader whose old region was this region is trying to elect a new leader -1 is for b'cast # you as a recipient of this message are among the "chosen" ones 
				# Start leader election
				assert(self.leader_status_ != LEADER); #There cannot be leader in this region, ie I am not a leader for instance, the assert only checks that I am not a leader 
				if(self.m_version == hdr.version):
					#start the leader election process. 
					self.leader_status_ = PENDING
					self.send_left_unicast(LEADER_REQUEST_REMOTE, hdr.src, self.m_version, hdr.old_x, hdr.old_y, UNKNOWN) # send a LEADER_REQUEST_REMOTE to the node in the other region who tried to elect a leader 
					# This amounts to saying, ok you wanted to elect a leader in my region, so yes, now I request to be one
					# It's a remote request since you are sending it to the guy in the next region, not your own. 
				else:
#TODO - non-sync nodes should be delayed a bit so that sync wins , Ask Niket why this is not done , wrong code version ?? Isn't the whole point to allow only synced nodes to be leader ? 
					self.leader_status_ = PENDING
					self.send_left_unicast(LEADER_REQUEST_REMOTE, hdr.src, self.m_version, hdr.old_x, hdr.old_y, UNKNOWN)
			
			if(hdr.type == LEADER_REQUEST_REMOTE and (hdr.dst == self.nodeID_)): # This is received by the exited node ie the Elector node, ie the node in the next region  
																					# I don't understand why there are multiple old leader datas ? Or should I code this blindly 
				pass
				for l in range(len(self.old_leaders)):
					old_l = self.old_leaders[l]
					if(old_l.old_x == hdr.regionX and old_l.old_y == hdr.regionY):
						if(old_l.is_valid == False):
							continue
						if(old_l.leader_ack == False):
							if(self.m_version == hdr.version or (old_l.retries >= 3)):
								self.old_leaders[l].leader_ack = True
								self.old_leaders[l].new_leader = hdr.src
								self.send_left_unicast(LEADER_ACK_REMOTE, hdr.src, UNKNOWN, old_l.old_x, old_l.old_y, UNKNOWN)
								# this says: Ok as a remote elector I guarantee you permission to be the leader of this region. 
								break
			
			if(hdr.type == LEADER_ACK_REMOTE and hdr.dst == self.nodeID_ and (hdr.old_x == self.regionX_) and (hdr.old_y == self.regionY_)):
				if(self.leader_status_ == PENDING): # my leader status was pending, and now I set myself leader.						
					self.setNeighbors() # initiate neighbor set
					#self.leader_start_ = Scheduler::instance().clock();
					self.leader_start_ = time.time()
					self.leader_ = self.nodeID_
					self.leader_status_ = LEADER
					self.send_left_unicast(LEADER_ACK_ACK, hdr.src, self.m_version, hdr.old_x, hdr.old_y, UNKNOWN)
					#LogParkingFile.setLeaderActive(self.regionX_, self.regionY_, True, Scheduler::instance().clock())
					LogParkingFile.setLeaderActive(self.regionX_, self.regionY_, True, time.time())
					self.state_synced_ = True
					self.send_loopback(NEWLEADER)
				elif(self.leader_status_ == LEADER):									# Ask Niket: When would this ever happen ? 
					self.send_left_unicast(LEADER_ACK_ACK, hdr.src, self.m_version, hdr.old_x, hdr.old_y, UNKNOWN)
					
			if(hdr.type == LEADER_ACK_ACK and (hdr.dst == self.nodeID_)): # The elector has now handed over responsibility. since it got an ACK ACK back from the newly elected node
																		# This is a 4 way handshake, LEADER_ELECT, LEADER_REQUEST_REMOTE, LEADER_ACK_REMOTE, LEADER_ACK_ACK
				pass
				for l in range(len(self.old_leaders)):
					old_l = self.old_leaders[l]
					if(old_l.old_x == hdr.regionX and old_l.old_y == hdr.regionY):	
						assert(old_l.leader_ack == True)
						if(old_l.leader_ack_ack == False):
							assert(old_l.is_valid == True)
							assert(old_l.new_leader == hdr.src)
							self.old_leaders[l].leader_ack_ack = True
							self.old_leaders[l].is_valid = False
							break
		return
	
	# void JoinAgent::check_old_leader_status()
	def check_old_leader_status(self):
		print 'JoinAgent.check_old_leader_status'
		for l in range(len(self.old_leaders)):
			old_l = self.old_leaders[l]
			if(old_l.is_valid == False):
				continue
			if(old_l.retries < 40):
				if(old_l.leader_ack == False):
					self.old_leaders[l].retries += 1
					assert(old_l.parking_spots == LogParkingFile.getFreeSpots(old_l.old_x, old_l.old_y))
					self.send_left_broadcast(LEADER_ELECT, old_l.latest_version, old_l.parking_spots, old_l.old_x, old_l.old_y, UNKNOWN)
					self.old_leader_timer.resched(4*claim_period_)
				elif(old_l.leader_ack_ack == False):
					self.old_leaders[l].retries += 1
					assert(old_l.new_leader != -1)
					self.send_left_unicast(LEADER_ACK_REMOTE, old_l.new_leader, UNKNOWN, old_l.old_x, old_l.old_y, UNKNOWN)
					self.old_leader_timer.resched(4*claim_period_)
			else:
				if(self.old_leaders[l].leader_ack_ack):
					self.old_leaders[l].is_valid = False
					continue
				print "Node - %d:: was old leader and GAVEUP electing a new leader. setting region to inactive" % (nodeID_)
				self.old_leaders[l].retries = 0
				self.old_leaders[l].leader_ack = True
				self.old_leaders[l].leader_ack_ack = True
				self.old_leaders[l].is_valid = False
				LogParkingFile.setRegionInActive(old_l.old_x, old_l.old_y, nodeID_, -1)
		return
	
	# Awaken by the leader election timer.
	# Check the current status to see if anything needs to be done.
	# void JoinAgent::check_leader_status()
	def check_leader_status(self):
		print 'JoinAgent.check_leader_status'
		if(self.leader_status_ == UNKNOWN):
			pass # Ignore since I should have got it. 
		elif(self.leader_status_ == REQUESTED): # No response to leadership request. Check central server now. 
			if not LogParkingFile.isRegionActive(self.regionX_, self.regionY_):
				LogParkingFile.setRegionActive(self.regionX_, self.regionY_, self.nodeID_);
				self.setNeighbors();
				# self.leader_start_ = Scheduler::instance().clock();
				self.leader_start_ = time.time()
				self.leader_ = self.nodeID_;
				self.leader_status_ = LEADER;
				LogParkingFile.setLeaderActive(self.regionX_, self.regionY_, True, self.leader_start_);
				self.state_synced_ = True;
				self.send_loopback(NEWLEADER);
			else:
				pass # Do nothing as there is already a VN running, which doesn't want me. 
		return
	
	# Awaken by the location checking timer.
	# Check the current location to see if anything needs to be done
	# double JoinAgent::check_location()
	def check_location(self):
		print 'JoinAgent.check_location'
		
		# use time to read the current GPS from a file  interpolating if necessary
		x = self.x
		y = self.y
		
		#get current region
		rx = 0
		ry = 0
		(rx, ry) = self.getRegion(x, y)
		
		self.region_changed = False
		
		if(rx != self.regionX_ or ry != self.regionY_): #region changed
			self.region_changed = True
			
			LogParkingFile.setNumNodes(self.regionX_, self.regionY_, -1)
			LogParkingFile.setNumNodes(rx, ry, 1)
			
			orx = self.regionX_ # old region x and old region y
			ory = self.regionY_
			
			self.regionX_ = rx
			self.regionY_ = ry
			
			#tell other nodes in the region that the leader has left
			if(self.leader_status_ == LEADER):
				LogParkingFile.setLeaderActive(orx, ory, False, time.time())
				self.old_leader_retries = 0
				self.old_leader_timer.resched(2*self.claim_period_)
				spots = LogParkingFile.getFreeSpots(orx, ory) # shared state among threads 
				old_l = oldLeaderData(self.m_version, orx, ory, spots)
				self.old_leaders.append(old_l) # corner case
				# is to augment the set of old leader data you have
				self.send_left_broadcast(LEADER_ELECT, self.m_version, spots, orx, ory, UNKNOWN)
			
			self.status_reset()
			self.leader_status_ = REQUESTED #REQUESTED, this is where it is set 
			self.send_loopback(NEWREGION)# from join agent to parking server, Niket feels shared memory is just an implementation thing
			self.send_broadcast(LEADER_REQUEST, UNKNOWN)#send a leader request message
			self.leader_req_timer_.resched(2*self.claim_period_) #wait for a claim period for someone to become leader ,
			  # how long do you wait before concluding there is no leader, how do these affect performance ?
			  # always check for state sanity  
			
			str = ""
			
			if(self.leader_status_ == DEAD):
				str = "(%d.%d),<,(-1.-1)" %(self.regionX_, self.regionY_)
#				if(LOG_ENABLED):
#					log(MOVE, START, str)
			else:
				str = "(%d.%d),<,(%d.%d)" % (self.regionX_, self.regionY_, orx, ory)
#				if(LOG_ENABLED):
#					log_info(MOVE,ENTER,str)
#		 # ignore this is checkLocation polls repeatedly 
#		#log_info(MOVE, SPEED, "speed checking ...");
#		#get the speed and direction
#		double destX,destY,speed;
#		destX = this_node .destX();
#		destY = this_node .destY();
#		speed = this_node .speed();
#
#		#check the destination region id
#		int drx, dry;
#		getRegion(destX,destY,&drx,&dry);
#
#		#get the speeds on every single direction
#		double speedX, speedY, speedZ;
#		this_node.getVelo(&speedX,&speedY,&speedZ);
		
		#schedule the next event
		self.status_ = MOVING
#TODO set wait_time: is this a delta or an absolute time?
		wait_time = 1 
		self.join_timer_.resched(wait_time)
#		if(LOG_ENABLED):
#			char * str = (char *) malloc(MSG_STRING_SIZE);
#			sprintf(str,"%.2f,%.2f,(%d.%d),>,%.2f,%.2f,(%d.%d),%.2f,(%.2f.%.2f)", x, y, rx, ry, destX,destY, drx, dry, speed, speedX, speedY);
#			log(MOVE,MOVING,str);
		
		self.time_to_leave_ = time.time() + wait_time #set to wait time
		return wait_time
		
	
	# void JoinAgent::send_left_broadcast(int msgType, int version, int parking_spots, int old_x, int old_y, int answer)
	def send_left_broadcast(self, msgType, version, parking_spots, old_x, old_y, answer):
		print 'JoinAgent.send_left_broadcast'
		pkt = Packet()
# cmn_hdr
#		cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_REGION;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = IP_BROADCAST;
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = -1; #broadcasting address, should be -1
#		iph.dport() = MY_PORT_;
#		iph.ttl() = 1;
		
		pkt.vnhdr.type = JOIN_MSG;
		pkt.vnhdr.subtype = msgType;
		
		pkt.join_hdr.type = msgType; #leader request
		pkt.join_hdr.send_time = time.time()
		pkt.join_hdr.ldr_start = self.leader_start_;
		pkt.join_hdr.time_to_leave = self.time_to_leave_;
		
		self.last_sent_ = pkt.join_hdr.send_time;
		
		pkt.join_hdr.regionX = self.regionX_; #sender's region X
		pkt.join_hdr.regionY = self.regionY_; #sender's region Y
		
		pkt.join_hdr.old_x = old_x; #sender's region X
		pkt.join_hdr.old_y = old_y; #sender's region Y
		
		pkt.join_hdr.version = version; # NIKET
		pkt.join_hdr.parking_spots = parking_spots; # NIKET
		
		pkt.join_hdr.src = self.nodeID_; #node id
		pkt.join_hdr.dst = -1; #all nodes
		
		self.seq_ += 1
		pkt.join_hdr.seq = self.seq_ #sender's request sequence number
		pkt.join_hdr.answer = answer; # 0 no, 1 yes
		
#		if(LOG_ENABLED)
#			log(JOIN, SEND, pkt.join_hdr.toString());
		send(pkt, 0);
		return
		
	
	# Send a broadcast leader election message
	# msgType: int message type
	# void JoinAgent::send_broadcast(int msgType, int answer)
	def send_broadcast(self, msgType, answer):
		print 'JoinAgent.send_broadcast'
		pkt = Packet()
# cmn_hdr
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_REGION;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = IP_BROADCAST;
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = -1; #broadcasting address, should be -1
#		iph.dport() = MY_PORT_;
#		iph.ttl() = 1;
		
		pkt.vnhdr.type = JOIN_MSG
		pkt.vnhdr.subtype = msgType
		
		pkt.join_hdr.type = msgType; #leader request
		pkt.join_hdr.send_time = time.time()
		pkt.join_hdr.ldr_start = self.leader_start_
		pkt.join_hdr.time_to_leave = self.time_to_leave_
		
		self.last_sent_ = pkt.join_hdr.send_time
		
		pkt.join_hdr.regionX = self.regionX_ #sender's region X
		pkt.join_hdr.regionY = self.regionY_ #sender's region Y

		pkt.join_hdr.src = self.nodeID_ #node id
		pkt.join_hdr.dst = -1 #all nodes
		
		self.seq_ += 1
		pkt.join_hdr.seq = self.seq_ #sender's request sequence number
		pkt.join_hdr.answer = answer # 0 no, 1 yes
		
#		if(LOG_ENABLED)
#			log(JOIN, SEND, join_hdr.toString());
		send(pkt, 0);
		return
	
	# void JoinAgent::send_left_unicast(int msgType, int dest, int version, int old_x, int old_y, int seq)
	def send_left_unicast(self, msgType, dest, version, old_x, old_y, seq):
		print 'JoinAgent.send_left_unicast'
		pkt = Packet()
# cmn_hdr
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_REGION;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = dest;
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = dest; #broadcasting address, should be -1
#		iph.dport() = MY_PORT_;
#		iph.ttl() = 1;
		
		pkt.vnhdr.type = JOIN_MSG
		pkt.vnhdr.subtype = msgType
		
		pkt.join_hdr.type = msgType; #leader request
		pkt.join_hdr.send_time = time.time()
		pkt.join_hdr.ldr_start = self.leader_start_
		pkt.join_hdr.time_to_leave = self.time_to_leave_
		
		#self.last_sent_ = pkt.join_hdr.send_time
		
		pkt.join_hdr.regionX = self.regionX_ #sender's region X
		pkt.join_hdr.regionY = self.regionY_ #sender's region Y
		
		pkt.join_hdr.src = self.nodeID_ #node id
		pkt.join_hdr.dst = dest #all nodes
		
		pkt.join_hdr.old_x = old_x #sender's region X
		pkt.join_hdr.old_y = old_y #sender's region Y
		
		pkt.join_hdr.version = version
		
		pkt.join_hdr.seq = seq #sender's request sequence number
		
#		if(LOG_ENABLED)
#			log(JOIN, SEND, join_hdr.toString());
		#Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, 0);
		return
	
	#Send a uni-cast leader election message
	# msgType: int message type
 	# dest: int destination node id
	# answer: 0 no, 1 yes
	# seq: int sequence number
	# void JoinAgent::send_unicast(int msgType, int dest, int answer, int seq)
	def send_unicast(self, msgType, dest, answer, seq):
		print 'JoinAgent.send_unicast'
		pkt = Packet()
# cmn_hdr
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_REGION;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = dest;
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = dest; #broadcasting address, should be -1
#		iph.dport() = MY_PORT_;
#		iph.ttl() = 1;
		
		pkt.vnhdr.type = JOIN_MSG;
		pkt.vnhdr.subtype = msgType;
		
		pkt.join_hdr.type = msgType; #leader request
		pkt.join_hdr.send_time = time.time()
		pkt.join_hdr.ldr_start = self.leader_start_;
		pkt.join_hdr.time_to_leave = self.time_to_leave_;
		
		#self.last_sent_ = pkt.join_hdr.send_time;
		
		pkt.join_hdr.regionX = self.regionX_; #sender's region X
		pkt.join_hdr.regionY = self.regionY_; #sender's region Y
		
		pkt.join_hdr.src = self.nodeID_; #node id
		pkt.join_hdr.dst = dest; #all nodes
		
		pkt.join_hdr.seq = seq; #sender's request sequence number
		pkt.join_hdr.answer = answer; # 0 no, 1 yes
		
#		if(LOG_ENABLED)
#			log(JOIN, SEND, join_hdr.toString());
		#Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, 0);
		return
	
	# Send a uni-cast vns message to a destination on a specific port
	# Not used so far
	# void JoinAgent::send_unicast_to(int msgType, int dest, int port)
	def send_unicast_to(self, msgType, dest, port):
		print 'JoinAgent.send_unicast_to'
		pkt = Packet()
# cmn_hdr
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_REGION;
#		#cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = dest;
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = dest;
#		iph.dport() = port;
#		iph.ttl() = 1;
		
		pkt.vns_hdr.type = msgType; #leader request
		pkt.vns_hdr.send_time = time.time()
		
		pkt.vns_hdr.regionX = self.regionX_; #sender's region X
		pkt.vns_hdr.regionY = self.regionY_; #sender's region Y
		
#		if(LOG_ENABLED)
#			log(JOIN, SENDTOPORT, vns_hdr.toString());
		send(pkt, 0);
		return
	
	# Send a loopback vns message to the VNS agents attached to the same node
	# msgType: int message type
	# void JoinAgent::copy_loopback(Packet* pkt_in, int port)
	def copy_loopback(self, pkt_in, port):
		print 'JoinAgent.copy_loopback'
		join_hdr = pkt_in.join_hdr; #old vns header, only used in join agent now
		
		pkt_out = Packet() #create a packet for sending
		
# cmn_hdr	
#		hdr_cmn * cmn_hdr_out = hdr_cmn::access(pkt_out);
#		cmn_hdr_out.ptype() = PT_VNCOMMON;
#		cmn_hdr_out.size() = size_ + IP_HDR_LEN; #
#		cmn_hdr_out.next_hop_ = nodeID_;
#		hdr_ip  * iph_out = HDR_IP(pkt_out);
#		iph_out.saddr() = Agent::addr();
#		iph_out.daddr() = Agent::addr(); #local address
#		iph_out.dport() = port;
#		iph_out.sport() = MY_PORT_; #no need
#		iph_out.ttl() = 1;
		
		pkt_out.vnhdr.type = JOIN_MSG;
		pkt_out.vnhdr.subtype = join_hdr.type;
		pkt_out.vnhdr.regionX = join_hdr.regionX;
		pkt_out.vnhdr.regionY = join_hdr.regionY;
		pkt_out.vnhdr.send_time = join_hdr.send_time;
		
		send(pkt_out, 0);
		return
	
	# Send a loopback vns message to the agents attached to the same node
	# msgType: int message type
	# void JoinAgent::send_loopback(int msgType)
	def send_loopback(self, msgType):
		print 'JoinAgent.send_loopback'
		for i in range(self.num_apps):
			pkt = Packet()
# cmn_hdr
#			hdr_cmn * cmn_hdr = hdr_cmn::access(pkt)
#			cmn_hdr.ptype() = PT_VNCOMMON
#			cmn_hdr.size() = size_ + IP_HDR_LEN # add in IP header
#			cmn_hdr.next_hop_ = nodeID_
#			hdr_ip  * iph = HDR_IP(pkt)
#			iph.saddr() = Agent::addr()
#			iph.daddr() = Agent::addr() #local address
#			#iph.dport() = app_port
#			iph.sport() = MY_PORT_ #no need
#			iph.ttl() = 1
#			iph.dport() = app_port[i]
			
			if(msgType == NEWREGION):
				pkt.vnhdr.type = REGION_MSG
				pkt.vnhdr.subtype = NEWREGION
			elif(msgType == NEWLEADER):
				pkt.vnhdr.type = LEADER_MSG
				pkt.vnhdr.subtype = NEWLEADER
				pkt.vnhdr.dst = self.leader_
			elif(msgType == NONLEADER):
				pkt.vnhdr.type = LEADER_MSG
				pkt.vnhdr.subtype = NONLEADER
				pkt.vnhdr.dst = self.leader_
			elif(msgType == OLDLEADER):
				pkt.vnhdr.type = LEADER_MSG
				pkt.vnhdr.subtype = OLDLEADER
				pkt.vnhdr.dst = self.leader_
			pkt.vnhdr.regionX = self.regionX_
			pkt.vnhdr.regionY = self.regionY_
			pkt.vnhdr.send_time = time.time()
			
			#the fields below are to be removed.
			pkt.vns_hdr.message_class = INTERNAL_MESSAGE #internal loopback message
			pkt.vns_hdr.type = msgType #leader request
			pkt.vns_hdr.send_time = time.time()
			#last_sent_ = join_hdr.send_time;
			
			pkt.vns_hdr.regionX = self.regionX_ #sender's region X
			pkt.vns_hdr.regionY = self.regionY_ #sender's region Y
			
			pkt.vns_hdr.server_regionX = self.leader_
			#Scheduler::instance().schedule(ll,pkt,0.0)
			
			#log_info(JOIN, LOOPBACKSENTTO, app_port[i])
			send(pkt, 0);
		return
	
	# get the id of the region where the node is located
	# double x: the x position of the node
	# double y: the y position of the node
	# int * regionX: the x part of the region id, a pointer
	# int * regionY: the y part of the region id, a pointer
	# return 0: error
	# return 1: normal
	# int JoinAgent::getRegion(double x, double y, int * regionX, int * regionY)
	# WARNING: We're changing the interface to avoid passing pointers
	def getRegion(self, x, y):
		print 'JoinAgent.getRegion'
		xUnit = float(self.maxX_)/self.columns_ #size of a region X
		yUnit = float(self.maxY_)/self.rows_	#size of a region Y
		
		if( x<0 or x>self.maxX_):
			print "X position out of bound\n" #warning.
			return 0
		if( y<0 or y>self.maxY_):
			print "Y position out of bound\n"
			return 0
		
		rx = (int)(x/xUnit);
		ry = (int)(y/yUnit);
		
		if( rx == self.columns_):
			print "X position reached bound\n"
			rx = self.columns_-1;
		if( ry == self.rows_):
			print "Y position reached bound\n"
			ry = self.rows_-1;
		
		return (rx, ry)
	
	# reset states related to leader election
	def status_reset(self):
		self.leader_ = UNKNOWN
		self.leader_status_ = UNKNOWN
		self.last_sent_ = 0
		self.beat_misses_ = 0
		self.leader_start_ = UNKNOWN
		self.state_synced_ = False
	
	# Upon region change, calculate the neighbor list and initialize the status of each neighbor
	# void JoinAgent::setNeighbors()
	def setNeighbors(self):
		print 'JoinAgent.setNeighbors'
		#valid region id
		if(self.regionX_ >=0 and self.regionX_ < self.columns_ and self.regionY_ >= 0 and self.regionY_ < self.rows_):
			for i in range(NUM_NEIGHBORS):
				if i == 0:
					self.neighbors[i][0] = self.regionX_ -1
					self.neighbors[i][1] = self.regionY_ +1
				elif i == 1:
					self.neighbors[i][0] = self.regionX_
					self.neighbors[i][1] = self.regionY_ +1
				elif i == 2:
					self.neighbors[i][0] = self.regionX_ +1
					self.neighbors[i][1] = self.regionY_ +1
				elif i == 3:
					self.neighbors[i][0] = self.regionX_ +1
					self.neighbors[i][1] = self.regionY_
				elif i == 4:
					self.neighbors[i][0] = self.regionX_ +1
					self.neighbors[i][1] = self.regionY_ -1
				elif i == 5:
					self.neighbors[i][0] = self.regionX_
					self.neighbors[i][1] = self.regionY_ -1
				elif i == 6:
					self.neighbors[i][0] = self.regionX_ -1
					self.neighbors[i][1] = self.regionY_ -1
				elif i == 7:
					self.neighbors[i][0] = self.regionX_ -1
					self.neighbors[i][1] = self.regionY_
				else:
					self.neighbors[i][0] = -1
					self.neighbors[i][1] = -1
				#set invalid neighbor flags to -1
				if(self.neighbors[i][0] <0 or self.neighbors[i][0] >= self.columns_ or self.neighbors[i][1] <0 or self.neighbors[i][1] >= self.rows_):
					self.neighbor_flags[i] = INVALID;
					self.neighbor_timers[i] = 0;
				else:
					self.neighbor_flags[i] = ACTIVE; #set to ACTIVE here, we may want to set all the valid neighbors to inactive at first.
					self.neighbor_timers[i] = time.time() + 3*self.beat_period_; #once a neighbor is active, if no heartbeat is heard for over 3 heartbeat periods, the neighbor is treated as inactive.
		else: #set all neighbors to invalid
			for i in range(NUM_NEIGHBORS):
				self.neighbors[i][0] = -1;
				self.neighbors[i][1] = -1;
				self.neighbor_flags[i] = INVALID;
				self.neighbor_timers[i] = 0;
		return