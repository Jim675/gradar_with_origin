#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QActionGroup>
#include <vtkImageGaussianSmooth.h>
#include <vtkWindowToImageFilter.h>
#include <vtkFloatArray.h>
#include <vtkPolyData.h>
#include "tradar3dwnd.h"
#include "trenderconfigdlg.h"
#include "gradarvolume.h"
#include "gvisualwidget.h"
#include "gconfig.h"
#include "tsliceconfigdlg.h"
#include "gvtkutil.h"
#include "gmapcoordconvert.h"
#include "gradaralgorithm.h"
#include "gmath.h"

#include <QDebug>

TRadar3DWnd::TRadar3DWnd(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	mVisualWidget = new GVisualWidget(this);
	setCentralWidget(mVisualWidget);

	QActionGroup* renderGroup = new QActionGroup(this);
	renderGroup->addAction(ui.actShowVolume);
	renderGroup->addAction(ui.actShowSlice);
	ui.actShowVolume->setChecked(true);

	// 设置主窗口大小
	QRect desk = QApplication::desktop()->availableGeometry();
	int width = desk.width() * 0.8;
	int height = desk.height() * 0.8;
	int left = (desk.width() - width) / 2;
	int top = (desk.height() - height) / 2;
	setGeometry(left, top, width, height);

	mRenderConfigDlg = new TRenderConfigDlg(this);
	mRenderConfigDlg->setVisualWidget(mVisualWidget);
	mRenderConfigDlg->setGeometry(left + width - mRenderConfigDlg->width(), top + 30,
		mRenderConfigDlg->width(), mRenderConfigDlg->height());

	mSliceDialog = new TSliceConfigDlg(this);
	mSliceDialog->setGeometry(left + width - mSliceDialog->width(), top + height - mSliceDialog->height() - 30,
		mSliceDialog->width(), mSliceDialog->height());
	connect(mSliceDialog, SIGNAL(sliceChanged(int, int)), this, SLOT(onSliceChange(int, int)));
	connect(mSliceDialog, SIGNAL(sliceEnableChanged(int, bool)), this, SLOT(onSliceAxisEnable(int, bool)));
}

TRadar3DWnd::~TRadar3DWnd()
{
	if (!mSliceDialog) delete mSliceDialog;
	delete mRenderConfigDlg;
	delete mVisualWidget;
}

const GRadarVolume* TRadar3DWnd::getCurRadarData()
{
	if (mRadarDataIndex < 0 || mRadarDataIndex >= mRadarDataList->count()) return nullptr;
	
	return (*this->mRadarDataList)[this->mRadarDataIndex];
}

