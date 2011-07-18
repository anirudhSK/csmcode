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
 * vnserver.cc: Code for a VNS Agent Class for the virtual node system
 *       Supporting the server layer of leaders and nonleader nodes
 */
#include "vnserver.h"
#include "time.h"
#include <sys/ipc.h>
#include <sys/shm.h>

/*OTCL linkage for the VNSClass */
static class VNSClass : public TclClass {
public:
	VNSClass() : TclClass("Agent/VNSAgent") {}
	TclObject* create(int, const char*const*) {
		return (new VNSAgent());
	}
} class_vnserver;

/* Agent/VNS constructor */
VNSAgent::VNSAgent() : Agent(PT_VNS),syn_timer_(this),queue_timer_(this),server_timer_(this),tcl(Tcl::instance())
{
	bind("port_number_", &MY_PORT_);
	bind("client_port_", &client_port_);
	bind("join_port_", &join_port_);
	bind("columns_", &columns_);
	bind("rows_", &rows_);
	bind("maxX_", &maxX_);
	bind("maxY_", &maxY_);

	bind("nodeID_",&nodeID_);

	bind("leader_", &leader_);
	bind("leader_status_", &leader_status_);
	bind("state_", &state_);

	//state synchronization related parameters
	bind("syn_wait_", &syn_wait_);
	bind("syn_interval_",&syn_interval_);
	bind("max_retries_",&max_retries_);

	bind("sync_enabled_", &sync_enabled_);
	bind("sync_delay_",&sync_delay_);

	bind("total_ordering_enabled_",&total_ordering_enabled_);
	bind("total_ordering_mode_",&total_ordering_mode_);
	bind("first_node_hold_off_",&first_node_hold_off_);

	bind("ordering_delay_", &ordering_delay_);

	bind("send_wait_",&send_wait_);;

	bind("packetSize_", &size_);

	bind("neighbor_timer_", &neighbor_timer_);


	app_code = "UNKNOWN";

	regionX_ = -1;
	regionY_ = -1;

	retries_=0;
	seq_ = 0;

	lastTime_ = -1;
	next_sync_ = 0;

	first_node_ = TRUE;
	first_node_deadline_ = 5;

	input_queue = (struct PacketQ *) malloc(sizeof(struct PacketQ *));
	input_queue->init();

	queue = (struct PacketQ *) malloc(sizeof(struct PacketQ *));
	queue->init();

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
	if ((logfile = (LogFile *)shmat(shmid, NULL, 0)) == (LogFile *) -1) {
		perror("shmat");
		exit(1);
	}

	logfile->fp = 0;

	//srand(time(NULL)+nodeID_);
	//srand ( time(NULL) );


}

void VNSAgent::check_neighbor_regions(Packet* pkt)
{
	return;
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

  	double timenow = Scheduler::instance().clock();

	for(int i=0; i<NUM_NEIGHBORS; i++)
	{
		//a virtual node message is found from a neighbor region
		if(	neighbor_flags[i] == ACTIVE && timenow > neighbor_timeouts[i])
		{
		//	neighbor_flags[i] = INACTIVE;
			//log_info(CODE_VNS, "NEIGHBORDEAD", i);
		}

		//check if the message comes from a neighbor region
		if(vnhdr->regionX == neighbors[i][0] && vnhdr->regionY == neighbors[i][1])
		{
			//a message from a virtual node, syn-ack, non client appl message or heartbeat message or leaderreply no way
			if((vnhdr->type == SYNC_MSG && vnhdr->subtype == SYN_ACK) || (vnhdr->type == APPL_MSG && vnhdr->subtype != CLIENT_MESSAGE) || (vnhdr->type == JOIN_MSG && vnhdr->subtype == HEART_BEAT) )
			{


				//log_info(CODE_VNS, "UPDATENEIGHBOR", i);
				//log_info(CODE_VNS, "UPDATENEIGHBOR with", vnhdr->type);
				//log_info(CODE_VNS, "UPDATENEIGHBOR with", vnhdr->subtype);

				//if(vnhdr->type == JOIN_MSG && vnhdr->subtype == HEART_BEAT)
				//{
				//	log_info(CODE_VNS, "HBRECV", "updating neighbor list");
				//}



				if( neighbor_flags[i] == INACTIVE )
				{
					neighbor_flags[i] = ACTIVE; //set to ACTIVE here, we may want to set all the valid neighbors to inactive at first.
					neighbor_timeouts[i] = Scheduler::instance().clock()+neighbor_timer_;
				}
				else if( neighbor_flags[i] == ACTIVE )
				{
					neighbor_timeouts[i] = Scheduler::instance().clock()+neighbor_timer_;
				}
				else
				{
					//do nothing.//could be an invalid region
				}

			}

		}

	}
}

