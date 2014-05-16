#ifndef ns_rmac_h
#define ns_rmac_h

#include "underwatermac.h"
#include "config.h"
#include "packet.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "uwbuffer.h"

#define TABLE_SIZE 20 // the size of delay table
#define MAXIMUMBACKOFF 4 // the maximum times of backoffs
#define BACKOFF 1 //deleted later, used by TxProcess



#define UW_ND 1
#define UW_ACK_ND 2


#define PHASEONE 1
#define PHASETWO 2
#define PHASETHREE 3

enum RmacPacketType{
  P_DATA,
  P_REV,
  P_ACKREV,
  P_ND,
  P_SACKND,
  P_ACKDATA,
  P_SYN
};


enum MAC_STATUS{
  RMAC_IDLE,
  RMAC_REV,
  RMAC_ACKREV,
  RMAC_RECV,
  RMAC_WAIT_ACKREV,
  RMAC_WAIT_ACKDATA,
  RMAC_FORBIDDED,
  RMAC_TRANSMISSION,
};




struct forbidden_time_record{
  int node_addr;// the address of the node
  double start_time;// the time to receive the ND packet from the node
  double next_time; // the  start time of next availabe period 
  double duration; // the sending time of ND in local clock
 
};

struct time_record{
  int node_addr;// the address of the node
  double arrival_time;// the time to receive the ND packet from the node
  double sending_time; // the sending time of ND in local clock
};


struct reservation_record{
  int  node_addr;    // the address of the node 
  double required_time;    // the duration of required time slot 
  double interval;
  int block_id;
//the  interval between the start time of the transmission and the 
// start time of the cycle
};



struct ackdata_record{
  int node_addr;// the address of the sender
  int bitmap[MAXIMUM_BUFFER];  // the pointer to the bitmap
  int block_num;// the block id 
};


struct period_record{
  int node_addr;// the address of the node
  double difference;// the difference with my period
  double duration; // duration of duty cycle
  double last_update_time; // the time last updated
};


struct latency_record{
  int node_addr;      // the address of the node
  double latency;    // the propagation latency with that node
  double sumLatency;// the sum of latency
  int num;         // number of ACKND packets 
  double last_update_time; // the time of last update
};

struct hdr_rmac{
         int ptype;     //packet type
        int pk_num;    // sequence number
        int data_num; 
        int block_num; // the block num, in real world, one bit is enough
        int sender_addr;  //original sender' address
   	double st;           // Timestamp when pkt is generated.
        int receiver_addr;
        double duration;
        double interval;
        double arrival_time;
         double ts;	

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_rmac* access(const Packet*  p) {
		return (hdr_rmac*) p->access(offset_);
	}
};

/*
struct hdr_rmac_data{
        double duration;  // there is a bug, put this to fix it
          int pk_num;    // sequence number
         int sender_addr;     //original sender' address
       // int receiver_addr;  // the address of the intended receiver
           
	
	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_rmac_data* access(const Packet*  p) {
		return (hdr_rmac_data*) p->access(offset_);
	}
};




struct hdr_rev{
        // unsigned int type;     //packet type
         int pk_num;    // sequence number
        int sender_addr;     //original sender' address
  // int receiver_addr;  // the address of the intended receiver
        double duration;           // time interval for reservation 
	
	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_rev* access(const Packet*  p) {
		return (hdr_rev*) p->access(offset_);
	}
};


struct hdr_ack_rev{
        // unsigned int type;     //packet type
        int pk_num;    // sequence number
        int sender_addr;     //original sender' address
        int receiver_addr;  // the address of the intended receiver
        double duration;           // time interval for reservation 
        double st;                 // start time of reservation 	

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_ack_rev* access(const Packet*  p) {
		return (hdr_ack_rev*) p->access(offset_);
	}
};




struct hdr_syn{
        //  unsigned int type;     //packet type
         int pk_num;    // sequence number
        int sender_addr;  //original sender' address
        double interval;    // interval to the begining of periodic operation
        double duration; // duration of duty cycle;

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_syn* access(const Packet*  p) {
		return (hdr_syn*) p->access(offset_);
	}
};

struct hdr_ack_nd{
  //	unsigned int type;
         int pk_num;    // sequence number
        int sender_addr;  //original sender' address
         double ts;// sending time of the ND in sender's clock
  double arrival_time; //arrival time of ND in  the receiver's clock
       //  struct  time_record table[TABLE_SIZE]; // delay table
	
	static int offset_;
  	inline static int& offset() { return offset_; }

  	inline static hdr_ack_nd* access(const Packet*  p) {
		return (hdr_ack_nd*) p->access(offset_);
	}
};

*/



