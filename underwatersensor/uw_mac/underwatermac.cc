
//#include "delay.h"
//#include "connector.h"
#include "packet.h"
//#include "random.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "rmac.h"
#include "mac.h"
#include "underwatermac.h"
#include "underwaterphy.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"

void 
IncommingChannel::AddNewPacket(Packet* p){
  IncommingPacket* t1;
  IncommingPacket* t2;
  t1=new IncommingPacket;
  t1->next=NULL;
  t1->packet=p;
  t1->status=RECEPTION;
  
  // insert this packet at the head of the link
  t2=head_;
  head_=t1;
  t1->next=t2;
  //
  
  num_of_active_incomming_packet++;
  //  printf("IncommingChannel: number of packet is %d\n",num_of_active_incomming_packet);
  TotalSignalStrength=TotalSignalStrength+p->txinfo_.RxPr;
  UpdatePacketStatus();
}


int
IncommingChannel::DeleteIncommingPacket(Packet* p){
  Packet* t1;
  IncommingPacket* t2;
  
  // insert this packet at the head of the link
  t2=head_;

  if (!t2) return 0;//0 no such point, 1:delete this point

  if (p==t2->packet){
    //    printf("underwatermac: the packet is at the head of list\n");
    head_=t2->next;
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
    delete t1;
    modified=1;
    }
  }
  
  return modified;
}

IncommingPacket* 
IncommingChannel::lookup(Packet* p){
  IncommingPacket* t2;
  t2=head_;  
  while((t2->packet!=p)&&(!t2)) t2=t2->next;
  return t2;
}


void 
IncommingChannel::InvalidateIncommingPacket(){
  IncommingPacket* t2;
  //printf("underwatermac IncommingChannel: node %d invalidate buffer %f\n",index_,NOW);  
  t2=head_;  
  while(t2) {
   t2->status=INVALID;
 t2=t2->next;
  }
}



enum PacketStatus 
IncommingChannel::Status(Packet* p){
  Packet* t1;
  IncommingPacket* t2;
  
  t2=head_;
 
  while ((t2->packet!=p)&&(t2)) t2=t2->next;

  if (!t2) {
    printf("IncommingChannel:oops! no such packet\n");
    return INVALID;
  }
  else return t2->status;
}


void
IncommingChannel::UpdatePacketStatus(){
  IncommingPacket * t1;
  t1=head_;

  while(t1)
    {
	// printf("!!!IncommingChannel: Total is %f and packe Rxpr is %f\n",TotalSignalStrength,(t1->packet)->txinfo_.RxPr);
      double noise=TotalSignalStrength-(t1->packet)->txinfo_.RxPr;
      double t2=(t1->packet)->txinfo_.RxPr;     
      double alpha=0.00000001;

      if (TotalSignalStrength<t2) {
	  //   printf("IncommingChannel what a hell %f\n",noise);
	 //  printf("RECEPTION\n");
	     t1->status=RECEPTION;
	     return;
      }

    if (fabs(TotalSignalStrength-t2)<alpha){
	//      printf("IncommingChannel noise is zero %f \n",noise);      //       printf("RECEPTION\n");
	        t1->status=RECEPTION;
		// return;
    }
    else {
      // printf("IncommingChannel: current packet RX is %f and noise is %f and CPThresh is %f\n",(t1->packet)->txinfo_.RxPr,noise,(t1->packet)->txinfo_.CPThresh);
      if (((t1->packet)->txinfo_.RxPr)/noise >= (t1->packet)->txinfo_.CPThresh)
	{/*printf("RECEPTION\n");*/	t1->status=RECEPTION;}
      else {//printf("COLLISION noise=%f \n",noise); 
               t1->status=COLLISION;}
    }
      t1=t1->next;
    }
}

RecvHandler::RecvHandler(UnderwaterMac* p):mac_(p){}

void RecvHandler::handle(Event* e)
{
  // printf("Recv_handler\n");
  mac_->IncommingChannelProcess(e);
}




/* ======================================================================
   Base class for underwater sensor MAC
   ====================================================================== */
static class UnderwaterMacClass : public TclClass {
public:
	UnderwaterMacClass() : TclClass("Mac/UnderwaterMac") {}
	TclObject* create(int, const char*const*) {
	  return (new UnderwaterMac());

	}
} class_underwatermac;


UnderwaterMac::UnderwaterMac() : Mac(),recv_handler(this)
{
  bit_rate_=100;
  encoding_efficiency_=1;
  bind("bit_rate_",&bit_rate_);
  bind("encoding_efficiency_",&encoding_efficiency_);
  callback_=NULL;
}

