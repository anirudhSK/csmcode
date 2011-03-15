import time
from header import *
from log_parking import *
from recurring_timer import *
from collections import deque
from operator import itemgetter, attrgetter

# TODO move self.queue to deque

CODE_VNS = "VNS"         #code for VN layer messages

#consistency manager statemachine states
UNKNOWN = -1                      #the initial state, dead node
NEWNODE = 0                       #a new node in a region
SYNC =    1                       #non-leader node just get in a region
INIT =    2                       #leader node just get in a region
SERVER =  3                       #a leader node initialized
BACKUP =  4                       #a non-leader node who got its states synchronized with the leader already
MSGSYNC = 5                       #sync caused by message sequence inconsistency
OLD_INACTIVE = 6

NUM_NEIGHBORS = 8 #to be replaced by a configurable parameter

SENDING =   1             #sending buffer enabled to send
NOSENDING = 0             #sending buffer disabled

#define
FROM_HEAD = 0             #insertion into total ordering queue from head
FROM_TAIL = 1             #insertion into total ordering queue from tail


#Synchronization timeout
#void VNSTimer::expire(Event *e)
#	a_.syn_timeout(0);

#Total Ordered input queue timeout
#void OrderedQTimer::expire(Event *e)
#	a_.orderedQueue_timeout(0);

#sending buffer time out.
#void ServerSendingTimer::expire(Event *e)
#	a_.server_timeout(0);



class VNSAgent(object):
	def __init__(self, id, total_ordering_enabled_, total_ordering_mode_, sync_enabled_, rows_, columns_):
		# TODO binds
#		self.port_number_ =
#		self.client_port_ =
#		self.join_port_ =
		self.columns_ = columns_
		self.rows_ = rows_
#		self.maxX_ =
#		self.maxY_ =
		self.nodeID_ = id
#		self.leader_ =
#		self.leader_status_ =
#		self.state_ =
#		self.syn_wait_ =
#		self.syn_interval_ =
#		self.max_retries_ =
		self.sync_enabled_ = sync_enabled_
#		self.sync_delay_ =
		self.total_ordering_enabled_ = total_ordering_enabled_
		self.total_ordering_mode_ = total_ordering_mode_
#		self.first_node_hold_off_ =
#		self.ordering_delay_ =
#		self.send_wait_ =
#		self.packetSize_ =
#		self.neighbor_timer_ =
		# end binds
		
		
		self.app_code = UNKNOWN
		self.regionX_ = -1
		self.regionY_ = -1
		
		self.retries_ = 0;
		self.seq_ = 0;
		
		self.lastTime_ = -1;
		self.next_sync_ = 0;
		
		self.first_node_ = True;
