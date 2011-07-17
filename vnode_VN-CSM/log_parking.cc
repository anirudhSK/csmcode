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
#include "log_parking.h"

 int LogParkingFile::getFreeSpots(int row, int col)
{
	int free_spots;
	free_spots = global_free_spaces[row][col];
	return free_spots;
}

 int LogParkingFile::getVersion(int row, int col)
{
	int ver;
	ver = m_version[row][col];
	return ver;
}

 int LogParkingFile::getNumNodes(int row, int col)
{
	int n;
	n = m_num_nodes[row][col];
	return n;
}

 void LogParkingFile::setNumNodes(int row, int col, int val)
{
	m_num_nodes[row][col] += val;
}

 bool LogParkingFile::isLeaderActive(int row, int col)
{
	bool is_active;
	is_active = has_active_leader[row][col];
	return is_active;
}

 void LogParkingFile::setLeaderActive(int row, int col, bool status, double time)
{
	if(status)
	{
		if(election_time[row][col] != 0)
			printf("ELECTION TIME = %f\n", time - election_time[row][col]);
		election_time[row][col] = time;
	}
	else
	{
		printf("STABLE TIME = %f\n", time - election_time[row][col]);
		election_time[row][col] = time;
	}	
	has_active_leader[row][col] = status;
}

 bool LogParkingFile::isRegionActive(int row, int col)
{
	bool is_active;
	is_active = is_region_alive[row][col];
	global_accesses++;
	printf("GLOBAL ACCESS = %d\n", global_accesses);
	return is_active;
}

 bool LogParkingFile::setRegionActive(int row, int col, int node_id)
{
	assert(is_region_alive[row][col] == false);
	is_region_alive[row][col] = true;
	region_leader[row][col] = node_id;
	return true;
}

 bool LogParkingFile::setRegionInActive(int row, int col, int node_id, int pass)
{
	global_accesses++;
	printf("GLOBAL ACCESS = %d\n", global_accesses);
//	assert(region_leader[row][col] == node_id);
	assert(is_region_alive[row][col] == true);
	is_region_alive[row][col] = false;
	return true;
}

 void LogParkingFile::incrementFreeSpots(int row, int col)
{
	global_free_spaces[row][col]++;
	m_version[row][col]++;
}

 void LogParkingFile::decrementFreeSpots(int row, int col)
{
	global_free_spaces[row][col]--;
	m_version[row][col]++;
}
