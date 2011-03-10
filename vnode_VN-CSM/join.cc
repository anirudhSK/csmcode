/*
 * Copyright (C) 2009 City University of New York
 * All rights reserved.
 *
 * NOTICE: This software is provided "as is", without any warranty,
 * including any implied warranty for merchantability or fitness for a
 * particular purpose.  Under no circumstances shall CUNY
 * or its faculty, staff, students or agents be liable for any use of,
 * misuse of, or inability to use this software, including incidental
 * and consequential damages.

 * License is hereby given to use, modify, and redistribute this
 * software, in whole or in part, for any commercial or non-commercial
 * purpose, provided that the user agrees to the terms of this
 * copyright notice, including disclaimer of warranty, and provided
 * that this copyright notice, including disclaimer of warranty, is
 * preserved in the source code and documentation of anything derived
 * from this software.  Any redistributor of this software or anything
 * derived from this software assumes responsibility for ensuring that
 * any parties to whom such a redistribution is made are fully aware of
 * the terms of this license and disclaimer.
 *
 * Author: Jiang Wu, CS Dept., City University of New York
 * Email address: jwu1@gc.cuny.edu
 * Nov. 25, 2009
 *
 * join.cc: Code for a 'Join' Agent Class for the leader election
 *       and movement tracking layer of the virtual node system
 *
 */
#include "join.h"		//header for agent JOIN
#include "mobilenode.h"
#include "time.h"

#include <sys/ipc.h>
#include <sys/shm.h>

NsObject *ll;//not used anymore

/* OTCL linkage for the VnsHeader */
int hdr_vns::offset_;
static class VnsHeaderClass : public PacketHeaderClass {
public:
	VnsHeaderClass() : PacketHeaderClass("PacketHeader/VNS",
					      sizeof(hdr_vns)) {
		bind_offset(&hdr_vns::offset_);
	}
} class_vnshdr;

/* OTCL linkage for the Virtual Node Common Message Header */
int hdr_vncommon::offset_;
static class VncommonHeaderClass : public PacketHeaderClass {
public:
	VncommonHeaderClass() : PacketHeaderClass("PacketHeader/VNCOMMON",
					      sizeof(hdr_vncommon)) {
		bind_offset(&hdr_vncommon::offset_);
	}
} class_vncommonhdr;

/* OTCL linkage for the JoinHeader so that the class can be accessed from TCL code too */
int hdr_join::offset_;
static class JoinHeaderClass : public PacketHeaderClass {
public:
	JoinHeaderClass() : PacketHeaderClass("PacketHeader/Join",
					      sizeof(hdr_join)) {
		bind_offset(&hdr_join::offset_);
	}
} class_joinhdr;

/*otcl linkage for the JoinClass so that the class can be accessed from TCL code too */
static class JoinClass : public TclClass {
public:
	JoinClass() : TclClass("Agent/Join") {}
	TclObject* create(int, const char*const*) {
		return (new JoinAgent());
	}
} class_join;

/* Agent/Join constructor */
JoinAgent::JoinAgent() : Agent(PT_REGION),join_timer_(this),leader_req_timer_(this),old_leader_timer(this),tcl(Tcl::instance())
{
	bind("port_number_", &MY_PORT_);
	bind("maxX_", &maxX_);
	bind("maxY_", &maxY_);
	bind("columns_", &columns_);
	bind("rows_", &rows_);

	bind("nodeID_",&nodeID_);
	bind("regionX_", &regionX_);
	bind("regionY_", &regionY_);
	bind("seq_", &seq_);
	bind("status_", &status_);
	bind("leader_", &leader_);
	bind("leader_status_", &leader_status_);

	bind("beat_period_", &beat_period_);
	bind("max_delay_", &max_delay_);
	bind("claim_period_",&claim_period_);
	bind("beat_miss_limit_",&beat_miss_limit_);

	bind_time("interval_",&interval_);
	bind_time("slowInterval_",&slowInterval_);
	bind_time("zeroDistance_",&zeroDistance_);

	bind("packetSize_", &size_);

	beat_misses_ = 0;

	num_apps = 0;

	srand(time(NULL)+nodeID_);

	state_synced_ = FALSE;//state unknown
	time_to_leave_ = UNKNOWN;//static
	old_leader_retries = 0;

	int shmid;
	/*
	 * Create the shared memory segment for the LogFile object.
	 */
	 //printf("getting mem in agent join");
	if ((shmid = shmget(KEYVALUE, sizeof(LogFile), IPC_CREAT | 0666)) < 0)
	{
		perror("shmget");
		exit(1);
	}

	/*
	 * Now we attach the segment to our pointer to the logfile object.
	 */
	if ((logfile = (LogFile *)shmat(shmid, NULL, 0)) == (LogFile *) -1)
	{
		perror("shmat");
		exit(1);
	}

	logfile->fp = 0;


}

bool JoinAgent::isOldRequest(int ver, int src, int x, int y)
{
}

bool JoinAgent::isOldRequestValid(int x, int y, int ver)
{
}

