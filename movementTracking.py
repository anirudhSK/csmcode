from symbol import assert_stmt
from datetime import datetime
from time import sleep
import time
from datetime import datetime
from datetime import timedelta
from socket import AF_INET
from socket import *
from threading import Timer

# To change this template, choose Tools | Templates
# and open the template in the editor.
# Output seems sane TODO Verify formally that it is correct.
__author__="anirudh"
__date__ ="$Feb 16, 2011 11:00:02 PM$"


year=2010

month=8
day=1
hour=0
minute=0
second=0

regionX=0
regionY=0

currentRegionX=0
currentRegionY=0

gridX=100
gridY=100 # ie the size of the grid in the x and the y direction. 



# UDP socket for telling the other processes that the region has changed
host="<broadcast>"
port=21567
buf=1024
broadcastAddr=(host,port)
UDPSock=socket(AF_INET,SOCK_DGRAM)
UDPSock.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)


LeaderElection=Timer(LEADER_ELECTION_INTERVAL,timeoutHandler)

def timeoutHandler():
	# fetch up to date version from server.
	return

def GetCurrentTime() :
    # notional time that is given by the trace.
    global year
    global month
    global day
    global hour
    global minute
    global second
    global WallTimeAtStartOfSimulation 
    beginningOfTimeReference=datetime(year,month,day,hour,minute,second)
    timeElapsed=timedelta(seconds=(time.time()-WallTimeAtStartOfSimulation)) # beginningOfTime is the wall clock time read at the simulation beginning
    currentDateTime=beginningOfTimeReference+timeElapsed
    return currentDateTime


def GetCurrentGPS(filehandle):
    # TODO: exception handling is still required in this data, if you can't interpolate the data
    # TODO I don't think the output is correct. It looks like garbage for now.
    currentDateTime=GetCurrentTime() # get current date and time from the tracker

    cursor=filehandle.tell()	     #record previous cursor positions
    cursorLine=filehandle.readline()
    	
    #previousLine=cursorLine
    cursorDateTime=RetrieveDateTime(cursorLine)

    assert (currentDateTime>=cursorDateTime) # otherwise no point interpolating

    while(cursorDateTime<currentDateTime) :
	  previousLine=cursorLine;		# store the previous line for interpolation on exit of loop
	  previousCursor=cursor;
	  cursor=filehandle.tell();
	  cursorLine=filehandle.readline()
	  cursorDateTime=RetrieveDateTime(cursorLine)
	  
   #INVARIANT: cursor is the position at the beginning of cursor line always, check this invariant 

    #  now interpolate loop invariant at this point cursorDateTime >= currentDateTime , also previousLine <currentDateTime

    if(previousLine==0):			# the currentDateTime is the first entry in file
	   print currentDateTime
	   recordValues=cursorLine.split(',')
           nextLongitude=float(recordValues[3])
           nextLatitude=float(recordValues[4])
	   return (nextLatitude,nextLongitude)

   

    assert (previousLine!=0)
   
    timeElapsedSinceLastGPS=currentDateTime-RetrieveDateTime(previousLine)
    timeUntilNextGPS=cursorDateTime-currentDateTime;

    
    recordValues=previousLine.split(',')
    lastLongitude=float(recordValues[3])
    lastLatitude=float(recordValues[4])

    recordValues=cursorLine.split(',')
    nextLongitude=float(recordValues[3])
    nextLatitude=float(recordValues[4])
    
    interpolatedLatitude=(lastLatitude*timeUntilNextGPS.seconds+nextLatitude*timeElapsedSinceLastGPS.seconds)/(timeElapsedSinceLastGPS.seconds+timeUntilNextGPS.seconds);
    interpolatedLongitude=(lastLongitude*timeUntilNextGPS.seconds+nextLongitude*timeElapsedSinceLastGPS.seconds)/(timeElapsedSinceLastGPS.seconds+timeUntilNextGPS.seconds);

    filehandle.seek(previousCursor) # hopefully this roll back and book keeping works. TODO: ensure it does. Performance improvement with or without seek
    
    print currentDateTime
    return (interpolatedLatitude,interpolatedLongitude)
      
def RetrieveDateTime(dateTimeAsString):
    recordValues=dateTimeAsString.split(',') # break the CSV into parts
    timeStamp=recordValues[0]
    dateAndTime=timeStamp.split(' ')
    date=dateAndTime[0]
    time=dateAndTime[1]
    day=int(date.split('/')[0]);
    month=int(date.split('/')[1]);
    year=int(date.split('/')[2]);
    hour=int(time.split(':')[0]);
    minute=int(time.split(':')[1]);
    second=int(time.split(':')[2]);
    #print (year,month,day,hour,minute,second)
    cursorDateTime=datetime(year,month,day,hour,minute,second)
    return cursorDateTime

def checkRegion(lat,long):
     # input lat and long coordinates.
     # convert it into UTM coordinates
     # do a mod against the grid size
     # TODO Find a way to achieve this properly, worst case, system call to the other binary to do it. ie system("./GeoConvert");
     return(lat,long) # return region as such here, as a dummy for now. 

def TrackMovement(gpsTraceFile):
    # use time to read the current GPS from a file
    # interpolate if necessary
    gpsFileHandle=open(gpsTraceFile,'r')
    while(1):
	    gpsFileHandle=open(gpsTraceFile,'r')
	    (lat,long)=GetCurrentGPS(gpsFileHandle)
	    (regionX,regionY)=checkRegion(lat,long)
	    if(regionX!=currentRegionX |regionY!=currentRegionY) :
		# send message to other application.
		UDPSock.sendto("LEADER_REQUEST",broadcastAddr); # send a broadcast message saying you want to be leader
		# start Timer
	    print(lat,long)
	    # TODO Check that this whole broadcast thing works
	    sleep(1)  
if __name__ == "__main__":
   traceFile = raw_input("Enter trace file name: ")
   global WallTimeAtStartOfSimulation
   WallTimeAtStartOfSimulation=time.time() # in a real emulation this should be done on receipt of a synchronization signal
   TrackMovement(traceFile)