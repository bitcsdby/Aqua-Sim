#ifndef ns_vectorbasedforward_h
#define ns_vectorbasedforward_h

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
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"


#define THIS_NODE             here_
//#define THIS_NODE             here_.addr_
#define JITTER                0.08       // (sec) to jitter broadcast
#define DELAY                 0.8 // maximum delay that a node holdes a packet
#define DELAY2                0.01
//#define DELAY3                0.12

#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)

#define INTEREST      1
#define DATA          2
#define DATA_READY    3
#define SOURCE_DISCOVERY 4
#define SOURCE_TIMEOUT   5
#define TARGET_DISCOVERY 6
#define TARGET_REQUEST 7
#define SOURCE_DENY  8




//#define DATA_READY    3 // not used yet
//#define DATA_REQUEST  4 // not used yet
//#define POS_REINFORCE 5 //not used yet
//#define NEG_REINFORCE 6 //not used yet
//#define INHIBIT       7 //not used yet
//#define TX_FAILED     8 //not used yet
//#define DATA_STOP     9 

#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30
#define MAX_NEIGHBOR 10
#define WINDOW_SIZE  19 
//used by hash table to limited the maximum length



#define ROUTING_PORT 255

//#define ORIGINAL    100        
//#define SUB_SAMPLED 1         

// For positive reinforcement in simple mode. 
// And for TX_FAILED of backtracking mode.


struct position{
  double x;
  double y;
  double z;
};

struct neighborhood{
  int number;
  position neighbor[MAX_NEIGHBOR];
};


struct uw_extra_info {

  // ns_addr_t osender_id;            // The original sender of this message
  // unsigned int seq;           //  sequence number

  double ox;  // the original sender's position
  double oy;
  double oz;

  //ns_addr_t sender_id;            // The forwarder of this message

  double fx;  // the original sender's position
  double fy;
  double fz;

  double tx;
  double ty;
  double tz;

// this is the information about relative position of the receiver to the forwarder
  double dx;
  double dy;
  double dz; 

};


struct hdr_uwvb{
	unsigned int mess_type;
	unsigned int pk_num;
        ns_addr_t target_id; // the target id  of this data 
        ns_addr_t sender_id;  //original sender id
  // nsaddr_t next_nodes[MAX_NEIGHBORS];
  //int      num_next;
        unsigned int data_type;
        ns_addr_t forward_agent_id;// the forwarder id

       struct uw_extra_info info;
        
         double token;
  	double ts_;                       // Timestamp when pkt is generated.
      double range;    // target range
  //    int    report_rate;               // For simple diffusion only.
  //	int    attr[MAX_ATTRIBUTE];
	
	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_uwvb* access(const Packet*  p) {
		return (hdr_uwvb*) p->access(offset_);
	}
};





class UWPkt_Hash_Table {
 public:
  Tcl_HashTable htable;

  UWPkt_Hash_Table() {
    window_size=WINDOW_SIZE;
    //  lower_counter=0;
// 50 items in the hash table, however, because it begins by 0, so, minus 1
    Tcl_InitHashTable(&htable, 3);
  }

  int  window_size;
  void reset();
  void put_in_hash(hdr_uwvb *);
  void put_in_hash(hdr_uwvb *, position *);
  neighborhood* GetHash(ns_addr_t sender_id, unsigned int pkt_num);
  //private:
  //int lower_counter;
};


class UWData_Hash_Table {
 public:
  Tcl_HashTable htable;

  UWData_Hash_Table() {
    Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }

  void reset();
  void PutInHash(int *attr);
  Tcl_HashEntry  *GetHash(int *attr);
};



class VectorbasedforwardAgent;

class UWDelayTimer: public TimerHandler{
public:
UWDelayTimer(VectorbasedforwardAgent * a):TimerHandler(){a_=a;}
Packet * packet;
protected :
virtual void expire(Event* e);
  VectorbasedforwardAgent * a_;
  
};


class VectorbasedforwardAgent : public Agent {
 public:
  VectorbasedforwardAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int pk_count;
  int counter;
  double priority;
  double minProjection;

  bool measureStatus;
  int  forwarder_based;
  // int port_number;
  UWPkt_Hash_Table PktTable;
  UWPkt_Hash_Table SourceTable;
  UWPkt_Hash_Table Target_discoveryTable;
  UWPkt_Hash_Table SinkTable;
  UWDelayTimer delaytimer; 
  

  
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
  void ConsiderNew(Packet *pkt);
  void set_delaytimer(Packet*,double);
  void timeout(Packet*);
  double advance(Packet *);
  double distance(Packet *);
  double projection(Packet*, bool);
  double projection(const position*, const position*,const position *);
  double calculateDelay(Packet*, position*);
  //double recoveryDelay(Packet*, position*);
  void calculatePosition(Packet*);
   void setMeasureTimer(Packet*,double);
  bool IsTarget(Packet*);
  bool IsCloseEnough(Packet*, bool);
 
 
  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  
  void DataForSink(Packet *pkt);
  void StopSource();
  void MACprepare(Packet *pkt);
  void MACsend(Packet *pkt, Time delay=0);

  void trace(char *fmt,...);
  friend class UWDelayTimer;
};



#endif




