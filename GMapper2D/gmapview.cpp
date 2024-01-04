#include <QPainter>
#include <QWheelEvent>
#include <QImage>
#include <QFont>
#include <QFontInfo>
#include <QToolTip>
#include <QDate>
#include <QMessageBox>
#include <qpainter.h>
#include <qdebug.h>
#include <utility>
#include <iostream>
#include <cstdio>

#include "gmapcoordconvert.h"
#include "gmapview.h"
#include "gmaptile.h"
#include "gmath.h"
#include "gmaplayerset.h"
#include "gmaplayer.h"
#include "ggpscorrect.h"
#include "gutils.h"

using std::cout;
using std::endl;

#define MAX_MECTOR		20037508.3427892
#define MIN_MECTOR		-20037508.3427892
#define MECTOR_RANGE	40075016.6855784

// 数字转为度分秒
void gDecimal2DMS(double dec, int* degree, int* minute, double* second)
{
    *degree = (int)dec;
    dec -= *degree;
    dec *= 60;
    *minute = (int)dec;
    dec -= *minute;
    *second = dec * 60;
}

// 度分秒转为数字
void gDMS2Decimal(int degree, int minute, double second, double& dec)
{
    dec = degree + minute / 60.0 + second / 3600;
}

GMapView::GMapView(QWidget* parent): QWidget(parent)
{
    strcpy(mDownloadFormat, GOOGLE_SR_FMT);

    mIsMapEnabled = true;

    mpMapLayerSet = new GMapLayerSet();
    mpMapLayerSet->attachView(this);
    mpActiveLayer = nullptr;

    mpTileLoader = new GMapTileLoader(this);
    // 设置地图瓦片路径
    mpTileLoader->setMapTilesPath("./tiles");
    mTileSize = QSize(256, 256);
    // 设置高程瓦片路径
    //mpTileLoader->setElevTilesPath("D:/elevation_tiles");
    mpTileLoader->setElevTilesPath("./elevation_tiles");
    mElevTileSize = 256;

    mpTileLoader->setCreateSubPath(true);
    mMaxTileCaheSize = 128;

    mBoundingRect.setLeft(MIN_MECTOR);
    mBoundingRect.setRight(MAX_MECTOR);
    mBoundingRect.setTop(MIN_MECTOR);
    mBoundingRect.setBottom(MAX_MECTOR);

    mMinScaleGrade = 4;
    mMaxScaleGrade = 16;
    mScaleGrade = 4;
    mScale = (double)mTileSize.width() * (1LL << mScaleGrade) / MECTOR_RANGE;
    mIsLeftDrag = mIsRightDrag = false;
    mIsDragRect = false;
    mIsDrawZoomRect = false;

    mCustomMargins.setLeft(0);	mCustomMargins.setRight(0);
    mCustomMargins.setTop(0);	mCustomMargins.setBottom(0);
    setMouseTracking(true);

    QString str;
    str.sprintf("%02d", mScaleGrade);
    mInfoLabel = nullptr;
    mScaleGradeStr = tr("Scale Grade: ") + str;
    //str.sprintf("%4.8f, %4.8f", 0, 0);
    //mLonLatStr = tr("Longitude and Latitude: ") + str;
    //mLonLatStr = tr("Lon, Lat: 0, 0");
    mLonLatStr = "Lon, Lat: 0, 0";

    mSelectMode = SELECT_NONE;

    connect(mpTileLoader, SIGNAL(httpTileFinished(GMapTile*)), this, SLOT(onHttpTileFinished(GMapTile*)));
    //if (QDate::currentDate() > QDate(2021, 11, 1))
    //{
    //    QMessageBox::warning(this, "GMapper2D", tr("Trial time is already in place. Please contact dengfei@cdut.cn"));
    //}
}

GMapView::~GMapView()
{
    delete mpMapLayerSet;
    delete mpTileLoader;
}

// 获取可视区域逻辑坐标
QRectF GMapView::viewRect() const
{
    return mViewRect;
}

bool GMapView::isMapEnabled() const
{
    return mIsMapEnabled;
}

void GMapView::setMapEnabled(bool isEnabled)
{
    mIsMapEnabled = isEnabled;
    update();
}

void GMapView::clearCache()
{
    qDeleteAll(mTileCache);
    mTileCache.clear();
}

GMapTile* GMapView::findCacheTile(int grade, int row, int col)
{
    //for (int i = 0; i < mTileCache.count(); i++) {
    for (int i = mTileCache.count() - 1; i >= 0; i--) {
        GMapTile* pTile = mTileCache[i];
        if (pTile->mGrade == grade && pTile->mRow == row && pTile->mCol == col) {
            // 将找到的瓦片移动到尾部
            mTileCache.remove(i);
            mTileCache.append(pTile);
            return pTile;
        }
    }
    return nullptr;
}

