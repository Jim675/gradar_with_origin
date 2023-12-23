#pragma once

#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QLineF>
#include <qimage.h>
#include <qtimer.h>
#include <chrono>
#include <memory>
#include <utility>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkColorTransferFunction.h>

#include "gmaplayer.h"
#include "greflection.h"
#include "gradarvolume.h"
#include "gradialbody.h"
#include "gcolorbarlayer.h"

class GNodeSet;
class GRadialVolume;
//class vtkDiscretizableColorTransferFunction;
//class vtkColorTransferFunction;

// �״�2Dͼ��
class GRader2DLayer : public GMapLayer, public QObject
{
    //Q_OBJECT;
    // ע��ڵ�ͼ��
    GREFLECTION_CLASS(GRader2DLayer, GMapLayer)

protected:

    // ��ɫӳ�亯��
    vtkSmartPointer<vtkColorTransferFunction> mColorTF;

    // ��ǰ���ӻ����״�����
    int mRaderIndex = -1;

    // ��ǰ���ӻ����״�׶������
    int mSurfIndex = -1;

    // �״��б�
    const QVector<GRadarVolume*>* mRaderDataList = nullptr;

    // ׶���άͶӰ����ֵ�����ɫ����
    std::unique_ptr<uchar[]> imageData;

    // ׶���άͶӰ����ֵ���ͼ�����
    QImage surfImage;

    // ʵʱ׶���άͶӰ����ֵ�����ɫ����
    std::unique_ptr<uchar[]> imageDataRealTime;

    // ʵʱ׶���άͶӰ����ֵ���ͼ�����
    QImage surfImageRealTime;

    // ��ǰ�Ƿ���ʾ��ֵ����״��άͼ������
    bool mInterpolate = false;

    // �����Ƿ񲥷��״�2D����
    bool mIsAnimate = false;

    // ��ɫӳ�䷶Χ[min, max]
    double mColorRange[2] = {};

protected:
	// ��surface׶���ֵ, �������imageData��surfImage�������ʾ
	void interpolateSurface(const GRadarVolume* body, GRadialSurf* surface);

	// ��surface׶���ֵ, �������imageDataRealTime��surfImageRealTime�������ʾ
	void interpolateSurfaceRealTime(const GRadarVolume* body, const GRadialSurf* surface,
		const QRectF& bound, size_t width, size_t height);

	// ���Ʋ���ֵ���״�����
	void drawSector(QPainter* painter, const QRect& rect);

	// ����ƽ����ֵ���ͼ��
	void drawInterpolation(QPainter* painter, const QRect& rect);

public:
    GRader2DLayer();
    ~GRader2DLayer();

    // ��ȡ��ǰ�״�����
    const GRadarVolume* getCurRaderData() const;

    // �����״��б�
    void setRaderDataList(const QVector<GRadarVolume*>* raderList);

    // ��ȡ�״������б�
    const QVector<GRadarVolume*>* getRaderDataList() const;

    // ��ȡ��ǰ��ʾ���״�����
    int getRaderDataIndex() const;

    // ���õ�ǰ��ʾ���״�����
    void setRaderDataIndex(int index);

    // ��ȡ��ǰ��ʾ��׶������
    int getSurfaceIndex() const;

    // ���õ�ǰ��ʾ��׶������
    void setSurfaceIndex(int index);

    // ��ȡ��ɫ��
    vtkColorTransferFunction* getColorTransferFunction();

	// ������ɫ��
	void setColorTransferFunction(vtkColorTransferFunction* pColorTF);

    // �����Ƿ���ʾ��ֵ����״�ͼ, ���Զ�ˢ��ͼ��
    // isInterpolated: ���Ϊ true ����ʾ��ֵ���ͼ��, ���Ϊ false ����ʾ����ֵ���״�����
    void setInterpolate(bool mIsInterpolated);

    // ����ͼ��
    virtual void draw(QPainter* painter, const QRect& rect) override;

    // �ļ�->��� �ص�����
    void clear();
};

//double adjustAZ(double az, double dlon, double dlat);