vtkSmartPointer<vtkImageData> TRadar3DWnd::griddingData(QRectF* rect)
{
	const GRenderConfig& config = GConfig::mRenderConfig;
	const GRadarVolume* data = getCurRadarData();
	if (!data) return nullptr;
	//if (!data || data->surfs.size() == 0) return nullptr;

	auto t0 = std::chrono::steady_clock::now();
	double mx = 0, my = 0;
	GMapCoordConvert::lonLatToMercator(data->longitude, data->latitude, &mx, &my);
	// 计算数据体边界范围
	double x0 = 0, x1 = 0, y0 = 0, y1 = 0, z0 = 0, z1 = 0;
	data->calcBoundBox(x0, x1, y0, y1, z0, z1);

	x0 = mGridRect.left() - mx;
	x1 = mGridRect.right() - mx;
	y0 = mGridRect.top() - my;
	y1 = mGridRect.bottom() - my;

	// 选择框的web墨卡托范围转经纬度坐标
	double lon0 = 0, lat0 = 0;
	GMapCoordConvert::mercatorToLonLat(mGridRect.left(), mGridRect.top(), &lon0, &lat0);
	double lon1 = 0, lat1 = 0;
	GMapCoordConvert::mercatorToLonLat(mGridRect.right(), mGridRect.bottom(), &lon1, &lat1);

	// 选择框的经纬度坐标范围转雷达数据体网格化XY屏幕上的范围（相对于雷达基站的坐标）
	double gx0 = 0, gy0 = 0;
	double gx1 = 0, gy1 = 0;
	GRadarAlgorithm::lonLatToGrid(lon0, lat0, 0,
		data->longitude, data->latitude, data->elev,
		&gx0, &gy0);
	GRadarAlgorithm::lonLatToGrid(lon1, lat1, 0,
		data->longitude, data->latitude, data->elev,
		&gx1, &gy1);
	// 设置网格化范围
	//rect->setRect(x0, y0, x1 - x0, y1 - y0);
	//rect->setRect(gx0, gy0, gx1 - gx0, gy1 - gy0);
	qDebug("web墨卡托网格化范围:");
	cout << "x:[" << x0 << ',' << x1 << ']' << endl;
	cout << "y:[" << y0 << ',' << y1 << ']' << endl;
	cout << "z:[" << z0 << ',' << z1 << ']' << endl;

	// 找到离网格中心最远的点
	double maxX = std::max(abs(gx0), abs(gx1));
	double maxY = std::max(abs(gy0), abs(gy1));
	// 最高仰角
	double maxEl = 0;
	if (data->surfs.size() > 0) {
		maxEl = ANGLE_TO_RADIAN * (data->surfs[data->surfs.size() - 1]->el);
	}
	double z2 = std::min(sqrt(maxX * maxX + maxY * maxY) * tan(maxEl), z1);
	double gz0 = z0;
	double gz1 = z2;
	qDebug("真正的网格化范围:");
	cout << "x:[" << gx0 << ',' << gx1 << ']' << endl;
	cout << "y:[" << gy0 << ',' << gy1 << ']' << endl;
	cout << "z:[" << gz0 << ',' << gz1 << ']' << endl;

	vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();

	int nx = GConfig::mGriddingConfig.mNX;
	int ny = GConfig::mGriddingConfig.mNY;
	int nz = GConfig::mGriddingConfig.mNZ;

	imageData->SetDimensions(nx, ny, nz);
	//double spaceX = static_cast<double>(mMapImage.width()) / nx;
	//double spaceY = static_cast<double>(mMapImage.height()) / ny;

	//double spaceX = static_cast<double>(mMapImage.width()) / nx;
	//double spaceY = static_cast<double>(mMapImage.height()) / ny;
	//double spaceZ = static_cast<double>(gz1 - gz0) * mMapImage.width() / (gx1 - gx0) / nz;

	double range[2] = {};
	mVisualWidget->getColorTransferFunction()->GetRange(range);

	// 获取最高有效Z坐标
	gz1 = data->detectMaxValidZ(gx0, gx1,
		gy0, gy1,
		gz0, gz1,
		nx / 2, ny / 2, nz / 4, // 粗略一点降低计算量
		range[0], range[1]);

	vtkNew<vtkFloatArray> dataArray;
	dataArray->SetNumberOfComponents(1);
	dataArray->SetNumberOfTuples(static_cast<vtkIdType>(nx) * ny * nz);

	// 网格化
	data->gridding(gx0, gx1,
		gy0, gy1,
		gz0, gz1,
		nx, ny, nz,
		-1000, (float*)(dataArray->GetVoidPointer(0)));

	//double spaceX = (gx1 - gx0) / (nx - 1);
	//double spaceY = (gy1 - gy0) / (ny - 1);
	double spaceX = (x1 - x0) / (nx - 1);
	double spaceY = (y1 - y0) / (ny - 1);
	double spaceZ = (gz1 - gz0) / (nz - 1);
	//mSpaceZ = spaceZ;

	cout << "spaceX:" << spaceX << endl
		<< "spaceY:" << spaceY << endl
		<< "spaceZ:" << spaceZ << endl;

	imageData->GetPointData()->SetScalars(dataArray);
	imageData->SetSpacing(spaceX, spaceY, spaceZ);
	imageData->SetOrigin(0, 0, (data->elev + gz0));
	rect->setRect(0, 0, x1 - x0, y1 - y0);
	//imageData->SetOrigin(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), (data->elev + gz0));
	//rect->setRect(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), x1 - x0, y1 - y0);
	//imageData->SetOrigin(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), (data->elev + gz0));
	//auto t2 = std::chrono::steady_clock::now();
	//printDTime(t1, t2, "抽取数据耗时");
	return imageData;
}

