#pragma once

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QRect>
#include <QPaintEvent>
#include <QLabel>

class GGraphView : public QWidget
{
public:
	enum HRulerPos {TOP, BOTTOM};
	enum VRulerPos {LEFT, RIGHT};

protected:
	QRect						mViewRect;				// 视图矩形区域
	QRect						mMargins;				// 边距
	QString						mTitle;					// 曲线图名称
	QString						mXTitle;				// x轴的标注名称
	QString						mYTitle;				// y轴的标注名称
	QFont						mMainTitleFont;			// 主窗体标题字体
	QFont						mXYTitleFont;			// 设置XY轴标题的字体
	double						mZoomScale;				// 放大系数
	int							mOffsetX;				// x方向偏移量
	int							mMinOffsetX;			// x方向最小平移量
	int							mOffsetY;				// y方向偏移量
	int							mMinOffsetY;			// y方向最小平移量
	double						mExtendRange;			// 扩展比例
	QPoint						mMousePos;				// 记录鼠标拖拽位置
	double						mMaxLpX;				// 最大的逻辑x值
	double						mMinLpX;				// 最小的逻辑x值
	double						mMaxLpY;				// 最大的逻辑y值
	double						mMinLpY;				// 最小的逻辑y值
	bool						mIsDrag;				// 是否拖拽
	bool						mIsDrawGrid;			// 是否绘画网格
	double						mRulerDeltaX;			// 设置X轴间隔
	double						mRulerDeltaY;			// 设置Y轴间隔
	int							mRulerXCount;			// 设置X轴上坐标的条数
	int							mRulerYCount;			// 设置Y轴上坐标的条数
	bool						mIsFlipX;				// 是否反转X坐标系
	bool						mIsFlipY;				// 是否反转Y坐标系
	QColor						mBackgroundColor;		// 绘图区背景色
	HRulerPos					mHRulerPos;				// 水平卷尺位置
	VRulerPos					mVRulerPos;				// 垂直卷尺位置
	int							mTitleMargin;			// 标题距色块图的边距
	QLabel						*mInfoLabel;			// 信息输出标签
protected:

	virtual void resizeEvent(QResizeEvent* event);

	// 绘图事件
	virtual void paintEvent(QPaintEvent * event);

	// 鼠标滚轮事件 支持按鼠标位置中心缩放
	virtual void wheelEvent(QWheelEvent * event);

	// **********拖拽事件组***********
	virtual void mouseMoveEvent(QMouseEvent * event);

	virtual void mousePressEvent(QMouseEvent * event);

	virtual void mouseReleaseEvent(QMouseEvent * event);
	// **********拖拽事件组***********

	// 绘制绘图区的背景色
	void drawBackground(QPainter *p, const QRect &r);

	// 绘制折线图标题
	void drawTitle(QPainter *p);

	// 绘制横坐标轴
	void drawHRuler(QPainter *p);

	// 绘制纵坐标轴
	void drawVRuler(QPainter *p);

	// 逻辑X坐标转物理X坐标
	int lpXTodpX(double lx);
	
	// 物理坐标x转实际坐标
	double dpXTolpX(int dpX);

	// 逻辑Y坐标转物理Y坐标
	int lpYTodpY(double ly);
	
	// 物理坐标x转实际坐标
	double dpYTolpY(int dpY);

public:
	GGraphView(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~GGraphView();

	virtual void clear();

	// 设置信息标签
	void setInfoLabel(QLabel *label);
	
	// 设置水平卷尺位置
	void setHRulerPos(HRulerPos pos);

	// 设置垂直卷尺位置
	void setVRulerPos(VRulerPos pos);

	// 设置绘图边距
	void setMargins(int marginLeft, int marginRight, int marginTop, int marginBottom);

	// 设置曲线图的标题
	void setTitle(const QString & title);

	// 设置标题距色块图的边距
	void setTitleMargin(int margin);

	// 设置x轴的标注
	void setXTitle(const QString & xTitle);

	// 设置y轴的标注
	void setYTitle(const QString & yTitle);

	// 设置主标题的字体
	void setTitleFont(QFont font);

	// 设置XY标题字体
	void setXYTitleFont(QFont font);

	// 设置是否绘画网格
	void setDrawGrid(bool isDrawGrid);

	// 是否反转X轴
	void setFlipX(bool isFlipX);

	// 是否反转Y轴
	void setFlipY(bool isFlipY);

	// 设置横轴显示的间隔
	void setHRulerLdx (double ldx);

	// 设置纵轴显示的间隔
	void setVRulerLdy (double ldy);

	// 获取横轴显示的间隔
	double getHRulerLdx();

	// 获取纵轴显示的间隔
	double getVRulerLdy();

	// 设置横轴间隔的条数
	void setXRulerCount(int count);

	// 设置纵轴间隔的条数
	void setYRulerCount(int count);

	// 设置绘图区背景色
	void setBackground(QColor cr);

	// 得到x、y方向的逻辑变化范围（求出两个方向的最值）
	virtual void statDataRange();

	// 允许外界设置变化范围
	void setDataRange(double minX, double minY, double maxX, double maxY);

	void setXDataRange(double minX, double maxX);

	void setYDataRange(double minY, double maxY);

	// 获取求得的范围
	double getRangeMinX();

	double getRangeMaxX();

	double getRangeMinY();

	double getRangeMaxY();
};
