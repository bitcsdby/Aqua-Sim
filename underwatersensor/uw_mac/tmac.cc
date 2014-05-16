#include "packet.h"
#include "random.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"
#include "mac.h"
#include "tmac.h"
#include "underwaterphy.h"
#include "random.h"
#include "underwatermac.h"

int hdr_tmac::offset_;

static class TMACHeaderClass: public PacketHeaderClass{
 public:
  TMACHeaderClass():PacketHeaderClass("PacketHeader/TMAC",sizeof(hdr_tmac))
{
 bind_offset(&hdr_tmac::offset_);
}
} class_tmachdr;


//TBackoffHandler::TBackoffHandler(TMac* p):mac_(p),window_(0),counter_(0){}
 
void TBackoffHandler::handle(Event*e)
{
  counter_++;
  if(counter_<MAXIMUMBACKOFF)
    mac_->TxND((Packet*)e, window_);
  else 
    {
    clear();
  printf("Rmac:backoff:too many backoffs\n");
  Packet::free((Packet*)e);
    }
}
 // void SetStartTime(double);
void TBackoffHandler::clear(){
counter_=0;
}



//TStatusHandler::TStatusHandler(TMac* p):mac_(p),status_(SLEEP){}
void TStatusHandler::handle(Event* e)
{
  if(status_!=SLEEP) mac_->StatusProcess(e,status_);
  else mac_->Poweroff();
}


void TStatusHandler::SetStatus(TransmissionStatus  status)
{
  status_=status;
}



//TSYNHandler::TSYNHandler(TMac* p): mac_(p){}

void 
TSYNHandler::handle(Event* e)
{
  printf("SYNHandler: node%d handle syn\n",mac_->index_);
  mac_->TxND((Packet*) e, mac_->PhaseTwo_window_);
}



//TNDHandler::TNDHandler(TMac* p):mac_(p){}

void TNDHandler::handle(Event* e)
{  
  
    mac_->cycle_start_time=NOW;
    mac_->SendND(mac_->short_packet_size_);
}


//TACKNDWindowHandler::TACKNDWindowHandler(TMac* p):mac_(p){}

void TACKNDWindowHandler::handle(Event* e)
{ 
   mac_->SendShortAckND();
}


//TACKNDHandler::TACKNDHandler(TMac* p):mac_(p){}

void TACKNDHandler::handle(Event* e)
{ 
    mac_->TxND((Packet*) e, mac_->ACKND_window_);
}



//TPhaseOneHandler::TPhaseOneHandler(TMac* p):mac_(p){}

void TPhaseOneHandler::handle(Event* e)
{ 
    mac_->InitPhaseOne(mac_->ND_window_,mac_->ACKND_window_, mac_->PhaseOne_window_);
}



//TPhaseTwoHandler::TPhaseTwoHandler(TMac* p):mac_(p){}

void TPhaseTwoHandler::handle(Event* e)
{ 
    mac_->SendSYN();
}

//TPhaseThreeHandler::TPhaseThreeHandler(TMac* p):mac_(p){}

void TPhaseThreeHandler::handle(Event* e)
{ 
    mac_->InitPhaseThree();
}


//SilenceHandler::SilenceHandler(TMac* p): mac_(p){}

void 
SilenceHandler::handle(Event* e)
{
  printf("SilenceHandler: node%d handle silence\n",mac_->index_);
  mac_->ProcessSilence();
}


//ACKHandler::ACKHandler(TMac* p): mac_(p){}

void 
ACKHandler::handle(Event* e)
{
  mac_->SendACKPacket();
}



//PoweroffHandler::PoweroffHandler(TMac* p): mac_(p){}

void 
PoweroffHandler::handle(Event* e)
{
 mac_->ResetMacStatus();  
}


//TTimeoutHandler::TTimeoutHandler(TMac* p): mac_(p){}

void 
TTimeoutHandler::handle(Event* e)
{
  mac_->SetIdle();
}

//RTSTimeoutHandler::RTSTimeoutHandler(TMac* p): mac_(p){num=0;}

void 
RTSTimeoutHandler::handle(Event* e)
{
  printf("rts_timeout_handler: node %d timeout %d times\n",mac_->index_,num);
  num++;
  if(num<2)
  mac_->SendRTS();
  else
    {
      num=0;
      mac_->ProcessSleep();
    }
}



//TxHandler::TxHandler(TMac* p):mac_(p), receiver(0){}
 
void TxHandler::handle(Event*e)
{
  
  mac_->TxData(receiver);
}


//TWakeupHandler::TWakeupHandler(TMac* p): mac_(p){}

void 
TWakeupHandler::handle(Event* e)
{
  mac_->Wakeup();
}


//RTSHandler::RTSHandler(TMac* p):mac_(p){receiver_addr=0;}
 
void RTSHandler::handle(Event*e)
{
   printf("rts_handler: node %d rts handelr\n",mac_->index_);
  mac_->TxRTS(e,receiver_addr);
}


/*
RTSSilenceHandler::RTSSilenceHandler(TMac* p):mac_(p){start_time=0;duration=0;}
 
void RTSSilenceHandler::handle(Event*e)
{
  mac_->ProcessRTSSilence();
}
*/




//CTSHandler::CTSHandler(TMac* p):mac_(p),num(0){}
 
void CTSHandler::handle(Event*e)
{
  num++;
  mac_->TxCTS((Packet *)e);
}



/* ======================================================================
    TMAC for  underwater sensor
    Implementation of TMAC in underwater scenarios 

   ====================================================================== */
static class TMacClass : public TclClass {
public:
 TMacClass():TclClass("Mac/UnderwaterMac/TMac") {}
   TclObject* create(int, const char*const*) {
	  return (new TMac());
   }
} class_tmac;


TMac::TMac():UnderwaterMac(),short_nd_handler(this),short_acknd_window_handler(this), status_handler(this), phaseone_handler(this),acknd_handler(this), phasetwo_handler(this), phasethree_handler(this), wakeup_handler(this),timeout_handler(this),transmission_handler(this),ackdata_handler(this), rts_timeout_handler(this),backoff_handler(this),silence_handler(this),ack_handler(this),poweroff_handler(this),rts_handler(this),cts_handler(this),syn_handler(this)
{
  num_send=0;
  num_data=0;
  large_packet_size_=30;
  short_packet_size_=10;
 
  short_latency_table_index=0;
  
  InitializeSilenceTable();
 

  period_table_index=0;

  next_period=0;  
 
  last_silenceTime=0;  
  last_rts_silenceTime=0;
  
  
 for(int i=0;i<TABLE_SIZE;i++){


    short_latency_table[i].node_addr=-1;
    short_latency_table[i].num=0;
    short_latency_table[i].last_update_time=0.0;

    period_table[i].node_addr=-1;
    period_table[i].difference=0.0;
    period_table[i].last_update_time=0.0; 
  }

    arrival_table_index=0; 
  for(int i=0;i<TABLE_SIZE;i++)
    arrival_table[i].node_addr=-1;
 
  bind("PhaseOne_window_",&PhaseOne_window_);
  bind("PhaseTwo_window_",&PhaseTwo_window_);
  bind("duration_",&duration_);
  bind("ND_window_",&ND_window_); 
  bind("PhaseTwo_interval_",&PhaseTwo_interval_);
  bind("ACKND_window_",&ACKND_window_); 
  bind("PhyOverhead_",&PhyOverhead_);
  bind("large_packet_size_",&large_packet_size_);
  bind("short_packet_size_",&short_packet_size_);
  bind("PhaseOne_cycle_",&PhaseOne_cycle_);
   bind("PhaseTwo_cycle_",&PhaseTwo_cycle_);
  bind("IntervalPhase2Phase3_",&IntervalPhase2Phase3_);
  bind("PeriodInterval_",&PeriodInterval_);
  bind("transmission_time_error_",&transmission_time_error_);
  bind("SIF_",&SIF_);
  
  bind("ContentionWindow_",&ContentionWindow_);
  bind ("TransmissionRange_",&TransmissionRange_);
 

   max_short_packet_transmissiontime=((1.0*short_packet_size_)/bit_rate_)*(1+transmission_time_error_);
   max_large_packet_transmissiontime=((1.0*large_packet_size_)/bit_rate_)*(1+transmission_time_error_);
      
   MinBackoffWindow=max_large_packet_transmissiontime;
  


   max_propagationTime=TransmissionRange_/1500+max_short_packet_transmissiontime;
   TAduration_=(ContentionWindow_+max_short_packet_transmissiontime
	      +max_propagationTime*2)*1.5;
  InitPhaseOne(ND_window_,ACKND_window_, PhaseOne_window_);
  
}

