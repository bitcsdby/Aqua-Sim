#include "vectorbasedvoidavoidance.h"
#include "god.h"
#include  "underwatersensor/uw_mac/underwaterpropagation.h"
#include "underwatersensor/uw_mac/underwaterchannel.h"

int hdr_uwvbva::offset_;

static class UWVBVAHeaderClass: public PacketHeaderClass{
 public:
  UWVBVAHeaderClass():PacketHeaderClass("PacketHeader/UWVBVA",sizeof(hdr_uwvbva))
{
 bind_offset(&hdr_uwvbva::offset_);
} 
} class_uwvbvahdr;


void UWVBVAPkt_Hash_Table::reset()
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



neighborhood* UWVBVAPkt_Hash_Table::GetHash(ns_addr_t sender_id, 
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


void UWVBVAPkt_Hash_Table::delete_hash(hdr_uwvbva * vbh)
{
    Tcl_HashEntry *entryPtr;
 
    neighborhood* hashPtr;
    unsigned int key[3];
    // int newPtr;

    key[0]=(vbh->sender_id).addr_;
    key[1]=(vbh->sender_id).port_;
    key[2]=vbh->pk_num;

      
       entryPtr=Tcl_FindHashEntry(&htable, (char *)key);
      if (entryPtr)
      {
	hashPtr=(neighborhood*)Tcl_GetHashValue(entryPtr);
	delete hashPtr;
	Tcl_DeleteHashEntry(entryPtr);
      }
	return;    
}



void UWVBVAPkt_Hash_Table::delete_hash(ns_addr_t source, unsigned int pkt_num)
{
    Tcl_HashEntry *entryPtr;
 
    neighborhood* hashPtr;
    unsigned int key[3];
    //int newPtr;

    key[0]=source.addr_;
    key[1]=source.port_;
    key[2]=pkt_num;

      
       entryPtr=Tcl_FindHashEntry(&htable, (char *)key);
      if (entryPtr)
      {
	hashPtr=(neighborhood*)Tcl_GetHashValue(entryPtr);
	delete hashPtr;
	Tcl_DeleteHashEntry(entryPtr);
      }
	return;    
}

void UWVBVAPkt_Hash_Table::MarkNextHopStatus(ns_addr_t sender_id, unsigned int pk_num,unsigned int forwarder_id, unsigned int status)
{
    Tcl_HashEntry *entryPtr;
  
    neighborhood* hashPtr;
    unsigned int key[3];
    int newPtr;
    // unsigned int forwarder_id=forwarder_id;

    key[0]=sender_id.addr_;
    key[1]=sender_id.port_;
    key[2]=pk_num;


     entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
    if (!newPtr){
     hashPtr=GetHash(sender_id,pk_num);
    int m=hashPtr->number;

    for (int i=0;i<m;i++) {
        if ((hashPtr->neighbor[i].forwarder_id==forwarder_id)&&
	    (hashPtr->neighbor[i].status==FRESHED))
            hashPtr->neighbor[i].status=status;
    }
    }
    else printf("hashtable, the packet record doesn't exist\n");

    return;
}


void UWVBVAPkt_Hash_Table::put_in_hash(hdr_uwvbva * vbh)
{
    Tcl_HashEntry *entryPtr;
    neighborhood* hashPtr;
    unsigned int key[3];
    int newPtr;
    unsigned int forwarder_id=(vbh->forward_agent_id).addr_;

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

    if (!newPtr){// the record already exist
    hashPtr=GetHash(vbh->sender_id,vbh->pk_num);
    int m=hashPtr->number;

    int k=0;
    while((hashPtr->neighbor[k].forwarder_id!=forwarder_id)&&(k<m)) k++;

    if(k==m) hashPtr->number++;    
    if (k<MAX_NEIGHBOR){
	hashPtr->neighbor[k].vec.start.x=0;
        hashPtr->neighbor[k].vec.start.y=0;
	hashPtr->neighbor[k].vec.start.z=0;

   	hashPtr->neighbor[k].vec.end.x=0;
        hashPtr->neighbor[k].vec.end.y=0;
	hashPtr->neighbor[k].vec.end.z=0;

	hashPtr->neighbor[k].node.x=0;
        hashPtr->neighbor[k].node.y=0;
	hashPtr->neighbor[k].node.z=0;

        hashPtr->neighbor[k].forwarder_id=forwarder_id;
        hashPtr->neighbor[k].status=FRESHED;
    }
    else {
	for(int i=1;i<MAX_NEIGHBOR;i++)
	{
	    hashPtr->neighbor[i-1].vec=hashPtr->neighbor[i].vec; 
            hashPtr->neighbor[i-1].node=hashPtr->neighbor[i].node;

            hashPtr->neighbor[i-1].forwarder_id=hashPtr->neighbor[i].forwarder_id;
            hashPtr->neighbor[i-1].status= hashPtr->neighbor[i].status;
	}
      
       	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.x=0;
        hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.y=0;
	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start.z=0;

       	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.x=0;
        hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.y=0;
	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end.z=0;

        hashPtr->neighbor[MAX_NEIGHBOR-1].node.x=0;
        hashPtr->neighbor[MAX_NEIGHBOR-1].node.y=0;
	hashPtr->neighbor[MAX_NEIGHBOR-1].node.z=0;


       hashPtr->neighbor[MAX_NEIGHBOR-1].forwarder_id=forwarder_id;
       hashPtr->neighbor[MAX_NEIGHBOR-1].status=FRESHED;
    }
    return;
    }

    // the record does not exist

    hashPtr=new neighborhood;
    hashPtr->number=1;


    hashPtr->neighbor[0].vec.start.x=0;
    hashPtr->neighbor[0].vec.start.y=0;
    hashPtr->neighbor[0].vec.start.z=0;

    hashPtr->neighbor[0].vec.end.x=0;
    hashPtr->neighbor[0].vec.end.y=0;
    hashPtr->neighbor[0].vec.end.z=0;

    hashPtr->neighbor[0].node.x=0;
    hashPtr->neighbor[0].node.y=0;
    hashPtr->neighbor[0].node.z=0;

    hashPtr->neighbor[0].forwarder_id=forwarder_id;
    hashPtr->neighbor[0].status=FRESHED;

    Tcl_SetHashValue(entryPtr, hashPtr);
}


void UWVBVAPkt_Hash_Table::put_in_hash(hdr_uwvbva * vbh, const position* sp, const position* tp, const position* fp, unsigned  int status)
{
    Tcl_HashEntry *entryPtr;
    neighborhood* hashPtr;
    unsigned int key[3];
    int newPtr;
    unsigned int forwarder_id=(vbh->forward_agent_id).addr_;

    key[0]=vbh->sender_id.addr_;
    key[1]=vbh->sender_id.port_;
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

    if (!newPtr)// record already exists
{
     hashPtr=GetHash(vbh->sender_id,vbh->pk_num);
    int m=hashPtr->number;

    int k=0;
    while((hashPtr->neighbor[k].forwarder_id!=forwarder_id)&&(k<m)) k++;
    if(k==m) hashPtr->number++;
    

  // printf("hash_table: this is not old item, there are %d item inside\n",m); 
    if (k<MAX_NEIGHBOR){
	hashPtr->neighbor[k].vec.start.x=sp->x;
        hashPtr->neighbor[k].vec.start.y=sp->y;
	hashPtr->neighbor[k].vec.start.z=sp->z;

	hashPtr->neighbor[k].vec.end.x=tp->x;
        hashPtr->neighbor[k].vec.end.y=tp->y;
	hashPtr->neighbor[k].vec.end.z=tp->z;

        hashPtr->neighbor[k].node.x=fp->x;
        hashPtr->neighbor[k].node.y=fp->y;
	hashPtr->neighbor[k].node.z=fp->z;
      
      
       hashPtr->neighbor[k].forwarder_id=forwarder_id;
       hashPtr->neighbor[k].status=status;
    }
    else {
	for(int i=1;i<MAX_NEIGHBOR;i++)
	{
	    hashPtr->neighbor[i-1].vec=hashPtr->neighbor[i].vec; 
            hashPtr->neighbor[i-1].node=hashPtr->neighbor[i].node;

            hashPtr->neighbor[i-1].forwarder_id=hashPtr->neighbor[i].forwarder_id;
            hashPtr->neighbor[i-1].status= hashPtr->neighbor[i].status;
	}

	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.start=(*sp);
	hashPtr->neighbor[MAX_NEIGHBOR-1].vec.end=(*tp);
 	hashPtr->neighbor[MAX_NEIGHBOR-1].node=(*fp);

       hashPtr->neighbor[MAX_NEIGHBOR-1].forwarder_id=forwarder_id;
       hashPtr->neighbor[MAX_NEIGHBOR-1].status=FRESHED;
    }

        return;
} 
    // record does not exist
    hashPtr=new neighborhood;
    hashPtr->number=1;

    hashPtr->neighbor[0].vec.start=(*sp);
    hashPtr->neighbor[0].vec.end=(*tp);
    hashPtr->neighbor[0].node=(*fp);
    

    hashPtr->neighbor[0].forwarder_id=forwarder_id;
    hashPtr->neighbor[0].status=status;


    Tcl_SetHashValue(entryPtr, hashPtr);
    return;   
}


void UWVBVAData_Hash_Table::reset()
{
  unsigned int *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (unsigned int *)Tcl_GetHashValue(entryPtr);
     delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}


void UWVBVAData_Hash_Table::put_in_hash(ns_addr_t source, unsigned int pkt_num, unsigned int status)
{
    Tcl_HashEntry *entryPtr;
    int* hashPtr;
    unsigned int key[3];
    int statusPtr;
    unsigned int* valuePtr=0;
   
    valuePtr=new unsigned int;
    (*valuePtr)=status;   

    key[0]=source.addr_;
    key[1]=source.port_;
    key[2]=pkt_num;
    
    entryPtr = Tcl_CreateHashEntry(&htable, (char*)key, &statusPtr);
    Tcl_SetHashValue(entryPtr,valuePtr);        
    return;
}
  

void UWVBVAData_Hash_Table::delete_hash(ns_addr_t source, unsigned int pkt_num)
{
    Tcl_HashEntry *entryPtr;
 
    unsigned int* hashPtr;
    unsigned int key[3];

    key[0]=source.addr_;
    key[1]=source.port_;
    key[2]=pkt_num;

      
       entryPtr=Tcl_FindHashEntry(&htable, (char *)key);
      if (entryPtr)
      {
	hashPtr=(unsigned int*)Tcl_GetHashValue(entryPtr);
	delete hashPtr;
	Tcl_DeleteHashEntry(entryPtr);
      }
	return;    
}




unsigned int*  UWVBVAData_Hash_Table::GetHash(ns_addr_t source, unsigned int pkt_num)
{
  unsigned int key[3];

  key[0] =source.addr_;
  key[1] =source.port_;
  key[2] = pkt_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (!entryPtr)
     return NULL;
 
  return (unsigned int*)Tcl_GetHashValue(entryPtr);
}

void UWDelayHandler:: handle(Event* e)
{
  a_->process_forward_timeout((Packet*) e);
}


void UWVoidAvoidHandler:: handle(Event* e)
{
  a_->process_void_avoidance_timeout((Packet*)e);
}

/*
void UWFloodingHandler::handle(Event* e)
{
  a_->process_flooding_timeout((Packet*)e);
}
*/


void UWFloodingBackwardHandler::handle(Event* e)
{
  a_->process_backpressure_timeout((Packet*)e);
}

void UWFloodingForwardHandler::handle(Event* e)
{
  a_->process_selfcentered_timeout((Packet*)e);
}


/*
void UWFBackwardFloodingHandler::handle(Event* e)
{
  a_->process_backwardflooding_packet_timeout((Packet*)e);
}
*/



static class VectorbasedVoidAvoidanceClass : public TclClass {
public:
  VectorbasedVoidAvoidanceClass() : TclClass("Agent/VectorbasedVoidAvoidance") {}
  TclObject* create(int argc, const char*const* argv) {
    return(new VectorbasedVoidAvoidanceAgent());
  }
} class_vectorbasedvoidavoidance;




VectorbasedVoidAvoidanceAgent::VectorbasedVoidAvoidanceAgent() : Agent(PT_UWVBVA),forward_delay_handler(this),void_avoidance_buffer(15,0),void_avoidance_handler(this),backpressure_handler(this),mini_distance(20.0),mini_threshold(1.5), /*receiving_buffer(10),*/self_centered_forward_handler(this)
{

  position_update_time=-1.0;
  pk_count = 0;
  target_ = 0;
  node = NULL;
  tracetarget = NULL;
  width=0;
  counter=0;
  bind("width",& width);
  bind("control_packet_size",& control_packet_size);
  
}


void VectorbasedVoidAvoidanceAgent::recv(Packet* packet, Handler*)
{
  if (node->failure_status()==1){    
  printf ("vectorbasedvoidavoidance%d: I fails!!!!\n ",here_.addr_);
  Packet::free(packet);
  return;
  }
  

  printf (">>>vectorbasedvoidavoidance%d: recv  at %f\n ",here_.addr_,NOW);


  hdr_uwvbva* vbh = HDR_UWVBVA(packet);
  unsigned  int  msg_type =vbh->mess_type;
  double t1=vbh->ts_;
  position * p1;
  ns_addr_t source=vbh->sender_id;
  ns_addr_t forwarder=vbh->forward_agent_id;  
  unsigned int pkt_num=vbh->pk_num; 

  unsigned int* statusPtr=PacketStatusTable.GetHash(source,pkt_num);
 if(statusPtr&&((*statusPtr)==TERMINATED)){
printf("vectrobasedforward node %d: this packet has been terminated\n",here_.addr_);
 Packet::free(packet);
 return;
 }

 /*
      if(msg_type==BACKFLOODING){
// printf("vectrobasedforward node %d: recv backflooding  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);

     unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);
       
     if (((packet_status*)==FLOODED)||((packet_status*)==CENTERED)||((packet_status*)==SENT)) {       
 // printf("vectrobasedforward node %d:the data packet is flooded, termonated or center_sent\n",here_.addr_); 
         Packet::free(packet);
	return;
    }    
     processBackFloodedPacket(packet);
      return;
      }  
 */

      if(msg_type==DATA_TERMINATION){
// printf("vectrobasedforward node %d: recv DATA_TERMINATION  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);
      PacketStatusTable.put_in_hash(source,pkt_num,TERMINATED);
      Packet::free(packet);
      return;
      }  

      if((msg_type==V_SHIFT)||(msg_type==V_SHIFT_DATA)){
	//printf("vectrobasedforward node %d: recv V_SHIFT or V_D from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);  
        if(IsNewlyTouchedNode(source, pkt_num)){
  printf("vectrobasedforward node %d is a newly touched node\n",here_.addr_);
  
//  unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);

        PacketStatusTable.put_in_hash(source,pkt_num,SUPPRESSED);
	processCenteredPacket(packet);
         return;
 } else Packet::free(packet);
      }

      if((msg_type==EXPENSION)||(msg_type==EXPENSION_DATA)){
	//printf("vectrobasedforward node %d: recv EXPENSION  from the sender %d\n",here_.addr_,vbh->forward_agent_id.addr_);  
      
        unsigned int* packet_status=PacketStatusTable.GetHash(source,pkt_num);

        if((!packet_status)||((*packet_status)==SUPPRESSED)){
        PacketStatusTable.put_in_hash(source,pkt_num,VOID_SUPPRESSED);
	processCenteredPacket(packet);
         return;
 } else Packet::free(packet);
      }

  if(msg_type==BACKPRESSURE){
    //printf("vectrobasedforward node %d: receives a backpressure packet(%d) from %d\n",here_.addr_,pkt_num,vbh->forward_agent_id.addr_); 
        processBackpressurePacket(packet);     
    return;
}

  if(msg_type==DATA){
    printf(">>>vectrobasedforward node %d: receives a DATA packet from %d\n",here_.addr_,vbh->forward_agent_id.addr_);      
     unsigned int* statusPtr= PacketStatusTable.GetHash(source,pkt_num);
     neighborhood *  packetPtr=PktTable.GetHash(source,pkt_num);
   
 if ((statusPtr)||(packetPtr)) {  
   
        recordPacket(vbh);
        Packet::free(packet);
	return;
    }

 //   if(t1>position_update_time) {
            calculatePosition(packet);
               position_update_time=t1;
	       // }
 
     ConsiderNew(packet);     
     return ;
  }
}



// this function assme that the end points of the vectors are the same
bool 
VectorbasedVoidAvoidanceAgent::IsSamePosition(const position* sp1, const position* sp2)
{
    double err=0.1;
    if(fabs(sp1->x-sp2->x)>err) return false;
    if(fabs(sp1->y-sp2->y)>err) return false;
    if(fabs(sp1->z-sp2->z)>err) return false;
    return true;
} 

void 
VectorbasedVoidAvoidanceAgent::recordPacket(hdr_uwvbva* vbh, unsigned int status)
{
      position sp,ep,fp;
  

      fp.x=vbh->info.fx;
      fp.y=vbh->info.fy;
      fp.z=vbh->info.fz;    

      sp.x=vbh->info.ox;
      sp.y=vbh->info.oy;
      sp.z=vbh->info.oz;
   
      ep.x=vbh->info.tx;
      ep.y=vbh->info.ty;
      ep.z=vbh->info.tz;
      
      PktTable.put_in_hash(vbh,&sp,&ep,&fp, status);

    return;
}


void VectorbasedVoidAvoidanceAgent::processCenteredPacket(Packet* pkt)
{
  //  printf("vectorbased node %d: process centered packet\n",here_.addr_);



 if(!pkt){
     printf("vectorbased node %d: the data packet is empty\n",here_.addr_);
    return;
  }




      hdr_uwvbva* vbh = HDR_UWVBVA(pkt);

      ns_addr_t source=vbh->sender_id;
      unsigned pkt_num=vbh->pk_num;
    
      Packet* tpacket=0;
    
    
      position mp,tp,sp,fp;
 
      mp.x=node->CX();
      mp.y=node->CY();
      mp.z=node->CZ();

      tp.x=vbh->info.tx;
      tp.y=vbh->info.ty;
      tp.z=vbh->info.tz;
    
      fp.x=vbh->info.fx;
      fp.y=vbh->info.fy;
      fp.z=vbh->info.fz;
 
      sp.x=vbh->info.ox;
      sp.y=vbh->info.oy;
      sp.z=vbh->info.oz;

      //printf("vectorbased: node(%d) sp (%f,%f,%f) tp (%f,%f,%f) and fp(%f,%f,%f)\n",here_.addr_,sp.x,sp.y,sp.z,tp.x,tp.y,tp.z,fp.x,fp.y,fp.z);


 double delay_factor=calculateSelfCenteredDelay(&sp,&tp,&mp,&fp);
 if (delay_factor>1.2){
   Packet::free(pkt);
   return;
 }


      if((vbh->mess_type==V_SHIFT)||(vbh->mess_type==EXPENSION)){

      Packet::free(pkt);

      Packet* p=void_avoidance_buffer.LookupCopy(source,pkt_num);

 if(!p){
     printf("vectorbased node %d: can not find the corresponding packet in the buffer\n",here_.addr_);
    return;
  }
   tpacket=p->copy();

      } else{

	// in case this node is target
       if (THIS_NODE.addr_==vbh->target_id.addr_)
               {
	      sendDataTermination(pkt); 
	      PacketStatusTable.put_in_hash(source,pkt_num,TERMINATED); 
	      DataForSink(pkt); // process it
	      return;
	       }

      tpacket=pkt->copy();
      Packet::free(pkt);
      }   

     if(!tpacket) {
 printf("vectorbased node %d: can not generate the corresponding packet\n",here_.addr_);
return;
     }
     vbh = HDR_UWVBVA(tpacket);
   
     vbh->info.ox=mp.x;
     vbh->info.oy=mp.y;
     vbh->info.oz=mp.z;

     vbh->info.fx=mp.x;
     vbh->info.fy=mp.y;
     vbh->info.fz=mp.z;
  
     vbh->mess_type=DATA;

     
     double delay= sqrt(delay_factor)*DELAY*2.0;
     //     printf("vectorbased:node %d sets its timer for %f at %f\n", here_.addr_, delay,NOW);

     Scheduler& s=Scheduler::instance();
     s.schedule(&self_centered_forward_handler,(Event*)tpacket,delay);  

     // added by peng xie 20071118 
      PktTable.delete_hash(source,pkt_num);// to be used by timeout process     
     return;
}


void VectorbasedVoidAvoidanceAgent::processBackpressurePacket(Packet* pkt)
{
  //  printf("Vectorbasedvoidavoidance node %d process BackpressurePacket\n",here_.addr_);
  if(!pkt) {
    // printf("Vectorbasedvoidavoidance node %d processBackpressurePacket: the packet is empty\n",here_.addr_);
    return; 
  }

  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
  unsigned int msg_type =vbh->mess_type;

  ns_addr_t   target_id, source, forwarder;
  int num=vbh->pk_num;
  unsigned int packet_status=SUPPRESSED;
  
  target_id=vbh->target_id;
  source=vbh->sender_id;  
  
  position tp, fp,mp;

   tp.x=vbh->info.tx;
   tp.y=vbh->info.ty;
   tp.z=vbh->info.tz;

   fp.x=vbh->info.fx;
   fp.y=vbh->info.fy;
   fp.z=vbh->info.fz;
  
   mp.x=node->CX();
   mp.y=node->CY();
   mp.z=node->CZ();

  

   // added by peng xie 20071118
   if(!IsUpstreamNode(mp,fp,tp)){
     // not from upstream node, ignore it, 
     Packet::free(pkt);
     return;
   }    
   

   unsigned int * statusPtr= PacketStatusTable.GetHash(source,num);  
  if(!statusPtr){ 
 printf("Vectorbasedvoidavoidance: %d never process the data packet referenced by this backpressure packt\n", here_.addr_); 
       Packet::free(pkt);
       return; 
  } else {
    packet_status=(*statusPtr); 
  }

  // PktTable.MarkNextHopStatus(source,num,forwarder.addr_,DEAD); 


  // addded by peng xie at 20071117
  if ((packet_status==SUPPRESSED)||(packet_status==TERMINATED)||(packet_status==VOID_SUPPRESSED)) {
printf("Vectorbasedvoidavoidance: %d  this backpressure have been processed or not sent by this node\n", here_.addr_); 
    Packet::free(pkt);
       return;
  } 

 
    neighborhood *hashPtr= PktTable.GetHash(source, num);

    if (!hashPtr){
  printf ("vectorbasedvoidavoidance(%d): there is no record for this backpressure\n ",here_.addr_);
  Packet::free(pkt);
  return;
    }


      PktTable.MarkNextHopStatus(source,num,forwarder.addr_,DEAD); 

    neighbornode* forwarder_list= hashPtr->neighbor; 
    int num_of_forwarder=hashPtr->number;

    if(IsStuckNode(forwarder_list,&tp,num_of_forwarder,packet_status)){
   printf ("vectorbasedvoidavoidance(%d): is stuck node\n ",here_.addr_);
      if((packet_status==FORWARDED)||(packet_status==CENTER_FORWARDED)){
        PacketStatusTable.put_in_hash(source,num,FLOODED);
      	  
              Packet* p;
           if(packet_status==FORWARDED) p=generateControlDataPacket(pkt,V_SHIFT_DATA);
	   else  p=generateControlDataPacket(pkt,EXPENSION_DATA);

             Packet* pt=generateBackpressurePacket(pkt);
        	 Packet::free(pkt);

	   if(!p){
  printf ("vectorbasedvoidavoidance(%d): can not generate control data packet\n ",here_.addr_);
	     return;
	   }
       if(!pt){
  printf ("vectorbasedvoidavoidance(%d): can not generate backpressure packet\n ",here_.addr_);
	     return;
	   }
        
          PktTable.delete_hash(source,num);

          double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;     
            double d4=Random::uniform()*JITTER;    
	    //  double c=DELAY*sqrt(mini_threshold)+JITTER+d3*3+d4;
	     double c=DELAY*sqrt(3.0)*4.0+JITTER+d3*3+d4;
           Scheduler& s=Scheduler::instance();
           s.schedule(&backpressure_handler,(Event*)pt,c);

        MACprepare(p);
        MACsend(p,0);
      } else {
        PacketStatusTable.put_in_hash(source,num,TERMINATED);
        Packet* p=generateBackpressurePacket(pkt);
	Packet::free(pkt);       

        if(!p){
  printf ("vectorbasedvoidavoidance(%d): can not generate backpressure packet\n ",here_.addr_);
	     return;
	   }

        MACprepare(p);
        MACsend(p,0);
      }

      return;
    }
}




void VectorbasedVoidAvoidanceAgent::ConsiderNew(Packet *pkt)
{
  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
  unsigned int msg_type =vbh->mess_type;
  ns_addr_t source=vbh->sender_id;
  unsigned int pkt_num=vbh->pk_num;

   double l,h;
  
   neighborhood * hashPtr;
   ns_addr_t   from_nodeID, forward_nodeID, target_nodeID;

  Packet *gen_pkt;
  hdr_uwvbva *gen_vbh;

  position sp,ep,fp;
  

  sp.x=vbh->info.ox;
  sp.y=vbh->info.oy;
  sp.z=vbh->info.oz;

  ep.x=vbh->info.tx;
  ep.y=vbh->info.ty;
  ep.z=vbh->info.tz;

  fp.x=vbh->info.fx;
  fp.y=vbh->info.fy;
  fp.z=vbh->info.fz;    

  printf("Consider New!\n");
  
  //   printf ("vectorbasedvoidavoidance:(id :%d) forward:(%d ,%d) sender is(%d,%d,%d), my position is (%f,%f,%f) forward position is (%f,%f,%f) at time %f  \n",here_.addr_, vbh->forward_agent_id.addr_, vbh->forward_agent_id.port_,vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num,node->X(),node->Y(),node->Z(),vbh->info.fx,vbh->info.fy,vbh->info.fz,NOW);
 
  
  switch (msg_type) {
    case INTEREST : 
      // printf("Vectorbasedvoidavoidance:it is interest packet!\n");
      hashPtr = PktTable.GetHash(vbh->sender_id, vbh->pk_num);

      // Check if it comes from sink agent of  this node
      // If so we have to keep it in sink list 

      from_nodeID = vbh->sender_id;
      forward_nodeID = vbh->forward_agent_id;
      //  printf("Vectorbasedvoidavoidance:it the from_nodeid is %d %d  and theb this node id is %d ,%d!\n", from_nodeID.addr_,from_nodeID.port_,THIS_NODE.addr_,THIS_NODE.port_ );

      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
   
      MACprepare(pkt);
      MACsend(pkt,0); 
      //      MACsend(pkt,Random::uniform()*JITTER); 
      printf("vectorbasedvoidavoidance: after MACprepare(pkt)\n");
      }
      else
       {
          calculatePosition(pkt);
	 //printf("vectorbasedvoidavoidance: This packet is from different node\n");
	 if (IsTarget(pkt)) 
           { 
            // If this node is target?
    	      l=advance(pkt);
        
	   //  send_to_demux(pkt,0);
         //  printf("vectorbasedvoidavoidance:%d send out the source-discovery \n",here_.addr_);
	     vbh->mess_type=SOURCE_DISCOVERY;
	     setForwardDelayTimer(pkt,l*JITTER);
                 // !!! need to re-think
	   }
	 else{ 
	   // calculatePosition(pkt);
	   // No the target forwared
          l=advance(pkt);
          h=projection(pkt);
        if (IsCloseEnough(pkt)){
	  // printf("vectorbasedvoidavoidance:%d I am close enough for the interest\n",here_.addr_);
      MACprepare(pkt);
      MACsend(pkt,Random::uniform()*JITTER);//!!!! need to re-think
	}
	else { 
	  //  printf("vectorbasedvoidavoidance:%d I am not close enough for the interest  \n",here_.addr_);
         Packet::free(pkt);
              }
	 }
       }
      // Packet::free(pkt); 
      return;

  case TARGET_DISCOVERY: 
// from other nodes hitted by the packet, it is supposed
// to be the one hop away from the sink 

// printf("Vectorbasedvoidavoidance(%d,%d):it is target-discovery  packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);    
    if (THIS_NODE.addr_==vbh->target_id.addr_) {
  pk_count = 0;
  target_ = 0;    
      // ns_addr_t *hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
     // Received this packet before ?
      // if (hashPtr == NULL) { 

       calculatePosition(pkt);
       DataForSink(pkt);
       //	 printf("Vectorbasedvoidavoidance: %d is the target\n", here_.addr_);
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
   //  printf("Vectorbasedvoidavoidance(%d,%d):it is data ready packet(%d)! it target id is %d \n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_);    
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
        printf("Vectorbasedvoidavoidance: %d is the target\n", here_.addr_);
	      DataForSink(pkt); // process it
	       } 
	else{
	  // printf("Vectorbasedvoidavoidance: %d is the not  target\n", here_.addr_); 
      MACprepare(pkt);
      MACsend(pkt, Random::uniform()*JITTER);
	}
      return;
 
    case DATA :
      //    printf("Vectorbasedvoidavoidance(%d,%d):it is data packet(%d)! it target id is %d  coordinate is %f,%f,%f and range is %f\n",here_.addr_,here_.port_,vbh->pk_num,vbh->target_id.addr_,vbh->info.tx, vbh->info.ty,vbh->info.tz,vbh->range);  

 // printf("Vectorbasedvoidavoidance(%d) the traget address is %d\n",THIS_NODE.addr_,vbh->sender_id.addr_);   
  
      from_nodeID = vbh->sender_id;
      if (THIS_NODE.addr_ == from_nodeID.addr_) {       
	// come from the same node, broadcast it
           PacketStatusTable.put_in_hash(source,pkt_num,CENTER_FORWARDED);
            void_avoidance_buffer.CopyNewPacket(pkt);          

        double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;   
	// Packet* pt=pkt->copy();
        double c=2*DELAY+JITTER+d3*3;

           set_shift_timer(pkt,c); 
          
      MACprepare(pkt);
      MACsend(pkt,Random::uniform()*JITTER);
      return;      
}	     
	 if (THIS_NODE.addr_==vbh->target_id.addr_)
               {
	      sendDataTermination(pkt); 
	      PacketStatusTable.put_in_hash(source,pkt_num,TERMINATED); 
	      DataForSink(pkt); // process it
	       }

	else{
     
	 if (IsCloseEnough(pkt)){
	        recordPacket(vbh);   
                void_avoidance_buffer.CopyNewPacket(pkt);
          double delay=calculateDesirableness(pkt);
	  PacketStatusTable.put_in_hash(source, pkt_num,SUPPRESSED);// later possibly changed in forward_timeout
	 
          double d2=(UnderwaterChannel::Transmit_distance()-distance(pkt))/SPEED_OF_SOUND_IN_WATER;
          double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;   
          double d4=Random::uniform()*JITTER;    
	   setForwardDelayTimer(pkt,(sqrt(delay)*DELAY+d2*2+d3+d4));
	  } else { 
       // put the data packet into its buffer to wait for void-avoidance use
	    //!!!!!!!!!!!!!!!  reconsider this action  
              recordPacket(vbh);    
	      void_avoidance_buffer.AddNewPacket(pkt);
   }   
	}
      break;

    default :       
      Packet::free(pkt);        
      break;
  }
}


