set opt(chan)		Channel/UnderwaterChannel
set opt(prop)		Propagation/UnderwaterPropagation
set opt(netif)		Phy/UnderwaterPhy
set opt(mac)		Mac/UnderwaterMac/BroadcastMac
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(energy)         EnergyModel
set opt(txpower)        0.6
set opt(rxpower)        0.3
set opt(initialenergy)  10000
set opt(idlepower)      0.01
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater

set opt(ifqlen)		50	;# max queue length in if
set opt(nn)		12	;# number of nodes in each layer
set opt(x)		400	;# X dimension of the topography
set opt(y)	        400  ;# Y dimension of the topography
set opt(z)              400
set opt(seed)		12
set opt(stop)		100	;# simulation time
set opt(prestop)        50     ;# time to prepare to stop
set opt(tr)		"vbva_example.tr"	;# trace file
set opt(nam)            "vbva_example.nam"  ;# nam file
set opt(adhocRouting)   VectorbasedVoidAvoidance
set opt(width)           100
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

Agent/VectorbasedVoidAvoidance set width 1

Mac/UnderwaterMac set bit_rate_  1.0e4
Mac/UnderwaterMac set encoding_efficiency_  1
Mac/UnderwaterMac/BroadcastMac set packetheader_size_  0  ;# # of bytes

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/UnderwaterPhy set CPThresh_ 2  ;#10.0
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
#$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)


set total_number [expr $opt(nn)-1]
set god_ [create-god $opt(nn)]

set chan_1_ [new $opt(chan)]



$ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
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
$node_(0) set Y_  10
$node_(0) set Z_   0.0
$node_(0) set passive 1
set a_(0) [new Agent/UW_VBVA_Sink]
$ns_ attach-agent $node_(0) $a_(0)
$a_(0) attach-routing
$a_(0) cmd set-range 20
$a_(0) cmd set-target-x -20
$a_(0) cmd set-target-y -10
$a_(0) cmd set-target-z -20
$a_(0) cmd set-packetsize  40 ;# # of bytes  



set node_(1) [$ns_  node 1]
$node_(1) set sinkStatus_ 1
$god_ new_node $node_(1)
$node_(1) set X_  -10
$node_(1) set Y_  50
$node_(1) set Z_   0
$node_(1) set-cx  -10
$node_(1) set-cy  50
$node_(1) set-cz  0
#$node_(1) set passive 1
set a_(1) [new Agent/UW_VBVA_Sink]
$ns_ attach-agent $node_(1) $a_(1)
$a_(1) attach-routing
 $a_(1) cmd set-range 20
 $a_(1) cmd set-target-x   0
 $a_(1) cmd set-target-y   0
 $a_(1) cmd set-target-z   0
 $a_(1) cmd set-packetsize  40 ;# # of bytes
 $a_(1) set data_rate_ 4




 set node_(2) [$ns_  node 2]
 $node_(2) set sinkStatus_ 1
 $god_ new_node $node_(2)
 $node_(2) set X_    -10
 $node_(2) set Y_    90
 $node_(2) set Z_   0
 $node_(2) set-cx   -10
 $node_(2) set-cy   90
 $node_(2) set-cz   0
 set a_(2) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(2) $a_(2)
 $a_(2) attach-routing
 $a_(2) cmd set-range 20
 $a_(2) cmd set-target-x   0
 $a_(2) cmd set-target-y   0
 $a_(2) cmd set-target-z   0
 $a_(2) cmd set-packetsize  40 ;# 40 bytes
 $a_(2) set data_rate_ 4




 set node_(3) [$ns_  node 3]
 $node_(3) set sinkStatus_ 1
 $god_ new_node $node_(3)
 $node_(3) set X_    20
 $node_(3) set Y_    180
 $node_(3) set Z_   0
 $node_(3) set-cx   20
 $node_(3) set-cy   180
 $node_(3) set-cz   0
 set a_(3) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(3) $a_(3)
 $a_(3) attach-routing
 $a_(3) cmd set-range 20
 $a_(3) cmd set-target-x   0
 $a_(3) cmd set-target-y   0
 $a_(3) cmd set-target-z   0
 $a_(3) cmd set-packetsize  40 ;# 40 bytes
 $a_(3) set data_rate_ 4




 set node_(4) [$ns_  node 4]
 $node_(4) set sinkStatus_ 1
 $god_ new_node $node_(4)
 $node_(4) set X_    100
 $node_(4) set Y_    210
 $node_(4) set Z_   0
 $node_(4) set-cx   100
 $node_(4) set-cy   210
 $node_(4) set-cz   0
 set a_(4) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(4) $a_(4)
 $a_(4) attach-routing
 $a_(4) cmd set-range 20
 $a_(4) cmd set-target-x   0
 $a_(4) cmd set-target-y   0
 $a_(4) cmd set-target-z   0
 $a_(4) cmd set-packetsize  40 ;# 40 bytes
 $a_(4) set data_rate_ 4


 set node_(5) [$ns_  node 5]
 $node_(5) set sinkStatus_ 1
 $god_ new_node $node_(5)
 $node_(5) set X_    150
 $node_(5) set Y_    250
 $node_(5) set Z_   0
 $node_(5) set-cx   150
 $node_(5) set-cy   250
 $node_(5) set-cz   0
 set a_(5) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(5) $a_(5)
 $a_(5) attach-routing
 $a_(5) cmd set-range 20
 $a_(5) cmd set-target-x   0
 $a_(5) cmd set-target-y   0
 $a_(5) cmd set-target-z   0
 $a_(5) cmd set-packetsize  40 ;# 40 bytes
 $a_(5) set data_rate_ 4


 set node_(6) [$ns_  node 6]
 $node_(6) set sinkStatus_ 1
 $god_ new_node $node_(6)
 $node_(6) set X_    200
 $node_(6) set Y_    200
 $node_(6) set Z_   0
 $node_(6) set-cx   200
 $node_(6) set-cy   200
 $node_(6) set-cz   0
 set a_(6) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(6) $a_(6)
 $a_(6) attach-routing
 $a_(6) cmd set-range 20
 $a_(6) cmd set-target-x   0
 $a_(6) cmd set-target-y   0
 $a_(6) cmd set-target-z   0
 $a_(6) cmd set-packetsize  40 ;# 40 bytes
 $a_(6) set data_rate_ 4


 set node_(7) [$ns_  node 7]
 $node_(7) set sinkStatus_ 1
 $god_ new_node $node_(7)
 $node_(7) set X_    200
 $node_(7) set Y_    150
 $node_(7) set Z_   0
 $node_(7) set-cx   200
 $node_(7) set-cy   150
 $node_(7) set-cz   0
 set a_(7) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(7) $a_(7)
 $a_(7) attach-routing
 $a_(7) cmd set-range 20
 $a_(7) cmd set-target-x   0
 $a_(7) cmd set-target-y   0
 $a_(7) cmd set-target-z   0
 $a_(7) cmd set-packetsize  40 ;# 40 bytes
 $a_(7) set data_rate_ 4




 set node_(8) [$ns_  node 8]
 $node_(8) set sinkStatus_ 1
 $god_ new_node $node_(8)
 $node_(8) set X_    250
 $node_(8) set Y_    50
 $node_(8) set Z_   0
 $node_(8) set-cx   250
 $node_(8) set-cy   50
 $node_(8) set-cz   0
 set a_(8) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(8) $a_(8)
 $a_(8) attach-routing
 $a_(8) cmd set-range 20
 $a_(8) cmd set-target-x   0
 $a_(8) cmd set-target-y   0
 $a_(8) cmd set-target-z   0
 $a_(8) cmd set-packetsize  40 ;# 40 bytes
 $a_(8) set data_rate_ 4