void 
TMac::InitPhaseOne(double t1,double t2, double t3)
{
  
   printf("TMac: Phaseone cycle: %d...\n",PhaseOne_cycle_);

   if(PhaseOne_cycle_)
    { 
    PhaseStatus=PHASEONE;
    Scheduler& s=Scheduler::instance();
    InitND(t1,t2,t3);
    s.schedule(&phaseone_handler,&phaseone_event,t3);
    PhaseOne_cycle_--;
    return;
    }
  
 
   InitPhaseTwo();
   return;
}

/*
void 
TMac::InitPhaseTwo(){

   double delay=Random::uniform()*PhaseTwo_window_;
   PhaseStatus=PHASETWO;

    cycle_start_time=NOW;
    //  next_period=IntervalPhase2Phase3_+PhaseTwo_window_+delay;
    next_period=IntervalPhase2Phase3_+PhaseTwo_window_;
 
   
    Scheduler& s=Scheduler::instance();
    s.schedule(&phasetwo_handler, &phasetwo_event,delay);
    s.schedule(&phasethree_handler,&phasethree_event,next_period);
    printf("tmac initphasetwo: the phasethree of node %d is scheduled at %f\n",index_,NOW+next_period);
 
    return;
}
*/


void 
TMac::InitPhaseTwo(){

  //   double delay=Random::uniform()*PhaseTwo_window_;
  //  next_period=IntervalPhase2Phase3_+PhaseTwo_cycle_*PhaseTwo_window_+delay;
  next_period=IntervalPhase2Phase3_+PhaseTwo_cycle_*PhaseTwo_window_;
   Scheduler& s=Scheduler::instance();
   s.schedule(&phasethree_handler, &phasethree_event,next_period);
  
   StartPhaseTwo();
    return;  
}

void 
TMac::StartPhaseTwo()
{
  printf("Phase Two: node %d  and cycle:%d\n",index_, PhaseTwo_cycle_);
   if(PhaseTwo_cycle_)
    {     
     PhaseStatus=PHASETWO;
     cycle_start_time=NOW;
    double  delay=Random::uniform()*PhaseTwo_window_; 
    Packet* pkt=GenerateSYN();
    Scheduler& s=Scheduler::instance();
    s.schedule(&syn_handler,(Event*) pkt, delay);    
    s.schedule(&phasetwo_handler,&phasetwo_event,PhaseTwo_window_+PhaseTwo_interval_);
    next_period-=PhaseTwo_window_-PhaseTwo_interval_;
    PhaseTwo_cycle_--;
    }
   return;   
}



Packet*  
TMac::GenerateSYN(){

       Packet* pkt =Packet::alloc();
       hdr_tmac* synh = HDR_TMAC(pkt); 
       hdr_cmn*  cmh = HDR_CMN(pkt);
      
       cmh->size()=short_packet_size_;
       cmh->next_hop()=MAC_BROADCAST;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_RMAC;
      
      
       synh->ptype=P_SYN;
       synh->pk_num = num_send;
       synh->sender_addr= node_->address();
     
       synh->duration=duration_;
        num_send++;

	printf("rmac GenerateSYN:node(%d) generates SYN packet at %f\n", synh->sender_addr,NOW);
	return pkt; 
}



void 
TMac::SendSYN(){

       Packet* pkt =Packet::alloc();
       hdr_tmac* synh = HDR_TMAC(pkt); 
       hdr_cmn*  cmh = HDR_CMN(pkt);
      
       cmh->size()=short_packet_size_;
       cmh->next_hop()=MAC_BROADCAST;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_TMAC;
      
      
       synh->ptype=P_SYN;
       synh->pk_num = num_send;
       synh->sender_addr= node_->address();
     
       synh->duration=duration_;
        num_send++;

	printf("tmac SendSYN:node(%d) send SYN packet at %f\n", synh->sender_addr,NOW);
      TxND(pkt, PhaseTwo_window_);  
}


void 
TMac::InitND(double t1,double t2, double t3)
{  
 
  double delay=Random::uniform()*t1;
  double itval=(t3-t2-t1)/2.0;
  double delay3=t1+itval;
 
  Scheduler& s=Scheduler::instance();

   s.schedule(&short_nd_handler, &short_nd_event, delay);
   s.schedule(&short_acknd_window_handler,&short_acknd_event,delay3);
  return;
}

void 
TMac::SendND(int pkt_size)
{

      Packet* pkt =Packet:: alloc();
      hdr_tmac* ndh = HDR_TMAC(pkt);
    
      hdr_cmn*  cmh = HDR_CMN(pkt);
      
   
     // additional 2*8 denotes the size of type,next-hop of the packet and 
     // timestamp
  
      //       cmh->size()=sizeof(hdr_nd)+3*8;
      //  printf("old size is %d\n",cmh->size());
        cmh->size()=pkt_size;

       cmh->next_hop()=MAC_BROADCAST;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_TMAC;
      
       
     
      ndh->ptype=P_ND;
      ndh->pk_num = num_send;
      ndh->sender_addr= node_->address();

      num_send++;

      // iph->src_.addr_=node_->address();
      // iph->dst_.addr_=node_->address();
      //iph->dst_.port_=255;     

 printf("tmac SendND:node(%d) send ND type is %d at %f\n", ndh->sender_addr,cmh->ptype_, NOW);
      TxND(pkt, ND_window_);  
}





void 
TMac::SendShortAckND()
{
    printf("tmac:SendShortND: node %d\n",index_);
  if (arrival_table_index==0) return;// not ND received


  while(arrival_table_index>0){ 
      Packet* pkt = Packet::alloc();
      hdr_tmac* ackndh = HDR_TMAC(pkt);
    
      hdr_cmn*  cmh = HDR_CMN(pkt);
      
      ackndh->ptype=P_SACKND;
      ackndh->pk_num = num_send;
      ackndh->sender_addr=node_->address();
      num_send++;

      cmh->ptype_=PT_TMAC;
        
         int index1=-1;
        index1=rand()%arrival_table_index; 
        double t2=-0.1;
        double t1=-0.1;
    
        int receiver=arrival_table[index1].node_addr;
         t2=arrival_table[index1].arrival_time; 
         t1=arrival_table[index1].sending_time; 

	 for(int i=index1;i<arrival_table_index;i++){
	   arrival_table[i].node_addr=arrival_table[i+1].node_addr;
           arrival_table[i].sending_time=arrival_table[i+1].sending_time;
           arrival_table[i].arrival_time=arrival_table[i+1].arrival_time;
	 }
   
          ackndh->arrival_time=t2;
          ackndh->ts=t1;
  // additional 2*8 denotes the size of type,next-hop of the packet and 
  // timestamp
    //  cmh->size()=sizeof(hdr_ack_nd)+3*8;
    

      cmh->size()=short_packet_size_;
      cmh->next_hop()=receiver;
      cmh->direction()=hdr_cmn::DOWN; 
      cmh->addr_type()=NS_AF_ILINK;

      
         Scheduler& s=Scheduler::instance();
         double delay=Random::uniform()*ACKND_window_;
         s.schedule(&acknd_handler, (Event*) pkt, delay);

	 arrival_table_index--;
  }


     arrival_table_index=0; 
  for(int i=0;i<TABLE_SIZE;i++)
    arrival_table[i].node_addr=-1;

          return; 
}