// 高程地图转vtkPolyData
vtkSmartPointer<vtkPolyData> TRadar3DWnd::toVtkPolyData(const QRectF& rect) 
{
	const GRenderConfig& config = GConfig::mRenderConfig;

	auto t0 = std::chrono::steady_clock::now();
	const int elevWidth = mElevImage.width();
	const int elevHeight = mElevImage.height();

	// 高程多边形
	vtkSmartPointer<vtkPolyData> elevationPoly = vtkSmartPointer<vtkPolyData>::New();
	// 网格点
	vtkSmartPointer<vtkPoints> elevationPoints = vtkSmartPointer<vtkPoints>::New();
	elevationPoints->SetNumberOfPoints(elevWidth * elevHeight);

	vtkIdType pointIndex = 0;
	// 起点
	double sx = -rect.width()*0.1;
	double sy = -rect.height()*0.1;

	double dx = rect.width()*1.2 / (elevWidth - 1);
	double dy = rect.height()*1.2 / (elevHeight - 1);

	for (int row = 0; row < elevHeight; row++) 
	{
		const double y = sy + row * dy;
		// 有符号16位
		const short* elevLine = (short*)mElevImage.scanLine(elevHeight - row - 1);
		for (int col = 0; col < elevWidth; col++) 
		{
			double elevtion = elevLine[col];
			//if (elevtion == -0x8000) { // 无效值
			//    elevtion = 0;
			//}
			//if (elevtion >= 0x8000) { // 32768
			//    elevtion = elevtion - 0x10000; // 65536
			//    if (elevtion == -0x8000) { // 无效值
			//        elevtion = 0;
			//    }
			//}
			// 设置高程 下降10km
			//elevationPoints->SetPoint(pointIndex++, sx + col * dx, y, elevtion * config.mElevScale - 10000);
			elevationPoints->SetPoint(pointIndex++, sx + col * dx, y, elevtion - 10000);
		}
	}
	elevationPoly->SetPoints(elevationPoints);
	auto t1 = std::chrono::steady_clock::now();
	auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
	cout << "Fill elevation: " << dtime.count() << " milliseconds" << endl;

	//// 三角剖分
	elevationPoly->Allocate((elevWidth - 1) * (elevHeight - 1) * 2);
	vtkIdList* list = vtkIdList::New();
	for (int j = 0; j < elevHeight - 1; j++) {
		vtkIdType b = j * elevWidth;
		vtkIdType t = b + elevWidth;
		for (int i = 0; i < elevWidth - 1; i++) {
			vtkIdType bl = b + i;
			vtkIdType br = bl + 1;
			vtkIdType tl = t + i;
			vtkIdType tr = tl + 1;
			list->Reset();
			list->InsertNextId(tl);
			list->InsertNextId(tr);
			list->InsertNextId(br);
			elevationPoly->InsertNextCell(VTKCellType::VTK_POLYGON, list);
			list->Reset();
			list->InsertNextId(tl);
			list->InsertNextId(bl);
			list->InsertNextId(br);
			elevationPoly->InsertNextCell(VTKCellType::VTK_POLYGON, list);
		}
	}
	list->Delete();

	auto t3 = std::chrono::steady_clock::now();
	dtime = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1);
	cout << "Elevation vtkDelaunay2D: " << dtime.count() << " milliseconds" << endl;
	return elevationPoly;
}

void TRadar3DWnd::setRadarDataIndex(const int index)
{
	if (this->mRadarDataIndex == index) return;
	this->mRadarDataIndex = index;
	if (mIsRendered) 
	{ 
		// 如果已经开始渲染，那么就更新渲染的雷达数据
		render();
	}
}

void TRadar3DWnd::setRadarDataList(const QVector<GRadarVolume*>* radarDataList)
{
	this->mRadarDataList = radarDataList;
	if (mRadarDataList->count() > 0) 
	{
		mRadarDataIndex = 0;
	}
}

void TRadar3DWnd::setMapImage(const QImage* mapImage)
{
	mMapImage = *mapImage;
}

