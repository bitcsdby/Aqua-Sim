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
//#include "underwatersensor/uw_common/uw_hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "uw_datastructure.h"



struct vbf_neighborhood{
  int number;
  position neighbor[MAX_NEIGHBOR];
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

       position original_source;
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
  vbf_neighborhood* GetHash(ns_addr_t sender_id, unsigned int pkt_num);
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

class UWDelayTimer: public Handler{
public:
UWDelayTimer(VectorbasedforwardAgent * a):Handler(){a_=a;}
 void handle(Event* e);
protected :
  VectorbasedforwardAgent * a_;
  
};


class VectorbasedforwardAgent : public Agent {
 public:
 	//constructor fun and two interface fun
  VectorbasedforwardAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  // Vectorbasedforward_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int pk_count;
  int counter;
  int hop_by_hop;
  double priority;
  bool measureStatus;
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
  double projection(Packet*);
  double calculateDelay(Packet*, position*);
  //double recoveryDelay(Packet*, position*);
  void calculatePosition(Packet*);
   void setMeasureTimer(Packet*,double);
  bool IsTarget(Packet*);
  bool IsCloseEnough(Packet*);
 
 
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




