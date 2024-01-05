#ifndef GCVTCOORDINATE_H
#define GCVTCOORDINATE_H

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include "gmapper2d_global.h"

// 坐标转换接口
class GMAPPER2D_EXPORT GCvtCoordinate
{
public:
    // 逻辑X坐标转换到屏幕X坐标
    virtual int	lpXTodpX(qreal lx) const = 0;

    // 逻辑Y坐标转换到屏幕Y坐标
    virtual int	lpYTodpY(qreal ly) const = 0;

    // 屏幕坐标到实际坐标
    virtual qreal dpXTolpX(int dx) const = 0;

    // 屏幕坐标到实际坐标	
    virtual qreal dpYTolpY(int dy) const = 0;

    // 逻辑坐标转换到屏幕坐标
    virtual QPoint lpTodp(const QPointF& lpt) const
    {
        return QPoint(lpXTodpX(lpt.x()), lpYTodpY(lpt.y()));
    }

    // 屏幕坐标转换到逻辑坐标
    virtual QPointF dpTolp(const QPoint& pt) const
    {
        return QPointF(dpXTolpX(pt.x()), dpYTolpY(pt.y()));
    }

    // 逻辑坐标转换到屏幕坐标
    virtual QRect lpTodp(const QRectF& lrc) const
    {
        QPoint pt1 = lpTodp(lrc.topLeft());
        QPoint pt2 = lpTodp(lrc.bottomRight());
        // 通过设置w h来构造Rect<int>
        return QRect(pt1.x(), pt2.y(), pt2.x() - pt1.x(), pt1.y() - pt2.y());
    }

    // 屏幕坐标转换到逻辑坐标
    virtual QRectF dpTolp(const QRect& rc) const
    {
        // 将矩形的左上角坐标通过调用 dpTolp 函数转换为逻辑坐标，并存储在 pt1 中。
        QPointF pt1 = dpTolp(rc.topLeft());
        // QPointF pt2 = dpTolp(rc.bottomRight()); 这种方法转换出来的宽和高都会少1个像素

        // 统一使用宽和高来转换坐标,将矩形的右下角坐标的 x 和 y 坐标转换为逻辑坐标，并分别存储在 x2 和 y2 中。
        qreal x2 = dpXTolpX(rc.x() + rc.width());
        qreal y2 = dpYTolpY(rc.y() + rc.height());
        // 最后，根据转换后的左上角和右下角坐标，构建一个新的逻辑坐标的矩形，并返回
        return QRectF(pt1.x(), y2, x2 - pt1.x(), pt1.y() - y2);
    }
    //////////2023-9-10
      // 逻辑X坐标转换到屏幕X坐标
    virtual int	lpXTodpXc(qreal lx) const = 0;

    // 逻辑Y坐标转换到屏幕Y坐标
    virtual int	lpYTodpYc(qreal ly) const = 0;

    // 屏幕坐标到实际坐标
    virtual qreal dpXTolpXc(int dx) const = 0;

    // 屏幕坐标到实际坐标	
    virtual qreal dpYTolpYc(int dy) const = 0;

    // 逻辑坐标转换到屏幕坐标
    virtual QPoint lpTodpc(const QPointF& lpt) const
    {
        return QPoint(lpXTodpXc(lpt.x()), lpYTodpYc(lpt.y()));
    }

    // 屏幕坐标转换到逻辑坐标
    virtual QPointF dpTolpc(const QPoint& pt) const
    {
        return QPointF(dpXTolpXc(pt.x()), dpYTolpYc(pt.y()));
    }

    // 逻辑坐标转换到屏幕坐标
    virtual QRect lpTodpc(const QRectF& lrc) const
    {
        QPoint pt1 = lpTodpc(lrc.topLeft());
        QPoint pt2 = lpTodpc(lrc.bottomRight());
        // 通过设置w h来构造Rect<int>
        return QRect(pt1.x(), pt2.y(), pt2.x() - pt1.x(), pt1.y() - pt2.y());
    }

    // 屏幕坐标转换到逻辑坐标
    virtual QRectF dpTolpc(const QRect& rc) const
    {
        QPointF pt1 = dpTolpc(rc.topLeft());
        // QPointF pt2 = dpTolp(rc.bottomRight()); 这种方法转换出来的宽和高都会少1个像素

        // 统一使用宽和高来转换坐标
        qreal x2 = dpXTolpXc(rc.x() + rc.width());
        qreal y2 = dpYTolpYc(rc.y() + rc.height());
        return QRectF(pt1.x(), y2, x2 - pt1.x(), pt1.y() - y2);
    }

};

#endif