/*
 * The method processing every packet received by the agent
 *
 */

void VNSAgent::recv(Packet* pkt, Handler*)
{
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

	double timenow = Scheduler::instance().clock();

	check_neighbor_regions(pkt);

	if(first_node_ == TRUE && vnhdr->send_time > first_node_deadline_)
	{
		assert(1 == 0);	
		first_node_ = FALSE;
	}

	if(vnhdr->type == LEADER_MSG)
	{
		if(vnhdr->subtype == NEWLEADER)
		{
			assert(regionX_ == vnhdr->regionX && regionY_ == vnhdr->regionY); //same region message
			leader_status_ = LEADER;
			leader_ = nodeID_;
			if(LOG_ENABLED)
			{
				log(CODE_VNS, "RECV_LEADER", vnhdr->toString());
				if(first_node_ == TRUE)
					log_info(CODE_VNS, "FIRSTNODE", (double)first_node_);
				//log(CODE_VNS, "RECV_LEADER", hdr->toString());
			}

			if(first_node_ == TRUE)
			{
				first_node_deadline_ = timenow + 5;
			}
			if(state_ == NEWNODE)
			{
				regionX_ = vnhdr->regionX;
				regionY_ = vnhdr->regionY;
				state_=SERVER;
				server_init();
			}
			if(state_ == BACKUP)
			{
				state_=SERVER;
				server_init();
			 	queue->empty(); //dump everything in the queue
			}
			else 
			{
				//Do nothing. 	
			}
		}
		else if(vnhdr->subtype == NONLEADER)
		{
			assert(regionX_ == vnhdr->regionX && regionY_ == vnhdr->regionY); //same region message
			leader_status_ = NON_LEADER;
			leader_ = vnhdr->dst;

			if(first_node_ == TRUE)
			{
				first_node_deadline_ = timenow + 5;
			}
			if(state_ == NEWNODE)
			{
				if(sync_enabled_ == 1 || sync_enabled_ == 2)
				{
					log_info(CODE_VNS,"MOV-SYNC","");
					server_init();
					state_=SYNC;
					sync();
				}
				else
				{	// I'll do nothing.
					// state_=BACKUP;
				}
			}
			else
			{
				//Do nothing
			}
		}
		else if(vnhdr->subtype == OLDLEADER)
		{
			state_= OLD_INACTIVE;
			leader_status_ = OLD_LEADER;
		}	
	}
	else if(vnhdr->type == REGION_MSG)
	{
		if(vnhdr->subtype == NEWREGION)
		{
			regionX_ = vnhdr->regionX;
			regionY_ = vnhdr->regionY;
			if(LOG_ENABLED)
			{
				log(CODE_VNS, "RECV_NEWREGION", vnhdr->toString());
			}
			//reset all states and wait for the leader information
			setNeighbors();
			reset_states();
			first_node_deadline_ = timenow + 5;
		}
	}
	else if(vnhdr->type == SYNC_MSG)
	{
		if(vnhdr->subtype == SYN_REQUEST)
		{
			if(vnhdr->regionX == regionX_ && vnhdr->regionY == regionY_) //the packet come from the same region
			{
				if(LOG_ENABLED)
					log(CODE_VNS, "RECV", vnhdr->toString());
				if(state_ == SERVER)
				{

					if( timenow > next_sync_)
					{
						next_sync_ = timenow + sync_delay_;
						server_broadcast(SYN_ACK, vnhdr->dst, getStateSize(), (u_char *) getState());
						log_info(CODE_VNS, "SENDSTATE", getStateSize());
					}
					else
					{
						log_info(CODE_VNS,"SYNC-HOLDOFF", next_sync_);
					}
				}
				else if(state_ == SYNC || state_ == MSGSYNC)
				{
					//postpone my own sync message a little
					double waiting_ = syn_wait_+2*syn_wait_*((rand()%1000)/1000.0);
					syn_timer_.resched( waiting_ );//wait a short period of time and send another syn_request if no response
				}
			}
		}
		else if(vnhdr->subtype == SYN_ACK)
		{
			if(vnhdr->regionX == regionX_ && vnhdr->regionY == regionY_) //the packet come from the same region
			{
				if(state_ == SYNC || state_ == MSGSYNC) //use any SYNC to synchronize my state
				{
					if(LOG_ENABLED)
						log(CODE_VNS,"RECV",vnhdr->toString());
					saveState(pkt->accessdata(), pkt->datalen());
					state_ = BACKUP;
					send_loopback(ST_SYNCED, seq_);
					if(first_node_ == TRUE)
					{
						first_node_ = FALSE;
					}
				}
			}
		}
	}
	else if(vnhdr->type == APPL_MSG)//application messages, application layer messages
	{
		if(state_ == SERVER || (state_ == BACKUP && sync_enabled_ == 1))//sync mode 2, no msg sync
		{
			//reject application layer messages coming from non-neighboring regions
			//this is not necessary is higher layer allows the processing of these messages
			if(abs(vnhdr->regionX - regionX_)>1 || abs(vnhdr->regionY -regionY_)>1)
			{
				//log(CODE_VNS,"LONGRECV",vnhdr->toString());
				Packet::free(pkt);
				return;
			}
			if(first_node_hold_off_ == TRUE && first_node_ == TRUE)
			{
				//log(CODE_VNS,"FIRSTNODE",vnhdr->toString());
				Packet::free(pkt);
				return;
			}
			//put packet in total ordered input queue
			if(total_ordering_enabled_ == TRUE)
			{
				//log_info(CODE_VNS,"TOTAL","ordering");
				sortPacket(pkt);
				return;
			}
			else
			{
				assert ( 1 == 0);	
				consistencyManager(pkt);//no sorting.
				return;
			}
		}
	}
	Packet::free(pkt);
	//log_info(CODE_VNS, "PFREE", "packet freed");
}

