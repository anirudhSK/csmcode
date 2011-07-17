from header import *
from parkingserver import *
from parkingclient import *
from join import *

### GPS Movement Tracker Stuff ###
from symbol import assert_stmt
import time
from socket import AF_INET
from socket import *
from threading import Timer
from math import sqrt
import sys 
import signal, os




class Val(object):
	sync = 1
#	chan =		Channel/WirelessChannel
#	prop =		Propagation/TwoRayGround
#	netif =	   Phy/WirelessPhy
#	mac =		 Mac/802_11
#	ifq =		 Queue/DropTail/PriQueue
#	ll =		  LL
#	ant =		 Antenna/OmniAntenna
	x =			700   	# X dimension of the topography
	y =			700   	# Y dimension of the topography
	rows =			8	# rows of regions
	columns =		8	# columns of regions 
	ifqlen =		250	  # max # of packets in ifq
	seed =			0.0
#	adhocRouting =	DumbAgent				# broadcast packet passed directly to application layer  
	nn =			4					# how many nodes are simulated
	sc =			"../../mobility/scene/paths-700-4-static"	# mobility trace 
	cp =			"../../mobility/scene/cbr-3-test"	# background traffic
	stop =			200.0		   			# simulation time
	stop =			5000.0		   			# simulation time
	myTraceFile =	"output"
	sn_size =		10000	#size of message sending sessions
	num_srcs =		15




class VNode(object):
	def __init__(self, id):
		self.id = id
		
		self.join_agent = JoinAgent(self.id, 1.0, 1.0, Val.x, Val.y, Val.rows, Val.columns)
		self.join_agent.regionX_ = 1
		self.join_agent.regionY_ = 0
		self.join_agent.seq = 1
		self.join_agent.m_version = 1
		
		self.parking_server_agent = ParkingServerAgent(self.id, 1, 1, Val.sync, Val.rows, Val.columns)
		self.parking_client_agent = ParkingClientAgent(self.id, Val.nn, Val.sn_size, Val.num_srcs, 1)
		return
	
	
	def recv(self, pkt):
		self.join_agent.recv(pkt)
		self.parking_server_agent.recv(pkt)
		self.parking_client_agent.recv(pkt)
		return
		
	
	def print_status(self):
		print '### NODE %d: join_agent.state_synced_: %s' % (self.id, str(self.join_agent.state_synced_))
		print '### NODE %d: join_agent.leader_start_: %s' % (self.id, str(self.join_agent.leader_start_))
		print '### NODE %d: join_agent.leader_status_: %s' % (self.id, str(self.join_agent.leader_status_))
		print '\n'
		return





# GPS and node start

regionXDimension=250
regionYDimension=250							  # TODO Initiate this from main or as a command line argument. 

currentRegionX=0
currentRegionY=0

# UDP socket for telling the other processes that the region has changed
host="<broadcast>"
port=21567
buf=1024
broadcastAddr=(host,port)
UDPSock=socket(AF_INET,SOCK_DGRAM)
UDPSock.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
LEADER_ELECTION_INTERVAL=1

cursorTimeStamp=-1
cursorX=-1
cursorY=-1
cursorXDest=-1
cursorYDest=-1
cursorSpeed=-1
counter=0
enableCSM=0
cursorSpeed=-1


def handler(signum, frame):
	global WallTimeAtStartOfSimulation 
	print 'Signal handler called with signal', signum
	WallTimeAtStartOfSimulation=time.time();	# 			   This is the sync signal 
#	raise IOError("Couldn't open device!")



def GetCurrentTime() :								 # TODO Get the current wall clock time , replace this with a call to the time function within python 
	global counter
	global WallTimeAtStartOfSimulation 
	
	counter=counter+1
	timeElapsed=time.time()-WallTimeAtStartOfSimulation				 # want to debug quickly 			 # beginningOfTime is the wall clock time read at the simulation beginning	
										 # wall time at start of simulation is the time when the real time simulation starts looking at the ns-2 trace and converting it. 
# return timeElapsed								 # TODO Wall clock time here. 	
	return counter								 # for now, we just increment a counter repeatedly to simulate time progressing. 