# TODO what to set deadline?
		self.first_node_deadline_ = time.time() + 5

		self.input_queue = Queue.Queue() # hold Packets
		self.queue = Queue.Queue() # holds Packets
		
		# TODO log file shmid
		return
	
	def check_neighbor_regions(self, pkt):
		return # returns immediately in original vnserver.cc, too
		
	# The method processing every packet received by the agent
	# void VNSAgent::recv(Packet* pkt, Handler*)
	def recv(self, pkt):
		vnhdr = pkt.vnhdr

		timenow = time.time()

		self.check_neighbor_regions(pkt)

		if(self.first_node_ == True and vnhdr.send_time > self.first_node_deadline_):
			assert(1 == 0);	
			self.first_node_ = False;

		if(vnhdr.type == LEADER_MSG):
			if(vnhdr.subtype == NEWLEADER):
				assert(self.regionX_ == vnhdr.regionX and self.regionY_ == vnhdr.regionY); #same region message
				self.leader_status_ = LEADER;
				self.leader_ = self.nodeID_;
				if(LOG_ENABLED):
					log(CODE_VNS, RECV_LEADER, vnhdr.toString());
					if(self.first_node_ == True):
						log_info(CODE_VNS, FIRSTNODE, self.first_node_);
					#log(CODE_VNS, RECV_LEADER, hdr.toString());

				if(self.first_node_ == True):
					self.first_node_deadline_ = timenow + 5;
				if(self.state_ == NEWNODE):
					self.regionX_ = vnhdr.regionX;
					self.regionY_ = vnhdr.regionY;
					self.state_=SERVER;
					self.server_init();
				if(self.state_ == BACKUP):
					self.state_=SERVER;
					self.server_init();
				 	self.queue = Queue.Queue(); #dump everything in the queue
				else:
					pass #Do nothing. 	
			elif(vnhdr.subtype == NONLEADER):
				assert(self.regionX_ == vnhdr.regionX and self.regionY_ == vnhdr.regionY); #same region message
				self.leader_status_ = NON_LEADER;
				self.leader_ = vnhdr.dst;

				if(self.first_node_ == True):
					self.first_node_deadline_ = timenow + 5;
				if(self.state_ == NEWNODE):
					if(self.sync_enabled_ == 1 or self.sync_enabled_ == 2):
						log_info(CODE_VNS,MOV-SYNC,"");
						self.server_init();
						self.state_=SYNC;
						self.sync();
					else:
						pass # I'll do nothing.
						# state_=BACKUP;
				else:
					pass #Do nothing
			elif(vnhdr.subtype == OLDLEADER):
				self.state_= OLD_INACTIVE
				self.leader_status_ = OLD_LEADER
		elif(vnhdr.type == REGION_MSG):
			if(vnhdr.subtype == NEWREGION):
				self.regionX_ = vnhdr.regionX;
				self.regionY_ = vnhdr.regionY;
				if(LOG_ENABLED):
					log(CODE_VNS, RECV_NEWREGION, vnhdr.toString());
				#reset all states and wait for the leader information
				self.setNeighbors();
				self.reset_states();
				self.first_node_deadline_ = timenow + 5;
		elif(vnhdr.type == SYNC_MSG):
			if(vnhdr.subtype == SYN_REQUEST):
				if(vnhdr.regionX == self.regionX_ and vnhdr.regionY == self.regionY_): #the packet come from the same region
					if(LOG_ENABLED):
						log(CODE_VNS, RECV, vnhdr.toString());
					if(self.state_ == SERVER):
						if( timenow > self.next_sync_):
							self.next_sync_ = timenow + self.sync_delay_;
							self.server_broadcast(SYN_ACK, vnhdr.dst, self.getStateSize(), self.getState());
							log_info(CODE_VNS, SENDSTATE, self.getStateSize());
						else:
							log_info(CODE_VNS,SYNC-HOLDOFF, self.next_sync_);
					elif(self.state_ == SYNC or self.state_ == MSGSYNC):
						#postpone my own sync message a little
						self.waiting_ = self.syn_wait_+2*self.syn_wait_*((rand()%1000)/1000.0);
						self.syn_timer_.resched( self.waiting_ );#wait a short period of time and send another syn_request if no response
			elif(vnhdr.subtype == SYN_ACK):
				if(vnhdr.regionX == self.regionX_ and vnhdr.regionY == self.regionY_): #the packet come from the same region
					if(self.state_ == SYNC or self.state_ == MSGSYNC): #use any SYNC to synchronize my state
						if(LOG_ENABLED):
							log(CODE_VNS,RECV,vnhdr.toString());
						self.saveState(pkt);
						self.state_ = BACKUP;
						self.send_loopback(ST_SYNCED, self.seq_);
						if(self.first_node_ == True):
							self.first_node_ = False;
		elif(vnhdr.type == APPL_MSG): #application messages, application layer messages
			if(self.state_ == SERVER or (self.state_ == BACKUP and self.sync_enabled_ == 1)):#sync mode 2, no msg sync
				#reject application layer messages coming from non-neighboring regions
				#this is not necessary is higher layer allows the processing of these messages
				if(abs(vnhdr.regionX - self.regionX_)>1 or abs(vnhdr.regionY - self.regionY_)>1):
					#log(CODE_VNS,LONGRECV,vnhdr.toString());
