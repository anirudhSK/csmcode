#ifndef ns_parkingserver_h
#define ns_parkingserver_h

#include "vnserver.h"
#include <vector>
#include <queue>
#include <utility>
#include <map>
#include <cassert>
#include "log_parking.h"

#define CODE_PARKINGS "PARKING"	//code for application layer messages

//the virtual node based PARKING router/server agent
//the user application should impletement the VCS struct
//the method: server_init(), handle_packet(Packet *pkt)
//and the method: equal(Packet * p1, Packet * p2), which is to compare if two packets a for the same transaction

class ParkingServerAgent;

//Timer for PARKING client
//Time interval between data packet sendings
class ParkingServerTimer : public TimerHandler {
	public:
		ParkingServerTimer(ParkingServerAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		ParkingServerAgent *a_;
};

class ParkingResendingTimer : public TimerHandler {
	public:
		ParkingResendingTimer(ParkingServerAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		ParkingServerAgent *a_;
};


struct ParkingState {
	int num_free_parking_spaces[MAX_ROWS][MAX_COLS];
};

struct ClientRequest {
	int origid;
	int m_count;
	int destX;
	int destY;
	int isWrite;
	double expiration_time;
	int m_retries;

	bool operator<(const ClientRequest& client_req) const
	{
		return expiration_time > client_req.expiration_time;
	}
};

struct RemoteRequest {
	int origid;
	int m_count;
	int isSuccess;
};

struct SharedState {
	int num_free_parking_spaces;
	int version;
	int origid;
	int m_count;
	int destX;
	int destY;
	int isWrite;
	double expiration_time;
	int m_retries;	
	int isSuccess;

	int isClientRequest;
};

class ParkingServerAgent : public VNSAgent {
public:
	ParkingServerAgent();
	
	virtual void parking_timeout(int)
	{
		check_parking_status();
	}

	virtual void resending_timeout(int)
	{
		check_resending_status();
	}

protected:

	ParkingServerTimer parking_timer_; 	//syn status check timeout
	ParkingResendingTimer resending_timer_;  // resending timer

	u_char * getState();//get state as a string
	int getStateSize();//get the size of the state
//	unsigned int getStateHash();//get a hash value from the local state
	void saveState(unsigned char* state, int size);//save state received into the local state vector

	void check_parking_status() {}
	void check_resending_status();
	void server_init();//start the application server
	void handle_packet(Packet *);
	int equal(Packet * p1, Packet * p2);

	void sendp(int send_type, int msg_class, int dest, int dest_port, int msgType, int origid, int m_count, int isSuccess, int isWrite, int srcX, int srcY, int nextX, int nextY, int destX, int destY, int low_seq);

	bool removeClientRequest(int origid, int m_count);
	int isDuplicateRequest(int origid, int m_count);

private:
	ParkingState m_parkingstate;
	int m_version;	
	priority_queue<ClientRequest> m_resending_queue;
	vector<RemoteRequest> m_remote_requests;

	// CSM data
	int g_seq, ;
	map<pair<int, int>, int> l_seq;
	map<int, vector<pair<int, int> > > m_seq_acks;
	map<pair<int, int>, priority_queue<WriteUpdate> > m_write_updates;
};

//timer expiration action for data traffic generation
void ParkingServerTimer::expire(Event *e)
{
	a_->parking_timeout(0);
}

void ParkingResendingTimer::expire(Event *e)
{
	a_->resending_timeout(0);
}

#endif // ns_parkingserver_h
