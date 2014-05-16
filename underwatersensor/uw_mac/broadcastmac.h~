#ifndef ns_broadcastmac_h
#define ns_broadcastmac_h

#include "underwatermac.h"



#define BACKOFF  0.5 // the maximum time period for backoff
#define MAXIMUMCOUNTER 1 // the maximum number of backoff
#define CALLBACK_DELAY 0.0001 // the interval between two consecutive sendings


class BroadcastMac;

class StatusHandler: public Handler{
 public:
  StatusHandler(BroadcastMac*);
  void handle(Event*);
 private:
  BroadcastMac* mac_;
};


class CallbackHandler: public Handler{
 public:
  CallbackHandler(BroadcastMac*);
  void handle(Event*);
 private:
  BroadcastMac* mac_;
};



class BackoffHandler: public Handler{
 public:
  BackoffHandler(BroadcastMac*);
  void handle(Event*);
  void clear();
 private:
  int counter;
  BroadcastMac* mac_;
};

class BroadcastMac: public UnderwaterMac {
   
public:
        BroadcastMac();
     
      	int  command(int argc, const char*const* argv);
        int packetheader_size_; //# of bytes in the header 

        Event backoff_event;
        Event status_event;
        Event callback_event;

        StatusHandler status_handler;
        BackoffHandler backoff_handler; 
        CallbackHandler callback_handler;

        //Node* node(void) const {return node_;}
        // to process the incomming packet
        virtual  void RecvProcess(Packet*);
        void StatusProcess(Event*);
        void CallbackProcess(Event*);
        void DropPacket(Packet*);

       // to process the outgoing packet
        virtual  void TxProcess(Packet*);

protected:        
	inline int initialized() {
	return  UnderwaterMac::initialized();
	}
 private:
        friend class StatusHandler;
        friend class BackoffHandler;
};

#endif /* __broadcastmac_h__ */

