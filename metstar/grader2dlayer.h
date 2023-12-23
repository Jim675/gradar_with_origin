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

// 雷达2D图层
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

    // 设置是否播放雷达2D动画
    bool mIsAnimate = false;

    // 颜色映射范围[min, max]
    double mColorRange[2] = {};

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
};

//double adjustAZ(double az, double dlon, double dlat);