#include "gvisualwidget.h"

#include <vtkVolumeProperty.h>
#include <vtkPointData.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkProperty.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkImageSliceMapper.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>

#include <qdebug.h>
//#include <vtkProp3D.h>

constexpr double operator"" _rgb(unsigned long long x)
{
    return static_cast<double>(x) / 255.0;
}

GVisualWidget::GVisualWidget(QWidget* parent, Qt::WindowFlags flags) :QVTKOpenGLNativeWidget(parent, flags)
{
    // mInteractor = vtkSmartPointer<QVTKInteractor>::New(); // 自己New出来再设置给其他对象可能会导致内存泄漏
    mInteractor = interactor();
    mInteractor->Initialize();

    mInteractorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    mInteractor->SetInteractorStyle(mInteractorStyle);
    //mInteractorStyle->SetEnabled(true); // 默认不能交互 似乎没有起作用

    //mRenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    mRenderWindow = renderWindow();
    //mRenderWindow->Initialize();
    //mRenderWindow->SetInteractor(mInteractor);

    mRenderer = vtkSmartPointer<vtkRenderer>::New();
    mRenderer->SetBackground(0.0, 0.0, 0.0);
    mRenderer->SetViewport(0.0, 0.0, 1.0, 1.0);
    mRenderWindow->AddRenderer(mRenderer);

    mColorTF = vtkSmartPointer<vtkColorTransferFunction>::New();
    mColorTF->SetAllowDuplicateScalars(false);
    mColorTF->SetClamping(false);

    mOpacityTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
    mOpacityTF->SetAllowDuplicateScalars(false);
    mOpacityTF->SetClamping(false);

    mSliceColorMap = vtkSmartPointer<GSliceColorMap>::New();
    mSliceColorMap->SetColorTransferFunction(mColorTF);
    mSliceColorMap->SetOpacityTransferFunction(mOpacityTF);

    mVolume = vtkSmartPointer<vtkVolume>::New();
    mVolume->SetVisibility(false); // 默认隐藏
    mRenderer->AddActor(mVolume);

    vtkVolumeProperty* volumeProperty = mVolume->GetProperty();
    volumeProperty->SetColor(mColorTF);
    volumeProperty->SetScalarOpacity(mOpacityTF);
    volumeProperty->SetScalarOpacityUnitDistance(1000); // 设置透明度单位距离

    mVolumMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    mVolumMapper->SetBlendModeToComposite();
    mVolumMapper->SetCropping(false);
    mVolumMapper->SetCroppingRegionFlagsToSubVolume();
    mVolume->SetMapper(mVolumMapper);

    mAxesActor = vtkSmartPointer<vtkAxesActor>::New();

    mOmWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    mOmWidget->SetOrientationMarker(mAxesActor);
    mOmWidget->SetInteractor(mInteractor); // 会增加一个Renderer到RenderWindow
    //mOmWidget->SetOutlineColor(1.0, 0.0, 0.0);
    mOmWidget->SetViewport(0.0, 0.0, 0.15, 0.15); // 似乎只能设置比例，不能设置绝对像素
    //mOmWidget->SetEnabled(true);
    //mOmWidget->SetInteractive(false);
    mOmWidget->SetTolerance(0); // 鼠标到控件的容差
	mOmWidget->SetEnabled(true);
	mOmWidget->SetInteractive(false);

    mColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
    mColorBar->SetLookupTable(mColorTF);
    mColorBar->SetNumberOfLabels(8); // 设置颜色条标签数量
    mRenderer->AddActor2D(mColorBar);

    mCubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    mCubeAxesActor->SetCamera(mRenderer->GetActiveCamera());

    mCubeAxesActor->SetXAxisMinorTickVisibility(false);
    mCubeAxesActor->SetYAxisMinorTickVisibility(false);
    mCubeAxesActor->SetZAxisMinorTickVisibility(false);

    mCubeAxesActor->SetDrawXGridlines(false);
    mCubeAxesActor->SetDrawYGridlines(false);
    mCubeAxesActor->SetDrawZGridlines(false);

    mCubeAxesActor->SetFlyMode(vtkCubeAxesActor::VTK_FLY_CLOSEST_TRIAD);
    mCubeAxesActor->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);

    /*for (int i = 0; i < 3; i++)
    {
        mCubeAxesActor->GetTitleTextProperty(i)->SetFontFamily(VTK_FONT_FILE);
        mCubeAxesActor->GetTitleTextProperty(i)->SetFontFile(".\\font\\simsunb.ttf");
    }*/
    mCubeAxesActor->SetXTitle("East");
    mCubeAxesActor->SetYTitle("North");
    mCubeAxesActor->SetZTitle("Elev");
    
    //mCubeAxes->SetDrawXInnerGridlines(false);
    //mCubeAxes->SetDrawYInnerGridlines(false);
    //mCubeAxes->SetDrawZInnerGridlines(false);

    //mCubeAxes->SetXAxisTickVisibility(true);
    vtkCamera* camera = mRenderer->GetActiveCamera();
    mCubeAxesActor->SetCamera(camera);
    mRenderer->AddActor(mCubeAxesActor);

    // 设置相机可视角度
    camera->SetViewAngle(30.0);
    // 设置相机位置
    double radius = 100;
    double distance = radius / sin(vtkMath::RadiansFromDegrees(camera->GetViewAngle() * 0.5));
    double pos[3] = { 0.0, 0.0, -distance };
    camera->SetPosition(pos);

    // 设置相机焦点
    double focus[3] = { 0.0, 0.0, 0.0 };
    camera->SetFocalPoint(focus);

    // 设置相机朝向
    double viewUp[3] = { 0.0, -1.0, 0.0 };
    camera->SetViewUp(viewUp);

    //applyConfig(ConfigManager::renderConfig);

    //setRenderWindow(mRenderWindow);
}