//new version with common vn header
//insert application messages into the total ordered input queue
//sort the packets with send_time
void VNSAgent::sortPacket(Packet * pkt)
{
	int count = 1;
	// Access the vns message header for the received packet:
  	hdr_vncommon * hdr = hdr_vncommon::access(pkt);

  	int size = input_queue->size;

  	double deadline = Scheduler::instance().clock()+ordering_delay_;

  	PacketUnit * insertedUnit;

  	if(size == 0)//first packet
  	{
		insertedUnit = input_queue->enqueue(pkt);
		insertedUnit->deadline = deadline;

		//if(LOG_ENABLED)
		//{
		//	log(CODE_VNS,"BUFFERED_START",hdr->toString());
		//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
		//}
		queue_timer_.resched(ordering_delay_);//wait for a ordering_delay_ period to reduce packet misorder
		next_to_expire=hdr->send_time;
	}
	else//queue not empty
	{
		struct PacketUnit * current_unit;
		hdr_vncommon * current_hdr;

		if(total_ordering_mode_ == FROM_HEAD)
		{
			current_unit = input_queue->head;

			while(1)
			{
				current_hdr = hdr_vncommon::access(current_unit->current);
				if(current_hdr->send_time > hdr->send_time) //the packet comes earlier than the current packet
				{

					insertedUnit = input_queue->insertBefore(current_unit, pkt);
					insertedUnit->deadline = deadline;

					//if(LOG_ENABLED)
					//{
					//	log(CODE_VNS,"BUFFERED_BEFORE",hdr->toString());
					//	log_info(CODE_VNS,"BUFFER_POS",(double)count);
					//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
					//}
					break;
				}
				else//the packet comes no ealier than the current one
				{
					if(current_unit->next !=0)//not the last one in the queue
					{
						current_unit = current_unit->next;
						count ++;
					}
					else
					{

						insertedUnit = input_queue->enqueue(pkt);
						insertedUnit->deadline = deadline;
						//if(LOG_ENABLED)
						//{
						//	log(CODE_VNS,"BUFFERED_END",hdr->toString());
						//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
						//}
						break;
					}
				}
			}
		}
		else
		{
			current_unit = input_queue->tail;

			while(1)
			{
				current_hdr = hdr_vncommon::access(current_unit->current);
				if(current_hdr->send_time <= hdr->send_time) //the packet comes no earlier than the current packet
				{

					insertedUnit = input_queue->insertBehind(current_unit, pkt);
					insertedUnit->deadline = deadline;

					//if(LOG_ENABLED)
					//{
					//	log(CODE_VNS,"BUFFERED_BEHIND",hdr->toString());
					//	log_info(CODE_VNS,"BUFFER_POS",(double)count);
					//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
					//}
					break;
				}
				else//the packet comes no ealier than the current one
				{
					if(current_unit->previous !=0)//not the first one in the queue
					{
						current_unit = current_unit->previous;
						count ++;
					}
					else//the packet comes earlier than the head
					{

						insertedUnit = input_queue->insertBefore(current_unit, pkt);
						insertedUnit->deadline = deadline;
						//if(LOG_ENABLED)
						//{
						//	log(CODE_VNS,"BUFFERED_HEAD",hdr->toString());
						//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
						//}
						break;
					}
				}
			}
		}
	}
}