bool VectorbasedVoidAvoidanceAgent::IsNewlyTouchedNode(ns_addr_t source, unsigned int pkt_num)
{
  
  unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
  if(statusPtr) return false;
  return true;
 
  /*
  unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
  if(statusPtr) return false;
  */

  /* old version of newly touched 
  neighborhood*  ptr=PktTable.GetHash(source, pkt_num);
  if(!ptr) return true;
  int num=ptr->number; 
  if((num==1)&&(ptr->neighbor[0].forwarder_id==forwarder.addr_)) return true;
  return false;
  */
}



void VectorbasedVoidAvoidanceAgent::reset()
{
  PktTable.reset();
  /*
  for (int i=0; i<MAX_DATA_TYPE; i++) {
    routing_table[i].reset();
  }
  */
}


void VectorbasedVoidAvoidanceAgent::Terminate() 
{
#ifdef DEBUG_OUTPUT
	printf("node %d: remaining energy %f, initial energy %f\n", THIS_NODE, 
	       node->energy_model()->energy(), 
	       node->energy_model()->initialenergy() );
#endif
}



void VectorbasedVoidAvoidanceAgent::MACprepare(Packet *pkt)
{

  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
  hdr_cmn* cmh = HDR_CMN(pkt);

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
  //printf("vectorbasedvoidavoidance: inside MACprepare%d %d %d \n",node->X(),node->Y(),node->Z());
  

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

}


