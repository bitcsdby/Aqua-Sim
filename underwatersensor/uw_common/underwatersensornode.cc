#include <math.h>
#include <stdlib.h>
#include "connector.h"
#include "delay.h"
#include "packet.h"
#include "random.h"
#include "trace.h"
#include "address.h"
#include "arp.h"
#include "topography.h"
#include "ll.h"
#include "mac.h"
#include "underwatersensor/uw_mac/underwaterpropagation.h"
#include "underwatersensornode.h"
#include "phy.h"
#include "wired-phy.h"
#include "god.h"


static class UnderwaterSensorNodeClass : public TclClass {
public:
        UnderwaterSensorNodeClass() : TclClass("Node/MobileNode/UnderwaterSensorNode") {}
        TclObject* create(int, const char*const*) {
                return (new UnderwaterSensorNode);
        }
} class_underwatersensornode;



void
UnderwaterPositionHandler::handle(Event*)
{
	Scheduler& s = Scheduler::instance();


#if 0
	fprintf(stderr, "*** POSITION HANDLER for node %d (time: %f) ***\n",
		node->address(), s.clock());
#endif
	/*
	 * Update current location
	 */

	//	printf("*** POSITION HANDLER for node %d (time: %f) ***\n", node->address(), s.clock()); // added by Peng Xie
	node->update_position();

	/*
	 * Choose a new random speed and direction
	 */
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                node->address_, __PRETTY_FUNCTION__);
#endif
	//	node->random_destination();
	node->check_position();
	s.schedule(&node->uw_pos_handle_, &node->uw_pos_intr_,
		   node->position_update_interval_);
}



/* ======================================================================
   Underwater Sensor Node 
   ====================================================================== */

UnderwaterSensorNode::UnderwaterSensorNode(void): 
  MobileNode(),uw_pos_handle_(this)
{
  
      	sinkStatus_=0;  // add by Peng Xie
        failure_status_=0;// add by peng xie
        failure_status_pro_=0;//added by peng xie AND ZHENG
        failure_pro_=0.0; // add by peng xie
        trans_status=IDLE;
        T_=0;
        setHopStatus=0;
	next_hop=-10;        
	position_update_interval_ = 1.0;

       	bind("sinkStatus_", &sinkStatus_);
	bind("position_update_interval_", &position_update_interval_);
        	bind("max_speed", &max_speed);
        	bind("min_speed", &min_speed);
		//  printf("underwatersensornode: constructor:node %d(%f,%f,%f)\n",nodeid(),X_,Y_,Z_);
	
}




void 
UnderwaterSensorNode::start()
{
	Scheduler& s = Scheduler::instance();

	//printf("underwatersensornode: start\n");
	if(random_motion_ == 0) {
	  //printf("underwatersensornode: before log_movement\n");
		log_movement();
		return;
	}

	assert(initialized());

	//printf("underwatersensornode: before random_position\n");

	random_position();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                address_, __PRETTY_FUNCTION__);
#endif
	random_destination();
       
	s.schedule(&uw_pos_handle_, &uw_pos_intr_, position_update_interval_);
}


