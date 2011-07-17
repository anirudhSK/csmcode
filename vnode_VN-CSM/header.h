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
 * header.h: Header file for the virtual node system
 * Definition of the MACROs and data structures for the messages
 *
 */

#ifndef ns_header_h
#define ns_header_h

#define CSM 1 // Whether allow caching? 
#define MAX_HOP_SHARING 3 // How many hops of sharing

#define MAX_ROWS 10
#define MAX_COLS 10
#define FREE_SPOTS 10
#define MAX_RETRIES 10
#define PERCENT_LOCAL -1
#define RESEND_BACKOFF 5 // Conservative for now!

#define KEYVALUE 12345 //key to access the piece of shared memory for LogFile object
#define MSG_STRING_SIZE 500 //maximum string size

#define MAX_ROUTE_LENGTH 100 //maximum length of a source route

#define LOG_ENABLED 1 //logging enabled or not

//boolean values
#define TRUE 1
#define FALSE 0

//leader election state machine state id's
//should be moved to join.h
#define DEAD -2 		//a node that is powered off
#define UNKNOWN -1		//a node that enters a region but doesn't know its leader status
#define REQUESTED 0		//a node that starts leader election
#define LEADER 1		//a node that claims to be the leader
#define NON_LEADER 2	//a node that claims to be the non-leader
#define UNSTABLE 3		//a node that missed at least one heartbeat message
#define PENDING 4		//requested from nearby VN
#define OLD_LEADER 5		//old leader

//leader election message types (struct hdr_join), for agent join
#define LEADER_REQUEST 0//request for leadership
#define LEADER_REPLY 1	//reply for leadership request
#define HEART_BEAT 2	//heartbeat message used to claim leadership
#define LEADER_LEFT 3	//the current leader has left
#define HEART_BEAT_ACK 4 //response from secondary leader
#define LEADER_LEFT_ACK 5 //ack for leader left msg
#define LEADER_ELECT 6	//leadership election start
#define LEADER_REQUEST_REMOTE 7	//leadership request from remote region
#define LEADER_ACK_REMOTE 8	//leadership ack from remote region
#define LEADER_ACK_ACK 9	//leadership ack_ack

//answer types for LEADER_REPLY messages
#define CONSENT 1 //ok
#define DISSENT 2 //need negotiation
#define NOWAY 0 //rejection since the receiver is a leader

//INTERNAL loopback messages
#define NEWLEADER 99 //from the join agent to the vns/vnc agent, notifying the change of leader status to leader
#define NEWREGION 88 //from the join agent to the vns/vnc agent, notifying the change of region
#define NONLEADER 77 //from the join agent to the vns/vnc agent, notifying the change of leader status to non-leader
#define RG_UPDATE 66 //from the join agent to the vns agent, notifying the change of neighbor regions' activeness
#define ST_SYNCED 55 //from the vns agent to the join agent, notifying that the local node has synchronized its state with the leader
#define BCKLEADER 44 //secondary leader
#define ST_VER 56 //from the vns agent to the join agent, notifying the version #
#define OLDLEADER 33 //

//VNS message types, struct header_vns
#define SYN_REQUEST 0
#define SYN_ACK 1

//Application message types, type 0, 1, 99, 88, 77, 66 reserved for vns layer messages
#define REQUEST 2
#define OFFER 3
#define ACQUIRE 4
#define ACK 5
#define RENEW 6
#define RACK 7

//message class, sub types in vns messages.
//This is for convenience, actually the internal loopback messages, vns messages and application messages should use
//separate message formats and data structures. However, it will be hard to tell from the
//same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
#define INTERNAL_MESSAGE 0 			//internal loopback messsages, shall be limited at vnlayer
#define SYNC_MESSAGE 1 				//sync_request and sync_ack messages, shall be limited at vnlayer
#define SERVER_MESSAGE 2 			//server messages sent directly from the servers, this set of messages will be used in synchronization
#define FORWARDED_SERVER_MESSAGE 3 	//forwarded server messages, needed for emptying sending queues of non-leader nodes, no need for synchronization
#define FORWARDED_CLIENT_MESSAGE 4 	//forwarded client messages, needed for emptying sending queues of non-leader nodes, no need for synchronization
#define CLIENT_MESSAGE 5			//messages sent directly from clients, this set of messages will be ignored by the vns layer packet processing
//#define SERVER_MESSAGE_AODV 6		//special message class designed for AODV/VN protocol. server messages will be sent to client port and
#define PARKING_REQUEST 6
#define PARKING_REPLY 7
#define PARKING_ACK 8
#define PARKING_REQUEST_RESEND 9
#define WRITE_UPDATE 10
#define WRITE_UPDATE_REPLY 11