void GMapView::addCacheTile(GMapTile* pTile)
{
    if (mTileCache.count() >= mMaxTileCaheSize) {
        for (int i = 0; i < mTileCache.count(); i++) {
            GMapTile* pTile = mTileCache[i];
            if (!(pTile->mState & GMapTile::gVisible) && !(pTile->mState & GMapTile::gLoading)) {
                mTileCache.remove(i);
                delete pTile;
                break;
            }
        }
    }
    mTileCache.append(pTile);
}

GMapLayerSet* GMapView::layerSet()
{
    return mpMapLayerSet;
}

void GMapView::setActiveLayer(GMapLayer* pLayer)
{
    mpActiveLayer = pLayer;
}

void GMapView::setScaleGrade(int s)
{
    zoom(s);
    refresh();
}

int GMapView::scaleGrade() const
{
    return mScaleGrade;
}

double GMapView::scale() const
{
    return mScale;
}

void GMapView::setInfoLabel(QLabel* label)
{
    mInfoLabel = label;
    if (mInfoLabel) {
        mInfoLabel->setText(mLonLatStr + " | " + mScaleGradeStr);
    }
}

// 以指定矩形区域为视图中央(经纬度坐标)
void GMapView::centerOn(const QRectF& rect)
{
    // 将经纬度坐标转换为墨卡托坐标
    centerOnM(GMapCoordConvert::lonLatToMercator(rect));
}

// 以指定矩形区域为视图中央(墨卡托坐标)
void GMapView::centerOnM(const QRectF& rect)
{
    double scale = 1.0;
    int w = width() - mMargins.left() - mMargins.right();
    int h = height() - mMargins.top() - mMargins.bottom();
    // 寻找一个最小缩放等级, 使得当前窗口可以完整显示lrc
    int scaleGrade = mMaxScaleGrade;
    for (; scaleGrade >= mMinScaleGrade; scaleGrade--) {
        scale = (double)mTileSize.width() * (1LL << scaleGrade) / MECTOR_RANGE;
        int dw = qRound(rect.width() * scale * 1.0);
        int dh = qRound(rect.height() * scale * 1.0);
        if (dw <= w && dh <= h) break;
    }
    if (scaleGrade != mScaleGrade) zoom(scaleGrade);

    // 设置可视矩形
    double lw = w / scale;
    double lh = h / scale;
    QPointF c = rect.center();
    mViewRect.setRect(c.x() - lw * 0.5, c.y() - lh * 0.5, lw, lh);
    if (mViewRect.left() < mBoundingRect.left()) {
        mViewRect.setLeft(mBoundingRect.left());
        mViewRect.setRight(mViewRect.left() + lw);
    }
    if (mViewRect.top() < mBoundingRect.top()) {
        mViewRect.setTop(mBoundingRect.top());
        mViewRect.setBottom(mBoundingRect.top() + lh);
    }
    if (mViewRect.right() > mBoundingRect.right()) {
        mViewRect.setRight(mBoundingRect.right());
        mViewRect.setLeft(mBoundingRect.right() - lw);
    }
    if (mViewRect.bottom() > mBoundingRect.bottom()) {
        mViewRect.setBottom(mBoundingRect.bottom());
        mViewRect.setTop(mBoundingRect.bottom() - lh);
    }

    // 刷新地图
    mIsUpdateMapTile = true;
    update();
}

// 转为度分秒
QString GMapView::formatDMS(double decimal)
{
    int degree = 0;
    int minute = 0;
    double second = 0;
    gDecimal2DMS(decimal, &degree, &minute, &second);
    //static const char32_t symbol[] = {0x00B0,0x2032,2033};
    //return QString::number(degree).append(QChar(0x00B0)).append(' ') // °
    //    .append(QString::number(minute)).append(QChar(0x2032)).append(' ') // ′
    //    .append(QString::number(second, 'f', 2)).append(QChar(0x2033)); // ″
    // 帅鹏飞 2022.1.1 使用0填充固定宽度
    return QString::number(degree).append(QChar(0x00B0)).append(' ') // °
        .append(QString::asprintf("%02d", minute)).append(QChar(0x2032)).append(' ') // ′
        .append(QString::asprintf("%05.2f", second)).append(QChar(0x2033)); // ″
}

int GMapView::lpXTodpX(qreal lx) const
{
    return qRound((lx - mViewRect.left()) * mScale) + mMargins.left();
}

int GMapView::lpYTodpY(qreal ly) const
{
    return qRound((mViewRect.bottom() - ly) * mScale) + mMargins.top();
}

qreal GMapView::dpXTolpX(int dx) const
{
    return mViewRect.left() + (dx - mMargins.left()) / mScale;
}

qreal GMapView::dpYTolpY(int dy) const
{
    return mViewRect.bottom() - (dy - mMargins.top()) / mScale;
}
///
int	GMapView::lpXTodpXc(qreal lx) const
{
   // qreal vrleft = 1.17192e+07;
    //qreal mgleft = 0;
    
    //return qRound((lx - mViewRect.left()) * mScale) + mMargins.left();
   // return qRound((lx - vrleft) * 6) + mgleft;
    return qRound((lx - mViewLeftConst) * mScaleConst) + mMarginsLeftConst;
}
int	GMapView::lpYTodpYc(qreal ly) const
{
   // qreal vrbottom = 4.55382e+06;
   // qreal mgtop = 0;
    return qRound((mViewBottomConst - ly) * mScaleConst) + mMarginsTopConst;
    //return qRound((vrbottom - ly) * 6) + mgtop;
}
qreal GMapView::dpXTolpXc(int dx) const
{
   // qreal vrleft = 1.17192e+07;
    //qreal mgleft = 0;
    return mViewLeftConst+ (dx - mMarginsLeftConst) / mScaleConst;
    //return vrleft + (dx - mgleft) / 6;
}

