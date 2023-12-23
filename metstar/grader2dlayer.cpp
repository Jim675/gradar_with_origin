#include "grader2dlayer.h"

#include "generic_basedata_cv.h"
#include "gradialbody.h"
#include "gmapconstant.h"
#include "gradaralgorithm.h"
#include "gradarvolume.h"
#include "gconfig.h"

#include <cmath>
#include <limits>
#include <omp.h>

#include <qvector.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qmessagebox.h>
#include <qpen.h>
#include <qxmlstream.h>
#include <qprogressdialog.h>
#include <qdebug.h>
//#include <cuda_runtime_api.h>

#include <gmapview.h>
//#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkColorTransferFunction.h>
#include <gmapcoordconvert.h>


IMPL_GREFLECTION_CLASS(GRader2DLayer, GMapLayer)

static void printDTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char* msg);
static void smoothImage(const size_t width, const size_t height, double* points, double* smooth, const double invalid);

GRader2DLayer::GRader2DLayer()
{
    mColorTF = NULL;
}

GRader2DLayer::~GRader2DLayer()
{
}

// �����״������б�
void GRader2DLayer::setRaderDataList(const QVector<GRadarVolume*>* raderList)
{
    this->mRaderDataList = raderList;
}

// ��ȡ�״������б�
const QVector<GRadarVolume*>* GRader2DLayer::getRaderDataList() const
{
    return this->mRaderDataList;
}

// ��ȡ��ɫ��
vtkColorTransferFunction* GRader2DLayer::getColorTransferFunction()
{
    return mColorTF;
}

void GRader2DLayer::setColorTransferFunction(vtkColorTransferFunction* pColorTF)
{
	mColorTF = pColorTF;
	if (mColorTF)
	{
		mColorTF->GetRange(mColorRange);
	}
}

// ƽ��ͼ��, ����data[width*height], �����smooth, ��Чֵ��invalidָ��
static void smoothImage(const size_t width, const size_t height, double* points, double* smooth, const double invalid)
{
    for (size_t row = 1; row < height - 1; row++) {

        for (size_t col = 1; col < width - 1; col++) {
            //const double v = imageValue[row * width + col];
            //if (v == invalid) continue;
            size_t n = 0;
            double total = 0.0;
            // ---------------------------------
            double* p = &points[(row - 1) * width];
            if (p[col - 1] != invalid) {
                n++;
                total += p[col - 1];
            }
            if (p[col] != invalid) {
                n++;
                total += p[col];
            }
            if (p[col + 1] != invalid) {
                n++;
                total += p[col + 1];
            }
            // ---------------------------------
            p = &points[row * width];
            if (p[col - 1] != invalid) {
                n++;
                total += p[col - 1];
            }
            if (p[col] != invalid) {
                n++;
                total += p[col];
            }
            if (p[col + 1] != invalid) {
                n++;
                total += p[col + 1];
            }
            // ---------------------------------
            p = &points[(row + 1) * width];
            if (p[col - 1] != invalid) {
                n++;
                total += p[col - 1];
            }
            if (p[col] != invalid) {
                n++;
                total += p[col];
            }
            if (p[col + 1] != invalid) {
                n++;
                total += p[col + 1];
            }
            if (n == 0) {
                smooth[row * width + col] = invalid;
            } else {
                smooth[row * width + col] = total / n;
            }
        }
    }
}

