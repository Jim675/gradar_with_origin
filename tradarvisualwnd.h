#pragma once
#ifndef TRADARVISUALWND_H
#define TRADARVISUALWND_H

#include <QMainWindow>
#include <QProgressBar>
#include <QImage>
#include <QTimer>
#include <QButtonGroup>
#include <QSet>
#include <vtkSmartPointer.h>
#include <vtkColorTransferFunction.h>
#include "ui_tradarvisualwnd.h"
#include "gmaptile.h"

class GMapView;
class GRader2DLayer;
class GColorBarLayer;
class GShapeLayer;
class GRadarVolume;
class TRadar3DWnd;

class TRadarVisualWnd : public QMainWindow
{
    Q_OBJECT

private:
	Ui::TRadarVisualWndClass		ui;					// ����ui����
	GMapView *						mpView;				// ����ͼ
	QButtonGroup *					mELBtnGroups;		// ���ǵ�ѡ��ť��
	int								mELIndex;			// �û�ѡ��������±�
	QCheckBox *						mInterpCBox;		// ��ֵ
	QCheckBox*						mRadarCBox;			// ��ʾ�״�ͼ��

	vtkSmartPointer< vtkColorTransferFunction > mColorTF;	// ��ɫ���亯��
	GRader2DLayer *					mpRaderLayer;		// �״�2Dͼ��
	GColorBarLayer *				mpColorBarLayer;	// ɫ��ͼ��
	GShapeLayer *					mpShapeLayer;		// Shapeͼ��

	QVector<GRadarVolume*>			mRadarList;			// �״��������б�
	QSet<QString>					mRadarPathSet;		// �״��ļ�·������

	bool							mIsAnimate;			// ��ǰ�Ƿ��ڲ����״�2D����
	QTimer							mAnimateTimer;		// �����״�2D�����Ķ�ʱ��

	TRadar3DWnd*					mLastRadar3DWnd = nullptr;

protected:

	// ���״��ļ��б�
	void loadRadarFiles(const QStringList& fileList);

	// �����Ƿ񲥷��״�2D����
	void setAnimate(bool mIsAnimate);

	// �״������ı亯��, ˢ�½���
	void onRaderIndexChange(int index);

public:
    TRadarVisualWnd(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~TRadarVisualWnd();

	// ����ͼ�ƶ���ָ�����η�Χ������
	void centerOn(const QRectF& rc);

private slots:

	void on_actTestServer_triggered();

	void on_actSelectRect_triggered(bool checked);

	// ���ļ�
	void on_actFileOpen_triggered();

	// ����ļ��б�
	void on_actClearFileList_triggered();

	// �˳�
	void on_actExit_triggered();

	// ���ô�����ͼ
	void on_actViewReset_triggered();

	// ��ǰ�ļ��仯
	void on_mFileListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

	// ��άͼ���ֵ״̬�仯
	void on_mInterpCBox_stateChanged(int state);

	void on_actInterpRadarData_toggled(bool checked);

	// �״���ʾ״̬�仯
	void on_mRadarCBox_stateChanged(int state);

	void on_actShowRadarData_toggled(bool checked);

	// ����ѡ��仯
	void onELButtonClicked(int id);

	// ��ʼ���Ŷ���
	void on_actAnimationStart_triggered();

	// ֹͣ���Ŷ���
	void on_actAnimationStop_triggered();

	// ���ö������Ų���
	void on_actAnimationSettings_triggered();

	// ������ʱ����ʱ�ص�
    void onAnimateTimeout(); 

    // ѡ����ο�ص�
    void onSelectedRect(const QRect& rect);

	void radar3DWndDestroyed(QObject* pObj= Q_NULLPTR);
};

#endif // TMAPPERWND_H
