from header import *
from parkingserver import *
from parkingclient import *
from join import *

class Val(object):
	sync = 1
#	chan =        Channel/WirelessChannel
#	prop =        Propagation/TwoRayGround
#	netif =       Phy/WirelessPhy
#	mac =         Mac/802_11
#	ifq =         Queue/DropTail/PriQueue
#	ll =          LL
#	ant =         Antenna/OmniAntenna
	x =			700   	# X dimension of the topography
	y =			700   	# Y dimension of the topography
	rows =			8	# rows of regions
	columns =		8	# columns of regions 
	ifqlen =		250      # max # of packets in ifq
	seed =			0.0
#	adhocRouting =    DumbAgent				# broadcast packet passed directly to application layer  
	nn =			4					# how many nodes are simulated
	sc =			"../../mobility/scene/paths-700-4-static"	# mobility trace 
	cp =			"../../mobility/scene/cbr-3-test"	# background traffic
	stop =			200.0           			# simulation time
	stop =			5000.0           			# simulation time
	myTraceFile =	"output"
	sn_size =		10000	#size of message sending sessions
	num_srcs =		15


class VNode(object):
	def __init__(self, id):
		self.id = id
		
		self.join_agent = JoinAgent(self.id, 1.0, 1.0, Val.x, Val.y, Val.rows, Val.columns)
		self.join_agent.regionX_ = 1
		self.join_agent.regionY_ = 0
		self.join_agent.seq = 1
		self.join_agent.m_version = 1
		
		self.parking_server_agent = ParkingServerAgent(self.id, 1, 1, Val.sync, Val.rows, Val.columns)
		self.parking_client_agent = ParkingClientAgent(self.id, Val.nn, Val.sn_size, Val.num_srcs, 1)
		
	
	def recv(self, pkt):
		self.join_agent.recv(pkt)
		self.parking_server_agent.recv(pkt)
		self.parking_client_agent.recv(pkt)
		
	
	def print_status(self):
		print '### NODE %d: join_agent.state_synced_: %s' % (self.id, str(self.join_agent.state_synced_))
		print '### NODE %d: join_agent.leader_start_: %s' % (self.id, str(self.join_agent.leader_start_))
		print '### NODE %d: join_agent.leader_status_: %s' % (self.id, str(self.join_agent.leader_status_))
		print '\n'