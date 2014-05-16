#ifndef ns_underwatermac_h
#define ns_underwatermac_h


enum PacketStatus{RECEPTION,COLLISION,INVALID};

struct IncommingPacket{
  Packet* packet;
 enum  PacketStatus status;
  IncommingPacket* next;
};


class IncommingChannel{
 public: 
       IncommingChannel(){
                  head_=NULL; 
                  num_of_active_incomming_packet=0;
                  TotalSignalStrength=0;
                          };
      
    void AddNewPacket(Packet*);
    int  DeleteIncommingPacket(Packet*);
    void InvalidateIncommingPacket();
    IncommingPacket * lookup(Packet*);
    enum PacketStatus Status(Packet*);

    int num_of_active_incomming_packet;// number of incomming packets
    double TotalSignalStrength; // total amount of incomming transmission power
    

 private:
        IncommingPacket* head_;
        void UpdatePacketStatus();
};


class UnderwaterMac;

class RecvHandler: public Handler{
 public:
  RecvHandler(UnderwaterMac*);
  void handle(Event*);
 private:
  UnderwaterMac* mac_;
};

class UnderwaterMac: public Mac {
   
public:
        UnderwaterMac();
        
	double  bit_rate_; // bit rate of the MAC
	double  encoding_efficiency_; //ratio of encoding 


	int  command(int argc, const char*const* argv);
        IncommingChannel recv_channel;
        RecvHandler recv_handler;
        Handler* callback_;  // for the upper layer protocol      


	

        Node* node(void) const {return node_;}
        // to process the incomming packet
        virtual   void RecvProcess(Packet*);

       // to process the outgoing packet
        virtual  void TxProcess(Packet*);

      // to receive packet from upper layer and lower layer 
	 virtual void recv(Packet*,Handler*); 
            

       virtual  void sendUp(Packet*);
       virtual  void sendDown(Packet*);

       //void whoareyou();
        void ResetTransmissionStatus();   
        void IncommingChannelProcess(Event*); // process packet collision 
        void Poweroff();
        void Poweron();
        void InterruptRecv(double); 
// The sending process can stop receiving process and change the transmission
// status of the node since underwatermac is half-deplex

protected:
	Node* node_;// the node this mac is attached


	inline int initialized() {
	return  (node_&&Mac::initialized());
	}
private:

	friend class RecvHandler;
};

#endif /* __uwmac_h__ */

