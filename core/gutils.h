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

// ����2�ֽڸߵ�˳��
void gSwapByte2(char *to, const char *from);

// ����4�ֽڸߵ�˳��
void gSwapByte4(char *to, const char *from);

// ���ļ���������ָ���ֽ�
void gSkipPadBytes(QDataStream &stream, int nbytes);

// �ļ�����
bool gFileCopy(const QString &dst, const QString &src);

// ��������
double gAdjustStep(double step0, int &exp);

// ��������2
double gAdaptStep(double step);

// ɾ��Ŀ¼
bool gDeleteDir(const QString &path );

 // ���������н�С��
 template <class TData> TData gGetMin(const TData &x, const TData &y)
 {
	 return x < y ? x : y;
 }

 // ���������нϴ���
 template <class TData> TData gGetMax(const TData &x, const TData &y)
 {
	 return x > y ? x : y;
 }

 // ����������СԪ�ص��±�
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


 // �����������Ԫ�ص��±�
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

// ��������
template <class T>
void gSwap(T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

// ����1ά����
template <class T>
T * gAlloc1(T *&p, int n1)
{
	p = new T[n1];
	return p;
}

// �ͷ�1ά����
template <class T>
void gFree1(T *&p)
{
	delete []p;
	p = NULL;
}

// Ϊ1ά�������ָ����ֵv
template <class T>
void gFill1(T *p, T v, int n1)
{
	int i;
	for(i=0; i<n1; i++) p[i] = v;
}

// 1ά���鿽��
template <class T>
void gCopy1(T *dst, const T *src, int n1)
{
	int i;
	for(i=0; i<n1; i++) dst[i] = src[i];
}

// ����2ά����
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

// �ͷ�2ά����
template <class T>
void gFree2(T **&p)
{
	delete []p[0];
	delete []p;
	p = NULL;
}

// 2ά�������ָ����ֵv
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

// 2ά���鿽��
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

// ����3ά����
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

// �ͷ���ά����
template <class T>
void gFree3(T ***&p)
{
	delete []p[0][0];
	delete []p[0];
	delete []p;
	p = NULL;
}

// Ϊ3ά�������ָ����ֵv
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

// 3ά���鿽��
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
