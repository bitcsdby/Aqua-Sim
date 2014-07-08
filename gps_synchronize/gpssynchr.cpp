#include "gpssynchr.h"
#include <math.h>
#include <time.h> 



gpssynchr::gpssynchr(underwaternode* underwaternodepointer, double z)
{
	this->nodepointer = underwaternodepointer;
	this->s.z = z;
}


gpssynchr::~gpssynchr(void)
{
}


enum{S_X,S_Y,TU};
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
	
	float last_result[3];
	this->cplist.push_back(n);

	printf("The new pair is (%lf,%lf,%lf)\n", n.s.x, n.s.y, n.s.z);

	if(this->cplist.size() == 4)
		this->cplist.erase(this->cplist.begin(),this->cplist.begin()+1);
	
	if (this->cplist.size() == 3)
	{
		this->s.x = (cplist[0].s.x + cplist[1].s.x + cplist[2].s.x) / 3.0;
		this->s.y = (cplist[0].s.y + cplist[1].s.y + cplist[2].s.y) / 3.0;
		this->tu = 0.0;

		int count = 1;
		do 
		{
			printf("++++++++++++\n\tRound %d\n++++++++++++\n",count++);
			last_result[S_X] = this->s.x;
			last_result[S_Y] = this->s.y;
			last_result[TU] = this -> tu;
			this->localization();
		} while (fabs(last_result[S_X] - this->s.x) > 0.001 || 
			fabs(last_result[S_Y] - this->s.y) > 0.001 || fabs(last_result[TU] - this->tu) > 0.001);
			
		for (int i = 0; i < 6;i ++)
			this->localization();

		this->nodepointer->x = this->s.x;
		this->nodepointer->y = this->s.y;
		this->nodepointer->z = this->s.z;
		this->nodepointer->timenow = this->tu;
		//对nodepointer 进行补偿
	}
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
	vret.push_back(-VELOCITY);

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
// 	vector<vector<double>> matrixH;
	vector<double> rou;
	vector<double> solution(3,0.0);

	float *matrixH = (float *)malloc(sizeof(float) * 18);
	float *p;

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			*(matrixH + i * 3 + j) = this->getcofficient(this->cplist[i].s).at(j);
		}
	}
		
// 
// 	matrixH.push_back(this->getcofficient(this->cplist[0].s));
// 	matrixH.push_back(this->getcofficient(this->cplist[1].s));
// 	matrixH.push_back(this->getcofficient(this->cplist[2].s));

	rou.push_back(this->getdeltarou(this->cplist[0]));
	rou.push_back(this->getdeltarou(this->cplist[1]));
	rou.push_back(this->getdeltarou(this->cplist[2]));



	//求逆， 考虑不可逆的情况get
	//solution = H^(-1) * rou

	float determ = matrixdeterm(matrixH,3);
	p = matrixH+9;
	if (fabs(determ) > 1e-6) //行列式值不为0，矩阵可逆
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				*(p + j * 3 + i) = make3adjmatrix(matrixH, i, j) / determ;
			}
		}

		printf("H-1\n");
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				printf("%lf\t",*(p+i*3+j));
			}
			printf("\n");
		}
		printf("---------------\n");
		for (int i = 0; i < 3; i++)
		{
			solution[i] = 0;
			for (int j = 0; j < 3; j++)
			{
				solution[i] += rou[j] * (*(p + i * 3 + j));
			}
		}
	}
	else //矩阵不可逆
	{
		printf("matrixH can't be inversed!\n");
		solution[0] = solution[1] = solution[2] = 0;
	}





	//补偿
	this->s.x += solution[0];
	this->s.y += solution[1];
	this->tu += solution[2]; 
	
	printf("s.x = %lf\n", this->s.x);
	printf("s.y = %lf\n", this->s.y);
	printf("tu = %lf\n", this->tu);
	printf("================\n");
}

/*
参数：matrixH 要计算行列式的矩阵H
返回值：矩阵的行列式值
说明：计算参数指向的矩阵的行列式值
*/