void JoinAgent::markOldRequestInValid(int x, int y, int ver)
{
}


/*
 * The method processing every packet received by the agent
 *
 */

void JoinAgent::recv(Packet* pkt, Handler*)
{
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);//deal with ST_SYNCED messages

	if(vnhdr->type == SYNC_MSG)//this must be a loopback message
	{
		if(vnhdr->subtype == ST_SYNCED)
		{
			//log_info(CODE_JOIN, "RECV", (double)vnhdr->subtype);
			state_synced_ = TRUE;
			leader_start_ = vnhdr->send_time;
		}
		else if(vnhdr->subtype == ST_VER)
		{
			m_version = vnhdr->src; // Just reusing the src field of vnhdr for sending version;	
		}
	}
	else if(vnhdr->type == JOIN_MSG)
	{
		hdr_join* hdr = hdr_join::access(pkt);

		if(hdr->regionX != regionX_ || hdr->regionY != regionY_)//not the same region
		{
			if(hdr->type == HEART_BEAT && hdr->dst == -1 && abs(hdr->regionX - regionX_) <= 1 && abs(hdr->regionY - regionY_) <= 1)
			{
				copy_loopback(pkt, 17920);
			}
		}
		// incoming packet has to be either destined for the node or broadcast, the source has to be in the same region.
		if((hdr->dst == nodeID_ || hdr->dst == -1) && hdr->regionX == regionX_ && hdr->regionY == regionY_)
		{
			log(CODE_JOIN, "RECV", hdr->toString());
			if (hdr->type == LEADER_REQUEST)// got a request message
			{
				if(leader_status_ == LEADER)//leader already, absolutely say NO
				{
					send_unicast(LEADER_REPLY, hdr->src, CONSENT, hdr->seq);
				}
			}
			else if(hdr->type == LEADER_REPLY) //got a reply message, this message is unicast, so it can be heard only by the requestor
			{
				if(leader_status_ == REQUESTED && hdr->dst == nodeID_ && hdr->seq == seq_)//the messages is for my last request
				{
					leader_status_ = UNKNOWN;
					/*
					if(hdr->answer == CONSENT) //Participate in the VN as non-leader
					{
						leader_ = hdr->src;
						leader_status_ = NON_LEADER; //REQUESTED
						send_loopback(NONLEADER);
					}
					else if(hdr->answer == DISSENT) //Do not participate in the VN
					{
						// Do nothing. 
					}
					*/	
				}
			}
		}
		if(hdr->type == LEADER_ELECT && (hdr->dst == -1) && (hdr->old_x == regionX_)&& (hdr->old_y == regionY_))
		{
			// Start leader election
			assert(leader_status_ != LEADER); //There cannot be leader in this region
			if(m_version == hdr->version)
			{
				//start the leader election process. 
				leader_status_ = PENDING;
				send_left_unicast(LEADER_REQUEST_REMOTE, hdr->src, m_version, hdr->old_x, hdr->old_y, UNKNOWN);
			}
			else
			{
				//TODO - non-sync nodes should be delayed a bit so that sync wins
				leader_status_ = PENDING;
				send_left_unicast(LEADER_REQUEST_REMOTE, hdr->src, m_version, hdr->old_x, hdr->old_y, UNKNOWN);
			}
		}
		if(hdr->type == LEADER_REQUEST_REMOTE && (hdr->dst == nodeID_))
		{
			for(int l = 0; l < old_leaders.size(); l++)
			{
				oldLeaderData old_l = old_leaders[l];
				if(old_l.old_x == hdr->regionX && old_l.old_y == hdr->regionY)
				{	
					if(old_l.is_valid == false)
						continue;
					if(old_l.leader_ack == false)
					{
						if(m_version == hdr->version || (old_l.retries >= 3))
						{
							old_leaders[l].leader_ack = true;
							old_leaders[l].new_leader = hdr->src;
							send_left_unicast(LEADER_ACK_REMOTE, hdr->src, UNKNOWN, old_l.old_x, old_l.old_y, UNKNOWN);
							break;
						}
					}	
				}
			}
		}
		if(hdr->type == LEADER_ACK_ACK && (hdr->dst == nodeID_))
		{
			for(int l = 0; l < old_leaders.size(); l++)
			{
				oldLeaderData old_l = old_leaders[l];
				if(old_l.old_x == hdr->regionX && old_l.old_y == hdr->regionY)
				{	
					assert(old_l.leader_ack == true);
					if(old_l.leader_ack_ack == false)
					{
						assert(old_l.is_valid == true);
						assert(old_l.new_leader == hdr->src);
						old_leaders[l].leader_ack_ack = true;
						old_leaders[l].is_valid = false;
						break;
					}
				}	
			}
		}
		if(hdr->type == LEADER_ACK_REMOTE && hdr->dst == nodeID_ && (hdr->old_x == regionX_) && (hdr->old_y == regionY_))
		{
			if(leader_status_ == PENDING)
			{
				setNeighbors();
				leader_start_ = Scheduler::instance().clock();
				leader_ = nodeID_;
				leader_status_ = LEADER;
			//	LogParkingFile::setRegionActive(regionX_, regionY_, nodeID_); 
				send_left_unicast(LEADER_ACK_ACK, hdr->src, m_version, hdr->old_x, hdr->old_y, UNKNOWN);
				LogParkingFile::setLeaderActive(regionX_, regionY_, true, Scheduler::instance().clock());
				state_synced_ = TRUE;
				send_loopback(NEWLEADER);
			}
			else if(leader_status_ == LEADER)
			{
				send_left_unicast(LEADER_ACK_ACK, hdr->src, m_version, hdr->old_x, hdr->old_y, UNKNOWN);
			}
			else
			{
			}
		}
	}
	Packet::free(pkt);
}

