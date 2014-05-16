/* 
In this version of RMAC, the major modifications for the 10-5-2006 
  version are :
1. Adding the number of control for phase 2
2. Adding the full collecting reservations, i.e., each node has 
   to collect reservations in two consecutive periods.       
3. when the revack windows collide, the receiver will schedule the ackrev 
in the next period interval
4. no carrier sensing necessary               
*/

#include "packet.h"
//#include "random.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"
#include "mac.h"
#include "rmac.h"
#include "underwaterphy.h"
#include "random.h"
#include <stdlib.h>


int hdr_rmac::offset_;



static class RMACHeaderClass: public PacketHeaderClass{
 public:
  RMACHeaderClass():PacketHeaderClass("PacketHeader/RMAC",sizeof(hdr_rmac))
{
 bind_offset(&hdr_rmac::offset_);
}
} class_rmachdr;




//MACRECVHandler::MACRECVHandler(RMac* p): mac_(p),duration(0),status(0),data_sender(0){}

void 
MACRECVHandler::handle(Event* e)
{
  mac_->StartRECV(duration, status, data_sender);
}

//ClearChannelHandler::ClearChannelHandler(RMac* p): mac_(p){}

void 
ClearChannelHandler::handle(Event* e)
{
  mac_->ClearChannel();
}




//ACKREVHandler::ACKREVHandler(RMac* p): mac_(p){}

void 
ACKREVHandler::handle(Event* e)
{
  printf("ACKREVHandler: node%d handle ackrev\n",mac_->index_);
  mac_->TxACKRev((Packet*) e);
}

//SYNHandler::SYNHandler(RMac* p): mac_(p){}

void 
SYNHandler::handle(Event* e)
{
  printf("SYNHandler: node%d handle syn\n",mac_->index_);
  mac_->TxND((Packet*) e, mac_->PhaseTwo_window_);
}


//TimeoutHandler::TimeoutHandler(RMac* p): mac_(p){}

void 
TimeoutHandler::handle(Event* e)
{
 
  mac_->ResetMacStatus();
}

/*

void 
RMac::InitPhaseThree(){

 
  printf("RMac: this is InitPhaseThree\n"); 

   SortPeriodTable();
   PrintTable();

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->TransmissionStatus()==SLEEP) Poweron();
  
  mac_status=RMAC_IDLE;
  Scheduler &s=Scheduler::instance();
   s.schedule(&sleep_handler,&sleep_event,duration_);
    return;
}
*/



//SleepHandler::SleepHandler(RMac* p): mac_(p){}

void 
SleepHandler::handle(Event* e)
{
  mac_->ProcessSleep();
}

//WakeupHandler::WakeupHandler(RMac* p): mac_(p){}

void 
WakeupHandler::handle(Event* e)
{
  mac_->Wakeup();
}



//NDBackoffHandler::NDBackoffHandler(RMac* p):mac_(p),window_(0),counter_(0){}
 
void NDBackoffHandler::handle(Event*e)
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

void NDBackoffHandler::clear(){
counter_=0;
}


//ReserveHandler::ReserveHandler(RMac* p):mac_(p){}
 
void ReserveHandler::handle(Event*e)
{
  mac_->TxRev(e);
}


//ACKDATAHandler::ACKDATAHandler(RMac* p):mac_(p){}
 
void ACKDATAHandler::handle(Event*e)
{
  mac_->TxACKData(e);
}


//ACKWindowHandler::ACKWindowHandler(RMac* p):mac_(p){}
 
void ACKWindowHandler::handle(Event*e)
{
  mac_->ProcessListen();
}



//CarrierSenseHandler::CarrierSenseHandler(RMac* p):mac_(p){}
 
void CarrierSenseHandler::handle(Event*e)
{
  mac_->ProcessCarrier();
}


//TransmissionHandler::TransmissionHandler(RMac* p):mac_(p), receiver(0){}
 
void TransmissionHandler::handle(Event*e)
{
  
  mac_->TxData(receiver);
}



//NDStatusHandler::NDStatusHandler(RMac* p):mac_(p),status_(SLEEP){}
void NDStatusHandler::handle(Event* e)
{
  if(status_!=SLEEP) mac_->StatusProcess(e,status_);
  else mac_->Poweroff();
}


void NDStatusHandler::SetStatus(TransmissionStatus  status)
{
  status_=status;
}




//ShortNDHandler::ShortNDHandler(RMac* p):mac_(p){}

void ShortNDHandler::handle(Event* e)
{  
  
    mac_->cycle_start_time=NOW;
    mac_->SendND(mac_->short_packet_size_);
}



//ShortAckNDWindowHandler::ShortAckNDWindowHandler(RMac* p):mac_(p){}

void ShortAckNDWindowHandler::handle(Event* e)
{ 
   mac_->SendShortAckND();
}


//ACKNDHandler::ACKNDHandler(RMac* p):mac_(p){}

void ACKNDHandler::handle(Event* e)
{ 
    mac_->TxND((Packet*) e, mac_->ACKND_window_);
}




//PhaseOneHandler::PhaseOneHandler(RMac* p):mac_(p){}

void PhaseOneHandler::handle(Event* e)
{ 
    mac_->InitPhaseOne(mac_->ND_window_,mac_->ACKND_window_, mac_->PhaseOne_window_);
}



//PhaseTwoHandler::PhaseTwoHandler(RMac* p):mac_(p){}

void PhaseTwoHandler::handle(Event* e)
{ 
    mac_->StartPhaseTwo();
}

//PhaseThreeHandler::PhaseThreeHandler(RMac* p):mac_(p){}

void PhaseThreeHandler::handle(Event* e)
{ 
    mac_->InitPhaseThree();
}

/* ======================================================================
    RMAC for  underwater sensor
   ====================================================================== */
static class RMacClass : public TclClass {
public:
 RMacClass():TclClass("Mac/UnderwaterMac/RMac") {}
   TclObject* create(int, const char*const*) {
	  return (new RMac());
   }
} class_rmac;


RMac::RMac() :UnderwaterMac(),backoff_handler(this),short_nd_handler(this),short_acknd_window_handler(this), status_handler(this), phaseone_handler(this),acknd_handler(this), phasetwo_handler(this), phasethree_handler(this), sleep_handler(this), wakeup_handler(this), reserve_handler(this), ackrev_handler(this),mac_recv_handler(this),timeout_handler(this),transmission_handler(this),ackdata_handler(this),clear_channel_handler(this),ackwindow_handler(this),carrier_sense_handler(this),syn_handler(this)
{
  num_send=0;
  num_data=0;
  num_block=0;

  large_packet_size_=30;
  short_packet_size_=10;
  Timer=5;
 
  short_latency_table_index=0;

  reserved_time_table_index=0;
  reservation_table_index=0;
  ackdata_table_index=0;

  period_table_index=0;

  next_period=0;  
  ack_rev_pt=NULL;



  recv_busy=false;
  carrier_sense=false;
  collect_rev=false;
    
 for(int i=0;i<TABLE_SIZE;i++){
  
   next_available_table[i].node_addr=-1;
   next_available_table[i].required_time=0;

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
  bind("PhaseTwo_interval_",&PhaseTwo_interval_);
  bind("duration_",&duration_);
  bind("ND_window_",&ND_window_); 
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
  bind("ACKRevInterval_",&ACKRevInterval_);


   theta=transmission_time_error_/10.0;
   max_short_packet_transmissiontime=((1.0*short_packet_size_*encoding_efficiency_
                      +PhyOverhead_)/bit_rate_)*(1+transmission_time_error_);
   max_large_packet_transmissiontime=((1.0*large_packet_size_*encoding_efficiency_
                      +PhyOverhead_)/bit_rate_)*(1+transmission_time_error_);
  
  InitPhaseOne(ND_window_,ACKND_window_, PhaseOne_window_);
  
}

void 
RMac::InitPhaseOne(double t1,double t2, double t3)
{
  
   printf("RMac: Phaseone cycle: %d...\n",PhaseOne_cycle_);

   if(PhaseOne_cycle_)
    { 
      PhaseStatus=PHASEONE;
    Scheduler& s=Scheduler::instance();
    InitND(t1,t2,t3);
    s.schedule(&phaseone_handler,&phaseone_event,t3);
    PhaseOne_cycle_--;
    return;
    }
  
   // PrintTable();
   InitPhaseTwo();
   return;
}




void 
RMac::TxACKRev(Packet* pkt){

  printf("rmac Txackrev: node %d at time %f\n",index_,NOW);     
  DeleteBufferCell(pkt);
   
  hdr_cmn* cmh=HDR_CMN(pkt);
  hdr_rmac* ackh = HDR_RMAC(pkt); 

  printf("rmacTxackrev: node %d is transmitting a packet st=%f at time %f\n",index_,ackh->st,NOW);     
  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();
  printf("rmacTxackrev: node %d is transmitting a packet st=%f tx=%f at time %f\n",
          index_,ackh->st,txtime,NOW);   

  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND); 
  cmh->ts_=NOW;

  sendDown(pkt);
  // check if the sending this ACKRev collides with my own ackrev windows
  if(IsACKREVWindowCovered(NOW)) {
      printf("rmacTxackrev: node %d converged with ACKwindow\n",index_);   
     InsertReservedTimeTable(index_,PeriodInterval_,4*PeriodInterval_);
  }
  status_handler.SetStatus(SLEEP);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==n->TransmissionStatus()){
  
    n->SetTransmissionStatus(SEND);
     cmh->ts_=NOW;
     sendDown(pkt);
  //  printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

  // check if the sending this ACKRev collides with my own ackrev windows
  if(IsACKREVWindowCovered(NOW)) {
  printf("rmacTxackrev: node %d converged with ACKwindow\n",index_);  
  InsertReservedTimeTable(index_,PeriodInterval_,4*PeriodInterval_);
  }
  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      
      InterruptRecv(txtime);
      cmh->ts_=NOW;
      sendDown(pkt);

      if(IsACKREVWindowCovered(NOW)){
  printf("rmacTxackrev: node %d converged with ACKwindow\n",index_);  
       InsertReservedTimeTable(index_,PeriodInterval_,4*PeriodInterval_);
      }
      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
     return;
    }


