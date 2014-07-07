#include "common.h"
#include <vector>

using namespace std;

#pragma once

class gpssynchr
{
private:
	//x y z的估计值坐标结构
	Coordinate s;
	vector<CoordinatePair> cplist;

	//function getxxx are used for caculation
	vector<double> getcofficient(Coordinate coor);
	double getdeltarou(CoordinatePair p);
	double getri(Coordinate coor);

	void localization();

	//tu 的估计值
	double tu;

	void* nodepointer;

public:
	// add a new pair of coordinate and localreceivetime;
	void addnewpair(CoordinatePair n);
	gpssynchr(void* underwaternodepointer,double z);
	~gpssynchr(void);
};

