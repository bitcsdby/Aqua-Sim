#ifndef ns_tmac_h
#define ns_tmac_h

#include "underwatermac.h"
#include "config.h"
#include "packet.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "uwbuffer.h"

#define TABLE_SIZE 10 // the size of delay table
#define MAXIMUMBACKOFF 4 // the maximum times of backoffs
#define BACKOFF 1 //deleted later, used by TxProcess


#define UW_ND 1
#define UW_ACK_ND 2


#define PHASEONE 1
#define PHASETWO 2
#define PHASETHREE 3

enum TmacPacketType{
  P_DATA,
  P_RTS,
  P_CTS,
  P_ND,
  P_SACKND,
  P_ACKDATA,
  P_SYN
};


enum MAC_STATUS{
  TMAC_IDLE,
  TMAC_RTS,
  TMAC_CTS,
  TMAC_RECV,
  TMAC_TRANSMISSION,
  TMAC_SILENCE,
  TMAC_SLEEP
};





struct time_record{
  int node_addr;// the address of the node
  double arrival_time;// the time to receive the ND packet from the node
  double sending_time; // the sending time of ND in local clock
};




struct period_record{
  int node_addr;// the address of the node
  double difference;// the difference with my period
  double duration; // duration of duty cycle
  double last_update_time; // the time last updated
};



struct silence_record {
  int node_addr;// the address of the node
  double start_time;// the start time of the silence
  double duration; // duration of duty cycle
  int  confirm_id; // silence is confirmed
};





struct latency_record{
  int node_addr;      // the address of the node
  double latency;    // the propagation latency with that node
  double sumLatency;// the sum of latency
  int num;         // number of ACKND packets 
  double last_update_time; // the time of last update
};

struct hdr_tmac{
        int ptype;     //packet type
        int pk_num;    // sequence number
        int data_num; 
        int sender_addr;  //original sender' address
   	double st;           // Timestamp when pkt is generated.
        int receiver_addr;
        double duration;
        double interval;
        double arrival_time;
         double ts;	

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_tmac* access(const Packet*  p) {
		return (hdr_tmac*) p->access(offset_);
	}
};




class TMac;

class SilenceHandler: public Handler{
 public:
  //SilenceHandler(TMac*);
  SilenceHandler(TMac* p): mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};

class ACKHandler: public Handler{
 public:
  // ACKHandler(TMac*);
  ACKHandler(TMac* p): mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};



class TTimeoutHandler: public Handler{
 public:
  //  TTimeoutHandler(TMac*);
  TTimeoutHandler(TMac* p): mac_(p){};
  void handle(Event*);

 private:
  TMac* mac_;
};

class RTSTimeoutHandler: public Handler{
 public:
  //RTSTimeoutHandler(TMac*);
  RTSTimeoutHandler(TMac* p): mac_(p){num=0;};
  void handle(Event*);
  int num;

 private:
  TMac* mac_;
};

class TSYNHandler: public Handler{
 public:
  // TSYNHandler(TMac*);
  TSYNHandler(TMac* p): mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};



class PoweroffHandler: public Handler{
 public:
  //PoweroffHandler(TMac*);
  PoweroffHandler(TMac* p): mac_(p){};
  void handle(Event*);

 private:
  TMac* mac_;
};



class TWakeupHandler: public Handler{
 public:
  //TWakeupHandler(TMac*);
  TWakeupHandler(TMac* p): mac_(p){};
  void handle(Event*);

 private:
  TMac* mac_;
};


class ACKDATAHandler: public Handler{
 public:
  ACKDATAHandler(TMac*);
  void handle(Event*);
 private:
  TMac* mac_;
};



class TxHandler: public Handler{
 public:
  //  TxHandler(TMac*);
  
  TxHandler(TMac* p):mac_(p), receiver(0){};
  void handle(Event*);
   int receiver;
 private: 
  TMac* mac_;
};


class TBackoffHandler: public Handler{
 public:
  //  TBackoffHandler(TMac*);
  TBackoffHandler(TMac* p):mac_(p),window_(0),counter_(0){};
  void handle(Event*);
  void clear();
  double window_;

