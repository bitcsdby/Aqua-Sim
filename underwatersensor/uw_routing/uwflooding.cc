

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>


//#include "underwatersensor/uw_common/uwvb_header.h"
#include "agent.h"
#include "tclcl.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "node.h"
#include "uwflooding.h"
#include "underwatersensor/uw_common/uw_hash_table.h"
//#include "arp.h"
//#include "mac.h"
//#include "ll.h"
//#include "dsr/path.h"
#include "god.h"



static class UWFloodingClass : public TclClass {
public:
  UWFloodingClass() : TclClass("Agent/UWFlooding") {}
  TclObject* create(int argc, const char*const* argv) {
    return(new UWFloodingAgent());
  }
} class_uwfloodingclass;


UWFloodingAgent::UWFloodingAgent() : Agent(PT_UWVB)
{
  // Initialize variables.
  //  printf("VB initialized\n");
  pk_count = 0;
  target_ = 0;
   node = NULL;
  tracetarget = NULL;
  
}


void UWFloodingAgent::recv(Packet* packet, Handler*)
{
 

  // printf ("uwflooding: I have get a packet \n ");
  hdr_uwvb* vbh = HDR_UWVB(packet);
  unsigned char msg_type =vbh->mess_type;
  unsigned int dtype =0;// vbh->data_type;
 
  // Packet Hash Table is used to keep info about experienced pkts.
 
    vbf_neighborhood *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);

     // Received this packet before ?

    if (hashPtr != NULL) {Packet::free(packet);}
      else {
 
     PktTable.put_in_hash(vbh);
 
     // Take action for a new pkt.
      
     ConsiderNew(packet);     
      }
}

void UWFloodingAgent::ConsiderNew(Packet *pkt)
{
  hdr_uwvb* vbh = HDR_UWVB(pkt);
  unsigned char msg_type =vbh->mess_type;
  unsigned int dtype = 0;//vbh->data_type;
  double l,h;
  
  //Pkt_Hash_Entry *hashPtr;
  ns_addr_t * hashPtr;
  //  Agent_List *agentPtr;
  // PrvCurPtr  RetVal;
   ns_addr_t   from_nodeID, forward_nodeID, target_nodeID;

  Packet *gen_pkt;
  hdr_uwvb *gen_vbh;
  //   printf("uwflooding(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
  //  printf("Vectorbasedforward:oops!\n");
  switch (msg_type) {
   
    case DATA :
      //    printf("uwflooding(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
      from_nodeID = vbh->sender_id;
      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
	// come from the same node, briadcast it
      MACprepare(pkt);
      MACsend(pkt,0); 
      return;      
}
         	  

	 if (THIS_NODE.addr_==vbh->target_id.addr_)
	   {
	     //	printf("uwflooding: %d is the target\n", here_.addr_);
	      DataForSink(pkt); // process it
	       }

	else{
	  // printf("uwflooding: %d is the not  target\n", here_.addr_);	 
      MACprepare(pkt);
      MACsend(pkt, 0);
	}
      return;

    default : 
      
      Packet::free(pkt);        
      break;
  }
}


void UWFloodingAgent::reset()
{
  PktTable.reset();
  /*
  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
  */
}


void UWFloodingAgent::Terminate() 
{
#ifdef DEBUG_OUTPUT
	printf("node %d: remaining energy %f, initial energy %f\n", THIS_NODE, 
	       node->energy_model()->energy(), 
	       node->energy_model()->initialenergy() );
#endif
}


void UWFloodingAgent::StopSource()
{
  /*
  Agent_List *cur;

  for (int i=0; i<MAX_DATA_TYPE; i++) {
    for (cur=routing_table[i].source; cur!=NULL; cur=AGENT_NEXT(cur) ) {
      SEND_MESSAGE(i, AGT_ADDR(cur), DATA_STOP);
    }
  }
  */
}


Packet * UWFloodingAgent:: create_packet()
{
  Packet *pkt = allocpkt();

  if (pkt==NULL) return NULL;

  hdr_cmn*  cmh = HDR_CMN(pkt);
  cmh->size() = 36;

  hdr_uwvb* vbh = HDR_UWVB(pkt);
  vbh->ts_ = NOW;
   

  return pkt;
}


