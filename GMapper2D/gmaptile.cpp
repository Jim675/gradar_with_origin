#include "gmaptile.h"

#include <QFile>
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>
#include <QWidget>
#include <QPainter>
#include <QTime>
#include <QApplication>
#include <QMap>
#include <qimagereader.h>


const char* GOOGLE_S_FMT = "http://mt%d.google.cn/vt/lyrs=p&hl=zh-CN&gl=cn&x=%d&y=%d&z=%d";
const char* GOOGLE_SR_FMT = "http://mt%d.google.cn/vt?lyrs=p@142&x=%d&y=%d&z=%d&apistyle=s.t:3%7Cs.e:l.t%7Cp.v:off,s.t:2%7Cp.v:off,s.t:1%7Cs.e:l%7Cp.v:off,s.t:5%7Cs.e:l%7Cp.v:off,s.e:l%7Cp.v:off&style=api%7Csmartmaps";

//"http://mt%d.google.cn/vt/lyrs=p&hl=zh-CN&gl=cn&x=%d&y=%d&z=%d";
const char* GOOGLE_M_FMT = "http://mt%d.google.cn/vt?lyrs=m@142&x=%d&y=%d&z=%d&apistyle=s.t:3%7Cs.e:l.t%7Cp.v:off,s.t:2%7Cp.v:off,s.t:1%7Cs.e:l%7Cp.v:off,s.t:5%7Cs.e:l%7Cp.v:off,s.e:l%7Cp.v:off&style=api%7Csmartmaps";

const char* TECENT_SR_FMT = "http://p%d.map.gtimg.com/sateTiles/%d/%d/%d/%d_%d.jpg";

//----struct GMapTile-----------------------------------------------------------
// 瓦片初始化时图像设为0尺寸, 可以减少开销
GMapTile::GMapTile(int grade, int row, int col): mImage(0, 0, QImage::Format_ARGB32)
{
    mGrade = grade;
    mRow = row;
    mCol = col;
    mState = gUnLoad;
}

GMapTile::~GMapTile()
{
}

bool GMapTile::operator == (const GMapTile& other)
{
    if (mGrade == other.mGrade && mRow == other.mRow && mCol == other.mCol)
        return true;
    else return false;
}

bool GMapTile::operator != (const GMapTile& other)
{
    return !(*this == other);
}

//----struct GMapServerTest-----------------------------------------------------
struct GMapServerTest
{
    int		mId;		// 服务器ID
    QTime	mTimer;		// 计时器
    int		mElpased;	// 花费时间
    QNetworkReply::NetworkError	mError;		// 网络错误

    GMapServerTest(int id = 0)
    {
        mId = id;
        mElpased = 0;
        mError = QNetworkReply::NoError;
    }
};

//----class GMapTileLoader------------------------------------------------------
int GMapTileLoader::mServerId = 0;

QMap<QNetworkReply*, GMapServerTest*>	GMapTileLoader::mTestReplyMap;

GMapTileLoader::GMapTileLoader(QObject* parent): QObject(parent), mNetManager(this)
{
    mMaxAccessCount = 64;
    mHttpTimerId = -1;
    mMapTilesPath = "./tiles";		// 瓦片存盘目录默认为当前目录
    mIsCreateSubDir = true;
    mIsSaveToCache = true;
    mDownloadFormat = GOOGLE_SR_FMT;
    mMapType = 0;
    // 感觉是Qt的Bug, 在执行了大量操作后首次调用get函数将会非常缓慢, 因此构造时就调用一次
    //QString str;
    //str.sprintf(GOOGLE_SR_FMT, 0, 0, 0, 0);
    //QNetworkReply *pReply = mNetManager.get(QNetworkRequest(QUrl(str)));
    //pReply->deleteLater();
    //connect(pReply, SIGNAL(finished()), this, SLOT(httpTestFinished())); 
}

GMapTileLoader::~GMapTileLoader()
{

}