if (SEND==n->TransmissionStatus())
  {
    printf("rmac: queue send data too fast\n");
    Packet::free(pkt);
      return;
  }
}



bool 
RMac::IsACKREVWindowCovered(double current_time)
{
  if((current_time-cycle_start_time==PeriodInterval_) 
     ||(current_time-cycle_start_time==0)) return true;
  else return false;
}

void 
RMac:: InsertReservedTimeTable(int sender_addr, double start_time, double dt)
{
 printf("rmac:InsertReservedTimeTable: node %d inserts  the reserved_time_table at %f\n",index_,NOW);
 if(reserved_time_table_index>=TABLE_SIZE){
  printf("rmac:InsertReservedTimeTable: the reserved_time_table is full\n");
        return;
 }
  int index=-1;
  for(int i=0;i<reserved_time_table_index;i++){
    if(reserved_time_table[i].node_addr==sender_addr) index=i;
  }
  if(index==-1)
    {
 reserved_time_table[reserved_time_table_index].node_addr=sender_addr;
      reserved_time_table[reserved_time_table_index].start_time=start_time;
      reserved_time_table[reserved_time_table_index].duration=dt;
      reserved_time_table_index++;
    }
  else 
{
      reserved_time_table[index].node_addr=sender_addr;
      reserved_time_table[index].start_time=start_time;
      reserved_time_table[index].duration=dt;
}
}


void 
RMac::DeleteBufferCell(Packet* p)
{
 printf("Rmac: node %d ack_rev link\n",index_);
  buffer_cell* t1;
  buffer_cell* t2;
  t1=ack_rev_pt;
 
  if(!t1){
  printf("Rmac: there is no ack_rev link\n");
  return;
  }

 
  if(t1->next) t2=t1->next; 


  if(t1->packet==p) {
           ack_rev_pt=ack_rev_pt->next;
           delete t1;
           return;
  }

  /*
  if(t2){

    Packet* t=t2->packet;
    hdr_ack_rev*  th=HDR_ACK_REV(t);
   printf("Rmac:node %d  !!!!sender_addr=%d\n", index_, th->sender_addr); 
  }
  */

  while(t2){
    if(p==t2->packet) {

      t1->next=t2->next;
      delete t2;
      return;
    }
    t1=t2;
    t2=t2->next;
  }

  return;
}


/*

void 
RMac::InitPhaseThree(){

 
  printf("RMac: this is InitPhaseThree\n"); 

   SortPeriodTable();
   PrintTable();

  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->TransmissionStatus()==SLEEP) Poweron();
  
  mac_status=RMAC_IDLE;
  Scheduler &s=Scheduler::instance();
   s.schedule(&sleep_handler,&sleep_event,duration_);
    return;
}
*/


void 
RMac::InitPhaseThree()
{
  printf("RMac: this is InitPhaseThree...\n"); 

   SortPeriodTable(period_table);
   PrintTable();

   mac_status=RMAC_IDLE;
   Wakeup();
    return;
}


void 
RMac::PrintTable(){

 
  printf("RMac: the short latency table in node%d...\n",index_); 

  for (int i=0;i<TABLE_SIZE;i++)
{
  printf("node addr is%d and short latency is%f\n",
        short_latency_table[i].node_addr,short_latency_table[i].latency);
}


printf("RMac: the period table in node%d...\n",index_); 

  for (int i=0;i<TABLE_SIZE;i++)
{
  printf("node addr is%d and difference is%f\n",period_table[i].node_addr,period_table[i].difference);
}

}


/*
void 
RMac::SortPeriodTable()
{
  printf("RMac:SortPeriodTable;node %d sort period table \n",index_);
  bool unswapped=false;
  int i=0;
  int j=0;

  while ((!unswapped)&&(i<period_table_index-1))
    {
      j=0;
      unswapped=true;
      while (j<period_table_index-1-i)
	{
       if(period_table[j].difference>period_table[j+1].difference)
	{
	  // printf("sortperiodtable;node %d swictch two values %f and %f \n",
	  // index_,period_table[j].difference, period_table[j+1].difference);
	  double dt=period_table[j].difference;
          int addr=period_table[i].node_addr;
          double du=period_table[i].duration;
          double lut=period_table[i].last_update_time;

          period_table[j].difference=period_table[j+1].difference;
          period_table[j].node_addr=period_table[j+1].node_addr;
          period_table[j].duration=period_table[j+1].duration;
          period_table[j].last_update_time=period_table[j+1].last_update_time;       
          
	  period_table[j+1].difference=dt;
          period_table[j+1].node_addr=addr;
          period_table[j+1].duration=du;
          period_table[j+1].last_update_time=lut;
          unswapped=false;
	}
       j++;
	}
      i++;
    }
}

*/






// use bubble algorithm to sort the period table

void 
RMac::SortPeriodTable(struct period_record * table)
{
  printf("RMac:SortPeriodTable;node %d sort period table \n",index_);
  bool unswapped=false;
  int i=0;
  int j=0;

  while ((!unswapped)&&(i<period_table_index-1))
    {
      j=0;
      unswapped=true;
      while (j<period_table_index-1-i)
	{
       if(table[j].difference>table[j+1].difference)
	{
	  // printf("sortperiodtable;node %d swictch two values %f and %f \n",
	  // index_,period_table[j].difference, period_table[j+1].difference);
	  double dt=table[j].difference;
          int addr=table[j].node_addr;
          double du=table[j].duration;
          double lut=table[j].last_update_time;

          table[j].difference=table[j+1].difference;
          table[j].node_addr=table[j+1].node_addr;
          table[j].duration=table[j+1].duration;
          table[j].last_update_time=table[j+1].last_update_time;       
          
	  table[j+1].difference=dt;
          table[j+1].node_addr=addr;
          table[j+1].duration=du;
          table[j+1].last_update_time=lut;
          unswapped=false;
	}
       j++;
	}
      i++;
    }
}


void 
RMac::ProcessSleep(){
   printf("RMac: node %d  is ProcessSleep at %f and wake up afte %f -%f\n", 
            index_,NOW,PeriodInterval_,duration_); 

   if(mac_status==RMAC_RECV) return; 

   Poweroff(); //? Is it safe to poweroff 

   if((mac_status==RMAC_IDLE)&&(reservation_table_index!=0))
   {
     if(!collect_rev) collect_rev=true;
      else{
     printf("RMac: node %d process_sleep reservation table is not empty %d\n",index_, reservation_table_index); 
     // mac_status=RMAC_ACKREV;
     ArrangeReservation();
      }
   }
   return;
}


void 
RMac::InsertBackoff()
{
  // int indx=-1;
  double elapsed_time=NOW-cycle_start_time;
  double start_time=elapsed_time+PeriodInterval_; 
  // adding PeriodInterval_ just to make sure that subsequent process correct
  double dt=4*PeriodInterval_;
  InsertReservedTimeTable(index_,start_time,dt);
}

void 
RMac:: CancelREVtimeout(){ 
  printf("rmac:CancelREVtimeout node %d \n",index_);
   Scheduler& s=Scheduler::instance();
   s.cancel(&timeout_event);      
    return;
}


void 
RMac:: ClearChannel(){ 
  printf("rmac:ClearChannel node %d at %f!!!!!!!!\n",index_,NOW);
  if(NewData()){
    if(mac_status==RMAC_FORBIDDED)
     {// avoid overlap
     MakeReservation();
     mac_status=RMAC_REV;
     }
  }
  else mac_status=RMAC_IDLE;
    return;
}


void 
RMac:: CancelReservation(){
   printf("rmac:cancelReservation: node %d\n",index_);
 
    for (int i=0;i<TABLE_SIZE;i++){
      reservation_table[i].node_addr=-1;
       } 
    return;
   
}


void 
RMac::StartRECV(double dt, int id, int data_sender)
 {
   printf("rmac:StartRECV: node %d at %f \n",index_,NOW);
  if(id==0) 
    {
      //   data_sender=-12;
     
   /*
   for (int i=0;i<MAXIMUM_BUFFER;i++) bit_map[i]=0;
   printf("rmac:StartRECV: node %d at %f to power on\n",index_,NOW);
   */

   Poweron();
   recv_busy=false;
  

   mac_status=RMAC_RECV;
   mac_recv_handler.status=1;
  
   double t=2*max_large_packet_transmissiontime;
  
   Scheduler& s=Scheduler::instance();
   s.schedule(&timeout_handler, &timeout_event,t);  
   s.schedule(&mac_recv_handler, &mac_recv_event,dt);  
    }
  else {
    // modification for the version02-10-2006 here, the RECV status ends when 
    // the receiver receives the data packets.
    // mac_status=RMAC_IDLE;
 printf("rmac:StartRECV: node %d at %f to power off\n",index_,NOW);
 ScheduleACKData(data_sender);
     Poweroff();
  }
	 return;
 }