void TRadar3DWnd::setElevImage(const QImage* elevImage)
{
	mElevImage = *elevImage;
}

void TRadar3DWnd::setGridRect(const QRectF& rect)
{
	this->mGridRect = rect;
}

void TRadar3DWnd::setColorTransferFunction(vtkColorTransferFunction* colorTF)
{
	mVisualWidget->setColorTransferFunction(colorTF);
}

#include <QTime>
#include <vtkImageConvolve.h>
void TRadar3DWnd::render()
{
	auto t0 = std::chrono::steady_clock::now();

	QRectF rect;
	QTime timer;
	timer.start();
	mImageData = griddingData(&rect);
	qDebug() << "Griding Data time: " << timer.elapsed();

	if (!mImageData) 
	{
		qDebug() << "griddingData fail";
		return;
	}

	timer.restart();
	vtkNew<vtkImageGaussianSmooth> smoother;
	smoother->SetInputData(mImageData);
	smoother->SetDimensionality(3);
	smoother->SetRadiusFactors(2, 2, 2);
	//smoother->SetSplitMode(BLOCK);
	//smoother->SetSplitModeToBeam();
	smoother->SetStandardDeviation(2);
	smoother->Update();

	/*double kernel[125];
	for (int i = 0; i < 125; i++) kernel[i] = 1.0 / 125;
	vtkNew<vtkImageConvolve> smoother;
	smoother->SetInputData(mImageData);
	smoother->SetKernel5x5x5(kernel);
	smoother->Update();*/

	//vtkNew<vtkImageMedian3D> filter;
	//filter->SetInputData(mImageData);
	//filter->SetKernelSize(3, 3, 3);
	//filter->Update();
	//mImageData = filter->GetOutput();

	mImageData = smoother->GetOutput();
	qDebug() << "Smooth Data time: " << timer.elapsed();
	
	if (!mImageData) 
	{
		qDebug() << "Smooth fail";
		return;
	}
	mVisualWidget->setImageData(mImageData);

	if (!mIsRendered)
	{
		// 如果是第一次显示数据体

		auto t1 = std::chrono::steady_clock::now();
		auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
		cout << "Data volume gridding: " << dtime.count() << " milliseconds" << endl;

		vtkSmartPointer<vtkPolyData> terrainPolyData = toVtkPolyData(rect);
		//vtkPointData* pointData = terrainPolyData->GetPointData();

		//pointData->SetCopyTCoords(true);

		// 计算纹理映射坐标
		vtkNew<vtkFloatArray> tCoords;
		//tCoords->SetName("Texture Coordinates");
		tCoords->SetNumberOfComponents(2);
		const vtkIdType pointsNum = terrainPolyData->GetNumberOfPoints();
		tCoords->SetNumberOfTuples(pointsNum);

		const double x0 = -rect.width() * 0.1;
		const double y0 = -rect.height() * 0.1;
		const double w = rect.width() * 1.2;
		const double h = rect.height() * 1.2;
		double point[3] = { 0 };
		double tcoords[2] = { 0 };
		for (vtkIdType i = 0; i < pointsNum; i++) 
		{
			terrainPolyData->GetPoint(i, point);
			tcoords[0] = (point[0] - x0) / w;
			tcoords[1] = (point[1] - y0) / h;
			tCoords->SetTuple(i, tcoords);
		}
		// 设置纹理映射坐标
		terrainPolyData->GetPointData()->SetTCoords(tCoords);

		auto t2 = std::chrono::steady_clock::now();
		dtime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		cout << "toVtkPolyData + Texture map: " << dtime.count() << " milliseconds" << endl;

		// 创建纹理
		vtkSmartPointer<vtkImageData> terrainImage = VTKUtil::toVtkImageData(&mMapImage);
		mVisualWidget->setTerrainData(terrainPolyData, terrainImage);

		const GRenderConfig& config = GConfig::mRenderConfig;
		mVisualWidget->applyConfig(config);

		mVisualWidget->resetCamera();
		mIsRendered = true;
	}

	mVisualWidget->renderScene();
}

