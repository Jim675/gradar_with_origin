#ifndef GMAP2D_MAKER_H
#define GMAP2D_MAKER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QProgressDialog>
#include "gmaptile.h"
#include "gmapper2d_global.h"

class GMAPPER2D_EXPORT GMapDownloader: public QObject
{
    Q_OBJECT

private:
    QString					mDirPath;			// 目录名
    QRectF					mWorldRect;			// 墨卡托世界坐标范围
    QRectF					mLonLatRect;		// 下载经纬度范围
    QRectF					mLonLatExtendRect;	// 扩展的地图经纬度范围
    QRectF					mImageLonLatRect;	// 卫照图像经纬度范围
    GMapTileLoader* mpTileLoader;		// 瓦片加载器
    double					mTileScale;			// 瓦片缩放比例
    QVector<GMapTile*>		mImageTiles;		// 瓦片图像列表
    int						mDownloadCount;		// 瓦片下载数目
    QProgressDialog* mProgressDlg;		// 进度对话框
    bool					mIsDownloadCancled;	// 是否取消下载

public:
    GMapDownloader(QObject* parent = 0);
    ~GMapDownloader();

    // 产生地图数据
    bool download(const QRectF& lonLatRect, int fromGrade, int toGrade, const char* fmt = GOOGLE_SR_FMT);

    // 获取下载瓦片数量
    int tileCount() const;

private:

    // 进行初始化准备
    bool prepare(const QRectF& lonLatRect, int scaleGrade);

protected slots:
    // 响应瓦片下载完成事件, 拼接地图
    void onTileFinished(GMapTile* pTile);
};


#endif
