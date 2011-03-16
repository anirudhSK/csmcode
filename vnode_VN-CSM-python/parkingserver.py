from vnserver import *
import time
import copy
from packet_queue import *

# TODO move m_resending_queue and temp_queue to deque


CODE_PARKINGS = "PARKING" #code for application layer messages

class ClientRequest(object):
	def __init__(self):
		self.origid = 0
		self.m_count = 0
		self.destX = 0
		self.destY = 0
		self.isWrite = 0
		self.expiration_time = 0
		self.m_retries = 0

class RemoteRequest(object):
	def __init__(self):
		self.origid = 0
		self.m_count = 0
		self.isSuccess = 0

class SharedState(object):
	def __init__(self):
		self.num_free_parking_spaces = 0
		self.version = 0
		self.origid = 0
		self.m_count = 0
		self.destX = 0
		self.destY = 0
		self.isWrite = 0
		self.expiration_time = 0
		self.m_retries = 0
		self.isSuccess = 0
		self.isClientRequest = 0


#int LogParkingFile.self.m_version[MAX_ROWS][MAX_COLS];	
#int LogParkingFile.m_num_nodes[MAX_ROWS][MAX_COLS];	
#int LogParkingFile.global_free_spaces[MAX_ROWS][MAX_COLS];
#int LogParkingFile.region_leader[MAX_ROWS][MAX_COLS];
#bool LogParkingFile.is_region_alive[MAX_ROWS][MAX_COLS];
#bool LogParkingFile.has_active_leader[MAX_ROWS][MAX_COLS];
#bool LogParkingFile.log_init = False;
#int LogParkingFile.global_accesses;
#double LogParkingFile.election_time[MAX_ROWS][MAX_COLS];
#int LogParkingFile.central_g_seq[MAX_ROWS][MAX_COLS];
#map<pair<int, int>, int> LogParkingFile.central_l_seq[MAX_ROWS][MAX_COLS];
#map<int, vector<pair<int, int> > > LogParkingFile.central_m_seq_acks[MAX_ROWS][MAX_COLS];
#map<pair<int, int>, priority_queue<WriteUpdate> > LogParkingFile.central_m_write_updates[MAX_ROWS][MAX_COLS];


class ParkingServerAgent(VNSAgent):
	
	def check_parking_status(self):
		print 'ParkingServerAgent.check_parking_status'
		return
	
	def __init__(self, id, total_ordering_enabled_, total_ordering_mode_, sync_enabled_, rows_, columns_):
		print 'ParkingServerAgent.__init__'
		VNSAgent.__init__(self, id, total_ordering_enabled_, total_ordering_mode_, sync_enabled_, rows_, columns_)

