#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <QVTKInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkPolyDataMapper.h>
#include <vtkVolume.h>
#include <vtkTexture.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkScalarBarActor.h>
#include <vtkImagePlaneWidget.h>
#include <vtkImageActor.h>
#include <vtkCubeAxesActor.h>

#include "gconfig.h"
#include "gslicecolormap.h"

class GVisualWidget : public QVTKOpenGLNativeWidget
{
private:
	// 初始化切片
	bool mIsSliceInit = false;              // 切片是否已经初始化
	bool mIsSliceEnable = false;            // 是否开启切片
	bool mXSliceEnable = true;              // 开启X轴切片
	bool mYSliceEnable = true;              // 开启Y轴切片
	bool mZSliceEnable = true;              // 开启Z轴切片

	double mOriginZSpace = 0.0;             // 原始Z方向间距
	double mOriginZRange[2] = {0.0};        // 原始Z方向范围
	double mZScale = 1.0;                   // 深度方向间距缩放系数

    
	vtkSmartPointer<QVTKInteractor> mInteractor;                // VTK交互器   
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> mInteractorStyle;    // VTK交互方式(摄像机鼠标追踪球)
	vtkSmartPointer<vtkRenderWindow> mRenderWindow;             // VTK渲染窗口
	vtkSmartPointer<vtkRenderer> mRenderer;                     // VTK主渲染器

	vtkSmartPointer<vtkImageData> mImageData;                   // 气象三维数据
    vtkSmartPointer<vtkVolume> mVolume;                         // 三维数据体对象
	vtkSmartPointer<vtkGPUVolumeRayCastMapper> mVolumMapper;    // 数据体映射器

	vtkSmartPointer<vtkImageData> mTerrainImage;                // 地面图像数据
	vtkSmartPointer<vtkTexture> mTerrainTexture;                // 地面纹理对象
	vtkSmartPointer<vtkPolyDataMapper> mTerrainMapper;          // 地形映射器
	vtkSmartPointer<vtkActor> mTerrainActor;                    // VTK地形Actor

	// 颜色、透明度传输函数
	vtkSmartPointer<GSliceColorMap> mSliceColorMap;             // 切片颜色传输函数, 自定义类 组合了颜色与透明度的传输函数
	vtkSmartPointer<vtkColorTransferFunction> mColorTF;         // 数据体颜色传输函数
	vtkSmartPointer<vtkPiecewiseFunction> mOpacityTF;           // 数据体透明度传输函数

	vtkSmartPointer<vtkAxesActor> mAxesActor;                   // 左下角坐标系Actor
	vtkSmartPointer<vtkOrientationMarkerWidget> mOmWidget;      // 左下角坐标系控件

	vtkSmartPointer<vtkScalarBarActor> mColorBar;               // 颜色条Actor
	vtkSmartPointer<vtkCubeAxesActor> mCubeAxesActor;           // 坐标系网格Actor

	// 切片平面控件
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneX;          // X切片控件
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneY;          // Y切片控件
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneZ;          // Z切片控件

protected:
	// 初始化切片
	void initSlice();

public:
    GVisualWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~GVisualWidget(); // 继承自虚拟函数

    // 设置数据体颜色传输函数
    void setColorTransferFunction(vtkColorTransferFunction* colorTF);

    // 设置数据体透明度传输函数
    void setOpacityTransferFunction(vtkPiecewiseFunction* opacityTF);

    // 获取数据体颜色传输函数
    vtkColorTransferFunction* getColorTransferFunction();

    // 设置数据体透明度传输函数
    vtkPiecewiseFunction* getOpacityTransferFunction();

    // 设置数据体范围
    void setValueRange(double min, double max);

    // 获取数据体数据
    vtkImageData* getImageData();

    // 设置数据体数据
    void setImageData(vtkImageData* imageData);

    // 设置地形数据与纹理
    void setTerrainData(vtkPolyData* terrainPolyData, vtkImageData* textureData);

    // 重置相机
    void resetCamera();

    // 渲染场景
    void renderScene();

	// 设置渲染配置
	void applyConfig(const GRenderConfig& config);

	// 设置数据体是否可见
	void setVolumnVisibility(bool visibility);

	// 获取数据体是否可见
	bool getVolumnVisibility();

    // 是否开启切片(整体开启)
    bool isSliceEnable();

    // 设置是否开启切片(整体开启)
    void setSliceEnable(bool enable);

    // 设置每根轴是否开启切片
    void setSliceAxisEnable(int axis, bool enable);

    // 设置每根轴的切片索引
    void setSliceAxisIndex(int axis, int pos);

	// 获取切片索引
	void getSliceIndex(int index[3]);

    // 设置Z轴平面裁剪是否开启
    void setZClip(bool enable);

    // 获取Z轴裁剪边界
    void getZClipRange(double* top, double* bottom);

    // 设置Z轴裁剪边界
    void setZClipRange(double top, double bottom);
};