//new version with vn common header
//receive a packet from the ordered input packet queue
//check against sending queue if the packet is not a local client message
//do synchronization for local server messages
void VNSAgent::consistencyManager(Packet * pkt)
{
	// Access the vns message header for the received packet:
  	hdr_vncommon * hdr = hdr_vncommon::access(pkt);
  	int handled = FALSE; //not dealt with by vns

	//if a server message is received, check it against the sending queue to see
	//if a match can be found. If so, remove the corresponding messages from the queue.
	//Otherwise, start a synchronization procedure.
	if(hdr->subtype == SERVER_MESSAGE || hdr->subtype == PARKING_REQUEST || hdr->subtype == PARKING_REPLY || hdr->subtype == WRITE_UPDATE || hdr->subtype == WRITE_UPDATE_REPLY) 
	{
	  if(hdr->regionX == regionX_ && hdr->regionY == regionY_) //message comes from the same region, need to be handled by vns
	  {
		  handled = TRUE; //dealt with by vns.
		  if(state_ == BACKUP && sync_enabled_ == 1)
		  {

				/*if(hdr->hash == getStateHash())//a matching state.
				{
					log_info(CODE_VNS,"MATCHING","");
					//log_info(CODE_VNS,"SERVER-HASH",hdr->hash);
					//log_info(CODE_VNS,"MY-HASH",getStateHash());

				}*/
				//if(LOG_ENABLED)
				//	log(CODE_VNS,"RECV_SERVER_MESSAGE", hdr->toString());

				if(lookup(queue, pkt)==0)//not found
				{

					log_info(CODE_VNS,"MSG-SYNC","");
					/*if(hdr->hash != getStateHash())//check to see if the hash value of the state matches or not.
					{
						log_info(CODE_VNS,"HASH-MISMATCH","");

						log_info(CODE_VNS,"SERVER-HASH",hdr->hash);
						log_info(CODE_VNS,"MY-HASH",getStateHash());

					}*/
					state_ = MSGSYNC;
					sync();
				}
		  }
		  else if(state_ == BACKUP && sync_enabled_ == 2)
		  {
				//if(LOG_ENABLED)
				//	log(CODE_VNS,"RECV_SERVER_MESSAGE", hdr->toString());
				if(lookup(queue, pkt)==0)//not found
				{
					//do nothing, in mode 2

					//log_info(CODE_VNS,"MSG-SYNC","");
					//state_ = MSGSYNC;
					//sync();
				}
		  }
	  }
	}
	//if a forwarded server message is received, check it against the sending queue to see
	//if a match can be found. If so, remove the corresponding messages from the queue.
	//Otherwise, do nothing. (no state synchronization needed)
	else if(hdr->subtype == FORWARDED_SERVER_MESSAGE)
	{
	  if(hdr->regionX == regionX_ && hdr->regionY == regionY_) //message comes from the same region, need to be handled by vns
	  {
		  handled = TRUE; //dealt with by vns.
		  if(state_ == BACKUP && (sync_enabled_ == 1 || sync_enabled_==2))
		  {
				//if(LOG_ENABLED)
				//	log(CODE_VNS,"RECV_FORWARDED_SERVER_MESSAGE", hdr->toString());
				if(lookup(queue, pkt)==0)//not found
				{
					//if(LOG_ENABLED)
					//	log_info(CODE_VNS,"SYNC","no match found, sync");
				}
		  }
	  }
	}
	//if a forwarded client message is received, check it against the sending queue to see
	//if a match can be found. If so, remove the corresponding messages from the queue.
	//Otherwise, do nothing. (no state synchronization needed)
	else if(hdr->subtype == FORWARDED_CLIENT_MESSAGE)
	{
	  if(hdr->regionX == regionX_ && hdr->regionY == regionY_) //message comes from the same region, need to be handled by vns
	  {
		  handled = TRUE; //dealt with by vns.
		  if(state_ == BACKUP && hdr->regionX == regionX_ && hdr->regionY == regionY_ && (sync_enabled_ == 1 || sync_enabled_==2))
		  {
				//if(LOG_ENABLED)
				//	log(CODE_VNS,"RECV_FORWARDED_CLIENT_MESSAGE", hdr->toString());
				if(lookup(queue, pkt)==0 )//not found
				{
					//if(LOG_ENABLED)
					//	log_info(CODE_VNS,"SYNC","no match found, sync");
				}
		  }
	  }

	}
	//if a client message is received, do nothing. (no state synchronization needed)
	else if(hdr->subtype == CLIENT_MESSAGE || hdr->subtype == PARKING_REQUEST || hdr->subtype == PARKING_REPLY || hdr->subtype == PARKING_ACK || hdr->subtype == WRITE_UPDATE || hdr->subtype == WRITE_UPDATE_REPLY)
	{
	  //client messages
	  //passed on by vns

	}
	else
	{
		handled = TRUE; //dealt with by vns.
		if(LOG_ENABLED)
			log(CODE_VNS, "WRONG_MSG_CLASS", hdr->toString());
	}
	//log_info(CODE_VNS, "RECV", "unknown message");

	if(handled == FALSE) //dealt with by vns.
	{
		if(state_ == SERVER)
		{
	  	  //handle packets and send responses out
	  	  handle_packet(pkt);
		  send_packets();
		}
		else if(state_ == BACKUP && sync_enabled_ == 1)
	  	{
	  	  //handle packets and keep responses in the queue
	  	  handle_packet(pkt);
		}
		else if(state_ == BACKUP && sync_enabled_ == 2)//still handle packets, but no sync
	  	{
	  	  //handle packets and keep responses in the queue
	  	  handle_packet(pkt);
		}
		else//backup node don't even handle packets
	  	{
	  	  //if(LOG_ENABLED)
	  		//log(CODE_VNS, "IGNORE", hdr->toString());
		}
	}

    Packet::free(pkt);
    //log_info(CODE_VNS, "PFREE", "packet freed");

}

