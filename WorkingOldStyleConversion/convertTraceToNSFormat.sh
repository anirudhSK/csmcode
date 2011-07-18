#! /bin/sh
# Take the log file
# split it three ways column wise using cut
# process the column of GPS coordinatees using GeoConvert and save this
# merge the files with diff side-by-side

fileName=$1	# ie the file to be converted.
xStart=$2	# The lower limit on x coordinates
yStart=$3	# The lower limit on y coordinates
xEnd=$4;	# The upper limit on x coordinates
yEnd=$5;	# The upper limit on y coordinates
timeStart=$4;	# The starting time of all the traces. 
timeEnd=$5;	# The ending time of all traces

# Below the values are hard coded for the time being. 
fileName="sample.txt"
#fileName="logs_20100801_20100802.csv"
timeStart="02/08/2010-16:59:00";	
timeEnd="02/08/2010-20:01:00";	# to start we try and gather two hours of traces.
xStart=369863;
yStart=138126;
xEnd=376160;
yEnd=148556;
#GeoConvert=
# crude hack, but hey it works :)

perl parseTrace.pl $fileName $timeStart $timeEnd > arrangedSamples.txt   
							 	    # create unique and tractable IDs, and create reasonable times relative to the start of day, for the entire original log file
								    # Also invert the order of the GPS coordinates in the original data File 
cut -f 3-4 arrangedSamples.txt > gpsCoordinates.txt     	    # get GPS coordinates alone

GeoConvert -u < gpsCoordinates.txt > UTMCoordinates.txt		    # convert to planar coordinates 

# put these UTM coordinates as required back into the GPS coordinate locations
cut -f 1-2 arrangedSamples.txt > carIDs.txt
cut -f 5 arrangedSamples.txt > speed.txt
paste carIDs.txt UTMCoordinates.txt speed.txt > UTMTrace.txt


perl convertFromUTM2NS.pl UTMTrace.txt $xStart $yStart $xEnd $yEnd > nsTrace.txt # correct the UTM Coordinates by the offset and also do the required setdest manipulation 

## To DO:
##.

## 
# parseTrace.pl extracts relevant information from the original trace file
# First, the GPD coordinates are extracted, and outputted in the right order
# This is fed into the GeoConvert utility to generate UTM coordinates and store this as a separate file.
# While extracting the GPS coordinates, the time Offset is subtracted from all the trace entries and the hash values are converted into something tractable.
#Now, while subtracting the UTM offset from the UTM coordinates, also convert it into setdest format.   