#					Packet::free(pkt);
					return;
				if(self.first_node_hold_off_ == True and self.first_node_ == True):
					#log(CODE_VNS,FIRSTNODE,vnhdr.toString());
#					Packet::free(pkt);
					return;
				#put packet in total ordered input queue
				if(self.total_ordering_enabled_ == True):
					#log_info(CODE_VNS,TOTAL,"ordering");
					self.sortPacket(pkt);
					return;
				else:
					assert ( 1 == 0);	
					self.consistencyManager(pkt);#no sorting.
					return;
#			Packet::free(pkt);
		#log_info(CODE_VNS, PFREE, "packet freed");
		return
	
	# new version with common vn header
	# insert application messages into the total ordered input queue
	# sort the packets with send_time
	# void VNSAgent::sortPacket(Packet * pkt)
	def sort_packet(self, pkt):
		count = 1;
		# Access the vns message header for the received packet:
	  	hdr = pkt.vnhdr

	  	size = len(self.input_queue)

	  	deadline = time.time()+self.ordering_delay_;

#TODO	  	PacketUnit * insertedUnit;
		insertedUnit = None

	  	if(size == 0):#first packet
			insertedUnit = self.input_queue.put(pkt);
			insertedUnit.deadline = deadline;

			#if(LOG_ENABLED):
			#	log(CODE_VNS,BUFFERED_START,hdr.toString());
			#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
			self.queue_timer_.resched(self.ordering_delay_);#wait for a ordering_delay_ period to reduce packet misorder
			self.next_to_expire=hdr.send_time;
		else:#queue not empty
#TODO			struct PacketUnit * current_unit;
#			hdr_vncommon * current_hdr;
			current_hdr = None

			if(self.total_ordering_mode_ == FROM_HEAD):
				self.current_unit = input_queue.head;

				while(1):
					current_hdr = current_unit.current.vnhdr
					if(current_hdr.send_time > hdr.send_time): #the packet comes earlier than the current packet

						insertedUnit = input_queue.insertBefore(current_unit, pkt);
						insertedUnit.deadline = deadline;

						#if(LOG_ENABLED):
						#	log(CODE_VNS,BUFFERED_BEFORE,hdr.toString());
						#	log_info(CODE_VNS,BUFFER_POS,(double)count);
						#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
						break;
					else:#the packet comes no ealier than the current one
						if(current_unit.next !=0):#not the last one in the queue
							current_unit = current_unit.next;
							count += 1;
						else:
							insertedUnit = self.input_queue.put(pkt);
							insertedUnit.deadline = deadline;
							#if(LOG_ENABLED):
							#	log(CODE_VNS,BUFFERED_END,hdr.toString());
							#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
							break;
			else:
				current_unit = self.input_queue.tail;

				while(1):
					current_hdr = current_unit.current.vnhdr;
					if(current_hdr.send_time <= hdr.send_time): #the packet comes no earlier than the current packet
						insertedUnit = input_queue.insertBehind(current_unit, pkt);
						insertedUnit.deadline = deadline;
						#if(LOG_ENABLED):
						#	log(CODE_VNS,BUFFERED_BEHIND,hdr.toString());
						#	log_info(CODE_VNS,BUFFER_POS,(double)count);
						#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
						break;
					else:#the packet comes no ealier than the current one
						if(current_unit.previous !=0):#not the first one in the queue
							current_unit = current_unit.previous;
							count += 1;
						else:#the packet comes earlier than the head
							insertedUnit = input_queue.insertBefore(current_unit, pkt);
							insertedUnit.deadline = deadline;
							#if(LOG_ENABLED):
							#	log(CODE_VNS,BUFFERED_HEAD,hdr.toString());
							#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
							break;
		return
	
	# new version with vn common header
	# receive a packet from the ordered input packet queue
	# check against sending queue if the packet is not a local client message
	# do synchronization for local server messages
	# void VNSAgent::consistencyManager(Packet * pkt)
	def consistencyManager(self, pkt):
		# Access the vns message header for the received packet:
	  	hdr = pkt.vnhdr
	  	handled = False; #not dealt with by vns

		#if a server message is received, check it against the sending queue to see
		#if a match can be found. If so, remove the corresponding messages from the queue.
		#Otherwise, start a synchronization procedure.
		if(hdr.subtype == SERVER_MESSAGE or hdr.subtype == PARKING_REQUEST or hdr.subtype == PARKING_REPLY or hdr.subtype == WRITE_UPDATE or hdr.subtype == WRITE_UPDATE_REPLY): 
		  if(hdr.regionX == self.regionX_ and hdr.regionY == self.regionY_): #message comes from the same region, need to be handled by vns
			  handled = True; #dealt with by vns.
			  if(self.state_ == BACKUP and self.sync_enabled_ == 1):