qreal GMapView::dpYTolpYc(int dy) const
{
   // qreal vrbottom = 4.55382e+06;
   // qreal mgtop = 0;
    //return vrbottom - (dy - mgtop) / 6;
    return mViewBottomConst - (dy - mMarginsTopConst) / mScaleConst;
}
const QRectF& GMapView::boundingRect() const
{
    return mBoundingRect;
}

void GMapView::setBoundingRect(const QRectF& rect)
{
    mBoundingRect = rect;
    //mViewRect = mBoundingRect;
}

void GMapView::onHttpTileFinished(GMapTile* pTile)
{
    if (pTile->mState & GMapTile::gLoaded) {
        update();
    } else if (!(pTile->mState & GMapTile::gVisible)) {
        int i = mTileCache.indexOf(pTile);
        if (i >= 0) {
            mTileCache.remove(i);
            delete pTile;
        }
    }
}

QVector<GMapTile*> GMapView::getSelectedTiles(QRect& iBound, int align)
{
    int nx = 1 << mScaleGrade;
    int ny = 1 << mScaleGrade;

    double ldx = mTileSize.width() / mScale;
    double ldy = mTileSize.height() / mScale;

    int ix0, ix1, iy0, iy1, rx, ry;
    QRect dpRect = lpTodp(mSelectedRect);
    ix0 = (int)((mSelectedRect.left() - MIN_MECTOR) / ldx);
    if (ix0 < 0) ix0 = 0;
    ix1 = (int)((mSelectedRect.right() - MIN_MECTOR) / ldx);
    rx = (ix1 - ix0 + 1) % align;
    ix1 += rx;
    if (ix1 >= nx) ix1 -= align;

    iy0 = (int)((mSelectedRect.top() - MIN_MECTOR) / ldy);
    if (iy0 < 0) iy0 = 0;
    iy1 = (int)((mSelectedRect.bottom() - MIN_MECTOR) / ldy);
    ry = (iy1 - iy0 + 1) % align;
    iy1 += ry;
    if (iy1 >= ny) iy1 -= align;
    QVector<GMapTile*> selectedTitles;

    iBound.setRect(ix0, ny - iy1 - 1, ix1 - ix0 + 1, iy1 - iy0 + 1);

    for (int i = iy0; i <= iy1; i++) {
        for (int j = ix0; j <= ix1; j++) {
            GMapTile* pTile = new GMapTile(mScaleGrade, ny - i - 1, j);
            selectedTitles.append(pTile);
        }
    }
    return selectedTitles;
}

void GMapView::updateMapTiles()
{
    // 地图XY轴各有多少个地图瓦片
    const int nx = 1 << mScaleGrade;
    const int ny = 1 << mScaleGrade;
    // 每个地图瓦片的宽和高对应的逻辑（墨卡托投影）长度
    double ldx = mTileSize.width() / mScale;
    double ldy = mTileSize.height() / mScale;
    if (mIsUpdateMapTile) {
        // 当前屏幕可见区域
        //QRect dpRect = lpTodp(mViewRect);
        // 可见区域四个角地图瓦片索引
        int ix0 = static_cast<int>((mViewRect.left() - MIN_MECTOR) / ldx);
        if (ix0 < 0) ix0 = 0;

        int ix1 = static_cast<int>((mViewRect.right() - MIN_MECTOR) / ldx);
        if (ix1 >= nx) ix1 = nx - 1;

        int iy0 = static_cast<int>((mViewRect.top() - MIN_MECTOR) / ldy);
        if (iy0 < 0) iy0 = 0;

        int iy1 = static_cast<int>((mViewRect.bottom() - MIN_MECTOR) / ldy);
        if (iy1 >= ny) iy1 = ny - 1;

        for (int i = 0; i < mTileCache.count(); i++) {
            GMapTile* pTile = mTileCache[i];
            // 清除缓存瓦片可见状态
            pTile->mState &= ~GMapTile::gVisible;
        }

        mTiles.clear();
        // 缓存没有需要加载或下载的瓦片
        QVector<GMapTile*> needLoadTitles;

        for (int i = iy0; i <= iy1; i++) {
            for (int j = ix0; j <= ix1; j++) {
                GMapTile* pTile = findCacheTile(mScaleGrade, ny - i - 1, j);
                if (!pTile) {
                    pTile = new GMapTile(mScaleGrade, ny - i - 1, j);
                    needLoadTitles.append(pTile);
                }
                pTile->mState |= GMapTile::gVisible;
                mTiles.append(pTile);
            }
        }

        if (!needLoadTitles.empty()) {
            // 加载不在Cache中的瓦片
            mpTileLoader->load(needLoadTitles, mDownloadFormat);
            for (auto item : needLoadTitles) {
                if (item->mState & GMapTile::gLoaded) {
                    // 把已经加载的瓦片放入缓存
                    addCacheTile(item);
                }
            }
        }
        mIsUpdateMapTile = false;
    }
}