// ��ͼ����
void GRader2DLayer::draw(QPainter* painter, const QRect& rect)
{
    //qDebug() << "draw" << mRadarIndex << ", " << mSurfIndex;
    if (mRaderIndex < 0 || mSurfIndex < 0) {
        // ��ǰû�м��� ���� û��ѡ��Ҫ��ʾ���״�����
        return;
    }
    GRadarVolume* volume = mRaderDataList->at(mRaderIndex);
    if (volume->surfs.size() <= mSurfIndex) {
        qDebug("������ͼ���Ʋ����ڵ�׶��");
        return;
    }
    GRadialSurf* surface = volume->surfs[mSurfIndex];

    if (mInterpolate) {
        drawInterpolation(painter, rect);
    } else {
        drawSector(painter, rect);
    }

    if (surface->bound.isNull()) {
        surface->bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
        qDebug("����ʱû��Webī���б߽�Ϊ��");
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    const GMapView* mapView = getView();
    const QRect boundDp = mapView->lpTodp(surface->bound);

    QPen pen(QColor(0, 0, 180));
    pen.setWidth(2);
    painter->setPen(pen);
    painter->setBrush(Qt::BrushStyle::NoBrush);

    painter->drawEllipse(boundDp);
    painter->setRenderHint(QPainter::Antialiasing, false);
}

// ��Сֵģ�庯��
template<typename T>
int fourMin(T a, T b, T c, T d)
{
    return std::min(std::min(std::min(a, b), c), d);
}

template<typename T>
int fourMax(T a, T b, T c, T d)
{
    return std::max(std::max(std::max(a, b), c), d);
}

// ���Ʋ���ֵ���״�����
void GRader2DLayer::drawSector(QPainter* painter, const QRect& rect)
{
    auto t1 = std::chrono::steady_clock::now();
    QPen pen;
    pen.setStyle(Qt::PenStyle::NoPen);
    painter->setPen(pen);

    const GMapView* mapView = getView();
    const QRectF viewBound = mapView->viewRect();
    //cout << "scale=" << mapView->scale() << endl;

    //const vector<GRadialLine*>& radials = surfAfterProj.radials;
    const GRadarVolume* volume = mRaderDataList->at(mRaderIndex);
    const GRadialSurf* surface = volume->surfs[mSurfIndex];
    if (!surface->isConvert) {
        qDebug() << "�����״�׶�����ݻ�δת��Ϊī��������";
        return;
    }
    const vector<GRadialLine*>& radials = surface->radials;
    const int radialSize = radials.size();
    const int pointSize = radials[0]->points.size();

    // ��������ϵ������������ı��εļ��
    //const int space1 = min(max((int)(0.02 / mapView->scale()), 1), radialSize / 36);
    const int space2 = std::min(std::max((int)(0.03 / mapView->scale()), 1), pointSize / 20);
    //const int space = min(max((int)(0.04 / mapView->scale()), 1), pointSize / 10);
    //qDebug() << "space1=" << space1;
    //cout << "space2=" << space2 << endl;
    //if (space >= pointSize) {
    //    return;
    //}

    QRectF bound;
    QBrush brush(QColor(0, 0, 0, 255)); // ���в���ɾ��
    //QBrush brush;
    painter->setBrush(brush);

    QVector<QPoint> points(5);
    double rgb[3] = { 0 };

    double mx = 0, my = 0;
    GMapCoordConvert::lonLatToMercator(volume->longitude, volume->latitude, &mx, &my);
    const QPoint center = mapView->lpTodp(QPointF(mx, my)); // �״����ĵĵ�ͼ��������

    int dr = 1;
    for (size_t k = 0; k < pointSize; k += space2) {
        int next = k + space2;
        if (next >= pointSize) {
            next = pointSize - 1;
        }
        //const auto& t = radials.at(0)->points[k];
        //const QPoint p = mapView->lpTodp(QPointF(t.x, t.y));
        //double r = std::max(sqrt(pow(p.x() - center.x(), 2) + pow(p.y() - center.y(), 2)), 3.0);
        //constexpr int dp = 100; // ���ζ̱�������ؾ���
        //int tdr = std::max(std::min(int(asin(dp / (2 * r)) * radials.size() / PI + 0.5), 5), 1);
        for (int i = 0; i < radialSize; i++) {
            int nextRadial = i + dr; // TODO ����Ӧ
            if (nextRadial >= radialSize) {
                nextRadial = 0;
            }
            const GRadialLine* radial1 = radials.at(i);
            const GRadialLine* radial2 = radials.at(nextRadial);

            const GRadialPoint& gpt0 = radial1->points[k];
            const GRadialPoint& gpt1 = radial1->points[next];
            const GRadialPoint& gpt2 = radial2->points[next];
            const GRadialPoint& gpt3 = radial2->points[k];

            //if (pData1.value < 0 || pData2.value < 0 || pData3.value < 0 || pData4.value < 0) {
            //    // ���������Чֵ�Ͳ�����
            //}
            // �����ĸ���ƽ��ֵ
            const double value = (gpt0.value + gpt1.value + gpt2.value + gpt3.value) / 4.0;
            if (value < mColorRange[0] || value > mColorRange[1]) {
                // �������Чֵ�Ͳ�����
                continue;
            }

            bound.setLeft(fourMin(gpt0.x, gpt1.x, gpt2.x, gpt3.x));
            bound.setRight(fourMax(gpt0.x, gpt1.x, gpt2.x, gpt3.x));
            bound.setTop(fourMin(gpt0.y, gpt1.y, gpt2.y, gpt3.y));
            bound.setBottom(fourMax(gpt0.y, gpt1.y, gpt2.y, gpt3.y));

            if (!viewBound.intersects(bound)) {
                // ������ڿ������������
                continue;
            }

            const QPoint&& pt0 = mapView->lpTodp(QPointF(gpt0.x, gpt0.y));
            const QPoint&& pt1 = mapView->lpTodp(QPointF(gpt1.x, gpt1.y));
            const QPoint&& pt2 = mapView->lpTodp(QPointF(gpt2.x, gpt2.y));
            const QPoint&& pt3 = mapView->lpTodp(QPointF(gpt3.x, gpt3.y));

            points.clear();

            points.push_back(pt0);
            points.push_back(pt1);
            points.push_back(pt2);
            points.push_back(pt3);

            mColorTF->GetColor(value, rgb);

            //QBrush brush(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
            brush.setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
            painter->setBrush(brush);
            //const_cast<QBrush&>(painter->brush()).setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
            painter->drawPolygon(points);
        }
    }
    auto t2 = std::chrono::steady_clock::now();
    //printDTime(t1, t2, "draw spend time");
}

// ���Ʋ�ֵ���ͼ��
void GRader2DLayer::drawInterpolation(QPainter* painter, const QRect& rect)
{
    const GMapView* mapView = getView();
    const GRadarVolume* volume = mRaderDataList->at(mRaderIndex);
    GRadialSurf* surface = volume->surfs[mSurfIndex];

    if (surface->bound.isNull()) {
        surface->bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
    }
    const QRect radarRect = mapView->lpTodp(surface->bound);
    // �ɼ���Ļ��Χ���״ｻ��
    const QRect drawRect = rect.intersected(radarRect);
    if (drawRect.isNull()) {
        // ���û���ཻ�ͷ���
        return;
    }
    // ī�������귶Χ
    const QRectF bound = mapView->dpTolp(drawRect);
    //painter->setRenderHint(QPainter::RenderHint::Antialiasing, true);
    //painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
    interpolateSurfaceRealTime(volume, surface, bound, drawRect.width(), drawRect.height());
    painter->drawImage(drawRect, surfImageRealTime);
    //painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);
    //painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, false);
}

// �ļ�->��� �ص�����
void GRader2DLayer::clear()
{
    mRaderIndex = -1;
    mSurfIndex = -1;
    getView()->update();
}

// ��ȡ��ǰ��ʾ���״�����
int GRader2DLayer::getRaderDataIndex() const
{
    return mRaderIndex;
}

// ���õ�ǰ��ʾ���״�����
void GRader2DLayer::setRaderDataIndex(int index)
{
    if (index == mRaderIndex) {
        return;
    }

    if (index == -1) {
        mRaderIndex = -1;
        mSurfIndex = -1;
        getView()->update();
        return;
    }

    if (index < 0 || index >= mRaderDataList->size()) {
        qDebug("ѡ����״�������Ч");
        return;
    }

    int oldIndex = mRaderIndex;
    mRaderIndex = index;

    auto* volume = mRaderDataList->at(mRaderIndex);

    if (volume->surfs.size() != 0) 
    {
        if (mSurfIndex >= volume->surfs.size()) mSurfIndex = 0;
        auto* surface = volume->surfs[mSurfIndex];
        if (!surface->isConvert) 
        { 
            // �ж��Ƿ��Ѿ�ת��Ϊ��Webī��������
            GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
        }
        if (oldIndex == -1) 
        {
            // �۽�
            mpView->centerOnM(surface->bound);
        }
    } 
    else 
    {
        mSurfIndex = -1;
    }

    getView()->update();
}

// ��ȡ��ǰ��ʾ��׶������
int GRader2DLayer::getSurfaceIndex() const
{
    return mSurfIndex;
}

// ���õ�ǰ��ʾ��׶������
void GRader2DLayer::setSurfaceIndex(int index)
{
    if (index == mSurfIndex) {
        return;
    }

    if (index == -1) {
        mSurfIndex = -1;
        getView()->update();
    }

    mSurfIndex = index;

    if (mRaderIndex == -1) {
        return;
    }

    auto* volume = mRaderDataList->at(mRaderIndex);

    if (index < 0 || index >= volume->surfs.size()) {
        qDebug("ѡ���׶��������Ч");
        return;
    }

    auto* surface = volume->surfs[mSurfIndex];

    if (!surface->isConvert) {
        GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
    }

    getView()->update();
}

// ��ȡ��ǰ�״�����
const GRadarVolume* GRader2DLayer::getCurRaderData() const
{
    if (mRaderIndex == -1) {
        return nullptr;
    }
    return mRaderDataList->at(mRaderIndex);
}

// ��surface׶���ֵ, �������imageData��surfImage�������ʾ
void GRader2DLayer::interpolateSurface(const GRadarVolume* body, GRadialSurf* surface)
{
    const size_t width = 2048;
    const size_t height = 2048;
    const size_t n = width * height;
    // ʹ��std::unique_ptr�Զ��ͷ��ڴ�
    //imageValue.reset(new double[n] {});
    imageData.reset(new uchar[width * height * 4]{ 0 });
    // TODO ����ֵͼ������, �Զ�ѡ��ʹ��CPU����GPU����
    auto t0 = std::chrono::steady_clock::now();
    QRectF bound = surface->bound;
    if (bound.isNull()) {
        bound = GRadarAlgorithm::calcMercatorBound(surface, body->longitude, body->latitude, body->elev);
    }
    GRadarAlgorithm::interpolateImageOMP(surface, width, height,
        body->longitude, body->latitude, body->elev,
        bound, mColorTF,
        nullptr,
        imageData.get());
    auto t1 = std::chrono::steady_clock::now();
    //cout << width << ", " << height << endl;
    //printDTime(t0, t1, "interpolateImageOMP spend time");

    //auto t1_3 = std::chrono::steady_clock::now();
    //printDTime(t1_2, t1_3, "interpolateImageGPU2 on GPU spend time");
    //interpolateImageGPU(surfIndex, width, height, surfBound, imageValue);
    //auto t1_4 = std::chrono::steady_clock::now();
    //printDTime(t1_3, t1_4, "interpolateImageGPU spend time");
    //cout << "----------------------------------" << endl;

    //interpolateImageGPU2(surfIndex, width, height, surfBound, imageValue);
    //auto t1_5 = std::chrono::steady_clock::now();
    //printDTime(t1_4, t1_5, "interpolateImageGPU2 on GPU spend time");
    //interpolateImageGPU(surfIndex, width, height, surfBound, imageValue);
    //auto t1_6 = std::chrono::steady_clock::now();
    //printDTime(t1_5, t1_6, "interpolateImageGPU spend time");

    // ƽ��ͼ������
    //auto t2 = std::chrono::steady_clock::now();

    //double* smoothValue = new double[n] {};
    //smoothImage(width, height, imageValue.get(), smoothValue, INVALID_VALUE);
    // ʹ��ƽ��ͼ�����ݴ���ԭ��û��ƽ��������
    //imageValue.reset(smoothValue);

    //auto t3 = std::chrono::steady_clock::now();
    //printDTime(t2, t3, "smoothImage spend time:");

    // ����ͼƬRGB����
    //imageData.reset(new uchar[width * height * 4]{0});
    //double rgb[3] = {0};
    //for (size_t row = 0; row < height; row++) {
    //    for (size_t col = 0; col < width; col++) {
    //        const double v = imageValue[row * width + col];
    //        if (v < -10.0 || v > 80.0) continue;
    //        colorTF->GetColor(v, rgb);
    //        uchar* current = &imageData[((height - row - 1) * width + col) * 4];
    //        current[0] = 255 * rgb[0];
    //        current[1] = 255 * rgb[1];
    //        current[2] = 255 * rgb[2];
    //        current[3] = 255;
    //    }
    //}
    // ������ƶ����캯��ת����Դ
    this->surfImage = std::move(QImage(imageData.get(), width, height, QImage::Format::Format_RGBA8888));
    //auto t4 = std::chrono::steady_clock::now();
    //printDTime(t3, t4, "RGB spend time:");
    //printDTime(t0, t4, "interpolateSurface total spend time:");
}

// ��surface׶���ֵ, �������imageValue, imageData��surfImage�������ʾ
void GRader2DLayer::interpolateSurfaceRealTime(const GRadarVolume* body, const GRadialSurf* surface,
    const QRectF& bound, size_t width, size_t height)
{
    const size_t n = width * height;
    // ʹ��std::unique_ptr�Զ��ͷ��ڴ�
    imageDataRealTime.reset(new uchar[width * height * 4]{ 0 });
    // TODO ����ֵͼ������, �Զ�ѡ��ʹ��CPU����GPU����
    auto t0 = std::chrono::steady_clock::now();
    GRadarAlgorithm::interpolateImageOMP(surface, width, height,
        body->longitude, body->latitude, body->elev,
        bound, mColorTF,
        nullptr,
        imageDataRealTime.get());
    auto t1 = std::chrono::steady_clock::now();
    // ������ƶ����캯��ת����Դ
    this->surfImageRealTime = QImage(imageDataRealTime.get(), width, height, QImage::Format::Format_RGBA8888);
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    //qDebug() << width << "*" << height << ":" << t << "ms";
}

/*
* �����Ƿ���ʾ��ֵ����״�ͼ, ���Զ�ˢ��ͼ��
*
* isInterpolated: ���Ϊ true ����ʾ��ֵ���ͼ��, ���Ϊ false ����ʾ����ֵ���״�����
*/
void GRader2DLayer::setInterpolate(bool interpolate)
{
    if (this->mInterpolate == interpolate) {
        return;
    }
    this->mInterpolate = interpolate;

    // �����ǰû���״�����
    if (mRaderIndex == -1 || mSurfIndex == -1) {
        return;
    }

    const GRadarVolume* body = mRaderDataList->at(mRaderIndex);
    GRadialSurf* surface = body->surfs[mSurfIndex];

    if (this->mInterpolate) {
        interpolateSurface(body, surface);
    } else {
        auto t0 = std::chrono::steady_clock::now();
        //qDebug() << "radarIndex=" << mRadarIndex << ", surfIndex=" << mSurfIndex;
        // ׶��תī����ͶӰ
        if (!surface->isConvert) {
            GRadarAlgorithm::surfToMercator(surface,
                body->longitude, body->latitude,
                body->elev);
        }
        auto t1 = std::chrono::steady_clock::now();
        printDTime(t0, t1, "surfToMercator spend time:");
    }
    getView()->update();
}

//void GRader2DLayer::setAnimate(bool isAnimate)
//{
//
//}

// ��ӡ start �� end �ĺ�ʱ������, msg�൱�ڱ�ע
static void printDTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char* msg)
{
    auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    qDebug() << msg << ": " << dtime.count() << "ms";
}

