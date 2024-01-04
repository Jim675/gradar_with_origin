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

// ����תΪ�ȷ���
void gDecimal2DMS(double dec, int* degree, int* minute, double* second)
{
    *degree = (int)dec;
    dec -= *degree;
    dec *= 60;
    *minute = (int)dec;
    dec -= *minute;
    *second = dec * 60;
}

// �ȷ���תΪ����
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
    // ���õ�ͼ��Ƭ·��
    mpTileLoader->setMapTilesPath("./tiles");
    mTileSize = QSize(256, 256);
    // ���ø߳���Ƭ·��
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

// ��ȡ���������߼�����
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
            // ���ҵ�����Ƭ�ƶ���β��
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

// ��ָ����������Ϊ��ͼ����(��γ������)
void GMapView::centerOn(const QRectF& rect)
{
    // ����γ������ת��Ϊī��������
    centerOnM(GMapCoordConvert::lonLatToMercator(rect));
}

// ��ָ����������Ϊ��ͼ����(ī��������)
void GMapView::centerOnM(const QRectF& rect)
{
    double scale = 1.0;
    int w = width() - mMargins.left() - mMargins.right();
    int h = height() - mMargins.top() - mMargins.bottom();
    // Ѱ��һ����С���ŵȼ�, ʹ�õ�ǰ���ڿ���������ʾlrc
    int scaleGrade = mMaxScaleGrade;
    for (; scaleGrade >= mMinScaleGrade; scaleGrade--) {
        scale = (double)mTileSize.width() * (1LL << scaleGrade) / MECTOR_RANGE;
        int dw = qRound(rect.width() * scale * 1.0);
        int dh = qRound(rect.height() * scale * 1.0);
        if (dw <= w && dh <= h) break;
    }
    if (scaleGrade != mScaleGrade) zoom(scaleGrade);

    // ���ÿ��Ӿ���
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

    // ˢ�µ�ͼ
    mIsUpdateMapTile = true;
    update();
}