void JoinAgent::check_old_leader_status()
{
	for(int l = 0; l < old_leaders.size(); l++)
	{
		oldLeaderData old_l = old_leaders[l];
		if(old_l.is_valid == false)
			continue;
		if(old_l.retries < 40)
		{
			if(old_l.leader_ack == false)
			{
				old_leaders[l].retries++;
			//	assert(old_l.latest_version == LogParkingFile::getVersion(old_l.old_x, old_l.old_y));
				assert(old_l.parking_spots == LogParkingFile::getFreeSpots(old_l.old_x, old_l.old_y));
				send_left_broadcast(LEADER_ELECT, old_l.latest_version, old_l.parking_spots, old_l.old_x, old_l.old_y, UNKNOWN); 
				old_leader_timer.resched(4*claim_period_); 
			}
			else if(old_l.leader_ack_ack == false)
			{
				old_leaders[l].retries++;
				assert(old_l.new_leader != -1);
				send_left_unicast(LEADER_ACK_REMOTE, old_l.new_leader, UNKNOWN, old_l.old_x, old_l.old_y, UNKNOWN);
				old_leader_timer.resched(4*claim_period_); 
			}
		}	
		else 
		{
			if(old_leaders[l].leader_ack_ack)
			{
				old_leaders[l].is_valid = false;
				continue;
			}
			printf("Node - %d:: was old leader and GAVEUP electing a new leader. setting region to inactive\n", nodeID_);
			old_leaders[l].retries = 0;
			old_leaders[l].leader_ack = true;
			old_leaders[l].leader_ack_ack = true;
			old_leaders[l].is_valid = false;
			LogParkingFile::setRegionInActive(old_l.old_x, old_l.old_y, nodeID_, -1); 
		}
	}	
}

/*
 * Awaken by the leader election timer.
 * Check the current status to see if anything needs to be done.
 */
void JoinAgent::check_leader_status()
{
	if(leader_status_ == UNKNOWN) //UNKNOWN
	{
		//Ignore since I should have got it. 
	}
	else if(leader_status_ == REQUESTED)//No response to leadership request. Check central server now. 
	{
		if(!LogParkingFile::isRegionActive(regionX_, regionY_))
		{
			LogParkingFile::setRegionActive(regionX_, regionY_, nodeID_);
			setNeighbors();
			leader_start_ = Scheduler::instance().clock();
			leader_ = nodeID_;
			leader_status_ = LEADER;
			LogParkingFile::setLeaderActive(regionX_, regionY_, true, leader_start_);
			state_synced_ = TRUE;
			send_loopback(NEWLEADER);
		}
		else
		{	
			// Do nothing as there is already a VN running, which doesn't want me. 
		}
	}
}

/*
 * Awaken by the location checking timer.
 * Check the current location to see if anything needs to be done
 */