//transmission type
#define FLOOD 0
#define UNICAST 1

//forwardingType
#define LOCAL 0			//local forwarding, last hop
#define INTER_REGION 1	//inter-region forwarding

//sending type
#define SEND 0
#define FORWARD 1
#define SENDLOOPBACK 2 //no effect on bandwidth

//neighbor status
#define ACTIVE 1
#define INACTIVE 0
#define INVALID -1

#define PGEN_DMSG 333 //PGEN_DMSG

struct WriteUpdate {
        int reg_x, reg_y;
        int parking_spots;
        int seq_no;

        bool operator<(const WriteUpdate& wrt_up) const
        {
                return seq_no < wrt_up.seq_no;
        }
};

struct hdr_pgen {
	int type; //packet type
	double send_time; //sending time
	int src; //source address
	int dst; //destination address
	int seq; //sequence number

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }

	inline static hdr_pgen* access(const Packet* p) {
		return (hdr_pgen*) p->access(offset_);
	}

	// toString() method used to display the message content
	// can be used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%.4f,%d,%d,%d", type, send_time, src, dst, seq);
		return str;
	}
};

//common message header, message types
#define JOIN_MSG	1 //JOIN agent messages, subtype LEADER_REQUEST, LEADER_REPLY, HEART_BEAT, LEADER_LEFT
#define SYNC_MSG	2 //Synchronization messages, subtype SYN_REQUEST, SYN_ACK
#define LEADER_MSG	3 //loopback messages, leadership info, subtype NEWLEADER, NONLEADER
#define REGION_MSG	4 //loopback messages, region info, subtype NEWREGION
#define APPL_MSG	5 //Application messages, to be passed on to the application layer, subtypes: INTERNAL_MESSAGE,

/*
 *	Common Message Header for virtual node system.Including the vns layer synchonization message,
 *  internal loopback messages from the join agent and application messages defined by the user.
 *  This is for convenience, actually the loopback messages, vns messages and application messages should use
 *  separate message formats and data structures. However, it will be hard to tell from the
 *  same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
 *  The message_layer field is used to tell apart the three type of messages and the messages that need to be used
 *  for synchronization.
 */
struct hdr_vncommon {

	int type;			//Top level message type:

	int subtype; 		//Subtype

	double send_time;	//sending time

 	int regionX;		//source region X
 	int regionY;		//source region Y

 	int src;			//source node
 	int dst;			//destination node

 	int send_type;		//sending type. for the VNLayer to generate debugging info

 	unsigned int hash;	//this is to store the state hash.

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }

	inline static hdr_vncommon* access(const Packet* p) {
		return (hdr_vncommon*) p->access(offset_);
	}

	// toString() method used to display the message content
	// can be used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%d,%.4f,(%d.%d),%d,%d", type, subtype, send_time, regionX, regionY, src, dst);
		return str;
	}
};




/*
 *	Message format of the vns messages. Including the vns layer synchonization message,
 *  internal loopback messages from the join agent and application messages defined by the user.
 *  This is for convenience, actually the loopback messages, vns messages and application messages should use
 *  separate message formats and data structures. However, it will be hard to tell from the
 *  same receiving node which message are of which format. Hence, we use the same data structure to hold the two message types.
 *  The message_layer field is used to tell apart the three type of messages and the messages that need to be used
 *  for synchronization.
 */
struct hdr_vns {



	int message_class; 	//needed by consistency manager to classify messages
						//INTERNAL_MESSAGE 0, SYNC_MESSAGE 1, SERVER_MESSAGE 2, FORWARDED_SERVER_MESSAGE 3, FORWARDED_CLIENT_MESSAGE 4, CLIENT_MESSAGE 5
 	int type;			//message type: 0 request, 1 reply,

	double send_time;	//sending time

 	int regionX;		//source region X
 	int regionY;		//source region Y