void VectorbasedVoidAvoidanceAgent::MACsend(Packet *pkt, Time delay)
{
  hdr_cmn*  cmh = HDR_CMN(pkt);
  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);

  // cmh->size() +=control_packet_size;
  Scheduler::instance().schedule(ll, pkt, delay);
}

bool VectorbasedVoidAvoidanceAgent::IsControlMessage(const Packet* pkt){

  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
  if ((vbh->mess_type == DATA)||(vbh->mess_type==FLOODING))
      return false;
  else
      return true;
}

bool VectorbasedVoidAvoidanceAgent::IsUpstreamNode(const position& mp, const position& fp, const position& tp){

 double dtx=tp.x-mp.x;
 double dty=tp.y-mp.y;
 double dtz=tp.z-mp.z;  

 // double mydistance= sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double dis=calculateMappedDistance(&mp,&tp,&fp);
 if(dis>0) return true;
 else false;   
}



void VectorbasedVoidAvoidanceAgent::DataForSink(Packet *pkt)
{

  //  printf("DataforSink: the packet is send to demux\n");
      send_to_dmux(pkt, 0);

}



void VectorbasedVoidAvoidanceAgent::trace (char *fmt,...)
{
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer(), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}

void VectorbasedVoidAvoidanceAgent::setForwardDelayTimer(Packet* pkt, double c){
   printf(">>> vectorbased node(%d) is setting the timer %f at %f\n",THIS_NODE.addr_,c,NOW);
      Scheduler& s=Scheduler::instance();
      s.schedule(&forward_delay_handler,(Event*)pkt,c);
}
 



