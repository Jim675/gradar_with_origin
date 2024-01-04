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

// 雷达2D图层, 用于在主页面显示雷达数据的二维图像
class GRader2DLayer : public GMapLayer, public QObject
{
    //Q_OBJECT;
    // 注册节点图层
    GREFLECTION_CLASS(GRader2DLayer, GMapLayer)

protected:

    // 颜色映射函数
    vtkSmartPointer<vtkColorTransferFunction> mColorTF;

    // 当前可视化的雷达索引
    int mRaderIndex = -1;

    // 当前可视化的雷达锥面索引
    int mSurfIndex = -1;

    // 雷达列表
    const QVector<GRadarVolume*>* mRaderDataList = nullptr;

    // 锥面二维投影并插值后的颜色数据
    std::unique_ptr<uchar[]> imageData;

    // 锥面二维投影并插值后的图像对象
    QImage surfImage;

    // 实时锥面二维投影并插值后的颜色数据
    std::unique_ptr<uchar[]> imageDataRealTime;

    // 实时锥面二维投影并插值后的图像对象
    QImage surfImageRealTime;

    // 当前是否显示插值后的雷达二维图像数据
    bool mInterpolate = false;

    ////////
   //预测
    // 实时锥面二维投影并插值后的颜色数据
    std::unique_ptr<uchar[]> PredictimageDataRealTime;

    // 实时锥面二维投影并插值后的图像对象
    QImage PredictsurfImageRealTime;

    // 设置是否播放雷达2D动画
    bool mIsAnimate = false;

    // 颜色映射范围[min, max]
    double mColorRange[2] = {};

    //雷达文件的信息保存
     //   // 雷达站点经纬度
    double mSLon = 0;
    double mSLat = 0;
    //雷达站点屏幕坐标
    double Lonp = 0;
    double Latp = 0;
    //雷达站点海拔高度
    double mElev = 0;
    //数据的部分信息
    //仰角
    int mSurnums = 0;
    vector<double> mELs;
    
    //判断是否已经保存
    int cheekisSave = -1;
    //存储图片
    QVector<QImage> mPreImages;

    // 当前是否显示预测图像
    bool mPredict = false;

    //存储图像的墨卡托坐标
    QVector<QVector<QPointF>> mMXYS;
    QVector<QVector<float>> mValues;
    //屏幕坐标到实际坐标的经纬度
    vector<double> mlons;
    vector<double> mlats;
    //相对于雷达站点的坐标
    vector<double> mGXS;
    vector<double> mGYS;
    vector<double> mGZS;
    //存放值
    vector<double> mvalues;
    vector<double> mgvalues;
    vector<int> moneELnums;

    int mImageWideth = 0;
    int mImageHeight = 0;
    //预测后的雷达数据体
    GRadarVolume* mPredictVolume = nullptr;
    //保存原始的坐标
    vector<vector<vector<double>>> mLonsP;
    vector<vector<vector<double>>> mLatsP;
    //1840*1840
    int mCenterX;
    int mCenterY;

    double mScaleLon;
    double mScaleLat;
protected:
	// 对surface锥面插值, 结果存在imageData和surfImage供随后显示
	void interpolateSurface(const GRadarVolume* body, GRadialSurf* surface);

	// 对surface锥面插值, 结果存在imageDataRealTime和surfImageRealTime供随后显示
	void interpolateSurfaceRealTime(const GRadarVolume* body, const GRadialSurf* surface,
		const QRectF& bound, size_t width, size_t height);

	// 绘制不插值的雷达扇区
	void drawSector(QPainter* painter, const QRect& rect);

	// 绘制平滑插值后的图像
	void drawInterpolation(QPainter* painter, const QRect& rect);

    //绘制预测后不插值的雷达扇区
    void drawPredictSector(QPainter* painter, const QRect& rect);

public:
    GRader2DLayer();
    ~GRader2DLayer();

    // 获取当前雷达数据
    const GRadarVolume* getCurRaderData() const;

    // 设置雷达列表
    void setRaderDataList(const QVector<GRadarVolume*>* raderList);

    // 获取雷达数据列表
    const QVector<GRadarVolume*>* getRaderDataList() const;

    // 获取当前显示的雷达索引
    int getRaderDataIndex() const;

    // 设置当前显示的雷达索引
    void setRaderDataIndex(int index);

    // 获取当前显示的锥面索引
    int getSurfaceIndex() const;

    // 设置当前显示的锥面索引
    void setSurfaceIndex(int index);

    // 获取颜色表
    vtkColorTransferFunction* getColorTransferFunction();

	// 设置颜色表
	void setColorTransferFunction(vtkColorTransferFunction* pColorTF);

    // 设置是否显示插值后的雷达图, 会自动刷新图层
    // isInterpolated: 如果为 true 则显示插值后的图像, 如果为 false 则显示不插值的雷达扇区
    void setInterpolate(bool mIsInterpolated);

    // 绘制图层
    virtual void draw(QPainter* painter, const QRect& rect) override;

    // 文件->清空 回调函数
    void clear();

    //坐标转换
    //屏幕转逻辑坐标
    QPointF cvdptolp(QPoint p);
    //逻辑转屏幕坐标
    QPoint cvlptodp(QPointF p);

    //绘制每一个仰角的图片
    int drawElpng(QString root);

    //信息保存
    //设置雷达站点屏幕坐标、海拔高度、数据仰角个数和仰角值
    void setLonpLatp(QPoint p);
    //获得雷达站点屏幕坐标、海拔高度、数据仰角个数和仰角值
    QPoint getLonpLatp();

    //设置海拔高度
    void setElev(double elev);
    //获得海拔高度
    double getElev();

    //设置数据仰角个数和仰角值
    void setELs(int surfsnum, vector<double> v);
    //获得数据仰角个数和仰角值
    vector<double> getELs();

    //保存预测信息
    
    int savePreInfo(QVector<QVector<QImage>>& Images,int PredictNum);
    // 直接画
    int savePreInfo(QVector<QVector<QImage>>& Images);
    //插值画
    int savePreInfoIn(QVector<QVector<QImage>>& Images);
    //设置预测图片
    void setPreImage(QVector<QImage>& image);

    //设置是否现在是预测状态
    void setPredict(bool mIsPredict);
    //将所有屏幕坐标转化为逻辑坐标
    void convertdptolp();

    QVector<QImage> getPreImage()
    {
        return mPreImages;
    }

   //////
    int load_PredictImage(QVector<QImage> image, int surindex);
    int load_PredictImageIn(QVector<QImage> Image, int surfindex);
    //将数据组合成雷达数据体
    void CompleteVolume();
    
    //找最近的仰角层
    int findel(double e, vector<double>& ELs);
    //最近邻居法找
    double NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz,
        vector<double>& lons, vector<double>& lats,
        vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
        vector<int>& oneELnums, vector<double>& ELs);

    //绘制预测后插值的雷达扇区
    void drawPredictInterpolation(QPainter* painter, const QRect& rect);

    // 对surface锥面插值, 结果存在imageDataRealTime和surfImageRealTime供随后显示
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

    //找最近的仰角层
    int findel(double e);
    //最近邻居法找
    double NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz);
    void CompPredictVolume();
    GRadarVolume* CompPredictVolumeP(int oldsize,int index);
    int getPredict();
    //调整中心位置
    void justCenter();

    //清空之前的内容
    void clearConten();

    //图像大小
    int imgwidth = 920;
};

//double adjustAZ(double az, double dlon, double dlat);