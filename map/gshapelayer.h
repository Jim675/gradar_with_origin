#pragma once

#include "gshapereader.h"
#include "gshapereader.h"
#include <gmaplayer.h>

#include <fstream>

#include <qvector.h>
#include <qpoint.h>
#include <qrect.h>
#include <qpolygon.h>

// �����ڵ�
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

// ���ڻ�ͼ��Shape��
class ShapeDraw
{
public:
    Shape* mShape;
    QVector<QPolygonF> mShapeList;
    QVector<QRectF> mBoundList;
    QVector<AddressNode> mAddressList;

    ShapeDraw(const QString& shapePath, const QString& addressPath);

    // ��ȡ��ַ�б�
    static QVector<AddressNode> readAddressList(const QString& path);

    ~ShapeDraw()
    {
        delete mShape;
    }
};

class GShapeLayer: public GMapLayer
{
    // ע��ڵ�ͼ��
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

    // ����ͼ��
    virtual void draw(QPainter* painter, const QRect& rect) override;

    // ����Shp�ļ�
    // void drawShape(QPainter* painter, const QRect& rect, const Shape* shape);

    // ����Point
    void drawPoint(QPainter* painter, const QRect& rect, const Shape* shape);

    // ����Line
    void drawLine(QPainter* painter, const QRect& rect, const Shape* shape);

    // ����Polygon
    void drawPolygon(QPainter* painter, const QRect& rect, const ShapeDraw* shape);

    // ���Ƶ�ַ����
    void drawAddress(QPainter* painter, const QRect& rect, const ShapeDraw* shape);
};