void 
RMac::ArrangeReservation()
{
    printf("rmac:ArrangeReservation: node %d at %f...\n",index_,NOW);
       int sender_index=-1;
       if(ProcessRetransmission())
	 {
        printf("Rmac:Arrangereservation: node %d handle retransmission this time!!\n",index_); 
       return;
         }

        sender_index=SelectReservation();  
       if(sender_index==-1){
	 printf("Rmac:Arrangereservation: no reservation selected!!\n");
	 return;
       }
       else{
          
          mac_status=RMAC_ACKREV;

         int sender=reservation_table[sender_index].node_addr;
         double dt=reservation_table[sender_index].required_time;   
         double offset=reservation_table[sender_index].interval;

          printf("Rmac:Arrangereservation: sender %d and dutation %f is scheduled\n",
                 sender,dt);         
          ScheduleACKREV(sender,dt,offset);
       }
}


//the receiver is the address of the data sender

void 
RMac::ScheduleACKREV(int receiver, double duration, double offset)
{
  printf("rmac:ScheduleACKREV: node %d\n",index_);
  int i=0;
  //  double Number_Period=0;
  double last_time=0.0;
  double upper_bound=0;
  double elapsed_time=NOW-cycle_start_time;
 
   int     receiver_addr=receiver;
   double  dt=CheckDifference(period_table,receiver);
   double  latency=CheckLatency(short_latency_table,receiver_addr)
                     -max_short_packet_transmissiontime;



    printf("rmac:scheduleACKRev: node %d is scheduling ackrev, duration=%f, interval=%f\n",
             index_,duration, offset);
    while ((period_table[i].node_addr!=-1)&&(i<TABLE_SIZE))
      {
        if (period_table[i].node_addr!=receiver)
	  { 
	    int nid=period_table[i].node_addr;
            double d1=CheckDifference(period_table,nid);
            double l1=CheckLatency(short_latency_table,nid)-max_short_packet_transmissiontime;       
	    double t1=CalculateACKRevTime(d1,l1,elapsed_time); 

           
	    Packet* ackrev=GenerateACKRev(nid,receiver,duration);
             
            InsertACKRevLink(ackrev,&t1); 
      
            Scheduler& s=Scheduler::instance();
            s.schedule(&ackrev_handler, (Event*) ackrev,t1);
  printf("rmac:scheduleACKRev: node %d and node %d t1=%f the current  time is %f\n",
                  index_,nid,t1,NOW);
	    //   if (Number_Period<1) Number_Period=1;
       if(t1+2*l1>last_time) last_time=t1+2*l1;
       if(t1+l1>upper_bound) upper_bound=t1+l1;
	  }
	i++;

    } // end of all the neighbors


    //      double l=offset;

      // int receiver_addr=receiver;
      // double dt=CheckDifference(period_table,receiver);

      //  double latency=CheckLatency(short_latency_table,receiver_addr)
      //          -max_short_packet_transmissiontime;
      double t1=CalculateACKRevTime(dt,latency,elapsed_time);
     
       while(t1<upper_bound) t1+=PeriodInterval_;
       double t3=t1+2*latency+max_short_packet_transmissiontime;

        
        Packet* ackrevpacket=GenerateACKRev(receiver,receiver,duration);
        InsertACKRevLink(ackrevpacket,&t1);   

        Scheduler& s=Scheduler::instance();
        s.schedule(&ackrev_handler, (Event*) ackrevpacket,t1);  
   printf("rmac:scheduleACKRev: node %d and node %d t1=%f t3=%f latency=%f the current time is %f\n",
                index_, receiver,t1,t3,latency,NOW); 
        // decide the start time of reservation duration

        if(t3<last_time)t3=last_time;
 
        double st=dt+offset-elapsed_time+latency;
        while(t3>st) st+=PeriodInterval_;  // start time of reserved duration

	double end_time=st+duration;  // end time of reserved duration
         


	/* 
             this is modified from the version 02-10-2006 
	 */
	double available_period=PeriodInterval_+dt-elapsed_time;
        double end_period=PeriodInterval_-elapsed_time;

        // the earliest time period that is available for new request 
	while(available_period<(end_time+latency))available_period+=PeriodInterval_;
     

        available_period=available_period+
                        duration_-latency-max_short_packet_transmissiontime;
          
        while (end_period<available_period) end_period+=PeriodInterval_;

	// SetStartTime(ack_rev_pt,st,available_period);
        
        SetStartTime(ack_rev_pt,st,end_period);
	

	// SetStartTime(ack_rev_pt,st,end_time);
 printf("rmac:scheduleACKRev: node %d offset time %f  and set the end time  is %f  at %f\n", 
                index_,st,end_period,NOW);
        mac_recv_handler.status=0;

	// 2 times max_large_packet_transmissiontime is enough for ackdata
	// mac_recv_handler.duration=(duration+2*max_large_packet_transmissiontime);
   
	// this is time for ackdata
        mac_recv_handler.duration=duration;
        mac_recv_handler.data_sender=receiver;
        s.schedule(&mac_recv_handler, &mac_recv_event,st);  
}



// this function first check if the arranged slot collides with 
// the slot for the intended receiver if so, adjust its sending 
 //time to avoid interference the intended receiver, however, 
//  interfere with the receiver.

double 
RMac::CalculateACKRevTime(double diff1, double l1,double diff2,double l2)
{

  bool collision_status=false;
  double elapsed_time=NOW-cycle_start_time;
  double s1=diff1-l1;
  while (s1<0) s1+=PeriodInterval_;
  double s2=diff2-l2;
  while (s2<0) s2+=PeriodInterval_;

   
  double delta=s1-s2;
  if(((s1<=s2)&&(s2<=s1+max_short_packet_transmissiontime))
     || ((s2<=s1)&&(s1<=s2+max_short_packet_transmissiontime))) 
    collision_status=true;
 

  
  if(collision_status) 
  {
   printf("calculateACKRev: collision!! delat=%f\n",delta);
   delta=s1-s2;
  }
  else delta=0;
 double offset_time=diff2+delta;  
 while (elapsed_time+l2>offset_time)offset_time+=PeriodInterval_;
 return offset_time-l2-elapsed_time;

}


double 
RMac::CalculateACKRevTime(double diff,double latency, double elapsed_time)
{

 double offset_time=diff;
 while (elapsed_time+latency>offset_time)offset_time+=PeriodInterval_;
 return offset_time-latency-elapsed_time;
			 
}



Packet* 
RMac::GenerateACKRev(int receiver, int intended_receiver, double duration){

  printf("rmac:GenerateACKREV: node %d\n",index_);
      Packet* pkt =Packet::alloc();
      hdr_rmac* ackrevh = HDR_RMAC(pkt);
      hdr_cmn*  cmh = HDR_CMN(pkt);
 

       cmh->next_hop()=receiver;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_RMAC;
       cmh->size()=short_packet_size_;      

      ackrevh->ptype=P_ACKREV; 
      ackrevh->pk_num = num_send;
      ackrevh->receiver_addr=intended_receiver;
      ackrevh->duration=duration;
      ackrevh->sender_addr=index_;

      num_send++;
      return pkt;
}


// In old version of this program, the silence duration is not  changed 
// however, in new version of this program, the silence duration is also changed

void 
RMac::SetStartTime(buffer_cell* ack_rev_pt, double st, double next_period){
 printf("rmac setstarttime: node %d \n",index_);
  buffer_cell* t1;
  t1=ack_rev_pt;
  while(t1){
    hdr_rmac*  ackrevh=HDR_RMAC(t1->packet);
    double d=t1->delay;
    ackrevh->st=st-d;
    ackrevh->interval=next_period-d;
    ackrevh->duration=next_period-d;
     printf("rmac Setstarttime: node %d: offset time is: %f and next period is %f\n",
         index_,ackrevh->st,ackrevh->interval);
    t1=t1->next;
  }
}


/* // old version of SetStartTime
void 
RMac::SetStartTime(buffer_cell* ack_rev_pt, double st, double next_period){
 printf("rmac setstarttime: node %d \n",index_);
  buffer_cell* t1;
  t1=ack_rev_pt;
  while(t1){
    hdr_rmac*  ackrevh=HDR_RMAC(t1->packet);
    double d=t1->delay;
    ackrevh->st=st-d;
    ackrevh->interval=next_period-d;
 printf("rmac setstarttime: node %d interval to recv =%f and next period is %f\n",index_,ackrevh->st,ackrevh->interval);
    t1=t1->next;
  }
}

*/



void 
RMac::InsertACKRevLink(Packet* p, double d){
   printf("rmac:InsertACKREVLink: node %d\n",index_);
  buffer_cell* t1=new buffer_cell;
  t1->packet=p;
  t1->delay=d;
  t1->next=NULL;  

  if(ack_rev_pt==NULL){
    ack_rev_pt=t1;  
    printf("node %d ackrev link is empty\n", index_);
    return;
  }
  else 
    {
      buffer_cell* t2=ack_rev_pt;
      ack_rev_pt=t1;
      t1->next=t2;
      printf("node %d ackrev link is not empty\n", index_);
    
  return;
    }
}