int
UnderwaterSensorNode::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if(argc==2){
	  if(strcmp(argv[1],"start")==0){
	    start();
            return TCL_OK;
	  }
          if(strcmp(argv[1],"move")==0){
	    move();
            return TCL_OK;
	  }
	   if(strcmp(argv[1],"failure-status")==0){
	   tcl.resultf("%d",failure_status());
	  return TCL_OK;
	   }
	}
	else if(argc == 3) {
	     
		     // added by peng xie
            if(strcmp(argv[1], "set-cx") == 0) {
			CX_=atof(argv[2]);
			return TCL_OK;
	     }else if(strcmp(argv[1], "set-cy") == 0){
		      CY_=atof(argv[2]);
		     return TCL_OK;
             }else if(strcmp(argv[1], "set-cz")==0){
                      CZ_=atof(argv[2]);
		     return TCL_OK;   
		return TCL_OK;
	     }else if(strcmp(argv[1], "setSpeed") == 0){
		      speed_=atof(argv[2]);
		     return TCL_OK;
             }else if(strcmp(argv[1], "random-motion") == 0){
		      random_motion_=atoi(argv[2]);
		     return TCL_OK;
             }else if(strcmp(argv[1], "setPositionUpdateInterval") == 0){
		      position_update_interval_=atof(argv[2]);
		     return TCL_OK;
	     } else if(strcmp(argv[1], "set-failure_status") == 0){
	          failure_status_=atoi(argv[2]);
		  // printf("underwaternode (%d) set the status %d at %f\n",address_,failure_status_,NOW);
	         return TCL_OK;
	     }
            else if(strcmp(argv[1], "set-failure_status_pro") == 0){
		      failure_status_pro_=atof(argv[2]);
                      generateFailure();   
		     return TCL_OK;
             } else if(strcmp(argv[1], "set-failure_pro") == 0){
		      failure_pro_=atof(argv[2]);
		     return TCL_OK;
                    //end of peng Xie's addition 
	     } else  if (strcmp(argv[1], "set_next_hop") == 0) {
		  setHopStatus=1;
		  next_hop=atoi(argv[2]);
		   return TCL_OK;
	     }
              else if(strcmp(argv[1],"topography")==0){
               T_=(Topography*) TclObject::lookup(argv[2]);
	       if(T_==0) return TCL_ERROR; 
              //I am not sure if I can do this 
               MobileNode::command(argc, argv);            
	       return TCL_OK;
	     }
	}
	    
	return MobileNode::command(argc, argv);
}


//added by peng xie
void 
UnderwaterSensorNode::move()
{
	Scheduler& s = Scheduler::instance();
	//	printf("underwatersensornode: ?????????????????the move\n");
	//	if(random_motion_ == 0) {
	//log_movement();
	//return;
	//}

	//assert(initialized());

	//	random_position(z);
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                address_, __PRETTY_FUNCTION__);
#endif
	random_destination();
	s.schedule(&uw_pos_handle_, &uw_pos_intr_, position_update_interval_);
}



void 
UnderwaterSensorNode::random_position()
{
  //  printf("underwatersensornode:  in the random_position\n");
 if (T_ == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	X_ = Random::uniform() * T_->upperX();
	Y_ = Random::uniform() * T_->upperY();
        Z_ = Random::uniform() * T_->upperZ();
	//original one	Z_ = T_->height(X_, Y_);
	// Z_ =z;
	position_update_time_ = 0.0;
}



void 
UnderwaterSensorNode::generateFailure()
{
  double error_pro=Random::uniform();
  if(error_pro<failure_status_pro_) failure_status_=1;


}

void 
UnderwaterSensorNode::check_position()
{
	if((X_==destX_)||(Y_==destY_)) {
          random_speed();
          random_destination();
	  //    printf("vectorbased:set new des and speed+++++\n");
        }
        else {
		 log_movement();
	}
}







void
UnderwaterSensorNode::random_speed()
{
	
	speed_ = (Random::uniform() * (max_speed-min_speed))+min_speed;
	//	printf("underwatersensornode: ?????????????????the max_speed%f min %f speed  %f interval%f\n",max_speed,min_speed,speed_,position_update_interval_);
}

void
UnderwaterSensorNode
::random_destination()
{
	//printf("mobilenode: ?????????????????the randmo_destionation\n");
	if (T_ == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	random_speed();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling set_destination()\n",
                address_, __FUNCTION__);
#endif
	(void) set_destination(Random::uniform() * T_->upperX(),
                               Random::uniform() * T_->upperY(),
                               speed_);
}




double
UnderwaterSensorNode::propdelay(UnderwaterSensorNode *m)
{
	//printf("underwatersensornode: ?????????????????the properdelay\n");
  return distance(m) / SPEED_OF_SOUND_IN_WATER;
}




int
UnderwaterSensorNode::setSinkStatus()
{
	//	printf("underwatersensornode: I change it to 1\n");
	sinkStatus_=1;
	return 0; 
}



int
UnderwaterSensorNode::clearSinkStatus()
{
	//	printf("underwatersensornode: I change it to 1\n");
	sinkStatus_=0;
	return 0; 
}


