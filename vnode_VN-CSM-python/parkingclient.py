import vnclient
import time
import Queue

### DEFINE ###
CODE_PARKING = "PARKING"
UP = 1

class ClientRequest:
	def __init__(self):
		self.origid = 0
		self.m_count = 0
		self.destX = 0
		self.destY = 0
		self.isWrite = 0
		self.expiration_time = 0
		self.m_retries = 0

class ParkingClientAgent(vnclient.VNCAgent):
	def __init__(self):
		self.client_state_ = UNKNOWN;
		self.app_code = CODE_PARKING;
		self.m_count = 0;
		#TODO Timers
	
	def check_clock_status(self, ):
		if(self.client_state_ == UP):
			pass
		return
	
	def check_parking_status(self):
		if(self.client_state_ == UP):
			isWrite = 0;
			if(self.nodeID_ % 2 == 0):
				isWrite = 1;
				
			if(self.m_count < 100):
				dest_x = -1;
				dest_y = -1;
				
				prob = random.randint(0,100)
				
				if(prob < 25):
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 3;
						dest_y = self.regionY_ + 3;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 3;
						dest_y = self.regionY_ - 3;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ + 3;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 3;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 3;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ - 3;
				elif(prob < 50):
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 2;
						dest_y = self.regionY_ + 2;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 2;
						dest_y = self.regionY_ - 2;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ + 2;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 2;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 2;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ - 2;
				elif(prob < 75):
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 1;
						dest_y = self.regionY_ + 1;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 1;
						dest_y = self.regionY_ - 1;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ + 1;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ + 1;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_ - 1;
						dest_y = self.regionY_;
					if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
						dest_x = self.regionX_;
						dest_y = self.regionY_ - 1;
				else:
					dest_x = self.regionX_;
					dest_y = self.regionY_;
				if( not (dest_x >=0 and dest_x < 4 and dest_y >= 0 and dest_y < 4)):
					dest_x = self.regionX_;
					dest_y = self.regionY_;
					
				n_nodes = LogParkingFile.getNumNodes(self.regionX_, self.regionY_);
				r_status = LogParkingFile.isLeaderActive(self.regionX_, self.regionY_);
				
				if(n_nodes == 0):
					assert(r_status == False);
					print "REGION EMPTY \n";
				elif(r_status == False):
					print "NO CURRENT LEADER at time - %f \n" % (time.time());
					
				client_req = ClientRequest()
				client_req.origid = self.nodeID_;
				client_req.m_count = self.m_count;
				client_req.isWrite = isWrite;
				client_req.destX = dest_x;
				client_req.destY = dest_y;
				client_req.m_retries = 0;
				client_req.expiration_time = time.time() + RESEND_BACKOFF;

				self.m_resending_queue.push(client_req);
				if(self.m_resending_queue.size() > 0):
					wait = self.m_resending_queue.top().expiration_time - time.time();
					if(wait <= 0):
						wait = 0.00001;
					resending_timer_.resched(wait);

				print "Client Node-%d (%d, %d):: sending request for region (%d, %d) with m_count = %d and num servers = %d, and leadership status = %d at time - %f and isWrite = %d\n" % (self.nodeID_, self.regionX_, self.regionY_, dest_x, dest_y, m_count, n_nodes, r_status, time.time(), isWrite);

				if(dest_x == self.regionX_ and dest_y == self.regionY_):			
					self.msg_hops[0]+= 1;
					self.hop_count[m_count] = 0;
				elif(abs(dest_x-self.regionX_) == 3 or abs(dest_y - self.regionY_) == 3):	
					self.msg_hops[3]+= 1;
					self.hop_count[m_count] = 3;	
				elif(abs(dest_x-self.regionX_) == 2 or abs(dest_y - self.regionY_) == 2):	
					self.msg_hops[2]+= 1;
					self.hop_count[m_count] = 2;	
				elif(abs(dest_x-self.regionX_) == 1 or abs(dest_y - self.regionY_) == 1):	
					self.msg_hops[1]+= 1;
					self.hop_count[m_count] = 1;
				else:
					assert(1==0);

				self.sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, DMSG, self.nodeID_, isWrite, self.m_count, dest_x, dest_y);
				self.send_packets();

				self.m_send_time[self.m_count] = time.time();
				self.m_count+= 1;
			elif(self.m_count >= 1):
				for hop in range(4):
					avg_res_time = 0.0;
					for i in range(self.m_response_time[hop].size()):
						avg_res_time += self.m_response_time[hop][i];
					if(self.m_response_time[hop].size() > 0):
						print "Node-%d:: Average response time for %d hops = %f\n" % (self.nodeID_, hop, avg_res_time/self.m_response_time[hop].size()); 
				if(self.m_res_time.size() > 0):	
					print "Node-%d:: Msgs send = %d; Msgs acked = %d;\n" % (self.nodeID_, self.m_count, self.m_res_time.size()); 
					print "Node-%d:: zero_hop = %d, one_hop = %d, two_hop = %d, three_hop = %d\n" % (self.nodeID_, self.msg_hops[0], self.msg_hops[1], self.msg_hops[2], self.msg_hops[3]); 	
			self.parking_timer_.resched(100); # every X cycles a node requests a parking spot. Think of a better way to get this done. 
		return
		
	
	def check_resending_status(self, ):
		if(self.m_resending_queue.empty()):
			return;
			
		if(self.m_resending_queue.top().expiration_time <= time.time()):
			print "Client with NodeID_ - %d is got RESEND_EXP for request = %d, %d and time - %f\n" % (self.nodeID_, self.m_resending_queue.top().origid, self.m_resending_queue.top().m_count, time.time());
			top_req = self.m_resending_queue.top() # ClientRequest 

			dest_x = top_req.destX;
			dest_y = top_req.destY;

			if(dest_x == self.regionX_ and dest_y == self.regionY_):	
				self.msg_hops[self.hop_count[m_count]]-= 1;
				self.msg_hops[0]+= 1;
				self.hop_count[m_count] = 0;
			elif(abs(dest_x-self.regionX_) == 3 or abs(dest_y - self.regionY_) == 3):	
				self.msg_hops[self.hop_count[m_count]]-= 1;
				self.msg_hops[3]+= 1;
				self.hop_count[m_count] = 3;	
			elif(abs(dest_x-self.regionX_) == 2 or abs(dest_y - self.regionY_) == 2):	
				self.msg_hops[self.hop_count[m_count]]-= 1;
				self.msg_hops[2]+= 1;
				self.hop_count[m_count] = 2;	
			elif(abs(dest_x-self.regionX_) == 1 or abs(dest_y - self.regionY_) == 1):	
				self.msg_hops[self.hop_count[m_count]]-= 1;
				self.msg_hops[1]+= 1;
				self.hop_count[m_count] = 1;
			else:
				assert(1==0);

			self.m_send_time[top_req.m_count] = time.time();

			sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, PARKING_REQUEST_RESEND, top_req.origid, top_req.isWrite, top_req.m_count, top_req.destX, top_req.destY);
			#		sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, PARKING_REQUEST_RESEND, top_req.origid, top_req.isWrite, top_req.m_count, self.regionX_, self.regionY_);
			send_packets();

			if(top_req.m_retries == MAX_RETRIES):
				print "CLIENT Trying for the last time. hoping it is this time LUCKY :P\n";
				self.m_resending_queue.popleft();
				return;

			client_req = ClientRequest()
			client_req.origid = top_req.origid;
			client_req.m_count = top_req.m_count;
			client_req.isWrite = top_req.isWrite;
			client_req.destX = top_req.destX;
			client_req.destY = top_req.destY;
			client_req.m_retries = top_req.m_retries + 1;
			client_req.expiration_time = time.time() + RESEND_BACKOFF;

			self.m_resending_queue.popleft();
			self.m_resending_queue.append(client_req);
			if(self.m_resending_queue.size() > 0):
				wait = self.m_resending_queue.top().expiration_time - time.time();
				if(wait <= 0):
					wait = 0.00001;
				self.resending_timer_.resched(wait);
				
	def removeClientRequest(self, origid, m_count):
