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

#ifndef ns_log_h
#define ns_log_h

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <cassert>
#include <vector>
#include <utility>

using namespace std;

class LogFile{
	public:
		FILE * fp;
		pthread_mutex_t mut; //initialize it as PTHREAD_MUTEX_INITIALIZER;

		/* All accesses and modifications to "fp" should be bracketed by calls to
		 * pthread_mutex_lock and pthread_mutex_unlock as follows:
		 *
		 *		pthread_mutex_lock(&mut);
		 *		// operations on fp
		 *		pthread_mutex_unlock(&mut);
		 *
		 */

		void createFilePointer(char *);
		//log a message string and release the memory.
		void printMsgEvent(char *, char*, double, int, int, int, int, char*, char*);
		//log a text string without worrying about freeing the memory used
		void printInfoEvent(char *, char*, double, int, int, int, int, char*, char*);
		//log a value
		void printInfoEvent(char *, char*, double, int, int, int, int, char*, double);
		void inline init()
		{
			pthread_mutex_init (&mut, NULL);
		}
};

#endif // ns_log_h
