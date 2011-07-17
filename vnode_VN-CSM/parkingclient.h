#ifndef ns_parkingclient_h
#define ns_parkingclient_h

#include "vnclient.h"
#include <vector>
#include <queue>
#include <map>
#include <utility>
#include "log_parking.h"

#define CODE_PARKING "PARKING"

#define UP 1 

class ParkingClientAgent;

//Timer for PARKING client
//Time interval between data packet sendings
class ParkingClientTimer : public TimerHandler {
	public:
		ParkingClientTimer(ParkingClientAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		ParkingClientAgent *a_;
};

//Time interval between clock messages
class ParkingClockTimer : public TimerHandler {
	public:
		ParkingClockTimer(ParkingClientAgent *a) : TimerHandler() { a_ = a; }
	protected:
		virtual void expire(Event *e);
		ParkingClientAgent *a_;
};

class ParkingClientResendingTimer : public TimerHandler {
        public:
                ParkingClientResendingTimer(ParkingClientAgent *a) : TimerHandler() { a_ = a; }
        protected:
                virtual void expire(Event *e);
                ParkingClientAgent *a_;
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



//the virtual node client agent
//the user application should impletement
//the method: client_init(), handle_packet(Packet *pkt), process_packet(Packet *pkt), send_packets()

class ParkingClientAgent : public VNCAgent {
public:
	ParkingClientAgent();			//constructor

	void sendp(int send_type, int msg_class, int dest, int dest_port, int msgType, int origid, int m_count, int isWrite, int dest_regionX, int dest_regionY);

	void send_loopback(int msgType, int origid, int m_count, int isWrite, int dest_regionX, int dest_regionY);

	bool removeClientRequest(int origid, int m_count);
	void handle_packet(Packet * pkt);
	//what to do when address allocation timer times out
	virtual void parking_timeout(int)
	{
		check_parking_status();
	}

	virtual void clock_timeout(int)
	{
		check_clock_status();
	}

        virtual void resending_timeout(int)
        {
                check_resending_status();
        }

protected:

	ParkingClientTimer parking_timer_; 	//syn status check timeout
	ParkingClockTimer clock_timer_;
        ParkingClientResendingTimer resending_timer_;  // resending timer
	//check the current state and see if anything needs to be done
	void check_parking_status();
	void check_resending_status();
	void check_clock_status();

private:
	int m_count;
	map<int, double> m_send_time; 
	vector<double> m_response_time[5], m_res_time; 
	vector<int> m_response_count; 
        priority_queue<ClientRequest> m_resending_queue;

	int msg_hops[5];
	map<int, int> hop_count;

	//reset states before runnning a node
	void init()
	{
		printf("Client with NodeID_ - %d is initialized\n", nodeID_);
		srand(time(NULL)+nodeID_);
		client_state_ = UP;

	  	double wait = 1000*((rand()%1000)/1000.0);

	  	parking_timer_.resched(wait);

		for(int i = 0; i < 5; i++)
			msg_hops[i] = 0;

	  	reset(); 
	}

	//reset the states
	void reset()
	{
	  	double backoff = 0.01*((rand()%1000)/1000.0);
	  	clock_timer_.resched(backoff);
	}
};

//timer expiration action for data traffic generation
void ParkingClientTimer::expire(Event *e)
{
	a_->parking_timeout(0);
}

//timer action for timer message
void ParkingClockTimer::expire(Event *e)
{
	a_->clock_timeout(0);
}

void ParkingClientResendingTimer::expire(Event *e)
{
        a_->resending_timeout(0);
}

#endif // ns_parking_h

