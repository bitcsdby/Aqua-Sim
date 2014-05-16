#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>

#include "underwatersensor/uw_mac/underwaterchannel.h"
#include "agent.h"
#include "tclcl.h"
#include "ip.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "node.h"
#include "vectorbasedforward_hop_by_hop.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"
#include  "underwatersensor/uw_mac/underwaterpropagation.h"

#define EXP  1e6
#define INF  1e9

int hdr_uwvb::offset_;


static class UWVBHeaderClass: public PacketHeaderClass{
 public:
  UWVBHeaderClass():PacketHeaderClass("PacketHeader/UWVB",sizeof(hdr_uwvb))
{
 bind_offset(&hdr_uwvb::offset_);
} 
} class_uwvbhdr;




void UWPkt_Hash_Table::reset()
{
  neighborhood *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (neighborhood *)Tcl_GetHashValue(entryPtr);
     delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}



neighborhood* UWPkt_Hash_Table::GetHash(ns_addr_t sender_id, 
					unsigned int pk_num)
{
  unsigned int key[3];

  key[0] = sender_id.addr_;
  key[1] = sender_id.port_;
  key[2] = pk_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL )
     return NULL;

  return (neighborhood *)Tcl_GetHashValue(entryPtr);
}



void UWPkt_Hash_Table::put_in_hash(hdr_uwvb * vbh)
{
    Tcl_HashEntry *entryPtr;
    // Pkt_Hash_Entry    *hashPtr;
    neighborhood* hashPtr;
    unsigned int key[3];
    int newPtr;

    key[0]=(vbh->sender_id).addr_;
    key[1]=(vbh->sender_id).port_;
    key[2]=vbh->pk_num;


     int  k=key[2]-window_size;
    if(k>0)
      {
      for (int i=0;i<k;i++)
	{
          key[2]=i;
       entryPtr=Tcl_FindHashEntry(&htable, (char *)key);
      if (entryPtr)
     {
	hashPtr=(neighborhood*)Tcl_GetHashValue(entryPtr);
	delete hashPtr;
	Tcl_DeleteHashEntry(entryPtr);
      }
	}
      }     
     
  key[2]=vbh->pk_num;
    entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
    if (!newPtr){
     hashPtr=GetHash(vbh->sender_id,vbh->pk_num);
    int m=hashPtr->number;
    if (m<MAX_NEIGHBOR){
        hashPtr->number++;
	hashPtr->neighbor[m].x=0;
        hashPtr->neighbor[m].y=0;
	hashPtr->neighbor[m].z=0;
    }
      return;
}
    hashPtr=new neighborhood[1];
    hashPtr[0].number=1;
    hashPtr[0].neighbor[0].x=0;
    hashPtr[0].neighbor[0].y=0;
    hashPtr[0].neighbor[0].z=0;
    Tcl_SetHashValue(entryPtr, hashPtr);
   
}







void UWPkt_Hash_Table::put_in_hash(hdr_uwvb * vbh, position* p)
{
    Tcl_HashEntry *entryPtr;
    // Pkt_Hash_Entry    *hashPtr;
    neighborhood* hashPtr;
    unsigned int key[3];
    int newPtr;

    key[0]=(vbh->sender_id).addr_;
    key[1]=(vbh->sender_id).port_;
    key[2]=vbh->pk_num;


     int  k=key[2]-window_size;
    if(k>0)
      {
	for (int i=0;i<k;i++){
	  key[2]=i;
       entryPtr=Tcl_FindHashEntry(&htable, (char *)key);
      if (entryPtr)
     {
	hashPtr=(neighborhood*)Tcl_GetHashValue(entryPtr);
	delete hashPtr;
	Tcl_DeleteHashEntry(entryPtr);
      }

      }       
      }
       key[2]=vbh->pk_num;
    entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
    if (!newPtr)
{

     hashPtr=GetHash(vbh->sender_id,vbh->pk_num);
    int m=hashPtr->number;
    // printf("hash_table: this is not old item, there are %d item inside\n",m); 
    if (m<MAX_NEIGHBOR){
        hashPtr->number++;
	hashPtr->neighbor[m].x=p->x;
        hashPtr->neighbor[m].y=p->y;
	hashPtr->neighbor[m].z=p->z;
    }
        return;
}
    hashPtr=new neighborhood[1];
    hashPtr[0].number=1;
    hashPtr[0].neighbor[0].x=p->x;
    hashPtr[0].neighbor[0].y=p->y;
    hashPtr[0].neighbor[0].z=p->z;
    Tcl_SetHashValue(entryPtr, hashPtr);
   
}


