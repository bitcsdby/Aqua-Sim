#ifndef ns_vectorbasedvoidavoidance_h
#define ns_vectorbasedvoidavoidance_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
//#include <signal.h>
//#include <float.h>
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
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "uw_routing_buffer.h"
#include "uw_datastructure.h"

typedef struct Neighbornode{
      routing_vector    vec;
      position node;   
      unsigned int forwarder_id;
      unsigned int status;
 
} neighbornode;


typedef struct Neighborhood{
  int number;
  neighbornode  neighbor[MAX_NEIGHBOR];
} neighborhood;


// the same as hdr_uwvb, just for isolation
struct hdr_uwvbva{
	unsigned int mess_type;
	unsigned int pk_num;
        ns_addr_t target_id; // the target id  of this data 
        ns_addr_t sender_id;  //original sender id
         // nsaddr_t next_nodes[MAX_NEIGHBORS];
        //int      num_next;
        unsigned int data_type;
        ns_addr_t forward_agent_id;// the forwarder id

       struct uw_extra_info info;
       position original_source;
    //  double token;
  	double ts_;                       // Timestamp when pkt is generated.
        double range;    // target range
	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_uwvbva* access(const Packet*  p) {
		return (hdr_uwvbva*) p->access(offset_);
	}
};


class UWVBVAPkt_Hash_Table {
 public:
  Tcl_HashTable htable;

  UWVBVAPkt_Hash_Table() {
    window_size=WINDOW_SIZE;
    Tcl_InitHashTable(&htable, 3);
  }

  int  window_size;
  void reset();
  void delete_hash(hdr_uwvbva*); //delete the enrty that has the same key as the new packet
  void delete_hash(ns_addr_t, unsigned); 

  void MarkNextHopStatus(ns_addr_t, unsigned int,unsigned int, unsigned int);

  void put_in_hash(hdr_uwvbva *);
  void put_in_hash(hdr_uwvbva *, const position *, const position*, const position*, unsigned int=FRESHED);
  neighborhood* GetHash(ns_addr_t sender_id, unsigned int pkt_num);
};


class UWVBVAData_Hash_Table {
 public:
  Tcl_HashTable htable;

  UWVBVAData_Hash_Table() {
    Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }

  void reset();
  void delete_hash(ns_addr_t, unsigned int);
  void put_in_hash(ns_addr_t, unsigned int,unsigned int);
  unsigned int  *GetHash(ns_addr_t, unsigned int);
};



class VectorbasedVoidAvoidanceAgent;

class UWDelayHandler: public Handler{
public:
UWDelayHandler(VectorbasedVoidAvoidanceAgent * a):Handler(){a_=a;}
void handle(Event* e);
protected :
  VectorbasedVoidAvoidanceAgent * a_;
  
};

class UWVoidAvoidHandler: public Handler{
public:
UWVoidAvoidHandler(VectorbasedVoidAvoidanceAgent * a):Handler(){a_=a;}
void handle(Event* e);
protected :
  VectorbasedVoidAvoidanceAgent * a_;
};


// no use right now
/*
class UWFloodingHandler: public Handler{
public:
UWFloodingHandler(VectorbasedforwardAgent * a):Handler(){a_=a;}
 void handle(Event*);
protected :
  VectorbasedforwardAgent * a_;
};
*/


class UWFloodingBackwardHandler: public Handler{
public:
UWFloodingBackwardHandler(VectorbasedVoidAvoidanceAgent * a):Handler(){a_=a;}
 void handle(Event*);
protected :
  VectorbasedVoidAvoidanceAgent * a_;
};


class UWFloodingForwardHandler: public Handler{
public:
UWFloodingForwardHandler(VectorbasedVoidAvoidanceAgent * a):Handler(){a_=a;}
 void handle(Event*);
protected :
  VectorbasedVoidAvoidanceAgent * a_;
};


// no use right now
/*
class UWBackwardFloodingPacketHandler: public Handler{
public:
UWBackwardFloodingPacketHandler(VectorbasedforwardAgent * a):Handler(){a_=a;}
 void handle(Event*);
protected :
  VectorbasedforwardAgent * a_;
};
*/



class VectorbasedVoidAvoidanceAgent : public Agent {
 public:
  VectorbasedVoidAvoidanceAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  double position_update_time;
  int pk_count;
  int counter;
  double priority;
  const double mini_distance;// distance used for flooding packet delay
  const double mini_threshold;// desirablenss used for normal data packet delay
  bool measureStatus;  //?? where do I use this?
  int control_packet_size;
 
