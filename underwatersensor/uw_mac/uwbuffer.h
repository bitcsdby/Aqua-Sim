#ifndef ns_uwbuffer_h
#define ns_uwbuffer_h

#include "config.h"
#include "packet.h"

#define MAXIMUM_BUFFER 1


struct buffer_cell{
  Packet* packet;
  buffer_cell * next;
  double delay;
};


class TransmissionBuffer{
 public: 
       TransmissionBuffer(){
                  head_=NULL;
                  current_p=NULL; 
                  num_of_packet=0;
                  lock=false;
                  tail_=NULL;
                  lock_p=NULL;
                          };
      
    void AddNewPacket(Packet*);
    void LockBuffer();
    void UnlockBuffer();
    int  DeletePacket(Packet*);
    Packet* dehead();
    Packet* next();
    Packet* head();
    bool  IsEnd();
    bool IsEmpty();
    bool IsFull();
    bool ToBeFull();
    bool IsLocked(){return lock;};
    buffer_cell * lookup(Packet*);
    int num_of_packet;// number of sending packets
       buffer_cell* head_;
       bool lock;
 private:
       buffer_cell* current_p;
       buffer_cell* lock_p;
       buffer_cell* tail_;
};
#endif /* __uwbuffer_h__ */