/*
 this program is used to handle the received packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
UnderwaterMac::RecvProcess(Packet* p){
 printf("uwmac: there is no implementation of RecvHandler!\n");
}




void 
UnderwaterMac::sendUp(Packet* p){
	assert(initialized());
        uptarget_->recv(p,this);
}


void 
UnderwaterMac::sendDown(Packet* p){
	assert(initialized());
	//  printf("uwmac: node %d sendDown\n",index_);
        downtarget_->recv(p,this);
}




/*
 this program is used to handle the transmitted packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
UnderwaterMac::TxProcess(Packet* p){
  printf("uwmac: there is no implementation of TxHandle\n");
}



void
UnderwaterMac::recv(Packet *p, Handler *h)
{
	struct hdr_cmn *hdr = HDR_CMN(p);
	int direction=hdr->direction();      
    
        struct hdr_uwvb* hdr2=HDR_UWVB(p);
        int sender=hdr2->sender_id.addr_;
        /*
	 * Sanity Check
	 */

	// printf("underwatermac I (%d) receive a packet  direction is %d down=%d at %f\n",node_->nodeid(),direction,hdr_cmn::DOWN,NOW);

	assert(initialized());



	/*
	 *  Handle outgoing packets.
	 */

       	

	if(direction == hdr_cmn::DOWN) {
          callback_=h;
	  TxProcess(p);
	  //  printf("underwatermac I am done with sending down\n");
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
        double txtime=hdr_cmn::access(p)->txtime();
        UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
        n->SetTransmissionStatus(RECV);
	n->SetCarrierSense(true);
  
//	printf("!!!underwatermac recv(%d) upcoming packet : tx=%f set time at %f\n",node_->nodeid(),txtime,NOW); 
     
        Scheduler& s=Scheduler::instance();
        s.schedule(&recv_handler,p,txtime);     
        return;
}


int
UnderwaterMac::command(int argc, const char*const* argv)
{


     if(argc == 3) {
		TclObject *obj;
                 if (strcmp(argv[1], "node_on") == 0) {
		   Node* n1=(Node*) TclObject::lookup(argv[2]);
		   if (!n1) return TCL_ERROR;
		   node_ =n1; 
		   return TCL_OK;
		 }
     }

	return Mac::command(argc, argv);
}


void
UnderwaterMac::IncommingChannelProcess(Event* e){

    //  printf("underwatermac recv(%d) :process incommingchannel at %f\n",node_->nodeid(),NOW); 
 
  //IncommingPacket* p;
  Packet*  target;
           target=(Packet*) e;
	   enum PacketStatus status=recv_channel.Status(target);

 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
       


   struct hdr_cmn *hdr = HDR_CMN(target);

	   //  p=recv_channel.lookup(target);
   if (COLLISION==status) {
      printf("underwater: the packet is interfered at node %d\n",node_->nodeid()); 
            recv_channel.DeleteIncommingPacket(target);
            ResetTransmissionStatus();
            hdr->error()=1; //set error flag
            RecvProcess(target);
	    // Packet::free(target);  
	    // return;
  }

  if (INVALID==status) {
    printf("underwater:the packet is invalidated at node %d\n",node_->nodeid()); 
            recv_channel.DeleteIncommingPacket(target);
            ResetTransmissionStatus();
             hdr->error()=1; //set error flag
             RecvProcess(target);
	    // Packet::free(target);  
	     // return;
  }
 
 if (RECEPTION==status) {
     // printf("underwater:the packet is correctly received at node %d\n",node_->nodeid()); 
            recv_channel.DeleteIncommingPacket(target);
            ResetTransmissionStatus();
            RecvProcess(target);
	    // return;
 }

 if(recv_channel.num_of_active_incomming_packet==0) n->ResetCarrierSense();
 return;
}



void 
UnderwaterMac::Poweron()
{
   assert(initialized());
   UnderwaterPhy* phy;
   //phy=(UnderwaterPhy*) downtarget_;
   phy = (UnderwaterPhy*) netif_;

        UnderwaterSensorNode* n;

	n=(UnderwaterSensorNode*) node_ ;         
        n->SetTransmissionStatus(IDLE);
        phy->power_on();       
}

void 
UnderwaterMac::Poweroff(){
 	assert(initialized());
       UnderwaterPhy* phy;
        //phy=(UnderwaterPhy*) downtarget_;
        phy = (UnderwaterPhy*) netif_;

        UnderwaterSensorNode* n;

        recv_channel.InvalidateIncommingPacket();
	n=(UnderwaterSensorNode*) node_ ;         
        n->SetTransmissionStatus(SLEEP);
        phy->power_off();       
        
}

void 
UnderwaterMac::ResetTransmissionStatus()
{
  UnderwaterSensorNode* n;
	assert(initialized());
	n=(UnderwaterSensorNode*) node_ ;
        if (0!=recv_channel.num_of_active_incomming_packet)
           {
	     //printf("UnderwaterMac: there exist %d packets\n",recv_channel.num_of_active_incomming_packet); 
return;}
      
	if (RECV==n->TransmissionStatus())
	  {
	    n->SetTransmissionStatus(IDLE);
	  }
	return;
}


void 
UnderwaterMac::InterruptRecv(double txtime){
       assert(initialized());
       UnderwaterPhy* phy;
       //phy=(UnderwaterPhy*) downtarget_;
       phy = (UnderwaterPhy*)netif_;
       UnderwaterSensorNode* n;
	n=(UnderwaterSensorNode*) node_ ;  
        n->SetTransmissionStatus(SEND);

	if (RECV==n->TransmissionStatus()){ 
        recv_channel.InvalidateIncommingPacket();   
        phy->status_shift(txtime);       
	}
}