void 
RMac::InsertACKRevLink(Packet* p, double* d){
  double s1=*d;
  double win=max_short_packet_transmissiontime;

   printf("rmac:InsertACKREVLink: node %d\n",index_);
  buffer_cell* t1=new buffer_cell;
  t1->packet=p;
  t1->delay=s1;
  t1->next=NULL;  

  if(ack_rev_pt==NULL){
    ack_rev_pt=t1;  
    printf("node %d ackrev link is empty\n", index_);
    return;
  }
  else 
    {
      buffer_cell* t2=ack_rev_pt;
      buffer_cell* tmp;
      printf("node %d ackrev link is not empty\n", index_);

      while(t2)
	{
	  tmp=t2;
	  double s2=t2->delay;
	  if(((s1<=s2)&&(s2<=s1+win))|| ((s2<=s1)&&(s1<=s2+win))) 
          {
	 printf("InsertACKrev:node %d finds collisions!\n",index_);
	 s1+=PeriodInterval_;
	  }
       t2=t2->next;
	}

      t1->delay=s1;
      tmp->next=t1;
      *d=s1;
  return;
    }
      
}



void 
RMac::ResetReservationTable()
{
   printf("rmac:ReserReservation: node %d\n",index_);
  for(int i=0;i<TABLE_SIZE;i++){
    reservation_table[i].node_addr=-1;
    reservation_table[i].required_time=0.0;
  }
}



// returned true if there exist retransmission request, false otherwise
bool 
RMac::ProcessRetransmission()
{
  bool status=false;
  int i=0;
  while (i<reservation_table_index)
    {
    if(IsRetransmission(i)) 
       {
	 status=true;
      ScheduleACKData(reservation_table[i].node_addr);
      ClearReservationTable(i); // delete the record from table
      i--;
       }
  i++;
    }
  return status;
}

void 
RMac::ClearReservationTable(int index)
{
  for(int i=index;i<reservation_table_index-1;i++)
    {
    reservation_table[i].node_addr=reservation_table[i+1].node_addr;
    reservation_table[i].block_id=reservation_table[i+1].block_id;
    reservation_table[i].required_time=reservation_table[i+1].required_time;   
    reservation_table[i].interval=reservation_table[i+1].interval;          
    }
  reservation_table_index--;
}


bool 
RMac::IsRetransmission(int reservation_index)
{
  int block=reservation_table[reservation_index].block_id;
  int node_id=reservation_table[reservation_index].node_addr;

  for(int i=0;i<ackdata_table_index;i++)
    if((ackdata_table[i].node_addr==node_id)
       &&(ackdata_table[i].block_num==block))
      {
	printf("RMac: IsRetransmission: node %d received a retx from %d\n",index_,node_id); 
      return true;
      }
  return false;
}



int 
RMac::SelectReservation(){

  /* this one favor the long queue
  printf("rmac:selectReservation: node %d\n",index_);
  int index=-1;
  double dt=-1.0;
  int i=0;

  while(!(reservation_table[i].node_addr==-1))
   {
     printf("rmac:select reservation: node %d, request id is%d \n",index_,reservation_table[i].node_addr);
    if (reservation_table[i].required_time>dt) index=i;
    i++;
   }
  
  return index;

  */

  if(0==reservation_table_index) return -1; // no new reservation request
  // if(skip){
  // if(rand()%2==0) return -1;
    // } 
  //  if(rand()%2==0) return -1;
  int i=0;
  while(!(reservation_table[i].node_addr==-1))
   {
     printf("rmac:select reservation: node %d, request id is%d  i=%d\n",index_,reservation_table[i].node_addr,i);
   
    i++;
   }
  //  printf("rmac:select reservation  node %d i=%d\n",index_,i);
  return rand()%i;
 
}


void 
RMac::ResetMacStatus(){
  printf("node %d timeout at %f!\n\n",index_,NOW);

  if((mac_status==RMAC_WAIT_ACKREV)||(mac_status==RMAC_FORBIDDED)) {
    txbuffer.UnlockBuffer();
    //ResumeTxProcess();
    printf("ResetMacStatus: node %d unlock txbuffer\n",index_); 
  }

  if(mac_status==RMAC_RECV){
  printf("ResetMacStatus: node %d don't receive the data packet at %f\n",index_,NOW);
  Scheduler & s=Scheduler::instance();
  s.cancel(&mac_recv_event);
  Poweroff();  
  }
 mac_status=RMAC_IDLE;
}


void 
RMac::Wakeup(){

  printf("\n. ..WakeUp node %d wake up at %f  and the number of packet is %d...\n\n",
           index_,NOW,txbuffer.num_of_packet);

  // reset the carrier sense
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
 
  n->ResetCarrierSense(); 
  carrier_sense=false;

  /*
  if(n->CarrierSense()){
    printf("Rmac: node %d sense the busy channel at %f\n",index_,NOW); 
   carrier_sense=true;
  }
  else {
     carrier_sense=false;
     n->ResetCarrierId(); 
  }
  */
  //skip=!skip;

  Poweron();
  cycle_start_time=NOW;
 

  /*
  for(int i=0;i<TABLE_SIZE;i++){
    reservation_table[i].node_addr=-1;
  }
 
  reservation_table_index=0;
  */

   // one ack windows:rev
   double ACKwindow=max_short_packet_transmissiontime; 

   printf("\n. ..WakeUp node %d schedule sleep after %f at%f...\n\n",index_,duration_,NOW);
   Scheduler &s=Scheduler::instance();
   s.schedule(&sleep_handler,&sleep_event,duration_);
   s.schedule(&wakeup_handler,&wakeup_event,PeriodInterval_);
  
   // s.schedule(&carrier_sense_handler,&carrier_sense_event,ACKwindow);
   s.schedule(&ackwindow_handler,&ackwindow_event,1.5*ACKwindow);
   return;
}

void 
RMac::ResetReservation()
{

 for(int i=0;i<TABLE_SIZE;i++){
    reservation_table[i].node_addr=-1;
  }
 
  reservation_table_index=0;
  return;
}




void 
RMac::ProcessCarrier()
{ 
  printf("RMac:node %d processes carrier sense at %f...\n",index_,NOW);
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->CarrierId())
{
  printf("RMac:node %d senses carrier!!\n",index_);
 carrier_sense=true;
}
   else carrier_sense=false;
  n->ResetCarrierId();
}


void 
RMac::ProcessListen()
{
  printf("RMac:node %d processes listen  at %f...\n",index_,NOW); 
  if(carrier_sense){
  printf("RMac:node %d senses carriers  at %f...\n",index_,NOW); 
  // InsertBackoff();
   carrier_sense=false;
  }
  ProcessReservedTimeTable();
  
  switch (mac_status){
  case RMAC_IDLE: 
              
                 if (NewData()&&(!collect_rev)){ 
   printf("WakeUp: There is new data in node %d and the number of packet is %d\n", 
                   index_,txbuffer.num_of_packet);
	           mac_status=RMAC_REV;
                   MakeReservation();
                        }
                 break;
  case RMAC_FORBIDDED:
   printf("WakeUp NODE %d is in state RMAC_FORBIDDED\n",index_);
      CancelReservation();
      CancelREVtimeout();
      ClearACKRevLink();
      collect_rev=false;
    break;
  case RMAC_WAIT_ACKREV:  
     collect_rev=false;
    printf("WakeUp NODE %d is in state RMAC_WAIT_ACKREV\n",index_);
    break;
  case RMAC_RECV:  
     collect_rev=false;
    printf("WakeUp NODE %d is in state RMAC_RECV\n",index_);
    break;
 case RMAC_TRANSMISSION:  
     collect_rev=false;
    printf("WakeUp NODE %d is in state RMAC_TRANSMISSION\n",index_);
    break;
 case RMAC_REV:  
     collect_rev=false;
    printf("WakeUp NODE %d is in state RMAC_REV\n",index_);
    break;
  case RMAC_ACKREV:  
     collect_rev=false;
    printf("WakeUp NODE %d is in state RMAC_ACKREV\n",index_);
    break;
   case RMAC_WAIT_ACKDATA: 
     collect_rev=false; 
    printf("WakeUp NODE %d is in state RMAC_WAIT_ACKDATA\n",index_);
    break;
  default: 
   collect_rev=false;
  printf("WakeUp node %d don't expect to be in this state\n",index_);
    break;
  
  }

  if(!collect_rev) ResetReservation();
  return;
}


/*
void 
RMac::ProcessCarrier()
{
  printf("RMac:node %d processes carrier sense at %f...\n",index_,NOW);
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->CarrierSense()) 
     {
      InsertBackoff();
      n->ResetCarrierSense();
     }
}
*/

void 
RMac::ClearACKRevLink(){
 printf("rmac clearACKREV: node %d\n",index_);
  if(!ack_rev_pt) return;
  buffer_cell* t1;
  buffer_cell* t2;
  Event * e1;
   Scheduler &s=Scheduler::instance();
 


  // t1=ack_rev_pt->next;
  t1=ack_rev_pt;
  while (t1){
    t2=t1->next;
    e1=(Event*)t1->packet;
    s.cancel(e1);
    Packet::free(t1->packet);
    delete t1;
    t1=t2;
    ack_rev_pt=t1;
  }
   
}


