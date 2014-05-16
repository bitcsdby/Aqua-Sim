#ifndef ns_uw_routing_buffer_h
#define ns_uw_routing_buffer_h

#include "packet.h"


typedef struct RoutingBufferCell{
  Packet* packet;
  RoutingBufferCell* next;
  double arrival_time;
} routing_buffer_cell;


class RoutingBuffer{
 public: 
                  RoutingBuffer(int size=10, int myuser=1){
                  head_=NULL;
                  tail_=NULL;
                  num_of_packet=0;
                  maximum_size=size;
                  usr=myuser;
                          };

   
    void AddNewPacket(Packet*);
    void CopyNewPacket(Packet*);// copy the packet and put into queue
    Packet* Dehead();
    Packet* DeQueue(ns_addr_t,unsigned int);
    // Packet* Lookup(ns_addr_t, int);
    Packet* LookupCopy(ns_addr_t,unsigned int);
    Packet* head();
    inline int bufferSize()const {return num_of_packet;}
    bool IsEmpty();
    bool IsFull();
    int usr; // this added later distinguish VBVA and VBF 1 is used for vbf 0 is used for vbva
 private:
       int num_of_packet;
       int maximum_size;
       routing_buffer_cell* head_;
       routing_buffer_cell* tail_;
};
#endif /* __uw_routing_buffer_h__ */