# TODO start timers
		self.resending_timer_ = RecurringTimer(3, self.check_resending_status)
		self.parking_timer_ = RecurringTimer(3, self.check_parking_status)
		
		self.app_code = CODE_PARKINGS;
		self.m_parkingstate = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
		self.m_parkingstate = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
		self.m_remote_requests = [] # vector<RemoteRequest>
		self.m_resending_queue = CustomQueue()
		
		# CSM data
		self.g_seq = 0
		self.l_seq = {} # map<pair<int, int>, int> l_seq;
		self.m_seq_acks = {} # map<int, vector<pair<int, int> > > m_seq_acks;
		self.m_write_updates = {} # map<pair<int, int>, priority_queue<WriteUpdate> > m_write_updates;


	#initialize the server states when a server comes into a new region
	def server_init(self, ):
		print 'ParkingServerAgent.server_init'
		for row in range(MAX_ROWS):
			for col in range(MAX_COLS):
				self.m_parkingstate[row][col] = LogParkingFile.getFreeSpots(row, col);	
		
		self.m_version = LogParkingFile.getVersion(self.regionX_, self.regionY_);
		self.send_loopback(ST_VER, self.m_version);	

		del self.m_remote_requests[0:len(self.m_remote_requests)] # self.m_remote_requests.clear();
		while( not self.m_resending_queue.empty()):
			self.m_resending_queue.pop();

		# Setting the CSM sequences
		if(CSM == 1):
			self.l_seq.clear();	
			self.m_seq_acks.clear();
			self.m_write_updates.clear();
			self.l_seq = copy.copy(LogParkingFile.central_l_seq[self.regionX_][self.regionY_]);
			self.g_seq = copy.copy(LogParkingFile.central_g_seq[self.regionX_][self.regionY_]);
			self.m_seq_acks = copy.copy(LogParkingFile.central_m_seq_acks[self.regionX_][self.regionY_]);
			self.m_write_updates = copy.copy(LogParkingFile.central_m_write_updates[self.regionX_][self.regionY_]);


	def handle_packet(self, pkt):
		print 'ParkingServerAgent.handle_packet'
		cmnhdr = pkt.vnhdr; #get the vnlayer common header
		hdr = pkt.vnparking_hdr #get the header


		#*********************** CLIENT MESSAGE ***************************************#
		if(cmnhdr.subtype == CLIENT_MESSAGE):
			#*********************** LOCAL MESSAGE ***************************************#
			if(hdr.srcX == self.regionX_ and hdr.srcY == self.regionY_ and hdr.destX == self.regionX_ and hdr.destY == self.regionY_):
				isDuplicate = self.isDuplicateRequest(hdr.origid, hdr.m_count)
				isSuccess = 0
				doBroadcast = False

				if(isDuplicate != -1): #implies it is a duplicate request
					assert(hdr.type == PARKING_REQUEST_RESEND); # TODO - uncomment this. why assert failing?
					isSuccess = self.m_remote_requests[isDuplicate].isSuccess; 
				else:
					if(self.leader_status_ == LEADER):
						print "sender was - %d; SUCCESSFULL\n" % (hdr.origid);
						print "sender was - %d; REQUEST-DONE!\n" % (hdr.origid);
						if(hdr.isWrite != 0):	
							self.m_version+= 1;
							self.send_loopback(ST_VER, self.m_version);	
							doBroadcast = True;
							assert(self.m_parkingstate[self.regionX_][self.regionY_] == LogParkingFile.getFreeSpots(self.regionX_, self.regionY_));
							self.m_parkingstate[self.regionX_][self.regionY_] += hdr.isWrite;

							if(hdr.isWrite == 1):
								LogParkingFile.incrementFreeSpots(self.regionX_, self.regionY_);
							else:
								LogParkingFile.decrementFreeSpots(self.regionX_, self.regionY_);

							assert(self.m_version == LogParkingFile.getVersion(self.regionX_, self.regionY_));
							assert(self.m_parkingstate[self.regionX_][self.regionY_] == LogParkingFile.getFreeSpots(self.regionX_, self.regionY_));
						else:
							pass
						#	assert(m_parkingstate[regionX_][regionY_] == LogParkingFile.getFreeSpots(regionX_, regionY_));
						#	assert(m_parkingstate[regionX_][regionY_] == LogParkingFile.getFreeSpots(regionX_, regionY_));
						#	m_parkingstate[regionX_][regionY_]-= 1;
						#	LogParkingFile.decrementFreeSpots(regionX_, regionY_);
						#	assert(self.m_version == LogParkingFile.getVersion(regionX_, regionY_));
						#	assert(m_parkingstate[regionX_][regionY_] == LogParkingFile.getFreeSpots(regionX_, regionY_));
					else:
						if(hdr.isWrite != 0):	
							self.m_version+= 1;
							self.send_loopback(ST_VER, self.m_version);	
							doBroadcast = True;
							self.m_parkingstate[self.regionX_][self.regionY_] += hdr.isWrite;
						else:
							pass
						#	m_parkingstate[regionX_][regionY_]-= 1;
					isSuccess = 1;
					remote_req = RemoteRequest();
					remote_req.origid = hdr.origid;
					remote_req.m_count = hdr.m_count;
					remote_req.isSuccess = isSuccess;
					self.m_remote_requests.append(remote_req);
				if(self.leader_status_ == LEADER):
					print "sender was - %d; SENTTOCLIENT!\n" % (hdr.origid);
				self.sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
				self.sendp(SENDLOOPBACK, SERVER_MESSAGE, self.nodeID_, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
			
				if(doBroadcast and (CSM == 1)):
					self.sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, self.m_parkingstate[self.regionX_][self.regionY_], hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1, -1, self.g_seq);
			
					region = (self.regionX_, self.regionY_);
		
					LogParkingFile.central_m_seq_acks[self.regionX_][self.regionY_][self.g_seq].append(region);
					LogParkingFile.central_g_seq[self.regionX_][self.regionY_]+= 1;
		
					self.m_seq_acks[self.g_seq].append(region);
					self.g_seq+= 1;

					if(self.leader_status_ == LEADER):
						print "NodeID_ - %d (%d, %d) sending a write update for sequence - %d\n" % (self.nodeID_, self.regionX_, self.regionY_, self.g_seq-1);		
			#*********************** LOCAL MESSAGE ***************************************#
			#*********************** REMOTE MESSAGE ***************************************#
			elif(hdr.srcX == self.regionX_ and hdr.srcY == self.regionY_):
				dest_hops = abs(hdr.destX - self.regionX_);
				if(dest_hops < abs(hdr.destY - self.regionY_)):
					dest_hops = abs(hdr.destY - self.regionY_);
				if((CSM == 1) and (hdr.isWrite == 0) and (dest_hops <= MAX_HOP_SHARING)):
					if(self.leader_status_ == LEADER):
						print "sender was - %d; SUCCESSFULL\n" % (hdr.origid);
						print "sender was - %d; REQUEST-DONE!\n" % (hdr.origid);
						print "sender was - %d; SENTTOCLIENT!\n" % (hdr.origid);
						if(self.m_parkingstate[hdr.destX][hdr.destY] != LogParkingFile.getFreeSpots(hdr.destX, hdr.destY)):
							print "sender was - %d; STALE-READ and hops = %d with max = %d\n" % (hdr.origid, dest_hops, MAX_HOP_SHARING);
							print "local spots - %d; actual = %d\n" % (self.m_parkingstate[hdr.destX][hdr.destY], LogParkingFile.getFreeSpots(hdr.destX, hdr.destY));
					isSuccess = 1;
					self.sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
					self.sendp(SENDLOOPBACK, SERVER_MESSAGE, self.nodeID_, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
				else:
					next_hop_index = self.getNextHop(hdr.destX, hdr.destY);
					if(next_hop_index == -1):
						return;
						
					if(self.leader_status_ == LEADER):
						print "sender was - %d; REQUEST-DONE!\n" % (hdr.origid);
						
					nextX = self.neighbors[next_hop_index][0];
					nextY = self.neighbors[next_hop_index][1];
					isSuccess = -1;
					
					client_req = ClientRequest();
					client_req.origid = hdr.origid;
					client_req.m_count = hdr.m_count;
					client_req.isWrite = hdr.isWrite;
					client_req.destX = hdr.destX;
					client_req.destY = hdr.destY;
					client_req.m_retries = 0;
					client_req.expiration_time = time.time() + 0.25;			
					self.m_resending_queue.push(client_req);
					
					if(self.m_resending_queue.size() > 0):
						wait = self.m_resending_queue[0].expiration_time - time.time()
						if(wait <= 0):
							wait = 0.00001;
						self.resending_timer_.resched(wait);
						
					reg = (hdr.destX, hdr.destY);
					low_seq = -1;
					if(CSM == 1):
						low_seq = l_seq[reg]-1;
						if(self.leader_status_ == LEADER):
							print "NodeID_ - %d (%d, %d) sending a remote request for region (%d, %d) and low sequence = %d\n" % (nodeID_, regionX_, regionY_, hdr.destX, hdr.destY, low_seq);		
					self.sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, PARKING_REQUEST, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, hdr.srcX, hdr.srcY, nextX, nextY, hdr.destX, hdr.destY, low_seq);		
			#*********************** REMOTE MESSAGE ***************************************#
		#*********************** CLIENT MESSAGE ***************************************#
		#*********************** PARKING REQUEST ***************************************#
		elif(cmnhdr.subtype == PARKING_REQUEST):
			if(hdr.destX == self.regionX_ and hdr.destY == self.regionY_ and hdr.nextX == self.regionX_ and hdr.nextY == self.regionY_):
				if(self.leader_status_ == LEADER):
					print "Destation server with NodeID_ - %d (%d, %d) is UP and got fwded REQUEST packet and sender is - %d, m_count = %d. Destination is (%d, %d) and time - %f\n" % (nodeID_, regionX_, regionY_, hdr.origid, hdr.m_count, hdr.destX, hdr.destY, Scheduler.instance().clock());		
				isSuccess = 0
				doBroadcast = False
				isDuplicate = self.isDuplicateRequest(hdr.origid, hdr.m_count)
				if(isDuplicate != -1):
					print "Duplicate request!!\n";
					#	assert(hdr.type == PARKING_REQUEST_RESEND);
					isSuccess = self.m_remote_requests[isDuplicate].isSuccess;
					if(self.leader_status_ == LEADER):
						print "sender was - %d; LEADER sending duplicate reply with success = %d\n" % (hdr.origid, isSuccess);
				else:	
					if(self.leader_status_ == LEADER):
						if(hdr.isWrite != 0):
							self.m_version+= 1;
							self.send_loopback(ST_VER, self.m_version);	
							doBroadcast = True;	
							assert(self.m_parkingstate[self.regionX_][self.regionY_] == LogParkingFile.getFreeSpots(self.regionX_, self.regionY_));
							self.m_parkingstate[self.regionX_][self.regionY_] += hdr.isWrite;
							if(hdr.isWrite == 1):
								LogParkingFile.incrementFreeSpots(self.regionX_, self.regionY_);
							else:
								LogParkingFile.decrementFreeSpots(self.regionX_, self.regionY_);
							assert(self.m_parkingstate[self.regionX_][self.regionY_] == LogParkingFile.getFreeSpots(self.regionX_, self.regionY_));
						else:
							pass
						#	assert(m_parkingstate[regionX_][regionY_] == LogParkingFile.getFreeSpots(regionX_, regionY_));
						#	m_parkingstate[regionX_][regionY_]-= 1;
						#	LogParkingFile.decrementFreeSpots(regionX_, regionY_);
						#	assert(m_parkingstate[regionX_][regionY_] == LogParkingFile.getFreeSpots(regionX_, regionY_));
					else:
						if(hdr.isWrite != 0):	
							self.m_version+= 1;
							self.send_loopback(ST_VER, self.m_version);	
							doBroadcast = True;
							self.m_parkingstate[self.regionX_][self.regionY_] += hdr.isWrite;
						else:
							pass
						#	m_parkingstate[regionX_][regionY_]-= 1;
					isSuccess = 1;
					remote_req = RemoteRequest()
					remote_req.origid = hdr.origid;
					remote_req.m_count = hdr.m_count;
					remote_req.isSuccess = isSuccess;
					self.m_remote_requests.append(remote_req);

				if(self.leader_status_ == LEADER and isDuplicate == -1):
					print "sender was - %d; SUCCESSFULL\n" % (hdr.origid);
				next_hop_index = self.getNextHop(hdr.srcX, hdr.srcY);
				if(next_hop_index == -1):
					return;
				nextX = self.neighbors[next_hop_index][0];
				nextY = self.neighbors[next_hop_index][1];

				self.sendp(SEND, PARKING_REPLY, IP_BROADCAST, MY_PORT_, PARKING_REPLY, hdr.origid, hdr.m_count, isSuccess, hdr.isWrite, hdr.destX, hdr.destY, nextX, nextY, hdr.srcX, hdr.srcY, -1);

				if(doBroadcast and (CSM == 1)):
					self.sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, m_parkingstate[self.regionX_][self.regionY_], hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1, -1, self.g_seq);
					region = (self.regionX_, self.regionY_);

					LogParkingFile.central_m_seq_acks[regionX_][regionY_][g_seq].append(region);
					LogParkingFile.central_g_seq[regionX_][regionY_]+= 1;

					self.m_seq_acks[self.g_seq].append(region);
					self.g_seq+= 1;

					if(self.leader_status_ == LEADER):
						print "NodeID_ - %d (%d, %d) sending a write update for sequence - %d\n" % ( self.nodeID_, self.regionX_, self.regionY_, self.g_seq-1);		
			elif(hdr.nextX == self.regionX_ and hdr.nextY == self.regionY_):
				next_hop_index = self.getNextHop(hdr.destX, hdr.destY);
				if(next_hop_index == -1):
					return;
				nextX = self.neighbors[next_hop_index][0];
				nextY = self.neighbors[next_hop_index][1];
				self.sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, hdr.type, hdr.origid, hdr.m_count, hdr.isSuccess, hdr.isWrite, hdr.srcX, hdr.srcY, nextX, nextY, hdr.destX, hdr.destY, hdr.csm_seq);
		#*********************** PARKING REQUEST ***************************************#
		#*********************** PARKING REPLY ***************************************#
		elif(cmnhdr.subtype == PARKING_REPLY):
			if(hdr.destX == regionX_ and hdr.destY == self.regionY_ and hdr.nextX == self.regionX_ and hdr.nextY == self.regionY_):
				if( not removeClientRequest(hdr.origid, hdr.m_count)):
					return;			
				if(self.leader_status_ == LEADER):
					print "sender was - %d; SENTTOCLIENT!\n" % (hdr.origid);
				self.sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, hdr.isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
				self.sendp(SENDLOOPBACK, SERVER_MESSAGE, self.nodeID_, self.client_port_, PARKING_ACK, hdr.origid, hdr.m_count, hdr.isSuccess, hdr.isWrite, self.regionX_, self.regionY_, self.regionX_, self.regionY_, self.regionX_, self.regionY_, -1);
			elif(hdr.nextX == self.regionX_ and hdr.nextY == self.regionY_):
				next_hop_index = getNextHop(hdr.destX, hdr.destY);
				if(next_hop_index == -1):
					return;
				nextX = self.neighbors[next_hop_index][0];
				nextY = self.neighbors[next_hop_index][1];
				self.sendp(SEND, PARKING_REPLY, IP_BROADCAST, MY_PORT_, hdr.type, hdr.origid, hdr.m_count, hdr.isSuccess, hdr.isWrite, hdr.srcX, hdr.srcY, nextX, nextY, hdr.destX, hdr.destY, -1);
		#*********************** PARKING REPLY ***************************************#
		#*********************** WRITE UPDATE ***************************************#
		elif(cmnhdr.subtype == WRITE_UPDATE):
			assert(CSM == 1);
			hops = abs(hdr.srcX - self.regionX_);
			if(hops < abs(hdr.srcY - self.regionY_)):
				hops = abs(hdr.srcY - self.regionY_);
			if((abs(hdr.nextX-self.regionX_) <= 1) and (abs(hdr.nextY-self.regionY_) <= 1) and (hops <= MAX_HOP_SHARING)):
				if(leader_status_ == LEADER):
					print "NodeID_ - %d (%d, %d) Got a write update for sequence - %d from region (%d, %d)\n" % (nodeID_, regionX_, regionY_, hdr.csm_seq, hdr.srcX, hdr.srcY);		

				region = (hdr.srcX, hdr.srcY);
				myRegion = (self.regionX_, self.regionY_);
				if((hdr.csm_seq >= self.l_seq[region]) and (region != myRegion)):	
					update_req = WriteUpdate;
					update_req.reg_x = hdr.srcX;
					update_req.reg_y = hdr.srcY;
					update_req.parking_spots = hdr.isSuccess;
					update_req.seq_no = hdr.csm_seq;
					m_write_updates[region].append(update_req);
					
					LogParkingFile.central_m_write_updates[self.regionX_][self.regionY_][region].append(update_req);
					
				#	while(l_seq[region] == m_write_updates[region][0].seq_no)
					while(1):
						up_req = self.m_write_updates[region][0];
						
						if(self.leader_status_ == LEADER):
							print "NodeID_ - %d (%d, %d) UPDATING parking spots = %d for region (%d, %d)\n" % (self.nodeID_, self.regionX_, self.regionY_, up_req.parking_spots, region.first, region.second);		
							
						self.m_parkingstate[up_req.reg_x][up_req.reg_y] = up_req.parking_spots;
						self.m_write_updates[region].get();
						
						LogParkingFile.central_m_write_updates[self.regionX_][self.regionY_][region].get();
						LogParkingFile.central_l_seq[self.regionX_][self.regionY_][region]+= 1;
						
						self.l_seq[region]+= 1;
						if(self.m_write_updates[region].size() == 0):
							break;
			
					next_hop_index = getNextHop(hdr.srcX, hdr.srcY);
					if(next_hop_index == -1):
						assert(1 == 0);
					nextX = self.neighbors[next_hop_index][0];
					nextY = self.neighbors[next_hop_index][1];

					self.sendp(SEND, WRITE_UPDATE_REPLY, IP_BROADCAST, MY_PORT_, WRITE_UPDATE_REPLY, -1, -1, -1, -1, self.regionX_, self.regionY_, nextX, nextY, hdr.srcX, hdr.srcY, hdr.csm_seq);

					# Rebroadcast the write updates. 
					if((abs(hdr.nextX-regionX_) == 1) and (abs(hdr.nextY-regionY_) == 1) and (hops < MAX_HOP_SHARING)): #Maximal coverage!
						sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, hdr.isSuccess, hdr.isWrite, hdr.srcX, hdr.srcY, self.regionX_, self.regionY_, -1, -1, hdr.csm_seq);
				else:
					pass
					#Already worked on this request!
		#*********************** WRITE UPDATE ***************************************#
		#*********************** WRITE UPDATE REPLY ***************************************#
		elif(cmnhdr.subtype == WRITE_UPDATE_REPLY):
			assert(CSM == 1);	
			if(hdr.nextX ==  self.regionX_ and hdr.nextY == self.regionY_ and hdr.destX == self.regionX_ and hdr.destY == self.regionY_):
				if(leader_status_ == LEADER):
					print "NodeID_ - %d (%d, %d) got a write update reply for seq = %d from region (%d, %d)\n" % (nodeID_, self.regionX_, self.regionY_, hdr.csm_seq, hdr.srcX, hdr.srcY);		
				region = (hdr.srcX, hdr.srcY);
				self.m_seq_acks[hdr.csm_seq].append(region);
				LogParkingFile.central_m_seq_acks[self.regionX_][self.regionY_][hdr.csm_seq].append(region);
			
				if(self.m_seq_acks[hdr.csm_seq].size() == 16):
					self.m_seq_acks.erase(hdr.csm_seq); #Got all acks!
					LogParkingFile.central_m_seq_acks[self.regionX_][self.regionY_].erase(hdr.csm_seq);
			elif(hdr.nextX == self.regionX_ and hdr.nextY == self.regionY_):
				next_hop_index = getNextHop(hdr.destX, hdr.destY);
				if(next_hop_index == -1):
					assert(1 == 0);
				nextX = self.neighbors[next_hop_index][0];
				nextY = self.neighbors[next_hop_index][1];

				self.sendp(SEND, WRITE_UPDATE_REPLY, IP_BROADCAST, MY_PORT_, WRITE_UPDATE_REPLY, -1, -1, -1, -1, hdr.srcX, hdr.srcY, nextX, nextY, hdr.destX, hdr.destY, hdr.csm_seq);
		#*********************** WRITE UPDATE REPLY ***************************************#

	def check_resending_status(self, ):
		print 'ParkingServerAgent.check_resending_status'
		if(self.m_resending_queue.empty()):
			return;

		if(self.m_resending_queue[0].expiration_time <= time.time()):
			top_req = self.m_resending_queue[0]; # ClientRequest
			if(self.leader_status_ == LEADER):
				print "RESENDING ORIG REQ = %d, %d\n" % (top_req.origid, top_req.m_count);
			next_hop_index = getNextHop(top_req.destX, top_req.destY);
			if(next_hop_index == -1):
				pass
			else:
				nextX = self.neighbors[next_hop_index][0];
				nextY = self.neighbors[next_hop_index][1];
				isSuccess = -1;
				self.sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, PARKING_REQUEST, top_req.origid, top_req.m_count, isSuccess, top_req.isWrite, self.regionX_, self.regionY_, nextX, nextY, top_req.destX, top_req.destY, -1);
				if(self.leader_status_ == LEADER):
					self.send_packets();

			if(top_req.m_retries == MAX_RETRIES):
				if(self.leader_status_ == LEADER):
					print "Trying for the last time. hoping it is this time lucky :P\n ";
				self.m_resending_queue.pop();
				return;
	
			client_req = ClientRequest();
			client_req.origid = top_req.origid;
			client_req.m_count = top_req.m_count;
			client_req.isWrite = top_req.isWrite;
			client_req.destX = top_req.destX;
			client_req.destY = top_req.destY;
			client_req.m_retries = top_req.m_retries + 1;
			client_req.expiration_time = time.time() + 0.25;			

			self.m_resending_queue.pop();
			self.m_resending_queue.push(client_req);
			if(self.m_resending_queue.size() > 0):
				wait = self.m_resending_queue[0].expiration_time - time.time();
				if(wait <= 0):
					wait = 0.00001;
				self.resending_timer_.resched(wait);	

	def sendp(self, send_type, msg_class, dest, dest_port, msgType, origid, m_count, isSuccess, isWrite, srcX, srcY, nextX, nextY, destX, destY, low_seq):
		print 'ParkingServerAgent.sendp'
		pkt = Packet()
		
		#common header
		# hdr_cmn * cmn_hdr = hdr_cmn.access(pkt);