void 
TMac::ProcessShortACKNDPacket(Packet* pkt)
{
  // printf("tmac:ProcessshortACKNDPacket: node %d\n",index_);
    hdr_tmac* ackndh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=ackndh->sender_addr;
    double t4=NOW;
    double t3=cmh->ts_;
    int myaddr=node_->address();
 
    double t2=ackndh->arrival_time;
    double t1=ackndh->ts;

double latency=((t4-t1)-(t3-t2))/2.0;
bool newone=true;   

Packet::free(pkt);
   
 for (int i=0;i<TABLE_SIZE;i++)
 if (short_latency_table[i].node_addr==sender)
      {
       short_latency_table[i].sumLatency+=latency;
       short_latency_table[i].num++;
       short_latency_table[i].last_update_time=NOW;
       short_latency_table[i].latency = 
                  short_latency_table[i].sumLatency/short_latency_table[i].num;
       newone=false;
      }
 
 if(newone)
{

    if(short_latency_table_index>=TABLE_SIZE){ 
      printf("tmac:ProcessNDPacket:arrival_table is full\n");
      return;
    }

    short_latency_table[short_latency_table_index].node_addr=sender;
    short_latency_table[short_latency_table_index].sumLatency+=latency;
    short_latency_table[short_latency_table_index].num++;
    short_latency_table[short_latency_table_index].last_update_time=NOW;
    short_latency_table[short_latency_table_index].latency = 
          short_latency_table[short_latency_table_index].sumLatency/short_latency_table[short_latency_table_index].num;
    short_latency_table_index++;
}
 for(int i=0;i<short_latency_table_index;i++)
   printf("node (%d) to node (%d) short latency is %f and number is %d\n", myaddr, short_latency_table[i].node_addr, short_latency_table[i].latency,short_latency_table[i].num); 
 
 return;

}




void 
TMac::ProcessSYN(Packet* pkt)
{
  // printf("rmac:ProcessSYN: node %d\n",index_);
    hdr_tmac* synh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=synh->sender_addr;
    double interval=synh->interval;
    double tduration=synh->duration;
      Packet::free(pkt);


    double t1=-1.0;
 for (int i=0;i<TABLE_SIZE;i++)
 if (short_latency_table[i].node_addr==sender)
     t1=short_latency_table[i].latency;

 if(t1==-1.0) {
   printf("Rmac:ProcessSYN: I receive a SYN from unknown neighbor\n");
   return; 
 }

 interval-=t1;
 double t2=next_period-(NOW-cycle_start_time);
 double d=interval-t2;

if (d>=0.0) {
   while (d>=PeriodInterval_) d-=PeriodInterval_;
 }
 else 
   {
     while (d+PeriodInterval_<=0.0) d+=PeriodInterval_;
   }



 bool newone=true;     
 
 if(d<0) d=d+PeriodInterval_;   

 for (int i=0;i<TABLE_SIZE;i++)
 if (period_table[i].node_addr==sender)
      {
       period_table[i].difference=d;
       period_table[i].last_update_time=NOW;
       period_table[i].duration =tduration; 
       newone=false;
      }
 
 if(newone)
{

    if(period_table_index>=TABLE_SIZE){ 
      printf("rmac:ProcessSYN:period_table is full\n");
      return;
    }


    period_table[period_table_index].node_addr=sender;
    period_table[period_table_index].difference=d;
    period_table[period_table_index].last_update_time=NOW;
    period_table[period_table_index].duration=tduration;
    period_table_index++;
}

 for(int i=0;i<period_table_index;i++)
   printf("node (%d) to node (%d) period difference  is %f \n",index_,period_table[i].node_addr, period_table[i].difference); 
 
 return;

}



/*
void 
TMac::ProcessSYN(Packet* pkt)
{
  // printf("tmac:ProcessSYN: node %d\n",index_);
    hdr_tmac* synh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=synh->sender_addr;
    double interval=synh->interval;
    double tduration=synh->duration;
      Packet::free(pkt);


    double t1=-1.0;
 for (int i=0;i<TABLE_SIZE;i++)
 if (short_latency_table[i].node_addr==sender)
     t1=short_latency_table[i].latency;

 if(t1==-1.0) {
   printf("TMac:ProcessSYN: I receive a SYN from unknown neighbor\n");
   return; 
 }

 interval-=t1;
 double t2=next_period-(NOW-cycle_start_time);
 double d=interval-t2;

if (d>=0.0) {
   while (d>=PeriodInterval_) d-=PeriodInterval_;
 }
 else 
   {
     while (d+PeriodInterval_<=0.0) d+=PeriodInterval_;
   }



 bool newone=true;     
   
 for (int i=0;i<TABLE_SIZE;i++)
 if (period_table[i].node_addr==sender)
      {
       period_table[i].difference=d;
       period_table[i].last_update_time=NOW;
       period_table[i].duration =tduration; 
       newone=false;
      }
 
 if(newone)
{

    if(period_table_index>=TABLE_SIZE){ 
      printf("tmac:ProcessSYN:period_table is full\n");
      return;
    }

    period_table[period_table_index].node_addr=sender;
    period_table[period_table_index].difference=d;
    period_table[period_table_index].last_update_time=NOW;
    period_table[period_table_index].duration=tduration;
    period_table_index++;
}

 for(int i=0;i<period_table_index;i++)
   printf("node (%d) to node (%d) period difference  is %f \n",index_,period_table[i].node_addr, period_table[i].difference); 
 
 return;

}

*/


void
TMac::ProcessSleep()
{
  printf("TMac: ProcessSleep node %d at time %f ...\n",index_,NOW); 
  mac_status=TMAC_IDLE;
  Poweroff();
}



/*
void 
TMac::TxND(Packet* pkt, double window)
{
  //  printf("TMac TxND node %d\n",index_); 
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_tmac* synh = HDR_TMAC(pkt); 
  
  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND);
  cmh->ts_=NOW;


  sendDown(pkt);
  backoff_handler.clear();

  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==n->TransmissionStatus()){
  
  n->SetTransmissionStatus(SEND);
 
  //printf("TxND the data type is %d\n",MAC_BROADCAST);
  //printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
     cmh->ts_=NOW;


    
  sendDown(pkt);
  backoff_handler.clear();
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

   status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      Scheduler& s=Scheduler::instance();
      double d1=window-(NOW-cycle_start_time);
 
      if(d1>0){
      double backoff=Random::uniform()*d1;
      backoff_handler.window_=window;
   // printf("broadcast Tx set timer at %f backoff is %f\n",NOW,backoff);
      s.schedule(&backoff_handler,(Event*) pkt,backoff);
      return;
      }
      else {
          backoff_handler.clear();
          printf("TMac:backoff:no time left \n");
          Packet::free(pkt);
      }

    }

if (SEND==n->TransmissionStatus())
{
  // this case is supposed not to  happen 
    printf("tmac: queue send data too fas\n");
    Packet::free(pkt);
      return;
}

}

*/


