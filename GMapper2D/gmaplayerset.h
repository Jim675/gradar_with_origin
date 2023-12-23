#ifndef GMAP_LAYER_SET_H
#define GMAP_LAYER_SET_H

#include <QVector>
#include <QPainter>
#include <QDataStream>
#include "gmapper2d_global.h"

class GMapLayer;
class GMapView;
class GCoordConvert;

// 剖面图层集合
class GMAPPER2D_EXPORT GMapLayerSet
{
private:
	QVector<GMapLayer *> mLayerList;		// 图层列表
	GMapView *mpView;						// 剖面视图

public:
	GMapLayerSet();
	~GMapLayerSet();

	// 清空图层
	void clear();

	// 得到图层数目
	int count()
	{ return mLayerList.count(); }

	// 得到指定下标图层
	GMapLayer * get(int idx);

	// 根据图层名得到图层下标
	int getLayerIdxByName(const QString &layerName);

	// 根据图层名得到图层
	GMapLayer * getLayerByName(const QString &layerName);

	// 添加图层(将图层添加到列表首部)
	void addLayer(GMapLayer *pLayer);

	// 删除图层
	void removeLayer(int index);

	// 移动图层
	void moveLayer(int ifrom, int ito);

	// 绘制图层集合
	void draw(QPainter *p, const QRect &cr);

	// 关联视图对象
	void attachView(GMapView *pView);

	// 保存
	bool save(QDataStream &stream);

	// 载入, isReplace指出图层资源文件是否存在缺失或被替换的情况
	bool load(QDataStream &stream, bool &isModify);
};

#endif