class RMac;

class ACKREVHandler: public Handler{
 public:
  ACKREVHandler(RMac* p): mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};

class ClearChannelHandler: public Handler{
 public:
  ClearChannelHandler(RMac* p): mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;

};




class MACRECVHandler: public Handler{
 public:
  MACRECVHandler(RMac* p): mac_(p),duration(0),status(0),data_sender(0){};
  void handle(Event*);
  int data_sender;
  double duration;// duration of RECV 
  int status;// 0 is open the recv window and 1 is close window
 private:
  RMac* mac_;
};



class TimeoutHandler: public Handler{
 public:
  TimeoutHandler(RMac* p): mac_(p){};
  void handle(Event*);

 private:
  RMac* mac_;
};



class SleepHandler: public Handler{
 public:
  SleepHandler(RMac* p): mac_(p){};
  void handle(Event*);

 private:
  RMac* mac_;
};

class WakeupHandler: public Handler{
 public:
  // WakeupHandler(RMac*);
  WakeupHandler(RMac* p): mac_(p){};
  void handle(Event*);

 private:
  RMac* mac_;
};


class ACKDATAHandler: public Handler{
 public:
  // ACKDATAHandler(RMac*);
  ACKDATAHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};


class ACKWindowHandler: public Handler{
 public:
  //  ACKWindowHandler(RMac*);
  ACKWindowHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};


class CarrierSenseHandler: public Handler{
 public:
  // CarrierSenseHandler(RMac*);
  CarrierSenseHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};


class ReserveHandler: public Handler{
 public:
  // ReserveHandler(RMac*);
  ReserveHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};


class TransmissionHandler: public Handler{
 public:
  //TransmissionHandler(RMac*);
  TransmissionHandler(RMac* p):mac_(p), receiver(0){};
  void handle(Event*);
   int receiver;
 private: 
  RMac* mac_;
};


class NDBackoffHandler: public Handler{
 public:
  //  NDBackoffHandler(RMac*);
  NDBackoffHandler(RMac* p):mac_(p),window_(0),counter_(0){};
  void handle(Event*);
  void clear();
  double window_;

 private:
  int counter_;
  RMac* mac_;
};




class ShortNDHandler: public Handler{
 public:
  //ShortNDHandler(RMac*);
  ShortNDHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};

class SYNHandler: public Handler{
 public:
  SYNHandler(RMac* p): mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};

class NDStatusHandler: public Handler{
 public:
  // NDStatusHandler(RMac*);
  NDStatusHandler(RMac* p):mac_(p),status_(SLEEP){}
  void SetStatus(TransmissionStatus);
  void handle(Event*);
 private:
  TransmissionStatus status_;     
  RMac* mac_;
};




class ShortAckNDWindowHandler: public Handler{
 public:
  //ShortAckNDWindowHandler(RMac*);
  ShortAckNDWindowHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};


class ACKNDHandler: public Handler{
 public:
  // ACKNDHandler(RMac*);
  ACKNDHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};



class PhaseOneHandler: public Handler{
 public:
  // PhaseOneHandler(RMac*);
  PhaseOneHandler(RMac* p):mac_(p){}
  void handle(Event*);
 private:
  RMac* mac_;
};

class PhaseTwoHandler: public Handler{
 public:
  // PhaseTwoHandler(RMac*);
  PhaseTwoHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};

class PhaseThreeHandler: public Handler{
 public:
  //PhaseThreeHandler(RMac*);
  PhaseThreeHandler(RMac* p):mac_(p){};
  void handle(Event*);
 private:
  RMac* mac_;
};



class RMac: public UnderwaterMac {
   
public:
        RMac();
     