void 
TMac::TxND(Packet* pkt, double window)
{
   printf("TMac TxND node %d\n",index_); 
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_tmac* synh = HDR_TMAC(pkt); 
  
  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND);
  cmh->ts_=NOW;

  if(PhaseStatus==PHASETWO){

    double t=NOW-cycle_start_time;

    synh->interval=next_period-t;
  }

  sendDown(pkt);
  backoff_handler.clear();

  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==n->TransmissionStatus()){
  
  n->SetTransmissionStatus(SEND);
 
  //printf("TxND the data type is %d\n",MAC_BROADCAST);
  //printf("broadcast : I am going to send the packet down tx is %f\n",txtime);
     cmh->ts_=NOW;

  if(PhaseStatus==PHASETWO){

   double t=NOW-cycle_start_time;
   synh->interval=next_period-t; 

  }

  sendDown(pkt);
  backoff_handler.clear();
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

   status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      Scheduler& s=Scheduler::instance();
      double d1=window-(NOW-cycle_start_time);
 
      if(d1>0){
      double backoff=Random::uniform()*d1;
      backoff_handler.window_=window;
   // printf("broadcast Tx set timer at %f backoff is %f\n",NOW,backoff);
      s.schedule(&backoff_handler,(Event*) pkt,backoff);
      return;
      }
      else {
          backoff_handler.clear();
          printf("Rmac:backoff:no time left \n");
          Packet::free(pkt);
      }

    }

if (SEND==n->TransmissionStatus())
{
  // this case is supposed not to  happen 
    printf("rmac: queue send data too fas\n");
    Packet::free(pkt);
      return;
}

}





void 
TMac::InitPhaseThree()
{
  printf("TMac: this is InitPhaseThree\n"); 

   PrintTable();

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  mac_status=TMAC_SLEEP;
  /* if(n->TransmissionStatus()==SLEEP)*/
    Wakeup();  
    return;
}


void 
TMac::ResetMacStatus()
{
printf("Tmac:ResetMacStatus  node %d at %f...\n",index_,NOW);
 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 // the node is receiving some data
  if(n->TransmissionStatus()==RECV) 
   {
   mac_status=TMAC_IDLE;
   Scheduler &s=Scheduler::instance();
  s.schedule(&poweroff_handler,&poweroff_event,TAduration_);
   }
  else 
  {
    mac_status=TMAC_SLEEP;
    Poweroff();
  }
}


void 
TMac::Wakeup(){
printf("\n. ..WakeUp node %d periodic wake up at %f...\n\n",index_,NOW);

  Scheduler &s=Scheduler::instance();
  //   s.cancel(&poweroff_event);
  //  s.cancel(&wakeup_event); //? necessary?

  // s.schedule(&poweroff_handler,&poweroff_event,TAduration_);

  s.schedule(&wakeup_handler,&wakeup_event,PeriodInterval_);

 if (mac_status==TMAC_SLEEP) 
{
       Poweron();

       mac_status=TMAC_IDLE;
       s.schedule(&poweroff_handler,&poweroff_event,TAduration_);
   
        cycle_start_time=NOW;
          
        if (NewData())
                       { 
       printf("WakeUp: There is new data in node %d and the number of packet is %d\n", 
                      index_,txbuffer.num_of_packet);
	     SendRTS();
		       }
}
  return;
}




void 
TMac::ReStart(){
printf("\n. ..ReStart node %d re-start up at %f...\n\n",index_,NOW); 

   printf("\n. ..Restart  node %d schedule poweroff after %f at%f...\n\n",index_,TAduration_,NOW);

  Scheduler &s=Scheduler::instance();
  s.cancel(&poweroff_event);

  // s.cancel(&wakeup_event);

  s.schedule(&poweroff_handler,&poweroff_event,TAduration_);
 

  if((mac_status==TMAC_SILENCE)||(mac_status==TMAC_IDLE))
    {
            mac_status=TMAC_IDLE;
                 if (NewData())
                       { 
           printf("Restart: There is new data in node %d and the number of packet is %d\n", 
                      index_,txbuffer.num_of_packet);
	     SendRTS();
		       }
    }
  return;
}


void 
TMac::PrintTable(){

 
  printf("TMac: the short latency table in node%d...\n",index_); 

  for (int i=0;i<TABLE_SIZE;i++)
{
  printf("node addr is%d and short latency is%f\n",short_latency_table[i].node_addr,short_latency_table[i].latency);
}


printf("TMac: the period table in node%d...\n",index_); 

  for (int i=0;i<TABLE_SIZE;i++)
{
  printf("node addr is%d and difference is%f\n",period_table[i].node_addr,period_table[i].difference);
}

}

bool 
TMac::NewData(){
  return (!txbuffer.IsEmpty());//?think about it
}


void 
TMac::ProcessSilence()
{
  printf("TMac: ProcessSilence node%d and num of silence record %d at %f\n",
           index_,silenceTableIndex,NOW);
  
  CleanSilenceTable();

  if(silenceTableIndex==0) 
  {
     InitializeSilenceTable();
      ReStart();
      return;
  }      

  printf("TMac: ProcessSilence node %d: there still exists silence record..\n",index_);
  double silenceTime=0;
  silenceTime=silence_table[0].start_time+silence_table[0].duration;
  for (int i=0;i<silenceTableIndex;i++)
    {
      double t1=silence_table[i].start_time;
      double t2=silence_table[i].duration;
      if(silenceTime<t1+t2) silenceTime=t1+t2;
    } 
  
  double t=silenceTime-NOW;
   Scheduler &s=Scheduler::instance();
   s.cancel(&silence_event);
   s.schedule(&silence_handler,&silence_event,t);
   last_silenceTime=NOW+t;    
  return; 
}

void 
TMac::CleanSilenceTable()
{
  if(silenceTableIndex==0) return;
  int i=0;
 
  while (i<silenceTableIndex)
    {
      double st=silence_table[i].start_time;
      double du=silence_table[i].duration;
  
      if (((silence_table[i].confirm_id==0)||(st+du<=NOW))&&(silence_table[i].node_addr!=-1))
        {
   printf("Tmac:CleanSilence : node %d clears the silence record... \n",index_);
      DeleteSilenceTable(i);
        }
  else i++;
    }

}



void
TMac::DeleteSilenceTable(int index)
{
  for(int i=index;i<silenceTableIndex;i++)
    {
   silence_table[i].node_addr=silence_table[i+1].node_addr;
   silence_table[i].start_time=silence_table[i+1].start_time;
   silence_table[i].duration=silence_table[i+1].duration;
   silence_table[i].confirm_id=silence_table[i+1].confirm_id;
    }
   silenceTableIndex--;    
   return;
}

void
TMac::DeleteSilenceRecord(int node_addr)
{
  int index=-1;
  for(int i=0;i<silenceTableIndex;i++)
  if (silence_table[i].node_addr==node_addr) index=i;
  
  if(index!=-1) DeleteSilenceTable(index);
   return;
}



void 
TMac::InitializeSilenceTable()
{
  for(int i=0;i<TABLE_SIZE;i++)
    {
      silence_table[i].node_addr=-1;
      silence_table[i].start_time=0;
      silence_table[i].duration=0;
      silence_table[i].confirm_id=0;
    }
  silenceTableIndex=0;
  return;
}


void 
TMac::ConfirmSilenceTable(int sender_addr, double duration)
{
  int index=-1;
  for(int i=0;i<silenceTableIndex;i++)
    if(silence_table[i].node_addr==sender_addr) index=i;
 
  if(index!=-1)silence_table[index].confirm_id=1;
  else 
{
 InsertSilenceTable(sender_addr,duration);
 ConfirmSilenceTable(sender_addr, duration);
}
  return;
}