void 
RMac::ProcessReservedTimeTable(){
   printf("rmac:ProcessReservedtimetable: node %d index=%d\n",index_, reserved_time_table_index);
   int i=0;
   //   double largest_duration=0;
   double elapsed_time=NOW-cycle_start_time;

  while(i<reserved_time_table_index){
    // printf("rmac:ProcessReservedtimetable: node %d index=%d\n",index_, reserved_time_table_index);
    double nst=reserved_time_table[i].start_time-PeriodInterval_-elapsed_time;
    double lt=reserved_time_table[i].duration;
    int addr=reserved_time_table[i].node_addr;
    double  l=CheckLatency(short_latency_table,addr);
    double t1=l-max_short_packet_transmissiontime;
    nst=nst-t1;

    if (nst<0) {
      if((lt+nst)<=0) {
        DeleteRecord(i);
	i--;    
      }
      else 
	{ // nst>=
       mac_status=RMAC_FORBIDDED;
 printf("RMac:ProcessReservedTimeTable: node %d sets reserved time  inetrval=%f and duration is%f\n",
           index_,0.0,(lt+nst));
       reserved_time_table[i].start_time=elapsed_time;
       reserved_time_table[i].duration=lt+nst;
	}
    }// end of nst<0
    else {
      // nst>0
      // if (nst<=PeriodInterval_) mac_status=RMAC_FORBIDDED;
       mac_status=RMAC_FORBIDDED;
       printf("RMac:ProcessReservedTimeTable: node %d sets reserved time  inetrval=%f and duration is%f\n",
                index_,nst,lt);
       reserved_time_table[i].start_time=nst;
       reserved_time_table[i].duration=lt;
    }
    i++;
  }

  if(mac_status==RMAC_FORBIDDED){
    // Scheduler& s=Scheduler::instance();
   // s.cancel(&clear_channel_event);
  
   // s.schedule(&clear_channel_handler,&clear_channel_event,largest_duration); //?? waht's this used for?
  }


  if((reserved_time_table_index==0)&&(mac_status==RMAC_FORBIDDED)) 
          mac_status=RMAC_IDLE; 
}


void 
RMac::DeleteRecord(int index){
 
  for(int i=index;i<reserved_time_table_index;i++)
    {
      reserved_time_table[i].node_addr= reserved_time_table[i+1].node_addr;
      reserved_time_table[i].start_time= reserved_time_table[i+1].start_time;
      reserved_time_table[i].duration= reserved_time_table[i+1].duration;
      reserved_time_table_index--;
    }
 printf("rmac:deleteRecord: node %d the reserved index=%d\n",index_, reserved_time_table_index);
}




bool 
RMac::NewData(){
  return (!txbuffer.IsEmpty());//!! this is correct??think about it
}


void 
RMac::MakeReservation(){
 
 printf("rmac MakeReservation: node %d MakeReservation at %f\n",index_,NOW);

  Packet* p=txbuffer.head();
  hdr_cmn*  cmh = HDR_CMN(p);
  int receiver_addr=cmh->next_hop();
  
   txbuffer.LockBuffer();
   int num=txbuffer.num_of_packet;
   printf("rmac MakeReservation: node %d lock txbuffer \n",index_);

   //   int sender_addr=index_;


   // double lt=-1.0;
     double dt=num*max_large_packet_transmissiontime+(num-1)*SIF_;
     double it=CalculateOffset(dt);
     double t2=DetermineSendingTime(receiver_addr);

      Packet* pkt =Packet::alloc();
      hdr_rmac* revh = HDR_RMAC(pkt);
      cmh = HDR_CMN(pkt);
 

       cmh->next_hop()=receiver_addr;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_RMAC;
      
       revh->ptype=P_REV;
       revh->block_num=num_block;
       revh->pk_num = num_send;
       revh->duration=dt;
       revh->sender_addr=index_;
       revh->interval=it;
       num_send++;

  printf("rmac:Makereservation node %d send a reservation to node %d, duration is %f and offset is %f after %f at %f \n",
          index_, receiver_addr,revh->duration,it,t2,NOW);  

         Scheduler& s=Scheduler::instance();
        s.schedule(&reserve_handler, (Event*) pkt,t2);	
}



/*
In this function, we assume that the sleep time is long enough such that there 
exist interval between two reserved slot (ACK slot) that is long for the max 
data transmission, this function will return the offset of the transmission 
duration to the beginning if this period.
*/

double 
RMac::CalculateOffset(double dt)
{

  int index=-1;
  
  double offset=0.0; 
  double ack_window=max_short_packet_transmissiontime;
  double elapsed_time=NOW-cycle_start_time;
  struct period_record table[TABLE_SIZE];
  
   for(int i=0;i<TABLE_SIZE;i++)
     {
      
       table[i].node_addr=period_table[i].node_addr;
       double l=CheckLatency(short_latency_table, table[i].node_addr)
	 -max_short_packet_transmissiontime;
       double d=period_table[i].difference-l;
       if (d<0) d+=PeriodInterval_;
       table[i].difference=d;

     }

     SortPeriodTable(table);

   
    

     for (int i=0;i<TABLE_SIZE;i++)
{
  printf("node addr is%d and difference is%f\n",table[i].node_addr,table[i].difference);
}
      
     

 // find the first index that can be reached by sending data after elapsed_time 
   int k=0;
  while((-1==index)&&(k<period_table_index))
    {
      
    if(table[k].difference+ack_window>elapsed_time) index=k;     
    k++;
    }
  
  if(-1==index)
 {
    offset=elapsed_time;
    return offset;
 }
 
  //  double it=0;
  int start_index=-1;
  double t0=elapsed_time;

  for(int i=index;i<period_table_index-1;i++)  
    {
      // double t=period_table[i+1].difference-period_table[i].difference;
      double t=table[i].difference-t0;
      
      //t-=ack_window;// avoid the reserved time slot for ackrev 
      if((t>=dt)&&(-1==start_index)) start_index=i;
      t0=table[i].difference+ack_window;
    }
 
  //  printf("Calculate offset start_index=%d and index=%d and elapsedtime=%f t0=%f\n",
  //            start_index,index, elapsed_time,t0);
  

  // we assumw that the listen window is large enough, there must 
  // exist slot larger enough for data transmission

  if(-1==start_index) start_index=period_table_index-1;
  if(start_index==index) return elapsed_time;

  offset=table[start_index-1].difference+ack_window;
  return offset;
}



// determine the sending time, we assume that listen duration is 
// long enough such that we have enough time slots to select.  
// this function randomly selects one of the available slots and 
// converts into time scale

double 
RMac::DetermineSendingTime(int receiver_addr)
{

   struct period_record table[TABLE_SIZE];
  
   for(int i=0;i<TABLE_SIZE;i++)
     {
      
       table[i].node_addr=period_table[i].node_addr;
       double l=CheckLatency(short_latency_table, table[i].node_addr)
	 -max_short_packet_transmissiontime;
       double d=period_table[i].difference-l;
       if (d<0) d+=PeriodInterval_;
       table[i].difference=d;
     }

     SortPeriodTable(table);


     //  double delay=CheckLatency(short_latency_table,receiver_addr)
     //  -max_short_packet_transmissiontime;

  /*
  double dt1=CheckDifference(period_table,receiver_addr);
  double elapsed_time=NOW-cycle_start_time;
  double offset_time=0;
  */
      double elapsed_time=NOW-cycle_start_time;
     double time_slot=max_short_packet_transmissiontime;
     double dt1=CheckDifference(table,receiver_addr);
     double offset_time=dt1+time_slot-elapsed_time;
     while (offset_time<0) offset_time+=PeriodInterval_;        
    
   double dt2=dt1+duration_-time_slot; // end of the time slot 
   double t0=dt1;   //start time of the time slot
   int num_slot=0;
   int n=0;
   int i=0;

   //  while ((period_table[i].difference>dt1)&&(period_table[i].difference<dt2))
   while (table[i].difference<dt2)
    {
      if(table[i].difference>dt1)
	{
      double t=table[i].difference;
      double l=t-t0-time_slot;
      n=(int) floor(l/time_slot);
      num_slot+=n;
      t0=t;
        }
      i++;
    }
  
     double l=dt2-t0-time_slot;
     n=(int) floor(l/time_slot);
     num_slot+=n;
  
     int randIndex=rand()% num_slot;     
    
  i=0;
  int sum=0; 
  double rand_time=0.0;
  //  int sum1=0;
  t0=dt1;
  bool allocated=false;
  // while ((period_table[i].difference>=dt1)&&(period_table[i].difference<dt2))


   
  while (table[i].difference<dt2)
    {
      if(table[i].difference>dt1)
    {
      double t=table[i].difference;
      rand_time=t0-dt1;

      double l=t-t0-time_slot;
      n=(int) floor(l/time_slot);

      t0=t;
      // i++;
      if((sum+n)>randIndex) {
        allocated=true;
        while(sum<=randIndex) 
       {
         rand_time+=time_slot;
         sum++;
       }
      }
	else sum+=n;
    }
      i++;
    }
  
  if(!allocated)
    {
     rand_time=t0-dt1;
     double l=dt2-t0-time_slot;
     n=(int) floor(l/time_slot);

        if((sum+n)>randIndex) {
        allocated=true;
        while(sum<=randIndex) 
       {
         rand_time+=time_slot;
         sum++;
       }
	}
    }
   
  if(!allocated) printf("RMac:DetermineSendingTime,node %d has some probem to allocate sending time \n",
			index_); 

   return offset_time+rand_time;
}


