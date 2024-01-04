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

// �״�������ά���ӻ�С����
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
	vector<float>                   mVPreData;						// �洢��ȡԤ���������״�����
public:
	//�״�վ�㾭γ�ȣ��߶�
	double SLon = 0;
	double SLat = 0;
	double SElev = 96.0;
	//׶�����
	int Sufnums = 9;
	//ÿһ��׶������ǵĶ���
	double Els[9] = { 0.65,1.34,2.26,3.16,4.15,5.86,9.88,13.84,19.33 };
	vector<double> VEls;

	int ImageWideth = 0;
	int ImageHeight = 0;

	//��Ļ���굽ʵ������ľ�γ��
	vector<double> lons;
	vector<double> lats;

	//������״�վ�������
	vector<double> GXS;
	vector<double> GYS;
	vector<double> GZS;
	//���ֵ
	vector<double> values;
	vector<double> gvalues;
	vector<int> oneELnums;
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

	//Ԥ��ǰ
	//�����������������Ⱦ
	void renderAll();
	//�������״���������
	vtkSmartPointer<vtkImageData> grridingdataAll(QRectF* rect);
	//�����񻯺���״�����ÿ��д��txt�ļ���
	void wirteData(int nx, int ny, int nz, float* pointer);

	
	//Ԥ���
	//�����������������Ⱦ
	void renderAfterPrerdict();
	//����ImageData
	vtkSmartPointer<vtkImageData> completeImageData();
	//��ȡ����
	void readData();


	//��γ��Ԥ��
	
	//����ͼƬ
	int load_PredictImage(GRader2DLayer* layer, QString rootpath);
	int load_PredictImage(GRader2DLayer* layer, QVector<QImage> image);
	//��������ϳ��״�������
	void CompleteVolume();
	//��ȾԤ�����״�������
	void renderPrerdict();
	//����Ԥ��������
	vtkSmartPointer<vtkImageData> gridingPreData();
	//����ÿ���ֵ
	void calGridValuepre(double minx, double maxx, double miny, double maxy, double minz, double maxz, int nx, int ny, int nz, float* output);
	//����������ǲ�
	int findel(double e);
	//����ھӷ���
	double Nearintep(double e, double gx, double gy, double gz, double dx, double dy, double dz);
	//�������Ȩ��ֵ
	double IDWinterp(double e, double gx, double gy, double gz, double dx, double dy, double dz);
	// �Ƕ�ת����
	constexpr double toRadian(double angle)
	{
		return angle * (3.1415926 / 180.0);
	}

	// ����ת�Ƕ�
	constexpr double toAngle(double radian)
	{
		return radian * (180.0 / 3.1415926);
	}
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
