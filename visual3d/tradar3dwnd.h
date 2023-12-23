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
	int								mRadarDataIndex = -1;			// ��ǰ��ʾ�״��������±�
	const QVector<GRadarVolume*>*	mRadarDataList = nullptr;		// �״��������б�
	vtkSmartPointer<vtkImageData>	mImageData;						// VTKͼ��������
	GVisualWidget *					mVisualWidget = nullptr;		// ���ӻ��ؼ�
	QImage							mMapImage;						// ��ͼͼ��
	QImage							mElevImage;						// �߳�ͼ��
	QRectF							mGridRect;						// ���񻯷�Χ��webī����ͶӰ���꣩
	bool							mIsRendered = false;			// �Ƿ��Ѿ���Ⱦ��
	TSliceConfigDlg *				mSliceDialog = nullptr;			// ��Ƭ���öԻ���
	TRenderConfigDlg *				mRenderConfigDlg = nullptr;		// ��Ⱦ���öԻ���

protected:
	// ��ȡ��ǰ�״�����
	const GRadarVolume* getCurRadarData();

	// rect:���񻯷�Χ(������״���������)
	vtkSmartPointer<vtkImageData> griddingData(QRectF* rect);

	// ��ͼ���߳�תvtkPolyData
	vtkSmartPointer<vtkPolyData> toVtkPolyData(const QRectF& rect);

public:
	TRadar3DWnd(QWidget *parent = nullptr);
	virtual ~TRadar3DWnd();

	// �����״���������
	void setRadarDataIndex(const int index);

	// �����״������б�
	void setRadarDataList(const QVector<GRadarVolume*>* radarDataList);

	// ���õ�ͼͼ��
	void setMapImage(const QImage* mapImage);

	// ���ø߳�ͼ��
	void setElevImage(const QImage* elevImage);

	// �������񻯷�Χ��webī����ͶӰ���꣩
	void setGridRect(const QRectF& rect);

	// ������ɫ���亯��
	void setColorTransferFunction(vtkColorTransferFunction* colorTF);

	// ��Ⱦ�״�������
	void render();

private slots:

	// ��Ⱦ����
	void on_actRenderSetting_triggered();

	// ��Ƭ����
	void on_actSliceSetting_triggered();

	// ����������
	void on_actSaveVolume_triggered();

	// ��׽��ǰ��Ļ������
	void on_actSnap_triggered();

	// �رմ���
	void on_actClose_triggered();

	// ��һ��
	void on_actMovePrev_triggered();

	// ��һ��
	void on_actMoveNext_triggered();

	// ��ʾ������
	void on_actShowVolume_toggled(bool checked);

	// ��ʾ��Ƭ
	void on_actShowSlice_toggled(bool checked);

	// ��Ƭ���ػص�
	void onSliceAxisEnable(int axis, bool enable);

	// ��Ƭλ�ñ仯�ص�
	void onSliceChange(int axis, int pos);
};