Packet *UWFloodingAgent::prepare_message(unsigned int dtype, ns_addr_t to_addr,  int msg_type)

{
  Packet *pkt;
  hdr_uwvb *vbh;
  //hdr_ip *iph;

    pkt = create_packet();
    vbh = HDR_UWVB(pkt);
    // iph = HDR_IP(pkt);
    
    vbh->mess_type = msg_type;
    vbh->pk_num = pk_count;
    pk_count++;
    vbh->sender_id = here_;
    // vbh->data_type = dtype;
    vbh->forward_agent_id = here_;

    vbh->ts_ = NOW;
    return pkt;
}

void UWFloodingAgent::MACprepare(Packet *pkt)
{

  hdr_uwvb* vbh = HDR_UWVB(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);
  // hdr_ip*  iph = HDR_IP(pkt); // I am not sure if we need it


  vbh->forward_agent_id = here_; 
 
  cmh->xmit_failure_ = 0;
  // printf("vectorbased: the mac_Broadcast is:%d\n",MAC_BROADCAST);
  cmh->next_hop() = MAC_BROADCAST; 
  cmh->addr_type() = NS_AF_ILINK;  
 
  cmh->direction() = hdr_cmn::DOWN;

}


void UWFloodingAgent::MACsend(Packet *pkt, Time delay)
{
  hdr_cmn*  cmh = HDR_CMN(pkt);
  hdr_uwvb* vbh = HDR_UWVB(pkt);
 
 if (vbh->mess_type == DATA)
    cmh->size() = (God::instance()->data_pkt_size) ;
  else
    cmh->size() = 36;
   
  Scheduler::instance().schedule(ll, pkt, delay);
}


void UWFloodingAgent::DataForSink(Packet *pkt)
{

  //  printf("DataforSink: the packet is send to demux\n");
      send_to_dmux(pkt, 0);

}



void UWFloodingAgent::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer(), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}



int UWFloodingAgent::command(int argc, const char*const* argv)
{  
  Tcl& tcl =  Tcl::instance();

  if (argc == 2) {

    if (strcasecmp(argv[1], "reset-state")==0) {
      
      reset();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "reset")==0) {
      
      return Agent::command(argc, argv);
    }

    if (strcasecmp(argv[1], "start")==0) {
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "stop")==0) {
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "terminate")==0) {
      Terminate();
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "stop-source")==0) {
      StopSource();
      return TCL_OK;
    }

  } else if (argc == 3) {

    if (strcasecmp(argv[1], "on-node")==0) {
      // printf ("inside on node\n");
      node = (MobileNode *)tcl.lookup(argv[2]);
      return TCL_OK;
    }
    /*
      if (strcasecmp(argv[1], "set-port")==0) {
      printf ("inside on node\n");
      port_number=atoi(argv[2]);
      return TCL_OK;
    }
    */
    if (strcasecmp(argv[1], "add-ll") == 0) {

      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
    fprintf(stderr, "UWFlooding Node: %d lookup of %s failed\n", THIS_NODE.addr_, argv[2]);
	return TCL_ERROR;
      }
      ll = (NsObject *) obj;

     return TCL_OK;
    }

    if (strcasecmp (argv[1], "tracetarget") == 0) {
      TclObject *obj;
      if ((obj = TclObject::lookup (argv[2])) == 0) {
	  fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		   argv[2]);
	  return TCL_ERROR;
      }

      tracetarget = (Trace *) obj;
      return TCL_OK;
    }

    if (strcasecmp(argv[1], "port-dmux") == 0) {
      //   printf("uwflooding:port demux is called \n");
      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
	fprintf(stderr, "VB node Node: %d lookup of %s failed\n", THIS_NODE.addr_, argv[2]);
	return TCL_ERROR;
      }
      port_dmux = (NsObject *) obj;
      return TCL_OK;
    }

  } 

  return Agent::command(argc, argv);
}


// Some methods for Flooding Entry

