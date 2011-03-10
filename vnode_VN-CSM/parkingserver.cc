#include "time.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#include "parkingserver.h"

#define GET_TIME Scheduler::instance().clock()

int LogParkingFile::m_version[MAX_ROWS][MAX_COLS];	
int LogParkingFile::m_num_nodes[MAX_ROWS][MAX_COLS];	
int LogParkingFile::global_free_spaces[MAX_ROWS][MAX_COLS];
int LogParkingFile::region_leader[MAX_ROWS][MAX_COLS];
bool LogParkingFile::is_region_alive[MAX_ROWS][MAX_COLS];
bool LogParkingFile::has_active_leader[MAX_ROWS][MAX_COLS];
bool LogParkingFile::log_init = false;
int LogParkingFile::global_accesses;
double LogParkingFile::election_time[MAX_ROWS][MAX_COLS];
int LogParkingFile::central_g_seq[MAX_ROWS][MAX_COLS];
map<pair<int, int>, int> LogParkingFile::central_l_seq[MAX_ROWS][MAX_COLS];
map<int, vector<pair<int, int> > > LogParkingFile::central_m_seq_acks[MAX_ROWS][MAX_COLS];
map<pair<int, int>, priority_queue<WriteUpdate> > LogParkingFile::central_m_write_updates[MAX_ROWS][MAX_COLS];


/*OTCL linkage for the VnsClass */
static class ParkingServerClass : public TclClass {
public:
	ParkingServerClass() : TclClass("Agent/VNSAgent/ParkingServer") {}
	TclObject* create(int, const char*const*) {
		LogParkingFile::init();
		return (new ParkingServerAgent());
	}
} class_parking;

/* Agent/VNS constructor   */
ParkingServerAgent::ParkingServerAgent() : VNSAgent(),parking_timer_(this), resending_timer_(this)
{
	app_code = CODE_PARKINGS;
}

//initialize the server states when a server comes into a new region
void ParkingServerAgent::server_init()
{	
	for(int row = 0; row < MAX_ROWS; row++)
	{
		for(int col = 0; col < MAX_COLS; col++)
		{
			m_parkingstate.num_free_parking_spaces[row][col] = LogParkingFile::getFreeSpots(row, col);	
		}
	}

	m_version = LogParkingFile::getVersion(regionX_, regionY_);
	send_loopback(ST_VER, m_version);	

	m_remote_requests.clear();
	while(!m_resending_queue.empty())
	{
		m_resending_queue.pop();
	}

	// Setting the CSM sequences
	if(CSM == 1)
	{
		l_seq.clear();	
		m_seq_acks.clear();
		m_write_updates.clear();
		l_seq = LogParkingFile::central_l_seq[regionX_][regionY_];
		g_seq = LogParkingFile::central_g_seq[regionX_][regionY_];
		m_seq_acks = LogParkingFile::central_m_seq_acks[regionX_][regionY_];
		m_write_updates = LogParkingFile::central_m_write_updates[regionX_][regionY_];
	}
}