double JoinAgent::check_location()
{
	MobileNode * this_node = (MobileNode*)(Node::get_node_by_address(Agent::addr()));
	//get current position
	double x,y,z;
	this_node->getLoc(&x,&y,&z);

	//get current region
	int rx,ry;
	getRegion(x,y,&rx,&ry);

	int region_changed = FALSE;

	if(rx != regionX_ || ry != regionY_)//region changed
	{
		region_changed = TRUE;

		LogParkingFile::setNumNodes(regionX_, regionY_, -1);
		LogParkingFile::setNumNodes(rx, ry, 1);

		int orx = regionX_;
		int ory = regionY_;

		regionX_ = rx;
		regionY_ = ry;
		//tell other nodes in the region that the leader has left
		if(leader_status_ == LEADER)
		{
			LogParkingFile::setLeaderActive(orx, ory, false, Scheduler::instance().clock());
			old_leader_retries = 0;
			old_leader_timer.resched(2*claim_period_); 
		//	assert(m_version == LogParkingFile::getVersion(orx, ory)); //TODO - why this failing. ?
			int spots = LogParkingFile::getFreeSpots(orx, ory);
			oldLeaderData old_l(m_version, orx, ory, spots);
			old_leaders.push_back(old_l);
			send_left_broadcast(LEADER_ELECT, m_version, spots, orx, ory, UNKNOWN); 
		}

		status_reset();
		leader_status_ = REQUESTED; //REQUESTED
		send_loopback(NEWREGION);
		send_broadcast(LEADER_REQUEST, UNKNOWN);//send a leader request message
		leader_req_timer_.resched(2*claim_period_); //wait for a claim period
	
		char * str = (char *) malloc(MSG_STRING_SIZE);

		if(leader_status_ == DEAD)
		{
			sprintf(str,"(%d.%d),<,(-1.-1)", regionX_, regionY_);
			if(LOG_ENABLED)
				log(CODE_MOVE, "START", str);
		}
		else
		{
			sprintf(str,"(%d.%d),<,(%d.%d)", regionX_, regionY_, orx, ory);
			if(LOG_ENABLED)
				log_info(CODE_MOVE,"ENTER",str);
		}
	}

	//log_info(CODE_MOVE, "SPEED", "speed checking ...");
	//get the speed and direction
	double destX,destY,speed;
	destX = this_node ->destX();
	destY = this_node ->destY();
	speed = this_node ->speed();

	//check the destination region id
	int drx, dry;
	getRegion(destX,destY,&drx,&dry);

	//get the speeds on every single direction
	double speedX, speedY, speedZ;
	this_node->getVelo(&speedX,&speedY,&speedZ);


	//schedule the next event
	double wait_time;
	if(speed == 0)//the node is not moving
	{
		status_ = STOPPED;
		if(LOG_ENABLED)
			log_info(CODE_MOVE,"STOPPED","node is not moving");
		//wait_time = slowInterval_;

		time_to_leave_ = UNKNOWN;////set to infinity
	}
	else if(drx==rx && dry==ry) // the node is heading toward the same region
	{
		//this may not be necessary
		if(speed < 0)
			speed = speed * (-1);

		double distance = sqrt((destX-x)*(destX-x)+(destY-y)*(destY-y))/speed;//distance in terms of traveling time left

		if(distance < zeroDistance_) //if the node has actually reached the destination, wait for the next setdest command
		{
			if(status_ == MOVING)
			{
				status_ = STOPPED;

				if(LOG_ENABLED)
				{

					char * str = (char *) malloc(MSG_STRING_SIZE);
					sprintf(str,"(%.2f.%.2f),(%d,%d)", x, y, regionX_, regionY_);
					log(CODE_MOVE,"STOPPED",str);
				}

			}



			//wait_time = slowInterval_;
		}
		else
		{
			status_= MOVING;
			wait_time = distance+zeroDistance_;
			//join_timer_.resched(wait_time);
			if(LOG_ENABLED)
			{
				char * str = (char *) malloc(MSG_STRING_SIZE);
				sprintf(str,"%.2f,%.2f,(%d.%d),>,%.2f,%.2f,(%d.%d),%.2f,(%.2f.%.2f)", x, y, rx, ry, destX,destY, drx, dry, speed, speedX, speedY);
				log(CODE_MOVE,"MOVING",str);
			}
		}

		time_to_leave_ = UNKNOWN;////set to infinity

		log_info(CODE_MOVE,"LASTREGION",(double) time_to_leave_);
	}
	else // the node is heading toward a differnt region
	{
		status_ = MOVING;
		wait_time = getArrivalTime(x, y, speedX, speedY);
		join_timer_.resched(wait_time);
		if(LOG_ENABLED)
		{
			char * str = (char *) malloc(MSG_STRING_SIZE);
			sprintf(str,"%.2f,%.2f,(%d.%d),>,%.2f,%.2f,(%d.%d),%.2f,(%.2f.%.2f)", x, y, rx, ry, destX,destY, drx, dry, speed, speedX, speedY);
			log(CODE_MOVE,"MOVING",str);
		}

		time_to_leave_ = Scheduler::instance().clock()+wait_time;////set to wait time
	}
	//log_info(CODE_MOVE, "DONE", "location checking is done");

	return wait_time;
}


//calculate the delay for next REQUEST message
//factor to consider
//1. time_to_leave_: time the node leaves the region
//2. claim_perid: time to wait for response
//3. state_synced: whether the node has already has its state synchronized with the previous leader
//4. leader_start_: -1 unknown, the actual time if itself is a leader or the last time the state is synchronized.

double JoinAgent::get_delay()
{
	double current_time =  Scheduler::instance().clock();

	if(state_synced_ == TRUE)
	{
		if(time_to_leave_ < 0 )//stable node
			return max_delay_*((rand()%1000)/1000.0);
		else if(time_to_leave_-current_time < claim_period_ )//hopeless
			return claim_period_+2*max_delay_+max_delay_*((rand()%1000)/1000.0);
		else if(time_to_leave_-current_time < 2)//too short lived
			return 2*max_delay_+max_delay_*((rand()%1000)/1000.0);
		else
			return max_delay_+max_delay_*((rand()%1000)/1000.0);
	}
	else//state not synched
	{
		return claim_period_ + 3*max_delay_+2*max_delay_*((rand()%1000)/1000.0);
	}
}

/**
 ** Supporting various otcl commands on this agent.
 **
 **/

