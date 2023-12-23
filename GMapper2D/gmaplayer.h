#ifndef GMAP_LAYER_H
#define GMAP_LAYER_H

#include <QRectF>
#include <QDataStream>
#include <QPainter>
#include <QMouseEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>

#include "greflection.h"
#include "gmapper2d_global.h"

class GMapView;
class GCoordConvert;

// ��ͼͼ�����
class GMAPPER2D_EXPORT GMapLayer: public GReflection<GMapLayer>
{
    //Q_OBJECT;

    GREFLECTION_CLASS(GMapLayer, GMapLayer)

protected:
    QString		mLayerName;				// ͼ����
    QRectF		mBoundRect;				// ��Χ����
    bool		mIsVisible;				// ͼ���Ƿ�ɼ�
    GMapView* mpView;					// ��ͼ��ͼָ��

public:
    GMapLayer();
    virtual ~GMapLayer();

    // �õ�Layer��
    const QString& getLayerName()
    {
        return mLayerName;
    }

    // ����Layer��
    void setLayerName(const QString& name)
    {
        mLayerName = name;
    }

    // �õ������Χ����
    virtual QRectF getBoundRect()
    {
        return mBoundRect;
    }

    // ����ͼԪ�Ƿ�ɼ�
    bool isVisible()
    {
        return mIsVisible;
    }

    // ����ͼԪ�Ƿ�ɼ�
    void setVisible(bool v)
    {
        mIsVisible = v;
    }

    // ������ͼ����Ŀ
    virtual int subLayerCount()
    {
        return 0;
    }

    // ����ָ���±����ͼ��
    virtual GMapLayer* getSubLayer(int /*idx*/)
    {
        return NULL;
    }

    // ������ͼ����
    virtual void attachView(GMapView* pView)
    {
        mpView = pView;
    }

    // �õ�������ͼ����
    GMapView* getView()
    {
        return mpView;
    }

    // �����༭�Ի���, �༭ͼ��, ����û�ȷ�Ϸ���true
    virtual bool execEditor(QWidget* /*parent*/)
    {
        return true;
    }

    // ����ͼ��
    virtual void draw(QPainter* p, const QRect& cr);

    // ������⺯��, ����ֵ����ͼ���Ƿ��޸�
    virtual void mouseDoubleClickEvent(QMouseEvent* /*e*/)
    {
    }
    virtual void mouseMoveEvent(QMouseEvent* /*e*/)
    {
    }
    virtual void mousePressEvent(QMouseEvent* /*e*/)
    {
    }
    virtual void mouseReleaseEvent(QMouseEvent* /*e*/)
    {
    }

    // �������⺯��, ����ֵ����ͼ���Ƿ��޸�
    virtual void keyPressEvent(QKeyEvent* /*e*/)
    {
    }
    virtual void keyReleaseEvent(QKeyEvent* /*e*/)
    {
    }

    // ִ�в˵�
    virtual void execAction(QAction* /*action*/)
    {
    }

    // �õ�ͼ��˵�, ����NULL��ʾû�в˵�
    virtual QMenu* getMenu()
    {
        return NULL;
    }

    // ��ͼ���ΪActiveͼ��ʱ����
    virtual void start()
    {
    }

    // ͼ���Ϊ��Activeͼ��ʱ����
    virtual void stop()
    {
    }

    // ͼ�㿽��(��ݼ�����)
    virtual void execCopy()
    {
    }

    // ͼ��ճ��(��ݼ�����)
    virtual void execPaste()
    {
    }

    // ����
    virtual bool save(QDataStream& stream);

    // ����
    virtual bool load(QDataStream& stream, bool& isReplace);

    // ����Ҫͨ������ˢ��ͼ��ʱ���ú����ᱻ����
    virtual void refresh()
    {
    }

    // ����Tooltip�ַ���
    virtual QString maybeTip(double lx, double ly);
};

#endif
