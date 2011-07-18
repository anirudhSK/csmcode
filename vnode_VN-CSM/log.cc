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
 * log.cc logging functions for virtual node system and applications
 */
#include "log.h"

//create a file pointer for writing
void LogFile::createFilePointer(char* filename)
{
	//check if the file already exist
	if((fp = fopen(filename, "r")))
	{
		fclose(fp);
	    fp = fopen(filename,"a");
	}
	else
	    fp = fopen(filename,"w");
}



/**
 ** log a message string and release the memory.
 **/
void LogFile::printMsgEvent(char *filename, char* agentType, double time, int regionX, int regionY, int nodeID, int leadingStatus, char * eventType, char *messageString )
{
	/**
	 ** agent type, time, region, node status, node id, event type, message type, message string
	 **/
	//check if the file already exist
	//FILE * fp;

	char * leaderStat = "";

	switch(leadingStatus)
	{
		case -1: leaderStat = "Unknown"; break;
		case 0: leaderStat = "Requesting"; break;
		case 1:	leaderStat = "Leader"; break;
		case 2: leaderStat = "Non-leader";break;
		case 3: leaderStat = "Unstable";break;
		case 4: leaderStat = "Client";break;
		case 5: leaderStat = "External";break;
		default: leaderStat = "Dead";break;
	}



	//if((fp = fopen(filename, "r")))
	//{
	//	fclose(fp);
	//    fp = fopen(filename,"a");
	//}
	//else
	//    fp = fopen(filename,"w");
	if(leadingStatus != 6 && (leadingStatus != 2 || strncmp(eventType, "SEND", 4)==0))
	{
		pthread_mutex_lock(&mut);//lock the LogFile ojbect
		fprintf(fp, "%s,%.4f,(%d.%d),%s,%d,%s,%s\n",agentType, time, regionX, regionY, leaderStat, nodeID, eventType, messageString);
		pthread_mutex_unlock(&mut);//unlock the LogFile object
	}

	delete messageString;

	//fclose(fp);
}


/**
 ** log a text string without worrying about freeing the memory used
 **/
void LogFile::printInfoEvent(char *filename, char* agentType, double time, int regionX, int regionY, int nodeID, int leadingStatus, char * eventType, char *messageString )
{
	/**
	 ** agent type, time, region, node status, node id, event type, message type, message string
	 **/
	//check if the file already exist
	//FILE * fp;

	char * leaderStat = "";

	switch(leadingStatus)
	{
		case -1: leaderStat = "Unknown"; break;
		case 0: leaderStat = "Requesting"; break;
		case 1:	leaderStat = "Leader"; break;
		case 2: leaderStat = "Non-leader";break;
		case 3: leaderStat = "Unstable";break;
		case 4: leaderStat = "Client";break;
		case 5: leaderStat = "External";break;
		default: leaderStat = "Dead";break;
	}

	/*if((fp = fopen(filename, "r")))
	{
		fclose(fp);
	    fp = fopen(filename,"a");
	}
	else
	    fp = fopen(filename,"w");
	    */
	if(leadingStatus != 6 )//&& leadingStatus != 2)
	{
		pthread_mutex_lock(&mut);//lock the LogFile ojbect
		fprintf(fp, "%s,%.4f,(%d.%d),%s,%d,%s,%s\n",agentType, time, regionX, regionY, leaderStat, nodeID, eventType, messageString);
		pthread_mutex_unlock(&mut);//unlock the LogFile object
	}
	//fclose(fp);
}

/**
 ** log a value
 **/
void LogFile::printInfoEvent(char *filename, char* agentType, double time, int regionX, int regionY, int nodeID, int leadingStatus, char * eventType, double value )
{
	/**
	 ** agent type, time, region, node status, node id, event type, message type, message string
	 **/
	//check if the file already exist
	//FILE * fp;

	char * leaderStat = "";

	switch(leadingStatus)
	{
		case -1: leaderStat = "Unknown"; break;
		case 0: leaderStat = "Requesting"; break;
		case 1:	leaderStat = "Leader"; break;
		case 2: leaderStat = "Non-leader";break;
		case 3: leaderStat = "Unstable";break;
		case 4: leaderStat = "Client";break;
		case 5: leaderStat = "External";break;
		default: leaderStat = "Dead";break;
	}

	/*if((fp = fopen(filename, "r")))
	{
		fclose(fp);
	    fp = fopen(filename,"a");
	}
	else
	    fp = fopen(filename,"w");
	    */

	if(leadingStatus != 6)// && leadingStatus != 2)
	{
		pthread_mutex_lock(&mut);//lock the LogFile ojbect
		fprintf(fp, "%s,%.4f,(%d.%d),%s,%d,%s,%lf\n",agentType, time, regionX, regionY, leaderStat, nodeID, eventType, value);
		pthread_mutex_unlock(&mut);//unlock the LogFile object
	}
	//fclose(fp);
}