int JoinAgent::command(int argc, const char*const* argv)
{
	TclObject * obj;
	if (argc == 2)
	{
		if (strcmp(argv[1], "send") == 0) //send a unicast message
		{
			// Create a new packet
			Packet* pkt = allocpkt();

			// Access the Ping header for the new packet:
			hdr_join* hdr = hdr_join::access(pkt);

			// Set the 'ret' field to 0, so the receiving node
			// knows that it has to generate an echo packet
			hdr->seq = seq_++;
			// Store the current time in the 'send_time' field

			hdr->send_time = Scheduler::instance().clock();
			// Send the packet

			send(pkt, 0);
			// return TCL_OK, so the calling function knows that
			// the command has been processed
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "start-WL-brdcast") == 0)// send a broadcast message
		{
			Packet* pkt = allocpkt();
			hdr_ip* iph = HDR_IP(pkt);
			hdr_join* join_hdr = hdr_join::access(pkt);
			hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);


			cmn_hdr->ptype() = PT_REGION;
			cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
			cmn_hdr->next_hop_ = IP_BROADCAST;

			iph->saddr() = Agent::addr();
			iph->daddr() = 0; //broadcasting address, should be -1
			iph->dport() = MY_PORT_;
			//iph->sport() = 123;
			iph->ttl() = 1;

			join_hdr->type = 0; //leader request
			join_hdr->send_time = Scheduler::instance().clock();

			join_hdr->regionX = regionX_; //sender's region X
			join_hdr->regionY = regionY_; //sender's region Y

			join_hdr->src = nodeID_; //node id
			join_hdr->dst = -1; //all nodes

			join_hdr->seq = seq_++; //sender's sequence number
			join_hdr->answer = 0; // 0 no, 1 yes


			if(LOG_ENABLED)
				log(CODE_JOIN, "SEND", join_hdr->toString());
			//Scheduler::instance().schedule(ll,pkt,0.0);
			send(pkt, (Handler*) 0);//send the packet out
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "start") == 0)
		{
			//printf("node start");
			timeout(0);
			return (TCL_OK);
		}
		else if(strcmp(argv[1],"check-location") == 0)//setdest command called by the tcl script. could be used to start or stop a node's motion
		{
			//if(LOG_ENABLED)
			//	log_info(CODE_MOVE,"SETDEST", "setdest called by tcl script");
			check_location();

			return (TCL_OK);
		}
	}
	else if(argc == 3)
	{
		//set-ll not used anymore.
    	if (strcmp(argv[1], "set-ll") == 0)
    	{
      		if( (obj = TclObject::lookup(argv[2])) == 0)
      		{
        		fprintf(stderr, "Agent/Join(set-ll): %s lookup of %s failed\n", argv[1], argv[2]);
        		return (TCL_ERROR);
      		}
      		ll = (NsObject*) obj;
      		return (TCL_OK);
		}
    	else if (strcmp(argv[1], "set-traceFileName") == 0)
    	{
			filename = (char*)malloc(100);
			strcpy(filename, argv[2]);




			/* Open the file if it hasn't been opened yet*/
			if(logfile->fp == 0)
			{
				//printf("fp is zero");
				logfile->createFilePointer(filename);
				//logfile->createFilePointer((char*)argv[2]);

				logfile->init();// = PTHREAD_MUTEX_INITIALIZER;
			}

      		return (TCL_OK);
		}
    	else if (strcmp(argv[1], "add-app-port") == 0)//add a port that the service report leader and movement info to
    	{
			int temp_port;
			sscanf(argv[2],"%d", &temp_port);
			if(temp_port > 0 && temp_port < 65536)//valid port
			{
				app_port[num_apps]=temp_port;
				num_apps = num_apps + 1;
			}

      		return (TCL_OK);
		}
  }

  // If the command hasn't been processed by JoinAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}

void JoinAgent::send_left_broadcast(int msgType, int version, int parking_spots, int old_x, int old_y, int answer)
{
	 	Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_join* join_hdr = hdr_join::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

		cmn_hdr->ptype() = PT_REGION;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = IP_BROADCAST;

		iph->saddr() = Agent::addr();
		iph->daddr() = -1; //broadcasting address, should be -1
		iph->dport() = MY_PORT_;
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vnhdr->type = JOIN_MSG;
		vnhdr->subtype = msgType;

		join_hdr->type = msgType; //leader request
		join_hdr->send_time = Scheduler::instance().clock();
		join_hdr->ldr_start = leader_start_;
		join_hdr->time_to_leave = time_to_leave_;

		last_sent_ = join_hdr->send_time;

		join_hdr->regionX = regionX_; //sender's region X
		join_hdr->regionY = regionY_; //sender's region Y
	
		join_hdr->old_x = old_x; //sender's region X
		join_hdr->old_y = old_y; //sender's region Y
	
		join_hdr->version = version; // NIKET
		join_hdr->parking_spots = parking_spots; // NIKET

		join_hdr->src = nodeID_; //node id
		join_hdr->dst = -1; //all nodes

		join_hdr->seq = ++seq_;//sender's request sequence number
		join_hdr->answer = answer; // 0 no, 1 yes


		if(LOG_ENABLED)
			log(CODE_JOIN, "SEND", join_hdr->toString());
		send(pkt, (Handler*) 0);
}


