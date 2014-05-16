
#ifndef ns_uwflooding_h
#define ns_uwflooding_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>

#include "agent.h"
#include "tclcl.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"
#include "underwatersensor/uw_routing/uw_datastructure.h"
//#include "arp.h"
//#include "mac.h"
//#include "ll.h"
//#include "dsr/path.h"


//#define THIS_NODE             here_
//#define THIS_NODE             here_.addr_
//#define JITTER                0.01       // (sec) to jitter broadcast
//#define DELAY                 0.01 //the timeout for electing source


#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)


// Vectorbasedforward  Entry



class UWFloodingAgent;


// Vectorbasedforward Agent

class UWFloodingAgent : public Agent {
 public:
  UWFloodingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int pk_count;
  // int port_number;
  UWPkt_Hash_Table PktTable;
  UWPkt_Hash_Table SourceTable;
  UWPkt_Hash_Table Target_discoveryTable;
  UWPkt_Hash_Table SinkTable;
  //UWDelayTimer delaytimer; 
  
  MobileNode *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;
 
  

   inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void Terminate();
  void reset();
  void ConsiderNew(Packet *pkt);
  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  
  void DataForSink(Packet *pkt);
  void StopSource();
  void MACprepare(Packet *pkt);
  void MACsend(Packet *pkt, Time delay=0);

  void trace(char *fmt,...);
};



#endif




