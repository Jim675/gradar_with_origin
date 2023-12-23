#pragma once
#include "gpredrnn.h"
#include "qdebug.h"
#include <qdir.h>
#include <QImage>
#include <QVector>
#include <iostream>

class GPredStart
{
public:
	QVector<QVector<QImage>> Images;
	QVector<QImage>   res;
public:
	//void start(string path,string svaepath);
	//void startPre(QVector<QVector<QImage>> images, QVector<QImage>& res);
	void startPre(QVector<QVector<QImage>> images);

	//�õ�����
	void setImages(QVector<QVector<QImage>>& Images);

	//�õ�Ԥ��������
	QVector<QImage> getImages();
};

