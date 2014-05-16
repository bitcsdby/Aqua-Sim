/* 
This program is the modified version of channel.cc, it supports 3-dimensional space and position estimation used by VBF--modified by xp at 2007

*/


#include <float.h>
#include "trace.h"
#include "delay.h"
#include "object.h"
#include "packet.h"
#include "mac.h"
#include "underwaterchannel.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "mobilenode.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "ip.h"
#include "dsr/hdr_sr.h"

#include "underwatersensor/uw_routing/vectorbasedforward.h" // added by peng xie
#include "rmac.h"



static class UnderwaterChannelClass : public TclClass {
public:
        UnderwaterChannelClass() : TclClass("Channel/UnderwaterChannel") {}
        TclObject* create(int, const char*const*) {
                return (new UnderwaterChannel);
        }
} class_underwaterchannel;

class MobileNode;

double UnderwaterChannel::distCST_ =100.0;


UnderwaterChannel::UnderwaterChannel(void) : Channel(), numNodes_(0), 
					 xListHead_(NULL), sorted_(0) {}

int UnderwaterChannel::command(int argc, const char*const* argv)
{
	
	if (argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		if (strcmp(argv[1], "add-node") == 0) {
			addNodeToList((MobileNode*) obj);
			return TCL_OK;
		}
		else if (strcmp(argv[1], "remove-node") == 0) {
			removeNodeFromList((MobileNode*) obj);
			return TCL_OK;
		}
	}
	return Channel::command(argc, argv);
}


void
UnderwaterChannel::sendUp(Packet* p, Phy *tifp)
{
  //	 printf("underwaterchannel sendup\n");
	Scheduler &s = Scheduler::instance();
	Phy *rifp = ifhead_.lh_first;
	Node *tnode = tifp->node();
	Node *rnode = 0;
	Packet *newp;
	double propdelay = 0.0;
	struct hdr_cmn *hdr = HDR_CMN(p);

	 hdr->direction() = hdr_cmn::UP;
	 //	 printf("underwaterchannel the direction is set %d real is%d\n",hdr_cmn::UP,hdr->direction());
                  // use list-based improvement
	 //printf("Underwaterchannel: the distCST is %f\n",distCST_);
	         MobileNode *mtnode = (MobileNode *) tnode;
		 MobileNode **affectedNodes;// **aN;
		 int numAffectedNodes = -1, i;
		 
		 if(!sorted_){
			 sortLists();
		 }
       affectedNodes = getAffectedNodes(mtnode, distCST_ , &numAffectedNodes); 
      
			 //  printf("underwaterchannel the affected number of node is %d\n",numAffectedNodes);
		 for (i=0; i < numAffectedNodes; i++) {
			 rnode = affectedNodes[i];
			 

		   double d1=distance(tnode,rnode);

		    if((rnode == tnode)||(d1>distCST_))
			{
			  //printf("channel they are same\n");
                          	 continue;
			}


			 newp = p->copy();
		     
                         // add by peng Xie
		      
                         calculatePosition(tnode,rnode,newp);
			 propdelay = get_pdelay(tnode, rnode);	      
 
			 rifp = (rnode->ifhead()).lh_first;
	
			 for(; rifp; rifp = rifp->nextnode()){	
			 
			   //   	 printf("channel :node %d and node %d distance is %f propdelay is %f at time %f\n",mtnode->nodeid(),rnode->nodeid(),d1,propdelay,NOW);		  
			   s.schedule(rifp, newp, propdelay);
			 }
		 
		 }
	 
		 delete [] affectedNodes;
			
	          	 Packet::free(p);   
}


double
UnderwaterChannel::distance(Node *sender,Node* receiver)
{
       	MobileNode *s1,*r1;
         
	 s1=(MobileNode*)sender;
         r1=(MobileNode*)receiver;
 
	 // printf("underwater channel calculateposition1: :(%f,%f,%f) and  (%f,%f,%f)\n", s1->X(),s1->Y(),s1->Z(),r1->X(),r1->Y(),r1->Z());
        double dx,dy,dz,d;
	dx=r1->X()-s1->X();
	dy=r1->Y()-s1->Y();
	dz=r1->Z()-s1->Z();
        d=sqrt((dx*dx)+(dy*dy)+(dz*dz));
      
	// printf("channel :node %d and node %d distance is %f \n",s1->address(),r1->address(),d);
  return d;
}



// this is used by VBF routing protocol
void
UnderwaterChannel::calculatePosition(Node *sender,Node* receiver, Packet* p)
{
	MobileNode *s1,*r1;
         hdr_uwvb* vbh=HDR_UWVB(p);
         
	 s1=(MobileNode*)sender;
         r1=(MobileNode*)receiver;
 
	 //	  printf("channel calculateposition :(%f,%f,%f) and  (%f,%f,%f)\n", s1->X(),s1->Y(),s1->Z(),r1->X(),r1->Y(),r1->Z());
        double dx,dy,dz;
	dx=r1->X()-s1->X();
	dy=r1->Y()-s1->Y();
	dz=r1->Z()-s1->Z();
        
      
        vbh->info.dx=dx;
        vbh->info.dy=dy;
        vbh->info.dz=dz;
      

      // printf("channel calculateposition2: :(%f,%f,%f) \n",dx,dy,dz);

}

