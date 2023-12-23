#pragma once
#include <qpixmap.h>
#include <qfont.h>

#include <gmaplayer.h>
#include <vtkColorTransferFunction.h>
//#include <vtkDiscretizableColorTransferFunction.h>

// �״�ɫ��ͼ��
class GColorBarLayer: public GMapLayer
{
    // ע��ڵ�ͼ��
    GREFLECTION_CLASS(GColorBarLayer, GMapLayer)

public:
    GColorBarLayer();
    ~GColorBarLayer();

    // ������ɫ���亯��
    void setColorTransferFunction(vtkColorTransferFunction* mColorTF);

    // ���ñ�ǩ����
    void setTagNum(int mTagNum);

    // ������ֵ���
    void setNumberSpace(double numberSpace);

    // ����ɫ��
    //void updateColorBar();

    // ����ɫ��
    virtual void draw(QPainter* painter, const QRect& rect) override;

private:
    // ɫ�굽�߾�
    int mLeftMargin = 8;
    int mRightMargin = 24;
    int mTopMargin = 24;
    int mBottomMargin = 24;

    // ɫ����
    int mBarWidth = 40;
    // ����ߴ�
    int mFontWidth;
    int mFontHeight;
    // �߶�ϵ��
    double mHeightFactor = 0.8;

    // ��ǩ����
    int mTagNum = 0;
    // ���ּ��
    double mNumberSpace = 10.0;

    vtkColorTransferFunction* mColorTF = nullptr;
    QPixmap mPixmap;
    QFont mFont;
};