#					#if(hdr.hash == getStateHash()):#a matching state.
#						log_info(CODE_VNS,MATCHING,"");
#						#log_info(CODE_VNS,SERVER-HASH,hdr.hash);
#						#log_info(CODE_VNS,MY-HASH,getStateHash());
#
#					#if(LOG_ENABLED):
#					#	log(CODE_VNS,RECV_SERVER_MESSAGE, hdr.toString());

					if(self.lookup(self.queue, pkt)==0):#not found

						log_info(CODE_VNS,MSG-SYNC,"");
#						#if(hdr.hash != getStateHash()):#check to see if the hash value of the state matches or not.
#							log_info(CODE_VNS,HASH-MISMATCH,"");
#
#							log_info(CODE_VNS,SERVER-HASH,hdr.hash);
#							log_info(CODE_VNS,MY-HASH,getStateHash());

						self.state_ = MSGSYNC;
						self.sync();
			  elif(self.state_ == BACKUP and self.sync_enabled_ == 2):
					#if(LOG_ENABLED):
					#	log(CODE_VNS,RECV_SERVER_MESSAGE, hdr.toString());
					if(self.lookup(self.queue, pkt)==0):#not found
						#do nothing, in mode 2
						pass
						#log_info(CODE_VNS,MSG-SYNC,"");
						#state_ = MSGSYNC;
						#sync();
		#if a forwarded server message is received, check it against the sending queue to see
		#if a match can be found. If so, remove the corresponding messages from the queue.
		#Otherwise, do nothing. (no state synchronization needed)
		elif(hdr.subtype == FORWARDED_SERVER_MESSAGE):
		  if(hdr.regionX == self.regionX_ and hdr.regionY == self.regionY_): #message comes from the same region, need to be handled by vns
			  handled = True; #dealt with by vns.
			  if(self.state_ == BACKUP and (self.sync_enabled_ == 1 or self.sync_enabled_==2)):
					#if(LOG_ENABLED):
					#	log(CODE_VNS,RECV_FORWARDED_SERVER_MESSAGE, hdr.toString());
					if(self.lookup(self.queue, pkt)==0):#not found
						pass
						#if(LOG_ENABLED):
						#	log_info(CODE_VNS,SYNC,"no match found, sync");
		#if a forwarded client message is received, check it against the sending queue to see
		#if a match can be found. If so, remove the corresponding messages from the queue.
		#Otherwise, do nothing. (no state synchronization needed)
		elif(hdr.subtype == FORWARDED_CLIENT_MESSAGE):
		  if(hdr.regionX == self.regionX_ and hdr.regionY == self.regionY_): #message comes from the same region, need to be handled by vns
			  handled = True; #dealt with by vns.
			  if(self.state_ == BACKUP and hdr.regionX == self.regionX_ and hdr.regionY == self.regionY_ and (self.sync_enabled_ == 1 or self.sync_enabled_==2)):
					#if(LOG_ENABLED):
					#	log(CODE_VNS,RECV_FORWARDED_CLIENT_MESSAGE, hdr.toString());
					if(self.lookup(self.queue, self.pkt)==0 ):#not found
						pass
						#if(LOG_ENABLED):
						#	log_info(CODE_VNS,SYNC,"no match found, sync");

		#if a client message is received, do nothing. (no state synchronization needed)
		elif(hdr.subtype == CLIENT_MESSAGE or hdr.subtype == PARKING_REQUEST or hdr.subtype == PARKING_REPLY or hdr.subtype == PARKING_ACK or hdr.subtype == WRITE_UPDATE or hdr.subtype == WRITE_UPDATE_REPLY):
			pass
			#client messages
			#passed on by vns

		else:
			handled = True; #dealt with by vns.
			if(LOG_ENABLED):
				log(CODE_VNS, WRONG_MSG_CLASS, hdr.toString());
		#log_info(CODE_VNS, RECV, "unknown message");

		if(handled == False): #dealt with by vns.
			if(self.state_ == SERVER):
		  	  #handle packets and send responses out
		  	  self.handle_packet(pkt);
			  self.send_packets();
			elif(self.state_ == BACKUP and self.sync_enabled_ == 1):
		  	  #handle packets and keep responses in the queue
		  	  self.handle_packet(pkt);
			elif(self.state_ == BACKUP and self.sync_enabled_ == 2):#still handle packets, but no sync
		  	  #handle packets and keep responses in the queue
		  	  self.handle_packet(pkt);
			else:#backup node don't even handle packets
				pass
				#if(LOG_ENABLED):
		  		#log(CODE_VNS, IGNORE, hdr.toString());