/**
 ** Send a broadcast leader election message
 ** msgType: int message type
 **/

void JoinAgent::send_broadcast(int msgType, int answer)
{
	 	Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_join* join_hdr = hdr_join::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

		cmn_hdr->ptype() = PT_REGION;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = IP_BROADCAST;

		iph->saddr() = Agent::addr();
		iph->daddr() = -1; //broadcasting address, should be -1
		iph->dport() = MY_PORT_;
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vnhdr->type = JOIN_MSG;
		vnhdr->subtype = msgType;

		join_hdr->type = msgType; //leader request
		join_hdr->send_time = Scheduler::instance().clock();
		join_hdr->ldr_start = leader_start_;
		join_hdr->time_to_leave = time_to_leave_;

		last_sent_ = join_hdr->send_time;

		join_hdr->regionX = regionX_; //sender's region X
		join_hdr->regionY = regionY_; //sender's region Y

		join_hdr->src = nodeID_; //node id
		join_hdr->dst = -1; //all nodes

		join_hdr->seq = ++seq_;//sender's request sequence number
		join_hdr->answer = answer; // 0 no, 1 yes


		if(LOG_ENABLED)
			log(CODE_JOIN, "SEND", join_hdr->toString());
		send(pkt, (Handler*) 0);
}

void JoinAgent::send_left_unicast(int msgType, int dest, int version, int old_x, int old_y, int seq)
{
		Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_join* join_hdr = hdr_join::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

		cmn_hdr->ptype() = PT_REGION;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = dest;

		iph->saddr() = Agent::addr();
		iph->daddr() = dest; //broadcasting address, should be -1
		iph->dport() = MY_PORT_;
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vnhdr->type = JOIN_MSG;
		vnhdr->subtype = msgType;

		join_hdr->type = msgType; //leader request
		join_hdr->send_time = Scheduler::instance().clock();
		join_hdr->ldr_start = leader_start_;
		join_hdr->time_to_leave = time_to_leave_;

		//last_sent_ = join_hdr->send_time;

		join_hdr->regionX = regionX_; //sender's region X
		join_hdr->regionY = regionY_; //sender's region Y

		join_hdr->src = nodeID_; //node id
		join_hdr->dst = dest; //all nodes

		join_hdr->old_x = old_x; //sender's region X
		join_hdr->old_y = old_y; //sender's region Y
	
		join_hdr->version = version; 

		join_hdr->seq = seq; //sender's request sequence number

		if(LOG_ENABLED)
			log(CODE_JOIN, "SEND", join_hdr->toString());
		//Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, 0);
}


/**
 ** Send a uni-cast leader election message
 ** msgType: int message type
 ** dest: int destination node id
 ** answer: 0 no, 1 yes
 ** seq: int sequence number
 **/

void JoinAgent::send_unicast(int msgType, int dest, int answer, int seq)
{
		Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_join* join_hdr = hdr_join::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

		cmn_hdr->ptype() = PT_REGION;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = dest;

		iph->saddr() = Agent::addr();
		iph->daddr() = dest; //broadcasting address, should be -1
		iph->dport() = MY_PORT_;
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vnhdr->type = JOIN_MSG;
		vnhdr->subtype = msgType;

		join_hdr->type = msgType; //leader request
		join_hdr->send_time = Scheduler::instance().clock();
		join_hdr->ldr_start = leader_start_;
		join_hdr->time_to_leave = time_to_leave_;

		//last_sent_ = join_hdr->send_time;

		join_hdr->regionX = regionX_; //sender's region X
		join_hdr->regionY = regionY_; //sender's region Y

		join_hdr->src = nodeID_; //node id
		join_hdr->dst = dest; //all nodes

		join_hdr->seq = seq; //sender's request sequence number
		join_hdr->answer = answer; // 0 no, 1 yes


		if(LOG_ENABLED)
			log(CODE_JOIN, "SEND", join_hdr->toString());
		//Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, 0);
}

/**
 ** Send a uni-cast vns message to a destination on a specific port
 ** Not used so far
 **/

void JoinAgent::send_unicast_to(int msgType, int dest, int port)
{
		Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		hdr_vns* vns_hdr = hdr_vns::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);


		cmn_hdr->ptype() = PT_REGION;
		//cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = dest;

		iph->saddr() = Agent::addr();
		iph->daddr() = dest;
		iph->dport() = port;
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vns_hdr->type = msgType; //leader request
		vns_hdr->send_time = Scheduler::instance().clock();

		vns_hdr->regionX = regionX_; //sender's region X
		vns_hdr->regionY = regionY_; //sender's region Y


		if(LOG_ENABLED)
			log(CODE_JOIN, "SENDTOPORT", vns_hdr->toString());
		send(pkt, 0);
}

/**
 ** Send a loopback vns message to the VNS agents attached to the same node
 ** msgType: int message type
 **/