void 
TMac::DataUpdateSilenceTable(int sender_addr)
{
  printf("TMac:DataUpdateSilenceTable node %d...\n",index_);

  int index=-1;
for(int i=0;i<silenceTableIndex;i++)
  if(silence_table[i].node_addr==sender_addr) index=i;
  
//printf("TMac:DataUpdateSilenceTable node %d index of this record is %d...\n",index_,index);
 if(index!=-1) silence_table[index].confirm_id=1;
 else 
{
  // printf("TMac:DataUpdateSilenceTable node %d this is new data record...\n",index_);
  double t=2*max_propagationTime+max_large_packet_transmissiontime;
  // InsertSilenceTable(sender_addr,t);
  ConfirmSilenceTable(sender_addr,t);
 }
}



void 
TMac::SendRTS()
{ 
printf("tmac SendRTS: node %d ...at %f\n",index_,NOW);

     Scheduler& s=Scheduler::instance();
           
	    rts_timeout_handler.num=0;
             s.cancel(&rts_timeout_event);

 if(mac_status==TMAC_SILENCE) 
   {
    printf("tmac SendRTS: node %d  is in TMAC_SILENCE state at %f\n",index_,NOW);       
	     return; 
   }

  Packet* p=txbuffer.head();
  hdr_cmn*  cmh = HDR_CMN(p);
  int receiver_addr=cmh->next_hop();
  
   txbuffer.LockBuffer();
   int num=txbuffer.num_of_packet;

  printf("tmac SendRTS: node %d lock txbuffer \n",index_);

  int sender_addr=index_;
  double l=CheckLatency(short_latency_table,receiver_addr);
  double du=num*(((large_packet_size_*encoding_efficiency_+PhyOverhead_)/bit_rate_)+transmission_time_error_);
  double dt=3.1*l+max_propagationTime+du*2+max_propagationTime-max_short_packet_transmissiontime;      
    
  // Generate a RTS Packet

      Packet* pkt =Packet::alloc();
      hdr_tmac* rtsh = HDR_TMAC(pkt);
      cmh = HDR_CMN(pkt);
 

       cmh->next_hop()=receiver_addr;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_TMAC;
      
       rtsh->ptype=P_RTS;
       rtsh->pk_num = num_send;
       rtsh->duration=dt;
       rtsh->sender_addr=index_;
       rtsh->receiver_addr=receiver_addr;
       num_send++;

       assert(initialized());
       UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 
       hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

       double txtime=hdr_cmn::access(pkt)->txtime();

       mac_status=TMAC_RTS;

       // Scheduler &s=Scheduler::instance();
       s.cancel(&poweroff_event);
      
        double t2=Random::uniform()*ContentionWindow_; 
   
        rts_handler.receiver_addr=receiver_addr;  
        printf("SendRTS, node %d is in  TMAC_RTS at %f will tx RTS in  %f\n",index_,NOW,t2);
        s.schedule(&rts_handler, (Event*) pkt,t2);

	//   printf("TxRTS, node %d is in  TMAC_RTS at %f\n",index_,NOW);

}

void 
TMac::TxRTS(Event* e,int receiver_addr)
{
     
  printf("TxRTS, node %d is at %f\n",index_,NOW);
        Packet* pkt=(Packet*) e;
 
       Scheduler &s=Scheduler::instance();  
       s.cancel(&rts_timeout_event);
	

      	if(mac_status==TMAC_SILENCE)
          {
  printf("TxRTS, node %d is at silence state quit %f\n",index_,NOW);
         rts_timeout_handler.num=0;
	  Packet::free(pkt);
          return;
	  }

      
        hdr_cmn*  cmh = HDR_CMN(pkt);

        hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

        double txtime=hdr_cmn::access(pkt)->txtime();

        double l=CheckLatency(short_latency_table, receiver_addr);
        double t=2.2*l+MinBackoffWindow*2;
        UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
 
         TransmissionStatus status=n->TransmissionStatus();      

         if(IDLE==status){
  
            n->SetTransmissionStatus(SEND);
 
              cmh->ts_=NOW;
             sendDown(pkt);
     // printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

      printf("TxRTS, node %d Tx RTS at %f and timeout is %f number of try is %d\n",index_,NOW,t,rts_timeout_handler.num);
            status_handler.SetStatus(IDLE);
	    //    Scheduler& s=Scheduler::instance();
	    // s.cancel(&rts_timeout_event);
            s.schedule(&status_handler,&status_event,txtime);
	    rts_timeout_handler.num=0;
             s.schedule(&rts_timeout_handler,&rts_timeout_event,t);

                return;
	 }


 if(RECV==status)
    {      
        printf("TxRTS, node %d is in RECV state, backoff...\n",index_);
	// Scheduler &s=Scheduler::instance();
    
        double t2=MinBackoffWindow+Random::uniform()*MinBackoffWindow; 
        rts_handler.receiver_addr=receiver_addr;  
        s.schedule(&rts_handler, (Event*) pkt,t2);

     return;
    }

if (SEND==status)
  {
    printf("tmac: queue send data too fast\n");
    Packet::free(pkt);
      return;
  }

}



double 
TMac::CheckLatency(latency_record* table,int addr)
{
  int i=0;
double d=0.0;
 
 while((table[i].node_addr!=addr)&&(i<TABLE_SIZE))
{
  //printf("node addr is%d and latency is%f\n",table[i].node_addr,table[i].latency);
 i++;
}
 if (i==TABLE_SIZE) return d;
 else return table[i].latency;
}




double 
TMac:: CheckDifference(period_record* table,int addr)
{
  int i=0;
double d=-0.0;
 
 while((table[i].node_addr!=addr)&&(i<TABLE_SIZE))i++;

 if (i==TABLE_SIZE) return d;
 else return table[i].difference;
}


void 
TMac::ProcessCTSPacket(Packet* pkt)
{
    
    printf("TMac:ProcessCTSPacket: node %d at time %f\n",index_,NOW);

    hdr_tmac* ctsh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int sender_addr=ctsh->sender_addr;
    int receiver_addr=ctsh->receiver_addr;
    double dt=ctsh->duration; 
    double  l=CheckLatency(short_latency_table,sender_addr);
    double t=dt-2*l;
      
    Packet::free(pkt);

    if(receiver_addr!=index_) 
      {
      if(t>0)
	{
	  ConfirmSilenceTable(receiver_addr,t);
	  if(mac_status==TMAC_SILENCE)
          {
         printf("tmac:ProcessCTS: node %d, I am already in silence state %d\n", index_,mac_status);  
	    if(last_silenceTime<t+NOW)
            {
            printf("tmac:ProcessCTS: node %d, the silence is longer than existing one...\n", index_);  
            Scheduler& s=Scheduler::instance();
            s.cancel(&silence_event);
            s.schedule(&silence_handler,&silence_event,t);
            last_silenceTime=NOW+t;
	    }
          }// end of silence state
          else              
	    {
      printf("tmac:ProcessCTS: node %d, I am going to be in silence state\n", index_);  
     
      Scheduler& s=Scheduler::instance();
      if(mac_status==TMAC_IDLE) s.cancel(&poweroff_event);
      s.cancel(&timeout_event);
      
      s.schedule(&silence_handler,&silence_event,t);
       mac_status=TMAC_SILENCE;
      last_silenceTime=NOW+t;
	    }
	}
      }// end of no-intended receiver
    else {
      if(mac_status!=TMAC_RTS) {
     printf("tmac:ProcessCTS:status change, I quit this chance\n");  
     return;
      }

      printf("tmac:ProcessCTS: node %d this CTS is for me \n",index_); 
 
      double t=max_propagationTime-l;    
      Scheduler& s=Scheduler::instance();   
      s.cancel(&rts_timeout_event);// cancel the timer of RTS
      rts_timeout_handler.num=0;
      transmission_handler.receiver=sender_addr;      
      s.schedule(&transmission_handler,&transmission_event,t);
    }
  
    return;
    
}


