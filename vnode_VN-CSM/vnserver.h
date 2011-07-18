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
 * vnserver.h: header file for a VNS Agent Class for the virtual node system
 *       Supporting the server layer of leaders and nonleader nodes
 */

#ifndef ns_vnserver_h
#define ns_vnserver_h


#include "agent.h"
#include "tclcl.h"
#include "address.h"
#include "ip.h"
#include "malloc.h"
#include "mobilenode.h"

#include "log_parking.h"
#include "header.h"		//common header for Virtual Node related applications
#include "log.h"		//header for logging functions


#define CODE_VNS  "VNS"		//code for VN layer messages

//consistency manager statemachine states
#define UNKNOWN -1			//the initial state, dead node
#define NEWNODE 0			//a new node in a region
#define SYNC    1			//non-leader node just get in a region
#define INIT    2			//leader node just get in a region
#define SERVER  3			//a leader node initialized
#define BACKUP  4			//a non-leader node who got its states synchronized with the leader already
#define MSGSYNC 5			//sync caused by message sequence inconsistency
#define OLD_INACTIVE 6

#define NUM_NEIGHBORS 8	//to be replaced by a configurable parameter

#define SENDING   1		//sending buffer enabled to send
#define NOSENDING 0		//sending buffer disabled

//define
#define FROM_HEAD 0		//insertion into total ordering queue from head
#define FROM_TAIL 1		//insertion into total ordering queue from tail


//declaration of VnsAgent so that it can be referred to by the Timer classes
class VNSAgent;

//Timer for Synchronization function
class VNSTimer : public TimerHandler {
	public:
		VNSTimer(VNSAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		VNSAgent *a_;
};

//Timer for total ordered input buffer
class OrderedQTimer : public TimerHandler {
	public:
		OrderedQTimer(VNSAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		VNSAgent *a_;
};


//Timer for sending buffer
class ServerSendingTimer : public TimerHandler {
	public:
		ServerSendingTimer(VNSAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		VNSAgent *a_;
};

//the virtual node service agent
//the user application should impletement the VCS struct
//the method: server_init(), handle_packet(Packet *pkt), process_packet(Packet *pkt), send_packets()
//and the method: equal(Packet * p1, Packet * p2), which is to compare if two packets a for the same transaction

class VNSAgent : public Agent {
public:
	VNSAgent();
	int MY_PORT_;		//the port number of the agent
	int client_port_;	//the port number of the client
	int join_port_;
 	//region and movement detection
 	int rows_;			//number of rows
 	int columns_;		//number of columns
 	double maxX_;
 	double maxY_;

 	//set from TCL script, shouldn't be tampered
 	int nodeID_;		//the id of the node that the agent is attached to

 	//set by region check from the join agent
 	int regionX_;		//the current region id x
 	int regionY_;		//the current region id y

 	//leader election result and maintanence of the leader
 	int leader_;		//leader's id
 	int leader_status_;	//-1, unknown, 0 requested, 1 denied, 2 stable, 3 unstable
 	int state_;			//the final state of the node, SERVER or BACKUP or UNKNOWN

 	int first_node_hold_off_; //whether a region just boot up should wait a route life time before functional
 	int first_node_;	//whether the node is the node booting up a region
 	double first_node_deadline_; //the time after which the node can function as a leader


 	int sending_state_;	//SENDING 1, NOSENDING 0

  	//input packet queue, total ordered by send time
 	struct PacketQ * input_queue;
 	double next_to_expire;//the next send_time before which all packets must be delivered to the consistency manager

 	//a packet sending buffer
 	struct PacketQ * queue;


 	//parameters
 	double syn_wait_;		//the wait for the server to be ready for synchronization
 	double syn_interval_; 	//the wait between syn_request retries
 	int max_retries_;		//maximum number of syn_request retries

 	int sync_enabled_;		//whether synchronization is allowed. State synchronization can increase the consistency among the leader and non-leader nodes, but can cause more traffic.

	int total_ordering_enabled_;//whether total ordering is enabled or not, when enabled, less synchronization but longer processing time
	int total_ordering_mode_;	//how sorting is done. from head of the queue or tail of the queue								//insertion mode, 0 insert from head, 1 insert from tail
 	double ordering_delay_; //time the messages are supposed to stay in the total order input packet queue

 	double send_wait_;		//interval between message sendings

