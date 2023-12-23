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

// ��������
int  gRound( double x );

// ȡ��
int  gTrunc( double x );

// ƽ��
double  gSqr( double x );

// 2D����
double gDistance2D(double x0, double y0, double x1, double y1); 

// 2D��ת
void  gRotate(double sinAF, double cosAF, double sx, double sy, double &dx, double &dy);

// ����sin��cosֵȷ���Ƕ�
double  gGetAngle(double sinAF, double cosAF);

// ��3������ʽ��ֵ
double  gDeterm3(double a1, double a2, double a3, double b1, double b2, double b3,
				double c1, double c2, double c3);

// ������С�������
bool  gFittingLine(double *xArray, double *yArray, int firstIndex, int lastIndex,
				  double &a, double &b);

// ��ά��С���������
bool  gFittingPlane(double *x, double *y, double *z, int n, double &a, double &b, double &c);

// �������
bool  gInverseMatrix(double *m, int n);

// ѡ��Ԫ���Ƿֽⷨ
void  gSelSlove(double **matrix, double *result, int dim);	

// ׷�Ϸ�������ԽǾ���
void  gTridagSlove( double *aArray, double *bArray,
					double *cArray, double *dArray,
					int count, double *rArray );

// ׷�Ϸ������չ���ԽǷ��� (������a1��cn����, �ʺϱպ�����ʸ����) 
void  gTridagExSlove(double aArray[], double bArray[],
				   double cArray[], double dArray[],
				   int count, double rArray[]); 

// ��һԪ���η���
int  gSolveTwoeTimeEqu(double a[3], double r[2]);

// ��һԪ���η���
int  gSolveThreeTimeEqu(double a[4], double r[3]);

// ţ�ٵ�������һԪ���η���
double  gNewtonSolveThreeTimeEqu(double a[4], double r, 
							  double errLimit = 0.01, int itCount = 32);

// �����������
// func:	��������ָ��
// a, b:	��������
// eps:		���־���
double  gRombergInt(double (*func)(double), double a, double b, int nh=1, double eps=1e-6);

// �䲽�������
// func:	��������ָ��
// a, b:	��������
// h:		��С����
// eps:		���־���
double  gTrapzInt(double (*func)(double), double a, double b, double h, double eps=1e-6);

#endif
