#include "uw_routing_buffer.h"
#include "vectorbasedforward.h"
#include "vectorbasedvoidavoidance.h"
void 
RoutingBuffer::AddNewPacket(Packet* p){
  routing_buffer_cell* t1=new routing_buffer_cell;


  hdr_uwvb* vbh=HDR_UWVB(p);
  hdr_uwvbva* vbvah=HDR_UWVBVA(p);


   ns_addr_t source;
   if (usr) source=vbh->sender_id;
   else source=vbvah->sender_id;
  unsigned int pkt_num;
   if(usr) pkt_num=vbh->pk_num;
   else pkt_num= vbvah->pk_num;


 Packet* tpkt=DeQueue(source,pkt_num); // avoid duplication
  if(tpkt)Packet::free(tpkt);


  if (IsFull()) {
//      printf("ok, full\n");
      Packet::free(Dehead());
      }
  if(!t1){ 
//  printf("routingBuffer: can't get allocated  memory\n");
  return;
  }

  t1->packet=p;
  t1->next=NULL;

  if(head_==NULL) {
      // printf("head is empty ok\n");
     tail_=t1;
     head_=t1;
  }
  else{
      //  printf("head is not empty ok\n");
  tail_->next=t1;
  tail_=t1;
  }
  
  num_of_packet++;
}


void 
RoutingBuffer::CopyNewPacket(Packet* pkt){
  routing_buffer_cell* t1=new routing_buffer_cell;
  Packet* p=pkt->copy();
 

  hdr_uwvb* vbh=HDR_UWVB(pkt);
  hdr_uwvbva* vbvah=HDR_UWVBVA(p);


 
   ns_addr_t source;
   if (usr) source=vbh->sender_id;
   else source=vbvah->sender_id;
  unsigned int pkt_num;
   if(usr) pkt_num=vbh->pk_num;
   else pkt_num= vbvah->pk_num;


   /*
  ns_addr_t source=((usr)?: vbh->sender_id,vbvah->sender_id);
  unsigned int pkt_num=((usr)?: vbh->pk_num, vbvah->pk_num);

  
  ns_addr_t source=vbh->sender_id;
  unsigned int pkt_num=vbh->pk_num;
  */

   //  printf("uw_buffer: pkt_num is%d\n",pkt_num);
 Packet* tpkt=DeQueue(source,pkt_num); // avoid duplication
  if(tpkt)Packet::free(tpkt);

  if (IsFull()) Packet::free(Dehead());


  t1->packet=p;
  t1->next=NULL;

  if(head_==NULL) {
     tail_=t1;
     head_=t1;
  }
  else{
  tail_->next=t1;
  tail_=t1;
  }
  
  num_of_packet++;
//printf("CopyNewPacket the pkt_num is %d and %d packets in buffer\n",vbh->pk_num,num_of_packet);
}


Packet* 
RoutingBuffer::head(){
  routing_buffer_cell* t1;
  routing_buffer_cell* t2;
  Packet* p;
  
  if(!head_) return NULL;
  else return head_->packet;
}


Packet* 
RoutingBuffer::Dehead(){
  routing_buffer_cell* t1;
  routing_buffer_cell* t2;
  Packet* p;
  
  if(!head_) return NULL;
   p=head_->packet;
   t2=head_;
   head_=head_->next;
   num_of_packet--;
   
   if(!head_) tail_=NULL; 
    delete t2;
   return p;
}

bool 
RoutingBuffer::IsEmpty(){
  return(0==num_of_packet);
}

bool 
RoutingBuffer::IsFull(){
    //printf("maximum size is %d and num_packet is%d\n",maximum_size,num_of_packet);
  return(maximum_size==num_of_packet);
}


Packet*
RoutingBuffer::DeQueue( ns_addr_t sender,unsigned int num){
    routing_buffer_cell * current_p=head_;
    routing_buffer_cell * previous_p=head_;
    Packet* p=0;
    if(IsEmpty()) return NULL; 
    while (current_p)
    {
        hdr_uwvb* vbh=HDR_UWVB(current_p->packet);
        hdr_uwvbva* vbvah=HDR_UWVBVA(current_p->packet);     


       
   ns_addr_t source;
   if (usr) source=vbh->sender_id;
   else source=vbvah->sender_id;
   unsigned int pkt_num;
   if(usr) pkt_num=vbh->pk_num;
   else pkt_num= vbvah->pk_num;

   /*
  ns_addr_t source=((usr)?: vbh->sender_id,vbvah->sender_id);
  unsigned int pkt_num=((usr)?: vbh->pk_num, vbvah->pk_num);
   */

//printf("ok, DEQUEUE buffer sender id is %d num=%d\n",vbh->sender_id.addr_,vbh->pk_num);
        if((source.addr_==sender.addr_)&&(source.port_==sender.port_)&&(pkt_num==num))
{
   
	    p=current_p->packet;
            
            if(current_p==head_){
		head_=head_->next;
		if(!head_) tail_=0;
		//	delete current_p;
	    }
	    else 
	    {
                if(current_p==tail_)tail_=previous_p; 
		previous_p->next=current_p->next;
		//    delete current_p;
	    }

            delete current_p;
            num_of_packet--;
	    return p;
}
          previous_p=current_p;
	current_p=current_p->next; 
        
    }
    return NULL;
}



Packet*
RoutingBuffer::LookupCopy( ns_addr_t sender,unsigned int num){
    routing_buffer_cell * current_p=head_;
    Packet* p=NULL;
    if(IsEmpty()) {
//printf("buffer: the data link is empty\n");
return NULL;
    }   
    while (current_p)
    {
        hdr_uwvb* vbh=HDR_UWVB(current_p->packet);
        hdr_uwvbva* vbvah=HDR_UWVBVA(current_p->packet);     
    
   ns_addr_t source;
   if (usr) source=vbh->sender_id;
   else source=vbvah->sender_id;
  unsigned int pkt_num;
   if(usr) pkt_num=vbh->pk_num;
   else pkt_num= vbvah->pk_num;

   /*
  ns_addr_t source=((usr)?: vbh->sender_id,vbvah->sender_id);
  unsigned int pkt_num=((usr)?: vbh->pk_num, vbvah->pk_num);
   */

//printf("ok, Lookup buffer sender id is %d num=%d\n",vbh->sender_id.addr_,vbh->pk_num);
        if((source.addr_==sender.addr_)&&(source.port_==sender.port_)&&(pkt_num==num))  {
          p=current_p->packet;
          return p;
	} 
	current_p=current_p->next; 
    }
    return p;
}