#		cmn_hdr.ptype() = PT_VNPARKING;
#		cmn_hdr.size() = size_ + IP_HDR_LEN; # add in IP header
#		cmn_hdr.next_hop_ = dest;
		
		#ip header
#		hdr_ip* iph = HDR_IP(pkt);
#		iph.saddr() = Agent.addr();
#		iph.daddr() = dest; #broadcasting address, should be -1
#		iph.dport() = dest_port; #destination port has to be the server port
#		iph.sport() = MY_PORT_; #no need
#		iph.ttl() = 1;
		
		#vncommon header
		#hdr_vncommon * vnhdr = hdr_vncommon.access(pkt);
		pkt.vnhdr.type = APPL_MSG;
		pkt.vnhdr.subtype = msg_class;
		pkt.vnhdr.regionX = self.regionX_;
		pkt.vnhdr.regionY = self.regionY_;
		pkt.vnhdr.send_time = self.lastTime_;
		pkt.vnhdr.src = Agent.addr();
		pkt.vnhdr.dst = dest;
		pkt.vnhdr.send_type = send_type;#sending, forwarding or loopback
		
		#parking header
		#hdr_vnparking* vnparking_hdr = hdr_vnparking.access(pkt);
		pkt.vnparking_hdr.type = msgType; #client message types
		
		pkt.vnparking_hdr.srcX = srcX; #original region
		pkt.vnparking_hdr.srcY = srcY; #original region
		pkt.vnparking_hdr.nextX = nextX; #next region
		pkt.vnparking_hdr.nextY = nextY; #next region
		pkt.vnparking_hdr.destX = destX; #dest region
		pkt.vnparking_hdr.destY = destY; #dest region
		
		pkt.vnparking_hdr.isWrite = isWrite;
		pkt.vnparking_hdr.isSuccess = isSuccess;
		pkt.vnparking_hdr.origid = origid; 
		pkt.vnparking_hdr.m_count = m_count;
		pkt.vnparking_hdr.csm_seq = low_seq;

		pkt.vnparking_hdr.m_send_time = time.time()
	
		self.queue.push(pkt)