void ParkingServerAgent::handle_packet(Packet * pkt)
{
	hdr_vncommon * cmnhdr = hdr_vncommon::access(pkt); //get the vnlayer common header
	hdr_vnparking * hdr = hdr_vnparking::access(pkt); //get the header


	/************************ CLIENT MESSAGE ****************************************/
	if(cmnhdr->subtype == CLIENT_MESSAGE)
	{
		/************************ LOCAL MESSAGE ****************************************/
		if(hdr->srcX == regionX_ && hdr->srcY == regionY_ && hdr->destX == regionX_ && hdr->destY == regionY_)
		{
			int isDuplicate = isDuplicateRequest(hdr->origid, hdr->m_count);
			int isSuccess;
			bool doBroadcast = false;

			if(isDuplicate != -1) //implies it is a duplicate request
			{
				assert(hdr->type == PARKING_REQUEST_RESEND); // TODO - uncomment this. why assert failing?
				isSuccess = m_remote_requests[isDuplicate].isSuccess; 
			}
			else
			{
				if(leader_status_ == LEADER)
				{
					printf("sender was - %d; SUCCESSFULL\n", hdr->origid);
					printf("sender was - %d; REQUEST-DONE!\n", hdr->origid);
					if(hdr->isWrite != 0)
					{	
						m_version++;
						send_loopback(ST_VER, m_version);	
						doBroadcast = true;
						assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
						m_parkingstate.num_free_parking_spaces[regionX_][regionY_] += hdr->isWrite;

						if(hdr->isWrite == 1)
							LogParkingFile::incrementFreeSpots(regionX_, regionY_);
						else
							LogParkingFile::decrementFreeSpots(regionX_, regionY_);

						assert(m_version == LogParkingFile::getVersion(regionX_, regionY_));
						assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					}
					else
					{
					//	assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					//	assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					//	m_parkingstate.num_free_parking_spaces[regionX_][regionY_]--;
					//	LogParkingFile::decrementFreeSpots(regionX_, regionY_);
					//	assert(m_version == LogParkingFile::getVersion(regionX_, regionY_));
					//	assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					}
				}
				else
				{
					if(hdr->isWrite != 0)
					{	
						m_version++;
						send_loopback(ST_VER, m_version);	
						doBroadcast = true;
						m_parkingstate.num_free_parking_spaces[regionX_][regionY_] += hdr->isWrite;
					}
					else
					{
					//	m_parkingstate.num_free_parking_spaces[regionX_][regionY_]--;
					}
				}
				isSuccess = 1;
				RemoteRequest remote_req;
				remote_req.origid = hdr->origid;
				remote_req.m_count = hdr->m_count;
				remote_req.isSuccess = isSuccess;
				m_remote_requests.push_back(remote_req);
			}
			if(leader_status_ == LEADER)
			{
				printf("sender was - %d; SENTTOCLIENT!\n", hdr->origid);
			}
			sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
			sendp(SENDLOOPBACK, SERVER_MESSAGE, nodeID_, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
			
			if(doBroadcast && (CSM == 1))
			{
				sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, m_parkingstate.num_free_parking_spaces[regionX_][regionY_], hdr->isWrite, regionX_, regionY_, regionX_, regionY_, -1, -1, g_seq);
			
				pair<int, int> region(regionX_, regionY_);
		
				LogParkingFile::central_m_seq_acks[regionX_][regionY_][g_seq].push_back(region);
				LogParkingFile::central_g_seq[regionX_][regionY_]++;
		
				m_seq_acks[g_seq].push_back(region);
				g_seq++;

				if(leader_status_ == LEADER)
					printf("NodeID_ - %d (%d, %d) sending a write update for sequence - %d\n", nodeID_, regionX_, regionY_, g_seq-1);		
			}
		}
		/************************ LOCAL MESSAGE ****************************************/
		/************************ REMOTE MESSAGE ****************************************/
		else if(hdr->srcX == regionX_ && hdr->srcY == regionY_)
		{
			int dest_hops = abs(hdr->destX - regionX_);
			if(dest_hops < abs(hdr->destY - regionY_))
			{
				dest_hops = abs(hdr->destY - regionY_);
			}
			if((CSM == 1) && (hdr->isWrite == 0) && (dest_hops <= MAX_HOP_SHARING))
			{
				if(leader_status_ == LEADER)
				{
					printf("sender was - %d; SUCCESSFULL\n", hdr->origid);
					printf("sender was - %d; REQUEST-DONE!\n", hdr->origid);
					printf("sender was - %d; SENTTOCLIENT!\n", hdr->origid);
					if(m_parkingstate.num_free_parking_spaces[hdr->destX][hdr->destY] != LogParkingFile::getFreeSpots(hdr->destX, hdr->destY))
					{
						printf("sender was - %d; STALE-READ and hops = %d with max = %d\n", hdr->origid, dest_hops, MAX_HOP_SHARING);
						printf("local spots - %d; actual = %d\n", m_parkingstate.num_free_parking_spaces[hdr->destX][hdr->destY], LogParkingFile::getFreeSpots(hdr->destX, hdr->destY));
					}
				}
				int isSuccess = 1;
				sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
				sendp(SENDLOOPBACK, SERVER_MESSAGE, nodeID_, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
			}
			else
			{
				int next_hop_index = getNextHop(hdr->destX, hdr->destY);
				if(next_hop_index == -1)
				{
					return;
				}

				if(leader_status_ == LEADER)
				{
					printf("sender was - %d; REQUEST-DONE!\n", hdr->origid);
				}

				int nextX = neighbors[next_hop_index][0];
				int nextY = neighbors[next_hop_index][1];
				int isSuccess = -1;

				ClientRequest client_req;
				client_req.origid = hdr->origid;
				client_req.m_count = hdr->m_count;
				client_req.isWrite = hdr->isWrite;
				client_req.destX = hdr->destX;
				client_req.destY = hdr->destY;
				client_req.m_retries = 0;
				client_req.expiration_time = Scheduler::instance().clock() + 0.25;			
				m_resending_queue.push(client_req);

				if(m_resending_queue.size() > 0)
				{
					double wait = m_resending_queue.top().expiration_time - Scheduler::instance().clock();
					if(wait <= 0)
						wait = 0.00001;
					resending_timer_.resched(wait);
				}

				pair<int, int> reg(hdr->destX, hdr->destY);
				int low_seq = -1;
				if(CSM == 1)
				{
					low_seq = l_seq[reg]-1;
					if(leader_status_ == LEADER)
						printf("NodeID_ - %d (%d, %d) sending a remote request for region (%d, %d) and low sequence = %d\n", nodeID_, regionX_, regionY_, hdr->destX, hdr->destY, low_seq);		
				}
				sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, PARKING_REQUEST, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, hdr->srcX, hdr->srcY, nextX, nextY, hdr->destX, hdr->destY, low_seq);
			}
		}		
		/************************ REMOTE MESSAGE ****************************************/
	}
	/************************ CLIENT MESSAGE ****************************************/
	/************************ PARKING REQUEST ****************************************/
	else if(cmnhdr->subtype == PARKING_REQUEST)
	{
		if(hdr->destX == regionX_ && hdr->destY == regionY_ && hdr->nextX == regionX_ && hdr->nextY == regionY_)
		{
			if(leader_status_ == LEADER)
				printf("Destation server with NodeID_ - %d (%d, %d) is UP and got fwded REQUEST packet and sender is - %d, m_count = %d. Destination is (%d, %d) and time - %f\n", nodeID_, regionX_, regionY_, hdr->origid, hdr->m_count, hdr->destX, hdr->destY, Scheduler::instance().clock());		
			int isSuccess;
			bool doBroadcast = false;
			int isDuplicate = isDuplicateRequest(hdr->origid, hdr->m_count);
			if(isDuplicate != -1)
			{
				printf("Duplicate request!!\n");
				//	assert(hdr->type == PARKING_REQUEST_RESEND);
				isSuccess = m_remote_requests[isDuplicate].isSuccess;
				if(leader_status_ == LEADER)
				{
					printf("sender was - %d; LEADER sending duplicate reply with success = %d\n", hdr->origid, isSuccess);
				}
			}
			else
			{	
				if(leader_status_ == LEADER)
				{
					if(hdr->isWrite != 0)
					{
						m_version++;
						send_loopback(ST_VER, m_version);	
						doBroadcast = true;	
						assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
						m_parkingstate.num_free_parking_spaces[regionX_][regionY_] += hdr->isWrite;
						if(hdr->isWrite == 1)
							LogParkingFile::incrementFreeSpots(regionX_, regionY_);
						else
							LogParkingFile::decrementFreeSpots(regionX_, regionY_);
						assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					}
					else
					{
					//	assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					//	m_parkingstate.num_free_parking_spaces[regionX_][regionY_]--;
					//	LogParkingFile::decrementFreeSpots(regionX_, regionY_);
					//	assert(m_parkingstate.num_free_parking_spaces[regionX_][regionY_] == LogParkingFile::getFreeSpots(regionX_, regionY_));
					}
				}
				else
				{
					if(hdr->isWrite != 0)
					{	
						m_version++;
						send_loopback(ST_VER, m_version);	
						doBroadcast = true;
						m_parkingstate.num_free_parking_spaces[regionX_][regionY_] += hdr->isWrite;
					}
					else
					{
					//	m_parkingstate.num_free_parking_spaces[regionX_][regionY_]--;
					}
				}
				isSuccess = 1;
				RemoteRequest remote_req;
				remote_req.origid = hdr->origid;
				remote_req.m_count = hdr->m_count;
				remote_req.isSuccess = isSuccess;
				m_remote_requests.push_back(remote_req);
			}

			if(leader_status_ == LEADER && isDuplicate == -1)
			{
				printf("sender was - %d; SUCCESSFULL\n", hdr->origid);
			}
			int next_hop_index = getNextHop(hdr->srcX, hdr->srcY);
			if(next_hop_index == -1)
			{
				return;
			}
			int nextX = neighbors[next_hop_index][0];
			int nextY = neighbors[next_hop_index][1];

			sendp(SEND, PARKING_REPLY, IP_BROADCAST, MY_PORT_, PARKING_REPLY, hdr->origid, hdr->m_count, isSuccess, hdr->isWrite, hdr->destX, hdr->destY, nextX, nextY, hdr->srcX, hdr->srcY, -1);

			if(doBroadcast && (CSM == 1))
			{
				sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, m_parkingstate.num_free_parking_spaces[regionX_][regionY_], hdr->isWrite, regionX_, regionY_, regionX_, regionY_, -1, -1, g_seq);
				pair<int, int> region(regionX_, regionY_);

				LogParkingFile::central_m_seq_acks[regionX_][regionY_][g_seq].push_back(region);
				LogParkingFile::central_g_seq[regionX_][regionY_]++;

				m_seq_acks[g_seq].push_back(region);
				g_seq++;

				if(leader_status_ == LEADER)
					printf("NodeID_ - %d (%d, %d) sending a write update for sequence - %d\n", nodeID_, regionX_, regionY_, g_seq-1);		
			}
		}
		else if(hdr->nextX == regionX_ && hdr->nextY == regionY_)
		{
			int next_hop_index = getNextHop(hdr->destX, hdr->destY);
			if(next_hop_index == -1)
			{
				return;
			}
			int nextX = neighbors[next_hop_index][0];
			int nextY = neighbors[next_hop_index][1];
			sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, hdr->type, hdr->origid, hdr->m_count, hdr->isSuccess, hdr->isWrite, hdr->srcX, hdr->srcY, nextX, nextY, hdr->destX, hdr->destY, hdr->csm_seq);
		}
	}
	/************************ PARKING REQUEST ****************************************/
	/************************ PARKING REPLY ****************************************/
	else if(cmnhdr->subtype == PARKING_REPLY)
	{
		if(hdr->destX == regionX_ && hdr->destY == regionY_ && hdr->nextX == regionX_ && hdr->nextY == regionY_)
		{
			if(!removeClientRequest(hdr->origid, hdr->m_count))
			{
				return;
			}			
			if(leader_status_ == LEADER)
			{
				printf("sender was - %d; SENTTOCLIENT!\n", hdr->origid);
			}
			sendp(SEND, SERVER_MESSAGE, IP_BROADCAST, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, hdr->isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
			sendp(SENDLOOPBACK, SERVER_MESSAGE, nodeID_, client_port_, PARKING_ACK, hdr->origid, hdr->m_count, hdr->isSuccess, hdr->isWrite, regionX_, regionY_, regionX_, regionY_, regionX_, regionY_, -1);
		}
		else if(hdr->nextX == regionX_ && hdr->nextY == regionY_)
		{
			int next_hop_index = getNextHop(hdr->destX, hdr->destY);
			if(next_hop_index == -1)
			{
				return;
			}
			int nextX = neighbors[next_hop_index][0];
			int nextY = neighbors[next_hop_index][1];
			sendp(SEND, PARKING_REPLY, IP_BROADCAST, MY_PORT_, hdr->type, hdr->origid, hdr->m_count, hdr->isSuccess, hdr->isWrite, hdr->srcX, hdr->srcY, nextX, nextY, hdr->destX, hdr->destY, -1);
		}
	}
	/************************ PARKING REPLY ****************************************/
	/************************ WRITE UPDATE ****************************************/
	else if(cmnhdr->subtype == WRITE_UPDATE)
	{
		assert(CSM == 1);
		int hops = abs(hdr->srcX - regionX_);
		if(hops < abs(hdr->srcY - regionY_))
		{
			hops = abs(hdr->srcY - regionY_);
		}
		if((abs(hdr->nextX-regionX_) <= 1) && (abs(hdr->nextY-regionY_) <= 1) && (hops <= MAX_HOP_SHARING))
		{
			if(leader_status_ == LEADER)
				printf("NodeID_ - %d (%d, %d) Got a write update for sequence - %d from region (%d, %d)\n", nodeID_, regionX_, regionY_, hdr->csm_seq, hdr->srcX, hdr->srcY);		

			pair<int, int> region(hdr->srcX, hdr->srcY);
			pair<int, int> myRegion(regionX_, regionY_);
			if((hdr->csm_seq >= l_seq[region]) && (region != myRegion))
			{	
				WriteUpdate update_req;
				update_req.reg_x = hdr->srcX;
				update_req.reg_y = hdr->srcY;
				update_req.parking_spots = hdr->isSuccess;
				update_req.seq_no = hdr->csm_seq;
				m_write_updates[region].push(update_req);

				LogParkingFile::central_m_write_updates[regionX_][regionY_][region].push(update_req);

			//	while(l_seq[region] == m_write_updates[region].top().seq_no)
				while(1)	
				{
					WriteUpdate up_req = m_write_updates[region].top();
	
					if(leader_status_ == LEADER)
						printf("NodeID_ - %d (%d, %d) UPDATING parking spots = %d for region (%d, %d)\n", nodeID_, regionX_, regionY_, up_req.parking_spots, region.first, region.second);		

					m_parkingstate.num_free_parking_spaces[up_req.reg_x][up_req.reg_y] = up_req.parking_spots;
					m_write_updates[region].pop();

					LogParkingFile::central_m_write_updates[regionX_][regionY_][region].pop();
					LogParkingFile::central_l_seq[regionX_][regionY_][region]++;

					l_seq[region]++;
					if(m_write_updates[region].size() == 0)
						break;
				}
			
				int next_hop_index = getNextHop(hdr->srcX, hdr->srcY);
				if(next_hop_index == -1)
				{
					assert(1 == 0);
				}
				int nextX = neighbors[next_hop_index][0];
				int nextY = neighbors[next_hop_index][1];

				sendp(SEND, WRITE_UPDATE_REPLY, IP_BROADCAST, MY_PORT_, WRITE_UPDATE_REPLY, -1, -1, -1, -1, regionX_, regionY_, nextX, nextY, hdr->srcX, hdr->srcY, hdr->csm_seq);

				// Rebroadcast the write updates. 
				if((abs(hdr->nextX-regionX_) == 1) && (abs(hdr->nextY-regionY_) == 1) && (hops < MAX_HOP_SHARING)) //Maximal coverage!
				{
					sendp(SEND, WRITE_UPDATE, IP_BROADCAST, MY_PORT_, WRITE_UPDATE, -1, -1, hdr->isSuccess, hdr->isWrite, hdr->srcX, hdr->srcY, regionX_, regionY_, -1, -1, hdr->csm_seq);
				}
			}
			else
			{
				//Already worked on this request!
			}
		}
	}
	/************************ WRITE UPDATE ****************************************/
	/************************ WRITE UPDATE REPLY ****************************************/
	else if(cmnhdr->subtype == WRITE_UPDATE_REPLY)
	{
		assert(CSM == 1);	
		if(hdr->nextX == regionX_ && hdr->nextY == regionY_ && hdr->destX == regionX_ && hdr->destY == regionY_)
		{
			if(leader_status_ == LEADER)
				printf("NodeID_ - %d (%d, %d) got a write update reply for seq = %d from region (%d, %d)\n", nodeID_, regionX_, regionY_, hdr->csm_seq, hdr->srcX, hdr->srcY);		
			pair<int, int> region(hdr->srcX, hdr->srcY);
			m_seq_acks[hdr->csm_seq].push_back(region);
			LogParkingFile::central_m_seq_acks[regionX_][regionY_][hdr->csm_seq].push_back(region);
			
			if(m_seq_acks[hdr->csm_seq].size() == 16)
			{
				m_seq_acks.erase(hdr->csm_seq); //Got all acks!
				LogParkingFile::central_m_seq_acks[regionX_][regionY_].erase(hdr->csm_seq);
			}
		}
		else if(hdr->nextX == regionX_ && hdr->nextY == regionY_)
		{
			int next_hop_index = getNextHop(hdr->destX, hdr->destY);
			if(next_hop_index == -1)
			{
				assert(1 == 0);
			}
			int nextX = neighbors[next_hop_index][0];
			int nextY = neighbors[next_hop_index][1];

			sendp(SEND, WRITE_UPDATE_REPLY, IP_BROADCAST, MY_PORT_, WRITE_UPDATE_REPLY, -1, -1, -1, -1, hdr->srcX, hdr->srcY, nextX, nextY, hdr->destX, hdr->destY, hdr->csm_seq);
		}
	}
	/************************ WRITE UPDATE REPLY ****************************************/
}

