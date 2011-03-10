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
 * vnclient.cc: Code for a Client Agent Class for the virtual node system
 *       Supporting the application clients
 *
 */

#include "mobilenode.h"
#include "vnclient.h"
#include "time.h"
#include <sys/ipc.h>
#include <sys/shm.h>

/* OTCL linkage for the VnsHeader */
/*int hdr_vns::offset_;
static class VnsHeaderClass : public PacketHeaderClass {
public:
	VnsHeaderClass() : PacketHeaderClass("PacketHeader/VNS",
					      sizeof(hdr_vns)) {
		bind_offset(&hdr_vns::offset_);
	}
} class_vnshdr;
*/

/*OTCL linkage for the VNCClass */
static class VNCClass : public TclClass {
public:
	VNCClass() : TclClass("Agent/VNCAgent") {}
	TclObject* create(int, const char*const*) {
		return (new VNCAgent());
	}
} class_vnclient;

/* Agent/VNCAgent constructor */
VNCAgent::VNCAgent() : Agent(PT_VNS),send_timer_(this),tcl(Tcl::instance())
{
	bind("port_number_", &MY_PORT_);
	bind("server_port_", &server_port_);
	bind("nodeID_",&nodeID_);
	bind("send_wait_",&send_wait_);
	bind("packetSize_", &size_);
	bind("client_state_", &client_state_);
	bind("leader_status_", &leader_status_);

	srand(time(NULL)+nodeID_);

	queue = (struct PacketQ *) malloc(sizeof(struct PacketQ *));
	queue->init();

	sending_status_=NOSENDING;

	regionX_ = -1;
	regionY_ = -1;

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

	app_code = "UNKNOWN";
}

/*
 * The method processing every packet received by the agent
 *
 */

void VNCAgent::recv(Packet* pkt, Handler*)
{

  // Access the Ping header for the received packet:
  //hdr_vns * hdr = hdr_vns::access(pkt);
  hdr_vncommon * vnhdr = hdr_vncommon::access(pkt);

  if(vnhdr->type == LEADER_MSG)
  {
	  if(vnhdr->subtype == NEWLEADER)
	  {
		  //log_info(CODE_ADDRC, "RECV_INTERNAL", "Agent/VNC: leader infomation received.");
		  //The local server is the leader.
		  leader_status_ = LEADER;
	  }
	  else if(vnhdr->subtype == NONLEADER)
	  {
		  //log_info(CODE_ADDRC, "RECV_INTERNAL", "Agent/VNC: leader infomation received.");
		  //the local server is non the leader
		  leader_status_ = NON_LEADER;
	  }
  }
  else if(vnhdr->type == REGION_MSG)
  {
	  if(vnhdr->subtype == NEWREGION)
	  {
		  regionX_ = vnhdr->regionX;//update the region id anyways
		  regionY_ = vnhdr->regionY;

		  if(client_state_ == UNKNOWN)//Do this only when a client node is uninitialized
		  {
			  init();
		  }
		  else
		  {
			  reset();//reset something when later the node enters a new region.
		  }
	  }
  }
  else if(vnhdr->type == SYNC_MSG)
  {
	  //do nothing for SYN messages
  }
  /*else if(hdr->type == SYN_REQUEST || hdr->type == SYN_ACK)
  {
	  //do nothing for SYN messages
  }*/
  else if(vnhdr->type == APPL_MSG)//unknown message type
  {
	  if(client_state_ != UNKNOWN)
	  {
		  //if(LOG_ENABLED)
		  //  log_info(CODE_VNC, "RECV_APPL", "Agent/VNC.");
		  handle_packet(pkt);
		  send_packets();
	  }
	  else
	  {
		  //the node is not supposed to process any packet if it's status is unknown
		  //dead node
	  }
  }

  Packet::free(pkt);
  //log_info(CODE_VNC, "PFREE", "packet freed");

}

//the packet handler of the application client, to be overriden by subclasses.
void VNCAgent::handle_packet(Packet * p)
{
	return;
}

//initialize a client application. to be overriden by subclasses.
void VNCAgent::init()
{
	return;
}

//reset a client application's state each time the node enters a region, to be overriden by subclasses.
void VNCAgent::reset()
{
	return;
}
/**
 ** Supporting various OTCL commands on this agent.
 **
 **/

int VNCAgent::command(int argc, const char*const* argv)
{

	if (argc == 2)
	{
		if (strcmp(argv[1], "start") == 0)//This is where the agent starts from
    	{
			//init();//reset all state and wait for REGION message from agent JOIN
      		return (TCL_OK);
    	}
	}
	else if(argc == 3)
	{
		if (strcmp(argv[1], "set-traceFileName") == 0)//set the trace file name used by the agent if the shared file pointer is Null.
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

  // If the command hasn't been processed by VnsAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}

//send the messages in the message buffer, if there is any
void VNCAgent::send_packets()
{
	//log_info(CODE_VNC, "QUEUE", (double)sending_status_);
	if(sending_status_ == SENDING)
	{
		//just wait for the next timeout
		//log_info(CODE_VNS, "QUEUE", "sending ongoing, wait");
	}
	else
	{
		if(queue->size != 0)
		{
			sending_status_ = SENDING;
			//log_info(CODE_VNS, "QUEUE", "sending enabled for the queue");
			send_timer_.resched(send_wait_);
		}
		else
		{
			//log_info(CODE_VNS, "QUEUE", "empty");
		}

	}
}

/*
 * Awaken by the client timer.
 * Check the current buffer status to see if anything needs to be done
 */
void VNCAgent::check_buffer_status()
{
	Packet * p;

	if(queue->size !=0)
	{
		sending_status_ = SENDING;
		p = queue->dequeue();
		if(p!=0)
		{
				hdr_vncommon* hdr = hdr_vncommon::access(p);
				switch(hdr->send_type)
				{
					case SEND: if(LOG_ENABLED) log(app_code,"SEND",getAppHeader(p));break;
					//case SENDLOOPBACK:  if(LOG_ENABLED) log(app_code,"SENDLOOPBACK",getAppHeader(p));break;
					default:;
				} 
			send(p,0);
		}
		send_timer_.resched(send_wait_);
	}
	else
	{
		//log_info(CODE_ADDRC,"QUEUE","Queue emptied");
		sending_status_ = NOSENDING;
	}

}

//get application packet header into a string
char * VNCAgent::getAppHeader(Packet *pkt)
{
	char * nullString = (char *)malloc(sizeof(""));
	return nullString;//the memory used need to be released by the user of this method.
}

//The sending timer times out, check the buffer status.
void SendingTimer::expire(Event *e)
{
	a_->check_buffer_status();
}