// תΪ�ȷ���
QString GMapView::formatDMS(double decimal)
{
    int degree = 0;
    int minute = 0;
    double second = 0;
    gDecimal2DMS(decimal, &degree, &minute, &second);
    //static const char32_t symbol[] = {0x00B0,0x2032,2033};
    //return QString::number(degree).append(QChar(0x00B0)).append(' ') // ��
    //    .append(QString::number(minute)).append(QChar(0x2032)).append(' ') // ��
    //    .append(QString::number(second, 'f', 2)).append(QChar(0x2033)); // ��
    // ˧���� 2022.1.1 ʹ��0���̶����
    return QString::number(degree).append(QChar(0x00B0)).append(' ') // ��
        .append(QString::asprintf("%02d", minute)).append(QChar(0x2032)).append(' ') // ��
        .append(QString::asprintf("%05.2f", second)).append(QChar(0x2033)); // ��
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
    // ��ͼXY����ж��ٸ���ͼ��Ƭ
    const int nx = 1 << mScaleGrade;
    const int ny = 1 << mScaleGrade;
    // ÿ����ͼ��Ƭ�Ŀ�͸߶�Ӧ���߼���ī����ͶӰ������
    double ldx = mTileSize.width() / mScale;
    double ldy = mTileSize.height() / mScale;
    if (mIsUpdateMapTile) {
        // ��ǰ��Ļ�ɼ�����
        //QRect dpRect = lpTodp(mViewRect);
        // �ɼ������ĸ��ǵ�ͼ��Ƭ����
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
            // ���������Ƭ�ɼ�״̬
            pTile->mState &= ~GMapTile::gVisible;
        }

        mTiles.clear();
        // ����û����Ҫ���ػ����ص���Ƭ
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
            // ���ز���Cache�е���Ƭ
            mpTileLoader->load(needLoadTitles, mDownloadFormat);
            for (auto item : needLoadTitles) {
                if (item->mState & GMapTile::gLoaded) {
                    // ���Ѿ����ص���Ƭ���뻺��
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
        // ���ƿɼ���Ƭ
        // ��ͼXY���ϵĵ�ͼ��Ƭ����
        //int nx = 1 << (mScaleGrade - 1);
        //int ny = 1 << (mScaleGrade - 1);
        //int nx = 1 << mScaleGrade;
        //int ny = 1 << mScaleGrade;
        int dx0 = lpXTodpX(MIN_MECTOR);
        int dy0 = lpYTodpY(MAX_MECTOR);

        for (int i = 0; i < mTiles.count(); i++) {
            GMapTile* pTile = mTiles[i];
            //if (mTileSize.isEmpty()) {
            //    // û��ͼ�������
            //    continue;
            //}
            int rx = dx0 + pTile->mCol * mTileSize.width();
            int ry = dy0 + pTile->mRow * mTileSize.height();
            p.drawImage(rx, ry, pTile->mImage, 0, 0, mTileSize.width(), mTileSize.height());
        }
    }

    // ����ͼ��
    mpMapLayerSet->draw(&p, rc);

    if (mSelectMode == SELECT_RECT) // ����ѡ����ο�
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
    // ���¼���߾�
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

    // ���¼�����������߼���Χ
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

    // ������������굱ǰ��λ��, ������Ĭ�ϵĴ�����������, ��ôӦ����������Ϊ��������, def 2018/10/29
    double xm = dpXTolpX(mouseX); // mViewRect.center().x();
    double ym = dpYTolpY(mouseY); // mViewRect.center().y();
    double rl = xm - mViewRect.left();	// ��¼����߼����굽mViewRect�����Ͻǵľ���
    double rt = ym - mViewRect.top();
    double s;
    if (scaleGrade < mScaleGrade) {
        s = (1LL << (mScaleGrade - scaleGrade));  //����һλ ���� 2^1
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

// ��갴��
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
            if (mIsDragRect) // ������϶����ѡ�����
            {
                mIsDragRect = false;
                // mSelectedRect�ǵ�ǰ�ӿ���ʾ���߼����귶Χ
                mSelectedRect.setRight(dpXTolpX(x + 1));
                mSelectedRect.setBottom(dpYTolpY(y + 1));

                // mSelectedRectDp������(x, y)�����
                mSelectedRectDp.setRight(x);
                // mSelectedRectDp��ѡ����ο���Ļ���귶Χ
                mSelectedRectDp.setBottom(y);

                if (!mSelectedRect.isValid()) 
                {   
                    // �������Ͻ������½�
                    mSelectedRect = mSelectedRect.normalized();
                    mSelectedRectDp = mSelectedRectDp.normalized();
                }
                //QRect rect = lpTodp(mSelectedRect);
                update();
                // ����ƶ�����>=2�����زŴ����ص�
                if (mSelectedRectDp.width() >= 2 && mSelectedRectDp.height() >= 2) 
                {
                    // ������ѡ����ĵ�ͼ
                    //double metersPerPixel = saveSelectRectImage(mSelectedRectDp);
                    // ����ص�����
                    //emit onSelectedRect(mSelectedRectDp, &mSelectedImage, metersPerPixel);
                    emit onSelectedRect(mSelectedRectDp);
                }
            }
        }
    }
}