 	int client_id;		//the node id of the client

 	int server_regionX;	//the region id X of the server
 	int server_regionY;	//the region id Y of the server

 	int seq;			//request sequence number, created from the requestor

	int addr;			//address given, last two bytes of a 4 byte IP address

	int transmissionType; 	// broadcast or unicast 0 FLOOD, non-negative integer: unicast

	int hop_count; 			// the count of the hops the packet has travelled through.

	int source_route_len; 	// length of the source route

	int src_route[MAX_ROUTE_LENGTH][2]; // the buffer for the source route

	double lease_time; 		// the time deadline that a response for the message is expected
							// used to time out stale messages

	int forwardingType; 	// LOCAL message 0 or INTER_REGION message 1

	int sendingType; 		// SEND 0, FORWARD 1, SENDLOOPBACK 2

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }

	inline static hdr_vns* access(const Packet* p) {
		return (hdr_vns*) p->access(offset_);
	}

	// toString() method used to display the message content
	// used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
		char * sroute = (char *)malloc(MAX_ROUTE_LENGTH*2);
		sroute[0]='\0';
		char * tmp = (char *)malloc(MAX_ROUTE_LENGTH*2);
		for(int i=0; i<source_route_len; i++)
		{
			if(i+1==source_route_len)
				sprintf(tmp, "%d.%d", src_route[i][0], src_route[i][1]);
			else
				sprintf(tmp, "%d.%d|",src_route[i][0], src_route[i][1]);

			strncat(sroute, tmp, MAX_ROUTE_LENGTH*2-strlen(sroute)-1);
		}

	  	sprintf(str, "%d,%.4f,(%d.%d),%d,(%d.%d),%d,%d,%d,%d,%d,%s,%.4f,%d,%d,C:%d", type, send_time, regionX, regionY, client_id, server_regionX, server_regionY, seq, addr, transmissionType, hop_count, source_route_len, sroute, lease_time, forwardingType,sendingType,message_class);
		delete sroute;
		delete tmp;
		return str;
	}
};




/*
 *	Message format of the leader election messages.
 */
struct hdr_join {

 	int type;		//message type: 0 request, 1 reply, heartbeat 2,
	double send_time;	//sending time
 	double ldr_start;	// when the sender became a leader or got its state synchronized
 	double time_to_leave; ////time to leave the current region

 	int src;		//the node id of the sender
 	int dst;		//the node id of the receiver

 	int regionX;	//source region X
 	int regionY;	//source region Y

 	int seq;		//sequence number of request from the requestor
 	int answer;		//the answer of reply, 0 no, 1 yes, 2 conflict
	int version;		//version of the leader. NIKET	
	int old_x;		//old_x of the leader. NIKET	
	int old_y;		//old_y of the leader. NIKET	
	int parking_spots;

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_join* access(const Packet* p) {
		return (hdr_join*) p->access(offset_);
	}

	// toString() method used to display the message content
	// used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%.4f,%.4f,(%d.%d),%d,%d,%d,%d,%.4f", type, send_time, ldr_start, regionX, regionY, src, dst, seq, answer,time_to_leave);
		return str;
	}
};

/*
 * A container holding Packet objects, the basic unit of a double linked list
 */
struct PacketUnit{
	Packet * current; //the packet held by this unit
	PacketUnit * next; //the next packet container on the list
	PacketUnit * previous; //the previous packet container on the list

	double deadline; //time that the packet can stay in the buffer
	//load a packet object into the struct
	inline void load(Packet *p)
	{
		current = p;
	}
};

/*
 * A packet queue structure, built on a double linked list
 */
struct PacketQ{
	int size; //the size of the queue
	PacketUnit * head; //the earliest packet
	PacketUnit * tail; //the latest packet

	//initialize the queue
	inline void init()
	{
		size = 0;
		head = 0;
		tail = 0;
	}

	//empty the queue
	inline void empty()
	{
		while(size!=0)
		{
			remove(head);
		}
	}