void UWData_Hash_Table::reset()
{
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}


Tcl_HashEntry  *UWData_Hash_Table::GetHash(int *attr)
{
  return Tcl_FindHashEntry(&htable, (char *)attr);
}


void UWData_Hash_Table::PutInHash(int *attr)
{
    int newPtr;

     Tcl_HashEntry* entryPtr=Tcl_CreateHashEntry(&htable, (char *)attr, &newPtr);

    if (!newPtr)
      return;
 
    int *hashPtr=new int[1];
    hashPtr[0]=1;
    Tcl_SetHashValue(entryPtr, hashPtr);

}

void UWDelayTimer:: expire(Event* e)
{
  a_->timeout(packet);
}


static class VectorbasedforwardClass : public TclClass {
public:
  VectorbasedforwardClass() : TclClass("Agent/Vectorbasedforward") {}
  TclObject* create(int argc, const char*const* argv) {
    return(new VectorbasedforwardAgent());
  }
} class_vectorbasedforward;



VectorbasedforwardAgent::VectorbasedforwardAgent() : Agent(PT_UWVB),
delaytimer(this)
{
  // Initialize variables.
  
  pk_count = 0;
  target_ = 0;
   node = NULL;
  tracetarget = NULL;
  // width=0;
  counter=0;
  priority=1.5;
 
 // forwarder_based=true;
 bind("hop_by_hop_", &forwarder_based);
 bind("width",& width);
 minProjection=width/4; 
//  printf("VB initialized and minProjection is %f\n",minProjection);
}


void VectorbasedforwardAgent::recv(Packet* packet, Handler*)
{
   
  hdr_cmn *hdr = HDR_CMN(packet);
  
  if(1==hdr->error()){
printf ("vectorbasedforward: I( %d ) received a  corrupted packet !!!!\n ",here_.addr_);
   Packet::free(packet);
  return;
  }
   

  if (node->failure_status()==1){
     
  printf ("vectorbasedforward: I( %d ) failed, I can't receive a packet !!!!\n ",here_.addr_);
  return;
  }
  
 
  
  hdr_uwvb* vbh = HDR_UWVB(packet);
  unsigned char msg_type =vbh->mess_type;
  unsigned int dtype = vbh->data_type;
  double t1=vbh->ts_;
  position * p1;
 
  p1=new position[1];
  p1[0].x=vbh->info.fx;
  p1[0].y=vbh->info.fy;
  p1[0].z=vbh->info.fz;    


    neighborhood *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
    //printf("vectrobasedforward node %d: recv  packet %d from the sender %d\n",here_.addr_,vbh->pk_num,vbh->sender_id.addr_); 
     // Received this packet before ?

    if (hashPtr) {       
      PktTable.put_in_hash(vbh,p1);
    
      printf("vectrobasedforward node %d: this is duplicate packet %d from the sender %d\n",here_.addr_,vbh->pk_num,vbh->sender_id.addr_); 
        Packet::free(packet);
                      }
      else {
 
     PktTable.put_in_hash(vbh,p1);
 
     // Take action for a new pkt.
      
     ConsiderNew(packet);     
      }
    delete p1;
}

