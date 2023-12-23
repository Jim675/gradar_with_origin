#pragma once

#include "gshapereader.h"
#include "gshapereader.h"
#include <gmaplayer.h>

#include <fstream>

#include <qvector.h>
#include <qpoint.h>
#include <qrect.h>
#include <qpolygon.h>

// 地名节点
class AddressNode
{
public:
    QString name;
    double x;
    double y;

    AddressNode(QString name, double x, double y):name(name), x(x), y(y)
    {

    }
};

// 用于绘图的Shape类
class ShapeDraw
{
public:
    Shape* mShape;
    QVector<QPolygonF> mShapeList;
    QVector<QRectF> mBoundList;
    QVector<AddressNode> mAddressList;

    ShapeDraw(const QString& shapePath, const QString& addressPath);

    // 读取地址列表
    static QVector<AddressNode> readAddressList(const QString& path);

    ~ShapeDraw()
    {
        delete mShape;
    }
};

class GShapeLayer: public GMapLayer
{
    // 注册节点图层
    GREFLECTION_CLASS(GShapeLayer, GMapLayer);

public:
    GShapeLayer();
    ~GShapeLayer();

private:
    ShapeDraw* mShapeProvince = nullptr;
    ShapeDraw* mShapeCity = nullptr;
    ShapeDraw* mShapeCounty = nullptr;

    QVector<QPolygonF> mProvinceShapeList;
    QVector<QRectF> mProvinceRectList;

    QVector<QPolygonF> mCityShapeList;
    QVector<QRectF> mCityRectList;

    QVector<QPolygonF> mCountyShapeList;
    QVector<QRectF> mCountyRectList;

    // 绘制图层
    virtual void draw(QPainter* painter, const QRect& rect) override;

    // 绘制Shp文件
    // void drawShape(QPainter* painter, const QRect& rect, const Shape* shape);

    // 绘制Point
    void drawPoint(QPainter* painter, const QRect& rect, const Shape* shape);

    // 绘制Line
    void drawLine(QPainter* painter, const QRect& rect, const Shape* shape);

    // 绘制Polygon
    void drawPolygon(QPainter* painter, const QRect& rect, const ShapeDraw* shape);

    // 绘制地址名称
    void drawAddress(QPainter* painter, const QRect& rect, const ShapeDraw* shape);
};