void VectorbasedVoidAvoidanceAgent::set_shift_timer(Packet* pkt, double c){
 if(!pkt) {
   // printf("Vectorbasedvoidavoidance: node(%d) the packet does exist in the set_shifter_timer \n",here_.addr_);
   return;
 }

 hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
 
 ns_addr_t source=vbh->sender_id;
 unsigned int pkt_num=vbh->pk_num;
 position s_position,t_position;

 unsigned int* status=PacketStatusTable.GetHash(source,pkt_num);
 if(!status) {
   //   printf("Vectorbasedvoidavoidance: node(%d) the packet status does exist in the set_shifter_timer \n",here_.addr_);
   return;
 }

 
//  added by peng xiw 20071118


/*
 s_position.x=vbh->info.ox;
 s_position.y=vbh->info.oy;
 s_position.z=vbh->info.oz;
*/

 s_position.x=vbh->info.fx;
 s_position.y=vbh->info.fy;
 s_position.z=vbh->info.fz;



 t_position.x=vbh->info.tx;
 t_position.y=vbh->info.ty;
 t_position.z=vbh->info.tz;

 //printf("vectorbased: node(%d) set v-shift  timer sp (%f,%f,%f) tp (%f,%f,%f) \n",here_.addr_,vbh->info.ox,vbh->info.oy,vbh->info.oz,vbh->info.tx,vbh->info.ty,vbh->info.tz);

 Packet* p=generateVectorShiftPacket(&source, pkt_num,&s_position,&t_position);
 
 if(!p) {
   printf ("vectorbasedvoidavoidance (%d): can not generate v_shift data\n ",here_.addr_);
   return;
 }
 hdr_uwvbva* vbh1 = HDR_UWVBVA(p);  
 if((*status)==CENTER_FORWARDED) vbh1->mess_type=EXPENSION;

 //printf ("vectorbasedvoidavoidance (%d): sets void_avoidance timer at %f and delay is %f\n ",here_.addr_,NOW,c);
       Scheduler& s=Scheduler::instance();
       s.schedule(&void_avoidance_handler,(Event*)p,c);
      return;
}