double 
RMac::CheckLatency(latency_record* table,int addr)
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
RMac:: CheckDifference(period_record* table,int addr)
{
  int i=0;
double d=-0.0;
 
 while((table[i].node_addr!=addr)&&(i<TABLE_SIZE))i++;

 if (i==TABLE_SIZE) return d;
 else return table[i].difference;
}


void 
RMac::TxRev(Event* e){

  printf("RMac TxREv node %d at %f\n",index_,NOW);

  Packet* pkt=(Packet*) e;  
  hdr_cmn* cmh=HDR_CMN(pkt);

  
  if(mac_status==RMAC_FORBIDDED) {
 printf("TxREV, node %d is in  RMAC_FORBIDDED, cancel sending REV at %f\n",index_,NOW);
 Packet::free(pkt);
 return;
  }

  // Is it possible??
 if(mac_status==RMAC_WAIT_ACKREV) {
 printf("TxREV, node %d is in  RMAC_ACKREV, cancel sending REV at %f\n",index_,NOW);
 Packet::free(pkt);
 return;
  } 

  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();


   mac_status=RMAC_WAIT_ACKREV;

   printf("TxREV, node %d is in  RMAC_WAIT_ACKREV at %f\n",index_,NOW);

   // double t=Timer*PeriodInterval_;

     double t=5*PeriodInterval_;
    Scheduler& s=Scheduler::instance();
    s.schedule(&timeout_handler,&timeout_event,t);

  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND); 
  cmh->ts_=NOW;

  sendDown(pkt);
  // printf("TxREV, node %d is in sleep at %f\n",index_,NOW);
  status_handler.SetStatus(SLEEP);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==n->TransmissionStatus()){
  
    n->SetTransmissionStatus(SEND);
 
     cmh->ts_=NOW;

     sendDown(pkt);
     // printf("broadcast %d Tx Idle set timer at %f tx is %f\n",node_->nodeid(),NOW,txtime);

     printf("TxREV, node %d is in idle at %f\n",index_,NOW);
  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      
      InterruptRecv(txtime);
      cmh->ts_=NOW;
      sendDown(pkt);
      printf("TxREV, node %d is in recv at %f\n",index_,NOW);
      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
     return;
    }


if (SEND==n->TransmissionStatus())
  {
    printf("rmac: queue send data too fast\n");
    Packet::free(pkt);
      return;
  }

}



void 
RMac::InitPhaseTwo(){

   double delay=Random::uniform()*PhaseTwo_window_;
   next_period=IntervalPhase2Phase3_+PhaseTwo_cycle_*PhaseTwo_window_+delay;

   Scheduler& s=Scheduler::instance();
   s.schedule(&phasethree_handler, &phasethree_event,next_period);
  
   StartPhaseTwo();
    return;  
}