// ������ѡ�����ͼ�� rectΪ��Ļ��������
QImage GMapView::saveSelectRectImage(const QRect& rect)
{
    // ��ǰ��Ļ�ɼ������߼�����
    QRectF selectedRect = dpTolp(rect);

    cout << "�����ͼ������Ļ����" << endl;
    cout << "x, y=" << rect.x() << ", " << rect.y()
        << " w, h=" << rect.width() << ", " << rect.height() << endl;

    //cout << "�����ͼ����ī��������" << endl;
    //cout << "x y=" << selectedRect.x() << ", " << selectedRectDp.y()
    //    << " w, h=" << selectedRect.width() << ", " << selectedRectDp.height() << endl;

    const int rectWidth = rect.width();
    const int rectHeight = rect.height();

    // �ҵ�ѡ�о�������ĳ���
    int longSide = rectWidth;

    if (rectWidth < rectHeight) {
        longSide = rectHeight;
    }

    // �ҵ�����Ҫ�ĳ��߳��������ŵȼ�, ��С4��, ���11��, ʹ�����ɵ�ͼ��߳�Ϊ[512, 1024)
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
    // ��ͼXY����ж��ٸ���ͼ��Ƭ
    const int xTiles = 1 << needScaleGrade;
    const int yTiles = 1 << needScaleGrade;

    // ��������ϵ��
    const double needScale = static_cast<double>(mTileSize.width()) * xTiles / MECTOR_RANGE;
    //const double metersPerPixel = static_cast<double>(MECTOR_RANGE) / (mTileSize.width() * xTiles);
    cout << "needScaleGrade:" << needScaleGrade << " needScale:" << needScale << endl;
    //--------------�ҵ���Ҫ����Ƭ-------------

    // ÿ����ͼ��Ƭ�Ŀ�͸߶�Ӧ���߼���ī����ͶӰ������
    const double ldx = MECTOR_RANGE / xTiles;
    const double ldy = ldx * mTileSize.height() / mTileSize.width();

    // ѡ�������ĸ��ǵ�ͼ��Ƭ����
    int ix0 = static_cast<int>((selectedRect.left() - MIN_MECTOR) / ldx);
    if (ix0 < 0) ix0 = 0;
    int ix1 = static_cast<int>((selectedRect.right() - MIN_MECTOR) / ldx);
    if (ix1 >= xTiles) ix1 = xTiles - 1;

    int iy0 = static_cast<int>((selectedRect.top() - MIN_MECTOR) / ldy);
    if (iy0 < 0) iy0 = 0;
    int iy1 = static_cast<int>((selectedRect.bottom() - MIN_MECTOR) / ldy);
    if (iy1 >= yTiles) iy1 = yTiles - 1;

    // ѡ��������������Ƭ
    QVector<GMapTile*> needTiles;
    QVector<GMapTile*> needLoadTiles;
    for (int i = iy0; i <= iy1; i++) {
        for (int j = ix0; j <= ix1; j++) {
            GMapTile* pTile = findCacheTile(needScaleGrade, yTiles - i - 1, j);
            if (pTile == nullptr) { // û���ڻ������ҵ�ѡ���������Ƭ
                pTile = new GMapTile(needScaleGrade, yTiles - i - 1, j);
                needLoadTiles.push_back(pTile);
            }
            needTiles.append(pTile);
        }
    }
    // ����û���ҵ�����Ƭ
    mpTileLoader->load(needLoadTiles);

    // ����ͼƬ��С
    int imgWidth = 0;
    int imgHeight = 0;

    // ��ͼ���ŵȼ�֮��
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

    // �����ŵȼ�ΪneedScaleGradeʱ, ������ͼ���Ͻ������ѡ����������Ļ�����ϵ�����
    const int dx0 = qRound((MIN_MECTOR - selectedRect.left()) * needScale) + mMargins.left();
    const int dy0 = qRound((selectedRect.bottom() - MAX_MECTOR) * needScale) + mMargins.left();

    for (int i = 0; i < needTiles.count(); i++) {
        const GMapTile* pTile = needTiles[i];
        // ��ǰ��Ƭ�������ɵ�ͼƬ�е�λ��
        int left = dx0 + pTile->mCol * mTileSize.width();
        int top = dy0 + pTile->mRow * mTileSize.height();
        //cout << "left:" << left << " top:" << top << endl;
        //QImage image = pTile->mImage.scaled(mTileSize.width(), mTileSize.height(),
        //                                    Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::FastTransformation);
        painter.drawImage(left, top, pTile->mImage, 0, 0, mTileSize.width(), mTileSize.height());
    }
    // �ͷ��ڴ�
    qDeleteAll(needLoadTiles);
    return image;
}