 	double sync_delay_;		//the minimum delay between two syn-ack messages from the same region


	double neighbor_timer_;

	LogFile * logfile;


	//command handler
	int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	//location check time out
	virtual void syn_timeout(int)
	{
		//log_info(CODE_VNS, "TIMEOUT", "syn timer timeout");
		check_syn_status();
	}

	//total ordered input queue time out
	virtual void orderedQueue_timeout(int)
	{
		//log_info(CODE_VNS, "TIMEOUT", "syn timer timeout");
		check_queue_status();
	}

	//check sending buffer status
	virtual void server_timeout(int)
	{
		check_server_status();
	}



	//send a VNLayer message, alway broadcast
	void server_broadcast(int, int, int, u_char *);

	//send the packets stored in the buffer out
	void send_packets();

	void send_loopback(int, int);

	//synchronize the states with the server
	void sync();



	//the packet handler of the application server, to be implemented by specific applications.
	virtual void handle_packet(Packet *);

	//compare if two application server packets are the same, return 1 if so.
	//to be implemented by specific applications.
	virtual int equal(Packet * p1, Packet * p2);

	//get application packet header into a string
	virtual char * getAppHeader(Packet *pkt);


	//looking for a Packet in a packet q, if a match can be found, remove it from the queue.
	//use equal method to look for a match
	int lookup(PacketQ * q, Packet *pkt);

	//time out packets in the sending queue that stayed in the queue longer than "time"
	void queue_timeout(PacketQ * q, double time);
	//time out packets in the sending queue that stayed in the queue longer than "time"
	void check_head(double time);

	//receive a packet from the ordered input packet queue
	//check against sending queue if the packet is not a local client message
	//do synchronization for local server messages
	void consistencyManager(Packet *);

	//insert application messages into the total ordered input queue
	//sort the packets with send_time
	void sortPacket(Packet *);

	int toNeigbhorIndex(int, int);

protected:

	double lastTime_;

	//virtual methods are supposed to be implemented by the application layer.
	virtual u_char * getState();//get state as a string
	virtual int getStateSize();//get the size of the state
	virtual char * stateToString();//turn the state vector into a string
	virtual void saveState(unsigned char* state, int size);//save state received into the local state vector
	virtual void server_init();//Initialize the application state, to be implemented by specific applications.

	virtual unsigned int getStateHash();//get a hash value based on the application state

	char * app_code; //the trace code for the application layer

	VNSTimer syn_timer_; //syn status check timeout
	OrderedQTimer queue_timer_; //orderQueue timeout
	ServerSendingTimer server_timer_; //server status timeout

	//check the current location and see if anything needs to be done
	void check_syn_status();
	//check the first packet in the total ordered input queue and see if anything needs to be done
	void check_queue_status();
	//check the current leading status and see if a new leader request needs to be sent
	void check_server_status();

	//int syncClientID_; //the current client being processed? the client side in SYNC.
	char * filename; //file pointer used for output
	Tcl &tcl;// the tcl instance

	int retries_;
	int seq_;

	double next_sync_; //the next time a sync is allowed

	int neighbors[NUM_NEIGHBORS][2]; //the list of neighbor region ids
	int neighbor_flags[NUM_NEIGHBORS];//which of the 8 neighbors can be used for routing
	double neighbor_timeouts[NUM_NEIGHBORS];//set a deadline for each neighbor region, after which a neighbor is considered inactive

	void setNeighbors();

	void check_neighbor_regions(Packet*);

	//convert a region id into an integer
	int encodeRID(int base, int x, int y);

	//convert an integer back into region id
	void decodeRID(int base, int RID, int * x, int * y);

	//get the distance between two regions
	int getDistance(int, int, int, int);

	/* Using the neighbor list, find the active neighbor that is the closest to the destination */
	int getNextHop(int, int);

	void inline log(char* agentType, char * eventType, char * eventString)//log a message string and release the memory.
	{
		logfile->printMsgEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, char * eventString)//log a text string without worrying about freeing the memory used
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, double value)//log a value
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, value);
	}

	//reset states when a node moves into a new region
	void inline reset_states()
	{
		sending_state_ = NOSENDING;
		state_ = NEWNODE;
		leader_ = UNKNOWN;
		leader_status_ = UNKNOWN;
		first_node_ = FALSE;//new node to the region
		queue->empty();//dump everything
	}
};

#endif // ns_vnserver_h
