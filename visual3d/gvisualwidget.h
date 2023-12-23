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
	// ��ʼ����Ƭ
	bool mIsSliceInit = false;              // ��Ƭ�Ƿ��Ѿ���ʼ��
	bool mIsSliceEnable = false;            // �Ƿ�����Ƭ
	bool mXSliceEnable = true;              // ����X����Ƭ
	bool mYSliceEnable = true;              // ����Y����Ƭ
	bool mZSliceEnable = true;              // ����Z����Ƭ

	double mOriginZSpace = 0.0;             // ԭʼZ������
	double mOriginZRange[2] = {0.0};        // ԭʼZ����Χ
	double mZScale = 1.0;                   // ��ȷ���������ϵ��

    
	vtkSmartPointer<QVTKInteractor> mInteractor;                // VTK������   
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> mInteractorStyle;    // VTK������ʽ(��������׷����)
	vtkSmartPointer<vtkRenderWindow> mRenderWindow;             // VTK��Ⱦ����
	vtkSmartPointer<vtkRenderer> mRenderer;                     // VTK����Ⱦ��

	vtkSmartPointer<vtkImageData> mImageData;                   // ������ά����
    vtkSmartPointer<vtkVolume> mVolume;                         // ��ά���������
	vtkSmartPointer<vtkGPUVolumeRayCastMapper> mVolumMapper;    // ������ӳ����

	vtkSmartPointer<vtkImageData> mTerrainImage;                // ����ͼ������
	vtkSmartPointer<vtkTexture> mTerrainTexture;                // �����������
	vtkSmartPointer<vtkPolyDataMapper> mTerrainMapper;          // ����ӳ����
	vtkSmartPointer<vtkActor> mTerrainActor;                    // VTK����Actor

	// ��ɫ��͸���ȴ��亯��
	vtkSmartPointer<GSliceColorMap> mSliceColorMap;             // ��Ƭ��ɫ���亯��, �Զ����� �������ɫ��͸���ȵĴ��亯��
	vtkSmartPointer<vtkColorTransferFunction> mColorTF;         // ��������ɫ���亯��
	vtkSmartPointer<vtkPiecewiseFunction> mOpacityTF;           // ������͸���ȴ��亯��

	vtkSmartPointer<vtkAxesActor> mAxesActor;                   // ���½�����ϵActor
	vtkSmartPointer<vtkOrientationMarkerWidget> mOmWidget;      // ���½�����ϵ�ؼ�

	vtkSmartPointer<vtkScalarBarActor> mColorBar;               // ��ɫ��Actor
	vtkSmartPointer<vtkCubeAxesActor> mCubeAxesActor;           // ����ϵ����Actor

	// ��Ƭƽ��ؼ�
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneX;          // X��Ƭ�ؼ�
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneY;          // Y��Ƭ�ؼ�
	vtkSmartPointer<vtkImagePlaneWidget> mImagePlaneZ;          // Z��Ƭ�ؼ�

protected:
	// ��ʼ����Ƭ
	void initSlice();

public:
    GVisualWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~GVisualWidget(); // �̳������⺯��

    // ������������ɫ���亯��
    void setColorTransferFunction(vtkColorTransferFunction* colorTF);

    // ����������͸���ȴ��亯��
    void setOpacityTransferFunction(vtkPiecewiseFunction* opacityTF);

    // ��ȡ��������ɫ���亯��
    vtkColorTransferFunction* getColorTransferFunction();

    // ����������͸���ȴ��亯��
    vtkPiecewiseFunction* getOpacityTransferFunction();

    // ���������巶Χ
    void setValueRange(double min, double max);

    // ��ȡ����������
    vtkImageData* getImageData();

    // ��������������
    void setImageData(vtkImageData* imageData);

    // ���õ�������������
    void setTerrainData(vtkPolyData* terrainPolyData, vtkImageData* textureData);

    // �������
    void resetCamera();

    // ��Ⱦ����
    void renderScene();

	// ������Ⱦ����
	void applyConfig(const GRenderConfig& config);

	// �����������Ƿ�ɼ�
	void setVolumnVisibility(bool visibility);

	// ��ȡ�������Ƿ�ɼ�
	bool getVolumnVisibility();

    // �Ƿ�����Ƭ(���忪��)
    bool isSliceEnable();

    // �����Ƿ�����Ƭ(���忪��)
    void setSliceEnable(bool enable);

    // ����ÿ�����Ƿ�����Ƭ
    void setSliceAxisEnable(int axis, bool enable);

    // ����ÿ�������Ƭ����
    void setSliceAxisIndex(int axis, int pos);

	// ��ȡ��Ƭ����
	void getSliceIndex(int index[3]);

    // ����Z��ƽ��ü��Ƿ���
    void setZClip(bool enable);

    // ��ȡZ��ü��߽�
    void getZClipRange(double* top, double* bottom);

    // ����Z��ü��߽�
    void setZClipRange(double top, double bottom);
};