void ParkingServerAgent::check_resending_status()
{
	if(m_resending_queue.empty())
		return;

	if(m_resending_queue.top().expiration_time <= Scheduler::instance().clock())
	{
		ClientRequest top_req = m_resending_queue.top();
		if(leader_status_ == LEADER)
			printf("RESENDING ORIG REQ = %d, %d\n", top_req.origid, top_req.m_count);
		int next_hop_index = getNextHop(top_req.destX, top_req.destY);
		if(next_hop_index == -1)
		{
		}
		else
		{
			int nextX = neighbors[next_hop_index][0];
			int nextY = neighbors[next_hop_index][1];
			int isSuccess = -1;
			sendp(SEND, PARKING_REQUEST, IP_BROADCAST, MY_PORT_, PARKING_REQUEST, top_req.origid, top_req.m_count, isSuccess, top_req.isWrite, regionX_, regionY_, nextX, nextY, top_req.destX, top_req.destY, -1);
			if(leader_status_ == LEADER)
				send_packets();
		}

		if(top_req.m_retries == MAX_RETRIES)
		{
			if(leader_status_ == LEADER)
				printf("Trying for the last time. hoping it is this time lucky :P\n ");
			m_resending_queue.pop();
			return;
		}
	
		ClientRequest client_req;
		client_req.origid = top_req.origid;
		client_req.m_count = top_req.m_count;
		client_req.isWrite = top_req.isWrite;
		client_req.destX = top_req.destX;
		client_req.destY = top_req.destY;
		client_req.m_retries = top_req.m_retries + 1;
		client_req.expiration_time = Scheduler::instance().clock() + 0.25;			

		m_resending_queue.pop();
		m_resending_queue.push(client_req);
		if(m_resending_queue.size() > 0)
		{
			double wait = m_resending_queue.top().expiration_time - Scheduler::instance().clock();
			if(wait <= 0)
				wait = 0.00001;
			resending_timer_.resched(wait);
		}
	}	
}

