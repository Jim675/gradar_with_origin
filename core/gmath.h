#ifndef GMATH_H
#define GMATH_H

#include <math.h>

class GVector2d;
class GVector3d;
class GPoint2d;
class GPoint3d;
class GPolyLine2d;
class GRect2d;

#define PI 3.1415926535897932384626433832795
#define RADIAN_TO_ANGLE 57.29577958
#define ANGLE_TO_RADIAN 0.01745329
#define GMATH_ZERO		1.0E-8
#define IS_ZERO(x)		(fabs(x) < GMATH_ZERO)
#define IRD(x)	((unsigned __int64 &)x)

// 四舍五入
int  gRound( double x );

// 取整
int  gTrunc( double x );

// 平方
double  gSqr( double x );

// 2D距离
double gDistance2D(double x0, double y0, double x1, double y1); 

// 2D旋转
void  gRotate(double sinAF, double cosAF, double sx, double sy, double &dx, double &dy);

// 根据sin和cos值确定角度
double  gGetAngle(double sinAF, double cosAF);

// 求3阶行列式的值
double  gDeterm3(double a1, double a2, double a3, double b1, double b2, double b3,
				double c1, double c2, double c3);

// 线性最小二乘拟合
bool  gFittingLine(double *xArray, double *yArray, int firstIndex, int lastIndex,
				  double &a, double &b);

// 二维最小二乘面拟合
bool  gFittingPlane(double *x, double *y, double *z, int n, double &a, double &b, double &c);

// 求逆矩阵
bool  gInverseMatrix(double *m, int n);

// 选主元三角分解法
void  gSelSlove(double **matrix, double *result, int dim);	

// 追赶法求解三对角矩阵
void  gTridagSlove( double *aArray, double *bArray,
					double *cArray, double *dArray,
					int count, double *rArray );

// 追赶法求解扩展三对角方程 (方程中a1与cn存在, 适合闭合三切矢方程) 
void  gTridagExSlove(double aArray[], double bArray[],
				   double cArray[], double dArray[],
				   int count, double rArray[]); 

// 解一元二次方程
int  gSolveTwoeTimeEqu(double a[3], double r[2]);

// 解一元三次方程
int  gSolveThreeTimeEqu(double a[4], double r[3]);

// 牛顿迭代法解一元三次方程
double  gNewtonSolveThreeTimeEqu(double a[4], double r, 
							  double errLimit = 0.01, int itCount = 32);

// 龙贝格求积法
// func:	被积函数指针
// a, b:	积分区间
// eps:		积分精度
double  gRombergInt(double (*func)(double), double a, double b, int nh=1, double eps=1e-6);

// 变步长求积法
// func:	被积函数指针
// a, b:	积分区间
// h:		最小步长
// eps:		积分精度
double  gTrapzInt(double (*func)(double), double a, double b, double h, double eps=1e-6);

#endif