def GetCurrentGPS(gpsFilehandle) :
	global cursorTimeStamp
	global cursorX
	global cursorY
	global cursorXDest
	global cursorYDest
	global cursorSpeed
	global enableCSM

	currentTime=int(GetCurrentTime());					 # return wall clock time	

	cursor=gpsFilehandle.tell();						 # store the current cursor position 
	cursorLine=gpsFilehandle.readline();					 # peek one step ahead 
   	cursorLine.rstrip('\n')							 # remove the newlines
	traceEntryValues=cursorLine.split(None);				 # I used None because you want to split on whitespaces. 
	nextTimeStamp=int(traceEntryValues[3]);					 # look at the nextTimeStamp 
										 # TODO: Don't keep reading the nextTimeStamp as an optimization on every read 
	if(currentTime>=nextTimeStamp) :

		opCode=traceEntryValues[5]					 # opCode is set, startNode, or stopNode 
		if(opCode=="set") :
			cursorTimeStamp=nextTimeStamp
			cursorX=float (traceEntryValues[6])
			cursorY=float(traceEntryValues[7])
			cursorXDest=float(traceEntryValues[8])
			cursorYDest=float(traceEntryValues[9])
			cursorSpeed=float(traceEntryValues[10])
			return	interpolate(cursorX,cursorY,cursorTimeStamp,cursorXDest,cursorYDest,cursorSpeed,currentTime)				
		elif(opCode=="startNode"):
			enableCSM=1;
			return (0,0)
			# notify other agents of the change.
		elif(opCode=="stopNode") :
			enableCSM=0;
			return (0,0)						 # TODO Get GPS just after reading startNode, for now it will return (0,0) as some sort of indeterminate state 
			# notify other agents of the change


	elif(currentTime<nextTimeStamp): 					 # either you can use the last timestamp or you have not even switched on CSM in which case it really does not matter
										 # interpolate based on the previous cursorTimeStamp
											 # rollback so that you can re-read the cursorTimeStamp again 
		gpsFilehandle.seek(cursor)		# rollback
		# interpolate
		# INPUT: cursorX,cursorY,cursorTimeStamp
		# INPUT: cursorXDest,cursorYDest
		# INPUT: currentTime, speed
		# OUTPUT: interpolated location.
		if(enableCSM) :
			return	interpolate(cursorX,cursorY,cursorTimeStamp,cursorXDest,cursorYDest,cursorSpeed,currentTime)
		else :				# no point interpolate for a "dead" node which is out of the simulation area. 
			return (-1,-1)

def interpolate(cursorX,cursorY,cursorTimeStamp,cursorXDest,cursorYDest,cursorSpeed,currentTime) :

#	print cursorYDest
#	print cursorXDest
#	print cursorY
#	print cursorX
	headingX=cursorXDest-cursorX
	headingY=cursorYDest-cursorY

	if(cursorSpeed==0) :
		return(cursorX,cursorY)		# it's not moving, return location as such. 

	if((headingX==0) & (headingY==0)):
		print "No movement at all"
		return(cursorX,cursorY)
	else : 
		timeElapsed=currentTime-cursorTimeStamp
		directionX=headingX/sqrt((headingX**2 + headingY**2))
		directionY=headingY/sqrt((headingX**2 + headingY**2))
		print "Direction vector is" + str(directionX) + "," + str(directionY);
		interpolatedX=cursorX+cursorSpeed*timeElapsed*directionX
		interpolatedY=cursorY+cursorSpeed*timeElapsed*directionY
		distanceTraversed=sqrt((interpolatedX-cursorX)**2 +(interpolatedY-cursorY)**2)
		distanceAllowed=sqrt((cursorXDest-cursorX)**2 +(cursorYDest-cursorY)**2)
		if(distanceTraversed < distanceAllowed) :
			return(interpolatedX,interpolatedY)
		else:
			print "Maximum distance traversed in direction of setdest \n"; 
			return(cursorXDest,cursorYDest)


if __name__ == "__main__":
	node1 = VNode(1)
	node1.join_agent.status_reset()
	node1.join_agent.setNeighbors()
	node1.print_status()
	
	# GPS
	global WallTimeAtStartOfSimulation
	WallTimeAtStartOfSimulation=time.time()
	signal.signal(10, handler)		# catch all signal number 10's.
	gpsTraceFile = sys.argv[1]
	gpsFileHandle=open(gpsTraceFile,'r')
	while(1):
		# Update join agent with new location
		(x, y) = GetCurrentGPS(gpsFileHandle)
		node1.join_agent.x = x
		node1.join_agent.y = y
		print"Current Time is " + str(counter),
		print "Join now has location (x,y):" + str(node1.join_agent.x) +"," + str(node1.join_agent.y)
		time.sleep(5)