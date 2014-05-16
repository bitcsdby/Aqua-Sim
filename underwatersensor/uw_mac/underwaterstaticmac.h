#ifndef ns_uwmac_h
#define ns_uwmac_h

enum PacketStatus{RECEPTION,COLLISION,/*SENSED,*/ INVALID};
struct IncommingPacket{
  Packet* pakcet;
  PacketStatus status;
  IncommingPacket* next;
}



class IncommingChannel{
 public: 
       IncommingChannel(){
                  head_=NULL; 
                  num_of_active_incomming_packet=0;
                  TotalSignakStrength=0;
                          };
      
    void AddNewPacket(Packet*);
    int  DeletePacket(Packet*);
    PacketStatus PacketStatus();
    int num_of_active_incomimng_packet;// number of incomming packets
    double TotalSignalStrength;// total amount of incomming transmission power
 private:
        Incommingpacket* head;
        void UpdatePacketStatus();
};


Class RecvHandler: public Handler{
 public:
  RecvHandler(UwStaticMac* p):mac(p){}
  void handle(Event*);
 private:
  UnderwaterPhy* mac;
}

class UWMac: public Mac {

public:
         UWMac();
        PacketQueue tx_buffer;
        InCommingChannel recv_channel;
        RecvHandler recv_handler;
        void RecvProcess();


protected:

private:
	int		command(int argc, const char*const* argv);



	inline int initialized() {
	return (cache_ && logtarget_
                        && Mac::initialized());

	}


	inline void mac_log(Packet *p) {
                logtarget_->recv(p, (Handler*) 0);
        }



protected:


private:

};

#endif /* __uwmac_h__ */