GVisualWidget::~GVisualWidget()
{

}

//vtkVolume* GVisualWidget::getVolume()
//{
//    return mVolume;
//}
//
//vtkRenderer* GVisualWidget::getMainRenderer()
//{
//    return mRenderer;
//}

void GVisualWidget::setColorTransferFunction(vtkColorTransferFunction* colorTF)
{
    mColorTF->DeepCopy(colorTF);
    mColorTF->SetAllowDuplicateScalars(false);
    
    /*vtkSmartPointer<vtkColorTransferFunction> colorBarTF = vtkColorTransferFunction::New();
    int n = mColorTF->GetSize();
    vtkSmartPointer<vtkDoubleArray> labels = vtkSmartPointer<vtkDoubleArray>::New();
    //labels->Allocate(n);
    labels->SetNumberOfComponents(1);
    labels->SetNumberOfTuples(static_cast<vtkIdType>(n));
    double *labelValues = labels->GetPointer(0);
    for (int i = n-1; i >= 0; i--)
    {
        double val[6];
        mColorTF->GetNodeValue(i, val);
        colorBarTF->AddRGBPoint(val[0], val[1], val[2], val[3], val[4], val[5]);
        //labels->SetComponent(i, 0, val[0]);
        labelValues[i] = -val[0];
    }
	mColorBar->SetLookupTable(colorBarTF);
    mColorBar->SetUseCustomLabels(true);
    mColorBar->SetCustomLabels(labels);
    mColorBar->SetNumberOfLabels(n);*/
    
    // 设置颜色传输函数的同时，需重新设置不透明度传输函数的范围
    double range[2] = {};
    mColorTF->GetRange(range);
    mOpacityTF->AddPoint(range[0], 1.0);
    mOpacityTF->AddPoint(range[1], 1.0);
    mSliceColorMap->SetColorTransferFunction(mColorTF);
}

void GVisualWidget::setOpacityTransferFunction(vtkPiecewiseFunction* opacityTF)
{
    //mOpacityTF = opacityTF;
    mOpacityTF->DeepCopy(opacityTF);
    mOpacityTF->SetClamping(false);
    mOpacityTF->SetAllowDuplicateScalars(false);
}

vtkColorTransferFunction* GVisualWidget::getColorTransferFunction()
{
    return mColorTF;
}

vtkPiecewiseFunction* GVisualWidget::getOpacityTransferFunction()
{
    return mOpacityTF;
}