void ParkingServerAgent::sendp(int send_type, int msg_class, int dest, int dest_port, int msgType, int origid, int m_count, int isSuccess, int isWrite, int srcX, int srcY, int nextX, int nextY, int destX, int destY, int low_seq)
{
	Packet* pkt = allocpkt();
	hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
	hdr_ip* iph = HDR_IP(pkt);
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);
	hdr_vnparking* hdr = hdr_vnparking::access(pkt);

	//common header
	cmn_hdr->ptype() = PT_VNPARKING;
	cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
	cmn_hdr->next_hop_ = dest;

	//ip header
	iph->saddr() = Agent::addr();
	iph->daddr() = dest; //broadcasting address, should be -1
	iph->dport() = dest_port; //destination port has to be the server port
	iph->sport() = MY_PORT_; //no need
	iph->ttl() = 1;

	//vncommon header
	vnhdr->type = APPL_MSG;
	vnhdr->subtype = msg_class;
	vnhdr->regionX = regionX_;
	vnhdr->regionY = regionY_;
	vnhdr->send_time = lastTime_;
	vnhdr->src = Agent::addr();
	vnhdr->dst = dest;
	vnhdr->send_type = send_type;//sending, forwarding or loopback

	//parking header
	hdr->type = msgType; //client message types

	hdr->srcX = srcX; //original region
	hdr->srcY = srcY; //original region
	hdr->nextX = nextX; //next region
	hdr->nextY = nextY; //next region
	hdr->destX = destX; //dest region
	hdr->destY = destY; //dest region

	hdr->isWrite = isWrite;
	hdr->isSuccess = isSuccess;
	hdr->origid = origid; 
	hdr->m_count = m_count;
	hdr->csm_seq = low_seq;

	hdr->m_send_time = Scheduler::instance().clock();
	
	queue->enqueue(pkt);
}