void GMapView::drawStatusInfo(QPainter* p)
{
    int statusWidth, statusHeight;

    QString str = mLonLatStr + " | " + mScaleGradeStr;
    QFont font("Consolas", 11);
    p->setFont(font);
    QFontMetrics fm = p->fontMetrics();

    statusWidth = 330;//8 + fm.width(str);
    statusHeight = fm.height() + 8;
    int x0 = width() - mCustomMargins.right() - statusWidth;
    int y0 = height() - mCustomMargins.bottom() - statusHeight;

    p->fillRect(QRect(x0, y0, statusWidth, statusHeight), QColor(255, 255, 255, 128));
    p->setPen(Qt::black);
    p->drawText(x0 + 4, y0, statusWidth, statusHeight, Qt::AlignLeft | Qt::AlignVCenter, str);
}

void GMapView::paintEvent(QPaintEvent* event)
{
    if (!isEnabled()) {
        QWidget::paintEvent(event);
        return;
    }

    QRect rc = event->rect();
    QPainter p(this);

    p.setClipRect(mCustomMargins.left(), mCustomMargins.top(),
                  width() - mCustomMargins.left() - mCustomMargins.right(),
                  height() - mCustomMargins.top() - mCustomMargins.bottom());

    if (isMapEnabled()) {
        if (mIsUpdateMapTile) updateMapTiles();
        // 绘制可见瓦片
        // 地图XY轴上的地图瓦片数量
        //int nx = 1 << (mScaleGrade - 1);
        //int ny = 1 << (mScaleGrade - 1);
        //int nx = 1 << mScaleGrade;
        //int ny = 1 << mScaleGrade;
        int dx0 = lpXTodpX(MIN_MECTOR);
        int dy0 = lpYTodpY(MAX_MECTOR);

        for (int i = 0; i < mTiles.count(); i++) {
            GMapTile* pTile = mTiles[i];
            //if (mTileSize.isEmpty()) {
            //    // 没有图像就跳过
            //    continue;
            //}
            int rx = dx0 + pTile->mCol * mTileSize.width();
            int ry = dy0 + pTile->mRow * mTileSize.height();
            p.drawImage(rx, ry, pTile->mImage, 0, 0, mTileSize.width(), mTileSize.height());
        }
    }

    // 绘制图层
    mpMapLayerSet->draw(&p, rc);

    if (mSelectMode == SELECT_RECT) // 绘制选择矩形框
    {
        if (mSelectedRect.width() != 0 && mSelectedRect.height() != 0) {
            p.setPen(QPen(Qt::red, 2));
            p.setBrush(Qt::NoBrush);
            QRect sr = lpTodp(mSelectedRect);
            p.drawRect(sr);
        }
    }

    // 	p.setBrush(Qt::red);
    // 	p.setPen(Qt::black);
    // 	p.drawEllipse(width()/2-3, height()/2-3, 7, 7);

    p.setClipping(false);
}

void GMapView::refresh()
{
    // 重新计算边距
    double lw = mBoundingRect.width();
    double lh = mBoundingRect.height();
    mMargins = mCustomMargins;

    double w = width() - mMargins.left() - mMargins.right();
    double w1 = lw * mScale;
    if (w > w1) {
        mMargins.setLeft((width() - w1) / 2);
        mMargins.setRight(mMargins.left());
    }

    double h = height() - mMargins.top() - mMargins.bottom();
    double h1 = lh * mScale;
    if (h > h1) {
        mMargins.setTop((height() - h1) / 2);
        mMargins.setBottom(mMargins.top());
    }

    // 重新计算可视区域逻辑范围
    double xm = mViewRect.center().x();
    double ym = mViewRect.center().y();
    double rx = 0.5 * (width() - mMargins.left() - mMargins.right()) / mScale;
    double ry = 0.5 * (height() - mMargins.top() - mMargins.bottom()) / mScale;
    mViewRect.setRect(xm - rx, ym - ry, 2 * rx, 2 * ry);
    if (mViewRect.left() < mBoundingRect.left()) {
        mViewRect.setLeft(mBoundingRect.left());
        mViewRect.setRight(mBoundingRect.left() + 2 * rx);
    }
    if (mViewRect.right() > mBoundingRect.right()) {
        mViewRect.setRight(mBoundingRect.right());
        mViewRect.setLeft(mBoundingRect.right() - 2 * rx);
    }
    if (mViewRect.top() < mBoundingRect.top()) {
        mViewRect.setTop(mBoundingRect.top());
        mViewRect.setBottom(mBoundingRect.top() + 2 * ry);
    }
    if (mViewRect.bottom() > mBoundingRect.bottom()) {
        mViewRect.setBottom(mBoundingRect.bottom());
        mViewRect.setTop(mBoundingRect.bottom() - 2 * ry);
    }

    mIsUpdateMapTile = true;
    update();
}