void JoinAgent::copy_loopback(Packet* pkt_in, int port)
{

	//hdr_ip  * iph = HDR_IP(pkt_in);//ip header
	hdr_join * join_hdr = hdr_join::access(pkt_in);//old vns header, only used in join agent now

	Packet  * pkt_out = allocpkt();	//create a packet for sending
	hdr_ip  * iph_out = HDR_IP(pkt_out);
	//hdr_vns * vns_hdr_out = hdr_vns::access(pkt_out);
	hdr_cmn * cmn_hdr_out = hdr_cmn::access(pkt_out);
	hdr_vncommon * vncmn_hdr_out = hdr_vncommon::access(pkt_out);

	cmn_hdr_out->ptype() = PT_VNCOMMON;
	cmn_hdr_out->size() = size_ + IP_HDR_LEN; //
	cmn_hdr_out->next_hop_ = nodeID_;

	iph_out->saddr() = Agent::addr();
	iph_out->daddr() = Agent::addr(); //local address
	iph_out->dport() = port;
	iph_out->sport() = MY_PORT_; //no need
	iph_out->ttl() = 1;

	vncmn_hdr_out->type = JOIN_MSG;
	vncmn_hdr_out->subtype = join_hdr->type;
	vncmn_hdr_out->regionX = join_hdr->regionX;
	vncmn_hdr_out->regionY = join_hdr->regionY;
	vncmn_hdr_out->send_time = join_hdr->send_time;

	//the fields below are to be removed.
	/*vns_hdr_out->message_class = vns_hdr->message_class; //internal loopback message
	vns_hdr_out->type = vns_hdr->type; //leader request
	vns_hdr_out->send_time = vns_hdr->send_time;


	vns_hdr_out->regionX = vns_hdr->regionX; //sender's region X
	vns_hdr_out->regionY = vns_hdr->regionY; //sender's region Y

	vns_hdr_out->server_regionX = vns_hdr->server_regionX;*/

	//log(CODE_JOIN, "LOOPBACKSEND", vncmn_hdr_out->toString());
	send(pkt_out, 0);

}

/**
 ** Send a loopback vns message to the agents attached to the same node
 ** msgType: int message type
 **/

void JoinAgent::send_loopback(int msgType)
{
	Packet  * pkt;
	hdr_ip  * iph;
	hdr_vns * vns_hdr;
	hdr_cmn * cmn_hdr;
	hdr_vncommon * vncmn_hdr;

	for(int i=0; i<num_apps; i++)
	{
		pkt = allocpkt();
		iph = HDR_IP(pkt);
		vns_hdr = hdr_vns::access(pkt);
		cmn_hdr = hdr_cmn::access(pkt);
		vncmn_hdr = hdr_vncommon::access(pkt);

		cmn_hdr->ptype() = PT_VNCOMMON;
		cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
		cmn_hdr->next_hop_ = nodeID_;

		iph->saddr() = Agent::addr();
		iph->daddr() = Agent::addr(); //local address
		//iph->dport() = app_port;
		iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		if(msgType == NEWREGION)
		{
			vncmn_hdr->type = REGION_MSG;
			vncmn_hdr->subtype = NEWREGION;
		}
		else if(msgType == NEWLEADER)
		{
			vncmn_hdr->type = LEADER_MSG;
			vncmn_hdr->subtype = NEWLEADER;
			vncmn_hdr->dst = leader_;
		}
		else if(msgType == NONLEADER)
		{
			vncmn_hdr->type = LEADER_MSG;
			vncmn_hdr->subtype = NONLEADER;
			vncmn_hdr->dst = leader_;
		}
		else if(msgType == OLDLEADER)
		{
			vncmn_hdr->type = LEADER_MSG;
			vncmn_hdr->subtype = OLDLEADER;
			vncmn_hdr->dst = leader_;
		}

		vncmn_hdr->regionX = regionX_;
		vncmn_hdr->regionY = regionY_;
		vncmn_hdr->send_time = Scheduler::instance().clock();

		//the fields below are to be removed.
		vns_hdr->message_class = INTERNAL_MESSAGE; //internal loopback message
		vns_hdr->type = msgType; //leader request
		vns_hdr->send_time = Scheduler::instance().clock();
		//last_sent_ = join_hdr->send_time;

		vns_hdr->regionX = regionX_; //sender's region X
		vns_hdr->regionY = regionY_; //sender's region Y

		vns_hdr->server_regionX = leader_;
		//Scheduler::instance().schedule(ll,pkt,0.0);

		iph->dport() = app_port[i];
		//log_info(CODE_JOIN, "LOOPBACKSENTTO", app_port[i]);
		send(pkt, 0);
	}


}

/* get the id of the region where the node is located
 * double x: the x position of the node
 * double y: the y position of the node
 * int * regionX: the x part of the region id, a pointer
 * int * regionY: the y part of the region id, a pointer
 * return 0: error
 * return 1: normal
 */