#TODO jason: getState - unsure about the porting of this function
	def getState(self, ):
		print 'ParkingServerAgent.getState'
		size = self.getStateSize();
		buff = [SharedState()] # array of shared states?
		
#TODO		units = size/sizeof(struct SharedState);
#		units = size/sizeof(struct SharedState);
		
		buff[0].version = self.m_version;
		buff[0].isClientRequest = -1;
		
		i = 1;
		for row in range(MAX_ROWS):
			for col in range(MAX_COLS):
				buff.append(SharedState())
				buff[i].num_free_parking_spaces = self.m_parkingstate[col][row];
				buff[i].isClientRequest = -1;
				i+= 1;
#TODO	assert(i == units - self.m_resending_queue.size() - len(self.m_remote_requests));
		if(self.m_resending_queue.size() != 0):
			temp_queue = CustomQueue() # priority_queue<ClientRequest> temp_queue;
			while( not self.m_resending_queue.empty()):
				top_req = self.m_resending_queue[0]; # ClientRequest 
				self.m_resending_queue.pop();
				temp_queue.push(top_req);

				buff[i].origid = top_req.origid;
				buff[i].m_count = top_req.m_count;
				buff[i].destX = top_req.destX;
				buff[i].destY = top_req.destY;
				buff[i].isWrite = top_req.isWrite;
				buff[i].m_retries = top_req.m_retries;
				buff[i].expiration_time = top_req.expiration_time;
				buff[i].isClientRequest = 1;
				buff[i].isSuccess = -1;
				i+= 1;
			while(not temp_queue.empty()):
				top_req = temp_queue[0]; # ClientRequest 
				temp_queue.pop();
				self.m_resending_queue.push(top_req);