void VectorbasedforwardAgent::ConsiderNew(Packet *pkt)
{
  hdr_uwvb* vbh = HDR_UWVB(pkt);
  unsigned char msg_type =vbh->mess_type;
  unsigned int dtype = vbh->data_type; 
 
  double l,h;
  
 
   neighborhood * hashPtr;
   ns_addr_t   from_nodeID, forward_nodeID, target_nodeID;

  Packet *gen_pkt;
  hdr_uwvb *gen_vbh;
  position * p1;
  p1=new position[1];
  p1[0].x=vbh->info.fx;
  p1[0].y=vbh->info.fy;
  p1[0].z=vbh->info.fz;    

  printf ("vectorbasedforward:(id :%d) forward:(%d ,%d) sender is(%d,%d,%d), (%f,%f,%f) the relative position is (%f ,%f,%f) forward position is is (%f,%f,%f) at time %f type is %d real one is %d\n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num,node->X(),node->Y(),node->Z(),vbh->info.dx,vbh->info.dy,vbh->info.dz, vbh->info.fx,vbh->info.fy,vbh->info.fz,NOW,vbh->mess_type,DATA);
 
  
  switch (msg_type) {
    case INTEREST : 
      // printf("Vectorbasedforward:it is interest packet!\n");
      hashPtr = PktTable.GetHash(vbh->sender_id, vbh->pk_num);

      // Check if it comes from sink agent of  this node
      // If so we have to keep it in sink list 

      from_nodeID = vbh->sender_id;
      forward_nodeID = vbh->forward_agent_id;
      //  printf("Vectorbasedforward:it the from_nodeid is %d %d  and theb this node id is %d ,%d!\n", from_nodeID.addr_,from_nodeID.port_,THIS_NODE.addr_,THIS_NODE.port_ );

      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
   
      MACprepare(pkt);
      MACsend(pkt,0); 
      //      MACsend(pkt,Random::uniform()*JITTER); 
      printf("vectorbasedforward: after MACprepare(pkt)\n");
      }
      else
       {
          calculatePosition(pkt);
	 //printf("vectorbasedforward: This packet is from different node\n");
	 if (IsTarget(pkt)) 
           { 
            // If this node is target?
    	      l=advance(pkt);
        
	   //  send_to_demux(pkt,0);
	      //  printf("vectorbasedforward:%d send out the source-discovery \n",here_.addr_);
	     vbh->mess_type=SOURCE_DISCOVERY;
	     set_delaytimer(pkt,l*JITTER);
                 // !!! need to re-think
	   }
	 else{ 
	   // calculatePosition(pkt);
	   // No the target forwared
          l=advance(pkt);
          h=projection(pkt, forwarder_based);
        if (IsCloseEnough(pkt,forwarder_based)){
	  // printf("vectorbasedforward:%d I am close enough for the interest\n",here_.addr_);
      MACprepare(pkt);
      MACsend(pkt,Random::uniform()*JITTER);//!!!! need to re-think
	}
	else { 
	  //  printf("vectorbasedforward:%d I am not close enough for the interest  \n",here_.addr_);
         Packet::free(pkt);}
	 }
       }
      // Packet::free(pkt); 
      return;


 

  case TARGET_DISCOVERY: 

    if (THIS_NODE.addr_==vbh->target_id.addr_) {
     
       calculatePosition(pkt);
       DataForSink(pkt);
       //	 printf("Vectorbasedforward: %d is the target\n", here_.addr_);
       // } //New data Process this data 
       // 
    } else  {Packet::free(pkt);}
   return;

  case SOURCE_DISCOVERY:
      Packet::free(pkt); 
// other nodes already claim to be the source of this interest
    //   SourceTable.put_in_hash(vbh);
    return;


 case DATA_READY :
   //  printf("Vectorbasedforward(%d,%d):it is data ready packet(%d)! it target id is %d \n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_);    
      from_nodeID = vbh->sender_id;
      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
	// come from the same node, broadcast it
      MACprepare(pkt);
      MACsend(pkt,Random::uniform()*JITTER); 
      return;      
          }
          calculatePosition(pkt);
      if (THIS_NODE.addr_==vbh->target_id.addr_)
               {
        printf("Vectorbasedforward: %d is the target\n", here_.addr_);
	      DataForSink(pkt); // process it
	       } 
	else{
	  // printf("Vectorbasedforward: %d is the not  target\n", here_.addr_); 
      MACprepare(pkt);
      MACsend(pkt, Random::uniform()*JITTER);
	}
      return;
 
    case DATA :
      //     printf("Vectorbasedforward(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);  

      // printf("Vectorbasedforward(%d) the traget address is %d\n",THIS_NODE.addr_,vbh->sender_id.addr_);   
  
      from_nodeID = vbh->sender_id;
      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
	// come from the same node, broadcast it
      MACprepare(pkt);
      MACsend(pkt,0); 
      return;      
}
           calculatePosition(pkt);
	  //  printf("vectorbasedforward: after MACprepare(pkt)\n");
          
	   // l=advance(pkt);
	   // h=projection(pkt, forwarder_based);
	
           if (THIS_NODE.addr_==vbh->target_id.addr_)
               {
		 // printf("Vectorbasedforward: %d is the target\n", here_.addr_);
	      DataForSink(pkt); // process it
	       }

	else{
   	    if (IsCloseEnough(pkt, forwarder_based)){
	      double delay=calculateDelay(pkt,p1);
              double d2=(UnderwaterChannel::Transmit_distance()-distance(pkt))/SPEED_OF_SOUND_IN_WATER;
	      set_delaytimer(pkt,(sqrt(delay)*DELAY+d2*2));
            printf("Vectorbasedforward: %d is forwarding a packet, speed is %f and transmission range is %f\n", here_.addr_, SPEED_OF_SOUND_IN_WATER,UnderwaterChannel::Transmit_distance() );	    
	  }
	  else { Packet::free(pkt);   }
	     
	}
      return;

    default : 
      
      Packet::free(pkt);        
      break;
  }
  delete p1;
}


