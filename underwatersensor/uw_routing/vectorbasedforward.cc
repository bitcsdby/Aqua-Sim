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
#include "vectorbasedforward.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"
#include  "underwatersensor/uw_mac/underwaterpropagation.h"


int hdr_uwvb::offset_;


static class UWVBHeaderClass: public PacketHeaderClass{
 public:
  UWVBHeaderClass():PacketHeaderClass("PacketHeader/UWVB",sizeof(hdr_uwvb))
{
 bind_offset(&hdr_uwvb::offset_);
} 
} class_uwvbhdr;

//===========UWVBHeaderClass================


void UWPkt_Hash_Table::reset()
{
  vbf_neighborhood *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
     delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}



vbf_neighborhood* UWPkt_Hash_Table::GetHash(ns_addr_t sender_id, 
					unsigned int pk_num)
{
  unsigned int key[3];

  key[0] = sender_id.addr_;
  key[1] = sender_id.port_;
  key[2] = pk_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL )
     return NULL;

  return (vbf_neighborhood *)Tcl_GetHashValue(entryPtr);
}



void UWPkt_Hash_Table::put_in_hash(hdr_uwvb * vbh)
{
    Tcl_HashEntry *entryPtr;
    // Pkt_Hash_Entry    *hashPtr;
    vbf_neighborhood* hashPtr;
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
	hashPtr=(vbf_neighborhood*)Tcl_GetHashValue(entryPtr);
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
    hashPtr=new vbf_neighborhood[1];
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
    vbf_neighborhood* hashPtr;
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
	hashPtr=(vbf_neighborhood*)Tcl_GetHashValue(entryPtr);
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
    hashPtr=new vbf_neighborhood[1];
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


void UWDelayTimer:: handle(Event* e)
{
  a_->timeout((Packet*) e);
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
  //  printf("VB initialized\n");
  pk_count = 0;
  target_ = 0;
   node = NULL;
  tracetarget = NULL;
  width=0;
  counter=0;
  priority=1.5;
  bind("hop_by_hop_", &hop_by_hop);
  bind("width",& width);//arg in the alg
  
}


void VectorbasedforwardAgent::recv(Packet* packet, Handler*)
{
  hdr_uwvb* vbh = HDR_UWVB(packet);
  unsigned char msg_type =vbh->mess_type;
  unsigned int dtype = vbh->data_type;
  double t1=vbh->ts_;
  position * p1;
 
  p1=new position[1];
  p1[0].x=vbh->info.fx;
  p1[0].y=vbh->info.fy;
  p1[0].z=vbh->info.fz; 

  //get position

  /*
  if(NOW-t1>=50)
    {
      printf("vectorbased: ops! too old %f %f!!!!!!!\n",t1, NOW);
    Packet::free(packet);  
    return;
    }
  */

  // Packet Hash Table is used to keep info about experienced pkts.
 
  // printf ("vectorbasedforward:(id :%d) forward:(%d ,%d) (%f,%f,%f) the relative position is (%f ,%f,%f) forward position is is (%f,%f,%f) at time %f\n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,node->X(),node->Y(),node->Z(),vbh->info.dx,vbh->info.dy,vbh->info.dz, vbh->info.fx,vbh->info.fy,vbh->info.fz,NOW);
  /*
  if (msg_type==SOURCE_DISCOVERY)
    {

     vbf_neighborhood *hashPtr= SourceTable.GetHash(vbh->sender_id, vbh->pk_num);
     if (hashPtr != NULL) {
       if(hashPtr->addr_<=vbh->forward_agent_id.addr_){

       // SourceTable.put_in_hash(vbh);
           //someone has claimed to be the source
        Packet::free(packet);  
	 }
        else  {
            SourceTable.put_in_hash(vbh,vbh->forward_agent_id);
                  Packet::free(packet); 
               // new node has smaller id
	}
     }
     else
       {
     // Never receive it before ? Put in hash table.
	 // printf("vectrobasedforward: %d get new discovery \n",here_.addr_);
     SourceTable.put_in_hash(vbh,vbh->forward_agent_id);
     ConsiderNew(packet);     
       }
     return;
    }



  
  if (msg_type==TARGET_DISCOVERY)
    {
     ns_addr_t *hashPtr= SinkTable.GetHash(vbh->sender_id, vbh->pk_num);
     if (hashPtr != NULL) {
       // SinkTable.put_in_hash(vbh);
           //someone has claimed to be the source
        Packet::free(packet);        
     }
     else
       {
     // Never receive it before ? Put in hash table.
	 // printf("vectrobasedforward: this is new packet\n");
     SinkTable.put_in_hash(vbh);
     ConsiderNew(packet);     
       }
     return;
    }
  */

    vbf_neighborhood *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);

     // Received this packet before ?

    if (hashPtr != NULL) {       
    PktTable.put_in_hash(vbh,p1);
     Packet::free(packet);
     // printf("vectrobasedforward: this is duplicate packet\n"); 
                      }
      else {

     // Never receive it before ? Put in hash table.
	//printf("vectrobasedforward: this is new packet\n");
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
  
  //Pkt_Hash_Entry *hashPtr;
   vbf_neighborhood * hashPtr;
  //  Agent_List *agentPtr;
  // PrvCurPtr  RetVal;
   ns_addr_t   from_nodeID, forward_nodeID, target_nodeID;

  Packet *gen_pkt;
  hdr_uwvb *gen_vbh;
  position * p1;
  p1=new position[1];
  p1[0].x=vbh->info.fx;
  p1[0].y=vbh->info.fy;
  p1[0].z=vbh->info.fz;    

  //if(((vbh->ts_>35.0)&&(vbh->ts_<=36))||((vbh->ts_>10.0)&&(vbh->ts_<11)))
  // printf ("vectorbasedforward:(id :%d) forward:(%d ,%d) sender is(%d,%d,%d), (%f,%f,%f) the relative position is (%f ,%f,%f) forward position is is (%f,%f,%f) at time %f\n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num,node->X(),node->Y(),node->Z(),vbh->info.dx,vbh->info.dy,vbh->info.dz, vbh->info.fx,vbh->info.fy,vbh->info.fz,NOW);
 
  //  printf ("vectorbasedforward:(id :%d) forward:(%d ,%d) sender is(%d,%d,%d),my position is  (%f,%f,%f)  at time %f\n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num,node->X(),node->Y(),node->Z(),NOW);


  //  printf("Vectorbasedforward:oops!\n");
  switch (msg_type) {
    case INTEREST : 
      // printf("Vectorbasedforward:it is interest packet!\n");
      hashPtr = PktTable.GetHash(vbh->sender_id, vbh->pk_num);

      // Check if it comes from sink agent of this node
      // If so we have to keep it in sink list 

      from_nodeID = vbh->sender_id;
      forward_nodeID = vbh->forward_agent_id;
      //  printf("Vectorbasedforward:it the from_nodeid is %d %d  and theb this node id is %d ,%d!\n", from_nodeID.addr_,from_nodeID.port_,THIS_NODE.addr_,THIS_NODE.port_ );

      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
   
      MACprepare(pkt);
      MACsend(pkt,Random::uniform()*JITTER); 
      //  printf("vectorbasedforward: after MACprepare(pkt)\n");
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
          h=projection(pkt);
        if (IsCloseEnough(pkt)){
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
// from other nodes hitted by the packet, it is supposed
// to be the one hop away from the sink 

// printf("Vectorbasedforward(%d,%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
    if (THIS_NODE.addr_==vbh->target_id.addr_) {
      //printf("Vectorbasedforward(%d,??%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
      // ns_addr_t *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
     // Received this packet before ?
      // if (hashPtr == NULL) { 

       calculatePosition(pkt);
       DataForSink(pkt);
	 printf("Vectorbasedforward: %d is the target\n", here_.addr_);
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

    /*
  case DATA_READY :

      // put source_agent in source list of routing table   
      agentPtr = new Agent_List;
      AGT_ADDR(agentPtr) = vbh->sender_id;
      agentPtr->next = routing_table[dtype].source;
      routing_table[dtype].source = agentPtr;
    
       // !! this part will be modified later
      God::instance()->AddSource(dtype, (vbh->sender_id).addr_);
	
      gen_pkt = prepare_message(dtype, vbh->sender_id, DATA_REQUEST);
      gen_vbh = HDR_UWVB(gen_pkt);
      //      gen_vbh->report_rate = ORIGINAL;
      send_to_dmux(gen_pkt, 0);
      Packet::free(pkt);
      return;
    */
 
    case DATA :
      // printf("Vectorbasedforward(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
      //  printf("Vectorbasedforward(%d):it is data packet(%d)\n",here_.addr_,vbh->pk_num);    
      from_nodeID = vbh->sender_id;
      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
	// come from the same node, broadcast it
      MACprepare(pkt);
      MACsend(pkt,0); 
      return;      
}
         	    calculatePosition(pkt);
		    //  printf("vectorbasedforward: after MACprepare(pkt)\n");
          l=advance(pkt);
         h=projection(pkt);
	

	 if (THIS_NODE.addr_==vbh->target_id.addr_)
               {
	 // printf("Vectorbasedforward: %d is the target\n", here_.addr_);
	      DataForSink(pkt); // process it
	       }

	else{
	  //  printf("Vectorbasedforward: %d is the not  target\n", here_.addr_);
	  if (IsCloseEnough(pkt)){
	      double delay=calculateDelay(pkt,p1);
              double d2=(UnderwaterChannel::Transmit_distance()-distance(pkt))/SPEED_OF_SOUND_IN_WATER;
	      //printf("Vectorbasedforward: I am  not  target delay is %f d2=%f distance=%f\n",(sqrt(delay)*DELAY+d2*2),d2,distance(pkt));
	      set_delaytimer(pkt,(sqrt(delay)*DELAY+d2*2));
	    
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
  /*
  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
  */
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
 

  /*
  if (vbh->mess_type == DATA)
   // cmh->size() = (God::instance()->data_pkt_size)+12 ;
   cmh->size() = 65+12 ;
  else
    cmh->size() =32;
  */



  
 //if(!ll) printf("ok, the LL is empty\n");
 //cmh->ptype_==PT_UWVB;
 //printf("vectorbased: the address type is :%d uid is %d\n", cmh->addr_type(),pkt->uid_);
 //printf("vectorbased: the packet type is :%d\n", cmh->ptype_);
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
  // delaytimer.packet=pkt; 
  //delaytimer.resched(c);
      Scheduler& s=Scheduler::instance();
       s.schedule(&delaytimer,(Event*)pkt,c);
      return;


}


//recall periodically
void VectorbasedforwardAgent::timeout(Packet * pkt){

 hdr_uwvb* vbh = HDR_UWVB(pkt);
 unsigned char msg_type =vbh->mess_type;
 vbf_neighborhood  *hashPtr;
 //Packet* p1;


 
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
	       position* tp;
               tp=new position[1];

	      	 tp[0].x=hashPtr->neighbor[i].x;   
	         tp[0].y=hashPtr->neighbor[i].y;   
        	 tp[0].z=hashPtr->neighbor[i].z; 
                double tdelay=calculateDelay(pkt,tp);
                // double tdelay=5;
		i++;
                double c=1;
	         while (i<num_neighbor){
		   c=c*2;
		   tp[0].x=hashPtr->neighbor[i].x;   
		   tp[0].y=hashPtr->neighbor[i].y;   
		   tp[0].z=hashPtr->neighbor[i].z;   
		 double t2delay=calculateDelay(pkt,tp);
		 if (t2delay<tdelay) tdelay=t2delay;
		 	 i++; 
		  }
 
	       delete tp;
               if(tdelay<=(priority/c)) { 
               MACprepare(pkt);
               MACsend(pkt,0);      
		 }
               else
	   Packet::free(pkt);//to much overlap, don;t send 
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
		   // printf("vectorbasedforward: !!%f\n",delay);
                    MACprepare(pkt);
                    MACsend(pkt,0);      
		              }
		 else  Packet::free(pkt);     
	 // printf("vectorbasedforward:  I%d am the only neighbor, I send it out at %f\n",here_.addr_,NOW);
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

 // double a=advance(pkt);
 double p=projection(pkt);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);
 // double delay=(TRANSMISSION_DISTANCE-d*cos_theta)/TRANSMISSION_DISTANCE;
   double delay=(p/width) +((UnderwaterChannel::Transmit_distance()-d*cos_theta)/UnderwaterChannel::Transmit_distance());
 // double delay=(p/width) +((TRANSMISSION_DISTANCE-d)/TRANSMISSION_DISTANCE)+(1-cos_theta);
  //printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f)\n",here_.addr_,p, cos_theta, d);
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

double VectorbasedforwardAgent::projection(Packet* pkt)
{
 hdr_uwvb *vbh  = HDR_UWVB(pkt);
 
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;
 
 double ox, oy, oz;

 if( !hop_by_hop )
 {
 //printf("vbf is used\n");
 	ox=vbh->info.ox;
 	oy=vbh->info.oy;
 	oz=vbh->info.oz;
 }
 else{
 //printf("hop_by_hop vbf is used\n");
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
 // printf("vectorbasedforward: the area is %f and length is %f\n",area,length);
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



bool VectorbasedforwardAgent::IsCloseEnough(Packet* pkt)
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
 // if (l<range)
 // {
     // printf("vectorbasedforward: IsClose?too close! it should be target!\n");
     // return true;
 // }
 // else {
   //double c=d/l;
   double c=1;
   // if ((d<=range)&&((z-oz)<0.01))  return true;
 if ((projection(pkt)<=(c*width)))  return true;
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


// Some methods for Flooding Entry

/*
void Vectorbasedforward_Entry::reset()
{
    clear_agentlist(source);
    clear_agentlist(sink);
    source = NULL;
    sink = NULL;
}

void Vectorbasedforward_Entry::clear_agentlist(Agent_List *list)
{
  Agent_List *cur=list;
  Agent_List *temp = NULL;

  while (cur != NULL) {
    temp = AGENT_NEXT(cur);
    delete cur;
    cur = temp;
  }
}

*/
