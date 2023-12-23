#pragma once
#include <qpixmap.h>
#include <qfont.h>

#include <gmaplayer.h>
#include <vtkColorTransferFunction.h>
//#include <vtkDiscretizableColorTransferFunction.h>

// 雷达色标图层
class GColorBarLayer: public GMapLayer
{
    // 注册节点图层
    GREFLECTION_CLASS(GColorBarLayer, GMapLayer)

public:
    GColorBarLayer();
    ~GColorBarLayer();

    // 设置颜色传输函数
    void setColorTransferFunction(vtkColorTransferFunction* mColorTF);

    // 设置标签数量
    void setTagNum(int mTagNum);

    // 设置数值间隔
    void setNumberSpace(double numberSpace);

    // 更新色标
    //void updateColorBar();

    // 绘制色标
    virtual void draw(QPainter* painter, const QRect& rect) override;

private:
    // 色标到边距
    int mLeftMargin = 8;
    int mRightMargin = 24;
    int mTopMargin = 24;
    int mBottomMargin = 24;

    // 色标宽度
    int mBarWidth = 40;
    // 字体尺寸
    int mFontWidth;
    int mFontHeight;
    // 高度系数
    double mHeightFactor = 0.8;

    // 标签数量
    int mTagNum = 0;
    // 数字间隔
    double mNumberSpace = 10.0;

    vtkColorTransferFunction* mColorTF = nullptr;
    QPixmap mPixmap;
    QFont mFont;
};