void VectorbasedforwardAgent::reset()
{
  PktTable.reset();
  
}


void VectorbasedforwardAgent::Terminate() 
{
#ifdef DEBUG_OUTPUT
	printf("node %d: remaining energy %f, initial energy %f\n", THIS_NODE, 
	       node->energy_model()->energy(), 
	       node->energy_model()->initialenergy() );
#endif
}


void VectorbasedforwardAgent::StopSource()
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


Packet * VectorbasedforwardAgent:: create_packet()
{
  Packet *pkt = allocpkt();

  if (pkt==NULL) return NULL;

  hdr_cmn*  cmh = HDR_CMN(pkt);
  cmh->size() = 36;

  hdr_uwvb* vbh = HDR_UWVB(pkt);
  vbh->ts_ = NOW;
   
  //!! I add new part
 
  vbh->info.ox=node->CX();
  vbh->info.oy=node->CY(); 
  vbh->info.oz=node->CZ(); 
  vbh->info.fx=node->CX(); 
  vbh->info.fy=node->CY();
  vbh->info.fz=node->CZ();



  return pkt;
}


Packet *VectorbasedforwardAgent::prepare_message(unsigned int dtype, ns_addr_t to_addr,  int msg_type)

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
    vbh->data_type = dtype;
    vbh->forward_agent_id = here_;

    vbh->ts_ = NOW;
    //    vbh->num_next = 1;
    // I am not sure if we need this
    // vbh->next_nodes[0] = to_addr.addr_;


    // I am not sure if we need it?    
    /*
    iph->src_ = here_;
    iph->dst_ = to_addr;
    */
    return pkt;
}

void VectorbasedforwardAgent::MACprepare(Packet *pkt)
{

  hdr_uwvb* vbh = HDR_UWVB(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);
  //  hdr_ip*  iph = HDR_IP(pkt); // I am not sure if we need it


  vbh->forward_agent_id = here_; 
 
  cmh->xmit_failure_ = 0;
  // printf("vectorbased: the mac_Broadcast is:%d\n",MAC_BROADCAST);
  cmh->next_hop() = MAC_BROADCAST; 
  cmh->addr_type() = NS_AF_ILINK;  
  // cmh->txtime()=0;
  // printf("vectorbased: the address type is :%d and suppose to be %d and  nexthop %d MAC_BROAD %d\n", cmh->addr_type(),NS_AF_ILINK,cmh->next_hop(),MAC_BROADCAST);
  cmh->direction() = hdr_cmn::DOWN;
  // cmh->ptype_==PT_UWVB;
  // printf("vectorbased: the packet type is :%d\n", cmh->ptype_);
  //  printf("ok\n");

  //if (node) printf("ok, node is not empty\n");
  //printf("vectorbasedforward: inside MACprepare%d %d %d \n",node->X(),node->Y(),node->Z());
  

  // iph->src_ = here_;
  //iph->dst_.addr_ = MAC_BROADCAST;
  //iph->dst_.port_ = ROUTING_PORT;

  //  vbh->num_next = 1;
  // vbh->next_nodes[0] = MAC_BROADCAST;


  if(!node->sinkStatus()){       //!! I add new part
  vbh->info.fx=node->CX();
  vbh->info.fy=node->CY();
  vbh->info.fz=node->CZ();
  }
  else{
    vbh->info.fx=node->X();
  vbh->info.fy=node->Y();
  vbh->info.fz=node->Z();
}

  // printf("vectorbasedforward: last line MACprepare\n");
}