void GVisualWidget::resetCamera()
{
    double bounds[6] = { 0 };
    bounds[4] = 0;
    bounds[5] = 20000;
    mImageData->GetBounds(bounds);// (xmin,xmax, ymin,ymax, zmin,zmax)
    double w = bounds[1] - bounds[0];
    double h = bounds[3] - bounds[2];
    double d = bounds[5] - bounds[4];
    w *= w;
    h *= h;
    d *= d;

    vtkCamera* camera = mRenderer->GetActiveCamera();
    // 相机可视角度
    camera->SetViewAngle(30.0);

    // 设置相机位置
    double radius = sqrt(w + h + d) * 0.5; // 可视物体半径
    double distance = radius / sin(vtkMath::RadiansFromDegrees(camera->GetViewAngle() * 0.5));
    camera->SetPosition(std::array<double, 3>{0.0, -distance, distance * 0.5}.data());

    // 设置相机焦点
    camera->SetFocalPoint(std::array<double, 3>{ 0.0, 0.0, 0.0 }.data());

    // 设置相机朝向
    camera->SetViewUp(std::array<double, 3>{ 0.0, 0.0, 1.0 }.data());

    mRenderer->ResetCamera();
}

void GVisualWidget::renderScene()
{
    mRenderWindow->Render();
}

 /*
void GVisualWidget::setColorTable(const GColorTable* colorTable)
{
    const int count = colorTable->colorCount();

    double min = 0, max = 0;
    colorTable->getValueRange(&min, &max);
    const double range = max - min;

    quint32* argb = new quint32[count];
    colorTable->fillMemUseClrTalbe(argb);

    mColorTF->RemoveAllPoints();

    for (int i = 0; i < count; i++) {
        unsigned char r = (argb[i] >> 16) & 0xFF;
        unsigned char g = (argb[i] >> 8) & 0xFF;
        unsigned char b = (argb[i] >> 0) & 0xFF;
        double value = min + range * (static_cast<double>(i) / (count - 1));
        mColorTF->AddRGBPoint(value, r / 255.0, g / 255.0, b / 255.0);
    }
    delete[] argb;

    mColorTF->Modified();
    mColorMap->SetColorTransferFunction(mColorTF);
}

void GVisualWidget::setOpacityTable(GOpacityTable* opacityTable)
{
    const int count = opacityTable->opacityCount();

    double min = 0, max = 0;
    opacityTable->getValueRange(&min, &max);
    const double range = max - min;

    float* opacity = new float[count];
    opacityTable->fillMemUseOpacityTalbe(opacity);

    mOpacityTF->RemoveAllPoints();
    mOpacityTF->SetClamping(opacityTable->getClamping());

    //mOpacityTF->AddPoint(-1.0, 1.0);

    for (int i = 0; i < count; i++) {
        double value = min + range * (static_cast<double>(i) / (count - 1));
        mOpacityTF->AddPoint(value, opacity[i]);
    }
    delete[] opacity;
    // TODO 获取0.0的时候始终为0 应该为1.0 需要修改vtkPiecewiseFunction源码
    //auto a = mOpacityTF->GetValue(0.0);

    mOpacityTF->Modified();
    mColorMap->SetOpacityTransferFunction(mOpacityTF);
    mRenderWindow->Render();
}
*/

void GVisualWidget::setValueRange(double min, double max)
{
    const double range = max - min;

    int count = mColorTF->GetSize();
    for (int i = 0; i < count; i++) {
        double value[6] = {};
        mColorTF->GetNodeValue(i, value);
        value[0] = min + range * (static_cast<double>(i) / (count - 1));
        mColorTF->SetNodeValue(i, value);
    }
    mColorTF->Modified();

    count = mOpacityTF->GetSize();
    for (int i = 0; i < count; i++) {
        double value[6] = {};
        mOpacityTF->GetNodeValue(i, value);
        value[0] = min + range * (static_cast<double>(i) / (count - 1));
        mOpacityTF->SetNodeValue(i, value);
    }
    mOpacityTF->Modified();

    mSliceColorMap->Modified();
}

// 获取数据体
vtkImageData* GVisualWidget::getImageData()
{
    return mImageData;
}

