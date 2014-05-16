#ifndef ns_uw_sink_vbva_h
#define ns_uw_sink_vbva_h

#include <tcl.h>

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "uw_hash_table.h"
#include "underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedvoidavoidance.h"

class UW_VBVA_SinkAgent;

// Timer for packet rate

class UW_VBVA_Sink_Timer : public TimerHandler {
 public:
	UW_VBVA_Sink_Timer(UW_VBVA_SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UW_VBVA_SinkAgent *a_;
};


// For periodic report of avg and var pkt received.

class UW_VBVA_Report_Timer : public TimerHandler {
 public:
	UW_VBVA_Report_Timer(UW_VBVA_SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UW_VBVA_SinkAgent *a_;
};


// Timer for periodic interest

class UW_VBVA_Periodic_Timer : public TimerHandler {
 public:
    UW_VBVA_Periodic_Timer(UW_VBVA_SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UW_VBVA_SinkAgent *a_;
};


// Class SinkAgent as source and sink for directed diffusion

class UW_VBVA_SinkAgent : public Agent {

 public:
  UW_VBVA_SinkAgent();
  int command(int argc, const char*const* argv);
  virtual void timeout(int);

  void report();
  void recv(Packet*, Handler*);
  void reset();
  void set_addr(ns_addr_t);
  int get_pk_count();
  void incr_pk_count();
  Packet *create_packet();

 protected:
  static int pkt_id_;
  bool APP_DUP_;
  bool periodic_;
  //bool always_max_rate_;
  int pk_count;
  //  unsigned int data_type_;
  int num_recv;
  int num_send;
  //  int RecvPerSec; //? what's this for

  /*used ti indicate if the sink is active, send out interest first or 
passive, it gets the data ready and then sends out the interest. 1 is passive 
and 0 is active.*/
  
  int passive;


 
  ns_addr_t target_id;
  double target_x;
  double target_y;
  double target_z;
  double range_;
  char   f_name[80];

  UnderwaterSensorNode* node;
 
  double cum_delay;
  double last_arrival_time;

  UW_Hash_Table DataTable;
  bool IsDeviation();
  void Terminate();
  void bcast_interest();
  void source_deny(ns_addr_t,double,double,double);
  void data_ready();
  void start();
   void generateInterval();
  void exponential_start();
  void stop();
  virtual void sendpkt();

  int running_;
  int random_;
  int maxpkts_;

  double interval_; // interval to send data pkt
  double explore_interval;
  double data_interval;
  double  data_rate_;
  

 int packetsize_;  // # of bytes in the packet
  int explore_rate;
  int data_counter;
  int  explore_counter;
  int explore_status;

  
  UW_VBVA_Sink_Timer sink_timer_;
  UW_VBVA_Periodic_Timer periodic_timer_;
  UW_VBVA_Report_Timer report_timer_;
  friend class UW_VBVA_Periodic_Timer;
};


#endif











