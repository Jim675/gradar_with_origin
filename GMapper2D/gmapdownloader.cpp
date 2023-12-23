#include <QApplication>
#include <QDir>
#include "gmapdownloader.h"
#include "gmaptile.h"
#include "gmaptile.h"
#include "gmath.h"
#include "gutils.h"
#include "ggpscorrect.h"
#include "gmapcoordconvert.h"

GMapDownloader::GMapDownloader(QObject* parent): QObject(parent),
mWorldRect(QPointF(-20037508.3427892, -20037508.3427892), QPointF(20037508.3427892, 20037508.3427892))
{
    mProgressDlg = NULL;

    mpTileLoader = new GMapTileLoader(this);
    mpTileLoader->setMapTilesPath("./tiles");
    mpTileLoader->setCreateSubPath(true);
    connect(mpTileLoader, SIGNAL(localTileFinished(GMapTile*)),
            this, SLOT(onTileFinished(GMapTile*)));
    connect(mpTileLoader, SIGNAL(httpTileFinished(GMapTile*)),
            this, SLOT(onTileFinished(GMapTile*)));

}

GMapDownloader::~GMapDownloader()
{
    delete mpTileLoader;
}

bool GMapDownloader::prepare(const QRectF& lonLatRect, int scaleGrade)
{
    // 确定地图经纬度扩展范围
    GGPSCorrect gpsCorrect;
    double minLon, minLat, maxLon, maxLat, adjw, adjh;
    mLonLatRect = lonLatRect;		// mLonLatRect保存WGS84无偏经纬度范围
    minLon = lonLatRect.left();		maxLon = lonLatRect.right();
    minLat = lonLatRect.top();		maxLat = lonLatRect.bottom();
    gpsCorrect.wgsTogcj2(minLon, minLat, minLon, minLat);
    gpsCorrect.wgsTogcj2(maxLon, maxLat, maxLon, maxLat);
    /*adjw = (maxLon - minLon) * 0.1;
    adjh = (maxLat - minLat) * 0.1;
    minLon -= adjw;		maxLon += adjw;
    minLat -= adjh;		maxLat += adjh;*/
    // mLonLatExtendRect保存加偏经纬度范围
    mLonLatExtendRect.setTopLeft(QPointF(minLon, minLat));
    mLonLatExtendRect.setBottomRight(QPointF(maxLon, maxLat));

    mLonLatExtendRect = lonLatRect;

    // 计算工区的墨卡托坐标范围
    QRectF viewRect;
    viewRect.setTopLeft(GMapCoordConvert::lonLatToMercator(mLonLatExtendRect.topLeft()));
    viewRect.setBottomRight(GMapCoordConvert::lonLatToMercator(mLonLatExtendRect.bottomRight()));

    // 产生Tiles
    double scale = 256 * (1 << (scaleGrade - 1)) / mWorldRect.width();
    int nx = 1 << (scaleGrade - 1);
    int ny = 1 << (scaleGrade - 1);
    double ldx = 256.0 / scale;
    double ldy = 256.0 / scale;

    int ix0, ix1, iy0, iy1;
    ix0 = int((viewRect.left() - mWorldRect.left()) / ldx);
    if (ix0 < 0) ix0 = 0;
    ix1 = int((viewRect.right() - mWorldRect.left()) / ldx);
    if (ix1 >= nx) ix1 = nx - 1;
    iy0 = int((viewRect.top() - mWorldRect.top()) / ldy);
    if (iy0 < 0) iy0 = 0;
    iy1 = int((viewRect.bottom() - mWorldRect.top()) / ldy);
    if (iy1 >= ny) iy1 = ny - 1;

    printf("y0 = %d, y1 = %d\n", ny - iy1 - 1, ny - iy0 - 1);

    qDeleteAll(mImageTiles);
    mImageTiles.clear();
    for (int i = iy0; i <= iy1; i++) {
        for (int j = ix0; j <= ix1; j++) {
            GMapTile* pTile = new GMapTile(scaleGrade, ny - i - 1, j);
            mImageTiles.append(pTile);
        }
    }

    //printf("mLonLatExtendRect: (%.4f, %.4f, %.4f, %.4f)\n", mLonLatExtendRect.left(), mLonLatExtendRect.top(),
    //	mLonLatExtendRect.right(), mLonLatExtendRect.bottom());

    return true;
}

int GMapDownloader::tileCount() const
{
    return mImageTiles.count();
}

void GMapDownloader::onTileFinished(GMapTile* pTile)
{
    if (mIsDownloadCancled) return;
    // if (pTile->mState != GMapTile::gLoaded) return;

    pTile->mImage = QImage(0, 0);	// 释放图像内存
    if (mProgressDlg) {
        mProgressDlg->setValue((mDownloadCount + 1) * 100 / mImageTiles.count());
    }
    mDownloadCount++;
}

bool GMapDownloader::download(const QRectF& lonLatRect, int fromGrade, int toGrade, const char* fmt)
{
    bool isCanceled = false;
    mProgressDlg = new QProgressDialog();
    mProgressDlg->setCancelButtonText("&Cancel");
    mProgressDlg->setAutoClose(false);
    mProgressDlg->setModal(true);

    // 加载卫星瓦片数据
    mProgressDlg->setMaximum(100);
    mProgressDlg->show();
    mIsDownloadCancled = false;

    for (int it = fromGrade; it <= toGrade; it++) {
        mProgressDlg->setLabelText(tr("Download Tile Map(Grade: %1)...").arg(it));
        mProgressDlg->setValue(0);
        prepare(lonLatRect, it);
        int n = mImageTiles.count();
        mDownloadCount = 0;
        mpTileLoader->loadNoCache(mImageTiles, fmt);

        while (mDownloadCount < n) {
            qApp->processEvents();
            if (mProgressDlg->wasCanceled()) {
                mIsDownloadCancled = true;
                mpTileLoader->cancelLoad();
                isCanceled = true;
                break;
            }
        }

        mpTileLoader->cancelLoad();
        qDeleteAll(mImageTiles);
        mImageTiles.clear();
        if (isCanceled) break;
    }

    //qDeleteAll(mImageTiles);
    delete mProgressDlg;
    mProgressDlg = NULL;
    return !isCanceled;
}

