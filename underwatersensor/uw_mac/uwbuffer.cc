
#include "packet.h"
#include "uwbuffer.h"


void 
TransmissionBuffer::AddNewPacket(Packet* p){
  buffer_cell* t2;
  buffer_cell* t1=new buffer_cell;

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
}


Packet* 
TransmissionBuffer::head(){
  buffer_cell* t1;
  buffer_cell* t2;
  Packet* p;
  
  if(!head_) return NULL;
  else return head_->packet;
}


Packet* 
TransmissionBuffer::dehead(){
  buffer_cell* t1;
  buffer_cell* t2;
  Packet* p;
  
  if(!head_) return NULL;
   p=head_->packet;
   t1=head_->next;
   t2=head_;
 
   head_=t1;
   num_of_packet--;
   
   if(head_==NULL) tail_=NULL; 
    delete t2;
   return p;
}


Packet* 
TransmissionBuffer::next(){
  Packet* p;
  if(!current_p) return NULL;
  p=current_p->packet;
  current_p=current_p->next;
  Packet* p1=p->copy();
   return p1;
}


int 
TransmissionBuffer::DeletePacket(Packet* p){
  buffer_cell* t1;
  buffer_cell* t2;
  
  // insert this packet at the head of the link
  t2=head_;
  

  if (!t2) return 0;//0 no such point, 1:delete this point

  if (p==t2->packet){
    printf("underwatermac: the packet is at the head of list\n");
    head_=t2->next;
    num_of_packet--;

   if(head_==NULL) tail_=NULL;
   
    Packet::free(p);  
     delete t2;
    
    return 1;
}
  
  int modified=0;
  while(t2->next){
    if ((t2->next)->packet!=p) t2=t2->next;
    else{
    
     t1=t2->next;
     t2->next=t1->next;

     if(t1==tail_) tail_=t2;
     num_of_packet--;
    delete t1;
    Packet::free(p);   
    modified=1;
    }
  }
  
  return modified;
}


buffer_cell*  
TransmissionBuffer::lookup(Packet* p){
  buffer_cell* t2;
  t2=head_;  
  while((t2->packet!=p)&&(!t2)) t2=t2->next;
  return t2;
}


void 
TransmissionBuffer::LockBuffer(){
  current_p=head_;
  lock_p=tail_;
  lock=true;
}


void 
TransmissionBuffer::UnlockBuffer(){
  lock=false;
  lock_p=NULL;
}


bool 
TransmissionBuffer::IsEmpty(){
  return(0==num_of_packet);
}

bool 
TransmissionBuffer::ToBeFull(){
  return((MAXIMUM_BUFFER-1)==num_of_packet);
}




bool 
TransmissionBuffer::IsEnd(){
  if (lock_p) return (lock_p->next==current_p);
  return(NULL==current_p);
}



bool 
TransmissionBuffer::IsFull(){
  return(MAXIMUM_BUFFER==num_of_packet);
}

