#include "mobilenode.h"
#include "parkingclient.h"
#include "time.h"

#define GET_TIME Scheduler::instance().clock()

//#include <sys/ipc.h>

/* OTCL linkage for the ParkingHeader */
int hdr_vnparking::offset_;
static class ParkingHeaderClass : public PacketHeaderClass {
public:
	ParkingHeaderClass() : PacketHeaderClass("PacketHeader/PARKING",
					      sizeof(hdr_vnparking)) {
		bind_offset(&hdr_vnparking::offset_);
	}
} class_parking;


/*OTCL linkage for the VnsClass */
static class ParkingClientClass : public TclClass {
public:
	ParkingClientClass() : TclClass("Agent/VNCAgent/ParkingClient") {}
	TclObject* create(int, const char*const*) {
		return (new ParkingClientAgent());
	}
} class_parkingclient;

/* Agent/VNS constructor */
ParkingClientAgent::ParkingClientAgent() : VNCAgent(),parking_timer_(this),clock_timer_(this), resending_timer_(this)
{
	client_state_ = UNKNOWN;

	app_code = CODE_PARKING;

	m_count = 0;
}

void ParkingClientAgent::check_clock_status()
{
	if(client_state_ == UP)
	{
	}
}

void ParkingClientAgent::check_parking_status()
{
	if(client_state_ == UP)
	{
		int isWrite = 0;
		if(nodeID_%2 == 0)
			isWrite = 1;

		if(m_count < 100)
		{
			int dest_x, dest_y;
			dest_x = -1;
			dest_y = -1;

			int prob =  ((int) rand()) % 100;

			if(prob < 25)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 3;
					dest_y = regionY_ + 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 3;
					dest_y = regionY_ - 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 3;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 3;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 3;
				}
			}
			else if(prob < 50)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 2;
					dest_y = regionY_ + 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 2;
					dest_y = regionY_ - 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 2;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 2;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 2;
				}
			}
			else if(prob < 75)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 1;
					dest_y = regionY_ + 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 1;
					dest_y = regionY_ - 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 1;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 1;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 1;
				}
			}
			else
			{
				dest_x = regionX_;
				dest_y = regionY_;
			}
			if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
			{
				dest_x = regionX_;
				dest_y = regionY_;
			}

			int n_nodes = LogParkingFile::getNumNodes(regionX_, regionY_);
			bool r_status = LogParkingFile::isLeaderActive(regionX_, regionY_);

			if(n_nodes == 0)
			{
				assert(r_status == false);
				printf("REGION EMPTY \n");
			}
			else if(r_status == false)
			{
				printf("NO CURRENT LEADER at time - %f \n", Scheduler::instance().clock());
			}

			ClientRequest client_req;
			client_req.origid = nodeID_;
			client_req.m_count = m_count;
			client_req.isWrite = isWrite;
			client_req.destX = dest_x;
			client_req.destY = dest_y;
			client_req.m_retries = 0;
			client_req.expiration_time = Scheduler::instance().clock() + RESEND_BACKOFF;

			m_resending_queue.push(client_req);
			if(m_resending_queue.size() > 0)
			{
				double wait = m_resending_queue.top().expiration_time - Scheduler::instance().clock();
				if(wait <= 0)
					wait = 0.00001;
				resending_timer_.resched(wait);
			}

			printf("Client Node-%d (%d, %d):: sending request for region (%d, %d) with m_count = %d and num servers = %d, and leadership status = %d at time - %f and isWrite = %d\n", nodeID_, regionX_, regionY_, dest_x, dest_y, m_count, n_nodes, r_status, Scheduler::instance().clock(), isWrite);

			if(dest_x == regionX_ && dest_y == regionY_)
			{			
				msg_hops[0]++;
				hop_count[m_count] = 0;
			}
			else if(abs(dest_x-regionX_) == 3 || abs(dest_y - regionY_) == 3)	
			{
				msg_hops[3]++;
				hop_count[m_count] = 3;
			}	
			else if(abs(dest_x-regionX_) == 2 || abs(dest_y - regionY_) == 2)	
			{
				msg_hops[2]++;
				hop_count[m_count] = 2;
			}	
			else if(abs(dest_x-regionX_) == 1 || abs(dest_y - regionY_) == 1)	
			{
				msg_hops[1]++;
				hop_count[m_count] = 1;
			}
			else
			{
				assert(1==0);
			}

			sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, DMSG, nodeID_, isWrite, m_count, dest_x, dest_y);
			send_packets();

			m_send_time[m_count] = GET_TIME;
			m_count++;
		}
		else if(m_count >= 1)
		{
			for(int hop = 0; hop < 4; hop++)
			{
				double avg_res_time = 0.0;
				for(int i = 0; i < m_response_time[hop].size(); i++)
				{
					avg_res_time += m_response_time[hop][i];
				}
				if(m_response_time[hop].size() > 0)
					printf("Node-%d:: Average response time for %d hops = %f\n", nodeID_, hop, avg_res_time/m_response_time[hop].size()); 
			}
			if(m_res_time.size() > 0)
			{	
				printf("Node-%d:: Msgs send = %d; Msgs acked = %d;\n", nodeID_, m_count, m_res_time.size()); 
				printf("Node-%d:: zero_hop = %d, one_hop = %d, two_hop = %d, three_hop = %d\n", nodeID_, msg_hops[0], msg_hops[1], msg_hops[2], msg_hops[3]); 
			}
		}	
		parking_timer_.resched(100); // every X cycles a node requests a parking spot. Think of a better way to get this done. 
	}
}

