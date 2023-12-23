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
	Ui::TRadarVisualWndClass		ui;					// 窗口ui对象
	GMapView *						mpView;				// 主视图
	QButtonGroup *					mELBtnGroups;		// 仰角单选按钮组
	int								mELIndex;			// 用户选择的仰角下标
	QCheckBox *						mInterpCBox;		// 插值
	QCheckBox*						mRadarCBox;			// 显示雷达图层

	vtkSmartPointer< vtkColorTransferFunction > mColorTF;	// 颜色传输函数
	GRader2DLayer *					mpRaderLayer;		// 雷达2D图层
	GColorBarLayer *				mpColorBarLayer;	// 色标图层
	GShapeLayer *					mpShapeLayer;		// Shape图层

	QVector<GRadarVolume*>			mRadarList;			// 雷达数据体列表
	QSet<QString>					mRadarPathSet;		// 雷达文件路径集合

	bool							mIsAnimate;			// 当前是否在播放雷达2D动画
	QTimer							mAnimateTimer;		// 播放雷达2D动画的定时器

	TRadar3DWnd*					mLastRadar3DWnd = nullptr;

protected:

	// 打开雷达文件列表
	void loadRadarFiles(const QStringList& fileList);

	// 设置是否播放雷达2D动画
	void setAnimate(bool mIsAnimate);

	// 雷达索引改变函数, 刷新界面
	void onRaderIndexChange(int index);

public:
    TRadarVisualWnd(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~TRadarVisualWnd();

	// 将视图移动到指定矩形范围的中心
	void centerOn(const QRectF& rc);

private slots:

	void on_actTestServer_triggered();

	void on_actSelectRect_triggered(bool checked);

	// 打开文件
	void on_actFileOpen_triggered();

	// 清空文件列表
	void on_actClearFileList_triggered();

	// 退出
	void on_actExit_triggered();

	// 重置窗口视图
	void on_actViewReset_triggered();

	// 当前文件变化
	void on_mFileListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

	// 二维图层插值状态变化
	void on_mInterpCBox_stateChanged(int state);

	void on_actInterpRadarData_toggled(bool checked);

	// 雷达显示状态变化
	void on_mRadarCBox_stateChanged(int state);

	void on_actShowRadarData_toggled(bool checked);

	// 仰角选择变化
	void onELButtonClicked(int id);

	// 开始播放动画
	void on_actAnimationStart_triggered();

	// 停止播放动画
	void on_actAnimationStop_triggered();

	// 设置动画播放参数
	void on_actAnimationSettings_triggered();

	// 动画定时器到时回调
    void onAnimateTimeout(); 

    // 选择矩形框回调
    void onSelectedRect(const QRect& rect);

	void radar3DWndDestroyed(QObject* pObj= Q_NULLPTR);
};

#endif // TMAPPERWND_H
