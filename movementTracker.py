# The equivalent of the GPS device on the mobile phone
# In our implementation, this python script reads from a trace file which is ns2 compliant 
# and tracks region changes etc 

from symbol import assert_stmt
import time
from socket import AF_INET
from socket import *
from threading import Timer
from math import sqrt
import sys 
import signal, os

_author__="anirudh"
__date__ ="$Feb 16, 2011 11:00:02 PM$"

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
    print 'Signal handler called with signal', signum
    WallTimeAtStartOfSimulation=time.time();	# 			   This is the sync signal 
#    raise IOError("Couldn't open device!")



def GetCurrentTime() :							     # TODO Get the current wall clock time , replace this with a call to the time function within python 
    global counter
    counter=counter+1
    global WallTimeAtStartOfSimulation 
    timeElapsed=time.time()-WallTimeAtStartOfSimulation			     # want to debug quickly 		     # beginningOfTime is the wall clock time read at the simulation beginning	
									     # wall time at start of simulation is the time when the real time simulation starts looking at the ns-2 trace and converting it. 
# return timeElapsed							     # TODO Wall clock time here. 	
    return counter							     # for now, we just increment a counter repeatedly to simulate time progressing. 


def GetCurrentGPS(gpsFilehandle) :
	global cursorTimeStamp
	global cursorX
	global cursorY
	global cursorXDest
	global cursorYDest
	global cursorSpeed
	global enableCSM

	currentTime=int(GetCurrentTime());				     # return wall clock time	

	cursor=gpsFilehandle.tell();					     # store the current cursor position 
	cursorLine=gpsFilehandle.readline();				     # peek one step ahead 
   	cursorLine.rstrip('\n')						     # remove the newlines
	traceEntryValues=cursorLine.split(None);			     # I used None because you want to split on whitespaces. 
	nextTimeStamp=int(traceEntryValues[3]);				     # look at the nextTimeStamp 
									     # TODO: Don't keep reading the nextTimeStamp as an optimization on every read 
	if(currentTime>=nextTimeStamp) :

		opCode=traceEntryValues[5]				     # opCode is set, startNode, or stopNode 
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
			return (0,0)					     # TODO Get GPS just after reading startNode, for now it will return (0,0) as some sort of indeterminate state 
			# notify other agents of the change

	
	elif(currentTime<nextTimeStamp): 				     # either you can use the last timestamp or you have not even switched on CSM in which case it really does not matter
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

def checkRegion(x,y):
    # use the coordinates x and y to figure out which region the node is in.
    global   regionXDimension # TODO Initialize this before the VM's start executing
    global   regionYDimension
    return (int(x/regionXDimension),int(y/regionYDimension));


def TrackMovement(gpsTraceFile):
    # use time to read the current GPS from a file  interpolating if necessary
    gpsFileHandle=open(gpsTraceFile,'r')
    currentRegionX=-1
    currentRegionY=-1				# to start off there is no region that the car is located in. TODO Check if this is a sensible thing to do.  
    while(1):
	    (x,y)=GetCurrentGPS(gpsFileHandle)
	    print"Current Time is " + str(counter),
	    print "Retrieved location is (x,y):" + str(x) +"," + str(y)
	    # TODO: Implement the checkRegion algorithm
	    (regionX,regionY)=checkRegion(x,y)
	    if( regionX!=currentRegionX | regionY!=currentRegionY ) :
		# send message to other application.
		UDPSock.sendto("REGION_CHANGE",broadcastAddr); # send a broadcast message saying you want to be leader
		currentRegionX=regionX;		# the region has just changed 
		currentRegionY=regionY;		# retain the new region  
		# start Timer
	    #print(x,y)
	    # TODO Check that this whole broadcast thing works
#	    sleep(1)  


if __name__ == "__main__":
   traceFile = sys.argv[1]
   global WallTimeAtStartOfSimulation
   signal.signal(10, handler)		# catch all signal number 10's. 

   #WallTimeAtStartOfSimulation=time.time() # in a real emulation this should be done on receipt of a synchronization signal, do this on a sync signal 
   while(1) :
	i=0				# TODO: sleep until you get a signal to sync, does sleeping disable signal handling ?	spin wait works, make sure the signal comes from the simulation just before it "runs"
	time.sleep(5)
	print "Woke up \n"		# Awesome: signals are caught even if you are sleeping, I guess that's the whole point of interrupts and signal handlers.
 
   #TrackMovement(traceFile)
