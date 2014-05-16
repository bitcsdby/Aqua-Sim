

#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"
#include "mobilenode.h"

// #define DEBUG 99

#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mac-timers.h"
#include "uwstaticmac.h"
#include "cmu-trace.h"

// Added by Sushmita to support event tracing
#include "agent.h"
#include "basetrace.h"



void
IncommingChannel::AddNewPacket(Packet* p){
  Packet* t1;
  IncommingPacket* t2;
  t1=new IncommingPacket;
  t1->next=NULL;
  t1->packet=p;
  t1->status=RECEPTION;
  
  // insert this packet at the head of the link
  t2=head;
  head=t1;
  t1->next=t2;
  //
  TotalSignalStrength=TotalSignalStrength+p->txinfo_.RxPr;
  UpdatePacketStatus();
}

int
IncommingChannel::DeletePacket(Packet* p){
  Packet* t1;
  IncommingPacket* t2;
  
  // insert this packet at the head of the link
  t2=head;

  if (!t2) return 0;//0 no such point, 1:delete this point

  if (t2==p){
    head=t2->next;
    num_of_active_incomming_packet--;
    TotalSignalStrength=TotalSignalStrength-p->txinfo_.RxPr;
    delete t2;
    return 1;
}
 
  int modified=0;
  while(t2->next){
    if ((t2->next)->packet!=p) t2=t2->next;
    else{
     IncommingPacket* t1;
  
     t1=t2->next;
     t2->next=t1->next;
     num_of_active_incomming_packet--;
    TotalSignalStrength=TotalSignalStrength-p->txinfo_.RxPr;
    delete t2;
    modify=1;
    }
  }
  
  return modified;
}

int
IncommingChannel::Packetstatus(Packet* p){
  Packet* t1;
  IncommingPacket* t2;
  
  t2=head;


 
  while(t2->packet!=p)&&(!t2) t2=t2->next;

  if (!t2) {
    printf("IncommingChannel:oops! no such packet\n");
return INVALID;
  }
  else return t2->status;
}

void
IncommingChannel::UpdatePacketStatus(){
  InCommingPacket * t1;
  t1=head;
  while(!t1)
    {
      double noise=TotalSignalStrength-(t1->packet)->txinfo_.RxPr;
      if (noise<0) {
         printf("IncommingChannel: what a hell!\n");
	 exit(0);
      }

  if ((t1->packet)->txinfo_.RxPr/noise >= (t1->packet)->txinfo_.CPThresh))
      t1->status=RECEPTION
	else t1->status=COLLISION;
}

void RecvHandler::handle(Event* e)
{
  mac->RecvProcess(Event* e);
}


void
UWMac::RecvProcess(Event* e){
  IncommingPacket* p;
  Packet* target=(Packet*) e;
     
  p=recv_channel.lookup(target);
  if (COLLISION==p.PacketStatus()) {
            recv_channel.DeletePacket(p);
	    Packet::free(p);  
	    return;
  }

  if (INVALID==p.PacketStatus()) {
            recv_channel.DeletePacket(p);
	    Packet::free(p);  
	    return;
  }
 
 if (RECEPTION==p.PacketStatus()) {
            recv_channel.DeletePacket(p);
            RecvHandler(p);
 }
}


/*
 this program is used to handle the received packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
UwMac::RecvHandler(Packet* p){
}


/*
 this program is used to handle the transmitted packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
UWStaticMac::TxHandler(Packet* p){
}



void
UwStaticMac::recv(Packet *p, Handler *h)
{
	struct hdr_cmn *hdr = HDR_CMN(p);
	
        /*
	 * Sanity Check
	 */

	assert(initialized());



	/*
	 *  Handle outgoing packets.
	 */

	if(hdr->direction() == hdr_cmn::DOWN) {
	  TxHandler(p);
                return;
        }

	/*
	 *  Handle incoming packets.
	 *
	 *  We just received the 1st bit of a packet on the network
	 *  interface.
	 *
	 */


	recv_channel.AddNewPacket(p);
        double txtime=har_cmn::access(p)->txtime();
        Scheduler& s=Scheduler::instance();
        s.schedule(recv_handler,p,txtime);     
}




