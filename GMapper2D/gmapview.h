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

// 地图瓦片结构体
struct GMapTile;

enum GMapSelectMode
{
    SELECT_NONE = 0x0000,
    SELECT_RECT = 0x0001
};

// 地图视图类
class GMAPPER2D_EXPORT GMapView: public QWidget, public GCvtCoordinate
{
    Q_OBJECT

protected:
    GMapLayerSet* mpMapLayerSet;		// 图层集合
    GMapLayer* mpActiveLayer;		    // 当前激活图层

    QSize					mTileSize;			// 基本瓦片尺寸
    int					    mElevTileSize;	    // 高程瓦片尺寸
    QSize                   mGraphSize;         // 绘图区域尺寸(屏幕像素尺寸)
    QRect					mCustomMargins;		// 用户指定的边距
    QRect                   mMargins;           // 边距(上下左右的实际边距)
    int						mScaleGrade;		// 缩放等级
    int						mMinScaleGrade;		// 最小缩放等级
    int						mMaxScaleGrade;		// 最大缩放等级
    qreal					mScale;				// 缩放比例(像素坐标 / 逻辑坐标)
    QRectF					mBoundingRect;		// 逻辑坐标范围
    QRectF					mViewRect;			// 当前视口显示的逻辑坐标范围
    GMapSelectMode			mSelectMode;		// 选择模式
    QRectF					mSelectedRect;		// 用户选择矩形, 单位为墨卡托投影后距离
    bool					mIsLeftDrag;		// 鼠标左键是否处于按下拖拽状态
    bool					mIsRightDrag;		// 鼠标右键是否处于按下拖拽状态
    int						mPreX, mPreY;		// 上次鼠标按下位置
    bool					mIsDragRect;		// 拖拽信息矩形
    bool					mIsDrawZoomRect;	// 是否绘制放大矩形框
    QRect					mZoomRect;			// 放大矩形

    int						mMaxTileCaheSize;	// 瓦片缓存最大容量
    QVector<GMapTile*>		mTileCache;			// 瓦片缓存
    GMapTileLoader* mpTileLoader;		        // 瓦片加载器
    bool					mIsUpdateMapTile;	// 是否更新地图瓦片
    QVector<GMapTile*>		mTiles;				// 当前用于显示的瓦片列表

    QLabel* mInfoLabel;		                    // 信息Label
    //QLabel* mScaleLabel;		                // 缩放级别Label
    QString					mScaleGradeStr;		// 显示级别字符串
    QString					mLonLatStr;			// 经纬度字符串

    char					mDownloadFormat[256];
    bool					mIsMapEnabled;		// 允许显示地图

    // ------------------新增变量 2021.8.22 帅鹏飞
    QRect mSelectedRectDp; // 选择矩形框屏幕坐标范围，是对 mSelectedRect 的补充
    QImage mSelectedImage; // 选择矩形框范围的地图
    //2023-9-10
    qreal mScaleConst = 0.000408833;
    qreal mViewLeftConst= 1.17192e+07;
    qreal mMarginsLeftConst = 0;
    qreal mViewBottomConst = 4.55382e+06;
    qreal mMarginsTopConst = 0;
    
protected:

    // 在Cache中查找瓦片
    GMapTile* findCacheTile(int grade, int row, int col);

    // 向Cache中添加瓦片
    void addCacheTile(GMapTile* pTile);

    // 根据mViewRect范围更新地图瓦片可见性
    void updateMapTiles();

    // 缩放视图
    virtual void zoom(int scaleGrade, int mouseX = -1, int mouseY = -1);

    // 尺寸变化事件函数
    virtual void resizeEvent(QResizeEvent* e);

    // 绘制状态信息
    void drawStatusInfo(QPainter* p);

    // 绘制
    virtual void paintEvent(QPaintEvent* e);

    // 鼠标事件函数组
    virtual void wheelEvent(QWheelEvent* e);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);

    // 键盘事件函数组
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);

    // 弹出Tooltip虚拟函数
    virtual bool event(QEvent* e);

public:
    GMapView(QWidget* parent);
    virtual ~GMapView();

    // 是否允许显示地图
    bool isMapEnabled() const;

    // 获取可视区域逻辑坐标
    QRectF viewRect() const;

    // 设置是否显示地图
    void setMapEnabled(bool isEnabled);

    // 清空瓦片缓存
    void clearCache();

    // 返回图层集合
    GMapLayerSet* layerSet();

    // 设置激活图层
    void setActiveLayer(GMapLayer* pLayer);

    // 设置缩放比例等级(1-20级)
    void setScaleGrade(int s);

    // 获取缩放等级
    int scaleGrade() const;

    // 获取缩放比例
    double scale() const;

    // 设置整个地图墨卡托投影矩形范围
    virtual void setBoundingRect(const QRectF& rect);

    // 得到整个地图墨卡托投影矩形范围
    const QRectF& boundingRect() const;

    // 刷新视图
    virtual void refresh();

    // 设置信息输出标签
    void setInfoLabel(QLabel* label);

    // 将经纬度坐标转换为度分秒格式的字符串
    static QString formatDMS(double decimal);

    virtual int	lpXTodpX(qreal lx) const;			// 逻辑X坐标转换到屏幕X坐标
    virtual int	lpYTodpY(qreal ly) const;			// 逻辑Y坐标转换到屏幕Y坐标
    virtual qreal dpXTolpX(int dx) const;			// 屏幕坐标到实际坐标
    virtual qreal dpYTolpY(int dy) const;			// 屏幕坐标到实际坐标	

    virtual int	lpXTodpXc(qreal lx) const;			// 逻辑X坐标转换到屏幕X坐标
    virtual int	lpYTodpYc(qreal ly) const;			// 逻辑Y坐标转换到屏幕Y坐标
    virtual qreal dpXTolpXc(int dx) const;			// 屏幕坐标到实际坐标
    virtual qreal dpYTolpYc(int dy) const;			// 屏幕坐标到实际坐标	


    // 以指定矩形区域为视图中央(经纬度坐标)
    void centerOn(const QRectF& rect);

    // 以指定矩形区域为视图中央(墨卡托坐标)
    void centerOnM(const QRectF& rect);

    // 设置区域选择模式
    void setSelectMode(GMapSelectMode mode);

    // 获取用户选择矩形区域
    const QRectF& getSelectedRect() const;

    // 设置选择矩形区域
    void setSelectedRect(const QRectF& rect);

    // 得到选中的瓦片数据
    QVector<GMapTile*> getSelectedTiles(QRect& iBound, int align = 1);

    // 设置地图下载格式
    void setDownloadFormat(const char* fmt);

    // 保存所选区域的图像 rect为屏幕像素坐标
    // return 每米多少像素
    QImage saveSelectRectImage(const QRect& rect);

    // 获取当前高程图像
    QImage getElevationImage(const QRect& rect);

    protected
slots:
    // 网络瓦片下载完成事件
    void onHttpTileFinished(GMapTile* pTile);

signals:
    // 设置选择区域后的回调函数 rect为屏幕坐标
    void onSelectedRect(const QRect& rect);
};

#endif