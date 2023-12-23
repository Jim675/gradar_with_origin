#pragma once

#include <QMainWindow>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include "ui_tradar3dwnd.h"

class GRadarVolume;
class GVisualWidget;
class GRenderConfig;
class TSliceConfigDlg;
class TRenderConfigDlg;

class TRadar3DWnd : public QMainWindow
{
	Q_OBJECT

private:
	Ui::TRadar3DWndClass			ui;
	int								mRadarDataIndex = -1;			// 当前显示雷达数据体下标
	const QVector<GRadarVolume*>*	mRadarDataList = nullptr;		// 雷达数据体列表
	vtkSmartPointer<vtkImageData>	mImageData;						// VTK图像数据体
	GVisualWidget *					mVisualWidget = nullptr;		// 可视化控件
	QImage							mMapImage;						// 地图图像
	QImage							mElevImage;						// 高程图像
	QRectF							mGridRect;						// 网格化范围（web墨卡托投影坐标）
	bool							mIsRendered = false;			// 是否已经渲染过
	TSliceConfigDlg *				mSliceDialog = nullptr;			// 切片设置对话框
	TRenderConfigDlg *				mRenderConfigDlg = nullptr;		// 渲染设置对话框

protected:
	// 获取当前雷达数据
	const GRadarVolume* getCurRadarData();

	// rect:网格化范围(相对于雷达中心坐标)
	vtkSmartPointer<vtkImageData> griddingData(QRectF* rect);

	// 地图、高程转vtkPolyData
	vtkSmartPointer<vtkPolyData> toVtkPolyData(const QRectF& rect);

public:
	TRadar3DWnd(QWidget *parent = nullptr);
	virtual ~TRadar3DWnd();

	// 设置雷达数据索引
	void setRadarDataIndex(const int index);

	// 设置雷达数据列表
	void setRadarDataList(const QVector<GRadarVolume*>* radarDataList);

	// 设置地图图像
	void setMapImage(const QImage* mapImage);

	// 设置高程图像
	void setElevImage(const QImage* elevImage);

	// 设置网格化范围（web墨卡托投影坐标）
	void setGridRect(const QRectF& rect);

	// 设置颜色传输函数
	void setColorTransferFunction(vtkColorTransferFunction* colorTF);

	// 渲染雷达数据体
	void render();

private slots:

	// 渲染设置
	void on_actRenderSetting_triggered();

	// 切片设置
	void on_actSliceSetting_triggered();

	// 保存数据体
	void on_actSaveVolume_triggered();

	// 捕捉当前屏幕并存盘
	void on_actSnap_triggered();

	// 关闭窗口
	void on_actClose_triggered();

	// 上一个
	void on_actMovePrev_triggered();

	// 下一个
	void on_actMoveNext_triggered();

	// 显示数据体
	void on_actShowVolume_toggled(bool checked);

	// 显示切片
	void on_actShowSlice_toggled(bool checked);

	// 切片开关回调
	void onSliceAxisEnable(int axis, bool enable);

	// 切片位置变化回调
	void onSliceChange(int axis, int pos);
};