/*
void ParkingClientAgent::check_parking_status()
{
	if(client_state_ == UP)
	{
		int isWrite = 0;
		if(nodeID_%2 == 0)
			isWrite = 1;

		if(m_count < 100)
		{
			int dest_x, dest_y;
			dest_x = -1; 
			dest_y = -1;

			int prob =  ((int) rand()) % 100;

			if(prob < 25)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 3;
					dest_y = regionY_ + 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 3;
					dest_y = regionY_ - 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 3;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 3;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 3;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 3;
				}
			}
			else if(prob < 50)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 2;
					dest_y = regionY_ + 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 2;
					dest_y = regionY_ - 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 2;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 2;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 2;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 2;
				}
			}
			else if(prob < 75)
			{
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 1;
					dest_y = regionY_ + 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 1;
					dest_y = regionY_ - 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ + 1;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ + 1;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_ - 1;
					dest_y = regionY_;
				}
				if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
				{
					dest_x = regionX_;
					dest_y = regionY_ - 1;
				}
			}
			else
			{
				dest_x = regionX_;
				dest_y = regionY_;
			}
			if(!(dest_x >=0 && dest_x < 4 && dest_y >= 0 && dest_y < 4))
			{
				dest_x = regionX_;
				dest_y = regionY_;
			}

			int n_nodes = LogParkingFile::getNumNodes(regionX_, regionY_);
			bool r_status = LogParkingFile::isLeaderActive(regionX_, regionY_);

			if(n_nodes == 0)
			{
				assert(r_status == false);
				printf("REGION EMPTY \n");
			}
			else if(r_status == false)
			{
				printf("NO CURRENT LEADER at time - %f \n", Scheduler::instance().clock());
			}

			ClientRequest client_req;
			client_req.origid = nodeID_;
			client_req.m_count = m_count;
			client_req.isWrite = isWrite;
			client_req.destX = dest_x;
			client_req.destY = dest_y;
			client_req.m_retries = 0;
			client_req.expiration_time = Scheduler::instance().clock() + RESEND_BACKOFF;

			m_resending_queue.push(client_req);
			if(m_resending_queue.size() > 0)
			{
				double wait = m_resending_queue.top().expiration_time - Scheduler::instance().clock();
				if(wait <= 0)
					wait = 0.00001;
				resending_timer_.resched(wait);
			}

			printf("Client Node-%d (%d, %d):: sending request for region (%d, %d) with m_count = %d and num servers = %d, and leadership status = %d at time - %f\n", nodeID_, regionX_, regionY_, dest_x, dest_y, m_count, n_nodes, r_status, Scheduler::instance().clock());

			if(dest_x == regionX_ && dest_y == regionY_)
			{			
				msg_hops[0]++;
				hop_count[m_count] = 0;
			}
			else if(abs(dest_x-regionX_) == 3 || abs(dest_y - regionY_) == 3)	
			{
				msg_hops[3]++;
				hop_count[m_count] = 3;
			}	
			else if(abs(dest_x-regionX_) == 2 || abs(dest_y - regionY_) == 2)	
			{
				msg_hops[2]++;
				hop_count[m_count] = 2;
			}	
			else if(abs(dest_x-regionX_) == 1 || abs(dest_y - regionY_) == 1)	
			{
				msg_hops[1]++;
				hop_count[m_count] = 1;
			}
			else
			{
				assert(1==0);
			}
	
			sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, DMSG, nodeID_, isWrite, m_count, dest_x, dest_y);
			send_packets();

			m_send_time[m_count] = GET_TIME;
			m_count++;
		}
		else if(m_count >= 1)
		{
			for(int hop = 0; hop < 4; hop++)
			{
				double avg_res_time = 0.0;
				for(int i = 0; i < m_response_time[hop].size(); i++)
				{
					avg_res_time += m_response_time[hop][i];
				}
				if(m_response_time[hop].size() > 0)
					printf("Node-%d:: Average response time for %d hops = %f\n", nodeID_, hop, avg_res_time/m_response_time[hop].size()); 
			}
			if(m_res_time.size() > 0)
			{	
				printf("Node-%d:: Msgs send = %d; Msgs acked = %d;\n", nodeID_, m_count, m_res_time.size()); 
				printf("Node-%d:: zero_hop = %d, one_hop = %d, two_hop = %d, three_hop = %d\n", nodeID_, msg_hops[0], msg_hops[1], msg_hops[2], msg_hops[3]); 
			}
		}	
		parking_timer_.resched(200); // every X cycles a node requests a parking spot. Think of a better way to get this done. 
	}
}	
*/