       	int  command(int argc, const char*const* argv);
        double  ND_window_;// the window to send ND
        double  ACKND_window_;// the winddow to send ACK_ND
        double  PhaseOne_window_; // the time for latency detection
        double  PhaseTwo_window_; // the time for SYN announcement
        double SIF_;// interval between two successive data packets
        double ACKRevInterval_;
        double PhaseTwo_interval_;// interval between windows of phase two
        int PhyOverhead_;// the overhead caused by phy layer
        int arrival_table_index;
	//        int large_latency_table_index;
        int short_latency_table_index;
        int period_table_index;
        int reservation_table_index;
        int reserved_time_table_index;
        int ackdata_table_index;
        int Timer;// number of periodIntervals to backoff
        //int data_sender; // address of the data sender

         // in real world, this is supposed to use bit map
	// to indicate the lost of packet
	  int bit_map[MAXIMUM_BUFFER];
        
	// these two variables are used to set next hop 
	// SetHopStatus=1 then set next hop using next_hop
	// int setHopStatus;
        //int next_hop;
         
        bool carrier_sense; // sense the channel in the ackrev time slot
	//   bool skip;
        int num_send; 
        int num_data;
        int num_block;

        int large_packet_size_;
        int short_packet_size_;   
        double duration_; // duration of duty cycle     
        double IntervalPhase2Phase3_;
        double next_period;//the start_time of next duty cycle
        double PeriodInterval_;
        double max_short_packet_transmissiontime;
        double max_large_packet_transmissiontime;
        double transmission_time_error_; //guardian time
        double theta; //used for precision control

        int PhaseOne_cycle_; // number of cycles in phase one
        int PhaseTwo_cycle_; // number of cycles in phase two 

        int PhaseStatus;  
       // used by the receiver to test if the data packet arrives on time   
        bool recv_busy; 
     // used to indicate the status of collecting reservations
        bool collect_rev;
      
        enum MAC_STATUS mac_status;

	double cycle_start_time; // the begining time of this cycle;  
        TransmissionBuffer txbuffer;
         struct  time_record arrival_table[TABLE_SIZE]; 
         struct forbidden_time_record reserved_time_table[TABLE_SIZE];
         //struct latency_record large_latency_table[TABLE_SIZE];            
         struct latency_record short_latency_table[TABLE_SIZE];    
         struct period_record  period_table[TABLE_SIZE];
         struct reservation_record reservation_table[TABLE_SIZE];
         struct reservation_record next_available_table[TABLE_SIZE];
         struct ackdata_record ackdata_table[TABLE_SIZE];
         struct buffer_cell * ack_rev_pt;// pointer to the link of ack_rev

 void InitPhaseOne(double/*ND window*/,double/*ack_nd window*/,double/* phaseOne window*/); 
    
 void InitPhaseTwo(); 
 void InitPhaseThree();
 void StartPhaseTwo();

     void InitND(double/*ND window*/,double/*ack_nd window*/,double/* phase One window*/);// to detect latency 

     void SendND(int);
     void TxND(Packet*, double);
     // void ProcessPacket(Packet*);   
     void ProcessNDPacket(Packet*);   
     void ProcessDataPacket(Packet*);   
     // void ProcessLargeACKNDPacket(Packet*);   
     void ProcessShortACKNDPacket(Packet*);   
     void ProcessSYN(Packet*);
     void ProcessSleep();
     void ProcessRevPacket(Packet*);
     void ProcessACKRevPacket(Packet*);
     void ProcessACKDataPacket(Packet*);
     void ProcessReservedTimeTable();
     bool ProcessRetransmission();
     void ProcessListen();
     void ProcessCarrier();

