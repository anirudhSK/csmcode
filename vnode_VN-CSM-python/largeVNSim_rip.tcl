# TCL script used to start VNSim
# largeVNSim_aodv.tcl is used to start a VN based Adhoc Ondemand Routing algorithm in a 8 by 8 region network
# Jiang Wu, jwu1@gc.cuny.edu
# 06/04/2008

# parameters
set val(sync) 1 ;#synchronization enabled or not

# ======================================================================
# Define wireless channel options
# ======================================================================

set val(chan)       Channel/WirelessChannel
set val(prop)       Propagation/TwoRayGround
set val(netif)      Phy/WirelessPhy
set val(mac)        Mac/802_11
set val(ifq)        Queue/DropTail/PriQueue
set val(ll)         LL
set val(ant)        Antenna/OmniAntenna
set val(x)              700   	;# X dimension of the topography
set val(y)              700   	;# Y dimension of the topography
set val(rows)		8	;# rows of regions
set val(columns)	8	;# columns of regions 
set val(ifqlen)         250      ;# max # of packets in ifq
set val(seed)           0.0
set val(adhocRouting)   DumbAgent				;# broadcast packet passed directly to application layer  
set val(nn)             4					;# how many nodes are simulated
set val(sc)             "../../mobility/scene/paths-700-4-static"	;# mobility trace 
set val(cp)             "../../mobility/scene/cbr-3-test"	;# background traffic
#set val(stop)           200.0           			;# simulation time
set val(stop)           5000.0           			;# simulation time

set val(myTraceFile)	"output"
set val(sn_size)	10000	;#size of message sending sessions
set val(num_srcs)	15


# =====================================================================
# Main Program
# ======================================================================

# create simulator instance 
# with only IP, Join, VNS, LL, Mac and ARP messages allowed in the packet header

remove-all-packet-headers
add-packet-header IP Join VNS LL Mac ARP VNCOMMON

set ns_		[new Simulator]

# setup topography object

set topo	[new Topography]

# create trace object for ns and nam

set tracefd	[open join.tr w]
#set namtrace    [open join.nam w]

$ns_ trace-all $tracefd
#$ns_ namtrace-all-wireless $namtrace $val(x) $val(y)

#delete the existing tracefile if it exists
file delete $val(myTraceFile)

# define topology
$topo load_flatgrid $val(x) $val(y)

#
# Create God
#
set god_ [create-god $val(nn)]

#
# Creat Channel
#
set chan_1_ [new $val(chan)]


#global node setting
$ns_ node-config  -adhocRouting $val(adhocRouting) \
                  -llType $val(ll) \
                  -macType $val(mac) \
                  -ifqType $val(ifq) \
                  -ifqLen $val(ifqlen) \
                  -antType $val(ant) \
                  -propType $val(prop) \
                  -phyType $val(netif) \
          	  -topoInstance $topo \
          	  -agentTrace OFF \
                  -routerTrace OFF \
                  -macTrace OFF \
          	  -mobilityTrace OFF \
          	  -channel $chan_1_


#Agent/VNSAgent/ParkingServer set update_interval 1
Agent/VNSAgent/ParkingServer set complete_update_interval 60

#
#  Create the specified number of nodes [$val(nn)] and "attach" them
#  to the channel. 
for {set i 0} {$i < $val(nn) } {incr i} {
     set node_($i) [$ns_ node $i]
     $node_($i) random-motion 0        ;# disable random motion
}

# Start every node with Agents
for {set i 0} {$i < $val(nn)} {incr i} {

        set join_($i) [new Agent/Join]
	set vns_($i)  [new Agent/VNSAgent/ParkingServer]
	set vnc_($i)  [new Agent/VNCAgent/ParkingClient]
	
	#set the port number to be used by the JOIN agent
	$node_($i) attach $join_($i) [$join_($i) set port_number_]
	$join_($i) set nodeID_ $i
	$join_($i) set slowInterval_ 1.0	
	$join_($i) set interval_ 1.0
	$join_($i) set maxX_ $val(x)
	$join_($i) set maxY_ $val(y)
	$join_($i) set rows_ $val(rows)
	$join_($i) set columns_ $val(columns)

	$ns_ at 0.0 "$join_($i) set-traceFileName $val(myTraceFile)"
	

	$vns_($i) set nodeID_ $i
	$vns_($i) set total_ordering_enabled_ 1
	$vns_($i) set total_ordering_mode_ 1
	#disable state synchronization between leaders and non-leaders
	$vns_($i) set sync_enabled_ $val(sync)
	

	$vns_($i) set rows_ $val(rows)
	$vns_($i) set columns_ $val(columns)
	
	

	#set the port number to be used by the VNS agent
	
	$node_($i) attach $vns_($i) [$vns_($i) set port_number_]
	
	$join_($i) add-app-port [$vns_($i) set port_number_]
	
	$join_($i) add-app-port [$vnc_($i) set port_number_]	

	$ns_ at 0.0 "$vns_($i) set-traceFileName $val(myTraceFile)"
	$ns_ at 0.0 "$vns_($i) start"	
	
	#set the port number to be used by the VNC agent
	#set port [$vnc_($i) set port_number_] 
	#puts "$i : $port"
	
	$node_($i) attach $vnc_($i) [$vnc_($i) set port_number_]
	$vnc_($i) set nodeID_ $i
	
	$vnc_($i) set max_node_id_ $val(nn)
	
	$vnc_($i) set sn_size_ $val(sn_size)
	$vnc_($i) set num_srcs_ $val(num_srcs)
	$vnc_($i) set clock_msg_enabled_ 1
	
	$ns_ at 0.0 "$vnc_($i) set-traceFileName $val(myTraceFile)"
	$ns_ at 0.0 "$vnc_($i) start"	
	
	#start everything from the join agent
	$ns_ at 0.0 "$join_($i) start"
}


# 
# Define node movement model
#
puts "Loading scenario file..."
source $val(sc)

# 
# Define traffic model
#
#puts "Loading connection pattern..."
#source $val(cp)

# Define node initial position in nam

for {set i 0} {$i < $val(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined
    
    $ns_ initial_node_pos $node_($i) 20
}

#
# Tell nodes when the simulation ends
#
for {set i 0} {$i < $val(nn) } {incr i} {
    $ns_ at $val(stop).0 "$node_($i) reset";
}

$ns_ at  $val(stop).0002 "puts \"NS EXITING...\" ; $ns_ halt"

#puts $tracefd "M 0.0 nn $val(nn) x $val(x) y $val(y) rp $val(adhocRouting)"
#puts $tracefd "M 0.0 sc $val(sc) cp $val(cp) seed $val(seed)"
#puts $tracefd "M 0.0 prop $val(prop) ant $val(ant)"

set time_start [clock seconds]

puts "Starting Simulation..."

$ns_ run

set time_end [clock seconds]

puts "time elapsed"
puts [expr $time_end - $time_start]