void 
RMac::StartPhaseTwo()
{
   if(PhaseTwo_cycle_)
    {  
     printf("Phase Two: node %d  and cycle:%d\n",index_, PhaseTwo_cycle_);
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




/*
void 
RMac::InitPhaseTwo(){

   double delay=Random::uniform()*PhaseTwo_window_;
   PhaseStatus=PHASETWO;

    cycle_start_time=NOW;
    next_period=IntervalPhase2Phase3_+PhaseTwo_window_+delay;
 
    printf("rmac Initphasetwo: the phasethree of node %d is scheduled at %f\n",index_,NOW+next_period);
    Scheduler& s=Scheduler::instance();
    s.schedule(&phasetwo_handler, &phasetwo_event,delay);
    return;
}

*/

Packet*  
RMac::GenerateSYN(){

       Packet* pkt =Packet::alloc();
       hdr_rmac* synh = HDR_RMAC(pkt); 
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
RMac::SendSYN(){

       Packet* pkt =Packet::alloc();
       hdr_rmac* synh = HDR_RMAC(pkt); 
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

	printf("rmac SendSYN:node(%d) send SYN packet at %f\n", synh->sender_addr,NOW);
      TxND(pkt, PhaseTwo_window_);  
}


void 
RMac::InitND(double t1,double t2, double t3)
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
RMac::SendND(int pkt_size)
{

      Packet* pkt =Packet:: alloc();
      hdr_rmac* ndh = HDR_RMAC(pkt);
    
      hdr_cmn*  cmh = HDR_CMN(pkt);
      
   
     // additional 2*8 denotes the size of type,next-hop of the packet and 
     // timestamp
  
      //       cmh->size()=sizeof(hdr_nd)+3*8;
      //  printf("old size is %d\n",cmh->size());
        cmh->size()=pkt_size;

       cmh->next_hop()=MAC_BROADCAST;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_RMAC;
      
       
     
      ndh->ptype=P_ND;
      ndh->pk_num = num_send;
      ndh->sender_addr= node_->address();

      num_send++;

      // iph->src_.addr_=node_->address();
      // iph->dst_.addr_=node_->address();
      //iph->dst_.port_=255;     

 printf("rmac SendND:node(%d) send ND type is %d at %f\n", ndh->sender_addr,cmh->ptype_, NOW);
      TxND(pkt, ND_window_);  
}





void 
RMac::SendShortAckND()
{
    printf("rmac:SendShortND: node %d\n",index_);
  if (arrival_table_index==0) return;// not ND received


  while(arrival_table_index>0){ 
      Packet* pkt = Packet::alloc();
      hdr_rmac* ackndh = HDR_RMAC(pkt);
    
      hdr_cmn*  cmh = HDR_CMN(pkt);
      
      ackndh->ptype=P_SACKND;
      ackndh->pk_num = num_send;
      ackndh->sender_addr=node_->address();
      num_send++;

      cmh->ptype_=PT_RMAC;
        
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

/*
void 
RMac::TxND(Packet* pkt, double window)
{
  //  printf("RMac TxND node %d\n",index_); 
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_rmac* synh = HDR_RMAC(pkt); 
  
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
 
    
    Scheduler & s=Scheduler::instance();
    s.schedule(&phasethree_handler,&phasethree_event,synh->interval);
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

  
    Scheduler & s=Scheduler::instance();
    s.schedule(&phasethree_handler,&phasethree_event,synh->interval);
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
          printf("Rmac:NODE %d backoff:no time left \n",index_);
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

*/



void 
RMac::TxND(Packet* pkt, double window)
{
   printf("RMac TxND node %d\n",index_); 
  hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_rmac* synh = HDR_RMAC(pkt); 
  
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
          printf("Rmac:backoff: node %d no time left, just drop the packet \n",index_);
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
RMac::ProcessACKRevPacket(Packet* pkt)
{
    printf("RMac:ProcessACKRevPacket: node %d at time %f\n",index_,NOW);

    hdr_rmac* ackrevh=HDR_RMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);
    int dst=cmh->next_hop();
    int ptype=ackrevh->ptype; 

    // since the node clearly receives the ACKrev, does not need to backoff, 
    //  therefore, resets the carrier sense;   
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
   

  
    if(ptype!=P_ACKREV) 
      {
	printf("processACKRevPacket:node %d receives no ACK_REV packet at %f\n",index_,NOW);
        return;
      }
    if(cmh->error()){
	printf("processACKRevPacket:node %d senses carrier in ackwindow at  %f\n",index_,NOW);
        carrier_sense=true;
        return;
    }
    n->ResetCarrierId();
    carrier_sense=false;


    int receiver_addr=ackrevh->receiver_addr;
    double dt=ackrevh->duration-ackrevh->st;
    double st=ackrevh->st;
    int sender_addr=ackrevh->sender_addr;
   
    double  l=CheckLatency(short_latency_table,sender_addr);
    double  it=st-l;
   
    double elapsedtime=NOW-cycle_start_time; 
 
    if(elapsedtime>1.1*max_short_packet_transmissiontime) 
      {
	printf("RMac:processACKRev:node %d this is out of my ackrev window...\n",index_);
	Packet::free(pkt);
        return;
      }


    if((dst!=index_)&&(index_==receiver_addr))
 {
  
   printf("RMac:processACKrev: node %d  receives a ackrev  targetd at %d and receiver is %d\n",
             index_,dst,receiver_addr);
   Packet::free(pkt);
 return;
 }
    

     printf("rmac:ProcessAckRevPacket:node %d I get the ACK REV packet offset is %f and duration=%f at %f\n",
            index_, it,dt,NOW);     
    //printf("rmac:ProcessAckRevPacket: node %d I get the ACK REV packet interval is %f \n",index_, it);

    if(it<0) 
     printf("rmac:ProcessAckRevPacket: the notification is too short\n");
    
    Packet::free(pkt);


    if(receiver_addr!=index_) {
      // This ackrev is not for me
     printf("rmac:ProcessAckRevPacket: node %d this ACKREV is not for me\n",index_);
     double poffset=PeriodInterval_+elapsedtime+it-l+max_short_packet_transmissiontime;
     InsertReservedTimeTable(receiver_addr,poffset,dt);
    }
    else {
    // This ackrev is for me
      if(mac_status!=RMAC_WAIT_ACKREV) {
     printf("rmac:ProcessAckRevPacket:status change, I quit this chance\n");  
     return;
      }
       
        printf("rmac:ProcessAckRevPacket: node %d this ACKREV is for me\n",index_);         
        num_data=0;
     
	double  it1=it-l+max_short_packet_transmissiontime;
        mac_status=RMAC_TRANSMISSION;
        Scheduler& s=Scheduler::instance();
        printf("rmac:ProcessAckRevPacket: node %d schedule Txdata after %f at time %f, latency is %f\n",
                 index_,it1,NOW,l);
    
        s.cancel(&timeout_event);// cancel the timer of rev
        transmission_handler.receiver=sender_addr;      
        s.schedule(&transmission_handler,&transmission_event,it1);
    } 
    return;
}


void 
RMac::ClearTxBuffer(){
 printf("RMac: ClearTxBuffer: node %d clear its buffer\n",index_);

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
 
  /*
  printf("ClearTxBuffer: show the queue****************after txbufferdelete\n");
      t->showqueue();  
  */

  printf("txbuffer is cleared, there are %d packets left\n",txbuffer.num_of_packet);

  return; 
}



void 
RMac::ProcessACKDataPacket(Packet* pkt)
{
printf("rmac:ProcessACKDATAPacket: node %d process ACKDATA packets at time %f duration_=%f\n",index_,NOW,duration_);
//  hdr_rmac* ackrevh=HDR_RMAC(pkt);
//    hdr_cmn* cmh=HDR_CMN(pkt);
 
    Scheduler& s=Scheduler::instance();
printf("rmac:ProcessAckData: node %d cancel timeout dutation=%f\n",index_,duration_);    
      s.cancel(&timeout_event);// cancel the timer of data
  

  
    
    for (int i=0;i<MAXIMUM_BUFFER;i++)bit_map[i]=0;

    memcpy(bit_map, pkt->accessdata(),sizeof(bit_map));
  
  printf("rmac:ProcessACKDATAPacket: node %d received the bitmap is:\n",index_);
  

  for (int i=0;i<MAXIMUM_BUFFER;i++) printf("bitmap[%d]=%d ",i,bit_map[i]);
  printf("\n");

  printf("txbuffer will be cleared, there are %d packets in queue and duration=%f\n",txbuffer.num_of_packet,duration_);

      Packet::free(pkt);   


 /*
!!!
This part should consider the retransmission state, in this implementation, we don't consider the packets loss
caused by channel errors, therefore, we just ignore it, it should be added later. 
*/

    ClearTxBuffer(); 
    num_block++;
    txbuffer.UnlockBuffer();
    mac_status=RMAC_IDLE;
printf("rmac:ProcessACKDATAPacket: node %d unlock txbuffer duration_=%f\n",index_,duration_);
   ResumeTxProcess();
    return;
}


void 
RMac::ProcessRevPacket(Packet* pkt)
{
    printf("RMac:ProcessRevPacket: node %d is processing rev\n",index_);
    hdr_rmac* revh=HDR_RMAC(pkt);
    // hdr_cmn* cmh=HDR_CMN(pkt);

    int sender_addr=revh->sender_addr;
    double dt=revh->duration;
    double interval=revh->interval;
    int block=revh->block_num;

    Packet::free(pkt);
   
   
    if (mac_status==RMAC_IDLE)
    {     
    if(reservation_table_index <TABLE_SIZE){  
    reservation_table[reservation_table_index].node_addr=sender_addr;
    reservation_table[reservation_table_index].required_time=dt;   
    reservation_table[reservation_table_index].interval=interval;      
    reservation_table[reservation_table_index].block_id=block;
    reservation_table_index++;
    }
    else {
    printf("ramc:ProcessRevPacket: too many reservation, drop the packet\n");
    }
    }
    else {
   printf("rmac:ProcessRevPacket: I am not in idle state, drop this packet\n"); 
    }
      return;
}




void 
RMac::ProcessNDPacket(Packet* pkt)
{
  // printf("rmac:ProcessNDPacket: node %d\n",index_);
    hdr_rmac* ndh=HDR_RMAC(pkt);
    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=ndh->sender_addr;
    double time=NOW;
    if(arrival_table_index>=TABLE_SIZE){ 
      printf("rmac:ProcessNDPacket:arrival_table is full\n");
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
RMac::ProcessDataPacket(Packet* pkt)
{
  printf("rmac:ProcessDataPacket: node %d get data packet\n",index_);

  //  hdr_uwvb* vbh=HDR_UWVB(pkt);
  hdr_rmac* rmach=HDR_RMAC(pkt);

  int  data_sender=rmach->sender_addr;
  int  bnum=rmach->block_num;
  int num=rmach->data_num;
 

      recv_busy=true;
      Scheduler& s=Scheduler::instance();
      s.cancel(&timeout_event);

      //     MarkBitMap(num);
      UpdateACKDataTable(data_sender,bnum,num);
      
      uptarget_->recv(pkt,this);
      return;
}

/*
void 
RMac::MarkBitMap(int num){
  if(num<MAXIMUM_BUFFER) bit_map[num]=1;
}
*/

void 
RMac::UpdateACKDataTable(int data_sender,int bnum,int num)
{
  int index=-1;
  for (int i=0;i<ackdata_table_index;i++) 
    if(ackdata_table[i].node_addr==data_sender) index=i;

  if(index==-1){
    ackdata_table[ackdata_table_index].node_addr=data_sender;
    ackdata_table[ackdata_table_index].block_num=bnum;
    ackdata_table[ackdata_table_index].bitmap[num]=1;
    ackdata_table_index++;
  }
  else 
    {
    ackdata_table[index].node_addr=data_sender;
    ackdata_table[index].block_num=bnum;
    ackdata_table[index].bitmap[num]=1;
    }
}


// this program need to be modified to handle the 
// retransmission 

void 
RMac::ScheduleACKData(int data_sender)
{
  printf("rmac Schdeule ACKDATA: node %d at %f\n",index_,NOW);
  
  if(data_sender<0) 
  {
    printf("Ramc:ScheduleACKData: invalid sender address\n");
    return; 
  }

    Packet* pkt=Packet::alloc(sizeof(bit_map)); 
    hdr_cmn*  cmh = HDR_CMN(pkt);
    hdr_rmac* revh = HDR_RMAC(pkt);
 
    CopyBitmap(pkt, data_sender);
 
       cmh->next_hop()=data_sender;
       cmh->direction()=hdr_cmn::DOWN; 
       cmh->addr_type()=NS_AF_ILINK;
       cmh->ptype_=PT_RMAC;
       cmh->size()=short_packet_size_;     
     
       revh->ptype=P_ACKDATA;
       revh->pk_num = num_send;
       revh->sender_addr=index_;
       num_send++;
 
       double t1=DetermineSendingTime(data_sender);
         
      printf("rmac Schdeule ACKDATA: node %d  schedule ackdata after %f at %f\n",index_,t1,NOW);
      Scheduler& s=Scheduler::instance();
      s.schedule(&ackdata_handler, (Event*) pkt,t1);
}

void 
RMac::CopyBitmap(Packet* pkt,int data_sender)
{
  int index=-1;
  for (int i=0;i<ackdata_table_index;i++)
    if(ackdata_table[i].node_addr==data_sender) index=i;
      
 
  if(index!=-1) 
    memcpy(pkt->accessdata(),ackdata_table[index].bitmap,sizeof(bit_map));
  else printf("CopyBitMap: Node %d: I can't find the entry of the sender %d \n",index_, data_sender);
}

bool
RMac::IsSafe()
{
  bool safe_status=true;
  if(RMAC_FORBIDDED!=mac_status) return safe_status;
  double start_time=NOW-cycle_start_time;
  double ending_time=start_time+max_short_packet_transmissiontime;
  for(int i=0;i<reserved_time_table_index;i++)
    {
      double t1=reserved_time_table[i].start_time;
      double d1=reserved_time_table[i].duration;
      if((ending_time>t1)&&((t1+d1)>start_time)) safe_status=false;
    }
  return safe_status;
}


void  
RMac::TxACKData(Event* e)
{
 printf("RMac TxACKData node %d at %f\n",index_,NOW);

  Packet* pkt=(Packet*) e;
  hdr_cmn* cmh=HDR_CMN(pkt);
 
  assert(initialized());
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

  if(!IsSafe()) {
    // It is not safe to send this ACKData  since it collides with reserved time  slot
    Packet::free(pkt);
    return;
  }
 
  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();


   mac_status=RMAC_IDLE;

  printf("TxACKData, node %d is in  RMAC_IDLE at %f\n",index_,NOW);

 
  if(SLEEP==n->TransmissionStatus()) {
  Poweron();
  n->SetTransmissionStatus(SEND); 
  cmh->ts_=NOW;

  sendDown(pkt);

  printf("RMac TxACKData node %d at %f\n",index_,NOW);
  status_handler.SetStatus(SLEEP);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(IDLE==n->TransmissionStatus()){
  
    n->SetTransmissionStatus(SEND);
 
     cmh->ts_=NOW;

     sendDown(pkt);
  printf("RMac TxACKData node %d at %f\n",index_,NOW);
  status_handler.SetStatus(IDLE);
  Scheduler& s=Scheduler::instance();
  s.schedule(&status_handler,&status_event,txtime);
  return;
  }

  if(RECV==n->TransmissionStatus())
    {
      
      InterruptRecv(txtime);
      cmh->ts_=NOW;
      sendDown(pkt);
printf("RMac TxACKData node %d at %f\n",index_,NOW);
      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
     return;
    }


if (SEND==n->TransmissionStatus())
  {
    printf("rmac: node%d send data too fast\n",index_);
    Packet::free(pkt);
      return;
  }

}


void 
RMac::ProcessShortACKNDPacket(Packet* pkt)
{
  // printf("rmac:ProcessshortACKNDPacket: node %d\n",index_);
    hdr_rmac* ackndh=HDR_RMAC(pkt);
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
      printf("rmac:ProcessNDPacket:arrival_table is full\n");
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
RMac::ProcessSYN(Packet* pkt)
{
  // printf("rmac:ProcessSYN: node %d\n",index_);
    hdr_rmac* synh=HDR_RMAC(pkt);
    //    hdr_cmn* cmh=HDR_CMN(pkt);

    int  sender=synh->sender_addr;
    double interval=synh->interval;
    double tduration=synh->duration;
      Packet::free(pkt);


    double t1=-1.0;
 for (int i=0;i<TABLE_SIZE;i++)
 if (short_latency_table[i].node_addr==sender)
     t1=short_latency_table[i].latency;

 if(t1==-1.0) {
 printf("Rmac:ProcessSYN: I receive a SYN from unknown neighbor %d\n",sender);
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
 this program is used to handle the received packet, 
it should be virtual function, different class may have 
different versions.
*/


void 
RMac::RecvProcess(Packet* pkt){
   
   hdr_cmn* cmh=HDR_CMN(pkt);
   hdr_rmac* cmh1=HDR_RMAC(pkt);
   int dst=cmh->next_hop();
   int ptype=cmh1->ptype; 
   double elapsed_time=NOW-cycle_start_time;
   double ack_window=max_short_packet_transmissiontime+theta;
    
   printf("rmac:node %d  gets a broadcast packet at  %f\n",index_,NOW);
   
   // check if it is in the revack window
   if(elapsed_time<=ack_window) 
      {
     ProcessACKRevPacket(pkt);
     return;
      }

   if (cmh->error()) 
     {
     printf("rmac:node %d  gets a corrupted packet at  %f\n",index_,NOW);
     Packet::free(pkt);
     return;
     }

  if(dst==MAC_BROADCAST){
    printf("rmac:node %d  gets a broadcast packet at  %f and type is %d\n",index_,NOW, cmh->ptype_);
    if (ptype==P_ND) ProcessNDPacket(pkt); //this is ND packet

        // this is ACK_ND packet  
    if (ptype==P_SYN) ProcessSYN(pkt);
   
    // uptarget_->recv(pkt, this);
    return;
  }

  //  if (ptype==P_ACKREV) {ProcessACKRevPacket(pkt); return;}
   if(dst==index_){
 printf("rmac:node %d  gets a packet at  %f and type is %d and %d\n",index_,NOW, cmh->ptype_,cmh1->ptype);
   if (ptype==P_SACKND) ProcessShortACKNDPacket(pkt); 
   if (ptype==P_REV) ProcessRevPacket(pkt); 
   if (ptype==P_DATA) ProcessDataPacket(pkt);
  // if (ptype==P_ACKREV) ProcessACKRevPacket(pkt);
   if(ptype==P_ACKDATA) ProcessACKDataPacket(pkt);
     // printf("underwaterbroadcastmac:this is my packet \n");
     //  uptarget_->recv(pkt, this);
    return;
}
    printf("rmac: node%d this is neither broadcast nor my packet %d, just drop it at %f\n",index_,dst, NOW);
   Packet::free(pkt);
   return;
}


void 
RMac::TxData(int receiver)
{
  printf("RMac:node %d TxData at time %f\n",index_,NOW);

  if (txbuffer.IsEmpty()) 
{
printf("Rmac:TxData: what a hell! I don't have data to send\n");
return;
}


  if(mac_status!=RMAC_TRANSMISSION) {
 printf("Rmac:TxData: node %d is not in transmission state\n",index_);
      return;
  }

   UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if(n->TransmissionStatus()==SLEEP) Poweron();

     mac_status=RMAC_TRANSMISSION;

    Packet* pkt=txbuffer.next();
   
    hdr_cmn* cmh=hdr_cmn::access(pkt);
    hdr_rmac* datah =hdr_rmac::access(pkt);
    hdr_uwvb* hdr2=hdr_uwvb::access(pkt);

    // printf("RMac:node %d TxData at time %f data type is %d offset is%d and size is %d and offset is %d and size is%d uwvb offset is %d and size is %d\n",index_,NOW,hdr2->mess_type,cmh,sizeof(hdr_cmn),datah,sizeof(hdr_rmac),hdr2,sizeof(hdr_uwvb));
         datah->ptype=P_DATA;  

         datah->sender_addr=index_;
   
         datah->pk_num=num_send;
         datah->data_num=num_data;
         datah->block_num=num_block;

         num_send++;
         num_data++;
  
          cmh->size()=large_packet_size_;

            cmh->next_hop()=receiver;
	   
            cmh->direction()=hdr_cmn::DOWN; 
            cmh->addr_type()=NS_AF_ILINK;
            cmh->ptype_=PT_RMAC; 

	    

  hdr_cmn::access(pkt)->txtime()=(cmh->size()*encoding_efficiency_+
                                   PhyOverhead_)/bit_rate_;

  double txtime=hdr_cmn::access(pkt)->txtime();

 printf("RMac:node %d TxData at time %f data type is %d packet data_num=%d class data_num=%d \n",index_,NOW,hdr2->mess_type,datah->data_num,num_data);
  TransmissionStatus status=n->TransmissionStatus();

 

 if(IDLE==status)
 {
  n->SetTransmissionStatus(SEND); 
        sendDown(pkt);
 printf("RMac:node %d TxData at %f\n ",index_,NOW);
        status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);  
 }

 if(RECV==status)
    {
      InterruptRecv(txtime);
      
      sendDown(pkt);
 printf("RMac:node %d TxData at %f\n ",index_,NOW);
      status_handler.SetStatus(IDLE);
      Scheduler& s=Scheduler::instance();
      s.schedule(&status_handler,&status_event,txtime);
    }

 if (SEND==status)
    { 
    printf("rmac:Txdata: queue send data too fast\n");
    Packet::free(pkt);
    }

  
  if (txbuffer.IsEnd()) {
   printf("rmac:Txdata: node %d is in state MAC_WAIT_ACKDATA\n",index_);
   mac_status=RMAC_WAIT_ACKDATA; 
  
   //  double txtime=Timer*PeriodInterval_;  
       double txtime=3*PeriodInterval_; 
   printf("RMac:node %d TxData at %f\n ",index_,NOW);
   Scheduler& s=Scheduler::instance();
   s.schedule(&timeout_handler,&timeout_event,txtime);
  
   //  num_block++;
   Poweroff();
  }
  else {
  double it=SIF_+txtime;   
 
 printf("rmac:Txdata: node%d schedule  next data packet , interval=%f at time%f\n",index_,it,NOW);
  Scheduler& s=Scheduler::instance();
  s.schedule(&transmission_handler,&transmission_event,it);
  }

}


void 
RMac::ResumeTxProcess(){

  printf("rmac:ResumeTxProcess: node %d at %f\n",index_,NOW);

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
RMac::TxProcess(Packet* pkt){

  hdr_uwvb* hdr=HDR_UWVB(pkt);

  printf("rmac:TxProcess: node %d type is %d\n",index_,hdr->mess_type);
 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
  if (n->setHopStatus){
   hdr_cmn* cmh=HDR_CMN(pkt);
   cmh->next_hop()=n->next_hop;
   cmh->error()=0;//set off the error flag

   // printf("rmac:TxProcess: node %d set next hop to %d\n",index_,cmh->next_hop());
  }
 

  txbuffer.AddNewPacket(pkt);
  printf("rmac:TxProcess: node %d put new data packets in txbuffer\n",index_);
  if(!txbuffer.IsFull()) 
  if(callback_) callback_->handle(&status_event);
  return;
}


 
void 
RMac::StatusProcess(Event* p, TransmissionStatus  state)
{

   printf("RMac StatusProcess node %d \n",index_);
 UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
 
 if(SLEEP==n->TransmissionStatus()) return;

    n->SetTransmissionStatus(state);

 return;
}




int
RMac::command(int argc, const char*const* argv)
{


     if(argc == 3) {
       //TclObject *obj;
                 if (strcmp(argv[1], "node_on") == 0) {
		   Node* n1=(Node*) TclObject::lookup(argv[2]);
		   if (!n1) return TCL_ERROR;
		   node_ =n1; 
		   return TCL_OK;
		 }

		 /*
               if (strcmp(argv[1], "set_next_hop") == 0) {
		 setHopStatus=1;
		 next_hop=atoi(argv[2]);
		   return TCL_OK;
	       }
	      */
     }

	return UnderwaterMac::command(argc, argv);
}
