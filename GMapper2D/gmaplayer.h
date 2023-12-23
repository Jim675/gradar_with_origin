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

// 地图图层基类
class GMAPPER2D_EXPORT GMapLayer: public GReflection<GMapLayer>
{
    //Q_OBJECT;

    GREFLECTION_CLASS(GMapLayer, GMapLayer)

protected:
    QString		mLayerName;				// 图层名
    QRectF		mBoundRect;				// 包围矩形
    bool		mIsVisible;				// 图层是否可见
    GMapView* mpView;					// 地图视图指针

public:
    GMapLayer();
    virtual ~GMapLayer();

    // 得到Layer名
    const QString& getLayerName()
    {
        return mLayerName;
    }

    // 设置Layer名
    void setLayerName(const QString& name)
    {
        mLayerName = name;
    }

    // 得到剖面包围矩形
    virtual QRectF getBoundRect()
    {
        return mBoundRect;
    }

    // 返回图元是否可见
    bool isVisible()
    {
        return mIsVisible;
    }

    // 设置图元是否可见
    void setVisible(bool v)
    {
        mIsVisible = v;
    }

    // 返回子图层数目
    virtual int subLayerCount()
    {
        return 0;
    }

    // 返回指定下标的子图层
    virtual GMapLayer* getSubLayer(int /*idx*/)
    {
        return NULL;
    }

    // 关联视图对象
    virtual void attachView(GMapView* pView)
    {
        mpView = pView;
    }

    // 得到关联视图对象
    GMapView* getView()
    {
        return mpView;
    }

    // 弹出编辑对话框, 编辑图层, 如果用户确认返回true
    virtual bool execEditor(QWidget* /*parent*/)
    {
        return true;
    }

    // 绘制图层
    virtual void draw(QPainter* p, const QRect& cr);

    // 鼠标虚拟函数, 返回值表明图层是否被修改
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

    // 键盘虚拟函数, 返回值表明图层是否被修改
    virtual void keyPressEvent(QKeyEvent* /*e*/)
    {
    }
    virtual void keyReleaseEvent(QKeyEvent* /*e*/)
    {
    }

    // 执行菜单
    virtual void execAction(QAction* /*action*/)
    {
    }

    // 得到图层菜单, 返回NULL表示没有菜单
    virtual QMenu* getMenu()
    {
        return NULL;
    }

    // 将图层变为Active图层时调用
    virtual void start()
    {
    }

    // 图层变为非Active图层时调用
    virtual void stop()
    {
    }

    // 图层拷贝(快捷键调用)
    virtual void execCopy()
    {
    }

    // 图层粘贴(快捷键调用)
    virtual void execPaste()
    {
    }

    // 保存
    virtual bool save(QDataStream& stream);

    // 载入
    virtual bool load(QDataStream& stream, bool& isReplace);

    // 当需要通过数据刷新图层时，该函数会被调起
    virtual void refresh()
    {
    }

    // 返回Tooltip字符串
    virtual QString maybeTip(double lx, double ly);
};

#endif