#		    Packet::free(pkt);
	    #log_info(CODE_VNS, PFREE, "packet freed");
		return
	
	
	# Awaken by the syn timer.
	# Check the current syn status to see if anything needs to be done.
	# void VNSAgent::check_syn_status()
	def check_syn_status(self):
		#log_info(CODE_VNS,SYNC, "checking sync status");
		if(self.state_==SYNC or self.state_ == MSGSYNC):
			#if(retries_ < max_retries_):
			if(True):
				#syncClientID_ = nodeID_; #I am the client now.
				#log_info(CODE_VNS,SEND_SYNC,retries_);
				self.seq_ += 1
#				self.server_broadcast(SYN_REQUEST, seq_, 0, (u_char *) 0);
				self.server_broadcast(SYN_REQUEST, seq_, 0, 0);
				#server_broadcast(SYN_ACK, seq, getStateSize(), (u_char *) getState());

				self.retries_ += 1;
				#log_info(CODE_VNS,SEND,"SYN_REQUEST sent");
				self.syn_timer_.resched(self.syn_interval_*self.retries_);
				#log_info(CODE_VNS,SYNC_RESCHEDULE,syn_interval_);
			#else:#give up on sync attempts, possibly due to congestion, therefore, it is ok to keep quiet until next packet received
			#	state_ = BACKUP;
			#	retries_ = 0;
		return
	
	# Awaken by the ordered input queue timer.
	# Check the current status of the input queue to see if anything needs to be done
	# void VNSAgent::check_queue_status()
	def check_queue_status(self):
		p = None # Packet

		while(len(self.input_queue) > 0):
			p = self.input_queue.get();
			if(p):
				#if(LOG_ENABLED):
				#	hdr_vns * hdr = hdr_vns::access(p);
				#	log(CODE_VNS,DEBUFFERED,hdr.toString());
				#	log_info(CODE_VNS,BUFFER_SIZE,(double)input_queue.size);
				self.consistencyManager(p);
				if(len(self.input_queue) != 0):#if there is more packets, check to see if the next one has expired or not.
					#log_info(CODE_VNS,NEXT_DEADLINE,input_queue.head.deadline);
					if(time.time() < self.input_queue.head.deadline):
					 	wait = self.input_queue.head.deadline - time.time()
						self.queue_timer_.resched(wait);
						break;
		return
	
	# Awaken by the server timer.
	# Check the current status to see if anything needs to be done
	# void VNSAgent::check_server_status()
	def check_server_status(self):
		p = None # Packet * p;

		if(self.state_ == SERVER):
			if(len(self.queue) != 0):
				self.sending_state_ = SENDING;
				p = self.queue.get();
				if(p):
					hdr = p.vnhdr
					if(hdr.send_type == SEND):
						if(LOG_ENABLED):
							log(app_code,SEND,self.getAppHeader(p))