#TODO		assert(i == units - len(self.m_remote_requests));
		if(len(self.m_remote_requests) != 0):
			assert(i == (1 + MAX_ROWS*MAX_COLS + self.m_resending_queue.size())); 
			for j in range(len(self.m_remote_requests)):
				buff[i].origid = self.m_remote_requests[j].origid;
				buff[i].m_count = self.m_remote_requests[j].m_count;
				buff[i].isSuccess = self.m_remote_requests[j].isSuccess;
				buff[i].isClientRequest = 0;
				buff[i].destX = -1;
				buff[i].destY = -1;
				buff[i].isWrite = -1;
				buff[i].m_retries = -1;
				buff[i].expiration_time = -1;
				i+= 1;
#TODO		assert(i == units);
		return buff;

	#return the size of the state
	def getStateSize(self, ):
		print 'ParkingServerAgent.getStateSize'
		count = 1; # 1 for the version number;

		count += MAX_COLS*MAX_ROWS;
		count += self.m_resending_queue.size();
		count += len(self.m_remote_requests);	

#TODO sizeof
		return count # remove this later
		return (count)*sizeof(SharedState);

#	def saveState(self, state, size):
	def saveState(self, pkt):
		print 'ParkingServerAgent.saveState'
		buff = [SharedState()] #struct SharedState * buff = (struct SharedState *) state;
		