 private:
  int counter_;
  TMac* mac_;
};


class TNDHandler: public Handler{
 public:
  // TNDHandler(TMac*);
  TNDHandler(TMac* p):mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};



class TStatusHandler: public Handler{
 public:
  // TStatusHandler(TMac*);
  TStatusHandler(TMac* p):mac_(p),status_(SLEEP){};
  void SetStatus(TransmissionStatus);
  void handle(Event*);
 private:
  TransmissionStatus status_;     
  TMac* mac_;
};




class TACKNDWindowHandler: public Handler{
 public:
  // TACKNDWindowHandler(TMac*);
  TACKNDWindowHandler(TMac* p):mac_(p){}
  void handle(Event*);
 private:
  TMac* mac_;
};


class TACKNDHandler: public Handler{
 public:
  //  TACKNDHandler(TMac*);
  TACKNDHandler(TMac* p):mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};

class RTSHandler: public Handler{
 public:
  //RTSHandler(TMac*);
  RTSHandler(TMac* p):mac_(p){receiver_addr=0;};
  void handle(Event*);
  int receiver_addr;
 private:
  TMac* mac_;
};




class CTSHandler: public Handler{
 public:
  //CTSHandler(TMac*);
  CTSHandler(TMac* p):mac_(p),num(0){};
  void handle(Event*);
  int num;
 private:
  TMac* mac_;
};




class TPhaseOneHandler: public Handler{
 public:
  //  TPhaseOneHandler(TMac*);
  TPhaseOneHandler(TMac* p):mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};


class TPhaseTwoHandler: public Handler{
 public:
  // TPhaseTwoHandler(TMac*);
  TPhaseTwoHandler(TMac* p):mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};

class TPhaseThreeHandler: public Handler{
 public:
  // TPhaseThreeHandler(TMac*);
  TPhaseThreeHandler(TMac* p):mac_(p){};
  void handle(Event*);
 private:
  TMac* mac_;
};



class TMac: public UnderwaterMac {
   
public:
        TMac();
     
       	int  command(int argc, const char*const* argv);
        double  ND_window_;// the window to send ND
        double  ACKND_window_;// the winddow to send ACK_ND
        double  PhaseOne_window_; // the time for latency detection
        double  PhaseTwo_window_; // the time for SYN announcement
        double SIF_;// interval between two successive data packets
        double last_silenceTime; // the time for the longest silence  
        double last_rts_silenceTime;    
        double PhaseTwo_interval_;// interval between windows of phase two

        int PhyOverhead_;// the overhead caused by phy layer
        int arrival_table_index;

        int short_latency_table_index;
        int period_table_index;
        int silenceTableIndex;      

        int data_sender; // address of the data sender
        int bit_map[MAXIMUM_BUFFER];// in real world, this is supposed to use bit map to indicate the lost of packet
	// these two variables are used to set next hop 
	// SetHopStatus=1 then set next hop using next_hop
	// int setHopStatus;
        //int next_hop;

        int num_send; 
        int num_data;
        int large_packet_size_;
        int short_packet_size_;   
        double TransmissionRange_;
        double duration_; // duration of duty cycle     
        double IntervalPhase2Phase3_;
        double next_period;//the start_time of next duty cycle
        double PeriodInterval_;
        double max_short_packet_transmissiontime;
        double max_large_packet_transmissiontime;
        double transmission_time_error_; // 
        double max_propagationTime;

        double TAduration_;
        double ContentionWindow_;
        double MinBackoffWindow;
        
	// bool overhearData;


       int PhaseOne_cycle_; // number of cycles in phase one
       int PhaseTwo_cycle_; // number of cycles in phase two 
        int PhaseStatus;  

        enum MAC_STATUS mac_status;

	double cycle_start_time; // the begining time of this cycle;  
        TransmissionBuffer txbuffer;
         struct  time_record arrival_table[TABLE_SIZE]; 
                