bool GMapTileLoader::testServer(int maxTime)
{
    static bool isTesting = false;

    if (isTesting) return false;

    isTesting = true;
    qDeleteAll(mTestReplyMap);
    mTestReplyMap.clear();

    GMapTileLoader loader = new GMapTileLoader;
    QNetworkAccessManager* pManager = &loader.mNetManager;

    for (int i = 0; i < 4; i++) {
        QString str;
        str.sprintf(GOOGLE_S_FMT, i, 0, 0, 0);

        QNetworkReply* pReply = pManager->get(QNetworkRequest(QUrl(str)));
        connect(pReply, SIGNAL(finished()), &loader, SLOT(httpTestFinished()));
        GMapServerTest* pTest = new GMapServerTest(i);
        pTest->mElpased = -1;
        pTest->mTimer.start();
        mTestReplyMap[pReply] = pTest;
    }

    QTime timer;
    timer.start();
    GMapServerTest* pFinishedTest = NULL;
    while (timer.elapsed() <= maxTime) {
        qApp->processEvents();
        foreach(GMapServerTest * pTest, mTestReplyMap)
        {
            if (pTest->mElpased >= 0 && pTest->mError == QNetworkReply::NoError) {
                pFinishedTest = pTest;
                mServerId = pTest->mId;
                break;
            }
        }

        if (pFinishedTest) break;
    }

    if (pFinishedTest != NULL) {
        foreach(QNetworkReply * pReply, mTestReplyMap.keys())
        {
            if (mTestReplyMap[pReply] != pFinishedTest) {
                pReply->abort();
                pReply->deleteLater();
            }
        }
    }
    qDeleteAll(mTestReplyMap);
    mTestReplyMap.clear();

    isTesting = false;
    if (pFinishedTest) return true;
    else return false;
}

void GMapTileLoader::setServerId(int id)
{
    if (id < 0) id = 0;
    if (id > 3) id = 3;
    mServerId = id;
}

int GMapTileLoader::getServerId()
{
    return mServerId;
}

const QString& GMapTileLoader::savePath() const
{
    return mMapTilesPath;
}

void GMapTileLoader::setMapTilesPath(const QString& mapTilesPath)
{
    mMapTilesPath = mapTilesPath;
}

// 设置本地高程瓦片路径
void GMapTileLoader::setElevTilesPath(const QString& elevTilesPath)
{
    mElevTilesPath = elevTilesPath;
}

bool GMapTileLoader::isCreateSubPath() const
{
    return mIsCreateSubDir;
}

void GMapTileLoader::setCreateSubPath(bool isCreate)
{
    mIsCreateSubDir = isCreate;
}

bool GMapTileLoader::loadFromLocal(GMapTile* pTile)
{
    QString fname;
    if (mIsCreateSubDir) {
        fname = QString::asprintf("/L%02d/R%06d/C%06d.jpg",
                                  pTile->mGrade, pTile->mRow, pTile->mCol);
    } else {
        fname = QString::asprintf("/L%02d_R%06d_C%06d.jpg",
                                  pTile->mGrade, pTile->mRow, pTile->mCol);
    }
    fname = mMapTilesPath + fname;
    auto a = QImageReader::supportedImageFormats();
    if (pTile->mImage.load(fname, "jpg")) {
        QImage::Format fmt = pTile->mImage.format();
        if (pTile->mImage.format() != QImage::Format_RGB32) {
            pTile->mImage = pTile->mImage.convertToFormat(QImage::Format_RGB32);
        }
        pTile->mState &= ~(GMapTile::gUnLoad | GMapTile::gLoading);
        pTile->mState |= GMapTile::gLoaded;
        return true;
    } else {
        QFileInfo info(fname);
        if (info.exists()) {
            qDebug() << fname;
        }
        // 加载最近的瓦片
        loadLocalNearest(pTile);
        if (mMapType == 0 && pTile->mGrade > 22) return true;
        else if (mMapType == 1 && pTile->mGrade > 19) return true;
        else return false;
        return false;
    }
}

// 加载最接近的瓦片地图
bool GMapTileLoader::loadLocalNearest(GMapTile* pTile)
{
    int i, j, grade, row, col, x, y, nlevel, sz;
    QVector<QPoint> pos;
    QString fname;
    QImage image;

    grade = pTile->mGrade;
    row = pTile->mRow;
    col = pTile->mCol;
    nlevel = 8;

    //for(i=0; i<nlevel; i++)
    while (grade >= 4) {
        grade--;
        y = row % 2;	x = col % 2;
        pos.append(QPoint(x, y));
        row /= 2;		col /= 2;
        fname.sprintf("/L%02d/R%06d/C%06d.jpg", grade, row, col);
        fname = mMapTilesPath + fname;
        if (image.load(fname, "jpg")) {
            sz = 256;
            x = y = 0;
            for (j = pos.count() - 1; j >= 0; j--) {
                sz /= 2;
                if (sz <= 0) sz = 1;
                x += sz * pos[j].x();
                y += sz * pos[j].y();
            }
            //QPainter p(&(pTile->mImage));
            //p.drawImage(QRect(0, 0, 256, 256), image, QRect(x, y, sz, sz));
            pTile->mImage = image.copy(QRect(x, y, sz, sz)).scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            //image.copy(QRect(x, y, sz, sz)).save("tt.jpg");

            // 2022-1-11 设置为gLoaded状态方便放入缓存--------------
            pTile->mState &= ~(GMapTile::gUnLoad | GMapTile::gLoading);
            pTile->mState |= GMapTile::gLoaded;
            // ---------------------------------
            return true;
        }
    }
    return false;
}

