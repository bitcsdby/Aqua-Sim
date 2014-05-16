

#ifndef ns_underwaterpropagation_h
#define ns_underwaterpropagation_h



#define SPEED_OF_SOUND_IN_WATER	1500  //actual speed of sound in water

#include <topography.h>
#include <packet-stamp.h>
#include "underwaterphy.h"

class PacketStamp;
class UnderwaterPhy;

class UnderwaterPropagation : public TclObject {

public:
  UnderwaterPropagation() : name(NULL), topo(NULL) {}

  // calculate the Pr by which the receiver will get a packet sent by
  // the node that applied the tx PacketStamp for a given inteface 
  // type
  
  double  Attenuation(double, double, double);
  double  Thorp(double);
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, UnderwaterPhy *);
  virtual int command(int argc, const char*const* argv);

protected:
  char *name;
  Topography *topo;
};

#endif /* __underwaterpropagation_h__ */

