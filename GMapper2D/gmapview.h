#ifndef GVIEW_H
#define GVIEW_H

#include "gcvtcoordinate.h"
#include "gmapper2d_global.h"

#include <QWidget>
#include <QLabel>
#include <QHelpEvent>
#include <QHelpEvent>
#include <functional>

class GMapTileLoader;
class GMapLayerSet;
class GMapLayer;

// ��ͼ��Ƭ�ṹ��
struct GMapTile;

enum GMapSelectMode
{
    SELECT_NONE = 0x0000,
    SELECT_RECT = 0x0001
};

// ��ͼ��ͼ��
class GMAPPER2D_EXPORT GMapView: public QWidget, public GCvtCoordinate
{
    Q_OBJECT

protected:
    GMapLayerSet* mpMapLayerSet;		// ͼ�㼯��
    GMapLayer* mpActiveLayer;		    // ��ǰ����ͼ��

    QSize					mTileSize;			// ������Ƭ�ߴ�
    int					    mElevTileSize;	    // �߳���Ƭ�ߴ�
    QSize                   mGraphSize;         // ��ͼ����ߴ�(��Ļ���سߴ�)
    QRect					mCustomMargins;		// �û�ָ���ı߾�
    QRect                   mMargins;           // �߾�(�������ҵ�ʵ�ʱ߾�)
    int						mScaleGrade;		// ���ŵȼ�
    int						mMinScaleGrade;		// ��С���ŵȼ�
    int						mMaxScaleGrade;		// ������ŵȼ�
    qreal					mScale;				// ���ű���(�������� / �߼�����)
    QRectF					mBoundingRect;		// �߼����귶Χ
    QRectF					mViewRect;			// ��ǰ�ӿ���ʾ���߼����귶Χ
    GMapSelectMode			mSelectMode;		// ѡ��ģʽ
    QRectF					mSelectedRect;		// �û�ѡ�����, ��λΪī����ͶӰ�����
    bool					mIsLeftDrag;		// �������Ƿ��ڰ�����ק״̬
    bool					mIsRightDrag;		// ����Ҽ��Ƿ��ڰ�����ק״̬
    int						mPreX, mPreY;		// �ϴ���갴��λ��
    bool					mIsDragRect;		// ��ק��Ϣ����
    bool					mIsDrawZoomRect;	// �Ƿ���ƷŴ���ο�
    QRect					mZoomRect;			// �Ŵ����

    int						mMaxTileCaheSize;	// ��Ƭ�����������
    QVector<GMapTile*>		mTileCache;			// ��Ƭ����
    GMapTileLoader* mpTileLoader;		        // ��Ƭ������
    bool					mIsUpdateMapTile;	// �Ƿ���µ�ͼ��Ƭ
    QVector<GMapTile*>		mTiles;				// ��ǰ������ʾ����Ƭ�б�

    QLabel* mInfoLabel;		                    // ��ϢLabel
    //QLabel* mScaleLabel;		                // ���ż���Label
    QString					mScaleGradeStr;		// ��ʾ�����ַ���
    QString					mLonLatStr;			// ��γ���ַ���

    char					mDownloadFormat[256];
    bool					mIsMapEnabled;		// ������ʾ��ͼ

    // ------------------�������� 2021.8.22 ˧����
    QRect mSelectedRectDp; // ѡ����ο���Ļ���귶Χ���Ƕ� mSelectedRect �Ĳ���
    QImage mSelectedImage; // ѡ����ο�Χ�ĵ�ͼ
    //2023-9-10
    qreal mScaleConst = 0.000408833;
    qreal mViewLeftConst= 1.17192e+07;
    qreal mMarginsLeftConst = 0;
    qreal mViewBottomConst = 4.55382e+06;
    qreal mMarginsTopConst = 0;
    
protected:

    // ��Cache�в�����Ƭ
    GMapTile* findCacheTile(int grade, int row, int col);

    // ��Cache�������Ƭ
    void addCacheTile(GMapTile* pTile);

