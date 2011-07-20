#include<stdlib.h>
#include<stdio.h>
#include"gpsFunctions.h"
#include<assert.h>
#include<string.h>
// variables 
#define MAX_REGIONX 3
#define MAX_REGIONY 3 			// MAX Region ID + 1 is the number of regions  
int regionCounter[MAX_REGIONX][MAX_REGIONY];
static int GPSState[NUMBER_OF_EMULATOR_CLIENTS];
static struct interpolationStructure interpolation[NUMBER_OF_EMULATOR_CLIENTS]; 			// to be used only within this file. 
long traceStartTime[NUMBER_OF_EMULATOR_CLIENTS];						// read this from the first line of the file when you open the file for the very first time
void InitializeStartOfTraceTime(FILE* fileHandle) {
	int timeStamp;
	float latitude;
	float longitude;
	int nodeID;
	int numberOfArguments=fscanf(fileHandle,"$GPS-FIX at time %d $node_(%d) new GPS fix longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude);
//	printf ("Time stamp is %d, node ID is %d \n",timeStamp,nodeID);
	assert(numberOfArguments==4);						// this is the first line, so this better be true
	traceStartTime[nodeID]=timeStamp;	
//	printf("Trace start time is %d \n",traceStartTime[nodeID]);
	fseek(fileHandle,0,SEEK_SET);		// roll back
}

void clearCounters() {

	int i=0;
	int j=0;
	for (i=0; i<MAX_REGIONX;i++) {
		for (j=0;j<MAX_REGIONY;j++) {
			regionCounter[i][j]=0;
		}
	}
}

void printRegionCounters() {

	int i=0;
	int j=0;
	for (i=0; i<MAX_REGIONX;i++) {
		for (j=0;j<MAX_REGIONY;j++) {
			printf("Region (%d,%d) : Counter : %d \n",i,j,regionCounter[i][j]);
		}
	}
	printf(" \n \n End of time step \n \n"); 
}

int RetrieveGPSFromFile(FILE* fileHandle,int nodeID,coordinate* retrievedCoordinate,int timePeriod) {		// id is known to the calling function since it processes all emulators in serial order 
	// file is already open that's why the fileHandle is directly passed, // Done while establishing the telnet connection itself. 	

//	printf("Time period is %d and trace begins at %d \n",timePeriod,traceStartTime[nodeID]);

	if(feof(fileHandle)) {
//		printf("Returning FEOF \n");
		if(timePeriod>=interpolation[nodeID].endTimeStamp) 	/* exceeded the trace file time */ {
			 // swap the interpolation limits 	
			interpolation[nodeID].startTimeStamp=interpolation[nodeID].endTimeStamp;	// shift the end point to the start point 
			interpolation[nodeID].startLatitude =interpolation[nodeID].endLatitude;
			interpolation[nodeID].startLongitude=interpolation[nodeID].endLongitude;
			return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,2);
		}
		else 									{
                        return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,1);
		}

	}
	

	if(timePeriod < traceStartTime[nodeID]) {
		retrievedCoordinate->latitude=0.0;
		retrievedCoordinate->longitude=0.0;
//		printf("Returning FUNKNOWN \n");
		return -3;								// error code to say that there is no information on it's positon yet. Use this to turn off the GPS in the emulator. 
	}
	
	else if(timePeriod==traceStartTime[nodeID]) {
		int timeStamp;
		float latitude;
		float longitude;
		// not seen any fix yet..  so do the initialization of interpolation here 
		assert(interpolation[nodeID].startTimeStamp==0.0 ) ;
		// This is our first read to the file, so the first line better be a new GPS fix. 
		int numberOfArguments=fscanf(fileHandle,"$GPS-FIX at time %d $node_(%d) new GPS fix longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude);
		assert(numberOfArguments==4);		// this is the first line, so this better be true
	
		// now that you have a sensible first line, use it to initialize the interpolation structures 	
		interpolation[nodeID].startTimeStamp=timeStamp;
		interpolation[nodeID].startLatitude=latitude;
		interpolation[nodeID].startLongitude=longitude;
	
		// read the next line for the endPoint of the interpolation, ie the second line of the file , this can be a normal or a new fix 
		int ret;
		if(feof(fileHandle))  {			// end of file after reading just one line
			GPSState[nodeID]=2;		// for further reads, just assume the start Location is where the node stays for good. 
//			printf("Returning EDF\n");
			interpolation[nodeID].endTimeStamp=timeStamp;// set the end time stamps to be the same too
			interpolation[nodeID].endLatitude=latitude;
			interpolation[nodeID].endLongitude=longitude;
			return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,GPSState[nodeID]);
		}
		assert(!feof(fileHandle));
		long rollBackPoint;
		rollBackPoint=ftell(fileHandle);	// the scanf implementation is so crappy that it won't roll back if it fails mid stream 
		int argumentsRead=fscanf(fileHandle,"$GPS at time %d $node_(%d) longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude);
		if(argumentsRead==4) { // register it if and only if it's a new point
			interpolation[nodeID].endTimeStamp=timeStamp;
			interpolation[nodeID].endLatitude=latitude;
			interpolation[nodeID].endLongitude=longitude;
			GPSState[nodeID]=1;	// from now you can interpolate
//			printf("Returning XYZ\n");
			return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,GPSState[nodeID]);	
		}
		else  {				// the next line is not a GPS line, it has to be a GPS Fix,
			fseek(fileHandle,rollBackPoint,SEEK_SET); 
			numberOfArguments=fscanf(fileHandle,"$GPS-FIX at time %d $node_(%d) new GPS fix longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude);
			assert((numberOfArguments==4));	// this better be a GPS fix or the end of file. 
			//if(!feof(fileHandle)) {	// This has been checked already
			interpolation[nodeID].endTimeStamp=timeStamp;
			interpolation[nodeID].endLatitude=latitude;
			interpolation[nodeID].endLongitude=longitude;
			GPSState[nodeID]=2;	// this is a jump, so use the previous coordinates until the next one  arrives 
//			printf("Returning ZWA\n");
			return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,GPSState[nodeID]);	
		}		
	}

	else if( timePeriod > traceStartTime[nodeID]) {
		int ret;
		int timeStamp;
		float latitude;
		float longitude;
		if((timePeriod <= interpolation[nodeID].endTimeStamp) && (timePeriod >= interpolation[nodeID].startTimeStamp)) {	// lies within the bounds of our current interpolation interval 
			return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,GPSState[nodeID]);	 // retain GPS state as such. 
		}
		else {	// we need to read the next line, and make sure the interpolation is updated to the next two waypoints.
			if(feof(fileHandle)) {		// end of file has been reached and the timePeriod is greater than the interpolation limits, so just return the last value. 
				interpolation[nodeID].startTimeStamp=interpolation[nodeID].endTimeStamp;	// shift the end point to the start point 
				interpolation[nodeID].startLatitude =interpolation[nodeID].endLatitude;
				interpolation[nodeID].startLongitude=interpolation[nodeID].endLongitude;
				
				GPSState[nodeID]=2;		
//				printf("Returning CDE\n");
				return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,2);		// interpolate
			}
			long rollBackPoint=ftell(fileHandle); 
			if(fscanf(fileHandle,"$GPS at time %d $node_(%d) longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude)==4) {
				interpolation[nodeID].startTimeStamp=interpolation[nodeID].endTimeStamp;	// shift the end point to the start point 
				interpolation[nodeID].startLatitude =interpolation[nodeID].endLatitude;
				interpolation[nodeID].startLongitude=interpolation[nodeID].endLongitude;
				
				interpolation[nodeID].endTimeStamp=timeStamp;						// read the next line into the end point. 
				interpolation[nodeID].endLatitude=latitude;
				interpolation[nodeID].endLongitude=longitude;
			
				GPSState[nodeID]=1;										// smooth transition	
//				printf("Returning CSM\n");
				return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,1);		// interpolate
			}
			else {
				fseek(fileHandle,rollBackPoint,SEEK_SET);
			}
			
		        if (fscanf(fileHandle,"$GPS-FIX at time %d $node_(%d) new GPS fix longitude: %f latitude: %f\n",&timeStamp,&nodeID,&longitude,&latitude)==4) {
				interpolation[nodeID].startTimeStamp=interpolation[nodeID].endTimeStamp;	// shift the end point to the start point 
				interpolation[nodeID].startLatitude=interpolation[nodeID].endLatitude;
				interpolation[nodeID].startLongitude=interpolation[nodeID].endLongitude;
			
				interpolation[nodeID].endTimeStamp=timeStamp;						// read the next line into the end point. 
				interpolation[nodeID].endLatitude=latitude;
				interpolation[nodeID].endLongitude=longitude;
				
				GPSState[nodeID]=2;										// jump
//				printf("Returning JUMP\n");
				return Interpolate(interpolation[nodeID],timePeriod,retrievedCoordinate,2);		// interpolate
			}
			else {
				printf("All else failed: Printing the line in the file \n");
				char lineString[100];
				fgets(lineString,100,fileHandle);
				printf("%s",lineString);
			}	
		}	
	}
	printf("Reached the end of function \n");
}
int Interpolate(struct interpolationStructure interpolation, int timePeriod , struct coordinate* retrievedCoordinate,int GPSState)	 {

	int startTimeStamp=interpolation.startTimeStamp;
	float startLatitude =interpolation.startLatitude;
	float startLongitude=interpolation.startLongitude;

	int endTimeStamp =interpolation.endTimeStamp;
	float endLatitude  =interpolation.endLatitude;
	float endLongitude =interpolation.endLongitude;

	if(GPSState==2) {
		// this is a jump, return the last value as is
		retrievedCoordinate->latitude=startLatitude;
		retrievedCoordinate->longitude=startLongitude;
		return 2;
	}

	else {
		if(endTimeStamp==startTimeStamp) {
			retrievedCoordinate->latitude=endLatitude;
			retrievedCoordinate->longitude=endLongitude;
			return 0; 
		}
		retrievedCoordinate->latitude=startLatitude  + ((float)(endLatitude-startLatitude)/(float)(endTimeStamp-startTimeStamp)) *(float) (timePeriod-startTimeStamp);
		retrievedCoordinate->longitude=startLongitude+ ((float)(endLongitude-startLongitude)/(float)(endTimeStamp-startTimeStamp))*((float)timePeriod-startTimeStamp);
		return 0; 
	}	
}