u_char * ParkingServerAgent::getState()
{
	int size;
	size = getStateSize();
	struct SharedState * buff;
	buff = (struct SharedState *)malloc(size);

	int units;
	units = size/sizeof(struct SharedState);

	buff[0].version = m_version;
	buff[0].isClientRequest = -1;

	int i = 1;
	for(int row = 0; row < MAX_ROWS; row++)
	{
		for(int col = 0; col < MAX_COLS; col++)
		{
			buff[i].num_free_parking_spaces = m_parkingstate.num_free_parking_spaces[col][row];
			buff[i].isClientRequest = -1;
			i++;
		}
	}
	assert(i == units - m_resending_queue.size() - m_remote_requests.size());
	if(m_resending_queue.size() != 0)
	{
		priority_queue<ClientRequest> temp_queue;
		while(!m_resending_queue.empty())
		{
			ClientRequest top_req = m_resending_queue.top();
			m_resending_queue.pop();
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
			i++;
		}
		while(!temp_queue.empty())
		{
			ClientRequest top_req = temp_queue.top();
			temp_queue.pop();
			m_resending_queue.push(top_req);
		}
		assert(i == units - m_remote_requests.size());
	}
	if(m_remote_requests.size() != 0)
	{
		assert(i == (1 + MAX_ROWS*MAX_COLS + m_resending_queue.size())); 
		for(int j = 0; j < m_remote_requests.size(); j++)
		{
			buff[i].origid = m_remote_requests[j].origid;
			buff[i].m_count = m_remote_requests[j].m_count;
			buff[i].isSuccess = m_remote_requests[j].isSuccess;
			buff[i].isClientRequest = 0;
			buff[i].destX = -1;
			buff[i].destY = -1;
			buff[i].isWrite = -1;
			buff[i].m_retries = -1;
			buff[i].expiration_time = -1;
			i++;
		}
		assert(i == units);
	}
	return (u_char *)buff;
}

