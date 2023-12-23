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
	QRect						mViewRect;				// ��ͼ��������
	QRect						mMargins;				// �߾�
	QString						mTitle;					// ����ͼ����
	QString						mXTitle;				// x��ı�ע����
	QString						mYTitle;				// y��ı�ע����
	QFont						mMainTitleFont;			// �������������
	QFont						mXYTitleFont;			// ����XY����������
	double						mZoomScale;				// �Ŵ�ϵ��
	int							mOffsetX;				// x����ƫ����
	int							mMinOffsetX;			// x������Сƽ����
	int							mOffsetY;				// y����ƫ����
	int							mMinOffsetY;			// y������Сƽ����
	double						mExtendRange;			// ��չ����
	QPoint						mMousePos;				// ��¼�����קλ��
	double						mMaxLpX;				// �����߼�xֵ
	double						mMinLpX;				// ��С���߼�xֵ
	double						mMaxLpY;				// �����߼�yֵ
	double						mMinLpY;				// ��С���߼�yֵ
	bool						mIsDrag;				// �Ƿ���ק
	bool						mIsDrawGrid;			// �Ƿ�滭����
	double						mRulerDeltaX;			// ����X����
	double						mRulerDeltaY;			// ����Y����
	int							mRulerXCount;			// ����X�������������
	int							mRulerYCount;			// ����Y�������������
	bool						mIsFlipX;				// �Ƿ�תX����ϵ
	bool						mIsFlipY;				// �Ƿ�תY����ϵ
	QColor						mBackgroundColor;		// ��ͼ������ɫ
	HRulerPos					mHRulerPos;				// ˮƽ���λ��
	VRulerPos					mVRulerPos;				// ��ֱ���λ��
	int							mTitleMargin;			// �����ɫ��ͼ�ı߾�
	QLabel						*mInfoLabel;			// ��Ϣ�����ǩ
protected:

	virtual void resizeEvent(QResizeEvent* event);

	// ��ͼ�¼�
	virtual void paintEvent(QPaintEvent * event);

	// �������¼� ֧�ְ����λ����������
	virtual void wheelEvent(QWheelEvent * event);

	// **********��ק�¼���***********
	virtual void mouseMoveEvent(QMouseEvent * event);

	virtual void mousePressEvent(QMouseEvent * event);

	virtual void mouseReleaseEvent(QMouseEvent * event);
	// **********��ק�¼���***********

	// ���ƻ�ͼ���ı���ɫ
	void drawBackground(QPainter *p, const QRect &r);

	// ��������ͼ����
	void drawTitle(QPainter *p);

	// ���ƺ�������
	void drawHRuler(QPainter *p);

	// ������������
	void drawVRuler(QPainter *p);

	// �߼�X����ת����X����
	int lpXTodpX(double lx);
	
	// ��������xתʵ������
	double dpXTolpX(int dpX);

	// �߼�Y����ת����Y����
	int lpYTodpY(double ly);
	
	// ��������xתʵ������
	double dpYTolpY(int dpY);

public:
	GGraphView(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~GGraphView();

	virtual void clear();

	// ������Ϣ��ǩ
	void setInfoLabel(QLabel *label);
	
	// ����ˮƽ���λ��
	void setHRulerPos(HRulerPos pos);

	// ���ô�ֱ���λ��
	void setVRulerPos(VRulerPos pos);

	// ���û�ͼ�߾�
	void setMargins(int marginLeft, int marginRight, int marginTop, int marginBottom);

	// ��������ͼ�ı���
	void setTitle(const QString & title);

	// ���ñ����ɫ��ͼ�ı߾�
	void setTitleMargin(int margin);

	// ����x��ı�ע
	void setXTitle(const QString & xTitle);

	// ����y��ı�ע
	void setYTitle(const QString & yTitle);

	// ���������������
	void setTitleFont(QFont font);

	// ����XY��������
	void setXYTitleFont(QFont font);

	// �����Ƿ�滭����
	void setDrawGrid(bool isDrawGrid);

	// �Ƿ�תX��
	void setFlipX(bool isFlipX);

	// �Ƿ�תY��
	void setFlipY(bool isFlipY);

	// ���ú�����ʾ�ļ��
	void setHRulerLdx (double ldx);

	// ����������ʾ�ļ��
	void setVRulerLdy (double ldy);

	// ��ȡ������ʾ�ļ��
	double getHRulerLdx();

	// ��ȡ������ʾ�ļ��
	double getVRulerLdy();

	// ���ú�����������
	void setXRulerCount(int count);

	// ����������������
	void setYRulerCount(int count);

	// ���û�ͼ������ɫ
	void setBackground(QColor cr);

	// �õ�x��y������߼��仯��Χ����������������ֵ��
	virtual void statDataRange();

	// ����������ñ仯��Χ
	void setDataRange(double minX, double minY, double maxX, double maxY);

	void setXDataRange(double minX, double maxX);

	void setYDataRange(double minY, double maxY);

	// ��ȡ��õķ�Χ
	double getRangeMinX();

	double getRangeMaxX();

	double getRangeMinY();

	double getRangeMaxY();
};