int GMapTileLoader::findTile(const QVector<GMapTile*>& tiles, const GMapTile* pTile)
{
    for (int i = 0; i < tiles.count(); i++) {
        GMapTile* qTile = tiles[i];
        if (*qTile == *pTile) {
            return i;
        }
    }
    return -1;
}

void GMapTileLoader::httpRequest(GMapTile* pTile, const char* fmt)
{
    mHttpTiles.append(pTile);
    QString str;
    if (strstr(fmt, "google")) {
        str.sprintf(fmt, mServerId, pTile->mCol, pTile->mRow, pTile->mGrade - 1);
    } else {
        int z = pTile->mGrade - 1;
        int x = pTile->mCol;
        int y = (1 << z) - 1 - pTile->mRow;
        str.sprintf(fmt, mServerId, z, x / 16, y / 16, x, y);
    }

    QNetworkReply* pReply = mNetManager.get(QNetworkRequest(QUrl::fromEncoded(str.toLocal8Bit())));
    mReplyMap[pReply] = pTile;
    connect(pReply, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(pReply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
}

void GMapTileLoader::loadNoCache(QVector<GMapTile*>& tiles, const char* fmt)
{
    mDownloadFormat = fmt;
    if (strstr(fmt, "google")) mMapType = 0;
    else mMapType = 1;

    // 先从本地下载, 并找出本地不存在的瓦片存储到httpTiles中
    for (int i = 0; i < tiles.count(); i++) {
        GMapTile* pTile = tiles[i];
        pTile->mState |= GMapTile::gLoading;
        pTile->mState &= ~(GMapTile::gUnLoad | GMapTile::gLoaded);

        // 如果瓦片已经在下载或者等待下载则不重复加入
        if (findTile(mHttpTiles, pTile) >= 0 || findTile(mWaitTiles, pTile) >= 0) {
            printf("(%d, %d, %d) has been loading!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
            continue;
        }

        if (mHttpTiles.count() < mMaxAccessCount) {
            httpRequest(pTile, fmt);
            //printf("(%d, %d, %d) is loading!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
        } else {
            mWaitTiles.append(pTile);
            //printf("(%d, %d, %d) is waiting for load!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
        }
    }
}

void GMapTileLoader::load(const QVector<GMapTile*>& tiles, const char* fmt)
{
    mDownloadFormat = fmt;
    if (strstr(fmt, "google")) mMapType = 0;
    else mMapType = 1;
    // 先从本地下载, 并找出本地不存在的瓦片存储到httpTiles中
    for (int i = 0; i < tiles.count(); i++) {
        GMapTile* pTile = tiles[i];
        pTile->mState &= ~(GMapTile::gUnLoad | GMapTile::gLoaded);
        pTile->mState |= GMapTile::gLoading;

        if (loadFromLocal(pTile)) {
            emit localTileFinished(pTile);
        } else {
            // 关闭网络加载
            //return;
            //// 如果瓦片已经在下载或者等待下载则不重复加入
            //if (findTile(mHttpTiles, pTile) >= 0 || findTile(mWaitTiles, pTile) >= 0) {
            //    //printf("(%d, %d, %d) has been loading!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
            //    continue;
            //}

            //if (mHttpTiles.count() < mMaxAccessCount) {
            //    httpRequest(pTile, fmt);
            //    //printf("(%d, %d, %d) is loading!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
            //} else {
            //    mWaitTiles.append(pTile);
            //    //printf("(%d, %d, %d) is waiting for load!\n", pTile->mGrade, pTile->mRow, pTile->mCol);
            //}
        }
    }

    //printf("Download Queue Size = %d\n", mHttpTiles.count());
//  	if (mHttpTiles.count() > 0 && mHttpTimerId < 0)
//  	{
//  		mHttpTimerId = startTimer(500);
//  	}
}

// 加载本地高程瓦片
bool GMapTileLoader::loadElevTileFromLocal(GMapTile* pTile)
{
    QString fname(mElevTilesPath);
    fname.append(QString::asprintf("/%d/%d_%d.tif",
                 pTile->mGrade, pTile->mRow, pTile->mCol));
    if (pTile->mImage.load(fname, "tif")) {
        //QImage::Format fmt = pTile->mImage.format();
        //if (pTile->mImage.format() != QImage::Format_RGB32) {
        //    pTile->mImage.convertToFormat(QImage::Format_RGB32);
        //}
        auto a = pTile->mImage.format();
        pTile->mState &= ~(GMapTile::gUnLoad | GMapTile::gLoading);
        pTile->mState |= GMapTile::gLoaded;
        return true;
    }
    return false;
}

void GMapTileLoader::cancelLoad()
{
    QMap<QNetworkReply*, GMapTile*>::iterator it;
    for (it = mReplyMap.begin(); it != mReplyMap.end(); ++it) {
        QNetworkReply* pReply = it.key();
        pReply->abort();
        pReply->deleteLater();

        //GMapTile *pTile = it.value();
        //pTile->mState &= ~GMapTile::gLoading;
        //pTile->mState |= GMapTile::gUnLoad;
    }
    mReplyMap.clear();
    mWaitTiles.clear();
    mHttpTiles.clear();
}

void GMapTileLoader::httpReadyRead()
{
}

void GMapTileLoader::httpTestFinished()
{
    QNetworkReply* pReply = static_cast<QNetworkReply*>(sender());
    if (!pReply) return;

    GMapServerTest* pTest = mTestReplyMap.value(pReply, NULL);
    if (!pTest) return;
    pTest->mElpased = pTest->mTimer.elapsed();
    pTest->mError = pReply->error();

    pReply->deleteLater();
}

void GMapTileLoader::httpFinished()
{
    QNetworkReply* pReply = static_cast<QNetworkReply*>(sender());
    if (!pReply) return;

    if (mWaitTiles.count() > 0) {
        GMapTile* pTile = mWaitTiles[0];
        mWaitTiles.remove(0);
        httpRequest(pTile, mDownloadFormat);
    }

    GMapTile* pTile = mReplyMap[pReply];
    if (pTile) {
        QString pname, fname;
        if (mIsSaveToCache) {
            if (mIsCreateSubDir) {
                pname.sprintf("/L%02d/R%06d", pTile->mGrade, pTile->mRow);
                pname = mMapTilesPath + pname;
            } else pname = mMapTilesPath;

            QDir dir;
            if (!dir.exists(pname)) {
                dir.mkpath(pname);
            }

            if (mIsCreateSubDir) {
                fname.sprintf("/L%02d/R%06d/C%06d.jpg",
                              pTile->mGrade, pTile->mRow, pTile->mCol);
            } else {
                fname.sprintf("/L%02d_R%06d_C%06d.jpg",
                              pTile->mGrade, pTile->mRow, pTile->mCol);
            }
            fname = mMapTilesPath + fname;
        }

        mHttpTiles.remove(mHttpTiles.indexOf(pTile));

        QByteArray ba = pReply->readAll();
        QImage image;
        if (image.loadFromData(ba, "JPG")) {
            pTile->mImage = image;
            if (pTile->mImage.format() != QImage::Format_RGB32 &&
                pTile->mImage.format() != QImage::Format_ARGB32) {
                pTile->mImage = pTile->mImage.convertToFormat(QImage::Format_RGB32);
            }
            if (mIsSaveToCache) {
                pTile->mImage.save(fname, "JPG");
            }
            pTile->mState |= GMapTile::gLoaded;
            //printf("Row = %d, Col = %d\n", pTile->mRow, pTile->mCol);
        } else if (image.loadFromData(ba, "PNG")) {
            pTile->mImage = image;
            if (pTile->mImage.format() != QImage::Format_RGB32 &&
                pTile->mImage.format() != QImage::Format_ARGB32) {
                pTile->mImage = pTile->mImage.convertToFormat(QImage::Format_RGB32);
            }
            if (mIsSaveToCache) {
                pTile->mImage.save(fname, "JPG");
            }
            pTile->mState |= GMapTile::gLoaded;
        } else {
            pTile->mState |= GMapTile::gUnLoad;
        }
        pTile->mState &= ~GMapTile::gLoading;

        mReplyMap.remove(pReply);
        pReply->deleteLater();

        emit httpTileFinished(pTile);
    }
}

/*void GMapTileLoader::timerEvent(QTimerEvent *)
{
    int n = mHttpTiles.count();
    for(int i=mHttpTiles.count()-1; i>=0; i--)
    {
        GMapTile *pTile = mHttpTiles[i];
        if (!(pTile->mState & GMapTile::gLoading))
        {
            mHttpTiles.remove(i);
        }
    }
    if (mHttpTiles.count() < n)
    {
        QWidget *pView = reinterpret_cast<QWidget *>(parent());
        if (pView)
        {
            pView->update();
            printf("Update View\n");
        }
    }
    if (mHttpTiles.count() <= 0)
    {
        killTimer(mHttpTimerId);
        mHttpTimerId = -1;
        printf("Download queue is empty!\n");

        QWidget *pView = reinterpret_cast<QWidget *>(parent());
        if (pView)
        {
            pView->update();
            printf("Update View\n");
        }
    }
}*/