/*
 * Awaken by the syn timer.
 * Check the current syn status to see if anything needs to be done.
 */


void VNSAgent::check_syn_status()
{
	//log_info(CODE_VNS,"SYNC", "checking sync status");
	if(state_==SYNC || state_ == MSGSYNC)
	{
		//if(retries_ < max_retries_)
		//{
			//syncClientID_ = nodeID_; //I am the client now.
			//log_info(CODE_VNS,"SEND_SYNC",retries_);
			seq_=seq_+1;
			server_broadcast(SYN_REQUEST, seq_, 0, (u_char *) 0);
			//server_broadcast(SYN_ACK, seq, getStateSize(), (u_char *) getState());

			retries_++;
			//log_info(CODE_VNS,"SEND","SYN_REQUEST sent");
			syn_timer_.resched(syn_interval_*retries_);
			//log_info(CODE_VNS,"SYNC_RESCHEDULE",syn_interval_);
		//}
		//else//give up on sync attempts, possibly due to congestion, therefore, it is ok to keep quiet until next packet received
		//{
		//	state_ = BACKUP;
		//	retries_ = 0;
		//}
	}
	else
	{
		return;
	}
}

/*
 * Awaken by the ordered input queue timer.
 * Check the current status of the input queue to see if anything needs to be done
 */
void VNSAgent::check_queue_status()
{
	Packet * p;

	while(input_queue->size > 0)
	{
		p = input_queue->dequeue();
		if(p!=0)
		{
			//if(LOG_ENABLED)
			//{
			//	hdr_vns * hdr = hdr_vns::access(p);
			//	log(CODE_VNS,"DEBUFFERED",hdr->toString());
			//	log_info(CODE_VNS,"BUFFER_SIZE",(double)input_queue->size);
			//}
			consistencyManager(p);
			if(input_queue->size != 0)//if there is more packets, check to see if the next one has expired or not.
			{
				//log_info(CODE_VNS,"NEXT_DEADLINE",input_queue->head->deadline);
				if(Scheduler::instance().clock() < input_queue->head->deadline)
				{
					double wait = input_queue->head->deadline - Scheduler::instance().clock();
					queue_timer_.resched(wait);
					break;
				}
			}

		}
	}


}

/*
 * Awaken by the server timer.
 * Check the current status to see if anything needs to be done
 */
void VNSAgent::check_server_status()
{
	Packet * p;

	if(state_ == SERVER)
	{
		if(queue->size !=0)
		{
			sending_state_ = SENDING;
			p = queue->dequeue();
			if(p!=0)
			{
				hdr_vncommon * hdr = hdr_vncommon::access(p);


				switch(hdr->send_type)
				{
					case SEND: if(LOG_ENABLED) log(app_code,"SEND",getAppHeader(p));break;
							   //case SENDLOOPBACK: if(LOG_ENABLED) log(app_code,"SENDLOOPBACK",getAppHeader(p));break;
					case FORWARD:  if(LOG_ENABLED) log(app_code,"FORWARD",getAppHeader(p));break;
					default:;
				}

				assert(leader_status_ == LEADER);
				send(p,0);
			}
			server_timer_.resched(send_wait_);
		}
		else
		{
			//log_info(app_code,"QUEUE","Queue emptied");
			sending_state_ = NOSENDING;
		}
	}

}

//////////////////////////////////////////////////////////////
// Methods to be implemented by specific applications.
//initialize the server states
//to be implemented by subclasses.
void VNSAgent::server_init()
{
	return;
}

