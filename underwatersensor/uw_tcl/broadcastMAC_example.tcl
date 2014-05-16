set opt(chan)		Channel/UnderwaterChannel
set opt(prop)		Propagation/UnderwaterPropagation
set opt(netif)		Phy/UnderwaterPhy
set opt(mac)		Mac/UnderwaterMac/BroadcastMac
set opt(ifq)		Queue/DropTail
set opt(ll)		LL
set opt(energy)         EnergyModel
set opt(txpower)        1
set opt(rxpower)        1
set opt(initialenergy)  10000
set opt(idlepower)      0
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter

set opt(dz)             10
set opt(ifqlen)		50	;# max packet in ifq
set opt(nn)		4	;# number of nodes in each layer
set opt(layers)         1
set opt(x)		100	;# X dimension of the topography
set opt(y)	        100  ;# Y dimension of the topography
set opt(z)              [expr ($opt(layers)-1)*$opt(dz)]
set opt(datarate)       0.1
set opt(seed)		0        ;# random seed
set opt(stop)		600	;# simulation time
set opt(prestop)        20     ;# time to prepare to stop
set opt(tr)		"broadcastmac.tr"	;# trace file
set opt(nam)            "broadcastmac.nam"  ;# nam file
set opt(datafile)	 "broadcastmac.data";# data file
set opt(adhocRouting)   Vectorbasedforward
set opt(width)           20
set opt(adj)             10
set opt(interval)        0.001

# ==================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

#Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Z_ 0.05
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0



Mac/UnderwaterMac set bit_rate_  1.0e4
Mac/UnderwaterMac set encoding_efficiency_  1
Mac/UnderwaterMac/BroadcastMac set packet_size_  568




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


remove-all-packet-headers 
#remove-packet-header AODV ARP TORA  IMEP TFRC
add-packet-header IP Mac LL  ARP  UWVB RMAC


set ns_ [new Simulator]
set topo  [new Topography]

$topo load_cubicgrid $opt(x) $opt(y) $opt(z)
$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)

set data [open $opt(datafile) a]

set total_number [expr $opt(nn)-1]
set god_ [create-god $opt(nn)]

set chan_1_ [new $opt(chan)]

$ns_ at 0.0 "$god_  set_filename $opt(datafile)"


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
		 -agentTrace OFF \
                 -routerTrace OFF \
                 -macTrace OFF\
                 -topoInstance $topo\
                 -energyModel $opt(energy)\
                 -txPower $opt(txpower)\
                 -rxPower $opt(rxpower)\
                 -initialEnergy $opt(initialenergy)\
                 -idlePower $opt(idlepower)\
                 -channel $chan_1_


set node_(0) [$ns_  node 0]
$node_(0) set sinkStatus_ 1
$node_(0) set passive 1
    
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
$a_(0) cmd set-filename $opt(datafile)


set node_(1) [$ns_  node 1]
$node_(1) set sinkStatus_ 1
$god_ new_node $node_(1)
$node_(1) set X_  -80
$node_(1) set Y_   0
$node_(1) set Z_   0
$node_(1) set-cx  -80
$node_(1) set-cy  0
$node_(1) set-cz  0
$node_(1) set_next_hop 0 ;# target is node 0 
#$node_(1) set passive 1
set a_(1) [new Agent/UWSink]
$ns_ attach-agent $node_(1) $a_(1)
$a_(1) attach-vectorbasedforward $opt(width)
 $a_(1) cmd set-range 20
 $a_(1) cmd set-target-x   0
 $a_(1) cmd set-target-y   0
 $a_(1) cmd set-target-z   0
 $a_(1) set data_rate_ $opt(datarate)
$a_(1) cmd set-filename $opt(datafile)


set node_(2) [$ns_  node 2]
$node_(2) set sinkStatus_ 1
$god_ new_node $node_(2)
$node_(2) set X_  80
$node_(2) set Y_  0
$node_(2) set Z_   0
$node_(2) set-cx  80
$node_(2) set-cy  0
$node_(2) set-cz  0
$node_(2) set_next_hop 0 ;# target is node 0 
#$node_(1) set passive 1
set a_(2) [new Agent/UWSink]
$ns_ attach-agent $node_(2) $a_(2)
$a_(2) attach-vectorbasedforward $opt(width)
 $a_(2) cmd set-range 20
 $a_(2) cmd set-target-x   0
 $a_(2) cmd set-target-y   0
 $a_(2) cmd set-target-z   0
 $a_(2) set data_rate_ $opt(datarate)
$a_(2) cmd set-filename $opt(datafile)

#puts "the total number is $total_number"
set node_($total_number) [$ns_  node $total_number]
$god_ new_node $node_($total_number)
$node_($total_number) set X_  0
$node_($total_number) set Y_  50
$node_($total_number) set Z_  0
$node_($total_number) set-cx  0
$node_($total_number) set-cy  40
$node_($total_number) set-cz  0
$node_($total_number) set_next_hop 0 ;# target is node 0 

set a_($total_number) [new Agent/UWSink]
$ns_ attach-agent $node_($total_number) $a_($total_number)
$a_($total_number) attach-vectorbasedforward $opt(width)
 $a_($total_number) cmd set-range 20
 $a_($total_number) cmd set-target-x 0
 $a_($total_number) cmd set-target-y 0
 $a_($total_number) cmd set-target-z 0
 $a_($total_number) set data_rate_ $opt(datarate)
 $a_($total_number) cmd set-filename $opt(datafile)




#set max_num [expr $total_number -1]



#$ns_ at 15 "$a_($total_number) cbr-start"
$ns_ at 20 "$a_($total_number) exp-start"
$ns_ at 20 "$a_(1) exp-start"
#$ns_ at 20 "$a_(2) exp-start"
$ns_ at 20 "$a_(2) cbr-start"

#$ns_ at 4 "$a_(0) cbr-start"
#$ns_ at 2.0003 "$a_(2) cbr-start"

#$ns_ at 0.1 "$a_(0) announce"


puts "+++++++AFTER ANNOUNCE++++++++++++++"





$ns_ at $opt(stop).001 "$a_(0) terminate"
$ns_ at $opt(stop).002 "$a_($total_number) terminate"
$ns_ at $opt(stop).003 "$a_(1) terminate"
$ns_ at $opt(stop).004 "$a_(2) terminate"
#$ns_ at $opt(stop).002 "$a_(3) terminate"


$ns_ at $opt(stop).05  "$god_ compute_energy"
$ns_ at $opt(stop).05  "$ns_ nam-end-wireless $opt(stop)"
$ns_ at $opt(stop).05 "puts \"NS EXISTING...\"; $ns_ halt"

 puts $tracefd "vectorbased"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."

 puts  $data  "New simulation...."
 close $data

 $ns_ run
