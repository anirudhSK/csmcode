#!/bin/python

MAX_ROWS = 10
MAX_COLS = 10
FREE_SPOTS = 10

class LogParkingFile:
	m_version = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	m_num_nodes = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]	
	global_free_spaces = [[0]*MAX_COLS for x in xrange(3)]
	region_leader = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	is_region_alive = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	has_active_leader = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	election_time = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	log_init = False
	global_accesses = 0
	
	#CSM
	central_g_seq = [[0]*MAX_COLS for x in xrange(MAX_ROWS)]
	central_l_seq = [[{}]*MAX_COLS for x in xrange(MAX_ROWS)] # empty dictionary at every entry in 2d array / list
	central_m_seq_acks = central_g_seq = [[{}]*MAX_COLS for x in xrange(MAX_ROWS)]
	central_m_write_updates = [[{}]*MAX_COLS for x in xrange(MAX_ROWS)]
	
	@staticmethod
	def init():
		if log_init:
			return
		print "******************* LogParkingFile ****"
		LogParkingFile.global_accesses = 0
		for i in range(MAX_ROWS):
			for j in range(MAX_COLS):
				LogParkingFile.global_free_spaces[i][j] = FREE_SPOTS
				LogParkingFile.m_version[i][j] = 0
				LogParkingFile.m_num_nodes[i][j] = 0
				LogParkingFile.is_region_alive[i][j] = False
				LogParkingFile.has_active_leader[i][j] = False
				LogParkingFile.region_leader[i][j] = -1
				LogParkingFile.election_time[i][j] = 0.0
				
				if(CSM == 1):
					LogParkingFile.central_g_seq[i][j] = 0;
					for row in range(MAX_ROWS):
						for col in range(MAX_COLS):
							pair<int, int> reg(row, col);
							central_l_seq[i][j][reg] = 0;
				
		log_init = True;
		return
	
	@staticmethod
	def getFreeSpots(row, col):
		return LogParkingFile.global_free_spaces[row][col]
	
	@staticmethod
	def getVersion(row, col):
		return LogParkingFile.m_version[row][col]
	
	@staticmethod
	def getNumNodes(row, col):
		return LogParkingFile.m_num_nodes[row][col]
	
	@staticmethod
	def setNumNodes(row, col, val):
		LogParkingFile.m_num_nodes[row][col] += val
		
	@staticmethod
	def isLeaderActive(row, col):
		return LogParkingFile.has_active_leader[row][col]
	
	@staticmethod
	def setLeaderActive(row, col, status, time):
		if(status):
			if(LogParkingFile.election_time[row][col] != 0):
				print "ELECTION TIME = %f\n" % (time - LogParkingFile.election_time[row][col])
			LogParkingFile.election_time[row][col] = time
		else:
			print "STABLE TIME = %f" % (time - LogParkingFile.election_time[row][col])
			LogParkingFile.election_time[row][col] = time
		LogParkingFile.has_active_leader[row][col] = status
		return
	
	@staticmethod
	def isRegionActive(row, col):
		LogParkingFile.global_accesses += 1
		print "GLOBAL ACCESS = %d\n" % (LogParkingFile.global_accesses)
	 	return LogParkingFile.is_region_alive[row][col];
	
	@staticmethod
	def setRegionActive(row, col, node_id):
		assert(LogParkingFile.is_region_alive[row][col] == False);
		LogParkingFile.is_region_alive[row][col] = True;
		LogParkingFile.region_leader[row][col] = node_id;
		return True;
	
	@staticmethod
	def setRegionInActive(row, col, node_id, pass_): # changed pass to pass_ to avoid keyword conflict
		LogParkingFile.global_accesses += 1
		print "GLOBAL ACCESS = %d" % (LogParkingFile.global_accesses)
		# assert(region_leader[row][col] == node_id)
		assert(LogParkingFile.is_region_alive[row][col] == True)
		LogParkingFile.is_region_alive[row][col] = False
		return True
	
	@staticmethod
	def incrementFreeSpots(row, col):
		LogParkingFile.global_free_spaces[row][col] += 1
		LogParkingFile.m_version[row][col] += 1
	
	@staticmethod
	def decrementFreeSpots(row, col):
		LogParkingFile.global_free_spaces[row][col] -= 1
		LogParkingFile.m_version[row][col] += 1