void ParkingClientAgent::check_resending_status()
{
	if(m_resending_queue.empty())
		return;

	if(m_resending_queue.top().expiration_time <= Scheduler::instance().clock())
	{
		printf("Client with NodeID_ - %d is got RESEND_EXP for request = %d, %d and time - %f\n", nodeID_, m_resending_queue.top().origid, m_resending_queue.top().m_count, Scheduler::instance().clock());
		ClientRequest top_req = m_resending_queue.top();

		int dest_x = top_req.destX;
		int dest_y = top_req.destY;

		if(dest_x == regionX_ && dest_y == regionY_)
		{	
			msg_hops[hop_count[m_count]]--;
			msg_hops[0]++;
			hop_count[m_count] = 0;
		}
		else if(abs(dest_x-regionX_) == 3 || abs(dest_y - regionY_) == 3)	
		{
			msg_hops[hop_count[m_count]]--;
			msg_hops[3]++;
			hop_count[m_count] = 3;
		}	
		else if(abs(dest_x-regionX_) == 2 || abs(dest_y - regionY_) == 2)	
		{
			msg_hops[hop_count[m_count]]--;
			msg_hops[2]++;
			hop_count[m_count] = 2;
		}	
		else if(abs(dest_x-regionX_) == 1 || abs(dest_y - regionY_) == 1)	
		{
			msg_hops[hop_count[m_count]]--;
			msg_hops[1]++;
			hop_count[m_count] = 1;
		}
		else
		{
			assert(1==0);
		}

		m_send_time[top_req.m_count] = GET_TIME;

		sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, PARKING_REQUEST_RESEND, top_req.origid, top_req.isWrite, top_req.m_count, top_req.destX, top_req.destY);
		//		sendp(SEND, CLIENT_MESSAGE, IP_BROADCAST, server_port_, PARKING_REQUEST_RESEND, top_req.origid, top_req.isWrite, top_req.m_count, regionX_, regionY_);
		send_packets();

		if(top_req.m_retries == MAX_RETRIES)
		{
			printf("CLIENT Trying for the last time. hoping it is this time LUCKY :P\n ");
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
		client_req.expiration_time = Scheduler::instance().clock() + RESEND_BACKOFF;

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

bool ParkingClientAgent::removeClientRequest(int origid, int m_count)
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

void ParkingClientAgent::sendp(int send_type, int msg_class, int dest, int dest_port, int msgType, int origid, int isWrite, int m_count, int dest_regionX, int dest_regionY)
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
		vnhdr->send_time = Scheduler::instance().clock();
		vnhdr->src = Agent::addr();
		vnhdr->dst = dest;
		vnhdr->send_type = send_type;//sending, forwarding or loopback

		//parking header
		hdr->type = msgType; //client message types
		hdr->m_send_time = Scheduler::instance().clock();

		hdr->srcX = regionX_; //original region
		hdr->srcY = regionY_; //original region

		hdr->destX = dest_regionX; //destination region
		hdr->destY = dest_regionY; //destination region
		
		hdr->nextX = -1;
		hdr->nextY = -1;

		hdr->origid = origid;
		hdr->isWrite = isWrite;
		hdr->m_count = m_count;

		queue->enqueue(pkt); 
		//send another one for the server located on the same node
		send_loopback(msgType, nodeID_, m_count, isWrite, dest_regionX, dest_regionY);
//		send_packets(); // NIKET - changed this
}

void ParkingClientAgent::send_loopback(int msgType, int origid, int m_count, int isWrite, int dest_regionX, int dest_regionY)
{
		Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_vnparking* hdr = hdr_vnparking::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);

		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);
		vnhdr->type = APPL_MSG;
		vnhdr->subtype = CLIENT_MESSAGE;
		vnhdr->regionX = regionX_;
		vnhdr->regionY = regionY_;
		vnhdr->send_time = Scheduler::instance().clock();
		vnhdr->src = Agent::addr();
		vnhdr->dst = Agent::addr();//sequence number of the transaction
		vnhdr->send_type = SENDLOOPBACK;//sending, forwarding or loopback

		cmn_hdr->ptype() = PT_VNPARKING;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = IP_BROADCAST;

		iph->saddr() = Agent::addr();
		iph->daddr() = Agent::addr(); //broadcasting address, should be -1
		iph->dport() = server_port_; //destination port has to be the server port
		//iph->sport() = MY_PORT_; //no need
		hdr->type = msgType; //client message types
		hdr->m_send_time = Scheduler::instance().clock();

		hdr->srcX = regionX_;
		hdr->srcY = regionY_;

		hdr->destX = dest_regionX;
		hdr->destY = dest_regionY;

		hdr->origid = origid;
		hdr->isWrite = isWrite;
		hdr->m_count = m_count;

		queue->enqueue(pkt);
}