void VectorbasedVoidAvoidanceAgent::process_backpressure_timeout(Packet* pkt)
{  
  //printf ("vectorbasedvoidavoidance (%d): processp  back pressure timeout at %f\n ",here_.addr_,NOW);
  if(!pkt) {
    // printf ("vectorbasedvoidavoidance %d: back pressure packet is null\n ",here_.addr_);
 return;
  }
 
  hdr_uwvbva* vbh = HDR_UWVBVA(pkt);

  hdr_cmn* cmh = HDR_CMN(pkt);
  int size=cmh->size();

   ns_addr_t source=vbh->sender_id;
   ns_addr_t forward=vbh->forward_agent_id;


   
   unsigned int pkt_num=vbh->pk_num;
 
    neighbornode* forwarder_list;
    int num_of_forwarder;
  

      unsigned int * statusPtr=PacketStatusTable.GetHash(source, pkt_num);
      if(!statusPtr) {
 printf ("vectorbasedvoidavoidance(%d): The packet is already terminated!\n ",here_.addr_);
      Packet::free(pkt);
      return;
      }

      if ((*statusPtr)==TERMINATED){
    printf ("vectorbasedvoidavoidance(%d): The packet is already terminated!\n ",here_.addr_);
      Packet::free(pkt);
  return;
     }
     

     
     if(IsEndNode(source, pkt_num)){   
        printf("vectorbased node %d is an end node size is %d\n",here_.addr_,size); 
  
        MACprepare(pkt);    
        MACsend(pkt,0);   
        PacketStatusTable.put_in_hash(source,pkt_num,TERMINATED); 
    } else {
printf("vectorbased node %d is not a end node\n",here_.addr_);
Packet::free(pkt);
    }

    return;
}





void VectorbasedVoidAvoidanceAgent::process_selfcentered_timeout(Packet* pkt)
{
  // printf ("vectorbasedvoidavoidance(%d):self-centered timer expires! at %f\n ",here_.addr_,NOW);
  if(!pkt)// printf ("vectorbasedvoidavoidance(%d): the packet doesn't exist \n ",here_.addr_);

    neighbornode* forwarder_list;
    int num_of_forwarder;
    ns_addr_t source;
    ns_addr_t forward;
    unsigned int pkt_num;
    hdr_uwvbva* vbh=HDR_UWVBVA(pkt);   
  
    source=vbh->sender_id;
    forward=vbh->forward_agent_id;
    pkt_num=vbh->pk_num;

    unsigned int  *statusPtr=PacketStatusTable.GetHash(source, pkt_num);
  
    if(!statusPtr) {
 printf ("vectorbasedvoidavoidance(%d): The packet status is null!\n ",here_.addr_);
    Packet::free(pkt);
    return;
    }
    
    if(((*statusPtr)==TERMINATED)||((*statusPtr)==FLOODED))
    {
  printf ("vectorbasedvoidavoidance(%d): The packet is already terminated or self-center forwarded!\n ",here_.addr_);
    Packet::free(pkt);
    return;
     }
   
    /*
    neighborhood *hashPtr=PktTable.GetHash(source, pkt_num);
    int num=hashPtr->number; 
    neighbornode* neighbor_list=hashPtr->neighbor;
    */

    if (!IsWorthFloodingForward(source,pkt_num))
     {
     printf ("vectorbasedvoidavoidance(%d): is not worth forwarding this packet\n ",here_.addr_);

     //????????????????????????
     //      PacketStatusTable.put_in_hash(source,pkt_num, SUPPRESSED); 
     //       PacketStatusTable.delete_hash(source,pkt_num); 
      Packet::free(pkt);
      return;
    }

    //        printf ("vectorbasedvoidavoidance(%d): is worth forwarding this packet\n ",here_.addr_);
       
          double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;     
          double d4=Random::uniform()*JITTER;    
          set_shift_timer(pkt,(sqrt(mini_threshold)*DELAY*2+d3*3+d4)); 
	

	  /*
	Packet* pt=generateBackpressurePacket(&source,pkt_num);
	PktTable.delete_hash(source, pkt_num);

            double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;
     
            double d4=Random::uniform()*JITTER;    
	    
	    //   double c=DELAY*sqrt(mini_threshold)+JITTER+d3*3+d4;
            double c=DELAY*sqrt(3.0)*4.0+d3*3+d4;
  

       Scheduler& s=Scheduler::instance();
       s.schedule(&backpressure_handler,(Event*)pt,c);
	  */
         
     if(*statusPtr==VOID_SUPPRESSED)PacketStatusTable.put_in_hash(source,pkt_num, CENTER_FORWARDED);
     else PacketStatusTable.put_in_hash(source,pkt_num, FORWARDED);

     MACprepare(pkt);
     MACsend(pkt,0);
       return;
}
 	    
 


void VectorbasedVoidAvoidanceAgent::process_forward_timeout(Packet * pkt){
  //printf("vectorbased: node (%d) pkt  self-adaption timeout at %f\n", here_.addr_,NOW);  
 hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
 unsigned char msg_type =vbh->mess_type;
 neighborhood  *hashPtr;
 int c=0;
 double tdelay=calculateDesirableness(pkt);
 double td=tdelay;
 ns_addr_t source=vbh->sender_id;
 unsigned int pkt_num=vbh->pk_num;
 unsigned int status;
     unsigned int  *statusPtr= PacketStatusTable.GetHash(source, pkt_num);
     int ncounter=0;

     position tsp,ttp,tmp,tfp;

     tsp.x=vbh->info.ox;
     tsp.y=vbh->info.oy;
     tsp.z=vbh->info.oz;


     ttp.x=vbh->info.tx;
     ttp.y=vbh->info.ty;
     ttp.z=vbh->info.tz;
 
     tfp.x=vbh->info.fx;
     tfp.y=vbh->info.fy;
     tfp.z=vbh->info.fz;

     tmp.x=node->CX();
     tmp.y=node->CY();
     tmp.z=node->CZ();
  
     //     td=calculateDelay(&tfp,&ttp,&tmp,&tfp);

     /*
     if(statusPtr){
    printf ("vectorbasedvoidavoidance(%d): The packet is already processed!\n ",here_.addr_);
  Packet::free(pkt);
  return;
     }
     */
    
     // printf("vectorbased: node (%d) pkt %d self-adaption timeout at %f\n", here_.addr_,pkt_num,NOW);  
 switch (msg_type){
 case DATA:
       hashPtr= PktTable.GetHash(vbh->sender_id, vbh->pk_num);
      

	if (hashPtr) {
          int num_neighbor=hashPtr->number;
	  position mysp,myep;
          int i=0;

	  mysp.x=vbh->info.ox;
          mysp.y=vbh->info.oy;
          mysp.z=vbh->info.oz;

          myep.x=vbh->info.tx;
          myep.y=vbh->info.ty;
          myep.z=vbh->info.tz;

                       
	       position  sp,fp;
	       tdelay=1000;

	       //printf("vectorbased: node (%d) self-adaption, num of neighbor is %d\n", here_.addr_,num_neighbor);  
	         while (i<num_neighbor){
		     sp=hashPtr->neighbor[i].vec.start;
                     fp=hashPtr->neighbor[i].node;
		     //printf("vectorbased: node (%d) self-adaption, sp.x=  %f sp.y=%f and sp.z=%f, fp.x=%f, fp.y=%f and fp.z=%f, myposition is (%f,%f,%f)\n", here_.addr_,sp.x, sp.y,sp.z,fp.x, fp.y,fp.z,node->X(), node->Y(),node->Z()); 		   
		     double t2delay=calculateDelay(pkt,&fp);
		     //printf("vectorbased: node (%d) self-adaption, t2delay is  %f\n", here_.addr_,t2delay); 
		 if (t2delay<tdelay) tdelay=t2delay;
		 	 i++; 
		 }
		 ncounter=i;
	}
		if(ncounter>0) ncounter--; // delete my first packet record    

		priority=mini_threshold/pow(2.0,ncounter);
                // priority=mini_threshold;

 
               if(tdelay<=priority) {  
		 // printf("vectorbased: node (%d) is still worth forwarding the data packet c=%d and tdelay=%f \n", here_.addr_,ncounter,tdelay);  

double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;  
            double d4=Random::uniform()*JITTER;    
         
           set_shift_timer(pkt,(sqrt(mini_threshold)*DELAY*2+d3*3+d4)); 
           PacketStatusTable.put_in_hash(source, pkt_num,FORWARDED);
               MACprepare(pkt);
               MACsend(pkt,0);      
		 } else{
		   //printf("vectorbased: node (%d) is not worth forwarding the data packet c=%d and tdelay=%f \n", here_.addr_,c,tdelay);       
// PktTable.MarkNextHopStatus(vbh->sender_id, vbh->pk_num,forwarder_id, SUPPRESSED);//??
// PacketStatusTable.put_in_hash(source, pkt_num,SUPPRESSED);
//  if(ncounter==0)     PacketStatusTable.delete_hash(source, pkt_num); 
//  if(td>2.0)     PacketStatusTable.delete_hash(source, pkt_num);
	   Packet::free(pkt); //to much overlap, don't send 
	       }
	break; 
 default: 
       break;
 }
}