     void ResetReservation();
     void Wakeup();
     void TxRev(Event*);
     void TxACKRev(Packet*);
     void TxACKData(Event*);
     void ResetMacStatus();
     void ScheduleACKREV(int,double,double);
     void ScheduleACKData(int);
     //void SendLargeAckND();
     void SendShortAckND();
     void StatusProcess(Event*,TransmissionStatus);
     void SendSYN();
     void MakeReservation();
     void ArrangeReservation();
     void ResetReservationTable();   
     void SetStartTime(buffer_cell*, double,double);
     void ClearTxBuffer();
     void InsertReservedTimeTable(int,double,double);   
     void SortPeriodTable(struct period_record*);
     void InsertBackoff();
     void CopyBitmap(Packet*,int);
     void UpdateACKDataTable(int,int,int);
     void ClearReservationTable(int);
     Packet*  GenerateACKRev(int,/* receiver*/int /* intended receiver's addr*/,
                         double/* time duration*/);
     Packet* GenerateSYN();
     int  SelectReservation();
     double CalculateOffset(double);
     double CalculateACKRevTime(double,double,double,double);
     double CalculateACKRevTime(double,double,double);
     double DetermineSendingTime(int);
     double CheckLatency(latency_record*,int);
     double CheckDifference(period_record*,int);
  
   
     bool IsRetransmission(int);
     bool NewData();// ture if there exist data needed to send, false otherwise
     bool IsACKREVWindowCovered(double current_time);
     bool IsSafe();

     //     void MarkBitMap(int);
 
     void InsertACKRevLink(Packet*, double);
     void InsertACKRevLink(Packet*, double*);
     void TxData(int);
     void ClearACKRevLink();
     void StartRECV(double,int,int);
     void SetNextHop();
     void DeleteBufferCell(Packet*);
     void DeleteRecord(int);
     void CancelReservation();
     void CancelREVtimeout();
     void PrintTable();
     void ResumeTxProcess();
     void ClearChannel();

     //     void InsertNextAvailableTable(int,double);
     //  void ProcessNextAvailableTable();
     // double CheckNextAvailableTable(int);

         Event large_nd_event;
         Event short_nd_event;
         Event status_event;
	 // Event large_acknd_event;
         Event short_acknd_event;         
         Event phaseone_event;
         Event phasetwo_event;
         Event phasethree_event;
         Event sleep_event;
         Event wakeup_event;
         Event timeout_event;
         Event transmission_event;
         Event mac_recv_event;
         Event clear_channel_event;
         Event ackwindow_event;
         Event carrier_sense_event;


         NDStatusHandler status_handler;
	 // LargeNDHandler large_nd_handler;
         ShortNDHandler short_nd_handler;
         NDBackoffHandler backoff_handler;
         
         ACKNDHandler acknd_handler;
	 //  LargeAckNDWindowHandler large_acknd_window_handler;         
         ShortAckNDWindowHandler short_acknd_window_handler; 

         PhaseOneHandler phaseone_handler;
         PhaseTwoHandler phasetwo_handler; 
         PhaseThreeHandler phasethree_handler;
         SleepHandler sleep_handler;   
         WakeupHandler wakeup_handler; 
         ReserveHandler reserve_handler; 
         ACKDATAHandler ackdata_handler;
         TimeoutHandler timeout_handler;
         ACKREVHandler ackrev_handler;
         TransmissionHandler transmission_handler;
         MACRECVHandler mac_recv_handler;
         ClearChannelHandler clear_channel_handler;
         ACKWindowHandler ackwindow_handler;
         CarrierSenseHandler carrier_sense_handler;
         SYNHandler syn_handler;      
        //Node* node(void) const {return node_;}
        // to process the incomming packet
        virtual  void RecvProcess(Packet*);
      
       // to process the outgoing packet
        virtual  void TxProcess(Packet*);

protected:        
	inline int initialized() {
	return  UnderwaterMac::initialized();
	}
 private:
	//	double interval_ND_ACKND;
        friend class NDBackoffHandler;
        friend class AckNDWindowHanlder;
        friend class NDHandler;
        friend class Status_handler;
        friend class PhaseOneHandler;
        friend class PhaseTwoHandler;
        friend class PhaseThreeHandler;
        friend class SleepHandler;
        friend class ReserveHandler;
        friend class TimeoutHandler;
        friend class ACKREVHandler;
        friend class TransmissionHandler;
        friend class MACRECVHandler;
        friend class ACKDATAHanlder;
        friend class ClearChannelHandler;
        friend class ACKWindowHandler;
        friend class CarrierSenseHandler;
        friend class SYNHandler;
        
};

#endif /* __rmac_h__ */