void ParkingClientAgent::handle_packet(Packet * pkt)
{
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);//get the vnlayer header
	hdr_vnparking * hdr = hdr_vnparking::access(pkt); //get the header

	if(hdr->srcX == regionX_ && hdr->srcY == regionY_)
	{
		if(vnhdr->type == APPL_MSG && hdr->origid == nodeID_)
		{
			for(int r = 0; r < m_response_count.size(); r++)
			{
				if(m_response_count[r] == hdr->m_count)
				{
					printf("Node = %d; DUPLICATE_ACK for count = %d\n", nodeID_, hdr->m_count);
					return;
				}
			}
			printf("Client with NodeID_ - %d is UP and got a server packet and sender is - %d message type is = %d\n", nodeID_, vnhdr->src, vnhdr->subtype);				
			printf("Node = %d; Success = %d for m_count - %d, hop_count = %d, and delay = %f\n", nodeID_, hdr->isSuccess, hdr->m_count, hop_count[hdr->m_count], GET_TIME - m_send_time[hdr->m_count]);
			printf("--------------------------------\n");
			m_response_time[hop_count[hdr->m_count]].push_back(GET_TIME - m_send_time[hdr->m_count]);
			m_res_time.push_back(GET_TIME - m_send_time[hdr->m_count]);
			m_response_count.push_back(hdr->m_count);
			removeClientRequest(nodeID_, hdr->m_count);
			return;
		}
	}
}