#TODO		priority_queue<ClientRequest> temp_queue;
			temp_queue = Queue.PriorityQueue()
			
			return_value = False;
			
			while( not self.m_resending_queue.empty()):
					top_req = self.m_resending_queue.top(); # ClientRequest 
					self.m_resending_queue.popleft();
					if(top_req.origid == origid and top_req.m_count == m_count):
							return_value = True;
					else:
							temp_queue.append(top_req);
			while( not temp_queue.empty()):
					top_req = temp_queue.top(); # ClientRequest 
					temp_queue.popleft();
					self.m_resending_queue.append(top_req);
			return return_value;
	
		
	def sendp(self, send_type, msg_class, dest, dest_port, msgType, origid, isWrite, m_count, dest_regionX, dest_regionY):
			pkt = Packet()
			
			
#TODO			#common header
#			hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#			cmn_hdr.ptype() = PT_VNPARKING;
#			cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#			cmn_hdr.next_hop_ = dest;

#TODO			#ip header
#			hdr_ip* iph = HDR_IP(pkt);
#			iph.saddr() = Agent.addr();
#			iph.daddr() = dest; #broadcasting address, should be -1
#			iph.dport() = dest_port; #destination port has to be the server port
#			iph.sport() = MY_PORT_; #no need
#			iph.ttl() = 1;

			#vncommon header
			#hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);
			pkt.vnhdr.type = APPL_MSG;
			pkt.vnhdr.subtype = msg_class;
			pkt.vnhdr.regionX = self.regionX_;
			pkt.vnhdr.regionY = self.regionY_;
			pkt.vnhdr.send_time = time.time();