         struct latency_record short_latency_table[TABLE_SIZE];    
         struct period_record  period_table[TABLE_SIZE];
         struct silence_record silence_table[TABLE_SIZE];

 void InitPhaseOne(double/*ND window*/,double/*ack_nd window*/,double/* phaseOne window*/); 
    
 void InitPhaseTwo(); 
 void InitPhaseThree();
  void StartPhaseTwo();


     void InitND(double/*ND window*/,double/*ack_nd window*/,double/* phase One window*/);// to detect latency 

     void SendND(int);
     void TxND(Packet*, double);
    
     void ProcessNDPacket(Packet*);   
     void ProcessDataPacket(Packet*);   
     void ProcessShortACKNDPacket(Packet*);   
     void ProcessSYN(Packet*);
     void ProcessRTSPacket(Packet*);
     void ProcessCTSPacket(Packet*);
     void ProcessSleep();
     void ProcessSilence();
     
     void InitializeSilenceTable();
     void DeleteSilenceTable(int);
     void ConfirmSilenceTable(int, double);
     void DeleteSilenceRecord(int);
     void DataUpdateSilenceTable(int);
     void InsertSilenceTable(int, double);
     void CleanSilenceTable();

  
     Packet* GenerateCTS(int,double);
     void ProcessACKDataPacket(Packet*);
     void ClearTxBuffer();
     void Wakeup();
     void ReStart();
     void SendACKPacket(); 
     void SetIdle();  
     void SendRTS();
     void SendCTS();
     void TxCTS(Packet*);

     void TxACKData(Packet*);
     void ResetMacStatus();
   
     void SendShortAckND();
     void StatusProcess(Event*,TransmissionStatus);
     void SendSYN();
     bool NewData();// ture if there exist data needed to send, false otherwise
    
     void TxRTS(Event*,int);
     double CheckLatency(latency_record*,int);
     double CheckDifference(period_record*,int);
       
     void MarkBitMap(int);

   
     void TxData(int);
     void PrintTable();
     void ResumeTxProcess();
   
     Packet* GenerateSYN();

         Event large_nd_event;
         Event short_nd_event;
         Event status_event;

         Event short_acknd_event;         
         Event phaseone_event;
         Event phasetwo_event;
         Event phasethree_event;
      
        
       
         Event transmission_event;

         Event silence_event;
         Event ack_event;
         Event poweroff_event;
         Event wakeup_event;
         Event timeout_event;
         Event rts_timeout_event;
	 // Event rts_silence_event;

         TStatusHandler status_handler;
         TNDHandler short_nd_handler;
         TBackoffHandler backoff_handler;
         
         TACKNDHandler acknd_handler;
         TACKNDWindowHandler short_acknd_window_handler; 

         TPhaseOneHandler phaseone_handler;
         TPhaseTwoHandler phasetwo_handler; 
         TPhaseThreeHandler phasethree_handler;
         
         ACKHandler ackdata_handler;
       
         TxHandler transmission_handler;
        
         SilenceHandler silence_handler;
         ACKHandler   ack_handler;
         PoweroffHandler poweroff_handler;
         TWakeupHandler wakeup_handler; 
         TTimeoutHandler timeout_handler;
         RTSTimeoutHandler rts_timeout_handler;
         RTSHandler rts_handler;
	 //     RTSSilenceHandler rts_silence_handler;
         CTSHandler cts_handler;
         TSYNHandler syn_handler;   
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

        friend class TBackoffHandler;
        friend class TACKNDWindowHandler;
        friend class TNDHandler;
        friend class TPhaseOneHandler;
        friend class TPhaseTwoHandler;
        friend class TPhaseThreeHandler;
        friend class TACKNDHandler;

        friend class TxHandler;
        friend class TTimeoutHandler;
        friend class RTSTimeoutHandler;
        friend class SilenceHandler;
        friend class ACKHandler;
        friend class PoweroffHandler;
        friend class RTSHandler;
        friend class TStatusHandler;
        friend class TWakeupHandler;
        friend class CTSHandler; 
        friend class TSYNHandler;


};

#endif /* __tmac_h__ */