/**
  * The packet handler a server needs to implement
 **/
void VNSAgent::handle_packet(Packet * pkt)
{
	return;
}

//evaluate two server packets to see if they are for the same transaction
int VNSAgent::equal(Packet * p1, Packet * p2)
{
	return 1;
}
///
//////////////////////////////////////////////////////////////////

//send the messages in the message buffer, if there is any
void VNSAgent::send_packets()
{
	//log_info(CODE_VNS, "QUEUE", (double)sending_state_);
	if(sending_state_ == SENDING)
	{
		//just wait for the next timeout
		//log_info(CODE_VNS, "QUEUE", "sending ongoing, wait");
	}
	else
	{
		if(queue->size != 0)
		{
			sending_state_ = SENDING;
			//log_info(CODE_VNS, "QUEUE", "sending enabled for the queue");
			server_timer_.resched(send_wait_);
		}
		else
		{
			//log_info(CODE_VNS, "QUEUE", "empty");
		}

	}
}

//synchronizing the states with the server
void VNSAgent::sync()
{
	//srand(Scheduler::instance().clock()+nodeID_);
	retries_ = 0;
	//queue->empty(); //dump everything in the queue
	double waiting_ = syn_wait_+2*syn_wait_*((rand()%1000)/1000.0);
	syn_timer_.resched( waiting_ );//wait a short period of time and send another syn_request if no response

	//log_info(CODE_VNS,"SYNC_TO_GO", waiting_);
}

/**
 ** Supporting various OTCL commands on this agent.
 **
 **/

int VNSAgent::command(int argc, const char*const* argv)
{

	if (argc == 2)
	{
    	if (strcmp(argv[1], "start") == 0)//this is where everything starts from
    	{
			srand(time(NULL)+nodeID_);

			reset_states();//reset everything and wait for REGION message
      		return (TCL_OK);
    	}
	}
	else if(argc == 3)
	{
    	if (strcmp(argv[1], "set-traceFileName") == 0)//set the tracefile name to be used for logging.
    	{
			filename = (char*)malloc(100);
			strcpy(filename, argv[2]);

			/* Open the file if it hasn't been opened yet*/
			if(logfile->fp == 0)
			{
				//printf("fp is zero");
				logfile->createFilePointer(filename);
				logfile->init();
			}

      		return (TCL_OK);
		}
  }

  // If the command hasn't been processed by VNSAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}


//lookup a packet in the packet buffer for a match, using the equal method.
//once a match is found, remove the match from the buffer.
int VNSAgent::lookup(PacketQ * queue, Packet * pkt)
{
	if(queue == 0)
		return 0;

	if(queue->size == 0)
		return 0;

	PacketUnit * packet = queue->head;
	PacketUnit * next;
	//hdr_vns * hdr = hdr_vns::access(pkt);
	while(1)
	{
		next = packet->next;

		if(equal(packet->current, pkt))
		{
			//if(LOG_ENABLED)
			//	log(CODE_VNS, "DEQUEUE", getAppHeader(pkt));

			queue->remove(packet);
			return 1;
		}
		if(next == 0)
			break;
		else
		{
			packet = next;
		}
	}
	return 0;
}



//check a packet queue, remove a packet if it is received a certain period of time ago
void VNSAgent::queue_timeout(PacketQ * queue, double time)
{
	if(queue == 0)
		return;

	if(queue->size == 0)
	{

		return;
	}

	PacketUnit * packet = queue->head;

	PacketUnit * next;
	//hdr_vns * hdr = hdr_vns::access(packet->current);
	hdr_vncommon * hdr = hdr_vncommon::access(packet->current);
	//log(app_code, "QHEAD", hdr->toString());

	while(1)
	{
		next = packet->next;

		//log(app_code, "QUNIT", hdr->toString());
		if(hdr->send_time < lastTime_ - time)
		{
			//log_info(app_code,"QSIZE",queue->size);
			log(app_code, "QDELX", hdr->toString());

			queue->remove(packet);
		}
		if(next == 0)
			break;
		else
		{
			packet = next;
			hdr = hdr_vncommon::access(packet->current);
		}
	}
}

//check the packet sending queue, remove a packet if it is older than the maximum response time
//check the head first, if the head is old, continue, else quit
void VNSAgent::check_head(double time)
{
	if(queue == 0)
		return;

	if(queue->size == 0)
	{
		return;
	}

	PacketUnit * packet = queue->head;

	PacketUnit * next;
	hdr_vncommon * hdr = hdr_vncommon::access(packet->current);
	//log(CODE_VNS, "QHEAD", hdr->toString());

	while(hdr->send_time < lastTime_-time) //the current packet is too old
	{
		next = packet->next;
		//log(CODE_VNS,"QHDEL",hdr->toString());
		//log_info(CODE_VNS,"QHDEL",queue->size);
		queue->remove(packet);

		if(next == 0)
			break;
		else
		{
			packet = next;
			hdr = hdr_vncommon::access(packet->current);
		}
	}
}


/**
 ** forward a VNS message to other local servers
 ** this is for the VNS messages, don't use it for the application
 **
 ** msgType: int message type
 ** seq: the sequence number
 ** addr: address allocated or offered
 ** trans: transmission type, unicast or flooding, corresponding to server message or client message
 ** hop_count: the maximum number of hops the message can travel
 ** sr_len: the length of the source route
 ** sourceRoute: the source route
 ** leaseTime: deadline for reponse
 ** forward: forwarding type, INTER_REGION or LOCAL message
 ** buff_len: the length of an adjustable buffer for the state synchronization messages
 ** state_buffer: the buffer for the states
 **/
//			   server_broadcast(SYN_ACK, vnhdr->dst, clientid, getStateSize(), (u_char *) getState());

void VNSAgent::server_broadcast(int msgType, int seq, int buff_len, u_char * state_buffer)
{
		Packet* pkt = allocpkt();
		hdr_ip* iph = HDR_IP(pkt);
		//hdr_vns* vns_hdr = hdr_vns::access(pkt);
		hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
		hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);


		cmn_hdr->ptype() = PT_VNS;
		cmn_hdr->size() = size_ + IP_HDR_LEN+buff_len; // add in IP header
		cmn_hdr->next_hop_ = IP_BROADCAST;

		iph->saddr() = Agent::addr();
		iph->daddr() = IP_BROADCAST; 	//broadcasting address, should be -1
		iph->dport() = MY_PORT_;		//send only to other server ports
		//iph->sport() = MY_PORT_; //no need
		iph->ttl() = 1;

		vnhdr->type = SYNC_MSG;
		vnhdr->subtype = msgType;
		vnhdr->regionX = regionX_;
		vnhdr->regionY = regionY_;
		vnhdr->send_time = Scheduler::instance().clock();
		vnhdr->src = nodeID_;//client id of transaction
		vnhdr->dst = seq;//sequence number of the transaction
		vnhdr->send_type = SEND;//sending, forwarding or loopback

		pkt->allocdata(buff_len);
		u_char * data = pkt->accessdata();
		memcpy(data, state_buffer, buff_len);


		//Scheduler::instance().schedule(ll,pkt,0.0);
		send(pkt, (Handler*) 0);//this send relies on the routing function, may not work for flooding mode
		if(LOG_ENABLED)
			log(CODE_VNS, "SEND", vnhdr->toString());
}

