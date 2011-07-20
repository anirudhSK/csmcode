#include<stdlib.h>
#include<stdio.h>
#define NUMBER_OF_EMULATOR_CLIENTS 500	// 500 is the maximum we plan to simulate anyway. 
typedef struct coordinate {
	float latitude;
	float longitude;
}coordinate;

typedef struct interpolationStructure {
	int startTimeStamp;
	float startLatitude;
	float startLongitude;
	int endTimeStamp;
	float endLatitude;
	float endLongitude;
}interpolationStructure;

int RetrieveGPSFromFile(FILE* fileHandle,int nodeID, coordinate* retrievedCoordinate,int timePeriod) ;

// function headers 
void printCoordinates(coordinate C);
int InitializeInterpolationStructures(int numberOfClients) ;
int Interpolate(struct interpolationStructure interpolationStructure, int timePeriod , struct coordinate* retrievedCoordinate,int GPSState) ;
void InitializeStartOfTraceTime(FILE* fileHandle) ;