	//add a packet to the end of the queue
	inline PacketUnit * enqueue(Packet * pkt)
	{
		//create a packetUnit container for the incoming packet
		struct PacketUnit * p = (struct PacketUnit *) malloc(sizeof(struct PacketUnit));
		p->load(pkt);

		if(head == 0 && tail == 0)//first packet in the buffer
		{
			head = p;
			p->next = 0;
			p->previous=0;
			tail = p;
			size = 1;
		}
		else if(head !=0 && tail != 0)
		{
			p->previous = tail;
			tail -> next = p;
			tail = p;
			p->next = (PacketUnit*)0;
			size = size + 1;
		}
		else
		{
			printf("Packet Buffer error. one of head and tail is zero\n");
			exit(0);
		}

		return p;
	}

	//insert a packet behind a packet Unit in the queue
	inline PacketUnit * insertBehind(PacketUnit * unit, Packet * pkt)
	{
		if(unit==0 || pkt ==0)
			return 0;//error
		if(tail == unit)//the current unit is already the end of queue
		{
			return enqueue(pkt);//just add the packet to the end
		}
		else
		{
			//create a packetUnit container for the incoming packet
			struct PacketUnit * p = (struct PacketUnit *) malloc(sizeof(struct PacketUnit));
			p->load(pkt);

			p->next = unit->next;
			unit->next->previous = p;

			p->previous = unit;
			unit->next = p;
			size=size+1;

			return p;
		}
	}

	//insert a packet in front of a packet Unit in the queue
	inline PacketUnit * insertBefore(PacketUnit * unit, Packet * pkt)
	{
		if(head == unit)//the current unit is already the end of queue
		{
			//create a packetUnit container for the incoming packet
			struct PacketUnit * p = (struct PacketUnit *) malloc(sizeof(struct PacketUnit));
			p->load(pkt);

			head=p;
			p->previous =0;
			p->next=unit;
			unit->previous = p;
			size=size+1;

			return p;
		}
		else
		{
			return insertBehind(unit->previous, pkt);
		}
	}

	//remove a packet from the head of the queue
	inline Packet * dequeue()
	{
		struct PacketUnit * p;
		Packet * temp;
		if(head == 0) //empty queue
		{
			if(size !=0)
			{
				printf("Packet Buffer error. head is zero but queue size is not zero\n");
				exit(0);
			}

			return 0;
		}
		else
		{
			if(head->next == 0)//last packetUnit
			{
				size = 0;
				p=head;
				head = 0;
				tail = 0;
				temp = p->current;

				delete p;
				if(temp ==0)
				{
					printf("Dequeue error. head is not zero but is empty\n");
					exit(0);
				}
				return temp;
			}
			else
			{
				size = size -1;
				p=head;
				head=p->next;
				head->previous=0;

				temp = p->current;
				delete p;

				if(temp ==0)
				{
					printf("Dequeue error 2. head is not zero but is empty\n");
					exit(0);
				}
				return temp;
			}
		}

	}

	//remove a packet container from the queue if the packet container contains the same packet object as the one in PacketUnit *p
	inline void remove(PacketUnit * p)
	{
		if(p!=0 && size!=0)
		{
			if(size == 1)
			{
				if(head == p)
				{
					size = 0;
					head = 0;
					tail = 0;
				}
				Packet::free(p->current);
				delete p;
				return;
			}
			else if(size == 2)
			{
				if(head == p)
				{
					head = p->next;
					tail->previous=0;
					size = 1;

				}
				else if(tail == p)
				{
					tail = head;
					head->next =0;
					size = 1;
				}
				Packet::free(p->current);
				delete p;
				return;
			}
			else
			{
				if(head == p)
				{
					head = p->next;
					head->previous =0;

				}
				else if(tail == p)
				{
					tail = p->previous;
					tail->next = 0;
				}
				else
				{
					p->previous->next = p->next;
					p->next->previous = p->previous;
				}
				size = size -1;

				Packet::free(p->current);
				delete p;
				return;
			}

		}
	}

	//remove a packet container from the queue, don't release the memory used by the packet, though (make sure there is no memory leak)
	inline void removeUnit(PacketUnit * p)
	{
		if(p!=0 && size!=0)
		{
			if(size == 1)
			{
				if(head == p)
				{
					size = 0;
					head = 0;
					tail = 0;
				}
				//Packet::free(p->current);
				delete p;
				return;
			}
			else if(size == 2)
			{
				if(head == p)
				{
					head = p->next;
					tail->previous=0;
					size = 1;

				}
				else if(tail == p)
				{
					tail = head;
					head->next =0;
					size = 1;
				}
				//Packet::free(p->current);
				delete p;
				return;
			}
			else
			{
				if(head == p)
				{
					head = p->next;
					head->previous =0;

				}
				else if(tail == p)
				{
					tail = p->previous;
					tail->next = 0;
				}
				else
				{
					p->previous->next = p->next;
					p->next->previous = p->previous;
				}
				size = size -1;

				//Packet::free(p->current);
				delete p;
				return;
			}

		}
	}
};