void 
TMac::ClearTxBuffer()
{
 printf("TMac: ClearTxBuffer: node %d clear its buffer\n",index_);

  Packet*  p1[MAXIMUM_BUFFER];
  
  for (int i=0;i<MAXIMUM_BUFFER;i++)p1[i]=NULL;
  buffer_cell* bp=txbuffer.head_;
  int i=0;
  while(bp){
    p1[i]=bp->packet;
    bp=bp->next;
    i++;
  }

 

  for (int i=0;i<MAXIMUM_BUFFER;i++){
    //   printf("ClearTxBuffer the poniter is%d\n",p1[i]);
    if (bit_map[i]==1) txbuffer.DeletePacket(p1[i]);
  

  }
 
  printf("txbuffer is cleared, there are %d packets left\n",txbuffer.num_of_packet);

  return; 
}



void 
TMac::ProcessACKDataPacket(Packet* pkt)
{
printf("tmac:ProcessACKDATAPacket: node %d process ACKDATA packets at time %f duration_=%f\n",index_,NOW,duration_);
    hdr_tmac* ackh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);
 
     int dst=cmh->next_hop();
 
     if(dst!=index_){
       Packet::free(pkt);
   return;
     }

     if(mac_status!=TMAC_TRANSMISSION)
       {
printf("tmac:ProcessACKDATAPacket: node %d not in transmission state, just quit\n",index_);
 Packet::free(pkt);
 return;
       }

    Scheduler& s=Scheduler::instance();
    printf("tmac:ProcessAckData: node %d cancel timeout dutation=%f\n",index_,duration_);    
      s.cancel(&timeout_event);// cancel the timer of data
  

  
    
    for (int i=0;i<MAXIMUM_BUFFER;i++)bit_map[i]=0;

    memcpy(bit_map, pkt->accessdata(),sizeof(bit_map));
  
  printf("tmac:ProcessACKDATAPacket: node %d received the bitmap is:\n",index_);
  

  for (int i=0;i<MAXIMUM_BUFFER;i++) printf("bmap[%d]=%d ",i,bit_map[i]);
  printf("\n");

  printf("txbuffer will be cleared, there are %d packets in queue and duration=%f\n",txbuffer.num_of_packet,duration_);

      Packet::free(pkt);   


 /*
!!!!
This part should consider the retransmission state, in this implementation, we don't consider the packets loss, therefore, we just ignore it, it should be added later. 

  */

    ClearTxBuffer(); 

    txbuffer.UnlockBuffer();
  

    printf("tmac:ProcessACKDATAPacket: node %d unlock txbuffer duration_=%f\n",index_,duration_);
    ResumeTxProcess();

    mac_status=TMAC_IDLE;
   
     ReStart();
    return;
}


void 
TMac::ProcessRTSPacket(Packet* pkt)
{
    
    printf("TMac:ProcessRTSPacket: node %d is processing rts\n",index_);

    hdr_tmac* rtsh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int sender_addr=rtsh->sender_addr;
    int receiver_addr=cmh->next_hop();
   
    double l=CheckLatency(short_latency_table, sender_addr);
    double duration=rtsh->duration;
    double silenceTime=duration-2*l;
    double t=2*max_propagationTime
              +2*max_short_packet_transmissiontime+2*MinBackoffWindow;
    Packet::free(pkt);
   

    if((receiver_addr!=index_)&&(silenceTime>0))
      {
   
  
      if(mac_status==TMAC_IDLE)
	{
      printf("TMAC:ProcessRTS: node %d: I am not the intended receiver and will be in silence \n", 
                index_);
       InsertSilenceTable(sender_addr,silenceTime);
     
       mac_status=TMAC_SILENCE;

      Scheduler &s=Scheduler::instance();
      s.cancel(&poweroff_event);
      s.schedule(&silence_handler,&silence_event,t);
      last_silenceTime=t+NOW;
      return;
	}
    if(mac_status==TMAC_SILENCE)
          {
       InsertSilenceTable(sender_addr,silenceTime);
	    if(last_silenceTime<t+NOW) 
            {
  printf("TMAC:ProcessRTS: node %d: I am not the intended receiver, gets a longer silence...  \n", index_);

            Scheduler& s=Scheduler::instance();
            s.cancel(&silence_event);
            s.schedule(&silence_handler,&silence_event,t);
            last_silenceTime=NOW+t;
	    }
              return;
	  }

    if(mac_status==TMAC_RTS)
      {
    
	if (sender_addr<index_) 
            {
 printf("TMAC:ProcessRTS: node %d: I am not the intended receiver and quits the  RTS state \n", index_);
         InsertSilenceTable(sender_addr,silenceTime);
              mac_status=TMAC_SILENCE;
              Scheduler &s=Scheduler::instance();
              s.cancel(&poweroff_event);
              s.cancel(&rts_timeout_event);
	       rts_timeout_handler.num=0;
              s.schedule(&silence_handler,&silence_event,t);
              last_silenceTime=t+NOW;
	    }
      return;
      }
    printf("TMac: ProcessRTS node %d : this RTS is not for me and I am in unknow state\n",index_);
    return;
      }

    if(receiver_addr!=index_) return;

    if(mac_status==TMAC_IDLE)
    {  
      printf("TMac:ProcessRTSPacket: node %d is in idle state and ready to process the RTS\n",index_);
      double dt=0;
   
      // dt=l+2.5*duration+max_propagationTime;
      dt=duration-l;
      double dt1=dt-l-max_short_packet_transmissiontime;

      Scheduler &s=Scheduler::instance();
      s.cancel(&poweroff_event);
     

      Packet* p=GenerateCTS(sender_addr,dt);
      TxCTS(p);
          
         s.cancel(&ack_event);    
           s.schedule(&ack_handler,&ack_event,dt1);

      return;
    }

    if(mac_status==TMAC_CTS)
      {
 printf("TMac:ProcessRTSPacket: node %d is in CTS state \n",index_);
	return;
      }

      if(mac_status==TMAC_SILENCE)
      {
 printf("TMac:ProcessRTSPacket: node %d is in SILENCE state\n",index_);
	return;
      }

  printf("TMac:ProcessRTSPacket: node %d is in Unknown state\n",index_);  
}

void 
TMac:: InsertSilenceTable(int sender_addr,double duration)
{
  int index=-1;
  for(int i=0;i<silenceTableIndex;i++)
    if(silence_table[i].node_addr==sender_addr) index=i;


  if(index==-1) // this is a new silence record
 {
   printf("TMac::InsertSilenceTable: node %d: this silence from node %d is new one,duration is %f at time %f \n",index_,sender_addr,duration,NOW);
   silence_table[silenceTableIndex].node_addr=sender_addr;
   silence_table[silenceTableIndex].start_time=NOW;
   silence_table[silenceTableIndex].duration=duration;
   silence_table[silenceTableIndex].confirm_id=0;
   silenceTableIndex++;
  }
  else
    {
 printf("TMac::InsertSilenceTable: node %d: this silence from node %d is old one, duration=%f at time %f..\n",index_,sender_addr, duration,NOW);
      silence_table[index].start_time=NOW;
      silence_table[index].duration=duration;
      silence_table[index].confirm_id=0;
    }

  return;
}