//return the size of the state
int ParkingServerAgent::getStateSize()
{
	int count = 1; // 1 for the version number;

	count += MAX_COLS*MAX_ROWS;
	count += m_resending_queue.size();
	count += m_remote_requests.size();	

	return (count)*sizeof(SharedState);
}

void ParkingServerAgent::saveState(unsigned char* state, int size)
{
	struct SharedState * buff;

	buff = (struct SharedState *) state;
	int units;
	units = size/sizeof(struct SharedState);

	if(units < 1)
	{
		log_info(CODE_PARKINGS, "GOTWRONGSTATESIZE", (float)units);
	}

	m_version = buff[0].version;
	send_loopback(ST_VER, m_version);	
	assert(buff[0].isClientRequest == -1);
	int i = 1;		
	for(int row = 0; row < MAX_ROWS; row++)
	{
		for(int col = 0; col < MAX_COLS; col++)
		{
			m_parkingstate.num_free_parking_spaces[col][row] = buff[i].num_free_parking_spaces ;
			assert(buff[i].isClientRequest == -1);
			i++;
		}
	}
	assert (i == 1 + MAX_COLS*MAX_ROWS);
	while(!m_resending_queue.empty())
	{
		m_resending_queue.pop();
	}
	m_remote_requests.clear();

	while (i < units)
	{
		if(buff[i].isClientRequest == 1)
		{
			struct ClientRequest client_req;

			client_req.origid = buff[i].origid;
			client_req.origid = buff[i].m_count;
			client_req.destX = buff[i].destX;
			client_req.destY = buff[i].destY;
			client_req.isWrite = buff[i].isWrite;
			client_req.m_retries = buff[i].m_retries;
			client_req.expiration_time = buff[i].expiration_time;
			assert(buff[i].isSuccess == -1);
			m_resending_queue.push(client_req);
			i++;
		}
		else if(buff[i].isClientRequest == 0)
		{

			struct RemoteRequest remote_req;
			remote_req.origid = buff[i].origid;
			remote_req.m_count = buff[i].m_count;
			remote_req.isSuccess = buff[i].isSuccess;
			i++;
		}
	} 
}