void
UnderwaterChannel::addNodeToList(MobileNode *mn)
{
	MobileNode *tmp;
	//  printf("channel: new node :(%f,%f,%f)\n",mn->X(),mn->Y(),mn->Z());
	// create list of mobilenodes for this channel
	if (xListHead_ == NULL) {
		fprintf(stderr, "INITIALIZE THE LIST xListHead\n");
		xListHead_ = mn;
		xListHead_->nextX_ = NULL;
		xListHead_->prevX_ = NULL;
	} else {
		for (tmp = xListHead_; tmp->nextX_ != NULL; tmp=tmp->nextX_);
		tmp->nextX_ = mn;
		mn->prevX_ = tmp;
		mn->nextX_ = NULL;
	}
	numNodes_++;
}

void
UnderwaterChannel::removeNodeFromList(MobileNode *mn) {
	
	MobileNode *tmp;
	// Find node in list
	for (tmp = xListHead_; tmp->nextX_ != NULL; tmp=tmp->nextX_) {
		if (tmp == mn) {
			if (tmp == xListHead_) {
				xListHead_ = tmp->nextX_;
				if (tmp->nextX_ != NULL)
					tmp->nextX_->prevX_ = NULL;
			} else if (tmp->nextX_ == NULL) 
				tmp->prevX_->nextX_ = NULL;
			else {
				tmp->prevX_->nextX_ = tmp->nextX_;
				tmp->nextX_->prevX_ = tmp->prevX_;
			}
			numNodes_--;
			return;
		}
	}
	fprintf(stderr, "Channel: node not found in list\n");
}

void
UnderwaterChannel::sortLists(void) {
	bool flag = true;
	MobileNode *m, *q;

	sorted_ = true;
	
	fprintf(stderr, "SORTING LISTS ...");
	/* Buble sort algorithm */
	// SORT x-list
	while(flag) {
		flag = false;
		m = xListHead_;
		while (m != NULL){
			if(m->nextX_ != NULL)
				if ( m->X() > m->nextX_->X() ){
					flag = true;
					//delete_after m;
					q = m->nextX_;
					m->nextX_ = q->nextX_;
					if (q->nextX_ != NULL)
						q->nextX_->prevX_ = m;
			    
					//insert_before m;
					q->nextX_ = m;
					q->prevX_ = m->prevX_;
					m->prevX_ = q;
					if (q->prevX_ != NULL)
						q->prevX_->nextX_ = q;

					// adjust Head of List
					if(m == xListHead_) 
						xListHead_ = m->prevX_;
				}
			m = m -> nextX_;
		}
	}
	
	fprintf(stderr, "DONE!\n");
}

void
UnderwaterChannel::updateNodesList(class MobileNode *mn, double oldX) {
	
	MobileNode* tmp;
	double X = mn->X();
	bool skipX=false;
	
	if(!sorted_) {
		sortLists();
		return;
	}
	
	/* xListHead cannot be NULL here (they are created during creation of mobilenode) */
	
	/***  DELETE ***/
	// deleting mn from x-list
	if(mn->nextX_ != NULL) {
		if(mn->prevX_ != NULL){
			if((mn->nextX_->X() >= X) && (mn->prevX_->X() <= X)) skipX = true; // the node doesn't change its position in the list
			else{
				mn->nextX_->prevX_ = mn->prevX_;
				mn->prevX_->nextX_ = mn->nextX_;
			}
		}
		
		else{
			if(mn->nextX_->X() >= X) skipX = true; // skip updating the first element
			else{
				mn->nextX_->prevX_ = NULL;
				xListHead_ = mn->nextX_;
			}
		}
	}
	
	else if(mn->prevX_ !=NULL){
		if(mn->prevX_->X() <= X) skipX = true; // skip updating the last element
		else mn->prevX_->nextX_ = NULL;
	}

	/*** INSERT ***/
	//inserting mn in x-list
	if(!skipX){
		if(X > oldX){			
			for(tmp = mn; tmp->nextX_ != NULL && tmp->nextX_->X() < X; tmp = tmp->nextX_);
			//fprintf(stdout,"Scanning the element addr %d X=%0.f, next addr %d X=%0.f\n", tmp, tmp->X(), tmp->nextX_, tmp->nextX_->X());
			if(tmp->nextX_ == NULL) { 
				//fprintf(stdout, "tmp->nextX_ is NULL\n");
				tmp->nextX_ = mn;
				mn->prevX_ = tmp;
				mn->nextX_ = NULL;
			} 
			else{ 
				//fprintf(stdout, "tmp->nextX_ is not NULL, tmp->nextX_->X()=%0.f\n", tmp->nextX_->X());
				mn->prevX_ = tmp->nextX_->prevX_;
				mn->nextX_ = tmp->nextX_;
				tmp->nextX_->prevX_ = mn;  	
				tmp->nextX_ = mn;
			} 
		}
		else{
			for(tmp = mn; tmp->prevX_ != NULL && tmp->prevX_->X() > X; tmp = tmp->prevX_);
				//fprintf(stdout,"Scanning the element addr %d X=%0.f, prev addr %d X=%0.f\n", tmp, tmp->X(), tmp->prevX_, tmp->prevX_->X());
			if(tmp->prevX_ == NULL) {
				//fprintf(stdout, "tmp->prevX_ is NULL\n");
				tmp->prevX_ = mn;
				mn->nextX_ = tmp;
				mn->prevX_ = NULL;
				xListHead_ = mn;
			} 
			else{
				//fprintf(stdout, "tmp->prevX_ is not NULL, tmp->prevX_->X()=%0.f\n", tmp->prevX_->X());
				mn->nextX_ = tmp->prevX_->nextX_;
				mn->prevX_ = tmp->prevX_;
				tmp->prevX_->nextX_ = mn;  	
				tmp->prevX_ = mn;		
			}
		}
	}
}