float gpssynchr::matrixdeterm(float *p, int n)
{
	int r, c, m;
	int lop ;
	float result = 0;
	float mid = 1;
	if (n == 1)
		return *p;
	lop = n == 2 ? 1 : n;
	for (m = 0; m < lop; m++)
	{
		mid = 1;
		for (r = 0, c = m; r < n; r++, c++)
		{
			mid = mid * (*(p + r*n + c%n));
		}
		result += mid;
	}
	for (m = 0; m < lop; m++)
	{
		mid = 1;
		for (r = 0, c = 2*n-1 - m; r < n; r++, c--)
		{
			mid = mid * (*(p + r*n + c%n));
		}
		result -= mid;
	}
	return result;
}
/*
参数：matrixH 要计算伴随矩阵的矩阵H
	m：矩阵元素的行坐标
	n：矩阵元素的列坐标
返回值：位于矩阵H中坐标为(m,n)的元素的伴随矩阵对应的值
说明：计算矩阵中某元素对应的伴随矩阵位置的值
*/
float gpssynchr::make3adjmatrix(float *p,int m,int n)
{
	int len = 4;
	int i, j;
	float mid_result = 0;
	int sign = 1;
	float *p_create, *p_mid;
	
	p_create = (float *)malloc(sizeof(float)*len);
	p_mid = p_create;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			if (i != m && j != n)
			{
				*p_mid++ = *(p + i*3 + j);
			}
		}
	}
	sign = (m + n) % 2 == 0?1: - 1;
	mid_result = (float)sign * matrixdeterm(p_create,2);
	free(p_create);
	return mid_result;
}


void gpssynchr::print()
{
	printf("x = %lf\n", nodepointer->x);
	printf("y = %lf\n", nodepointer->y);
	printf("z = %lf\n", nodepointer->z);
	printf("timenow = %lf\n", nodepointer->timenow);
}

double edistance(Coordinate c1, Coordinate c2)
{
	return sqrt((c1.x - c2.x) * (c1.x - c2.x) + \
		(c1.y - c2.y) * (c1.y - c2.y) + \
		(c1.z - c2.z) * (c1.z - c2.z));
}

double propagationtime(Coordinate c1, Coordinate c2)
{
	return edistance(c1, c2) / VELOCITY;
}

// #define MAX_X 500
// #define MAX_Y 500
// #define MAX_Z 500
// 
// void createnewpair(underwaternode* node,Coordinate c1, Coordinate c2, Coordinate c3)
// {
// 
// 
// 	
// }


int main()
{
	char a;
	underwaternode* node = new underwaternode(100.0, 200.0, 100.0);
	gpssynchr inanode(node, 320);

	Coordinate target(142, 350, 320, 1.23547);

	Coordinate c1(500.0, 400.0, 0., 1.23547);
	Coordinate c2(250.0, 150.0, 0., 1.23547);
	Coordinate c3(100.0, 200.0, 0., 1.23547);

// 	srand((unsigned)time(NULL));
// 	int coor_count = 0;
// 	while (scanf("%*c")!=-1)
// 	{
// 		Coordinate target(rand() % MAX_X, rand() % MAX_Y, rand() % MAX_Z, 1.23547);
// 		if (coor_count == 0)
// 		{
// 			CoordinatePair p(c1, node->timenow + propagationtime(c1, target)); 
// 			inanode.addnewpair(p);
// 		}
// 		else if (coor_count == 1)
// 		{
// 			CoordinatePair p(c2, node->timenow + propagationtime(c2, target));
// 			inanode.addnewpair(p);
// 		}
// 		else if (coor_count == 2)
// 		{
// 			CoordinatePair p(c3, node->timenow + propagationtime(c3, target));
// 			inanode.addnewpair(p);
// 		}
// 		coor_count = (coor_count + 1) % 3;
// 	
// 	inanode.print();
// 	}

 
  	CoordinatePair p1(c1, node->timenow + propagationtime(c1, target));
  	CoordinatePair p2(c2, node->timenow + propagationtime(c2, target));
  	CoordinatePair p3(c3, node->timenow + propagationtime(c3, target));
 
  	inanode.addnewpair(p1);
  	inanode.addnewpair(p2);
  	inanode.addnewpair(p3);
	inanode.print();
	

	system("pause");
}
