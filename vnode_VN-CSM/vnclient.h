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
 * vnclient.h: header file for a VN client Agent Class for the virtual node system
 *       Supporting the application clients
 *
 */
#ifndef ns_vnclient_h
#define ns_vnclient_h


#include "agent.h"
#include "tclcl.h"
#include "address.h"
#include "ip.h"
#include "malloc.h"

#include "header.h"	//common header for Virtual Node based applications
#include "log.h"	//header for logging

#define CODE_VNC "VNC"


//sending states
#define SENDING 1		//queue not empty
#define NOSENDING 0		//queue empty


class VNCAgent;

//Timer for sending buffer.
class SendingTimer : public TimerHandler {
	public:
		SendingTimer(VNCAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		VNCAgent *a_;
};


//the virtual node client agent
//the user application should impletement
//the method: client_init(), handle_packet(Packet *pkt), process_packet(Packet *pkt), send_packets()

class VNCAgent : public Agent {
public:
	VNCAgent();			//constructor
	//port number fo the client agent and its server agent
	int MY_PORT_; 		//the port number of the agent
	int server_port_; 	//the port number of the server

 	//set from TCL script, shouldn't be tampered
 	int nodeID_;		//the id of the node that the agent is attached to

 	//set by REGION message from the join agent
 	int regionX_;		//the current region id x
 	int regionY_;		//the current region id y

 	//a packet sending buffer
 	struct PacketQ * queue;

	//command handler
	int command(int argc, const char*const* argv);

	//packet handler
	virtual void recv(Packet*, Handler*);

	//the packet handler of the application client
	virtual void handle_packet(Packet *);

	//get application packet header into a string
	virtual char * getAppHeader(Packet *pkt);

	LogFile * logfile;

	//check the current buffer status and see if any packet need to be send
	void check_buffer_status();

	char * app_code;	//The code string marking the application

protected:

	SendingTimer send_timer_; 	//client timer for the sending buffer

	int sending_status_;//sending buffer flag. TRUE for sending.
	double send_wait_;	//the minimum interval between message sendings to ensure the sending order
	int client_state_; //the status of the client agent, initially UNKNOWN (-1)
	int leader_status_;	//host node leading status
	int packet_size_;	//the default simulated packet size

	//check the current sending status
	void send_packets();

	char * filename;	//file pointer used for output
	Tcl &tcl;			//the tcl instance

	void inline log(char* agentType, char * eventType, char * eventString)//log a message string and release the memory.
	{
		logfile->printMsgEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,4, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, char * eventString)//log a text string without worrying about freeing the memory used
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,4, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, double value)//log a value
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,4, eventType, value);
	}

	//prepare a node for start
	virtual void init();
	virtual void reset();
};



#endif // ns_vnclient_h