    // ����mViewRect��Χ���µ�ͼ��Ƭ�ɼ���
    void updateMapTiles();

    // ������ͼ
    virtual void zoom(int scaleGrade, int mouseX = -1, int mouseY = -1);

    // �ߴ�仯�¼�����
    virtual void resizeEvent(QResizeEvent* e);

    // ����״̬��Ϣ
    void drawStatusInfo(QPainter* p);

    // ����
    virtual void paintEvent(QPaintEvent* e);

    // ����¼�������
    virtual void wheelEvent(QWheelEvent* e);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);

    // �����¼�������
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);

    // ����Tooltip���⺯��
    virtual bool event(QEvent* e);

public:
    GMapView(QWidget* parent);
    virtual ~GMapView();

    // �Ƿ�������ʾ��ͼ
    bool isMapEnabled() const;

    // ��ȡ���������߼�����
    QRectF viewRect() const;

    // �����Ƿ���ʾ��ͼ
    void setMapEnabled(bool isEnabled);

    // �����Ƭ����
    void clearCache();

    // ����ͼ�㼯��
    GMapLayerSet* layerSet();

    // ���ü���ͼ��
    void setActiveLayer(GMapLayer* pLayer);

    // �������ű����ȼ�(1-20��)
    void setScaleGrade(int s);

    // ��ȡ���ŵȼ�
    int scaleGrade() const;

    // ��ȡ���ű���
    double scale() const;

    // ����������ͼī����ͶӰ���η�Χ
    virtual void setBoundingRect(const QRectF& rect);

    // �õ�������ͼī����ͶӰ���η�Χ
    const QRectF& boundingRect() const;

    // ˢ����ͼ
    virtual void refresh();

    // ������Ϣ�����ǩ
    void setInfoLabel(QLabel* label);

    // ����γ������ת��Ϊ�ȷ����ʽ���ַ���
    static QString formatDMS(double decimal);

    virtual int	lpXTodpX(qreal lx) const;			// �߼�X����ת������ĻX����
    virtual int	lpYTodpY(qreal ly) const;			// �߼�Y����ת������ĻY����
    virtual qreal dpXTolpX(int dx) const;			// ��Ļ���굽ʵ������
    virtual qreal dpYTolpY(int dy) const;			// ��Ļ���굽ʵ������	

    virtual int	lpXTodpXc(qreal lx) const;			// �߼�X����ת������ĻX����
    virtual int	lpYTodpYc(qreal ly) const;			// �߼�Y����ת������ĻY����
    virtual qreal dpXTolpXc(int dx) const;			// ��Ļ���굽ʵ������
    virtual qreal dpYTolpYc(int dy) const;			// ��Ļ���굽ʵ������	


    // ��ָ����������Ϊ��ͼ����(��γ������)
    void centerOn(const QRectF& rect);

    // ��ָ����������Ϊ��ͼ����(ī��������)
    void centerOnM(const QRectF& rect);

    // ��������ѡ��ģʽ
    void setSelectMode(GMapSelectMode mode);

    // ��ȡ�û�ѡ���������
    const QRectF& getSelectedRect() const;

    // ����ѡ���������
    void setSelectedRect(const QRectF& rect);

    // �õ�ѡ�е���Ƭ����
    QVector<GMapTile*> getSelectedTiles(QRect& iBound, int align = 1);

    // ���õ�ͼ���ظ�ʽ
    void setDownloadFormat(const char* fmt);

    // ������ѡ�����ͼ�� rectΪ��Ļ��������
    // return ÿ�׶�������
    QImage saveSelectRectImage(const QRect& rect);

    // ��ȡ��ǰ�߳�ͼ��
    QImage getElevationImage(const QRect& rect);

    protected
slots:
    // ������Ƭ��������¼�
    void onHttpTileFinished(GMapTile* pTile);

signals:
    // ����ѡ�������Ļص����� rectΪ��Ļ����
    void onSelectedRect(const QRect& rect);
};

#endif