//not necessary
 void VectorbasedVoidAvoidanceAgent::makeCopy(Packet* pkt){
     Packet* p1=pkt->copy();
     void_avoidance_buffer.AddNewPacket(pkt);        
 }


 void VectorbasedVoidAvoidanceAgent::sendFloodingPacket(Packet* pkt){
  
     hdr_uwvbva* vbh=HDR_UWVBVA(pkt);
    
     vbh->mess_type=FLOODING;   
 
     vbh->info.fx=node->CX();
     vbh->info.fy=node->CY();
     vbh->info.fz=node->CZ();

     vbh->info.ox=node->CX();
     vbh->info.oy=node->CY();
     vbh->info.oz=node->CZ();

     vbh->forward_agent_id=here_;
     
     //     printf ("vectorbasedvoidavoidance(%d): sends the flooding packet at %f !\n ",here_.addr_, NOW);
     MACprepare(pkt);    
     MACsend(pkt,0);
     return;
 }


void VectorbasedVoidAvoidanceAgent::process_void_avoidance_timeout(Packet* pkt)
{
  //printf ("vectorbasedvoidavoidance(%d): void_avoidance timeout at  %f !\n ",here_.addr_, NOW);

  if(!pkt) {
    //printf ("vectorbasedvoidavoidance(%d): void_avoidance timeout at  %f ! and the packet is empty\n ",here_.addr_, NOW);
 return;
  }
    hdr_uwvbva* vbh=HDR_UWVBVA(pkt);
    ns_addr_t source=vbh->sender_id;
    ns_addr_t forward=vbh->forward_agent_id;
    unsigned int pkt_num=vbh->pk_num;

    position t_p;
    position sp;
    unsigned int  * statusPtr=0;


    t_p.x=vbh->info.tx;
    t_p.y=vbh->info.ty;
    t_p.z=vbh->info.tz;

    sp.x=vbh->info.ox;
    sp.y=vbh->info.oy;
    sp.z=vbh->info.oz;

    neighbornode* forwarder_list;
    int num_of_forwarder;

    // printf ("vectorbasedvoidavoidance(%d): the timer for v_shift expires at %f !\n ",here_.addr_, NOW);

    statusPtr= PacketStatusTable.GetHash(source, pkt_num);
    
     if(statusPtr && (((*statusPtr)==TERMINATED) ||((*statusPtr)==FLOODED)))  { 
 
printf ("vectorbasedvoidavoidance(%d): The packet is already terminated!\n ",here_.addr_);
 Packet::free(pkt);
  return;
     }


       if (IsVoidNode(source,pkt_num,&sp)){

     //         if (IsVoidNode(source,pkt_num)){
       printf ("vectorbasedvoidavoidance(%d): is void node\n ",here_.addr_);
     
        PacketStatusTable.put_in_hash(source,pkt_num,FLOODED);    
        
	// Packet* pt=generateBackpressurePacket(pkt);

	// MACprepare(pkt);  
        MACsend(pkt, 0);       


        Packet* pdata=void_avoidance_buffer.LookupCopy(source,pkt_num); 

   if(!pdata){
     printf("Vectorbasedvoidavoidance: %d the data packet referenced by this flooding packet does not exist\n", here_.addr_);
     return; 
   }

	//	Packet* pt=generateBackpressurePacket(&source,pkt_num);
        
        Packet* pt=generateBackpressurePacket(pdata);

	PktTable.delete_hash(source, pkt_num);
        double d3=(UnderwaterChannel::Transmit_distance())/SPEED_OF_SOUND_IN_WATER;
     
            double d4=Random::uniform()*JITTER;    
	    //   double c=DELAY*sqrt(mini_threshold)+JITTER+d3*3+d4;
            double c=DELAY*sqrt(3.0)*4.0+d3*3+d4;

       if(!pt){
 printf("Vectorbasedvoidavoidance(%d): can not generate backpressure packet\n ",here_.addr_);
 return;
       } else {
printf("Vectorbasedvoidavoidance(%d): set timer  backpressure packet(%d) delay=%f at %f\n ",here_.addr_,pkt_num,c,NOW);
       Scheduler& s=Scheduler::instance();
       s.schedule(&backpressure_handler,(Event*)pt,c);
      return;   
       }
    } else{
   printf ("vectorbasedvoidavoidance(%d): is not a void node\n ",here_.addr_);
   Packet::free(pkt);
   return;
     }
}



void VectorbasedVoidAvoidanceAgent::sendDataTermination(const Packet* p)
{
     hdr_uwvbva* vbh2=HDR_UWVBVA(p);
     ns_addr_t source=vbh2->sender_id; 
     unsigned int pkt_num=vbh2->pk_num;
 
      DataTerminationPktTable.put_in_hash(vbh2);


      Packet * pkt=Packet::alloc();
     

      hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
      hdr_ip* iph = HDR_IP(pkt);
      hdr_cmn*  cmh = HDR_CMN(pkt);
     

      cmh->ptype()=PT_UWVB;
      cmh->size() =control_packet_size*8;

      iph->src_=here_;
      iph->dst_.addr_=here_.addr_;
      iph->dst_.port_=255;


      vbh->mess_type =DATA_TERMINATION;
      vbh->pk_num = pkt_num;
      vbh->ts_=NOW;     
      vbh->sender_id = source;
      vbh->forward_agent_id=here_;       
 
      vbh->info.ox=node->X();
      vbh->info.oy=node->Y();
      vbh->info.oz=node->Z();

      vbh->info.fx=node->X();
      vbh->info.fy=node->Y();
      vbh->info.fz=node->Z();
 
        cmh->xmit_failure_ = 0;
        cmh->next_hop() = MAC_BROADCAST; 
        cmh->addr_type() = NS_AF_ILINK;  
        cmh->direction() = hdr_cmn::DOWN;
    
        MACsend(pkt, 0);
       printf("node (%d,%d) send data termination %d at %lf\n ",here_.addr_,here_.port_,pkt_num,NOW);
          
}