int InitializeInterpolationStructures(int numberOfClients) {
	// 0 out all the entries to start out with . It may be doing this by default, but no risks. 
	int i=0;
	for (i=0; i<numberOfClients; i++ ) {
		interpolation[i].startTimeStamp=0;
		interpolation[i].startLatitude=0.0;
		interpolation[i].startLongitude=0.0;
		interpolation[i].endTimeStamp=0;
		interpolation[i].endLatitude=0.0;
		interpolation[i].endLongitude=0.0;
		GPSState[i]=-1;			// no fix received yet 
		traceStartTime[i]=-1;		// initializie this while opening the file. 
	}

}
void printCoordinates(coordinate C) {
	printf("%f,%f \n",C.latitude,C.longitude);
}
int main(int argc, char** argv) {
	FILE *fileHandle[2000];			// one for each node 
	long minLatitude,minLongitude,regionWidth;
	if(argc < 6 ) {
		printf("Usage : Enter number of nodes and trace folder, minLatitude,minLongitude, regionWidth \n");
		exit(1);
	}
	int numberOfNodes=atoi(argv[1]);
	minLatitude=atoi(argv[3]);
	minLongitude=atoi(argv[4]);
	regionWidth=atoi(argv[5]);

	printf ("The number of Nodes is %d ",numberOfNodes);
	InitializeInterpolationStructures(numberOfNodes);
	char traceFolder[100];
	strcpy(traceFolder,argv[2]);
//	printf("Trace Folder  is %s \n",traceFolder);
	int j;
	for (j=0;j<numberOfNodes;j++) {
		char fileName[100];
		strcpy(traceFolder,argv[2]);
		sprintf(fileName,"%s/%d.gps",traceFolder,j);
//		printf("FileName is %s \n",fileName);
		fileHandle[j]=fopen(fileName,"r");
		if(fileHandle[j]==NULL) {
			printf("Could not open file \n");
			return 1;
		}
		InitializeStartOfTraceTime(fileHandle[j]);
	}
	int retValue;
	struct coordinate retrievedCoordinate;
	int timeInstant=0;
	
	do {
	     int regionX,regionY;
	     int unassigned=0;
	     int outofBounds=0;
	     // clear all counters 
	     clearCounters();
	     int i;
	     for(i=0;i<numberOfNodes;i++) {
		retValue=RetrieveGPSFromFile(fileHandle[i],i,&retrievedCoordinate,timeInstant);
//		printf("Return value is %d \n",retValue);
	        if(retValue != -3) {
//			printf("Scaled coordinates at time %d for node %d are %d,%d \n",timeInstant,i,(long)(retrievedCoordinate.longitude*10000),(long)(retrievedCoordinate.latitude*10000));
			regionX=(long)(((retrievedCoordinate.longitude)*100000 - minLongitude)/regionWidth);
			regionY=(long)(((retrievedCoordinate.latitude)*100000  -minLatitude)/regionWidth);
			if((regionX >=0 && regionX <MAX_REGIONX) && (regionY>=0 && regionY < MAX_REGIONY)) {
				regionCounter[regionX][regionY]++;
			}
			else  {
				outofBounds++;
				fprintf(stderr,"At time %d Node %d out of bounds (%f,%f) \n",timeInstant,i,retrievedCoordinate.latitude,retrievedCoordinate.longitude);
			}
		}
		else  {
			unassigned++;
		}
	      }
	    	// end of all nodes
		printf(" At time step % d , unassigned nodes : %d , out of bound nodes : %d \n", timeInstant,unassigned,outofBounds);
		printRegionCounters(); 
		timeInstant=timeInstant+1;
	}
	while(timeInstant<=900) ;	// size of trace 
}