#TODO			pkt.vnhdr.src = Agent.addr();
			pkt.vnhdr.src = Agent.addr()
			pkt.vnhdr.dst = dest;
			pkt.vnhdr.send_type = send_type;#sending, forwarding or loopback

			#parking header
			#hdr_vnparking* hdr = hdr_vnparking::access(pkt);
			pkt.vnparking_hdr.type = msgType; #client message types
			pkt.vnparking_hdr.self.m_send_time = time.time();

			pkt.vnparking_hdr.srcX = self.regionX_; #original region
			pkt.vnparking_hdr.srcY = self.regionY_; #original region

			pkt.vnparking_hdr.destX = dest_regionX; #destination region
			pkt.vnparking_hdr.destY = dest_regionY; #destination region
		
			pkt.vnparking_hdr.nextX = -1;
			pkt.vnparking_hdr.nextY = -1;

			pkt.vnparking_hdr.origid = origid;
			pkt.vnparking_hdr.isWrite = isWrite;
			pkt.vnparking_hdr.m_count = m_count;

			queue.enqueue(pkt); 
			#send another one for the server located on the same node
			send_loopback(msgType, self.nodeID_, m_count, isWrite, dest_regionX, dest_regionY);
	#		send_packets(); # NIKET - changed this

	def send_loopback(self, msgType, origid, m_count, isWrite, dest_regionX, dest_regionY):
			pkt = Packet()
			
			# vn header
			#hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);
			pkt.vnhdr.type = APPL_MSG;
			pkt.vnhdr.subtype = CLIENT_MESSAGE;
			pkt.vnhdr.regionX = self.regionX_;
			pkt.vnhdr.regionY = self.regionY_;
			pkt.vnhdr.send_time = time.time();
			pkt.vnhdr.src = Agent.addr();
			pkt.vnhdr.dst = Agent.addr();#sequence number of the transaction
			pkt.vnhdr.send_type = SENDLOOPBACK;#sending, forwarding or loopback
			
			# common header
#			hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
#			cmn_hdr.ptype() = PT_VNPARKING;
#			cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#			cmn_hdr.next_hop_ = IP_BROADCAST;
			
			# hdr 
#			hdr_ip* iph = HDR_IP(pkt);
#			iph.saddr() = Agent.addr();
#			iph.daddr() = Agent.addr(); #broadcasting address, should be -1
#			iph.dport() = server_port_; #destination port has to be the server port
#			#iph.sport() = MY_PORT_; #no need

			# parking header
			#hdr_vnparking* hdr = hdr_vnparking::access(pkt);
			pkt.vnparking_hdr.type = msgType; #client message types
			pkt.vnparking_hdr.self.m_send_time = time.time();

			pkt.vnparking_hdr.srcX = self.regionX_;
			pkt.vnparking_hdr.srcY = self.regionY_;

			pkt.vnparking_hdr.destX = dest_regionX;
			pkt.vnparking_hdr.destY = dest_regionY;

			pkt.vnparking_hdr.origid = origid;
			pkt.vnparking_hdr.isWrite = isWrite;
			pkt.vnparking_hdr.m_count = m_count;

			queue.enqueue(pkt);

	def handle_packet(self, pkt):
		#hdr_vncommon * vnhdr = hdr_vncommon::access(pkt); #get the vnlayer header
		#hdr_vnparking * hdr = hdr_vnparking::access(pkt); #get the header
		vnhdr = pkt.vnhdr
		hdr = pkt.vnparking_hdr
		
		if(hdr.srcX == self.regionX_ and hdr.srcY == self.regionY_):
			if(vnhdr.type == APPL_MSG and hdr.origid == self.nodeID_):
				for r in range(m_response_count.size()):
					if(self.m_response_count[r] == hdr.m_count):
						print "Node = %d; DUPLICATE_ACK for count = %d\n" % (self.nodeID_, hdr.m_count);
						return;
				print "Client with NodeID_ - %d is UP and got a server packet and sender is - %d message type is = %d\n" % (self.nodeID_, vnhdr.src, vnhdr.subtype);				
				print "Node = %d; Success = %d for m_count - %d, self.hop_count = %d, and delay = %f\n" % (self.nodeID_, hdr.isSuccess, hdr.m_count, self.hop_count[hdr.m_count], time.time() - self.m_send_time[hdr.m_count]);
				print "--------------------------------\n";
				m_response_time[self.hop_count[hdr.m_count]].append(time.time() - self.m_send_time[hdr.m_count]);
				m_res_time.append(time.time() - self.m_send_time[hdr.m_count]);
				m_response_count.append(hdr.m_count);
				removeClientRequest(self.nodeID_, hdr.m_count);
				return;
