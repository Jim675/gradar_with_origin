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
#include <QTime>

#include <QDebug>
#include <QString>
#include <QDir>
#include <QTextStream>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

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

	width = 1728;
	height = 927;
	// int left = 96;
	// int top = 51;
	setGeometry(left, top, width, height);
	move((desk.width() - this->width()) / 2, (desk.height() - this->height()) / 2);

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
	qDebug() << "x:[" << x0 << ',' << x1 << ']' << endl;
	qDebug() << "y:[" << y0 << ',' << y1 << ']' << endl;
	qDebug() << "z:[" << z0 << ',' << z1 << ']' << endl;

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
	qDebug() << "x:[" << gx0 << ',' << gx1 << ']' << endl;
	qDebug() << "y:[" << gy0 << ',' << gy1 << ']' << endl;
	qDebug() << "z:[" << gz0 << ',' << gz1 << ']' << endl;

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

	qDebug() << "spaceX:" << spaceX << endl
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
int TRadar3DWnd::load_PredictImage(GRader2DLayer* layer, QVector<QImage> Image)
{
	QPoint p = layer->getLonpLatp();
	qDebug() << "得到的p：" << p << endl;
	QPointF tpf = layer->cvdptolp(QPoint(p.x(), p.y()));
	double lon1 = 0, lat1 = 0;
	GMapCoordConvert::mercatorToLonLat(tpf.x(), tpf.y(), &lon1, &lat1);
	qDebug() << "PPPlon:" << lon1 << "PPPlat:" << lat1 << endl;
	SLon = lon1;
	SLat = lat1;
	qDebug() << "SLon:" << SLon << "SLat:" << SLat << endl;
	SElev = layer->getElev();
	VEls = layer->getELs();
	Sufnums = VEls.size();
	/*
	for (int i = 0; i < VEls.size(); i++)
	{
		qDebug() << "VVELS:" << VEls[i] << endl;
	}*/

	//QString path = "F:\\radar-dataimage\\predict\\predict\\1\\";


	QVector<QPointF> pointfs;
	QVector<QPoint> vpoint;
	for (int i = 0; i < Sufnums; i++)
	{
		QImage image = Image[i];
		ImageWideth = image.width();
		ImageHeight = image.height();
		qDebug() << "图" << i + 1 << endl;
		for (int i = 0; i < ImageWideth; i++)
		{
			for (int j = 0; j < ImageHeight; j++)
			{
				QColor color = image.pixel(i, j);
				double value = color.red();
				//double value = qGray(image.pixel(i, j));
				values.push_back(value);
				vpoint.push_back(QPoint(i, j));
			}
		}
	}
	//将坐标转换为经纬度
	for (int i = 0; i < vpoint.size(); i++)
	{
		QPointF pf = layer->cvdptolp(vpoint[i]);
		pointfs.push_back(pf);
		double lon = 0, lat = 0;
		GMapCoordConvert::mercatorToLonLat(pf.x(), pf.y(), &lon, &lat);
		lons.push_back(lon);
		lats.push_back(lat);
		if (vpoint[i].x() == p.x() && vpoint[i].y() == p.y() && values[i] != 0)
		{
			qDebug() << "lon:" << lon << "lat:" << lat << endl;
		}
	}
	return 1;
}
int TRadar3DWnd::load_PredictImage(GRader2DLayer* layer,QString rootpath)
{
	QPoint p = layer->getLonpLatp();
	//qDebug() << "得到的p：" << p << endl;
	QPointF tpf = layer->cvdptolp(QPoint(p.x(), p.y()));
	double lon1 = 0, lat1 = 0;
	GMapCoordConvert::mercatorToLonLat(tpf.x(), tpf.y(), &lon1, &lat1);
	//qDebug() << "PPPlon:" << lon1 << "PPPlat:" << lat1 << endl;
	SLon = lon1;
	SLat = lat1;
	//qDebug() << "SLon:" << SLon << "SLat:" << SLat << endl;
	SElev = layer->getElev();
	VEls = layer->getELs();
	Sufnums = VEls.size();
	/*
	for (int i = 0; i < VEls.size(); i++)
	{
		qDebug() << "VVELS:" << VEls[i] << endl;
	}*/

	//QString path = "F:\\radar-dataimage\\predict\\predict\\1\\";
	QString path = rootpath + "/";

	QVector<QPointF> pointfs;
	QVector<QPoint> vpoint;
	for (int i = 0; i < Sufnums; i++)
	{
		QImage image;
		QString tpath = path + QString::number(i + 1) + ".png";
		//image.load(QString(path + QString::number(i+1) + ".png"));
		image.load(tpath);
		//qDebug() << "path:" << tpath << endl;
		if (image.isNull())
			return -1;
		ImageWideth = image.width();
		ImageHeight = image.height();
		qDebug() << "图" << i + 1 << endl;
		for (int i = 0; i < ImageWideth; i++)
		{
			for (int j = 0; j < ImageHeight; j++)
			{
				QColor color = image.pixel(i, j);
				double value = color.red();
				values.push_back(value);
				vpoint.push_back(QPoint(i, j));
			}
		}
	}
	//将坐标转换为经纬度
	for (int i = 0; i < vpoint.size(); i++)
	{
		QPointF pf = layer->cvdptolp(vpoint[i]);
		pointfs.push_back(pf);
		double lon = 0, lat = 0;
		GMapCoordConvert::mercatorToLonLat(pf.x(), pf.y(), &lon, &lat);
		lons.push_back(lon);
		lats.push_back(lat);
		if (vpoint[i].x() == p.x() && vpoint[i].y() == p.y() && values[i] != 0)
		{
			qDebug() << "lon:" << lon << "lat:" << lat << endl;
		}
	}
	return 1;
}