/**
 ** Send a loopback VNCOMMON message to the JOIN agent attached to the same node
 ** msgType: int message type
 **/

void VNSAgent::send_loopback(int msgType, int seq)
{

	Packet* pkt = allocpkt();
	hdr_ip* iph = HDR_IP(pkt);
	hdr_cmn * cmn_hdr = hdr_cmn::access(pkt);
	hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);


	cmn_hdr->ptype() = PT_VNS;
	cmn_hdr->size() = size_ + IP_HDR_LEN; // add in IP header
	cmn_hdr->next_hop_ = nodeID_;

	iph->saddr() = Agent::addr();
	iph->daddr() = Agent::addr(); 	//broadcasting address, should be -1
	iph->dport() = join_port_;		//send only to other server ports
	//iph->sport() = MY_PORT_; //no need
	iph->ttl() = 1;

	vnhdr->type = SYNC_MSG;
	vnhdr->subtype = msgType;
	vnhdr->regionX = regionX_;
	vnhdr->regionY = regionY_;
	vnhdr->send_time = Scheduler::instance().clock();
	vnhdr->src = seq;//client id of transaction
	vnhdr->dst = leader_;//sequence number of the transaction
	vnhdr->send_type = SEND;//sending, forwarding or loopback

	//log_info(CODE_VNS, "LOOPBACKSENTTO", (double) vnhdr->subtype);
	//if(LOG_ENABLED)
	//	log(CODE_VNS, "SEND", vnhdr->toString());
	send(pkt, (Handler*) 0);//this send relies on the routing function, may not work for flooding mode


}

//get a neighbor region's index in the array
int VNSAgent::toNeigbhorIndex(int rx, int ry)
{
	for(int i=0; i<NUM_NEIGHBORS; i++)
	{
		if(neighbors[i][0]==rx && neighbors[i][1] == ry)
		{
			return i;
		}
	}
	return -1;
}

