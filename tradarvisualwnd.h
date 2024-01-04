#pragma once
#ifndef TRADARVISUALWND_H
#define TRADARVISUALWND_H

#include <QMainWindow>
#include <QProgressBar>
#include <QImage>
#include <QTimer>
#include <QButtonGroup>
#include <QSet>
#include <QTranslator>
#include <vtkSmartPointer.h>
#include <vtkColorTransferFunction.h>
#include "ui_tradarvisualwnd.h"
#include "gmaptile.h"
#include "generic_basedata_cv.h"


class GMapView;
class GRader2DLayer;
class GColorBarLayer;
class GShapeLayer;
class GRadarVolume;
class TRadar3DWnd;

//	������Ĵ��ͼ�ϵ��״���ӻ�
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
	GRader2DLayer *					mpRaderLayer;		// ��ҳ���״�2Dͼ��
	GColorBarLayer *				mpColorBarLayer;	// ɫ��ͼ��
	GShapeLayer *					mpShapeLayer;		// Shapeͼ��

	QVector<GRadarVolume*>			mRadarList;			// �״��������б�
	QSet<QString>					mRadarPathSet;		// �״��ļ�·������

	bool							mIsAnimate;			// ��ǰ�Ƿ��ڲ����״�2D����
	QTimer							mAnimateTimer;		// �����״�2D�����Ķ�ʱ��
	bool							mIsPredict;         // ��ǰ�Ƿ����Ԥ��

	QVector<basedataImage*>			mBdiList;			// �״Ｐ�����б�

	QTranslator*                    mTranslator;
	TRadar3DWnd*					mLastRadar3DWnd = nullptr;	// ��һ���״�������ά��С��������

protected:

	// ���״��ļ��б�
	void loadRadarFiles(const QStringList& fileList);

	// �����޸ĺ���״��ļ�
	void saveRadarFile(const QString& filepath, int mIndex);

	// �����Ƿ񲥷��״�2D����
	void setAnimate(bool mIsAnimate);

	// �״������ı亯��, ˢ�½���
	void onRaderIndexChange(int index);

public:
    TRadarVisualWnd(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~TRadarVisualWnd();

	// ����ͼ�ƶ���ָ�����η�Χ������
	void centerOn(const QRectF& rc);

	void setDeskTopWidget();

private slots:

	void on_actTestServer_triggered();

	void on_actSelectRect_triggered(bool checked);

	// ���ļ�
	void on_actFileOpen_triggered();

	// �����ļ�
	void on_actFileSave_triggered();

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
	//Ԥ��2023-7-31
	void on_actPredict_triggered();

	//ֹͣԤ��
	void on_actStop_triggered();
	//��ImageDataÿ����벢����
	//void on_actSave_triggered();

	// �����л�
	void on_actCN_triggered();
	void on_actEN_triggered();
};

#endif // TMAPPERWND_H