// 设置数据体
void GVisualWidget::setImageData(vtkImageData* imageData)
{
    mImageData = imageData;
    mVolumMapper->SetInputData(mImageData);

    double space[3] = {};
    mImageData->GetSpacing(space);
    mOriginZSpace = space[2];       // 记录原始的数据体Z方向尺寸

    // 设置立体轴坐标范围
    double bounds[6] = {};

    mImageData->GetBounds(bounds);
    mOriginZRange[0] = 0;// bounds[4];
    mOriginZRange[1] = 20000;// bounds[5];

    //bounds[4] = 0; // Z轴从0开始
    mCubeAxesActor->SetBounds(bounds);
    mCubeAxesActor->SetZAxisRange(0, bounds[5]);

    applyConfig(GConfig::mRenderConfig);
}

// 设置地形数据与纹理
void GVisualWidget::setTerrainData(vtkPolyData* terrainPolyData, vtkImageData* textureData)
{
    if (!mTerrainMapper) {
        mTerrainMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    }
    mTerrainMapper->SetInputData(terrainPolyData);

    if (!mTerrainTexture) {
        mTerrainTexture = vtkSmartPointer<vtkTexture>::New();
    }
    mTerrainTexture->SetInputData(textureData);
    mTerrainTexture->SetInterpolate(true); // 开启纹理插值

    if (!mTerrainActor) {
        mTerrainActor = vtkSmartPointer<vtkActor>::New();
        mTerrainActor->SetMapper(mTerrainMapper);
        mTerrainActor->SetTexture(mTerrainTexture);

        mRenderer->AddActor(mTerrainActor);
    }

    applyConfig(GConfig::mRenderConfig);
}

// 设置渲染配置
void GVisualWidget::applyConfig(const GRenderConfig& config)
{
    if (mImageData) 
    {
        //if (mDeepScale != config.mElevScale) {
        mZScale = config.mZScale;

        //mVolume->SetScale(1, 1, mDeepScale);
        //mVolume->GetBounds(bounds);
        //mCubeAxesActor->SetBounds(bounds);
        //qDebug() << 0 << "," << bounds[5];

        // 更新间距
        double space[3] = {};
        mImageData->GetSpacing(space);
        space[2] = mOriginZSpace * mZScale;
        mImageData->SetSpacing(space);

        // 更新原点
        double origin[3] = {};
        mImageData->GetOrigin(origin);
        mImageData->SetOrigin(origin[0], origin[1], mOriginZRange[0] * mZScale);

        double bounds[6] = {};
        mImageData->GetBounds(bounds);
        bounds[4] = 0;
        bounds[5] = 20000;
        mCubeAxesActor->SetBounds(bounds);

        mCubeAxesActor->SetZAxisRange(0, mOriginZRange[1]);
        qDebug() << 0 << "," << bounds[5];

        // 处理切片尺寸变换
        if (mImagePlaneX) 
        {
            int index = mImagePlaneX->GetSliceIndex();
            mImagePlaneX->SetInputData(mImageData);
            mImagePlaneX->SetPlaneOrientationToXAxes();
            mImagePlaneX->SetSliceIndex(index);
            //mImagePlaneX->PlaceWidget(bounds);
            mImagePlaneX->Modified();

            index = mImagePlaneY->GetSliceIndex();
            mImagePlaneY->SetInputData(mImageData);
            mImagePlaneY->SetPlaneOrientationToYAxes();
            mImagePlaneY->SetSliceIndex(index);
            //mImagePlaneY->PlaceWidget(bounds);
            mImagePlaneY->Modified();

            index = mImagePlaneZ->GetSliceIndex();
            mImagePlaneZ->SetInputData(mImageData);
            mImagePlaneZ->SetPlaneOrientationToZAxes();
            mImagePlaneZ->SetSliceIndex(index);
            //mImagePlaneZ->PlaceWidget(bounds);
            mImagePlaneZ->Modified();
        }
        //}
    }


    // 数据体渲染参数
    vtkVolumeProperty* volumeProperty = mVolume->GetProperty();
    volumeProperty->SetInterpolationType(config.mInterpolationMethod);
    volumeProperty->SetShade(config.mVolumeShade);
    volumeProperty->SetAmbient(config.mVolumeAmbient);
    volumeProperty->SetDiffuse(config.mVolumeDiffuse);
    volumeProperty->SetSpecular(config.mVolumeSpecular);
    volumeProperty->SetSpecularPower(config.mVolumeSpecularPower);
    volumeProperty->Modified();
    mVolume->Modified();

    mVolumMapper->SetAutoAdjustSampleDistances(config.mAutoSampleDistance);
    mVolumMapper->SetSampleDistance(config.mSampleDistance);
    mVolumMapper->SetUseJittering(config.mUseJittering);
    mVolumMapper->Modified();

    // 不透明度
    //double range[2] = { 0 };
    //mColorTF->GetRange(range);
    //mOpacityTF->AddPoint(range[0], config.mOpacity);
    //mOpacityTF->AddPoint(range[1], config.mOpacity);

    const GOpacityTable* opacityTable = &config.mOpacityTable;
    const int count = opacityTable->opacityCount();
	double min = 0, max = 0;
	opacityTable->getValueRange(&min, &max);
	const double range = max - min;

	float* opacity = new float[count];
	opacityTable->fillMemUseOpacityTalbe(opacity);

	mOpacityTF->RemoveAllPoints();
	mOpacityTF->SetClamping(opacityTable->getClamping());
	for (int i = 0; i < count; i++) 
    {
		double value = min + range * (static_cast<double>(i) / (count - 1));
		mOpacityTF->AddPoint(value, opacity[i]);
	}
	delete[] opacity;
	mOpacityTF->Modified();

    if (mTerrainActor) 
    {
        vtkProperty* terrainProperty = mTerrainActor->GetProperty();
        terrainProperty->SetShading(config.mTerrainShade);
        terrainProperty->SetAmbient(config.mTerrainAmbient);
        terrainProperty->SetDiffuse(config.mTerrainDiffuse);
        terrainProperty->SetSpecular(config.mTerrainSpecular);
        terrainProperty->SetSpecularPower(config.mTerrainSpecularPower);
        terrainProperty->Modified();
    }

    // 颜色条渲染参数
    mColorBar->SetWidth(config.mScalarBarWidthRate);
    mColorBar->SetHeight(config.mScalarBarHeightRate);
    mColorBar->SetMaximumWidthInPixels(config.mScalarBarMaxWidthPixels);
    mColorBar->SetPosition(1 - config.mScalarBarWidthRate, (1 - config.mScalarBarHeightRate) / 2);
    mColorBar->SetNumberOfLabels(config.mScalarBarLabelsCount);
    mColorBar->Modified();

	// 坐标系
	mCubeAxesActor->SetVisibility(config.mCubeAxesVisibility);
}

