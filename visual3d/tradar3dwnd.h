#pragma once

#include <QMainWindow>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vector>
#include "ui_tradar3dwnd.h"
#include "grader2dlayer.h"

class GRadarVolume;
class GVisualWidget;
class GRenderConfig;
class TSliceConfigDlg;
class TRenderConfigDlg;

// 雷达数据三维可视化小窗口
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
	vector<float>                   mVPreData;						// 存储读取预测后的所有雷达数据
public:
	//雷达站点经纬度，高度
	double SLon = 0;
	double SLat = 0;
	double SElev = 96.0;
	//锥面个数
	int Sufnums = 9;
	//每一个锥面的仰角的度数
	double Els[9] = { 0.65,1.34,2.26,3.16,4.15,5.86,9.88,13.84,19.33 };
	vector<double> VEls;

	int ImageWideth = 0;
	int ImageHeight = 0;

	//屏幕坐标到实际坐标的经纬度
	vector<double> lons;
	vector<double> lats;

	//相对于雷达站点的坐标
	vector<double> GXS;
	vector<double> GYS;
	vector<double> GZS;
	//存放值
	vector<double> values;
	vector<double> gvalues;
	vector<int> oneELnums;
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

	//预测前
	//将整个数据体进行渲染
	void renderAll();
	//将整个雷达数据网格化
	vtkSmartPointer<vtkImageData> grridingdataAll(QRectF* rect);
	//将网格化后的雷达数据每层写入txt文件中
	void wirteData(int nx, int ny, int nz, float* pointer);

	
	//预测后
	//将整个数据体进行渲染
	void renderAfterPrerdict();
	//构建ImageData
	vtkSmartPointer<vtkImageData> completeImageData();
	//读取数据
	void readData();


	//经纬度预测
	
	//加载图片
	int load_PredictImage(GRader2DLayer* layer, QString rootpath);
	int load_PredictImage(GRader2DLayer* layer, QVector<QImage> image);
	//将数据组合成雷达数据体
	void CompleteVolume();
	//渲染预测后的雷达数据体
	void renderPrerdict();
	//网格化预测后的数据
	vtkSmartPointer<vtkImageData> gridingPreData();
	//计算每点的值
	void calGridValuepre(double minx, double maxx, double miny, double maxy, double minz, double maxz, int nx, int ny, int nz, float* output);
	//找最近的仰角层
	int findel(double e);
	//最近邻居法找
	double Nearintep(double e, double gx, double gy, double gz, double dx, double dy, double dz);
	//反距离加权插值
	double IDWinterp(double e, double gx, double gy, double gz, double dx, double dy, double dz);
	// 角度转弧度
	constexpr double toRadian(double angle)
	{
		return angle * (3.1415926 / 180.0);
	}

	// 弧度转角度
	constexpr double toAngle(double radian)
	{
		return radian * (180.0 / 3.1415926);
	}
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
