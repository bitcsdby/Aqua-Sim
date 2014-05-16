
#include <stdio.h>
#include <topography.h>
#include "underwaterpropagation.h"
#include  "underwaterphy.h"

class PacketStamp;


static class UnderwaterPropagationClass: public TclClass {
public:
  UnderwaterPropagationClass() : TclClass("Propagation/UnderwaterPropagation") {}
        TclObject* create(int, const char*const*) {
                return (new UnderwaterPropagation);
        }
} class_underwaterpropagation;




int
UnderwaterPropagation::command(int argc, const char*const* argv)
{
  TclObject *obj;  

  if(argc == 3) 
    {
      if( (obj = TclObject::lookup(argv[2])) == 0) 
	{
	  fprintf(stderr, "Propagation: %s lookup of %s failed\n", argv[1],
		  argv[2]);
	  return TCL_ERROR;
	}

      if (strcasecmp(argv[1], "topography") == 0) 
	{
	  topo = (Topography*) obj;
	  return TCL_OK;
	}
    }
  return TclObject::command(argc,argv);
}
 
double
UnderwaterPropagation::Pr(PacketStamp * t, PacketStamp * r, UnderwaterPhy * ifp)
{
	double F = ifp->getFrequency();	     // frequency range
	double K = ifp->getEnergyspread();   // spreading factor
   
	double Xt, Yt, Zt;		// location of transmitter
	double Xr, Yr, Zr;		// location of receiver

	t->getNode()->getLoc(&Xt, &Yt, &Zt);
	r->getNode()->getLoc(&Xr, &Yr, &Zr);
 


	double dX = Xr - Xt;
	double dY = Yr - Yt;
	double dZ = Zr - Zt;
	double d = sqrt(dX * dX + dY * dY + dZ * dZ);


	// calculate receiving power at distance

	double Pr = t->getTxPr()/Attenuation(d,K,F);
	//   printf("underwaterpropagation: original power is %f and the received power is %f\n",t->getTxPr(),Pr); 
	return Pr;
}


double 
UnderwaterPropagation::Attenuation(double d, double k, double f)
{


  /* the distance unit used for absorption coefficient is km,  
     but for the attenuation, the used unit is meter 
   */

  //printf("the distance is %f and k is %f and frequency is %f\n",d,k,f);
  double d1=d/1000.0; // convert to km 
  double t1=pow(d,k);
  double alpha_f=Thorp(f);
  //printf("the alpha_f is %f\n",alpha_f);
  double alpha=pow(10.0,(alpha_f/10.0));
  //printf("the alpha is %f\n",alpha);
  double t3=pow(alpha,d1);
  // printf("t1 is %f and  t3  is %f and attenuation is %f\n",t1,t3,t1*t3);
  return t1*t3;
}

double
UnderwaterPropagation::Thorp(double f){
  return 0.11*((f*f)/(1.0+(f*f)))+44.0*((f*f)/(4100.0+(f*f)))
    +2.75*((f*f)/10000.0)+0.0003;
}
