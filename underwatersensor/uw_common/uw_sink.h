
#ifndef ns_uw_sink_h
#define ns_uw_sink_h

#include <tcl.h>

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "uw_hash_table.h"
#include "underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"



class UWSinkAgent;

// Timer for packet rate

class UWSink_Timer : public TimerHandler {
 public:
	UWSink_Timer(UWSinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UWSinkAgent *a_;
};


// For periodic report of avg and var pkt received.

class UWReport_Timer : public TimerHandler {
 public:
	UWReport_Timer(UWSinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UWSinkAgent *a_;
};


// Timer for periodic interest

class UWPeriodic_Timer : public TimerHandler {
 public:
	UWPeriodic_Timer(UWSinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	UWSinkAgent *a_;
};


// Class SinkAgent as source and sink for directed diffusion

class UWSinkAgent : public Agent {

 public:
  UWSinkAgent();
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
  bool APP_DUP_;
  bool periodic_;
  static int pkt_id_;  
  //bool always_max_rate_;
  int pk_count;
//  unsigned int data_type_;
  int num_recv;
  int num_send;
  // int RecvPerSec; //? what's this for

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

  UW_Hash_Table  DataTable;

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
  int random_;   //1 is random; 2 is exponential distribution
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
 
  //int simple_report_rate;
    //  int data_counter;
 
  UWSink_Timer sink_timer_;
  UWPeriodic_Timer periodic_timer_;
  UWReport_Timer report_timer_;

  friend class UWPeriodic_Timer;
};


#endif











