#ifndef GMAP_LAYER_SET_H
#define GMAP_LAYER_SET_H

#include <QVector>
#include <QPainter>
#include <QDataStream>
#include "gmapper2d_global.h"

class GMapLayer;
class GMapView;
class GCoordConvert;

// ����ͼ�㼯��
class GMAPPER2D_EXPORT GMapLayerSet
{
private:
	QVector<GMapLayer *> mLayerList;		// ͼ���б�
	GMapView *mpView;						// ������ͼ

public:
	GMapLayerSet();
	~GMapLayerSet();

	// ���ͼ��
	void clear();

	// �õ�ͼ����Ŀ
	int count()
	{ return mLayerList.count(); }

	// �õ�ָ���±�ͼ��
	GMapLayer * get(int idx);

	// ����ͼ�����õ�ͼ���±�
	int getLayerIdxByName(const QString &layerName);

	// ����ͼ�����õ�ͼ��
	GMapLayer * getLayerByName(const QString &layerName);

	// ���ͼ��(��ͼ����ӵ��б��ײ�)
	void addLayer(GMapLayer *pLayer);

	// ɾ��ͼ��
	void removeLayer(int index);

	// �ƶ�ͼ��
	void moveLayer(int ifrom, int ito);

	// ����ͼ�㼯��
	void draw(QPainter *p, const QRect &cr);

	// ������ͼ����
	void attachView(GMapView *pView);

	// ����
	bool save(QDataStream &stream);

	// ����, isReplaceָ��ͼ����Դ�ļ��Ƿ����ȱʧ���滻�����
	bool load(QDataStream &stream, bool &isModify);
};

#endif