/*
 *
 * Upon region change, calculate the neighbor list and initialize the status of each neighbor
 *
 */
void VNSAgent::setNeighbors()
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
			}
			else
			{
				neighbor_flags[i] = ACTIVE; //set to ACTIVE here, we may want to set all the valid neighbors to inactive at first.
			//	neighbor_timeouts[i] = Scheduler::instance().clock()+neighbor_timer_;
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
		}
	}
}

/* get the distance between two regions */

int VNSAgent::getDistance(int rx1, int ry1, int rx2, int ry2)
{
    return (rx1-rx2)*(rx1-rx2)+(ry1-ry2)*(ry1-ry2);
}

//convert a region id into an integer
int VNSAgent::encodeRID(int base, int x, int y)
{
	return base+x*rows_+y;
}

	//convert an integer back into region id
void VNSAgent::decodeRID(int base, int RID, int * x, int * y)
{
	*x=(int)(RID-base)/rows_;
	*y=(RID-base)%rows_;
}

/*
 * Using the neighbor list, find the active neighbor that is the closest to the destination
 * Returns the index in neighbors[] where the next hop region id can be found.
 */
int VNSAgent::getNextHop(int destRegionX, int destRegionY)
{
	int minDistanceIndex = -1;
	int	minDistance = columns_*columns_+rows_*rows_;
	int	tempDistance = 0;

	if(regionX_ == destRegionX && regionY_ == destRegionY)
	{
		if(LOG_ENABLED)
			log_info(CODE_VNS,"WARNING","Destination Already Reached");
		return minDistanceIndex;

	}
	minDistance = columns_*columns_+rows_*rows_;
	for(int i=0; i< NUM_NEIGHBORS; i++)
	{
		//log_info(CODE_VNS,"neighbor",i);
		if(neighbor_flags[i] == ACTIVE)
		{
			tempDistance = getDistance(neighbors[i][0], neighbors[i][1], destRegionX, destRegionY);
			//log_info(CODE_VNS,"active",tempDistance);
			if(tempDistance < minDistance)
			{
				minDistance = tempDistance;
				minDistanceIndex = i;
			}
		}
	}

	/****************
	  bool got_minimum = false;
	  bool avoid = false;
	  int trials = 0; 		
	  vector<int> avoid_i;
	  while(!got_minimum)	
	  {
	  minDistance = columns_*columns_+rows_*rows_;
	  for(int i=0; i< NUM_NEIGHBORS; i++)
	  {
	  avoid = false; 
	  for(int j = 0; j < avoid_i.size(); j++)
	  {
	  if(avoid_i[j] == i)
	  {
	  avoid = true;
	  break;
	  }
	  }
	  if(avoid)
	  continue;
	//log_info(CODE_VNS,"neighbor",i);
	if(neighbor_flags[i] == ACTIVE)
	{
	tempDistance = getDistance(neighbors[i][0], neighbors[i][1], destRegionX, destRegionY);
	//log_info(CODE_VNS,"active",tempDistance);
	if(tempDistance < minDistance)
	{
	minDistance = tempDistance;
	minDistanceIndex = i;
	}
	}
	}
	if(LogParkingFile::isLeaderActive(neighbors[minDistanceIndex][0], neighbors[minDistanceIndex][1]))
	{
	got_minimum = true;
	break;
	}
	avoid_i.push_back(minDistanceIndex);
	trials++;
	if(trials >= 3)
	break;
	}
	if(!got_minimum)
	{

	}
	 ***************/
	//log_info(CODE_VNS,"nexthop",minDistanceIndex);

	return minDistanceIndex;
}

//Synchronization timeout
void VNSTimer::expire(Event *e)
{
	a_->syn_timeout(0);
}

//Total Ordered input queue timeout
void OrderedQTimer::expire(Event *e)
{
	a_->orderedQueue_timeout(0);
}

//sending buffer time out.
void ServerSendingTimer::expire(Event *e)
{
	a_->server_timeout(0);
}

u_char * VNSAgent::getState()
{
	return 0;
}

int VNSAgent::getStateSize()
{
	return 0;
}

char * VNSAgent::stateToString()
{
	return "";
}

void VNSAgent::saveState(unsigned char * state, int size)
{
	//memcpy(my_vcs, pkt->accessdata(), my_vcs->size());
	return;
}

//get application packet header into a string
char * VNSAgent::getAppHeader(Packet *pkt)
{
	char * nullString = (char *)malloc(sizeof(""));
	return nullString;//the memory used need to be released by the user of this method.
}

//use DJBHash
unsigned int VNSAgent::getStateHash()
{
   	return 0;

}

