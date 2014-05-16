#ifndef ns_uw_datastructure_h
#define ns_uw_datastructure_h


#define THIS_NODE             here_
#define JITTER                0.1    //use to generate random 
#define DELAY                 1.0
//#define DELAY2                0.01

#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)

// mess types
#define INTEREST      1
#define DATA          2
#define DATA_READY    3
#define SOURCE_DISCOVERY 4
#define SOURCE_TIMEOUT   5
#define TARGET_DISCOVERY 6
#define TARGET_REQUEST 7
#define SOURCE_DENY  8
#define V_SHIFT 9
#define FLOODING 10 // not used right now
#define DATA_TERMINATION 11
#define BACKPRESSURE 12
#define BACKFLOODING 13// not used right now 
#define EXPENSION 14
#define V_SHIFT_DATA 15
#define EXPENSION_DATA 16

// next hop status 
#define UNKNOWN 1
#define FRESHED 2
#define DEAD 3
//#define SUPPRESSED 4



// packet status
#define FORWARDED 3
#define CENTER_FORWARDED 4
#define FLOODED 5
//#define DROPPED 6
#define TERMINATED 7
//#define BACKFORWARDED 8
#define SUPPRESSED 9
#define VOID_SUPPRESSED 10


#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30
#define MAX_NEIGHBOR 10
#define WINDOW_SIZE  19 

//used by hash table to limited the maximum length

//#define ROUTING_PORT 255


typedef struct Position{
  double x;
  double y;
  double z;
} position;

typedef struct RoutingVector{
    position start;
    position end;
} routing_vector;



struct uw_extra_info {

  // ns_addr_t osender_id;            // The original sender of this message
  // unsigned int seq;           //  sequence number

  double ox;  // the start point of the forward path
  double oy;
  double oz;

  //ns_addr_t sender_id;            // The forwarder of this message

  double fx;  // the forward 's position
  double fy;
  double fz;
 
// the end point of the forward path
  double tx; 
  double ty;
  double tz;

// this is the information about relative position of the receiver to the forwarder, not include in the header of real packet
  double dx;
  double dy;
  double dz; 

};
#endif