#					elif(hdr.send_type == SENDLOOPBACK)
#						if(LOG_ENABLED):
#							log(app_code,SENDLOOPBACK,self.getAppHeader(p))
					elif(hdr.send_type == FORWARD):
						if(LOG_ENABLED):
							log(app_code,FORWARD,self.getAppHeader(p))
					else:
						pass
					assert(self.leader_status_ == LEADER);
					send(p,0);
				self.server_timer_.resched(self.send_wait_);
			else:
				#log_info(app_code,QUEUE,"Queue emptied");
				self.sending_state_ = NOSENDING;
		return
	
	#reset states when a node moves into a new region
	def reset_states(self):
		self.sending_state_ = NOSENDING;
		self.state_ = NEWNODE;
		self.leader_ = UNKNOWN;
		self.leader_status_ = UNKNOWN;
		self.first_node_ = False; #new node to the region
		self.queue = Queue.Queue(); #dump everything
		
	
	#send the messages in the message buffer, if there is any
	def send_packets(self, ):
		#log_info(CODE_VNS, QUEUE, (double)sending_state_);
		if(self.sending_state_ == SENDING):
			pass
			#just wait for the next timeout
			#log_info(CODE_VNS, QUEUE, "sending ongoing, wait");
		else:
			if(len(self.queue) != 0):
				self.sending_state_ = SENDING;
				#log_info(CODE_VNS, QUEUE, "sending enabled for the queue");
				self.server_timer_.resched(send_wait_);
			else:
				#log_info(CODE_VNS, QUEUE, "empty");
				pass


	#synchronizing the states with the server
	def sync(self, ):
		#srand(Scheduler::instance().clock()+nodeID_);
		self.retries_ = 0;
		#queue.empty(); #dump everything in the queue
		waiting = self.syn_wait_+2*self.syn_wait_*((rand()%1000)/1000.0);
		self.syn_timer_.resched( waiting );#wait a short period of time and send another syn_request if no response

		#log_info(CODE_VNS,SYNC_TO_GO, waiting_);


	#lookup a packet in the packet buffer for a match, using the equal method.
	#once a match is found, remove the match from the buffer.
	def lookup(self, queue, pkt):
		if not queue or len(queue) == 0:
			return 0
		for i in range(len(queue)):
			item = queue[i]
			if item != pkt:
				print '%s != %s' % (item, pkt)
			else:
				print '%s == %s, removing...' % (item, pkt)
				del queue[i]
				return 1
		return 0


	#check a packet queue, remove a packet if it is received a certain period of time ago
	def queue_timeout(self, queue, time):
#TODO port function. not necessary maybe?
		return

	#check the packet sending queue, remove a packet if it is older than the maximum response time
	#check the head first, if the head is old, continue, else: quit
	def check_head(self, time):
#TODO port function. not necessary maybe?
		return


	#forward a VNS message to other local servers
	#this is for the VNS messages, don't use it for the application
	#msgType: int message type
	#seq: the sequence number
	#addr: address allocated or offered
	#trans: transmission type, unicast or flooding, corresponding to server message or client message
	#hop_count: the maximum number of hops the message can travel
	#sr_len: the length of the source route
	#sourceRoute: the source route
	#leaseTime: deadline for reponse
	#forward: forwarding type, INTER_REGION or LOCAL message
	#buff_len: the length of an adjustable buffer for the state synchronization messages
	#state_buffer: the buffer for the states
	# server_broadcast(SYN_ACK, vnhdr.dst, clientid, getStateSize(), (u_char *) getState());
	def server_broadcast(self, msgType, seq, buff_len, state_buffer):
		pkt = Packet # Packet* pkt = allocpkt();

