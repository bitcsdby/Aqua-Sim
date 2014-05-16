set opt(chan)		Channel/UnderwaterChannel
set opt(prop)		Propagation/UnderwaterPropagation
set opt(netif)		Phy/UnderwaterPhy
set opt(mac)		Mac/UnderwaterMac/BroadcastMac
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(energy)         EnergyModel
set opt(txpower)        1.0
set opt(rxpower)        0.2
set opt(initialenergy)  10000
set opt(idlepower)      0.0001
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter



set opt(maxspeed)            20
set opt(minspeed)            10
set opt(speed)               5
set opt(position_update_interval) 0.2

set opt(ifqlen)		50	;# max packet in ifq
set opt(nn)		4	;# number of nodes in each layer
set opt(x)		100	;# X dimension of the topography
set opt(y)	        100  ;# Y dimension of the topography
set opt(z)              100
set opt(seed)		1.88
set opt(stop)		100	;# simulation time
set opt(prestop)        90    ;# time to prepare to stop
set opt(tr)		"hop_hop_vbf.tr"	;# trace file
set opt(nam)            "hop_hop_vbf.nam"  ;# nam file
set opt(adhocRouting)   Vectorbasedforward
set opt(width)           20
set opt(adj)             10
set opt(interval)        0.001

# ==================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Z_ 0.05
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

#set the mode of Vectorbasedforward as hop by hop
Agent/Vectorbasedforward set hop_by_hop_ 1

Mac/UnderwaterMac set bit_rate_  1.0e4  ;#10kbps
Mac/UnderwaterMac set encoding_efficiency_  1
Mac/UnderwaterMac/BroadcastMac set packet_size_  400 ;# bits 

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/UnderwaterPhy set CPThresh_ 10  ;#10.0
Phy/UnderwaterPhy set CSThresh_ 0  ;#1.559e-11
Phy/UnderwaterPhy set RXThresh_ 0   ;#3.652e-10
#Phy/UnderwaterPhy set Rb_ 2*1e6
Phy/UnderwaterPhy set Pt_ 0.2818
Phy/UnderwaterPhy set freq_ 25  ;#frequency range in khz 
Phy/UnderwaterPhy set K_ 2.0   ;#spherical spreading

# ==================================================================
# Main Program
# =================================================================

#
# Initialize Global Variables
# 
#set sink_ 1
set ns_ [new Simulator]
set topo  [new Topography]

$topo load_cubicgrid $opt(x) $opt(y) $opt(z)
$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y) 


set total_number [expr $opt(nn)-1]
set god_ [create-god $opt(nn)]

set chan_1_ [new $opt(chan)]


global defaultRNG
$defaultRNG seed $opt(seed)



$ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 #-channelType $opt(chan) \
		 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace ON\
                 -topoInstance $topo\
                 -energyModel $opt(energy)\
                 -txPower $opt(txpower)\
                 -rxPower $opt(rxpower)\
                 -initialEnergy $opt(initialenergy)\
                 -idlePower $opt(idlepower)\
                 -channel $chan_1_
                 

set node_(0) [$ns_  node 0]
#puts "after create underwater sensor node\n"
$node_(0) set sinkStatus_ 1
$god_ new_node $node_(0)
$node_(0) set X_  0
$node_(0) set Y_  0
$node_(0) set Z_   0.0
$node_(0) set passive 1
set a_(0) [new Agent/UWSink]
$ns_ attach-agent $node_(0) $a_(0)
$a_(0) attach-vectorbasedforward $opt(width)
 $a_(0) cmd set-range 20
 $a_(0) cmd set-target-x -20
 $a_(0) cmd set-target-y -10
 $a_(0) cmd set-target-z -20


set node_(1) [$ns_  node 1]
$node_(1) set sinkStatus_ 1
$god_ new_node $node_(1)
$node_(1) set X_  60
$node_(1) set Y_  5
$node_(1) set Z_   0
$node_(1) set-cx  60
$node_(1) set-cy  5
$node_(1) set-cz  0

# make  node 1 to fail
$node_(1) set-failure_status 1 
set a_(1) [new Agent/UWSink]
$ns_ attach-agent $node_(1) $a_(1)
$a_(1) attach-vectorbasedforward $opt(width)
 $a_(1) cmd set-range 20
 $a_(1) cmd set-target-x   0
 $a_(1) cmd set-target-y   0
 $a_(1) cmd set-target-z   0
 $a_(1) set interval_ 4




set node_(2) [$ns_  node 2]
$node_(2) set sinkStatus_ 1
$god_ new_node $node_(2)
$node_(2) set X_  20
$node_(2) set Y_  10
$node_(2) set Z_  0
$node_(2) set-cx  20
$node_(2) set-cy  10
$node_(2) set-cz  0
#$node_(2) set passive 1
set a_(2) [new Agent/UWSink]
$ns_ attach-agent $node_(2) $a_(2)
$a_(2) attach-vectorbasedforward $opt(width)
 $a_(2) cmd set-range 20
 $a_(2) cmd set-target-x   0
 $a_(2) cmd set-target-y   0
 $a_(2) cmd set-target-z   0
 $a_(2) set interval_ 4





#puts "the total number is $total_number"
set node_($total_number) [$ns_  node $total_number]
$god_ new_node $node_($total_number)
$node_($total_number) set X_  -20
$node_($total_number) set Y_  10
$node_($total_number) set Z_  0
$node_($total_number) set-cx  -20
$node_($total_number) set-cy  10
$node_($total_number) set-cz  0


set a_($total_number) [new Agent/UWSink]
$ns_ attach-agent $node_($total_number) $a_($total_number)
$a_($total_number) attach-vectorbasedforward $opt(width)
 $a_($total_number) cmd set-range 20
 $a_($total_number) cmd set-target-x 0
 $a_($total_number) cmd set-target-y 0
 $a_($total_number) cmd set-target-z 0
 $a_($total_number) set interval_ 4



# make node 2  mobile

$node_($total_number) set max_speed $opt(maxspeed)
$node_($total_number) set min_speed $opt(minspeed)

$node_($total_number) set position_update_interval_ $opt(position_update_interval)
$ns_  at 1.2 "$node_($total_number) move"


$ns_ at 2 "$a_($total_number) cbr-start"
$ns_ at 2 "$a_(2) cbr-start"
#$ns_ at 2.0003 "$a_(2) cbr-start"

#$ns_ at 0.1 "$a_(0) announce"


puts "+++++++AFTER ANNOUNCE++++++++++++++"





$ns_ at $opt(stop).001 "$a_(0) terminate"
$ns_ at $opt(stop).001 "$a_(2) terminate"
$ns_ at $opt(stop).002 "$a_($total_number) terminate"




$ns_ at $opt(stop).003  "$god_ compute_energy"
$ns_ at $opt(stop).004  "$ns_ nam-end-wireless $opt(stop)"
$ns_ at $opt(stop).005 "puts \"NS EXISTING...\"; $ns_ halt"

 puts $tracefd "vectorbased"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."
 $ns_ run