// Is sp useful? 
Packet* VectorbasedVoidAvoidanceAgent::generateVectorShiftPacket(const ns_addr_t* source,int pkt_num, const position* sp, const position* tp)
{
  //printf("Vectorbasedvoidavoidance node (%d) generates V_Shift\n",here_.addr_);

      Packet * v_shift=Packet::alloc();

 if(!v_shift) {
printf("Vectorbasedvoidavoidance node (%d) can't not generate v_shift packet since the data packet is empty\n",here_.addr_);
return NULL;
  }

printf("vectorbased: node(%d) generate v-shift sp (%f,%f,%f) tp (%f,%f,%f) \n",here_.addr_,sp->x,sp->y,sp->z,tp->x,tp->y,tp->z);


      hdr_uwvbva* vbh = HDR_UWVBVA(v_shift);
      hdr_ip* iph = HDR_IP(v_shift);
      hdr_cmn*  cmh = HDR_CMN(v_shift);
     

      cmh->ptype()=PT_UWVB;
      cmh->size() =control_packet_size*8;

      iph->src_=here_;
      iph->dst_.addr_=here_.addr_;
      iph->dst_.port_=255;


      vbh->mess_type =V_SHIFT;
      vbh->pk_num = pkt_num;
      vbh->ts_=0;     
      vbh->sender_id =(*source);
      vbh->forward_agent_id=here_;       
 
      /*
      vbh->info.ox=node->CX();
      vbh->info.oy=node->CY();
      vbh->info.oz=node->CZ();
      */

      // delete by peng xie 20071118

      
      vbh->info.ox=sp->x;
      vbh->info.oy=sp->y;
      vbh->info.oz=sp->z;
      


      vbh->info.fx=node->CX();
      vbh->info.fy=node->CY();
      vbh->info.fz=node->CZ();

      vbh->info.tx=tp->x;
      vbh->info.ty=tp->y;
      vbh->info.tz=tp->z;

      vbh->original_source=(*sp); 

        cmh->xmit_failure_ = 0;
        cmh->next_hop() = MAC_BROADCAST;        
     cmh->addr_type() = NS_AF_ILINK;  
        cmh->direction() = hdr_cmn::DOWN;
	return v_shift;          
}


Packet* VectorbasedVoidAvoidanceAgent::generateControlDataPacket(Packet* packet, unsigned int type)
{
 if(!packet) {
   //printf("Vectorbasedvoidavoidance node (%d) can't not generate control data packet since the data packet is empty\n",here_.addr_);
return NULL;
  }

      Packet * vD=packet->copy();

if(!vD) {
printf("Vectorbasedvoidavoidance node (%d) can't not generate control data packet due to the failure of meme allocation\n",here_.addr_);
return NULL;
  }
      hdr_uwvbva* vbh = HDR_UWVBVA(vD);
      hdr_ip* iph = HDR_IP(vD);
      hdr_cmn*  cmh = HDR_CMN(vD);
     
      cmh->ptype()=PT_UWVB;
     
      /*
      iph->src_=here_;
      iph->dst_.addr_=here_.addr_;
      iph->dst_.port_=255;
      */

      vbh->mess_type =type;
      vbh->ts_=0;     
      
       vbh->forward_agent_id=here_;

        cmh->xmit_failure_ = 0;
        cmh->next_hop() = MAC_BROADCAST; 
        cmh->addr_type() = NS_AF_ILINK;  
        cmh->direction() = hdr_cmn::DOWN;
	return vD;          
}



Packet* VectorbasedVoidAvoidanceAgent::generateBackpressurePacket(Packet* packet)
{

  if(!packet) {
    //printf("Vectorbasedvoidavoidance node (%d) can't not generate backpressure since the data packet is empty\n",here_.addr_);
return NULL;
  }
      Packet * backpressure=packet->copy();

 if(!backpressure) {
printf("Vectorbasedvoidavoidance node (%d) can't not generate backpressure due to the meme allocation\n",here_.addr_);
return NULL;
  }


      hdr_uwvbva* vbh = HDR_UWVBVA(backpressure);
      hdr_ip* iph = HDR_IP(backpressure);
      hdr_cmn*  cmh = HDR_CMN(backpressure);
     
      cmh->ptype()=PT_UWVBVA;
     
      
      iph->src_=here_;
      iph->dst_.addr_=here_.addr_;
      iph->dst_.port_=255;
      

      vbh->mess_type =BACKPRESSURE;
      vbh->ts_=0;     
      
       vbh->forward_agent_id=here_;
 
        cmh->xmit_failure_ = 0;
        cmh->next_hop() = MAC_BROADCAST; 
        cmh->addr_type() = NS_AF_ILINK;  
        cmh->direction() = hdr_cmn::DOWN;
	return backpressure;          
}



double VectorbasedVoidAvoidanceAgent::calculateFloodingDesirableness(const Packet* pkt)
{
  
   double d1=distance(pkt);  
   double dt=UnderwaterChannel::Transmit_distance(); 
   double dr=dt-d1;
   if(dr<0) dr=0.0; // in case of location error
   double d2=dr/SPEED_OF_SOUND_IN_WATER;

     return (dr/dt)*DELAY+2*d2;       
  
}


double VectorbasedVoidAvoidanceAgent::calculateDesirableness(const Packet* pkt)
{

    hdr_uwvbva* vbh = HDR_UWVBVA(pkt);
    position sp,tp,fp, mp;
 

	sp.x=vbh->info.ox;
        sp.y=vbh->info.oy;
        sp.z=vbh->info.oz;
   
     
   tp.x=vbh->info.tx;
   tp.y=vbh->info.ty;
   tp.z=vbh->info.tz;

   fp.x=vbh->info.fx;
   fp.y=vbh->info.fy;
   fp.z=vbh->info.fz;


   mp.x=node->CX();
   mp.y=node->CY();
   mp.z=node->CZ();

   return calculateDelay(&sp,&tp,&mp,&fp);
}






bool VectorbasedVoidAvoidanceAgent::IsStuckNode(const neighbornode* neighbor_list,const position* tp,int num_of_neighbor, unsigned int status )
{

  position mp;

  mp.x=node->CX();
  mp.y=node->CY();
  mp.z=node->CZ();
 
  double mydis=distance(tp,&mp);
  //  unsigned int * statusPtr= PacketStatusTable.GetHash(source,pk_num);  


    for(int i=0;i<num_of_neighbor;i++){
	position fp=neighbor_list[i].node;
        double tmp_dis=distance(tp, &fp);
        if ((tmp_dis<mydis)&&(neighbor_list[i].status!=DEAD)) return false;
    }
    if((status==FORWARDED)||(status==CENTER_FORWARDED)) return true;    
    if(status==FLOODED){
    for(int i=0;i<num_of_neighbor;i++){
	position fp=neighbor_list[i].node;
        position sp=neighbor_list[i].vec.start;
        if ((IsSamePosition(&fp,&sp))&&(neighbor_list[i].status!=DEAD)) return false;
    }
    }
    return true;
}


bool VectorbasedVoidAvoidanceAgent::IsWorthFloodingForward(ns_addr_t source, int pkt_num)
{
    printf("vectorbased: node(%d) is determining if it worth flooding forward \n",here_.addr_);
 
    neighborhood *hashPtr=PktTable.GetHash(source, pkt_num);
    if(!hashPtr) return true;

    int num=hashPtr->number; 
     neighbornode* neighbor_list=hashPtr->neighbor;

    // to check if there is some one self-centered sending the packet before me?? 
    for(int i=0;i<num;i++){
	position fp=neighbor_list[i].node;
        position sp=neighbor_list[i].vec.start;
        // added by peng xie
	unsigned fid=neighbor_list[i].forwarder_id;

	//added by peng xie 20071118
     if(IsSamePosition(&fp,&sp)&&((fid!=source.addr_))) return false;
    }
    return true;
}





bool VectorbasedVoidAvoidanceAgent::IsEndNode(ns_addr_t source,int pkt_num)
{
    printf("vectorbased: node(%d) is determining if it is an end node\n",here_.addr_);
   
    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);
    if(!hashPtr) return true;

     neighbornode* neighbor_list= hashPtr->neighbor;       
     int num_of_neighbor=hashPtr->number;
  
    for(int i=0;i<num_of_neighbor;i++){
	position fp=neighbor_list[i].node;
        position op=neighbor_list[i].vec.start;
        unsigned int forwarder_id=neighbor_list[i].forwarder_id;
	//printf("vectorbased: node(%d)  fp.x=%f,fp.y=%f and pf.z=%f op.x=%f, op.y=%f and op.z=%f\n",here_.addr_,fp.x,fp.y,fp.z,op.x,op.y,op.z);

        if (IsSamePosition(&op,&fp)) return false;
    }
    return true;
}




bool VectorbasedVoidAvoidanceAgent::IsVoidNode(ns_addr_t source, int pkt_num,const position* sp)
{
    printf("vectorbased: node(%d) is determining if it is void node\n",here_.addr_);

    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);

    if (!hashPtr) return true;

    neighbornode* neighbor_list= hashPtr->neighbor; 
    int num_of_neighbor=hashPtr->number;
     position mp;
     //  position sp=neighbor_list[0].vec.start; 
      position tp=neighbor_list[0].vec.end;

     mp.x=node->CX();
    mp.y=node->CY();
    mp.z=node->CZ();   
      printf("my position (%f %f %f) sp is (%f %f %f) tp is(%f %f %f)\n",mp.x,mp.y,mp.z, sp->x,sp->y,sp->z,tp.x,tp.y,tp.z);
 double myadvance=calculateMappedDistance(sp,&tp,&mp);

    printf("vectorbased: node(%d) is determining if it is void node,  # of neighbor is %d  dis is %f\n",here_.addr_,num_of_neighbor,myadvance);
    if(num_of_neighbor<=1) return true; // I only has one packet record in my hashtable
    for(int i=0;i<num_of_neighbor;i++){     
      position fp=neighbor_list[i].node;
    
      double advance=calculateMappedDistance(sp,&tp,&fp);
      //   printf("neighbor position (%f %f %f) dis is %f\n",fp.x,fp.y,fp.z,advance);
        if (advance>myadvance) return false;
    }
    return true;
}