// 初始化切片
void GVisualWidget::initSlice()
{
    if (mIsSliceInit) return;
    mIsSliceInit = true;

    mImagePlaneX = vtkSmartPointer<vtkImagePlaneWidget>::New();
    mImagePlaneY = vtkSmartPointer<vtkImagePlaneWidget>::New();
    mImagePlaneZ = vtkSmartPointer<vtkImagePlaneWidget>::New();

    vtkImagePlaneWidget* planes[] = { mImagePlaneX, mImagePlaneY, mImagePlaneZ };
    double planeColors[][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
	int dims[3] = {};
	mImageData->GetDimensions(dims);

    for (int i = 0; i < 3; i++)
    {
        vtkImagePlaneWidget* plane = planes[i];
        double *planeColor = planeColors[i];

        plane->SetDefaultRenderer(mRenderer);
        plane->SetInputData(mImageData);
        plane->SetPlaneOrientationToXAxes();
        plane->SetResliceInterpolate(VTK_CUBIC_RESLICE);

        plane->GetColorMap()->SetLookupTable(mSliceColorMap);
		//mImagePlaneX->SetTextureVisibility(true);
        plane->SetMarginSizeX(0);
        plane->SetMarginSizeY(0);
		//mImagePlaneX->SetDisplayText(true);

        plane->GetPlaneProperty()->SetColor(planeColor);
        plane->GetSelectedPlaneProperty()->SetColor(1.0, 1.0, 0.0);
        plane->GetSelectedPlaneProperty()->SetLineWidth(5.0);
        plane->SetInteractor(interactor());
        plane->SetEnabled(true);
        plane->SetInteraction(false);
        plane->SetEnabled(false);
        plane->SetPlaneOrientation(i);
        plane->SetSliceIndex(dims[i] / 2);
    }
}

bool GVisualWidget::isSliceEnable()
{
    return mIsSliceEnable;
}

void GVisualWidget::setSliceEnable(bool enable)
{
    if (mIsSliceEnable == enable) {
        return;
    }
    mIsSliceEnable = enable;
    if (!mIsSliceInit) {
        initSlice();
        mIsSliceInit = true;
    }
    if (mIsSliceEnable) 
    {
        mImagePlaneX->SetEnabled(mXSliceEnable);
        mImagePlaneY->SetEnabled(mYSliceEnable);
        mImagePlaneZ->SetEnabled(mZSliceEnable);
    } 
    else 
    {
        mImagePlaneX->SetEnabled(false);
        mImagePlaneY->SetEnabled(false);
        mImagePlaneZ->SetEnabled(false);
    }

    mVolume->SetVisibility(!mIsSliceEnable);

    mRenderWindow->Render();
}

void GVisualWidget::setSliceAxisEnable(int axis, bool enable)
{
    if (enable) 
    {
        switch (axis) 
        {
        case 0:
            mXSliceEnable = true;
            mImagePlaneX->SetEnabled(true);
            mImagePlaneX->SetInteraction(true);
            break;
        case 1:
            mYSliceEnable = true;
            mImagePlaneY->SetEnabled(true);
            mImagePlaneY->SetInteraction(true);
            break;
        case 2:
            mZSliceEnable = true;
            mImagePlaneZ->SetEnabled(true);
            mImagePlaneZ->SetInteraction(true);
            break;
        }
    } 
    else 
    {
        switch (axis) 
        {
        case 0:
            mXSliceEnable = false;
            mImagePlaneX->SetEnabled(false);
            break;
        case 1:
            mYSliceEnable = false;
            mImagePlaneY->SetEnabled(false);
            break;
        case 2:
            mZSliceEnable = false;
            mImagePlaneZ->SetEnabled(false);
            break;
        }
    }
    mRenderWindow->Render();
}

void GVisualWidget::setSliceAxisIndex(int axis, int pos)
{
    switch (axis) 
    {
    case 0:
        mImagePlaneX->SetSliceIndex(pos);
        break;
    case 1:
        mImagePlaneY->SetSliceIndex(pos);
        break;
    case 2:
        mImagePlaneZ->SetSliceIndex(pos);
        break;
    }
    mRenderWindow->Render();
}

void GVisualWidget::setVolumnVisibility(bool visibility)
{
    mVolume->SetVisibility(visibility);
    mRenderWindow->Render();
}

bool GVisualWidget::getVolumnVisibility()
{
    return mVolume->GetVisibility();
}

void GVisualWidget::setZClip(bool enable)
{
    double bounds[6] = {};
    if (enable) {
        mVolumMapper->GetCroppingRegionPlanes(bounds);
    } else {
        mImageData->GetBounds(bounds);
    }
    mCubeAxesActor->SetBounds(bounds);

    mVolumMapper->SetCropping(enable);
    mRenderWindow->Render();
}

// 获取Z轴裁剪边界
void GVisualWidget::getZClipRange(double* top, double* bottom)
{
    double bounds[6] = {};
    mVolumMapper->GetCroppingRegionPlanes(bounds);
    //// 如果开启Z轴上下边界 则通过裁剪函数获取裁剪范围
    //if (mVolumMapper->GetCropping()) {
    //    mVolumMapper->GetCroppingRegionPlanes(bounds);
    //} else {
    //    // 如果没有 则通过数据体获取范围
    //    mImageData->GetBounds(bounds);
    //}
    *top = bounds[2];
    *bottom = bounds[3];

}

// 设置Z轴裁剪边界
void GVisualWidget::setZClipRange(double top, double bottom)
{
    double bounds[6] = {}; // (xmin, xmax, ymin, ymax, zmin, zmax)
    mImageData->GetBounds(bounds);

    bounds[2] = top;
    bounds[3] = bottom;
    mVolumMapper->SetCroppingRegionPlanes(bounds);

    if (mVolumMapper->GetCropping()) {
        mCubeAxesActor->SetBounds(bounds);
    }
    //setZClip(true);
}


// 获取切片索引
void GVisualWidget::getSliceIndex(int index[3])
{
    if (!mIsSliceInit) 
    {
        initSlice();
    }
    index[0] = mImagePlaneX->GetSliceIndex();
    index[1] = mImagePlaneY->GetSliceIndex();
    index[2] = mImagePlaneZ->GetSliceIndex();
}