MobileNode **
UnderwaterChannel::getAffectedNodes(MobileNode *mn, double radius,
				  int *numAffectedNodes)
{
	double xmin, xmax, ymin, ymax;
        double zmin,zmax;// add by peng xie
	int n = 0;
	MobileNode *tmp, **list, **tmpList;

	if (xListHead_ == NULL) {
		*numAffectedNodes=-1;
		fprintf(stderr, "xListHead_ is NULL when trying to send!!!\n");
		return NULL;
	}
	//printf( "channel:mn(%d) is(%f,%f,%f) and radius is %f\n",mn->address() ,mn->X(),mn->Y(),mn->Z(),radius);	
	xmin = mn->X() - radius;
	xmax = mn->X() + radius;
	ymin = mn->Y() - radius;
	ymax = mn->Y() + radius;
	 
	// added by peng xie
        zmin = mn->Z() - radius;
	zmax = mn->Z() + radius;
	// end of addition 

	// First allocate as much as possibly needed
	tmpList = new MobileNode*[numNodes_];
	
	//	printf("channel: the min and max is%f and %fabd raduid is%f\n",zmin,zmax,radius);

	for(tmp = mn; tmp != NULL && tmp->X() >= xmin; tmp=tmp->prevX_){
	  //	printf("channel: put one is the list is (%f, %f,%f)\n",tmp->X(),tmp->Y(),tmp->Z());	
		if(tmp->Y() >= ymin && tmp->Y() <= ymax){
			//  	printf("channel2: (%f,%f,%f)\n",tmp->X(),tmp->Y(),tmp->Z());
			// added by peng xie
	       	if(tmp->Z() >= zmin && tmp->Z() <= zmax)  tmpList[n++] = tmp;

		}
		//	printf("channel: put one is the list is (%f, %f,%f)\n",tmp->X(),tmp->Y(),tmp->Z());	
}
      for(tmp = mn->nextX_; tmp != NULL && tmp->X() <= xmax; tmp=tmp->nextX_){
		if(tmp->Y() >= ymin && tmp->Y() <= ymax){  
			//	printf("channel2: (%f,%f,%f)\n",tmp->X(),tmp->Y(),tmp->Z());
       	if(tmp->Z() >= zmin && tmp->Z() <= zmax) { 
			
                    tmpList[n++] = tmp;}
       //	tmpList[n++] = tmp;
		}
		//	printf("channel2: put one is the list is (%f, %f,%f)\n",tmp->X(),tmp->Y(),tmp->Z());
      }

	list = new MobileNode*[n];
	memcpy(list, tmpList, n * sizeof(MobileNode *));
	delete [] tmpList;
         
	*numAffectedNodes = n;
	return list;
}
 
	
double
UnderwaterChannel::get_pdelay(Node* tnode, Node* rnode)
{
	// Scheduler	&s = Scheduler::instance();
	UnderwaterSensorNode* tmnode = (UnderwaterSensorNode*)tnode;
	UnderwaterSensorNode* rmnode = (UnderwaterSensorNode*)rnode;
	double propdelay = 0;
	
	propdelay = tmnode->propdelay(rmnode);

	assert(propdelay >= 0.0);
       
	//if (propdelay == 0.0) {
		/* if the propdelay is 0 b/c two nodes are on top of 
		   each other, move them slightly apart -dam 7/28/98 */
		//propdelay = 2 * DBL_EPSILON;
		//printf ("propdelay 0: %d->%d at %f\n",
		//	tmnode->address(), rmnode->address(), s.clock());
	//	}
	return propdelay;
}