int JoinAgent::getRegion(double x, double y, int * regionX, int * regionY)
{

	double xUnit = maxX_/columns_; //size of a region X
	double yUnit = maxY_/rows_;    //size of a region Y

	if( x<0 || x>maxX_)
	{
		printf("X position out of bound\n");//warning.
		return 0;
	}
	if( y<0 || y>maxY_)
	{
		printf("Y position out of bound\n");
		return 0;
	}

	int rx = (int)(x/xUnit);
	int ry = (int)(y/yUnit);

	if( rx == columns_)
	{
		printf("X position reached bound\n");
		rx = columns_-1;
	}
	if( ry == rows_)
	{
		printf("Y position reached bound\n");
		ry = rows_-1;
	}

	* regionX = rx;
	* regionY = ry;

	return 1;
}

/*
 * 		Get the time expected for the node to enter the next region
 * 		double x: the x position of the node
 * 		double y: the y position of the node
 * 		double speedX: the x direction moving speed
 * 		double speedY: the y direction moving speed
 * 		return double: the time it takes to reach the next region
 */
double JoinAgent::getArrivalTime(double x, double y, double speedX, double speedY)
{
	double xUnit = maxX_/columns_; //size of a region X
	double yUnit = maxY_/rows_;    //size of a region Y
	int regionHeadingX, regionHeadingY; // the region id to go
	double timeX, timeY; //the time spent on each direction

	if(speedX == 0)//check speedY only
	{
		if(speedY == 0)//error case
		{
			return slowInterval_;
		}

		if(speedY < 0)//moving downwards
		{
			regionHeadingY = regionY_;
			timeY = (regionHeadingY*yUnit-y)/speedY;
			return timeY + zeroDistance_;
		}
		else //moving upwards
		{
			regionHeadingY = regionY_ + 1;
			timeY = (regionHeadingY*yUnit-y)/speedY;
			return timeY + zeroDistance_;
		}
	}
	else if(speedY == 0)//check speedX only
	{
		if(speedX < 0)//moving leftwards
		{
			regionHeadingX= regionX_;
			timeX = (regionHeadingX*xUnit-x)/speedX;
			return timeX + zeroDistance_;
		}
		else //moving rightwards
		{
			regionHeadingX = regionX_ + 1;
			timeX = (regionHeadingX*xUnit-x)/speedX;
			return timeX + zeroDistance_;
		}
	}
	else //check both, use the shorter one
	{
		if(speedX < 0)//moving leftwards
		{
			regionHeadingX= regionX_;
		}
		else //moving rightwards
		{
			regionHeadingX = regionX_ + 1;
		}
		timeX = (regionHeadingX*xUnit-x)/speedX+zeroDistance_;

		if(speedY < 0)//moving downwards
		{
			regionHeadingY= regionY_;
		}
		else //moving upwards
		{
			regionHeadingY = regionY_ + 1;
		}
		timeY = (regionHeadingY*yUnit-y)/speedY+zeroDistance_;

		if(timeX<timeY)
		{
			return timeX;
		}
		else
		{
			return timeY;
		}
	}
}

/*
 *
 * Upon region change, calculate the neighbor list and initialize the status of each neighbor
 *
 */
void JoinAgent::setNeighbors()
{
	//valid region id
	if(regionX_ >=0 && regionX_ < columns_ && regionY_ >= 0 && regionY_ < rows_)
	{
		for(int i=0; i<NUM_NEIGHBORS; i++)
		{
			switch(i)
			{
				case 0: neighbors[i][0] = regionX_ -1; neighbors[i][1] = regionY_+1; break;
				case 1: neighbors[i][0] = regionX_;    neighbors[i][1] = regionY_+1; break;
				case 2: neighbors[i][0] = regionX_ +1; neighbors[i][1] = regionY_+1; break;
				case 3: neighbors[i][0] = regionX_ +1; neighbors[i][1] = regionY_;   break;
				case 4: neighbors[i][0] = regionX_ +1; neighbors[i][1] = regionY_-1; break;
				case 5: neighbors[i][0] = regionX_;    neighbors[i][1] = regionY_-1; break;
				case 6: neighbors[i][0] = regionX_ -1; neighbors[i][1] = regionY_-1; break;
				case 7: neighbors[i][0] = regionX_ -1; neighbors[i][1] = regionY_; break;
				default:neighbors[i][0] = -1; neighbors[i][1] = -1;
			}
			//set invalid neighbor flags to -1
			if(neighbors[i][0] <0 || neighbors[i][0] >= columns_ || neighbors[i][1] <0 || neighbors[i][1] >= rows_)
			{
				neighbor_flags[i] = INVALID;
				neighbor_timers[i] = 0;
			}
			else
			{
				neighbor_flags[i] = ACTIVE; //set to ACTIVE here, we may want to set all the valid neighbors to inactive at first.
				neighbor_timers[i] = Scheduler::instance().clock() + 3*beat_period_; //once a neighbor is active, if no heartbeat is heard for over 3 heartbeat periods, the neighbor is treated as inactive.
			}
		}
	}
	else//set all neighbors to invalid
	{
		for(int i=0; i<NUM_NEIGHBORS; i++)
		{
			neighbors[i][0] = -1;
			neighbors[i][1] = -1;
			neighbor_flags[i] = INVALID;
			neighbor_timers[i] = 0;
		}
	}
}