void VectorbasedforwardAgent::MACsend(Packet *pkt, Time delay)
{
  hdr_cmn*  cmh = HDR_CMN(pkt);
  hdr_uwvb* vbh = HDR_UWVB(pkt);

  // I don't understand why it works like this way
  /*
  if (vbh->mess_type == DATA)
    cmh->size() = (God::instance()->data_pkt_size) + 4*(vbh->num_next - 1);
  else
    cmh->size() = 36 + 4*(vbh->num_next -1);
  */
 
 if (vbh->mess_type == DATA)
   // cmh->size() = (God::instance()->data_pkt_size)+12 ;
  cmh->size() = 65+12 ;
  else
    cmh->size() =32;
  
 //if(!ll) printf("ok, the LL is empty\n");
 //cmh->ptype_==PT_UWVB;
 //printf("vectorbased: the address type is :%d uid is %d\n", cmh->addr_type(),pkt->uid_);
 // printf("vectorbased: the packet type is :%d\n", cmh->ptype_);
// ll->handle(pkt);
  Scheduler::instance().schedule(ll, pkt, delay);
}


void VectorbasedforwardAgent::DataForSink(Packet *pkt)
{


  /*
  hdr_uwvb     *vbh  = HDR_UWVB(pkt);
  unsigned int dtype = vbh->data_type;
  //Agent_List   *cur_agent;
  Packet       *cur_pkt;
  hdr_uwvb     *cur_vbh;
  hdr_ip       *cur_iph;

  for (cur_agent= (routing_table[dtype]).sink; cur_agent != NULL; 
	   cur_agent= AGENT_NEXT(cur_agent) ) {

      cur_pkt       = pkt->copy();
      //  cur_iph       = HDR_IP(cur_pkt);
      // cur_iph->dst_ = AGT_ADDR(cur_agent);

      cur_vbh       = HDR_UWVB(cur_pkt);
      cur_vbh->forward_agent_id = here_;
      cur_vbh->num_next = 1;
      // cur_vbh->next_nodes[0] = NODE_ADDR(cur_agent);
      
  */
  //  printf("DataforSink: the packet is send to demux\n");
      send_to_dmux(pkt, 0);

}



void VectorbasedforwardAgent::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer(), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}

void VectorbasedforwardAgent::set_delaytimer(Packet* pkt, double c){
 delaytimer.packet=pkt; 
 delaytimer.resched(c);
}

void VectorbasedforwardAgent::timeout(Packet * pkt){

 hdr_uwvb* vbh = HDR_UWVB(pkt);
 unsigned char msg_type =vbh->mess_type;
 neighborhood  *hashPtr;
 minProjection=width/4;
printf("VB node %d in timeout  minProjection is %f\n",here_.addr_,minProjection);
// right now, I just consider the case of DATA package, later, we should add on other packet types 
 switch (msg_type){
  
 case DATA:
       hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
	if (hashPtr != NULL) {
          int num_neighbor=hashPtr->number;          
	  // printf("vectorbasedforward: node %d have received %d when wake up at %f\n",here_.addr_,num_neighbor,NOW);
	  if (num_neighbor!=1){
      // Some guys transmit the data before me
	    if (num_neighbor==MAX_NEIGHBOR) {
             //I have too many neighbors, I quit
                  Packet::free(pkt);
		  return;  
	    }
	    else //I need to calculate my delay time again 
             {  
	       int i=0;
	       position tp,sp,mp;
               double tprojection=10000.0;

	       while (i<num_neighbor){
	      	 sp.x=hashPtr->neighbor[i].x;   
	         sp.y=hashPtr->neighbor[i].y;   
        	 sp.z=hashPtr->neighbor[i].z;
  
                 tp.x=vbh->info.tx;
                 tp.y=vbh->info.ty;
                 tp.z=vbh->info.tz;
                    
                 mp.x=node->CX_;
                 mp.y=node->CY_;
                 mp.z=node->CZ_;

                double t2projection=projection(&sp,&tp,&mp);
                 if (t2projection<tprojection) tprojection=t2projection;
		 	 i++; 
	       }
		
	      
               if(tprojection>minProjection){ 
     printf("vectorbasedforward:  I (%d) has neighbors minprojection=%f, but I still need to send at %f\n",here_.addr_,tprojection,NOW);
               MACprepare(pkt);
               MACsend(pkt,0);      
		 }
               else{
	   Packet::free(pkt);//to much overlap, don;t send 
 printf("vectorbasedforward:  I (%d) has neighbors, minprojection=%f I don't  need to send at %f\n",here_.addr_,tprojection,NOW);
	       }
	     }// end of calculate my new delay time 
	  }

 else{// I am the only neighbor
	       position* tp;
               tp=new position[1];
		 tp[0].x=vbh->info.fx;   
	         tp[0].y=vbh->info.fy;   
        	 tp[0].z=vbh->info.fz;   
		 double delay=calculateDelay(pkt,tp);

                 delete tp;
		 if (delay<=priority){
	printf("vectorbasedforward:  I (%d) am the only neighbor, I send it out at %f\n",here_.addr_,NOW);
		  
                    MACprepare(pkt);
                    MACsend(pkt,0);      
		              }
		 else 
{       
 printf("vectorbasedforward: I am not good enough even I am the only neighbor!!%f\n",here_.addr_);
Packet::free(pkt);     
}
	return;
 }
	}
	break; 
 default:
    Packet::free(pkt);  
   break;
 }
}