void GMapView::resizeEvent(QResizeEvent* e)
{
    if (!isEnabled()) return;

    refresh();
}

void GMapView::zoom(int scaleGrade, int mouseX, int mouseY)
{
    if (mouseX < 0) {
        mouseX = (width() - mMargins.left() - mMargins.right()) / 2;
        mouseY = (height() - mMargins.top() - mMargins.bottom()) / 2;
    }
    if (scaleGrade < mMinScaleGrade) scaleGrade = mMinScaleGrade;
    if (scaleGrade > mMaxScaleGrade) scaleGrade = mMaxScaleGrade;
    if (mScaleGrade == scaleGrade) return;

    //if (mScaleGrade == 24) {
    //    mScaleGrade = mScaleGrade;
    //}

    // 如果输入的是鼠标当前的位置, 而不是默认的窗口中心坐标, 那么应该能以鼠标点为中心缩放, def 2018/10/29
    double xm = dpXTolpX(mouseX); // mViewRect.center().x();
    double ym = dpYTolpY(mouseY); // mViewRect.center().y();
    double rl = xm - mViewRect.left();	// 记录鼠标逻辑坐标到mViewRect的左上角的距离
    double rt = ym - mViewRect.top();
    double s;
    if (scaleGrade < mScaleGrade) {
        s = (1LL << (mScaleGrade - scaleGrade));  //左移一位 乘以 2^1
    } else {
        s = 1.0 / (1LL << (scaleGrade - mScaleGrade));
    }
    mScaleGrade = scaleGrade;

    //double x0 = xm-mViewRect.width()*s*0.5;
    //double y0 = ym-mViewRect.height()*s*0.5;
    double x0 = xm - rl * s;
    double y0 = ym - rt * s;
    double w = mViewRect.width() * s;
    if (w > mBoundingRect.width()) {
        w = mBoundingRect.width();
    }
    double h = mViewRect.height() * s;
    if (h > mBoundingRect.height()) {
        h = mBoundingRect.height();
    }
    double x1 = x0 + w;
    double y1 = y0 + h;
    if (x0 < mBoundingRect.left()) {
        x0 = mBoundingRect.left();
        x1 = x0 + w;
    }
    if (x1 > mBoundingRect.right()) {
        x1 = mBoundingRect.right();
        x0 = x1 - w;
    }
    if (y0 < mBoundingRect.top()) {
        y0 = mBoundingRect.top();
        y1 = y0 + h;
    }
    if (y1 > mBoundingRect.bottom()) {
        y1 = mBoundingRect.bottom();
        y0 = y1 - h;
    }

    //mScale = mGraphSize.width() / w;
    //mScale = (double)mTileSize.width() * (1 << (mScaleGrade - 1)) / mBoundingRect.width();
    mScale = (double)mTileSize.width() * (1LL << mScaleGrade) / MECTOR_RANGE;
    mViewRect.setRect(x0, y0, x1 - x0, y1 - y0);

    if (mInfoLabel) {
        mScaleGradeStr = QString::asprintf("Scale Grade: %02d", mScaleGrade);
        mInfoLabel->setText(mLonLatStr + " | " + mScaleGradeStr);
    }
}

void GMapView::wheelEvent(QWheelEvent* e)
{
    int scaleGrade = mScaleGrade;
    if (e->delta() > 0) {
        if (scaleGrade + 1 > mMaxScaleGrade) {
            return;
        }
        scaleGrade++;
    } else {
        if (scaleGrade - 1 < mMinScaleGrade) {
            return;
        }
        scaleGrade--;
    }
    zoom(scaleGrade, e->x(), e->y());
    refresh();
    //update();
}

// 鼠标按下
void GMapView::mousePressEvent(QMouseEvent* e)
{
    int x = e->x();
    int y = e->y();

    if (e->button() == Qt::RightButton) {
        mIsRightDrag = true;
        mPreX = x;	mPreY = y;
    }

    if (mSelectMode == SELECT_NONE) {
        if (mpActiveLayer) {
            mpActiveLayer->mousePressEvent(e);
        }
    } else {
        if (mSelectMode == SELECT_RECT) {
            if (e->button() != Qt::LeftButton) return;

            mIsDragRect = true;
            mSelectedRect.setRect(dpXTolpX(x), dpYTolpY(y), 0, 0);
            mSelectedRectDp.setRect(x, y, 0, 0);
        }
    }
}