Packet* 
TMac::GenerateCTS(int receiver_addr, double duration){

      printf("tmac:GenerateCTS: node %d\n",index_);
    
      Packet* pkt =Packet::alloc();
      hdr_tmac* ctsh = HDR_TMAC(pkt);
      hdr_cmn*  cmh = HDR_CMN(pkt);
 

       cmh->next_hop()=receiver_addr;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_TMAC;
       cmh->size()=short_packet_size_;      

       ctsh->ptype=P_CTS; 
       ctsh->pk_num = num_send;
       ctsh->receiver_addr=receiver_addr;
       ctsh->duration=duration;
       ctsh->sender_addr=index_;
       num_send++;
       return pkt;         
}


void
TMac::TxCTS(Packet *p)
{
  printf("TxCTS, node %d tx CTS %f ...\n",index_,NOW);
  if(mac_status==TMAC_SILENCE)
    {
  printf("TMac:node %d, I am in silence state, I have to quit..\n",index_); 
       Scheduler& s=Scheduler::instance();    
       s.cancel(&ack_event);
       s.cancel(&timeout_event);
      cts_handler.num=0;
      Packet::free(p);
      return;
    }

  if(cts_handler.num==2)
    {
      printf("TMac:node %d, I have to try to send CTS twice, I have to quit..\n",index_); 
      cts_handler.num=0;

       Scheduler& s=Scheduler::instance();    
       s.cancel(&ack_event);
 
      Packet::free(p);
      mac_status=TMAC_IDLE;
      ReStart();
      return;
    }
  
  hdr_cmn* cmh=HDR_CMN(p);
  hdr_tmac* ctsh = HDR_TMAC(p); 

         mac_status=TMAC_CTS;
        hdr_cmn::access(p)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

        double txtime=hdr_cmn::access(p)->txtime();
        int receiver_addr=cmh->next_hop();

     
        double l=CheckLatency(short_latency_table, receiver_addr);
        double t=2.2*l+MinBackoffWindow*2+max_short_packet_transmissiontime
                                         +max_large_packet_transmissiontime;
        double t1=transmission_time_error_+ctsh->duration;     
       
       for (int i=0;i<MAXIMUM_BUFFER;i++) bit_map[i]=0;

       
         UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
          TransmissionStatus status=n->TransmissionStatus();

         if(IDLE==status){
           

            n->SetTransmissionStatus(SEND);
 
            cmh->ts_=NOW;

            sendDown(p);

      printf("TxCTS, node %d tx CTS %f and timeout is set at %f\n",index_,NOW,t);
           
            status_handler.SetStatus(IDLE);
            Scheduler& s=Scheduler::instance();
       
            s.cancel(&timeout_event);
	    cts_handler.num=0;
            s.schedule(&status_handler,&status_event,txtime);   
         
            s.schedule(&timeout_handler,&timeout_event,t);   
      
                return;
	 }

  if(RECV==status)
    { 
        printf("TxCTS, node %d has to back off \n",index_,NOW);
        Scheduler &s=Scheduler::instance();
    
        double t2=MinBackoffWindow*(1+Random::uniform()); 
      
        s.schedule(&cts_handler, (Event*) p,t2);


     return;      

    }
    
    if (SEND==status)
    {
  // this case is supposed not to  happen 
    printf("tmac: SendCTS is in wrong status\n");
    Packet::free(p);
      return;

     }
}


void 
TMac::SetIdle()
{

  // sometimes, need to cancel the timer 
  Scheduler& s=Scheduler::instance();
  s.cancel(&ack_event);

  printf("SetIdle: node %d at %f\n",index_,NOW);
  if(mac_status==TMAC_SILENCE) return;
  else 
   {
   mac_status=TMAC_IDLE;
   ReStart();
   return;
  }
}





void 
TMac::ProcessNDPacket(Packet* pkt)
{
  // printf("tmac:ProcessNDPacket: node %d\n",index_);
    hdr_tmac* ndh=HDR_TMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=ndh->sender_addr;
    double time=NOW;
    if(arrival_table_index>=TABLE_SIZE){ 
      printf("tmac:ProcessNDPacket:arrival_table is full\n");
      Packet::free(pkt);
      return;
    }
    arrival_table[arrival_table_index].node_addr=sender;
    arrival_table[arrival_table_index].arrival_time=time;
    arrival_table[arrival_table_index].sending_time=cmh->ts_;
    arrival_table_index++;
    Packet::free(pkt);
      return;
}


void 
TMac::ProcessDataPacket(Packet* pkt)
{
 printf("tmac:ProcessDataPacket: node %d  my status is %d\n",
         index_,mac_status);

  hdr_uwvb* vbh=HDR_UWVB(pkt);
  hdr_tmac* tmach=HDR_TMAC(pkt);
  hdr_cmn* cmh=HDR_CMN(pkt);


  int dst=cmh->next_hop();
  data_sender=tmach->sender_addr;
  int num=tmach->data_num;

  if(dst!=index_) 
{
  if(mac_status==TMAC_SILENCE) DataUpdateSilenceTable(data_sender);
  else 
  {
  printf("tmac:processDataPacket: node %d, I am not in silence state, my state is %d\n", index_,mac_status);
      
      double t=2*max_propagationTime+max_large_packet_transmissiontime;
      Scheduler& s=Scheduler::instance();
      
      if(mac_status==TMAC_IDLE) s.cancel(&poweroff_event);
      s.cancel(&timeout_event);
      mac_status=TMAC_SILENCE;
      s.cancel(&silence_event);
      s.schedule(&silence_handler,&silence_event,t);  

  }

return;
}
 
  if(mac_status==TMAC_CTS) mac_status=TMAC_RECV;
      Scheduler& s=Scheduler::instance();
      s.cancel(&timeout_event);

        MarkBitMap(num);
printf("tmac:ProcessDataPacket: node %d  send up the packet\n",index_);
    uptarget_->recv(pkt,this);
    // SendACKPacket();
      return;
}

void 
TMac::MarkBitMap(int num){
  if(num<MAXIMUM_BUFFER) bit_map[num]=1;
}


void 
TMac::SendACKPacket()
{
  printf("tmac Send ACK: node %d at %f\n",index_,NOW);
  
  if(data_sender<0) {
    printf("Ramc:ScheduleACKData: invalid sender address\n");
    return; 
  }

  if(mac_status!=TMAC_RECV) {
   printf("Ramc:SendACK: invalid state\n");
     return; 
  }
  Packet* pkt=Packet::alloc(sizeof(bit_map)); 
  hdr_cmn*  cmh = HDR_CMN(pkt);
   hdr_tmac* revh = HDR_TMAC(pkt);
 
       memcpy(pkt->accessdata(),bit_map,sizeof(bit_map));

       printf("tmac Schdeule ACKDATA: node %d return bitmap is \n",index_);
  for(int i=0;i<MAXIMUM_BUFFER;i++) printf("bit_map[%d]=%d ",i,bit_map[i]);

       printf("\n");         


 
       cmh->next_hop()=data_sender;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_TMAC;
       cmh->size()=short_packet_size_;     
     
       revh->ptype=P_ACKDATA;
       revh->pk_num = num_send;
       revh->sender_addr=index_;
       num_send++;
       TxACKData(pkt);
}