////AODV over VNLayer message types
#define RREQ 1
#define RREP 2
#define RERR 3
#define DMSG 10
#define DACK 11
#define CLCK 22


/*
 *	Message format of the AODV messages.
 */
struct hdr_vnaodv {

 	int type;		//message type: 0 request, 1 reply, heartbeat 2,
	double send_time;	//sending time

 	int regionX;	//source region X
 	int regionY;	//source region Y

 	int rreqRegionX; //region launching the RREQ
 	int rreqRegionY; //region lannching the RREQ

 	int nextX;		//next hop region id X
 	int nextY;		//next hop region id Y

 	int destid;		//final destination node's id
 	int destseq;	//route sequence number for the destination id

 	int origid;		//originator of a route request
 	int origseq;	//the route sequence number of the originator

 	int hopcount;	//the number of hops the message has traversed
 	int ttl;		//the number of hops the message can still be forwarded

 	double lifetime;//the life time of the route being sent

 	int rreqseq;	//RREQ sequence number

 	int forwardingType; //local or inter-region message

 	int size;		//the size of the payload

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_vnaodv* access(const Packet* p) {
		return (hdr_vnaodv*) p->access(offset_);
	}

	// toString() method used to display the message content
	// used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%.4f,(%d.%d),(%d.%d),(%d.%d),%d,%d,%d,%d,%d,%d,%d,%.4f,%d", type, send_time, regionX, regionY, rreqRegionX, rreqRegionY, nextX, nextY, destid, destseq, origid, origseq,hopcount,ttl,rreqseq,lifetime,forwardingType);
		return str;
	}
};

#define UPDATE 14   //route update
#define QUERY 15	//explict route request

/*
 *	Message format of the parking messages.
 */
struct hdr_vnparking {

 	int type;		//message type: 0 data, 1 route update, 2,

 	int srcX;	//source region X
 	int srcY;	//source region Y

 	int destX;	//destination region X
 	int destY;	//destination region Y

 	int nextX;	//next hop region X
 	int nextY;	//next hop region Y

	int isWrite;
	int isSuccess;
	double m_send_time; 

	int m_token;
	int m_count;
	int csm_seq;

	int destid;
	int origid;		//originator of a route request
 
	int size;		//the size of the payload, which is the routing information updates

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_vnparking* access(const Packet* p) {
		return (hdr_vnparking*) p->access(offset_);
	}

	// toString() method used to display the message content
	// used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	
	// TODO :: toString should accommodate all data.

	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%.4f,(%d.%d),(%d.%d),%d,%d, %d", type, m_send_time, srcX, srcY, destX, destY, destid, origid, isWrite);
		return str;
	}
};

////SNSR network over VNLayer message types
#define BEACON 1
#define REPORT 2
#define INFORM 3
#define WAKEUP 4

/*
 *	Message format of sensor network messages.
 */
struct hdr_vnsnsr {

 	int type;		//message type
	double send_time;	//sending time

	int sender; //sender id
	int seq;

 	int regionX;	//source region X
 	int regionY;	//source region Y

 	float X; //x coordinate
 	float Y; //y coordinate

 	float distance; //the distance between the target and the sensor. used in REPORT message.

	// Header access methods, required by PacketHeaderManager
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_vnsnsr* access(const Packet* p) {
		return (hdr_vnsnsr*) p->access(offset_);
	}

	// toString() method used to display the message content
	// used by the logging functions.
	// Be aware the memory used by the string need to be released after logging is done.
	inline char * toString(){
		char * str = (char *)malloc(MSG_STRING_SIZE);
	  	sprintf(str, "%d,%.4f,%d,%d,(%d.%d),(%f.%f),%f", type, send_time, sender, seq, regionX, regionY, X, Y, distance);
		return str;
	}
};

#endif // ns_header_h