#TODO get 'state' and 'size' from pkt
#TODO	units = size/sizeof(struct SharedState);

#		if(units < 1):
#			log_info(CODE_PARKINGS, "GOTWRONGSTATESIZE" % ((float)units);
#		self.log_info(CODE_PARKINGS, "GOTWRONGSTATESIZE %f" % (units))

 		self.m_version= buff[0].version;
		self.send_loopback(ST_VER, self.m_version);	
#TODO	assert(buff[0].isClientRequest == -1);
		i = 1	
		for row in range(MAX_ROWS):
			for col in range(MAX_COLS):
				self.m_parkingstate[col][row] = buff[i].num_free_parking_spaces ;
				assert(buff[i].isClientRequest == -1);
				i+= 1;
		assert (i == 1 + MAX_COLS*MAX_ROWS);
		while( not self.m_resending_queue.empty()):
			self.m_resending_queue.pop();
		m_remote_requests.clear();

		while (i < units):
			if(buff[i].isClientRequest == 1):
				client_req = ClientRequest()

				client_req.origid = buff[i].origid;
				client_req.origid = buff[i].m_count;
				client_req.destX = buff[i].destX;
				client_req.destY = buff[i].destY;
				client_req.isWrite = buff[i].isWrite;
				client_req.m_retries = buff[i].m_retries;
				client_req.expiration_time = buff[i].expiration_time;
				assert(buff[i].isSuccess == -1);
				self.m_resending_queue.push(client_req);
				i+= 1;
			elif(buff[i].isClientRequest == 0):
				remote_req = RemoteRequest()
				remote_req.origid = buff[i].origid;
				remote_req.m_count = buff[i].m_count;
				remote_req.isSuccess = buff[i].isSuccess;
				i+= 1; 

	def isDuplicateRequest(self, origid, m_count):
		print 'ParkingServerAgent.isDuplicateRequest'
		for i in range(len(self.m_remote_requests)):
			if(m_remote_requests[i].origid == origid and m_remote_requests[i].m_count == m_count):
				return i;
		return -1;	


	def removeClientRequest(self, origid, m_count):
		print 'ParkingServerAgent.removeClientRequest'
		temp_queue = CustomQueue() # priority_queue<ClientRequest> temp_queue;
		
		return_value = False;
		
		while( not self.m_resending_queue.empty()):
			#ClientRequest top_req = self.m_resending_queue[0];
			top_req = self.m_resending_queue.pop();
			if(top_req.origid == origid and top_req.m_count == m_count):
				return_value = True;
			else:
				temp_queue.push(top_req);
		while( not temp_queue.empty()):
			#ClientRequest top_req = temp_queue[0];
			top_req = temp_queue.pop();
			self.m_resending_queue.push(top_req);
		return return_value;

	#evaluate two server packets to see if they are for the same transaction     
	def equal(self, p1, p2):                                                                                                        
		print 'ParkingServerAgent.equal'
		if(p1.vnparking_hdr.type == p2.vnparking_hdr.type and
			p1.vnparking_hdr.srcX == p2.vnparking_hdr.srcX and
			p1.vnparking_hdr.srcY == p2.vnparking_hdr.srcY and
			p1.vnparking_hdr.nextX == p2.vnparking_hdr.nextX and
			p1.vnparking_hdr.nextY == p2.vnparking_hdr.nextY and
			p1.vnparking_hdr.destX == p2.vnparking_hdr.destX and
			p1.vnparking_hdr.destY == p2.vnparking_hdr.destY and
			p1.vnparking_hdr.isWrite == p2.vnparking_hdr.isWrite and
			p1.vnparking_hdr.isSuccess == p2.vnparking_hdr.isSuccess and
			p1.vnparking_hdr.origid == p2.vnparking_hdr.origid and
			p1.vnparking_hdr.m_count == p2.vnparking_hdr.m_count):
			return 1
		else:
			return 0