int ParkingServerAgent::isDuplicateRequest(int origid, int m_count)
{
	for(int i = 0; i < m_remote_requests.size(); i++)
	{
		if(m_remote_requests[i].origid == origid && m_remote_requests[i].m_count == m_count)
			return i;
	}
	return -1;	
}


bool ParkingServerAgent::removeClientRequest(int origid, int m_count)
{
	priority_queue<ClientRequest> temp_queue;

	int return_value = false;

	while(!m_resending_queue.empty())
	{
		ClientRequest top_req = m_resending_queue.top();
		m_resending_queue.pop();
		if(top_req.origid == origid && top_req.m_count == m_count)
		{
			return_value = true;
		}		
		else
		{
			temp_queue.push(top_req);
		}
	}
	while(!temp_queue.empty())
	{
		ClientRequest top_req = temp_queue.top();
		temp_queue.pop();
		m_resending_queue.push(top_req);
	}
	return return_value; 	
}

//evaluate two server packets to see if they are for the same transaction     
int ParkingServerAgent::equal(Packet * p1, Packet * p2)                           
{                                                                             
        hdr_vnparking * hdr1 = hdr_vnparking::access(p1) ;                            
        hdr_vnparking * hdr2 = hdr_vnparking::access(p2) ;                            
        if(hdr1->type == hdr2->type 
	&& hdr1->srcX == hdr2->srcX 
	&& hdr1->srcY == hdr2->srcY 
	&& hdr1->nextX == hdr2->nextX 
	&& hdr1->nextY == hdr2->nextY 
	&& hdr1->destX == hdr2->destX 
	&& hdr1->destY == hdr2->destY 
	&& hdr1->isWrite == hdr2->isWrite 
	&& hdr1->isSuccess == hdr2->isSuccess
	&& hdr1->origid == hdr2->origid 
	&& hdr1->m_count == hdr2->m_count) 
        {
                return 1 ;
        }                                                                     
        else                                                                  
        {       
                return 0 ;                                                    
        }
        
}               


