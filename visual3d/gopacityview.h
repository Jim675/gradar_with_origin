#pragma once

#include "ggraphview.h"
#include "gopacitytable.h"


class GOpacityView : public GGraphView
{
    Q_OBJECT

protected:
    GOpacityTable		mOpacityTable;
    bool				mIsLButtonDown;
    int					mSelIndex;
    double              mGainMinValue;
    double              mGainMaxValue;

protected:

    void drawKeyPoints(QPainter* p);

    void drawOpacity(QPainter* p);

    void drawGainValue(QPainter* p);

    // 绘图事件
    virtual void paintEvent(QPaintEvent* e);

    int ptInPoint(int x, int y, int diff = 6);

    int ptInRange(int x, int diff = 4);

    virtual void mousePressEvent(QMouseEvent* e);	//鼠标按压事件
    virtual void mouseMoveEvent(QMouseEvent* e);	//鼠标移动事件
    virtual void mouseReleaseEvent(QMouseEvent*);	//鼠标释放事件

    // 鼠标双击
    virtual void mouseDoubleClickEvent(QMouseEvent* e);


public:
    GOpacityView(QWidget* parent = 0, Qt::WindowFlags f = 0);

    virtual ~GOpacityView();

    void setOpacityTable(const GOpacityTable& table);

    const GOpacityTable& getOpacityTable();

    void setFuncType(int funcType);

    void setClamping(bool clamping);

    double getGainMinValue();

    double getGainMaxValue();

    void setGainMinValue(double v);

    void setGainMaxValue(double v);

signals:

    void valueChanged(int type);
};
