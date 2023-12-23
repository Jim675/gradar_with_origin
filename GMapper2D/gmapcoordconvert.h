#pragma once

#include "gmapper2d_global.h"

#include <qpoint.h>
#include <qrect.h>

// 坐标转换类
class GMAPPER2D_EXPORT GMapCoordConvert
{
    // 不生成的一些无用函数, 减少程序大小
    GMapCoordConvert() = delete;
    GMapCoordConvert(const GMapCoordConvert&) = delete;
    GMapCoordConvert(const GMapCoordConvert&&) = delete;
    ~GMapCoordConvert() = delete;

public:
    // 经纬度(角度)转Web墨卡托
    static void lonLatToMercator(double lon, double lat, double* mx, double* my);

    static QPointF lonLatToMercator(const QPointF& point)
    {
        double mx = 0, my = 0;
        lonLatToMercator(point.x(), point.y(), &mx, &my);
        return QPointF(mx, my);
    }

    static QRectF lonLatToMercator(const QRectF& rect)
    {
        QRectF ret;
        double mx = 0, my = 0;
        lonLatToMercator(rect.left(), rect.top(), &mx, &my);
        ret.setLeft(mx);
        ret.setTop(my);

        lonLatToMercator(rect.right(), rect.bottom(), &mx, &my);
        ret.setRight(mx);
        ret.setBottom(my);
        return ret;
    }

    // Web墨卡托转经纬度(角度)
    static void mercatorToLonLat(double mx, double my, double* lon, double* lat);

    static QPointF mercatorToLonLat(const QPointF& point)
    {
        double lon = 0, lat = 0;
        mercatorToLonLat(point.x(), point.y(), &lon, &lat);
        return QPointF(lon, lat);
    }

    static QRectF mercatorToLonLat(const QRectF& rect)
    {
        QRectF ret;
        double lon = 0, lat = 0;
        mercatorToLonLat(rect.left(), rect.top(), &lon, &lat);
        ret.setLeft(lon);
        ret.setTop(lat);

        mercatorToLonLat(rect.right(), rect.bottom(), &lon, &lat);
        ret.setRight(lon);
        ret.setBottom(lat);
        return ret;
    }
};