// �״���ֵ����
    //double* value = new double[width * height]{0};
    // �������
    //bool* mask = new bool[width * height * 4]{0};

    //QImage image(width, height, QImage::Format::Format_RGBA8888);
    //double rgb[3] = {0};
    /*const double dx = (maxX - minX) / width;
    const double dy = (maxY - minY) / height;
    for (const GQRadialData* radial : surf->radials) {
        //for (int i = 0; i < surf->radials.size(); i++) {
        //const QVector<GQPointData>& points = surf->radials[i]->data;
        const QVector<GQPointData>& points = radial->data;
        for (const GQPointData& point : points) {
            if (point.value < 10.0 || point.value > 80.0) {
                continue;
            }
            size_t row = (maxY - point.point.y()) / dy;
            size_t col = (point.point.x() - minX) / dx;
            if (row >= height) {
                row = height - 1;
            }
            size_t index = row * width + col;
            value[index] = point.value;
            mask[index] = true;
        }
    }*/
    /*
    constexpr size_t threshold = 5;
    // �����ɵ��״�ͼƬ���в�ֵ
    for (size_t row = 0; row < height; row++) {
        size_t lastCol = -1;
        for (size_t col = 0; col < width; col++) {
            if (mask[row * width + col]) {
                lastCol = col;
                continue;
            }
            size_t nextCol = -1;
            size_t lastRow = -1;
            size_t nextRow = -1;

            size_t dLastCol = col - lastCol;
            size_t dNextCol = 0;
            size_t dLastRow = 0;
            size_t dNextRow = 0;

            if (col < width - 1) {
                for (size_t tcol = col + 1; dNextCol <= threshold && tcol < width; tcol++) {
                    dNextCol++;
                    if (mask[row * width + col]) nextCol = tcol;
                }
            }
            if (row > 0) {
                for (size_t trow = row - 1; dLastRow <= threshold; trow--) {
                    dLastRow++;
                    if (mask[trow * width + col]) lastRow = trow;
                    if (trow == 0) break;
                }
            }
            if (row < height - 1) {
                for (size_t trow = row + 1; dNextRow <= threshold && trow < height; trow++) {
                    dNextRow++;
                    if (mask[row * width + col]) nextRow = trow;
                }
            }
            size_t n = 0;// �ҵ�������Чֵ
            size_t td = 0;// �ܾ���
            if (lastCol != -1)  n++, td += dLastCol;
            if (nextCol != -1)  n++, td += dNextCol;
            if (lastRow != -1)  n++, td += dLastRow;
            if (nextRow != -1)  n++, td += dNextCol;
            if (n > 1) {
                double v = 0.0;
                if (lastCol != -1)  v += (value[row * width + lastCol] * dLastCol / td);
                if (nextCol != -1)  v += (value[row * width + nextCol] * dNextCol / td);
                if (lastRow != -1)  v += (value[lastRow * width + lastCol] * dLastRow / td);
                if (nextRow != -1)  v += (value[nextRow * width + lastCol] * dNextCol / td);
                value[row * width + col] = v;
            }
        }
    }
    */

    //delete[] value;
    //delete[] mask;
    // 
    //size_t max = 0;
    //size_t min = -1;
    //for (GRadialData* data : surface->radials) {
    //    size_t size = data->data.size();
    //    if (size > max) {
    //        max = size;
    //    } else if (size < min) {
    //        min = size;
    //    }
    //}

    //qDebug() << "max=" << max << ", min=" << min << Qt::endl;