//将数据组合成雷达数据体
void  TRadar3DWnd::CompleteVolume()
{
	qDebug() << "a" << endl;
	int indexh = 0;
	int vsum = 0;
	int elsum = 0;
	double maxlon = 0;
	double maxlat = 0;
	for (int i = 0; i < Sufnums; i++)
	{
		const double te = Els[i];
		const double el = toRadian(te);
		elsum = 0;
		for (int j = 0; j < ImageWideth; j++)
		{
			for (int k = 0; k < ImageHeight; k++)
			{
				//if (values[indexh] != 0)
				if (values[indexh] != 0)
				{
					double gx = 0;
					double gy = 0;
					if (maxlon < lons[indexh])
						maxlon = lons[indexh];
					if (maxlat < lats[indexh])
						maxlat = lats[indexh];
					GRadarAlgorithm::lonLatToGrid(lons[indexh], lats[indexh], 0, SLon, SLat, SElev, &gx, &gy);
					GXS.push_back(gx);
					GYS.push_back(gy);
					double r = sqrt(gx * gx + gy * gy);
					double gz = r * tan(el);
					GZS.push_back(gz);
					gvalues.push_back(values[indexh]);
					vsum++;
					elsum++;
				}
				indexh++;
			}

		}
		oneELnums.push_back(elsum);
		qDebug() << "--elsum:" << elsum << endl;
	}
	qDebug() << "maxlon:" << maxlon << endl;
	qDebug() << "maxlat:" << maxlat << endl;
	qDebug() << "vsum:" << vsum << endl;
	qDebug() << "elsum:" << oneELnums.size() << endl;
	qDebug() << "c" << endl;

}
void TRadar3DWnd::renderPrerdict()
{
	auto t0 = std::chrono::steady_clock::now();
	QRectF rect;
	QTime timer;
	timer.start();
	mImageData = gridingPreData();
	qDebug() << "Griding Data time: " << timer.elapsed();

	if (!mImageData)
	{
		qDebug() << "griddingData fail";
		return;
	}

	//timer.restart();
	/*
	vtkNew<vtkImageGaussianSmooth> smoother;
	smoother->SetInputData(mImageData);
	smoother->SetDimensionality(3);
	smoother->SetRadiusFactors(2, 2, 2);
	//smoother->SetSplitMode(BLOCK);
	//smoother->SetSplitModeToBeam();
	smoother->SetStandardDeviation(2);
	smoother->Update();
	
	mImageData = smoother->GetOutput();
	*/
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
		qDebug() << "Data volume gridding: " << dtime.count() << " milliseconds" << endl;

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
		qDebug() << "toVtkPolyData + Texture map: " << dtime.count() << " milliseconds" << endl;

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
//网格化预测后的数据
vtkSmartPointer<vtkImageData> TRadar3DWnd::gridingPreData()
{
	const GRenderConfig& config = GConfig::mRenderConfig;
	//const GRadarVolume* data = getCurRadarData();
	//if (!data) return nullptr;
	//if (!data || data->surfs.size() == 0) return nullptr;
	double gx1 = -460000.0;
	double gy1 = -460000.0;
	double gx2 = 460000.0;
	double gy2 = 460000.0;

	//最高、最低仰角
	double maxel = VEls[8];
	qDebug() << "maxel:" << maxel << endl;
	double minel = VEls[0];
	qDebug() << "minel:" << minel << endl;
	//double Z = sqrt(mmaxX * mmaxX + mmaxY * mmaxY) * tan(toRadian(maxel));
	double z1 = sqrt(gx2 * gx2 + gy2 * gy2) * tan(toRadian(minel));
	double z2 = sqrt(gx2 * gx2 + gy2 * gy2) * tan(toRadian(maxel));

	double gz1 = SElev + 250;
	double gz2 = z2;
	//
	qDebug() << "minx:" << gx1 << "--" << "maxx:" << gx2 << endl;
	qDebug() << "miny:" << gy1 << "--" << "maxy:" << gy2 << endl;
	qDebug() << "minz:" << gz1 << "--" << "maxz:" << gz2 << endl;


	vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();

	int nx = GConfig::mGriddingConfig.mNX;
	int ny = GConfig::mGriddingConfig.mNY;
	int nz = GConfig::mGriddingConfig.mNZ;

	imageData->SetDimensions(nx, ny, nz);

	double range[2] = {};
	mVisualWidget->getColorTransferFunction()->GetRange(range);


	vtkNew<vtkFloatArray> dataArray;
	dataArray->SetNumberOfComponents(1);
	dataArray->SetNumberOfTuples(static_cast<vtkIdType>(nx) * ny * nz);

	// 网格化
	
	TRadar3DWnd::calGridValuepre(gx1, gx2,
		gy1, gy2,
		gz1, gz2,
		nx, ny, nz,
		(float*)(dataArray->GetVoidPointer(0)));
	//double spaceX = (gx1 - gx0) / (nx - 1);
	//double spaceY = (gy1 - gy0) / (ny - 1);
	double spaceX = (gx2 - gx1) / (nx - 1);
	double spaceY = (gy2 - gy1) / (ny - 1);
	double spaceZ = (gz2 - gz1) / (nz - 1);
	//mSpaceZ = spaceZ;

	qDebug() << "spaceX:" << spaceX << endl
		<< "spaceY:" << spaceY << endl
		<< "spaceZ:" << spaceZ << endl;
	//设置原点
	double ox = -0.5 * spaceX * nx;
	double oy = -0.5 * spaceY * ny;
	double oz = SElev + gz1;

	imageData->GetPointData()->SetScalars(dataArray);
	imageData->SetSpacing(spaceX, spaceY, spaceZ);
	imageData->SetOrigin(0, 0, (SElev + gz1));
	//imageData->SetOrigin(ox, oy, (SElev + gz1));


	return imageData;
}
//计算每点的值
void TRadar3DWnd::calGridValuepre(double minx, double maxx, double miny, double maxy, double minz, double maxz, int nx, int ny, int nz, float* output)
{
	const double dx = (maxx - minx) / nx;;
	const double dy = (maxy - miny) / ny;
	const double dz = (maxz - minz) / nz;
	const int nxy = nx * ny;

	qDebug() << "开始计算值" << endl;
	auto t0 = std::chrono::steady_clock::now();
	int t = 0; 
	double RE = 6378137;
	// 雷达传播圆弧路径的半径为地球半径4倍
	double RN = RE * 4;
#pragma omp parallel for
	for (int iz = 0; iz < nz; iz++) {
		const double gz = minz + iz * dz;
		
		for (int iy = 0; iy < ny; iy++) {
			const double gy = miny + iy * dy;
			const double gy_square = gy * gy;

			for (int ix = 0; ix < nx; ix++) {
				const double gx = minx + ix * dx;

				const double gl = sqrt(gx * gx + gy_square);
				const double el0 = atan2(gz, gl);
				const double r_ = gl / cos(el0);
				const double el1 = asin(0.5 * r_ / RN);
				// 仰角
				const double el = toAngle(el0 + el1);
				const double v = Nearintep(el, gx, gy, gz, dx, dy, dz);
				//double v = IDWinterp(el, gx, gy, gz,dx,dy,dz);
				//qDebug() << "v" << v<<endl;
				output[iz * nxy + iy * ny + ix] = v - 35;
			}
		}
	}
}
//找最近的仰角层
int TRadar3DWnd::findel(double el)
{
	const int size = VEls.size() - 1;
	//首尾的情况
	if (el < VEls[0] || el >= VEls[size])
	{
		return -1;
	}
	//中间情况
	for (int i = 1; i <= size - 1; i++)
	{
		if (el < VEls[i])
		{
			return i - 1;
		}
	}
	return size - 1;
}

double TRadar3DWnd::IDWinterp(double e, double gx, double gy, double gz, double dx, double dy, double dz)
{
	int iel = findel(e);
	if (iel == -1)
		return -1000;
	int size1 = oneELnums[iel];
	int size2 = oneELnums[iel + 1];
	int index1 = 0;
	int index2 = 0;
	int index3 = 0;
	if (iel == 0)
	{
		index1 = 0;
		index2 = size1;
		index3 = index2 + size2;
	} else
	{
		for (int i = 0; i < iel; i++)
		{
			index1 = index1 + oneELnums[i];
		}
		index2 = index1 + size1;
		index3 = index2 + size2;
	}

	//int search_R = 10000;
	int search_R = 2*dx;
	//vector<double> r;//距离
	vector<double> v;//值
	vector<double> w;//权重
	int size = GXS.size();
#pragma omp parallel for
	for (int i = index1; i < index2; i++)
	{
		double tx = gx - GXS[i];
		double ty = gy - GYS[i];
		double tz = gz - GZS[i];
		double d = sqrt(tx * tx) + sqrt(ty * ty) + sqrt(tz * tz);
		if (d <= search_R)
		{
			//r.push_back(d);
			v.push_back(gvalues[i]);
			//double tw = 1 / (d * d);
			double tw = 1 / d;
			w.push_back(tw);
		}
	}
	for (int i = index2; i < index3; i++)
	{
		double tx = gx - GXS[i];
		double ty = gy - GYS[i];
		double tz = gz - GZS[i];
		double d = sqrt(tx * tx) + sqrt(ty * ty) + sqrt(tz * tz);
		if (d <= search_R)
		{
			//r.push_back(d);
			v.push_back(gvalues[i]);
			double tw = 1 / (d * d);
			w.push_back(tw);
			//qDebug() << "gv:" << gvalues[i] << endl;
			//qDebug() << "w:" << tw << endl;
		}
	}
	
	int vsize = v.size();
	//double values;
	double vwsum = 0;
	double wsum = 0;
	for (int i = 0; i < vsize; i++)
	{
		vwsum = vwsum + v[i] * w[i];
		wsum = wsum + w[i];
	}
	if (wsum == 0.0)
		return 0;

	return vwsum / wsum;
}
//最近邻居法找
double TRadar3DWnd::Nearintep(double e, double gx, double gy, double gz, double dx, double dy, double dz)
{
	
	int iel = findel(e);
	if (iel == -1)
		return -1000;
	/*
	int size = GXS.size();
	int index;
	for (int i = 0; i < size; i++)
	{
		if (abs(gx - GXS[i]) < 2 * dx && abs(gy - GYS[i]) < 2 * dy && abs(gz - GZS[i]) < 2 * dz)
		{
			index = i;
			break;
		}
	}
	return gvalues[index];*/
	
	int size1 = oneELnums[iel];
	//int size2 = oneELnums[iel + 1];
	int index1 = 0;
	int index2 = 0;
	//int index3 = 0;
	if (iel == 0)
	{
		index1 = 0;
		index2 = size1;
		//index3 = index2 + size2;
	} else
	{
		for (int i = 0; i < iel; i++)
		{
			index1 = index1 + oneELnums[i];
		}
		index2 = index1 + size1;
		//index3 = index2 + size2;
	}
	for (int i = index1; i < index2; i++)
	{
		if (abs(gx - GXS[i]) < 2 * dx && abs(gy - GYS[i]) < 2 * dy && abs(gz - GZS[i]) < 2 * dz)
		//if (abs(gx - GXS[i]) < 4*dx && abs(gy - GYS[i]) < 4 * dy && abs(gz - GZS[i]) < 4 * dz)
			return gvalues[i];
	}
	return -1000;
}
//读取数据
void TRadar3DWnd::readData()
{
	
	qDebug() << "开始读取预测雷达数据文件" << endl;
	string rootPath = "G:/Z9592.20160914.133505.AR2/";
	//string rootPath = "F:/predModel/res(1)/";

	for (int i = 0; i < 128; i++)
	{
		string path = rootPath + "/" + std::to_string(i) + ".txt";
		std::ifstream data(path);
		float d;
		while (data >> d)
		{
			mVPreData.push_back(d);
		}
		data.close();
	}
	qDebug() << "结束读取预测雷达数据文件" << endl;
	qDebug() << "V.size():" << mVPreData.size()<<endl;
	
	/*
	QString DirPath = QFileDialog::getExistingDirectory(this, "打开文件");
	if (DirPath.isEmpty()) return;

	QDir dir(DirPath);
	QFileInfoList entries = dir.entryInfoList(QDir::Files);
	qDebug() << "aaaaaaaaa" << endl;
	for (auto& entry : entries)
	{
		if (entry.suffix() == "txt")
		{
			QFile file(entry.absoluteFilePath());
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QTextStream in(&file);
				while (!in.atEnd())
				{
					float value = in.readLine().toFloat();
					qDebug() << "bbbbbbbbbb" << endl;
					mVPreData.push_back(value);
				}
				file.close();
			}

		}
	}
	qDebug() << "结束读取预测雷达数据文件" << endl;
	qDebug() << "V.size():" << mVPreData.size() << endl;
	*/
}
//将网格化后的雷达数据每层写入txt文件中
void TRadar3DWnd::wirteData(int nx, int ny, int nz, float* pointer)
{
	QString DirPath = QFileDialog::getExistingDirectory(this, "选择保存位置");
	if (DirPath.isEmpty()) return;

	
	const int nxy = nx * ny;
	/*
	const GRadarVolume* data = getCurRadarData();
	QString qfilename = data->path;
	int index = qfilename.lastIndexOf('/');
	QString fileName = qfilename.mid(index + 1, qfilename.size() - index - 1);
	QString FilePath = DirPath + "/" + fileName;
	QDir* dir = new QDir();
	if (!dir->exists(FilePath))
	{
		dir->mkdir(FilePath);
	}*/
	
	for (int iz = 0; iz < nz; iz++)
	{
		QString index = QString::number(iz);
		QString path = DirPath + "/" + index + ".txt";
		//qDebug() << "path:" << path << endl;
		QFile file(path);
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream out(&file);
		for (int iy = 0; iy < ny; iy++)
		{
			for (int ix = 0; ix < nx; ix++)
			{
				float curvalue = pointer[iz * nxy + iy * ny + ix];
				if (curvalue < -6 || curvalue > 71)
				{
					curvalue = -1000;
				}
				
				out << curvalue << " ";		
			}
			out << "\n";
		}
		file.close();
		
	}

	/*
	//QString filename = QFileDialog::getSaveFileName(this, "选择保存位置", "", "数据体(*.txt)");

	//QString DirPath = QFileDialog::getExistingDirectory(this, "选择保存位置", "");
	//if (DirPath.isEmpty()) return;
	QFileDialog dialog(this, "选择保存位置");
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
		
	}
	QString qDirPath = dialog.selectedFiles().first();
	qDebug() << "DirPath:" << qDirPath << endl;
	string cDirPath = qDirPath.toStdString()+"/";
	cout << cDirPath<<"\n";
	QString q = QString::fromStdString(cDirPath);
	qDebug() << "DirPath2:" << q << endl;

	const int nxy = nx * ny;
	const GRadarVolume* data = getCurRadarData();
	QString qfilename = data->path;
	int index = qfilename.lastIndexOf('/');
	QString fileName = qfilename.mid(index + 1, qfilename.size() - index - 1);
	
	string filename = fileName.toStdString();
	//cout << filename ;
	//string folderPath = "F:/RadarDatas/"+filename;
	//string folderPath = "F:/Radar-data_photo/" + filename;
	string folderPath = cDirPath + filename;
	QString q2 = QString::fromStdString(folderPath);
	qDebug() << "DirPath2:" << q2 << endl;
	for (int iz = 0; iz < nz; iz++)
	{
		string path = folderPath + "/" + to_string(iz) + ".txt";
		ofstream file(path);
		//file.open(path);
		if (file.is_open())
		{
			for (int iy = 0; iy < ny; iy++)
			{
				for (int ix = 0; ix < nx; ix++)
				{
					float curvalue = pointer[iz * nxy + iy * ny + ix];
					file << curvalue << " ";
					//file << " ";
				}
				file << "\n";
			}
		} else
		{
			qDebug() << "Lose<<" << endl;
		}
		file.close();
	}
	*/
}
vtkSmartPointer<vtkImageData> TRadar3DWnd::completeImageData()
{
	const GRenderConfig& config = GConfig::mRenderConfig;
	
	vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();

	int nx = GConfig::mGriddingConfig.mNX;
	int ny = GConfig::mGriddingConfig.mNY;
	int nz = GConfig::mGriddingConfig.mNZ;

	imageData->SetDimensions(nx, ny, nz);
	

	double range[2] = {};
	mVisualWidget->getColorTransferFunction()->GetRange(range);

	vtkNew<vtkFloatArray> dataArray;
	dataArray->SetNumberOfComponents(1);
	dataArray->SetNumberOfTuples(static_cast<vtkIdType>(nx) * ny * nz);

	float* pointer = (float*)(dataArray->GetVoidPointer(0));
	const int nxy = nx * ny;
	int index = 0;
	for (int iz = 0; iz < nz; iz++)
	{
		for (int iy = 0; iy < ny; iy++)
		{
			for (int ix = 0; ix < nx; ix++)
			{
				pointer[iz * nxy + iy * ny + ix] = mVPreData[index];
				index++;
			}
		}
	}

	double spaceX = 4167.03;
	double spaceY = 4263.42;
	double spaceZ = 74.407;
	double OZ = 1167.54;
	qDebug() << "spaceX:" << spaceX << endl
		<< "spaceY:" << spaceY << endl
		<< "spaceZ:" << spaceZ << endl;

	imageData->GetPointData()->SetScalars(dataArray);
	imageData->SetSpacing(spaceX, spaceY, spaceZ);
	imageData->SetOrigin(0, 0, OZ);
	
	//imageData->SetOrigin(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), (data->elev + gz0));
	//rect->setRect(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), x1 - x0, y1 - y0);
	//imageData->SetOrigin(-0.5 * spaceX * (nx - 1), -0.5 * spaceY * (ny - 1), (data->elev + gz0));
	//auto t2 = std::chrono::steady_clock::now();
	//printDTime(t1, t2, "抽取数据耗时");
	return imageData;
}
//将整个雷达数据网格化
vtkSmartPointer<vtkImageData> TRadar3DWnd::grridingdataAll(QRectF* rect)
{
	const GRenderConfig& config = GConfig::mRenderConfig;
	const GRadarVolume* data = getCurRadarData();
	if (!data) return nullptr;
	//if (!data || data->surfs.size() == 0) return nullptr;

	auto t0 = std::chrono::steady_clock::now();
	double mx = 0, my = 0;
	GMapCoordConvert::lonLatToMercator(data->longitude, data->latitude, &mx, &my);

	// 计算数据体边界范围
	auto* surface = data->surfs[0];
	if (surface->bound.isNull()) {
		surface->bound = GRadarAlgorithm::calcMercatorBound(surface, data->longitude, data->latitude, data->elev);
		qDebug("绘制时没有Web墨卡托边界为空");
	}
	QRectF Bounds = surface->bound;
	double left = Bounds.x();
	double right = left + Bounds.width();
	double top = Bounds.y();
	double bottom = top + Bounds.height();
	qDebug() << "left:" << left << " top:" << top << endl;
	qDebug() << "right:" << right << " bottom:" << bottom << endl;

	double x0 = 0, x1 = 0, y0 = 0, y1 = 0, z0 = 0, z1 = 0;
	data->calcBoundBox(x0, x1, y0, y1, z0, z1);

	x0 = left - mx;
	x1 = right - mx;
	y0 = top - my;
	y1 = bottom - my;

	// 选择框的web墨卡托范围转经纬度坐标
	double lon0 = 0, lat0 = 0;
	GMapCoordConvert::mercatorToLonLat(left, top, &lon0, &lat0);
	double lon1 = 0, lat1 = 0;
	GMapCoordConvert::mercatorToLonLat(right, bottom, &lon1, &lat1);

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
	qDebug() << "x:[" << x0 << ',' << x1 << ']' << endl;
	qDebug() << "y:[" << y0 << ',' << y1 << ']' << endl;
	qDebug() << "z:[" << z0 << ',' << z1 << ']' << endl;

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
	qDebug() << "x:[" << gx0 << ',' << gx1 << ']' << endl;
	qDebug() << "y:[" << gy0 << ',' << gy1 << ']' << endl;
	qDebug() << "z:[" << gz0 << ',' << gz1 << ']' << endl;

	vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();

	int nx = GConfig::mGriddingConfig.mNX;
	int ny = GConfig::mGriddingConfig.mNY;
	int nz = GConfig::mGriddingConfig.mNZ;
	//nx = 256 * 2;
	//ny = nx;
	//nz = 256;
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

	float* pointer = dataArray->GetPointer(0);
	const int nxy = nx * ny;


	qDebug() << "开始写文件" << endl;
	TRadar3DWnd::wirteData(nx, ny, nz, pointer);
	qDebug() << "文件写入完成" << endl;

	//TRadar3DWnd::readData();

	//double spaceX = (gx1 - gx0) / (nx - 1);
	//double spaceY = (gy1 - gy0) / (ny - 1);
	double spaceX = (x1 - x0) / (nx - 1);
	double spaceY = (y1 - y0) / (ny - 1);
	double spaceZ = (gz1 - gz0) / (nz - 1);
	//mSpaceZ = spaceZ;

	qDebug() << "spaceX:" << spaceX << endl
		<< "spaceY:" << spaceY << endl
		<< "spaceZ:" << spaceZ << endl;

	imageData->GetPointData()->SetScalars(dataArray);
	imageData->SetSpacing(spaceX, spaceY, spaceZ);
	imageData->SetOrigin(0, 0, (data->elev + gz0));
	qDebug() << "Z：" << data->elev + gz0 << endl;;
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
	qDebug() << "Fill elevation: " << dtime.count() << " milliseconds" << endl;

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
	qDebug() << "Elevation vtkDelaunay2D: " << dtime.count() << " milliseconds" << endl;
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
		qDebug() << "Data volume gridding: " << dtime.count() << " milliseconds" << endl;

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
		qDebug() << "toVtkPolyData + Texture map: " << dtime.count() << " milliseconds" << endl;

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
void TRadar3DWnd::renderAfterPrerdict()
{
	auto t0 = std::chrono::steady_clock::now();
	QRectF rect;
	QTime timer;
	timer.start();
	mImageData = completeImageData();
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
		qDebug() << "Data volume gridding: " << dtime.count() << " milliseconds" << endl;

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
		qDebug() << "toVtkPolyData + Texture map: " << dtime.count() << " milliseconds" << endl;

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
//将整个数据体进行渲染
void TRadar3DWnd::renderAll()
{
	auto t0 = std::chrono::steady_clock::now();

	QRectF rect;
	QTime timer;
	timer.start();
	mImageData = grridingdataAll(&rect);
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
		qDebug() << "Data volume gridding: " << dtime.count() << " milliseconds" << endl;

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
		qDebug() << "toVtkPolyData + Texture map: " << dtime.count() << " milliseconds" << endl;

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
	filter->SetInput(mVisualWidget->renderWindow());
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