#TODO ip_hdr	
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = IP_BROADCAST; 	#broadcasting address, should be -1
#		iph.dport() = MY_PORT_;		#send only to other server ports
#		#iph.sport() = MY_PORT_; #no need
#		iph.ttl() = 1;
	
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_VNS;
#		cmn_hdr.size() = size_ + IP_HDR_LEN+buff_len; # add in IP header
#		cmn_hdr.next_hop_ = IP_BROADCAST;
		
		pkt.vnhdr.type = SYNC_MSG;
		pkt.vnhdr.subtype = msgType;
		pkt.vnhdr.regionX = self.regionX_;
		pkt.vnhdr.regionY = self.regionY_;
		pkt.vnhdr.send_time = time.time()
		pkt.vnhdr.src = self.nodeID_;#client id of transaction
		pkt.vnhdr.dst = seq;#sequence number of the transaction
		pkt.vnhdr.send_type = SEND;#sending, forwarding or loopback

#TODO copy state_buffer into packet
#		pkt.allocdata(buff_len);
#		u_char * data = pkt.accessdata();
#		memcpy(data, state_buffer, buff_len);

		#Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, 0);#this send relies on the routing function, may not work for flooding mode
		if(LOG_ENABLED):
			log(CODE_VNS, SEND, vnhdr.toString());
		return

	#Send a loopback VNCOMMON message to the JOIN agent attached to the same node
	#msgType: int message type
	def send_loopback(self, msgType, seq):
		pkt = Packet() # Packet* pkt = allocpkt();
#TODO ip_hdr
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent::addr();
#		iph.daddr() = Agent::addr(); 	#broadcasting address, should be -1
#		iph.dport() = join_port_;		#send only to other server ports
#		#iph.sport() = MY_PORT_; #no need
#		iph.ttl() = 1;
		