/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class Mac802_11Class : public TclClass {
public:
	Mac802_11Class() : TclClass("Mac/802_11") {}
	TclObject* create(int, const char*const*) {
	// change wrt Mike's code
	// return (new Mac802_11(&PMIB, &MMIB));
	return (new Mac802_11());
	// Mike change ends!

}
} class_mac802_11;


Mac802_11::Mac802_11() : Mac(), mhIF_(this), mhNav_(this), mhRecv_(this), mhSend_(this), mhDefer_(this), mhBackoff_(this),macmib_(this), phymib_(this), mhBeacon_(this)
// Mike change ends
{
	// change wrt Mike
	//macmib_ = m;
	//phymib_ = p;
	// change ends
	
	nav_ = 0.0;
	
	tx_state_ = rx_state_ = MAC_IDLE;
	tx_active_ = 0;

	// change wrt Mike
	eotPacket_ = NULL;
	// change ends


	pktRTS_ = 0;
	pktCTRL_ = 0;
		
	// change wrt Mike's code
	//cw_ = phymib_->CWMin;
	cw_ = phymib_.getCWMin();
	// change ends


	ssrc_ = slrc_ = 0;

	// change wrt Mike's code

	//sifs_ = phymib_->SIFSTime;
	//pifs_ = sifs_ + phymib_->SlotTime;
	//difs_ = sifs_ + 2*phymib_->SlotTime;
	
	// see (802.11-1999, 9.2.10) 
	//eifs_ = sifs_ + (8 * ETHER_ACK_LEN / phymib_->PLCPDataRate) + difs_;
	
	//tx_sifs_ = sifs_ - phymib_->RxTxTurnaroundTime;
	//tx_pifs_ = tx_sifs_ + phymib_->SlotTime;
	//tx_difs_ = tx_sifs_ + 2 * phymib_->SlotTime;

	// Added by Sushmita
        et_ = new EventTrace();
	
	sta_seqno_ = 1;
	cache_ = 0;
	cache_node_count_ = 0;
	
	// chk if basic/data rates are set
	// otherwise use bandwidth_ as default;
	
	Tcl& tcl = Tcl::instance();
	tcl.evalf("Mac/802_11 set basicRate_");
	if (strcmp(tcl.result(), "0") != 0) 
		bind_bw("basicRate_", &basicRate_);
	else
		basicRate_ = bandwidth_;

	tcl.evalf("Mac/802_11 set dataRate_");
	if (strcmp(tcl.result(), "0") != 0) 
		bind_bw("dataRate_", &dataRate_);
	else
		dataRate_ = bandwidth_;

	// change wrt Mike
        EOTtarget_ = 0;
       	bss_id_ = IBSS_ID;
	//-ak-----------
	//printf("bssid in constructor %d\n",bss_id_);


	// change ends
	//	printf("mac-802_11.datarateis %f, abd bandwidth is%f\n",dataRate_,bandwidth_);

}


int
Mac802_11::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		// change wrt Mike
		//if (strcmp(argv[1], "log-target") == 0) {
		 if (strcmp(argv[1], "eot-target") == 0) {
                         EOTtarget_ = (NsObject*) TclObject::lookup(argv[2]);
                         if (EOTtarget_ == 0)
                                 return TCL_ERROR;
                         return TCL_OK;
               } else if (strcmp(argv[1], "bss_id") == 0) {
                       bss_id_ = atoi(argv[2]);

//-ak-----		       
//printf("in command bssid %d \n",bss_id_);

                       return TCL_OK;
                 } else if (strcmp(argv[1], "log-target") == 0) {
		// change ends
 
 		logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "nodes") == 0) {
			if(cache_) return TCL_ERROR;
			cache_node_count_ = atoi(argv[2]);
			cache_ = new Host[cache_node_count_ + 1];
			assert(cache_);
			bzero(cache_, sizeof(Host) * (cache_node_count_+1 ));
			return TCL_OK;
		} else if(strcmp(argv[1], "eventtrace") == 0) {
			// command added to support event tracing by Sushmita
                        et_ = (EventTrace *)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
	}
	return Mac::command(argc, argv);
}