#dead path nodes
 
set node_(9) [$ns_  node 9]
 $node_(9) set sinkStatus_ 1
 $god_ new_node $node_(9)
 $node_(9) set X_    150
 $node_(9) set Y_    80
 $node_(9) set Z_   0
 $node_(9) set-cx   150
 $node_(9) set-cy   80
 $node_(9) set-cz   0
 set a_(9) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(9) $a_(9)
 $a_(9) attach-routing
 $a_(9) cmd set-range 20
 $a_(9) cmd set-target-x   0
 $a_(9) cmd set-target-y   0
 $a_(9) cmd set-target-z   0
 $a_(9) cmd set-packetsize  40 ;# 40 bytes
 $a_(9) set data_rate_ 4




 set node_(10) [$ns_  node 10]
 $node_(10) set sinkStatus_ 1
 $god_ new_node $node_(10)
 $node_(10) set X_    100
 $node_(10) set Y_    100
 $node_(10) set Z_   0
 $node_(10) set-cx   100
 $node_(10) set-cy   100
 $node_(10) set-cz   0
 set a_(10) [new Agent/UW_VBVA_Sink]
 $ns_ attach-agent $node_(10) $a_(10)
 $a_(10) attach-routing
 $a_(10) cmd set-range 20
 $a_(10) cmd set-target-x   0
 $a_(10) cmd set-target-y   0
 $a_(10) cmd set-target-z   0
 $a_(10) cmd set-packetsize  40 ;# 40 bytes
 $a_(10) set data_rate_ 4



#puts "the total number is $total_number"
set node_($total_number) [$ns_  node $total_number]
$god_ new_node $node_($total_number)
$node_($total_number) set X_  200
$node_($total_number) set Y_  0
$node_($total_number) set Z_  0
$node_($total_number) set-cx  200
$node_($total_number) set-cy  0
$node_($total_number) set-cz  0



set a_($total_number) [new Agent/UW_VBVA_Sink]
$ns_ attach-agent $node_($total_number) $a_($total_number)
$a_($total_number) attach-routing
 $a_($total_number) cmd set-range 100
 $a_($total_number) cmd set-target-x 0

 $a_($total_number) cmd set-target-y 0
 $a_($total_number) cmd set-target-z 0
 $a_($total_number) cmd set-packetsize  40 ;# 40 bytes
$a_($total_number) set data_rate_ 0.1;# #of packet per sec


# make nam workable
set node_size 10
for {set k 0} { $k<$opt(nn)} {incr k} {
$ns_ initial_node_pos $node_($k) $node_size
}


$ns_ at 2 "$a_($total_number) cbr-start"



puts "+++++++AFTER ANNOUNCE++++++++++++++"


$ns_ at $opt(stop).001 "$a_(0) terminate"
$ns_ at $opt(prestop).002 "$a_($total_number) stop"
$ns_ at $opt(stop).002 "$a_($total_number) terminate"


$ns_ at $opt(stop).003  "$god_ compute_energy"
$ns_ at $opt(stop).004  "$ns_ nam-end-wireless $opt(stop)"
$ns_ at $opt(stop).005 "puts \"NS EXISTING...\"; $ns_ halt"

 puts $tracefd "vectorbased"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."
 $ns_ run