void VectorbasedforwardAgent::calculatePosition(Packet* pkt)
{
 
 hdr_uwvb     *vbh  = HDR_UWVB(pkt); 
 double fx=vbh->info.fx;
 double fy=vbh->info.fy;
 double fz=vbh->info.fz;

 double dx=vbh->info.dx;
 double dy=vbh->info.dy;
 double dz=vbh->info.dz;

 node->CX_=fx+dx;
 node->CY_=fy+dy;
 node->CZ_=fz+dz;
 // printf("vectorbased: my position is computed as (%f,%f,%f)\n",node->CX_, node->CY_,node->CZ_);
}

double VectorbasedforwardAgent::calculateDelay(Packet* pkt,position* p1)
{
 
 hdr_uwvb     *vbh  = HDR_UWVB(pkt); 
 
 position tp;
 position sp;


 tp.x=vbh->info.tx;
 tp.y=vbh->info.ty;
 tp.z=vbh->info.tz;

 sp.x=p1->x;
 sp.y=p1->y;
 sp.z=p1->z;
 


 double fx=p1->x;
 double fy=p1->y;
 double fz=p1->z;

 double dx=node->CX_-fx; 
 double dy=node->CY_-fy;
 double dz=node->CZ_-fz;

  
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz; 

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;  

 double dp=dx*dtx+dy*dty+dz*dtz;


 double p=projection(&sp,&tp,p1);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta = 0;
/*
 if( d*l < EXP )
 	cos_theta = INF;
 else
 	cos_theta=dp/(d*l);
*/
 cos_theta = dp/(d*l);

 double delay=(p/width) +((UnderwaterChannel::Transmit_distance()-d*cos_theta)/UnderwaterChannel::Transmit_distance());
   return delay;
}

