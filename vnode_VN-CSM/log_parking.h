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
 *log.h logging functions*/

#ifndef ns_log_parking_h
#define ns_log_parking_h

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <cassert>
#include <vector>
#include <utility>
#include <map>
#include <queue>
#include "header.h"

#define MAX_ROWS 10
#define MAX_COLS 10
#define FREE_SPOTS 10

using namespace std;

class LogParkingFile{
	public:
		static void init()
		{
			if(log_init)
				return;
			printf("******************* LogParkingFile ****\n");
			global_accesses = 0;
			for(int i = 0; i < MAX_ROWS; i++)
			{
				for(int j = 0; j < MAX_COLS; j++)
				{
					global_free_spaces[i][j] = FREE_SPOTS;
					m_version[i][j] = 0;
					m_num_nodes[i][j] = 0;
					is_region_alive[i][j] = false;
					has_active_leader[i][j] = false;
					region_leader[i][j] = -1;
					election_time[i][j] = 0.0;

					if(CSM == 1)
					{
						central_g_seq[i][j] = 0;
						for(int row = 0; row < MAX_ROWS; row++)
						{
							for(int col = 0; col < MAX_COLS; col++)
							{		
								pair<int, int> reg(row, col);
								central_l_seq[i][j][reg] = 0;
							}
						}	
					}
				}
			}
			log_init = true;
		}

		static bool isLeaderActive(int row, int col);
		static void setLeaderActive(int row, int col, bool status, double time);
	
		static int getFreeSpots(int row, int col);
		static int getVersion(int row, int col);

		static int getNumNodes(int row, int col);
		static void setNumNodes(int row, int col, int val);

		static bool isRegionActive(int row, int col);
		static bool setRegionActive(int row, int col, int node_id);
		static bool setRegionInActive(int row, int col, int node_id, int pass);

		static void incrementFreeSpots(int row, int col);
		static void decrementFreeSpots(int row, int col);	
	
		static int m_version[MAX_ROWS][MAX_COLS];	
		static int m_num_nodes[MAX_ROWS][MAX_COLS];	
		static int global_free_spaces[MAX_ROWS][MAX_COLS];
		static int region_leader[MAX_ROWS][MAX_COLS];
		static bool is_region_alive[MAX_ROWS][MAX_COLS];
		static bool has_active_leader[MAX_ROWS][MAX_COLS];
		static bool log_init;
		static int global_accesses;
		static double election_time[MAX_ROWS][MAX_COLS];

		//CSM
		static int central_g_seq[MAX_ROWS][MAX_COLS];
		static map<pair<int, int>, int> central_l_seq[MAX_ROWS][MAX_COLS];
		static map<int, vector<pair<int, int> > > central_m_seq_acks[MAX_ROWS][MAX_COLS];
		static map<pair<int, int>, priority_queue<WriteUpdate> > central_m_write_updates[MAX_ROWS][MAX_COLS];
};

#endif 
