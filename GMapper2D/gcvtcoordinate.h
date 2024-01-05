#ifndef GCVTCOORDINATE_H
#define GCVTCOORDINATE_H

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include "gmapper2d_global.h"

// ����ת���ӿ�
class GMAPPER2D_EXPORT GCvtCoordinate
{
public:
    // �߼�X����ת������ĻX����
    virtual int	lpXTodpX(qreal lx) const = 0;

    // �߼�Y����ת������ĻY����
    virtual int	lpYTodpY(qreal ly) const = 0;

    // ��Ļ���굽ʵ������
    virtual qreal dpXTolpX(int dx) const = 0;

    // ��Ļ���굽ʵ������	
    virtual qreal dpYTolpY(int dy) const = 0;

    // �߼�����ת������Ļ����
    virtual QPoint lpTodp(const QPointF& lpt) const
    {
        return QPoint(lpXTodpX(lpt.x()), lpYTodpY(lpt.y()));
    }

    // ��Ļ����ת�����߼�����
    virtual QPointF dpTolp(const QPoint& pt) const
    {
        return QPointF(dpXTolpX(pt.x()), dpYTolpY(pt.y()));
    }

    // �߼�����ת������Ļ����
    virtual QRect lpTodp(const QRectF& lrc) const
    {
        QPoint pt1 = lpTodp(lrc.topLeft());
        QPoint pt2 = lpTodp(lrc.bottomRight());
        // ͨ������w h������Rect<int>
        return QRect(pt1.x(), pt2.y(), pt2.x() - pt1.x(), pt1.y() - pt2.y());
    }

    // ��Ļ����ת�����߼�����
    virtual QRectF dpTolp(const QRect& rc) const
    {
        // �����ε����Ͻ�����ͨ������ dpTolp ����ת��Ϊ�߼����꣬���洢�� pt1 �С�
        QPointF pt1 = dpTolp(rc.topLeft());
        // QPointF pt2 = dpTolp(rc.bottomRight()); ���ַ���ת�������Ŀ�͸߶�����1������

        // ͳһʹ�ÿ�͸���ת������,�����ε����½������ x �� y ����ת��Ϊ�߼����꣬���ֱ�洢�� x2 �� y2 �С�
        qreal x2 = dpXTolpX(rc.x() + rc.width());
        qreal y2 = dpYTolpY(rc.y() + rc.height());
        // ��󣬸���ת��������ϽǺ����½����꣬����һ���µ��߼�����ľ��Σ�������
        return QRectF(pt1.x(), y2, x2 - pt1.x(), pt1.y() - y2);
    }
    //////////2023-9-10
      // �߼�X����ת������ĻX����
    virtual int	lpXTodpXc(qreal lx) const = 0;

    // �߼�Y����ת������ĻY����
    virtual int	lpYTodpYc(qreal ly) const = 0;

    // ��Ļ���굽ʵ������
    virtual qreal dpXTolpXc(int dx) const = 0;

    // ��Ļ���굽ʵ������	
    virtual qreal dpYTolpYc(int dy) const = 0;

    // �߼�����ת������Ļ����
    virtual QPoint lpTodpc(const QPointF& lpt) const
    {
        return QPoint(lpXTodpXc(lpt.x()), lpYTodpYc(lpt.y()));
    }

    // ��Ļ����ת�����߼�����
    virtual QPointF dpTolpc(const QPoint& pt) const
    {
        return QPointF(dpXTolpXc(pt.x()), dpYTolpYc(pt.y()));
    }

    // �߼�����ת������Ļ����
    virtual QRect lpTodpc(const QRectF& lrc) const
    {
        QPoint pt1 = lpTodpc(lrc.topLeft());
        QPoint pt2 = lpTodpc(lrc.bottomRight());
        // ͨ������w h������Rect<int>
        return QRect(pt1.x(), pt2.y(), pt2.x() - pt1.x(), pt1.y() - pt2.y());
    }

    // ��Ļ����ת�����߼�����
    virtual QRectF dpTolpc(const QRect& rc) const
    {
        QPointF pt1 = dpTolpc(rc.topLeft());
        // QPointF pt2 = dpTolp(rc.bottomRight()); ���ַ���ת�������Ŀ�͸߶�����1������

        // ͳһʹ�ÿ�͸���ת������
        qreal x2 = dpXTolpXc(rc.x() + rc.width());
        qreal y2 = dpYTolpYc(rc.y() + rc.height());
        return QRectF(pt1.x(), y2, x2 - pt1.x(), pt1.y() - y2);
    }

};

#endif