void GMapView::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->x();
    int y = e->y();

    if (mInfoLabel) {
        QPointF lpt = dpTolp(QPoint(x, y));
        QPointF lonLat = GMapCoordConvert::mercatorToLonLat(lpt);

        GGPSCorrect::gcj2Towgs(lonLat.x(), lonLat.y(), lonLat.rx(), lonLat.ry());
        mLonLatStr = QString("Lon, Lat: ").append(formatDMS(lonLat.x())).append(", ").append(formatDMS(lonLat.y()));
        //mLonLatStr = tr("Longitude and Latitude: ") + str;
        //QString str;
        //str.sprintf("%.1f, %.1f", lpt.x(), lpt.y());
        mInfoLabel->setText(mLonLatStr + " | " + mScaleGradeStr);// +" | " + str);
    }

    if (mIsRightDrag) {
        float lx0 = dpXTolpX(mPreX);
        float ly0 = dpYTolpY(mPreY);
        float lx1 = dpXTolpX(x);
        float ly1 = dpYTolpY(y);
        float left = mViewRect.left() + lx0 - lx1;
        if (left < mBoundingRect.left()) {
            left = mBoundingRect.left();
        } else if (left + mViewRect.width() > mBoundingRect.right()) {
            left = mBoundingRect.right() - mViewRect.width();
        }

        float top = mViewRect.top() + ly0 - ly1;
        if (top < mBoundingRect.top()) {
            top = mBoundingRect.top();
        } else if (top + mViewRect.height() > mBoundingRect.bottom()) {
            top = mBoundingRect.bottom() - mViewRect.height();
        }
        mViewRect.moveTo(left, top);
        mPreX = x;
        mPreY = y;
        mIsUpdateMapTile = true;
        update();
    }

    if (mSelectMode == SELECT_NONE) {
        if (mpActiveLayer) {
            mpActiveLayer->mouseMoveEvent(e);
        }
    } else {
        if (mSelectMode == SELECT_RECT) {
            if (!mIsDragRect) return;
            mSelectedRect.setRight(dpXTolpX(x + 1));
            mSelectedRect.setBottom(dpYTolpY(y + 1));
            update();
        }
    }
}

void GMapView::mouseReleaseEvent(QMouseEvent* e)
{
    if (mIsRightDrag && e->button() == Qt::RightButton) {
        mIsRightDrag = false;
    }

    if (mSelectMode == SELECT_NONE) {
        if (mpActiveLayer) {
            mpActiveLayer->mouseReleaseEvent(e);
        }
    } 
    else 
    {
        int x = e->x();
        int y = e->y();

        if (mSelectMode == SELECT_RECT) 
        {
            if (mIsDragRect) // 如果在拖动鼠标选择矩形
            {
                mIsDragRect = false;
                // mSelectedRect是当前视口显示的逻辑坐标范围
                mSelectedRect.setRight(dpXTolpX(x + 1));
                mSelectedRect.setBottom(dpYTolpY(y + 1));

                // mSelectedRectDp包含了(x, y)这个点
                mSelectedRectDp.setRight(x);
                // mSelectedRectDp是选择矩形框屏幕坐标范围
                mSelectedRectDp.setBottom(y);

                if (!mSelectedRect.isValid()) 
                {   
                    // 交换左上角与右下角
                    mSelectedRect = mSelectedRect.normalized();
                    mSelectedRectDp = mSelectedRectDp.normalized();
                }
                //QRect rect = lpTodp(mSelectedRect);
                update();
                // 如果移动距离>=2个像素才触发回调
                if (mSelectedRectDp.width() >= 2 && mSelectedRectDp.height() >= 2) 
                {
                    // 保存所选区域的地图
                    //double metersPerPixel = saveSelectRectImage(mSelectedRectDp);
                    // 发射回调函数
                    //emit onSelectedRect(mSelectedRectDp, &mSelectedImage, metersPerPixel);
                    emit onSelectedRect(mSelectedRectDp);
                }
            }
        }
    }
}