// added by peng xie at 20071106 sp is start point and tp is the target

double VectorbasedVoidAvoidanceAgent::calculateMappedDistance(const position*  sp, const position* tp, const position* fp)
{

  if(IsSamePosition(sp, fp)) return 0.0;
 double fx=sp->x;
 double fy=sp->y;
 double fz=sp->z;

 double dx=fp->x-fx; 
 double dy=fp->y-fy;
 double dz=fp->z-fz;

  
 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z; 

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;  

 double dp=dx*dtx+dy*dty+dz*dtz;

 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);

    double mdis=d*cos_theta;
      return mdis;
}

bool VectorbasedVoidAvoidanceAgent::IsVoidNode(ns_addr_t source, int pkt_num)
{
    printf("vectorbased: node(%d) is determining if it is void node\n",here_.addr_);

    neighborhood *hashPtr= PktTable.GetHash(source, pkt_num);

    if (!hashPtr) return true;

    neighbornode* neighbor_list= hashPtr->neighbor; 
    int num_of_neighbor=hashPtr->number;
   
    position mp,tp;

    mp.x=node->CX();
    mp.y=node->CY();
    mp.z=node->CZ();

    tp=neighbor_list[0].vec.end;

    double mydis=distance(&tp,&mp);
    for(int i=0;i<num_of_neighbor;i++){
	position fp=neighbor_list[i].node;
        double tmp_dis=distance(&tp, &fp);
        if (tmp_dis<mydis) return false;
    }
    return true;
}

double VectorbasedVoidAvoidanceAgent::calculateSelfCenteredDelay(const position* sp,const position*  tp, const position* myp, const position* fp)
{


printf("vectorbased: node(%d) sp (%f,%f,%f) tp (%f,%f,%f) and fp(%f,%f,%f)\n",here_.addr_,sp->x,sp->y,sp->z,tp->x,tp->y,tp->z,fp->x,fp->y,fp->z);
 double fx=fp->x;
 double fy=fp->y;
 double fz=fp->z;

 double dx=myp->x-fx; 
 double dy=myp->y-fy;
 double dz=myp->z-fz;

  
 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z; 

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;  

 double dp=dx*dtx+dy*dty+dz*dtz;

 double p=projection(fp,tp,myp);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);

   double delay=(1.0-(p/width)) +((UnderwaterChannel::Transmit_distance()-d*cos_theta)/UnderwaterChannel::Transmit_distance());

 //double delay=(1.0-(p/width));
 
printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f l is %f\n",here_.addr_,p, cos_theta, d,l);
   if(delay<0.0) delay=0.0; // in case the location error, which may result in negative delay
      return sqrt(delay);
   //  return sqrt(delay)*DELAY*2.0;
}



double  VectorbasedVoidAvoidanceAgent::distance(const position* sp, const position* tp){
 
 double fx=sp->x;
 double fy=sp->y;
 double fz=sp->z;

 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z; 

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;  

 return sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
}

double VectorbasedVoidAvoidanceAgent::calculateDelay(const position* sp,const position*  tp,
                                                const position* myp, const position* fp)
{

 double fx=fp->x;
 double fy=fp->y;
 double fz=fp->z;

 double dx=myp->x-fx; 
 double dy=myp->y-fy;
 double dz=myp->z-fz;

  
 double tx=tp->x;
 double ty=tp->y;
 double tz=tp->z; 

 double dtx=tx-fx;
 double dty=ty-fy;
 double dtz=tz-fz;  

 double dp=dx*dtx+dy*dty+dz*dtz;

 double p=projection(sp,tp,myp);
 double d=sqrt((dx*dx)+(dy*dy)+ (dz*dz));
 double l=sqrt((dtx*dtx)+(dty*dty)+ (dtz*dtz));
 double cos_theta=dp/(d*l);

   double delay=(p/width) +((UnderwaterChannel::Transmit_distance()-d*cos_theta)/UnderwaterChannel::Transmit_distance());
 
// printf("vectorbased: node(%d) projection is %f, and cos is %f, and d is %f)\n",here_.addr_,p, cos_theta, d);
   if(delay<0.0) delay=0.0; // in case the location error, which may result in negative delay
   return delay;
}


double VectorbasedVoidAvoidanceAgent::projection(const position* sp, const position* tp,const position * p)
{
// two projection functions should be merged later
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
 // printf("vectorbasedvoidavoidance: the area is %f and length is %f\n",area,length);
 return area/length;
}


void VectorbasedVoidAvoidanceAgent::calculatePosition(Packet* pkt)
{


  node->CX_=node->X();
  node->CY_=node->Y();
  node->CZ_=node->Z();

  /*
 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt); 
 double fx=vbh->info.fx;
 double fy=vbh->info.fy;
 double fz=vbh->info.fz;

 double dx=vbh->info.dx;
 double dy=vbh->info.dy;
 double dz=vbh->info.dz;

 node->CX_=fx+dx;
 node->CY_=fy+dy;
 node->CZ_=fz+dz;
 printf("vectorbasedvoidavoidance: my position is computed as (%f,%f,%f) dx=%f dy=%f and dz=%f \n",node->CX_, node->CY_,node->CZ_,dx,dy,dz);
  */
}

double VectorbasedVoidAvoidanceAgent::calculateDelay(Packet* pkt,position* p1)
{
 
 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt); 
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


double VectorbasedVoidAvoidanceAgent::distance(const Packet* pkt)
{
 
 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt); 
 double tx=vbh->info.fx;
 double ty=vbh->info.fy;
 double tz=vbh->info.fz;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=node->CX(); //change later
 double y=node->CY();// printf(" Vectorbasedvoidavoidance: I am in advanced\n");
 double z=node->CZ();
 printf("the forwarder  is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}


double VectorbasedVoidAvoidanceAgent::advance(Packet* pkt)
{
 
 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt); 
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;
 // printf("vectorbased: the target is %lf,%lf,%lf \n",tx,ty,tz);
 double x=node->CX(); //change later
 double y=node->CY();// printf(" Vectorbasedvoidavoidance: I am in advanced\n");
 double z=node->CZ();
 // printf("the target is %lf,%lf,%lf and my coordinates are %lf,%lf,%lf\n",tx,ty,tz,x,y,z);
 return sqrt((tx-x)*(tx-x)+(ty-y)*(ty-y)+ (tz-z)*(tz-z));
}


double VectorbasedVoidAvoidanceAgent::projection(Packet* pkt)
{

 hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt);
 
 double tx=vbh->info.tx;
 double ty=vbh->info.ty;
 double tz=vbh->info.tz;
 

 double ox=vbh->info.ox;
 double oy=vbh->info.oy;
 double oz=vbh->info.oz;
 

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
 return area/length;
}

bool VectorbasedVoidAvoidanceAgent::IsTarget(Packet* pkt)
{
  hdr_uwvbva * vbh=HDR_UWVBVA(pkt);

  if (vbh->target_id.addr_==0){

  //  printf("vectorbased: advanced is %lf and my range is %f\n",advance(pkt),vbh->range);
    return (advance(pkt)<vbh->range);
}
  else return(THIS_NODE.addr_==vbh->target_id.addr_);


}



bool VectorbasedVoidAvoidanceAgent::IsCloseEnough(Packet* pkt)
{
  hdr_uwvbva     *vbh  = HDR_UWVBVA(pkt);
  double range=vbh->range;

 
  position sp, tp,p;

 
  sp.x=vbh->info.ox;
  sp.y=vbh->info.oy;
  sp.z=vbh->info.oz;
  
  tp.x=vbh->info.tx;
  tp.y=vbh->info.ty;
  tp.z=vbh->info.tz;

  p.x=node->CX();
  p.y=node->CY();
  p.z=node->CZ();

  printf ("vectorbasedvoidavoidance(%d): The projection is %f\n ",here_.addr_,projection(&sp,&tp,&p));  
 if ((projection(&sp,&tp,&p)<=width))  return true;
 return false;

}


int VectorbasedVoidAvoidanceAgent::command(int argc, const char*const* argv)
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
   // if (strcasecmp(argv[1], "stop-source")==0) {
   // StopSource();
   // return TCL_OK;
   // }

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
    fprintf(stderr, "Vectorbasedvoidavoidanceing Node: %d lookup of %s failed\n", THIS_NODE.addr_, argv[2]);
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
      // printf("vectorbasedvoidavoidance:port demux is called \n");
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