double VectorbasedforwardAgent::distance(Packet* pkt)
{
 
 hdr_uwvb     *vbh  = HDR_UWVB(pkt); 
 double tx=vbh->info.fx;
 double ty=vbh->info.fy;
 double tz=vbh->info.fz;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=node->CX(); //change later
 double y=node->CY();// printf(" Vectorbasedforward: I am in advanced\n");
 double z=node->CZ();
 // printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}


double VectorbasedforwardAgent::advance(Packet* pkt)
{
 
 hdr_uwvb     *vbh  = HDR_UWVB(pkt); 
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=node->CX(); //change later
 double y=node->CY();// printf(" Vectorbasedforward: I am in advanced\n");
 double z=node->CZ();
 // printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}


double VectorbasedforwardAgent::projection(const position* sp, const position* tp,const position * p)
{

 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z;
 

 double ox=sp->x;
 double oy=sp->y;
 double oz=sp->z;

 double x=p->x;
 double y=p->y;
 double z=p->z;
 
 double wx=tx-ox;
 double wy=ty-oy;
 double wz=tz-oz;

 double vx=x-ox;
 double vy=y-oy;
 double vz=z-oz;

 double cross_product_x=vy*wz-vz*wy;
 double cross_product_y=vz*wx-vx*wz;
 double cross_product_z=vx*wy-vy*wx;
  
 double area=sqrt(cross_product_x*cross_product_x+ 
          cross_product_y*cross_product_y+cross_product_z*cross_product_z);
 double length=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
 // printf("vectorbasedforward: the area is %f and length is %f\n",area,length);
 return area/length;
}


double VectorbasedforwardAgent::projection(Packet* pkt, bool forwarder_based)
{

 hdr_uwvb     *vbh  = HDR_UWVB(pkt);
 
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;
 
 double ox,oy,oz;

 if(!forwarder_based){
  ox=vbh->info.ox;
  oy=vbh->info.oy;
  oz=vbh->info.oz;
 }
 else {
  ox=vbh->info.fx;
  oy=vbh->info.fy;
  oz=vbh->info.fz;
 }

 double x=node->CX();
 double y=node->CY();
 double z=node->CZ();
 
 double wx=tx-ox;
 double wy=ty-oy;
 double wz=tz-oz;

 double vx=x-ox;
 double vy=y-oy;
 double vz=z-oz;

 double cross_product_x=vy*wz-vz*wy;
 double cross_product_y=vz*wx-vx*wz;
 double cross_product_z=vx*wy-vy*wx;
  
 double area=sqrt(cross_product_x*cross_product_x+ 
          cross_product_y*cross_product_y+cross_product_z*cross_product_z);
 double length=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
 //printf("vectorbasedforward: my %d projection is %f\n",here_.addr_,area/length);
/* if( length < EXP )
	return INF;
 else */
 	return area/length;
}

bool VectorbasedforwardAgent::IsTarget(Packet* pkt)
{
  hdr_uwvb * vbh=HDR_UWVB(pkt);

  if (vbh->target_id.addr_==0){

  //  printf("vectorbased: advanced is %lf and my range is %f\n",advance(pkt),vbh->range);
    return (advance(pkt)<vbh->range);
}
  else return(THIS_NODE.addr_==vbh->target_id.addr_);


}



bool VectorbasedforwardAgent::IsCloseEnough(Packet* pkt, bool forwarder_based)
{
  hdr_uwvb     *vbh  = HDR_UWVB(pkt);
  double range=vbh->range;
  //double range=width;

  //  printf("underwatersensor: The width is %f\n",range);
 double ox=vbh->info.ox;
 double oy=vbh->info.oy;
 double oz=vbh->info.oz;

 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;

 double fx=vbh->info.fx;
 double fy=vbh->info.fy;
 double fz=vbh->info.fz;


 double x=node->CX(); //change later
 double y=node->CY();
 double z=node->CZ();

 double d=sqrt((tx-fx)*(tx-fx)+(ty-fy)*(ty-fy)+(tz-fz)*(tz-fz));
 //double l=sqrt((tx-ox)*(tx-ox)+(ty-oy)*(ty-oy)+ (tz-oz)*(tz-oz));
 double l=advance(pkt);

   double c=1;

 if ((projection(pkt,forwarder_based)<=(c*width)))  return true;
 return false;

}


int VectorbasedforwardAgent::command(int argc, const char*const* argv)
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

   if (strcasecmp(argv[1], "name")==0) {
     printf("vectorbased \n");
      return TCL_OK;
    }
    if (strcasecmp(argv[1], "stop-source")==0) {
      StopSource();
      return TCL_OK;
    }

  } else if (argc == 3) {

    if (strcasecmp(argv[1], "on-node")==0) {
      //   printf ("inside on node\n");
      node = (UnderwaterSensorNode *)tcl.lookup(argv[2]);
      return TCL_OK;
    }
    
    if (strcasecmp(argv[1], "add-ll") == 0) {

      TclObject *obj;

      if ( (obj = TclObject::lookup(argv[2])) == 0) {
    fprintf(stderr, "Vectorbasedforwarding Node: %d lookup of %s failed\n", THIS_NODE.addr_, argv[2]);
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
      // printf("vectorbasedforward:port demux is called \n");
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