// 保存所选区域的图像 rect为屏幕像素坐标
QImage GMapView::saveSelectRectImage(const QRect& rect)
{
    // 当前屏幕可见区域逻辑坐标
    QRectF selectedRect = dpTolp(rect);

    cout << "保存地图区域屏幕坐标" << endl;
    cout << "x, y=" << rect.x() << ", " << rect.y()
        << " w, h=" << rect.width() << ", " << rect.height() << endl;

    //cout << "保存地图区域墨卡托坐标" << endl;
    //cout << "x y=" << selectedRect.x() << ", " << selectedRectDp.y()
    //    << " w, h=" << selectedRect.width() << ", " << selectedRectDp.height() << endl;

    const int rectWidth = rect.width();
    const int rectHeight = rect.height();

    // 找到选中矩形区域的长边
    int longSide = rectWidth;

    if (rectWidth < rectHeight) {
        longSide = rectHeight;
    }

    // 找到所需要的长边长度与缩放等级, 最小4级, 最大到11级, 使得生成的图像边长为[512, 1024)
    int needLongSide = longSide;
    int needScaleGrade = std::min(std::max(mScaleGrade, 4), 11);
    while (needLongSide < 1024 && needScaleGrade < 11) {
        needLongSide = needLongSide << 1;
        needScaleGrade++;
    }
    while (needLongSide >= 2048 && needScaleGrade > 4) {
        needLongSide = needLongSide >> 1;
        needScaleGrade--;
    }
    // 地图XY轴各有多少个地图瓦片
    const int xTiles = 1 << needScaleGrade;
    const int yTiles = 1 << needScaleGrade;

    // 所需缩放系数
    const double needScale = static_cast<double>(mTileSize.width()) * xTiles / MECTOR_RANGE;
    //const double metersPerPixel = static_cast<double>(MECTOR_RANGE) / (mTileSize.width() * xTiles);
    cout << "needScaleGrade:" << needScaleGrade << " needScale:" << needScale << endl;
    //--------------找到需要的瓦片-------------

    // 每个地图瓦片的宽和高对应的逻辑（墨卡托投影）长度
    const double ldx = MECTOR_RANGE / xTiles;
    const double ldy = ldx * mTileSize.height() / mTileSize.width();

    // 选择区域四个角地图瓦片索引
    int ix0 = static_cast<int>((selectedRect.left() - MIN_MECTOR) / ldx);
    if (ix0 < 0) ix0 = 0;
    int ix1 = static_cast<int>((selectedRect.right() - MIN_MECTOR) / ldx);
    if (ix1 >= xTiles) ix1 = xTiles - 1;

    int iy0 = static_cast<int>((selectedRect.top() - MIN_MECTOR) / ldy);
    if (iy0 < 0) iy0 = 0;
    int iy1 = static_cast<int>((selectedRect.bottom() - MIN_MECTOR) / ldy);
    if (iy1 >= yTiles) iy1 = yTiles - 1;

    // 选择的区域包含的瓦片
    QVector<GMapTile*> needTiles;
    QVector<GMapTile*> needLoadTiles;
    for (int i = iy0; i <= iy1; i++) {
        for (int j = ix0; j <= ix1; j++) {
            GMapTile* pTile = findCacheTile(needScaleGrade, yTiles - i - 1, j);
            if (pTile == nullptr) { // 没有在缓存中找到选择区域的瓦片
                pTile = new GMapTile(needScaleGrade, yTiles - i - 1, j);
                needLoadTiles.push_back(pTile);
            }
            needTiles.append(pTile);
        }
    }
    // 加载没有找到的瓦片
    mpTileLoader->load(needLoadTiles);

    // 生成图片大小
    int imgWidth = 0;
    int imgHeight = 0;

    // 地图缩放等级之差
    int dscale = needScaleGrade - mScaleGrade;
    if (dscale >= 0) {
        imgWidth = rectWidth << dscale;
        imgHeight = rectHeight << dscale;
    } else {
        imgWidth = rectWidth >> (-dscale);
        imgHeight = rectHeight >> (-dscale);
    }

    cout << "imgWidth:" << imgWidth << " imgHeight:" << imgHeight << endl;

    QImage image = QImage(imgWidth, imgHeight, QImage::Format::Format_RGB888);
    QPainter painter(&image);
    //painter.setClipRect(0, 0, imgWidth, imgHeight);

    // 在缩放等级为needScaleGrade时, 整个地图左上角相对于选择区域在屏幕像素上的坐标
    const int dx0 = qRound((MIN_MECTOR - selectedRect.left()) * needScale) + mMargins.left();
    const int dy0 = qRound((selectedRect.bottom() - MAX_MECTOR) * needScale) + mMargins.left();

    for (int i = 0; i < needTiles.count(); i++) {
        const GMapTile* pTile = needTiles[i];
        // 当前瓦片在所生成的图片中的位置
        int left = dx0 + pTile->mCol * mTileSize.width();
        int top = dy0 + pTile->mRow * mTileSize.height();
        //cout << "left:" << left << " top:" << top << endl;
        //QImage image = pTile->mImage.scaled(mTileSize.width(), mTileSize.height(),
        //                                    Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::FastTransformation);
        painter.drawImage(left, top, pTile->mImage, 0, 0, mTileSize.width(), mTileSize.height());
    }
    // 释放内存
    qDeleteAll(needLoadTiles);
    return image;
}