// ��ȡ��ǰ�߳�ͼ��
QImage GMapView::getElevationImage(const QRect& rect)
{
    // ��ǰ��Ļ�ɼ������߼�����
    QRectF selectedRect = dpTolp(rect);

    cout << "��ȡ��ǰ�߳�ͼ��" << endl;
    cout << "x, y=" << rect.x() << ", " << rect.y()
        << " w, h=" << rect.width() << ", " << rect.height() << endl;

    const int rectWidth = rect.width();
    const int rectHeight = rect.height();

    // �ҵ�ѡ�о�������ĳ���
    int longSide = rectWidth;

    if (rectWidth < rectHeight) {
        longSide = rectHeight;
    }

    // �ҵ�����Ҫ�ĳ��߳��������ŵȼ�, ��С5��, ���10��, ʹ�����ɵ�ͼ��߳�Ϊ[512, 1024)
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

    // ��ͼXY����ж��ٸ���ͼ��Ƭ
    const int xTiles = 1 << needScaleGrade;
    const int yTiles = 1 << needScaleGrade;

    // ��������ϵ��
    const double needScale = (double)mElevTileSize * xTiles / MECTOR_RANGE;

    // ÿ����ͼ��Ƭ�Ŀ�͸߶�Ӧ���߼���ī����ͶӰ������
    const double ldx = MECTOR_RANGE / xTiles;
    //const double ldy = ldx * mTileSize.height() / mTileSize.width();

    // ѡ�������ĸ��ǵ�ͼ��Ƭ����
    int ix0 = static_cast<int>((selectedRect.left() - MIN_MECTOR) / ldx);
    if (ix0 < 0) ix0 = 0;
    int ix1 = static_cast<int>((selectedRect.right() - MIN_MECTOR) / ldx);
    if (ix1 >= xTiles) ix1 = xTiles - 1;

    int iy0 = static_cast<int>((selectedRect.top() - MIN_MECTOR) / ldx);
    if (iy0 < 0) iy0 = 0;
    int iy1 = static_cast<int>((selectedRect.bottom() - MIN_MECTOR) / ldx);
    if (iy1 >= yTiles) iy1 = yTiles - 1;

    // ����ͼƬ��С
    int imgWidth = 0;
    int imgHeight = 0;

    // ��ͼ���ŵȼ�֮��
    const int dscale = needScaleGrade - mScaleGrade;
    if (dscale >= 0) {
        imgWidth = rectWidth << dscale;
        imgHeight = rectHeight << dscale;
    } else {
        imgWidth = rectWidth >> -dscale;
        imgHeight = rectHeight >> -dscale;
    }

    // ѡ��������������Ƭ
    QVector<GMapTile*> needTiles;
    for (int i = iy1; i >= iy0; i--) {
        for (int j = ix0; j <= ix1; j++) {
            const int row = yTiles - i - 1;
            GMapTile* pTile = new GMapTile(needScaleGrade, row, j);
            // ������Ƭ
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

    // �����ŵȼ�ΪneedScaleGradeʱ, ������ͼ���Ͻ������ѡ����������Ļ�����ϵ�����
    const int dx0 = qRound((MIN_MECTOR - selectedRect.left()) * needScale) + mMargins.left();
    const int dy0 = qRound((selectedRect.bottom() - MAX_MECTOR) * needScale) + mMargins.left();

    for (int i = 0; i < needTiles.count(); i++) {
        const GMapTile* pTile = needTiles[i];
        // ��ǰ��Ƭ�������ɵ�ͼƬ�е�λ��
        int left = dx0 + pTile->mCol * mTileSize.width();
        int top = dy0 + pTile->mRow * mTileSize.height();
        painter.drawImage(left, top, pTile->mImage, 0, 0, mElevTileSize, mElevTileSize);
    }
    // �ͷ��ڴ�
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
