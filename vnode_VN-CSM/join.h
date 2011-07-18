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
 * join.h: Header File for 'Join' Agent
 *
 */

#ifndef ns_join_h
#define ns_join_h

#include "agent.h"
#include "tclcl.h"
//#include "packet.h"
#include "address.h"
#include "ip.h"
#include "malloc.h"
#include <vector>

#include "log_parking.h"
#include "header.h"			//common header for Virtual Node agents
#include "log.h"			//header for logging functions

//State machine code for node motion
#define NEW_NODE 0
#define MOVING 1
#define STOPPED 2

#define NUM_NEIGHBORS 8 //for square regions, the maximum number of neighbors is 8

#define MAX_APPS 10 		//number of apps ports that the agent report region and leadership status to
							//[Might want to put this in TCL script later.]
#define CODE_JOIN "JOIN"	//code for leader election related events
#define CODE_MOVE "MOVE"	//code for node motion related events
#define CODE_INFO "INFO"	//code for other events

class oldLeaderData{
public:
	oldLeaderData(int ver, int ox, int oy, int spots) 
{
	latest_version = ver;
	old_x = ox;
	old_y = oy;
	parking_spots = spots; 
	retries = 0;
	is_valid = true;
	leader_ack = false;	
	leader_ack_ack = false;
	new_leader = -1;	
}

	int latest_version;
	int old_x, old_y;
	int parking_spots; 
	int retries, new_leader;	
	bool is_valid, leader_ack, leader_ack_ack;
};

//declaration of class JoinAgent here so that it can be used by the Timer classes.
class JoinAgent;

//Timer for location checking
class JoinTimer : public TimerHandler {
	public:
		JoinTimer(JoinAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		JoinAgent *a_;
};

//Timer for leader election
class LeaderReqTimer : public TimerHandler {
	public:
		LeaderReqTimer(JoinAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		JoinAgent *a_;
};

//Timer for old leader 
class OldLeaderTimer : public TimerHandler {
	public:
		OldLeaderTimer(JoinAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		JoinAgent *a_;
};



//definition of the class for agent JOIN
class JoinAgent : public Agent {
public:
	JoinAgent();
	int MY_PORT_; 	//the port number of the agent

 	//region and movement detection
 	double maxX_;	//maximum width of the area
 	double maxY_;	//maximum length of the area
 	int rows_;		//number of rows
 	int columns_;	//number of columns

 	int nodeID_;	//the id of the node that the agent is attached to
 	int regionX_;	//the current region id x
 	int regionY_;	//the current region id y
 	int seq_;		//a sequence number
 	int status_;	//the motion status of the node

 	// the following 3 variables are not used anymore
  	double interval_;		//the interval between which two location checks are to be done
  	double slowInterval_;	//a long interval doing the same thing
 	double zeroDistance_;	//the shortest distance to be treated as zero


 	//leader election
 	int leader_;			//leader's id
 	int leader_status_; 	//-1, unknown, 0 requested, 1 denied, 2 stable, 3 unstable
 	int beat_misses_; 		//during unstable status, used to count the missed heartbeats

 	double last_sent_; 		//the time the agent sent the last request message
 	double leader_start_;	//the time the agent start being a leader

 	double claim_period_; 	//the time a node wait for response before claiming leadership, 5ms
 	double beat_period_; 	//the interval between heartbeat messages sent by the leader, 5ms
 	double max_delay_; 		//the delay before the first leader request message is sent, 5ms
 	int beat_miss_limit_; 	//the maximum number of missed heartbeat messages before a node tries to claim leadership, 3times

	LogFile * logfile;

	double time_to_leave_;  //time to leave the current region
	int state_synced_;	//state_synced
	int m_version; //version of data

	//commands
	int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);


	//location check time out
	virtual void timeout(int)
	{
		check_location();
	}

	//leader status check time out
	virtual void leader_req_timeout(int)
	{
		check_leader_status();
	}

	virtual void old_leader_timeout(int)
	{
		check_old_leader_status();
	}

	//send a unicast message
	void send_unicast(int, int, int, int);

	//send a unicast message to a port
	void send_unicast_to(int, int, int);

	//send a broadcast message
	void send_broadcast(int, int);

	void send_left_broadcast(int, int, int, int, int, int);
	void send_left_unicast(int msgType, int dest, int version, int old_x, int old_y, int seq);
	//send a loopback message to the application layer
	void send_loopback(int);

	//send a loopback message to the application layer, VNS agent to port 17920
	void copy_loopback(Packet *, int);

protected:

	JoinTimer join_timer_; //location check timeout
	LeaderReqTimer leader_req_timer_; //leader request timeout
	OldLeaderTimer old_leader_timer;
	int old_leader_retries;

	//check the current location and see if anything needs to be done
	double check_location();

	//check the current leading status and see if a new leader request needs to be sent
	void check_leader_status();

	bool isOldRequest(int ver, int src, int x, int y);
	bool isOldRequestValid(int x, int y, int ver);
	void markOldRequestInValid(int x, int y, int ver);
	void check_old_leader_status();

	//find out how long the node needs to get into the next region
	double getArrivalTime(double, double, double, double);

	//returns the region a node is located in
	int getRegion(double, double, int *, int *);

	double get_delay();

private:
	char * filename; //file pointer used for output now.
	int num_apps;
	int app_port[MAX_APPS*2];

	Tcl &tcl;// the tcl instance

//NIKET - added for leader left
	vector<oldLeaderData > old_leaders;

	//neighbor status
	int neighbors[NUM_NEIGHBORS][2]; //the list of neighbor region ids
	int neighbor_flags[NUM_NEIGHBORS];//which of the 8 neighbors can be used for routing
	double neighbor_timers[NUM_NEIGHBORS];//use the heartbeat messages to timeout inactive neighbors


	void inline log(char* agentType, char * eventType, char * eventString)//the trace header
	{
		logfile->printMsgEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, char * eventString)//the trace header
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, eventString);
	}

	void inline log_info(char* agentType, char * eventType, double value)//the trace header
	{
		logfile->printInfoEvent(filename, agentType, Scheduler::instance().clock(), regionX_, regionY_, nodeID_,leader_status_, eventType, value);
	}

	//reset states related to leader election
	void status_reset()
	{
		leader_ = UNKNOWN;
		leader_status_ = UNKNOWN;
		last_sent_ = 0;
		beat_misses_ = 0;
		leader_start_ = UNKNOWN;
		state_synced_ = FALSE;
	}

	//upon region change, calculate the neighbor list and initialize the status of each neighbor
	void setNeighbors();
};

void JoinTimer::expire(Event *e)
{
	a_->timeout(0);
}

void LeaderReqTimer::expire(Event *e)
{
	a_->leader_req_timeout(0);
}


void OldLeaderTimer::expire(Event *e)
{
	a_->old_leader_timeout(0);
}

#endif // ns_join_h
