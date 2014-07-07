#include "gpssynchr.h"
#include <math.h>

gpssynchr::gpssynchr(void* underwaternodepointer,double z)
{
	this->nodepointer = underwaternodepointer;
	this->s.z = z;
}


gpssynchr::~gpssynchr(void)
{
}

/*
参数:CoordinatePair n 新加入的坐标时间对

返回值: void

说明：
此函数是唯一的对外接口
此函数判断类内cplist是否有三个以上的坐标对
如果有则调用localization()函数进行
*/
void gpssynchr::addnewpair(CoordinatePair n)
{
	this->cplist.push_back(n);

	if(this->cplist.size() == 4)
		this->cplist.erase(this->cplist.begin(),this->cplist.begin()+1);
	
	if(this->cplist.size() == 3)
		this->localization();
}


/*
参数:coor 来自某一刻卫星的坐标结构

返回值: [ax,ay,-c]

说明:
coor 是来自某一颗卫星的坐标结构
根据该结构看，结合自身估计的x y z值 
计算出系数矩阵 H 的一行
实现了公式1 2
*/
vector<double> gpssynchr::getcofficient(Coordinate coor)
{
	double ri = getri(coor);

	double ax = (coor.x - this->s.x) / ri;
	double ay = (coor.y - this->s.y) / ri;

	vector<double> vret;

	vret.push_back(ax);
	vret.push_back(ay);
	vret.push_back(VELOCITY);

	return vret;
}
	
/*
参数  来自本地cplist中的一个坐标时间对

返回值: deltarou = _rou - rou

说明:
该函数用于计算 deltarou  实现公式3
*/
double gpssynchr::getdeltarou(CoordinatePair p)
{
	double _rou = getri(p.s) + \
				  VELOCITY * this->tu;
	double rou = VELOCITY * (p.localreceivetime - p.s.ts);

	return _rou - rou;
}

/*
参数 一个坐标结构体
返回值 计算后的Ri值

说明:计算参数中坐标和本地坐标的欧几里得距离
*/
double gpssynchr::getri(Coordinate coor)
{
	return sqrt((pow(coor.x-this->s.x,2)+ \
		         pow(coor.y-this->s.y,2)+ \
				 pow(coor.z-this->s.z,2)));
}

/*
参数 无
返回值 无

说明:该函数是定位和同步的核心函数

首先计算系数矩阵 H，和 向量 deltarou

求解线性方程组 4

得到解向量为 [deltax,deltay,deltat]
然后用该向量分别矫正x y 和 t，并对节点的本地对应参数赋值矫正。
*/
void gpssynchr::localization()
{
	vector<vector<double>> matrixH;
	vector<double> rou;
	vector<double> solution(3,0.0);

	this->s.x = (cplist[0].s.x + cplist[1].s.x + cplist[2].s.x) / 3.0;
	this->s.y = (cplist[0].s.y + cplist[1].s.y + cplist[2].s.y) / 3.0;
	this->tu = 0.0;

	matrixH.push_back(this->getcofficient(this->cplist[0].s));
	matrixH.push_back(this->getcofficient(this->cplist[1].s));
	matrixH.push_back(this->getcofficient(this->cplist[2].s));

	rou.push_back(this->getdeltarou(this->cplist[0]));
	rou.push_back(this->getdeltarou(this->cplist[1]));
	rou.push_back(this->getdeltarou(this->cplist[2]));

	//solution = H^(-1) * rou

	//补偿
	//nodepointer
}
