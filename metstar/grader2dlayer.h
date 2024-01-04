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

// �״�2Dͼ��, ��������ҳ����ʾ�״����ݵĶ�άͼ��
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

    ////////
   //Ԥ��
    // ʵʱ׶���άͶӰ����ֵ�����ɫ����
    std::unique_ptr<uchar[]> PredictimageDataRealTime;

    // ʵʱ׶���άͶӰ����ֵ���ͼ�����
    QImage PredictsurfImageRealTime;

    // �����Ƿ񲥷��״�2D����
    bool mIsAnimate = false;

    // ��ɫӳ�䷶Χ[min, max]
    double mColorRange[2] = {};

    //�״��ļ�����Ϣ����
     //   // �״�վ�㾭γ��
    double mSLon = 0;
    double mSLat = 0;
    //�״�վ����Ļ����
    double Lonp = 0;
    double Latp = 0;
    //�״�վ�㺣�θ߶�
    double mElev = 0;
    //���ݵĲ�����Ϣ
    //����
    int mSurnums = 0;
    vector<double> mELs;
    
    //�ж��Ƿ��Ѿ�����
    int cheekisSave = -1;
    //�洢ͼƬ
    QVector<QImage> mPreImages;

    // ��ǰ�Ƿ���ʾԤ��ͼ��
    bool mPredict = false;

    //�洢ͼ���ī��������
    QVector<QVector<QPointF>> mMXYS;
    QVector<QVector<float>> mValues;
    //��Ļ���굽ʵ������ľ�γ��
    vector<double> mlons;
    vector<double> mlats;
    //������״�վ�������
    vector<double> mGXS;
    vector<double> mGYS;
    vector<double> mGZS;
    //���ֵ
    vector<double> mvalues;
    vector<double> mgvalues;
    vector<int> moneELnums;

    int mImageWideth = 0;
    int mImageHeight = 0;
    //Ԥ�����״�������
    GRadarVolume* mPredictVolume = nullptr;
    //����ԭʼ������
    vector<vector<vector<double>>> mLonsP;
    vector<vector<vector<double>>> mLatsP;
    //1840*1840
    int mCenterX;
    int mCenterY;

    double mScaleLon;
    double mScaleLat;
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

    //����Ԥ��󲻲�ֵ���״�����
    void drawPredictSector(QPainter* painter, const QRect& rect);

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

    //����ת��
    //��Ļת�߼�����
    QPointF cvdptolp(QPoint p);
    //�߼�ת��Ļ����
    QPoint cvlptodp(QPointF p);

    //����ÿһ�����ǵ�ͼƬ
    int drawElpng(QString root);

    //��Ϣ����
    //�����״�վ����Ļ���ꡢ���θ߶ȡ��������Ǹ���������ֵ
    void setLonpLatp(QPoint p);
    //����״�վ����Ļ���ꡢ���θ߶ȡ��������Ǹ���������ֵ
    QPoint getLonpLatp();

    //���ú��θ߶�
    void setElev(double elev);
    //��ú��θ߶�
    double getElev();

    //�����������Ǹ���������ֵ
    void setELs(int surfsnum, vector<double> v);
    //����������Ǹ���������ֵ
    vector<double> getELs();

    //����Ԥ����Ϣ
    
    int savePreInfo(QVector<QVector<QImage>>& Images,int PredictNum);
    // ֱ�ӻ�
    int savePreInfo(QVector<QVector<QImage>>& Images);
    //��ֵ��
    int savePreInfoIn(QVector<QVector<QImage>>& Images);
    //����Ԥ��ͼƬ
    void setPreImage(QVector<QImage>& image);

    //�����Ƿ�������Ԥ��״̬
    void setPredict(bool mIsPredict);
    //��������Ļ����ת��Ϊ�߼�����
    void convertdptolp();

    QVector<QImage> getPreImage()
    {
        return mPreImages;
    }

   //////
    int load_PredictImage(QVector<QImage> image, int surindex);
    int load_PredictImageIn(QVector<QImage> Image, int surfindex);
    //��������ϳ��״�������
    void CompleteVolume();
    
    //����������ǲ�
    int findel(double e, vector<double>& ELs);
    //����ھӷ���
    double NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz,
        vector<double>& lons, vector<double>& lats,
        vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
        vector<int>& oneELnums, vector<double>& ELs);

    //����Ԥ����ֵ���״�����
    void drawPredictInterpolation(QPainter* painter, const QRect& rect);

    // ��surface׶���ֵ, �������imageDataRealTime��surfImageRealTime�������ʾ
    void PredictinterpolateSurfaceRealTime(const GRadarVolume* body, const GRadialSurf* surface,
        const QRectF& bound, size_t width, size_t height, vector<double>& lons, vector<double>& lats,
        vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
        vector<int>& oneELnums, vector<double>& ELs);

    void PredictinterpolateImageOMP(const GRadialSurf* surface, const size_t width, const size_t height,
        const double longitude, const double latitude, const double elev,
        const QRectF& bound, vtkColorTransferFunction* mColorTF,
        double* points, uchar* imageData, vector<double>& lons, vector<double>& lats,
        vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
        vector<int>& oneELnums, vector<double>& ELs);

    void setPredictVolume(GRadarVolume* data)
    {
        this->mPredictVolume = data;
    }
    GRadarVolume* getPredictVolume()
    {
        return this->mPredictVolume;
    }
    void setPredictValue(double value, double* mvalue);

    //����������ǲ�
    int findel(double e);
    //����ھӷ���
    double NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz);
    void CompPredictVolume();
    GRadarVolume* CompPredictVolumeP(int oldsize,int index);
    int getPredict();
    //��������λ��
    void justCenter();

    //���֮ǰ������
    void clearConten();

    //ͼ���С
    int imgwidth = 920;
};

//double adjustAZ(double az, double dlon, double dlat);