void  
TMac::TxACKData(Packet* pkt){
 printf("TMac TxACKData node %d at %f\n",index_,NOW);

  hdr_cmn* cmh=HDR_CMN(pkt);
 
  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

  mac_status=TMAC_IDLE;

  Scheduler &s=Scheduler::instance();
  s.cancel(&poweroff_event);
  s.schedule(&poweroff_handler,&poweroff_event,TAduration_);

  //  printf("TxACKData, node %d is in  TMAC_IDLE at %f\n",index_,NOW);
 TransmissionStatus status=n->TransmissionStatus();
 
  if(SLEEP==status) {
  Poweron();
  n->SetTransmissionStatus(SEND); 
  cmh->ts_=NOW;

  sendDown(pkt);

  // printf("TMac TxACKData node %d at %f\n",index_,NOW);
  status_handler.SetStatus(SLEEP);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==status){
   printf("TMac TxACKData node %d is idle state at %f\n",index_,NOW);
    n->SetTransmissionStatus(SEND);
 
     cmh->ts_=NOW;

     sendDown(pkt);
 
  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==status)
    {
printf("TMac TxACKData node %d is in recv state at %f, will be interrupted\n",index_,NOW);
      
      InterruptRecv(txtime);
      cmh->ts_=NOW;
      sendDown(pkt);

      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
     return;
    }
if (SEND==status)
  {
    printf("tmac: node%d send data too fast\n",index_);
    Packet::free(pkt);
      return;
  }

}



/*
 this program is used to handle the received packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
TMac::TxData(int receiver)
{
  printf("TMac:node %d TxData at time %f\n",index_,NOW);

  if (txbuffer.IsEmpty()) 
{
printf("TMac:TxData: what a hell! I don't have data to send\n");
return;
}


  if((mac_status!=TMAC_RTS)&&(mac_status!=TMAC_TRANSMISSION)) {
 printf("TMac:TxData: node %d is not in transmission state\n");
      return;
  }

   UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
   if(n->TransmissionStatus()==SLEEP) Poweron();

     mac_status=TMAC_TRANSMISSION;

    Packet* pkt=txbuffer.next();
   
    hdr_cmn* cmh=hdr_cmn::access(pkt);
    hdr_tmac* datah =hdr_tmac::access(pkt);
    hdr_uwvb* hdr2=hdr_uwvb::access(pkt);

    // printf("TMac:node %d TxData at time %f data type is %d offset is%d and size is %d and offset is %d and size is%d uwvb offset is %d and size is %d\n",index_,NOW,hdr2->mess_type,cmh,sizeof(hdr_cmn),datah,sizeof(hdr_tmac),hdr2,sizeof(hdr_uwvb));
         datah->ptype=P_DATA;  

         datah->sender_addr=index_;
   
         datah->pk_num=num_send;
         datah->data_num=num_data;
         num_send++;
          num_data++;
  
          cmh->size()=large_packet_size_;

            cmh->next_hop()=receiver;
	   
            cmh->direction()=hdr_cmn::DOWN; 
            cmh->addr_type()=NS_AF_ILINK;
            cmh->ptype_=PT_TMAC; 

	    

  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

 printf("TMac:node %d TxData at time %f data type is %d packet data_num=%d class data_num=%d \n",index_,NOW,hdr2->mess_type,datah->data_num,num_data);
  TransmissionStatus status=n->TransmissionStatus();

 
 if(IDLE==status)
 {
  n->SetTransmissionStatus(SEND); 
        sendDown(pkt);
	// printf("TMac:node %d TxData at %f\n ",index_,NOW);
        status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);  
 }

 if(RECV==status)
    {

      printf("TMac:node %d TxData interrupt the receiving status  at %f\n ",index_,NOW);
     
      InterruptRecv(txtime);      
      sendDown(pkt);

      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
    }

 if (SEND==status)
    { 
    printf("tmac:Txdata: queue send data too fast\n");
    Packet::free(pkt);
    }

  
  if (txbuffer.IsEnd()) {
   
   
   printf("tmac:Txdata: node %d is in state MAC_TRANSMISSION\n",index_);
  
   // double l=CheckLatency(short_latency_table, receiver);  
   double dt=((large_packet_size_*encoding_efficiency_+PhyOverhead_)/bit_rate_)+transmission_time_error_;
   // double t=2.1*max_propagationTime+transmission_time_error_+dt;  
   double t=2.0*max_propagationTime+2.0*MinBackoffWindow+(2+num_data)*max_large_packet_transmissiontime;

   printf("TMac:node %d TxData at %f and timeout is set %f\n ",index_,NOW,t);
   Scheduler& s=Scheduler::instance();
   s.cancel(&timeout_event);
   s.schedule(&timeout_handler,&timeout_event,t);
   num_data=0;
  }
  else {
  double it=SIF_+txtime;   
 
 printf("tmac:Txdata: node%d schedule  next data packet , interval=%f at time%f\n",index_,it,NOW);
  Scheduler& s=Scheduler::instance();
  s.schedule(&transmission_handler,&transmission_event,it);
  }

}



void 
TMac::ResumeTxProcess(){

  printf("tmac:ResumeTxProcess: node %d at %f\n",index_,NOW);

  if(!txbuffer.IsFull()) 
  if(callback_) callback_->handle(&status_event);
  return;
}



/*
 this program is used to handle the transmitted packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
TMac::TxProcess(Packet* pkt){

  hdr_uwvb* hdr=HDR_UWVB(pkt);

  printf("tmac:TxProcess: node %d type is %d\n",index_,hdr->mess_type);
 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if (n->setHopStatus){
   hdr_cmn* cmh=HDR_CMN(pkt);
   cmh->next_hop()=n->next_hop;
   cmh->error()=0;// set off the error status;
   // printf("tmac:TxProcess: node %d set next hop to %d\n",index_,cmh->next_hop());
  }
 

  txbuffer.AddNewPacket(pkt);
  printf("tmac:TxProcess: node %d put new data packets in txbuffer\n",index_);
  if(!txbuffer.IsFull()) 
  if(callback_) callback_->handle(&status_event);
  return;
}


void 
TMac::RecvProcess(Packet* pkt)
{
 
   hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_tmac* cmh1=HDR_TMAC(pkt);
   int dst=cmh->next_hop();
   int ptype=cmh1->ptype; 
   int receiver_addr=cmh1->sender_addr;

   if(cmh->error()) 
 {
  printf("tmac:node %d  gets a corrupted packet from node %d at  %f \n",
          index_,receiver_addr,NOW);
    return;
   }
printf("tmac:node %d  gets a packet from node %d at  %f \n",
          index_,receiver_addr,NOW);

 if(dst==MAC_BROADCAST){
    if (ptype==P_ND) ProcessNDPacket(pkt); //this is ND packet
    if (ptype==P_SYN) ProcessSYN(pkt);
    return;
  }

  
 
   if ((ptype==P_SACKND)&&(dst==index_)) 
    {
        ProcessShortACKNDPacket(pkt); 
	return;
    }
   if (ptype==P_DATA){
      ProcessDataPacket(pkt);
      return;
   }
   if (ptype==P_ACKDATA) {
      ProcessACKDataPacket(pkt);
      return;
     // printf("underwaterbroadcastmac:this is my packet \n");
   } 

   if(ptype==P_RTS)
    {
    ProcessRTSPacket(pkt);
    return;
   }

   if(ptype==P_CTS)
      {
    ProcessCTSPacket(pkt);
    return;
      }
 printf("tmac: node%d this is neither broadcast nor my packet %d, just drop it at %f\n",index_,dst, NOW);
   Packet::free(pkt);
   return;
}


 
void 
TMac::StatusProcess(Event* p, TransmissionStatus  state)
{

   printf("TMac StatusProcess node %d \n",index_);
 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
 
 if(SLEEP==n->TransmissionStatus()) return;

    n->SetTransmissionStatus(state);

 return;
}




int
TMac::command(int argc, const char*const* argv)
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

	return UnderwaterMac::command(argc, argv);
}