// 获取当前高程图像
QImage GMapView::getElevationImage(const QRect& rect)
{
    // 当前屏幕可见区域逻辑坐标
    QRectF selectedRect = dpTolp(rect);

    cout << "获取当前高程图像" << endl;
    cout << "x, y=" << rect.x() << ", " << rect.y()
        << " w, h=" << rect.width() << ", " << rect.height() << endl;

    const int rectWidth = rect.width();
    const int rectHeight = rect.height();

    // 找到选中矩形区域的长边
    int longSide = rectWidth;

    if (rectWidth < rectHeight) {
        longSide = rectHeight;
    }

    // 找到所需要的长边长度与缩放等级, 最小5级, 最大到10级, 使得生成的图像边长为[512, 1024)
    int needLongSide = longSide;
    int needScaleGrade = std::min(std::max(mScaleGrade, 5), 10);
    //int needScaleGrade = 11;

    while (needLongSide < 512 && needScaleGrade < 10) {
        needLongSide = needLongSide << 1;
        needScaleGrade++;
    }

    while (needLongSide >= 1024 && needScaleGrade > 5) {
        needLongSide = needLongSide >> 1;
        needScaleGrade--;
    }

    // 地图XY轴各有多少个地图瓦片
    const int xTiles = 1 << needScaleGrade;
    const int yTiles = 1 << needScaleGrade;

    // 所需缩放系数
    const double needScale = (double)mElevTileSize * xTiles / MECTOR_RANGE;

    // 每个地图瓦片的宽和高对应的逻辑（墨卡托投影）长度
    const double ldx = MECTOR_RANGE / xTiles;
    //const double ldy = ldx * mTileSize.height() / mTileSize.width();

    // 选择区域四个角地图瓦片索引
    int ix0 = static_cast<int>((selectedRect.left() - MIN_MECTOR) / ldx);
    if (ix0 < 0) ix0 = 0;
    int ix1 = static_cast<int>((selectedRect.right() - MIN_MECTOR) / ldx);
    if (ix1 >= xTiles) ix1 = xTiles - 1;

    int iy0 = static_cast<int>((selectedRect.top() - MIN_MECTOR) / ldx);
    if (iy0 < 0) iy0 = 0;
    int iy1 = static_cast<int>((selectedRect.bottom() - MIN_MECTOR) / ldx);
    if (iy1 >= yTiles) iy1 = yTiles - 1;

    // 生成图片大小
    int imgWidth = 0;
    int imgHeight = 0;

    // 地图缩放等级之差
    const int dscale = needScaleGrade - mScaleGrade;
    if (dscale >= 0) {
        imgWidth = rectWidth << dscale;
        imgHeight = rectHeight << dscale;
    } else {
        imgWidth = rectWidth >> -dscale;
        imgHeight = rectHeight >> -dscale;
    }

    // 选择的区域包含的瓦片
    QVector<GMapTile*> needTiles;
    for (int i = iy1; i >= iy0; i--) {
        for (int j = ix0; j <= ix1; j++) {
            const int row = yTiles - i - 1;
            GMapTile* pTile = new GMapTile(needScaleGrade, row, j);
            // 加载瓦片
            if (mpTileLoader->loadElevTileFromLocal(pTile)) {
                needTiles.append(pTile);
            }
            else delete pTile;
        }
    }

    QImage elevImage(imgWidth, imgHeight, QImage::Format::Format_Grayscale16);
    QPainter painter(&elevImage);
    elevImage.fill(uint(0));
    //painter.setClipRect(0, 0, imgWidth, imgHeight);

    // 在缩放等级为needScaleGrade时, 整个地图左上角相对于选择区域在屏幕像素上的坐标
    const int dx0 = qRound((MIN_MECTOR - selectedRect.left()) * needScale) + mMargins.left();
    const int dy0 = qRound((selectedRect.bottom() - MAX_MECTOR) * needScale) + mMargins.left();

    for (int i = 0; i < needTiles.count(); i++) {
        const GMapTile* pTile = needTiles[i];
        // 当前瓦片在所生成的图片中的位置
        int left = dx0 + pTile->mCol * mTileSize.width();
        int top = dy0 + pTile->mRow * mTileSize.height();
        painter.drawImage(left, top, pTile->mImage, 0, 0, mElevTileSize, mElevTileSize);
    }
    // 释放内存
    qDeleteAll(needTiles);

    return elevImage;
}

void GMapView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (mSelectMode == SELECT_NONE) 
    {
        if (mpActiveLayer) 
        {
            mpActiveLayer->mouseDoubleClickEvent(e);
        }
    }
}

void GMapView::keyPressEvent(QKeyEvent* e)
{
    if (mSelectMode == SELECT_NONE) 
    {
        if (mpActiveLayer) 
        {
            mpActiveLayer->keyPressEvent(e);
        }
    }
}

void GMapView::keyReleaseEvent(QKeyEvent* e)
{
    if (mSelectMode == SELECT_NONE) 
    {
        if (mpActiveLayer) 
        {
            mpActiveLayer->keyReleaseEvent(e);
        }
    }
}

bool GMapView::event(QEvent* e)
{
    if (e->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
        if (mpActiveLayer) {
            double lx = dpXTolpX(helpEvent->x());
            double ly = dpYTolpY(helpEvent->y());
            QString s = mpActiveLayer->maybeTip(lx, ly);
            QToolTip::showText(helpEvent->globalPos(), s);
        }
    }

    return QWidget::event(e);
}

void GMapView::setSelectMode(GMapSelectMode mode)
{
    if (mSelectMode == mode) return;

    if (mode == SELECT_RECT) {
        mSelectedRect.setRect(0, 0, 0, 0);
        mSelectedRectDp.setRect(0, 0, 0, 0);
    }

    mSelectMode = mode;
    update();
}

const QRectF& GMapView::getSelectedRect() const
{
    return mSelectedRect;
}

void GMapView::setSelectedRect(const QRectF& rc)
{
    mSelectedRect = rc;
    mSelectedRectDp = lpTodp(rc);
}

void GMapView::setDownloadFormat(const char* fmt)
{
    strcpy(mDownloadFormat, fmt);
    mIsUpdateMapTile = true;
    mpTileLoader->cancelLoad();
    clearCache();

    update();
}
