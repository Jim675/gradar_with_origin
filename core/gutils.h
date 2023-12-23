#ifndef GUTILS_H
#define GUTILS_H

#include <QString>
#include <qfile.h>
#include <qdatastream.h>
#include <stdlib.h>
#include <ctype.h>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QFontMetrics>
#include <QPixmap>

extern  const QString QStringNull;

// 交换2字节高低顺序
void gSwapByte2(char *to, const char *from);

// 交换4字节高低顺序
void gSwapByte4(char *to, const char *from);

// 在文件流中跳过指定字节
void gSkipPadBytes(QDataStream &stream, int nbytes);

// 文件拷贝
bool gFileCopy(const QString &dst, const QString &src);

// 调整步长
double gAdjustStep(double step0, int &exp);

// 调整步长2
double gAdaptStep(double step);

// 删除目录
bool gDeleteDir(const QString &path );

 // 求两个数中较小者
 template <class TData> TData gGetMin(const TData &x, const TData &y)
 {
	 return x < y ? x : y;
 }

 // 求两个数中较大者
 template <class TData> TData gGetMax(const TData &x, const TData &y)
 {
	 return x > y ? x : y;
 }

 // 求数组中最小元素的下标
 template <class TData> int gGetMin(const TData *data, int begin, int end)
 {
	 int index = begin;
	 TData min = data[index];
	 for(int i=begin+1; i<=end; i++)
	 {
		 if(data[i] < min)
		 {
			 min = data[i];
			 index = i;
		 }
	 }
	 return index;
 }


 // 求数组中最大元素的下标
 template <class TData> int gGetMax(const TData *data, int begin, int end)
 {
	 int index = begin;
	 TData max = data[index];
	 for(int i=begin+1; i<=end; i++)
	 {
		 if(data[i] > max)
		 {
			 max = data[i];
			 index = i;
		 }
	 }
	 return index;
 }

// 交换变量
template <class T>
void gSwap(T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

// 分配1维数组
template <class T>
T * gAlloc1(T *&p, int n1)
{
	p = new T[n1];
	return p;
}

// 释放1维数组
template <class T>
void gFree1(T *&p)
{
	delete []p;
	p = NULL;
}

// 为1维数组填充指定数值v
template <class T>
void gFill1(T *p, T v, int n1)
{
	int i;
	for(i=0; i<n1; i++) p[i] = v;
}

// 1维数组拷贝
template <class T>
void gCopy1(T *dst, const T *src, int n1)
{
	int i;
	for(i=0; i<n1; i++) dst[i] = src[i];
}

// 分配2维数组
template <class T>
T ** gAlloc2(T **&p, int n2, int n1)
{
	int i, size;

	size = sizeof(T);
	p = new T *[n2];
	if(p == NULL) return NULL;

	p[0] = new T[n1*n2];
	if(p[0] == NULL) 
	{
		delete []p;
		p = NULL;
		return NULL;
	}

	for(i=0; i<n2; i++)
	{
		p[i] = p[0] + n1*i;
	}
	return p;
}

// 释放2维数组
template <class T>
void gFree2(T **&p)
{
	delete []p[0];
	delete []p;
	p = NULL;
}

// 2维数组填充指定数值v
template <class T>
void gFill2(T **p, T v, int n2, int n1)
{
	int i, j;
	for(i=0; i<n2; i++) 
	{
		for(j=0; j<n1; j++)
			p[i][j] = v;
	}
}

// 2维数组拷贝
template <class T>
void gCopy2(T **dst, const T **src, int n2, int n1)
{
	int i, j;

	for(i=0; i<n2; i++) 
	{
		for(j=0; j<n1; j++)
			dst[i][j] = src[i][j];
	}
}

// 分配3维数组
template <class T>
T *** gAlloc3(T ***&p, int n3, int n2, int n1)
{
	int i, j;

	p = new T **[n3];
	if(p == NULL) return NULL;

	p[0] = new T *[n3*n2];
	if(p[0] == NULL)
	{
		delete []p;
		p = NULL;
		return NULL;
	}

	p[0][0] = new T [n3*n2*n1];
	if(p[0][0] == NULL)
	{
		delete []p[0];
		delete []p;
		p = NULL;
		return NULL;
	}

	for(i=0; i<n3; i++)
	{
		p[i] = p[0]+n2*i;
		for(j=0; j<n2; j++)
		{
			p[i][j] = p[0][0]+n1*(j+n2*i);
		}
	}

	return p;
}

// 释放三维数组
template <class T>
void gFree3(T ***&p)
{
	delete []p[0][0];
	delete []p[0];
	delete []p;
	p = NULL;
}

// 为3维数组填充指定数值v
template <class T>
void gFill3(T ***p, T v, int n3, int n2, int n1)
{
	int i, j, k;
	for(i=0; i<n3; i++) 
	{	
		for(j=0; j<n2; j++)
		{
			for(k=0; k<n1; k++)
				p[i][j][k] = v;
		}
	}
}

// 3维数组拷贝
template <class T>
void gCopy3(T ***dst, const T ***src, int n3, int n2, int n1)
{
	int i, j, k;

	for(i=0; i<n3; i++) 
	{
		for(j=0; j<n2; j++)
		{
			for(k=0; k<n1; k++)
				dst[i][j][k] = src[i][j][k];
		}
	}
}

#endif