#		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#		cmn_hdr.ptype() = PT_VNS;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = nodeID_;
		
		pkt.vnhdr.type = SYNC_MSG;
		pkt.vnhdr.subtype = msgType;
		pkt.vnhdr.regionX = self.regionX_;
		pkt.vnhdr.regionY = self.regionY_;
		pkt.vnhdr.send_time = time.time()
		pkt.vnhdr.src = seq;#client id of transaction
		pkt.vnhdr.dst = self.leader_;#sequence number of the transaction
		pkt.vnhdr.send_type = SEND;#sending, forwarding or loopback

		#log_info(CODE_VNS, LOOPBACKSENTTO, (double) vnhdr.subtype);
		#if(LOG_ENABLED):
		#	log(CODE_VNS, SEND, vnhdr.toString());
		send(pkt, 0);#this send relies on the routing function, may not work for flooding mode
		return


	#get a neighbor region's index in the array
	def toNeigbhorIndex(self, rx, ry):
		for i in range(NUM_NEIGHBORS):
			if(self.neighbors[i][0]==rx and neighbors[i][1] == ry):
				return i;
		return -1;


	# Upon region change, calculate the neighbor list and initialize the status of each neighbor
	def setNeighbors(self, ):
		#valid region id
		if(self.regionX_ >=0 and self.regionX_ < self.columns_ and self.regionY_ >= 0 and self.regionY_ < self.rows_):
			for i in range(NUM_NEIGHBORS):
				switch(i)
				if(i==0):
					self.neighbors[i][0] = self.regionX_ -1;
					self.neighbors[i][1] = self.regionY_+1;
				elif(i==1):
					self.neighbors[i][0] = self.regionX_;
					self.neighbors[i][1] = self.regionY_+1;
				elif(i==2):
					self.neighbors[i][0] = self.regionX_ +1;
					self.neighbors[i][1] = self.regionY_+1;
				elif(i==3):
					self.neighbors[i][0] = self.regionX_ +1;
					self.neighbors[i][1] = self.regionY_;
				elif(i==4):
					self.neighbors[i][0] = self.regionX_ +1;
					self.neighbors[i][1] = self.regionY_-1;
				elif(i==5):
					self.neighbors[i][0] = self.regionX_;
					self.neighbors[i][1] = self.regionY_-1;
				elif(i==6):
					self.neighbors[i][0] = self.regionX_ -1;
					self.neighbors[i][1] = self.regionY_-1;
				elif(i==7):
					self.neighbors[i][0] = self.regionX_ -1;
					self.neighbors[i][1] = self.regionY_;
				else:
					self.neighbors[i][0] = -1;
					self.neighbors[i][1] = -1;
				
				#set invalid neighbor flags to -1
				if(self.neighbors[i][0] <0 or self.neighbors[i][0] >= self.columns_ or self.neighbors[i][1] <0 or self.neighbors[i][1] >= self.rows_):
					self.neighbor_flags[i] = INVALID;
				else:
					self.neighbor_flags[i] = ACTIVE; #set to ACTIVE here, we may want to set all the valid neighbors to inactive at first.
				#	neighbor_timeouts[i] = Scheduler::instance().clock()+neighbor_timer_;

		else: #set all neighbors to invalid
			for i in range(NUM_NEIGHBORS):
				self.neighbors[i][0] = -1;
				self.neighbors[i][1] = -1;
				self.neighbor_flags[i] = INVALID;

	
	# get the distance between two regions */
	def getDistance(self, rx1, ry1, rx2, ry2):
	    return (rx1-rx2)*(rx1-rx2)+(ry1-ry2)*(ry1-ry2)

	
	#convert a region id into an integer
	def encodeRID(self, base, x, y):
		return base+x*self.rows_+y

	
	#convert an integer back into region id
#WARNING: Changed interface
	def decodeRID(self, base, RID):
		return (int)(RID-base)/self.rows_, (RID-base)%self.rows_;

	
	# Using the neighbor list, find the active neighbor that is the closest to the destination
	# Returns the index in neighbors[] where the next hop region id can be found.
	def getNextHop(self, destRegionX, destRegionY):
		minDistanceIndex = -1;
		minDistance = self.columns_*self.columns_+self.rows_*self.rows_;
		tempDistance = 0;

		if(self.regionX_ == destRegionX and self.regionY_ == destRegionY):
			if(LOG_ENABLED):
				log_info(CODE_VNS,WARNING,"Destination Already Reached");
			return minDistanceIndex;

		minDistance = self.columns_*self.columns_+self.rows_*self.rows_;
		for i in range(NUM_NEIGHBORS):
			#log_info(CODE_VNS,"neighbor",i);
			if(self.neighbor_flags[i] == ACTIVE):
				tempDistance = self.getDistance(self.neighbors[i][0], self.neighbors[i][1], destRegionX, destRegionY);
				#log_info(CODE_VNS,"active",tempDistance);
				if(tempDistance < minDistance):
					minDistance = tempDistance;
					minDistanceIndex = i;

		return minDistanceIndex;


	def getState(self, ):
		return 0


	def getStateSize(self, ):
		return 0


	def stateToString(self, ):
		return ""


	def saveState(self, pkt):
		#memcpy(my_vcs, pkt.accessdata(), my_vcs.size());
		return


	#get application packet header into a string
	def getAppHeader(self, pkt):
		return ""
		#char * nullString = (char *)malloc(sizeof(""));
		#return nullString;#the memory used need to be released by the user of this method.


	#use DJBHash
	def getStateHash(self, ):
	   	return 0