  // int port_number;
  UWVBVAPkt_Hash_Table PktTable;
  UWVBVAPkt_Hash_Table SourceTable;
  UWVBVAPkt_Hash_Table Target_discoveryTable;
  UWVBVAPkt_Hash_Table SinkTable;

  UWVBVAData_Hash_Table PacketStatusTable;


 
  //delete later 
  UWVBVAPkt_Hash_Table CenterPktTable;
  UWVBVAPkt_Hash_Table DataTerminationPktTable; 

  UWDelayHandler forward_delay_handler; 
  UWVoidAvoidHandler void_avoidance_handler;
  //  UWFloodingHandler  flooding_handler;// no use right now 
  UWFloodingForwardHandler self_centered_forward_handler;
  UWFloodingBackwardHandler backpressure_handler;
  //UWBackwardFloodingPacketHandler backwardflooding_packet_handler;// no use right now
  
  RoutingBuffer void_avoidance_buffer;
  //  RoutingBuffer receiving_buffer;
  
  UnderwaterSensorNode *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;
  double width; 
// the width is used to test if the node is close enough to the path specified by the packet  
  

   inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void Terminate();
  void reset();
  void ConsiderNew(Packet*);

  void setForwardDelayTimer(Packet*,double);
  //void set_shift_timer(ns_addr_t,int,double);
  void set_shift_timer(Packet*,double);
  // void set_flooding_timer(ns_addr_t,int,double);
//  void set_flooding_forward_timer(Packet*, double);

 
  void recordPacket(hdr_uwvbva*, unsigned int=FRESHED);
  //  void recordPacket(hdr_uwvb*);
  void process_flooding_timeout(Packet*);// no use right now 
  void process_selfcentered_timeout(Packet*);
  void process_void_avoidance_timeout(Packet*);
  void process_backpressure_timeout(Packet*);
  void process_void_avoidance_timeout(ns_addr_t,position*,position*,int);
  void process_forward_timeout(Packet*);
  void process_backwardflooding_packet_timeout(Packet*);// no use right

  //void processFloodingPacket(Packet*);

  void processBackpressurePacket(Packet*);
  void processCenteredPacket(Packet*);
  void processBackFloodedPacket(Packet*);
 
  void makeCopy(Packet*);
  void sendFloodingPacket(Packet*); // delete this function later
  // void sendVectorShiftPacket(ns_addr_t, int);
  void sendDataTermination(const Packet*);

  double advance(Packet *);
  double distance(const Packet *);
  double distance(const position*, const position*);

  double projection(Packet*);
  double projection(const position*, const position*, const position *);
  double calculateMappedDistance(const position*, const position*, const position*);
  double calculateDelay(Packet*, position*);
  double calculateDelay(const position*,const position*,const position*, const position*);

  double calculateSelfCenteredDelay(const position*,const position*, const position*, const position*);

  double calculateFloodingDesirableness(const Packet*);// no use right now
  double calculateDesirableness(const Packet*);
  double calculateBackFloodDelay(const position*,const position*, const position*, const position*);// no use right now

  Packet* generateVectorShiftPacket(const ns_addr_t*, int,const position*, const position*);

 Packet* generateControlDataPacket(Packet*,unsigned int);
  //  Packet* generateBackpressurePacket(const ns_addr_t*,int);
  Packet* generateBackpressurePacket(Packet*);
  void calculatePosition(Packet*);
  void setMeasureTimer(Packet*,double);

  bool IsStuckNode(const neighbornode*,const position*,int,unsigned int);
  bool IsWorthFloodingForward(ns_addr_t,int);// useless??
  //bool IsWorthFloodingForward(ns_addr_t,int);
  bool IsVoidNode(ns_addr_t,int,const position*);
  bool IsUpstreamNode(const position&,const position&, const position&);
  bool IsVoidNode(ns_addr_t,int);
  bool IsEndNode(ns_addr_t,int);
  bool IsNewlyTouchedNode(ns_addr_t, unsigned int);
  bool IsTarget(Packet*);
  bool IsCloseEnough(Packet*);
  bool IsSamePosition(const position*, const position*);
  bool IsControlMessage(const Packet*);
 
//  Packet *create_packet();
//  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  
  void DataForSink(Packet *pkt);
  // void StopSource();
  void MACprepare(Packet *pkt);
  void MACsend(Packet *pkt, Time delay=0);

  void trace(char *fmt,...);
  friend class UWDelayHandler;

  friend class UWVoidAvoidHandler;
  // friend class UWFloodingHandler;
  friend class UWFloodingForwardHandler;
  friend class UWFloodingBackwardHandler;
};



#endif