void TRadar3DWnd::on_actRenderSetting_triggered()
{
	mRenderConfigDlg->show();
	//if (configDlg.exec() != QDialog::Accepted) return;

	//mVisualWidget->applyConfig(GConfig::mRenderConfig);
	//mVisualWidget->renderScene();
}

void TRadar3DWnd::on_actSliceSetting_triggered()
{
	auto imageData = mVisualWidget->getImageData();
	if (!imageData) return;

	if (!mVisualWidget->isSliceEnable())
	{
		mVisualWidget->setSliceEnable(true);
	}

	int dims[3];
	imageData->GetDimensions(dims);
	mSliceDialog->setSliceRange(dims[0] - 1, dims[1] - 1, dims[2] - 1);
	int indices[3];
	mVisualWidget->getSliceIndex(indices);
	mSliceDialog->setSlicePos(indices[0], indices[1], indices[2]);

	mSliceDialog->show();
}

void TRadar3DWnd::on_actSaveVolume_triggered()
{
	if (!mVisualWidget->getImageData()) return;

	QString filename = QFileDialog::getSaveFileName(this, "选择保存位置", "", "数据体(*.mha)");
	if (filename.isEmpty()) return;

	VTKUtil::saveImageDataToMeta(mVisualWidget->getImageData(), filename.toStdString().c_str());
}

void TRadar3DWnd::on_actSnap_triggered()
{
	QString filename = QFileDialog::getSaveFileName(this, "选择保存位置", "./", "图像文件 (*.jpg *.png)");
	if (filename.isEmpty()) return;

	if (!filename.endsWith(".jpg", Qt::CaseInsensitive) &&
		!filename.endsWith(".png", Qt::CaseInsensitive))
	{
		filename += ".png";
	}

	vtkNew<vtkWindowToImageFilter> filter;
	filter->SetInput(mVisualWidget->GetRenderWindow());
	//filter->SetScale(1); // 设置缩放系数
	filter->Update();
	vtkImageData* imageData = filter->GetOutput();

	bool ret = false;
	if (filename.endsWith(".jpg", Qt::CaseInsensitive))
	{
		ret = VTKUtil::saveImageDataToJPG(imageData, filename.toStdString().c_str());
	}
	else if (filename.endsWith(".png", Qt::CaseInsensitive)) 
	{
		ret = VTKUtil::saveImageDataToPNG(imageData, filename.toStdString().c_str());
	}

	if (!ret)
	{
		QMessageBox::warning(this, "警告", QString("保存文件: \"%1\" 失败!").arg(filename));
	}
}

void TRadar3DWnd::on_actClose_triggered()
{
	close();
}

void TRadar3DWnd::on_actMovePrev_triggered()
{
	if (!mRadarDataList) return;
	if (mRadarDataList->count() <= 0) return;

	if (mRadarDataIndex > 0) 
	{
		setRadarDataIndex(mRadarDataIndex - 1);
	}
	else
	{
		setRadarDataIndex(mRadarDataList->count() - 1);
	}
}

void TRadar3DWnd::on_actMoveNext_triggered()
{
	if (!mRadarDataList) return;
	if (mRadarDataList->count() <= 0) return;

	if (mRadarDataIndex < mRadarDataList->count()-1)
	{
		setRadarDataIndex(mRadarDataIndex + 1);
	}
	else
	{
		setRadarDataIndex(0);
	}
}

void TRadar3DWnd::on_actShowVolume_toggled(bool checked)
{
	mVisualWidget->setVolumnVisibility(true);
	mVisualWidget->setSliceEnable(false);
	if (mSliceDialog && mSliceDialog->isVisible())
	{
		mSliceDialog->close();
	}
	ui.actSliceSetting->setEnabled(false);
}

void TRadar3DWnd::on_actShowSlice_toggled(bool checked)
{
	mVisualWidget->setVolumnVisibility(false);
	mVisualWidget->setSliceEnable(true);
	ui.actSliceSetting->setEnabled(true);
}

void TRadar3DWnd::onSliceAxisEnable(int axis, bool enable)
{
	mVisualWidget->setSliceAxisEnable(axis, enable);
}

void TRadar3DWnd::onSliceChange(int axis, int pos)
{
	mVisualWidget->setSliceAxisIndex(axis, pos);
}

