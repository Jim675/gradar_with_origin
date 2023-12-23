#include "grader2dlayer.h"

#include "generic_basedata_cv.h"
#include "gradialbody.h"
#include "gmapconstant.h"
#include "gradaralgorithm.h"
#include "gradarvolume.h"
#include "gconfig.h"

#include <cmath>
#include <limits>

#include <qvector.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qmessagebox.h>
#include <qpen.h>
#include <qxmlstream.h>
#include <qprogressdialog.h>
#include <qdatetime.h>
#include <qdebug.h>
//#include <cuda_runtime_api.h>
#include <qcolor.h>
#include <gmapview.h>
//#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkColorTransferFunction.h>
#include <gmapcoordconvert.h>
#include <qdir.h>
#include <qfiledialog.h>
#include <chrono>

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

    /*
    if (mPredict)
    {
        //drawPredictSector(painter, rect);
        qDebug() << "��ʼ����ֵͼ:" << endl;
        drawPredictInterpolation(painter, rect);
        qDebug() << "��������ֵͼ:" << endl;
    }
    else
    {
        if (mInterpolate) {
            drawInterpolation(painter, rect);
        } else {
            drawSector(painter, rect);
        }
    }*/
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

//����Ԥ��󲻲�ֵ���״�����
void GRader2DLayer::drawPredictSector(QPainter* painter, const QRect& rect)
{
    QPen pen;
    pen.setStyle(Qt::PenStyle::NoPen);
    painter->setPen(pen);

    GMapView* mapView = getView();
    const QRectF viewBound = mapView->viewRect();
    QImage image = mPreImages[mSurfIndex];
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
    int imagewidth = image.width();
    int imageheight = image.height();



    QVector<QPointF> pointfs;
    vector<double> values;
    QBrush brush(QColor(0, 0, 0, 255)); // ���в���ɾ��
    painter->setBrush(brush);
    double rgb[3];
    int psize = mMXYS[mSurfIndex].size();
    for (int i = 0; i < psize; i++)
    {
        QPoint p = mapView->lpTodp(mMXYS[mSurfIndex].at(i));
        if (!viewBound.contains(p))
        {
            //double value = values[i];
            double value = mValues[mSurfIndex].at(i);
            //if (value == 0)
            if (value == 0 || value == 255 ||(value-35.0)<-5)
                continue;
            mColorTF->GetColor(value - 35.0, rgb);
            //qDebug() << "value - 35.0" << value - 35.0 << endl;
            //QPen pen;
            //painter->setPen(pen);
            brush.setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
            painter->setBrush(brush);
            //painter->setBrush(Qt::black);
            //painter->drawPoint(p);
            painter->drawEllipse(p, 1, 1);
        }
    }
    
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
    const GRadarVolume* volume = nullptr;
    if (mPredict)
    {
        volume = mPredictVolume;
    }
    else
    {
        volume = mRaderDataList->at(mRaderIndex);
    }
    //const GRadarVolume* volume = mRaderDataList->at(mRaderIndex);
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

int GRader2DLayer::getPredict()
{
    if (mPredict)
        return 1;
    else
        return -1;
}
//���֮ǰ������
void GRader2DLayer::clearConten()
{
    mlons.clear();
    mlats.clear();
    mGXS.clear();
    mGYS.clear();
    mGZS.clear();
    mvalues.clear();
    mgvalues.clear();
    moneELnums.clear();
}
void GRader2DLayer::justCenter()
{
    auto* volume = mRaderDataList->at(0);

    if (volume->surfs.size() != 0)
    {
        if (mSurfIndex >= volume->surfs.size()) mSurfIndex = 0;
        auto* surface = volume->surfs[mSurfIndex];
        if (!surface->isConvert)
        {
            // �ж��Ƿ��Ѿ�ת��Ϊ��Webī��������
            GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
        }
          // �۽�
         mpView->centerOnM(surface->bound);
    }
    getView()->update();
}
// ���Ʋ�ֵ���ͼ��
void GRader2DLayer::drawInterpolation(QPainter* painter, const QRect& rect)
{
    const GMapView* mapView = getView();
    ///2023-8-30���
   //GRadarVolume* volume = nullptr;
    GRadarVolume* volume = new GRadarVolume();

    if (mPredict)
    {
        volume = mPredictVolume;
        //qDebug() << "Ԥ��" << endl;
    }
    else
    {
        volume = mRaderDataList->at(mRaderIndex);
    }
    //const GRadarVolume* volume = mRaderDataList->at(mRaderIndex);
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
        if (mSurfIndex >= volume->surfs.size() || mSurfIndex < 0) mSurfIndex = 0;
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
//��Ļת�߼�����
QPointF GRader2DLayer::cvdptolp(QPoint p)
{
    const GMapView* view = getView();
    const int x = p.x();
    const int y = p.y();
    const QPointF& pf = view->dpTolpc(QPoint(x, y));
    return pf;
}
//�߼�ת��Ļ����
QPoint GRader2DLayer::cvlptodp(QPointF pf)
{
    const GMapView* view = getView();
    const QPoint& p = view->lpTodpc(QPointF(pf.x(), pf.y()));
    return p;
}
//�����״�վ����Ļ����
void GRader2DLayer::setLonpLatp(QPoint p)
{
    Lonp = p.x();
    Latp = p.y();
}
//����״�վ����Ļ����
QPoint GRader2DLayer::getLonpLatp()
{
    return QPoint(Lonp, Latp);
}
//���ú��θ߶�
void GRader2DLayer::setElev(double elev)
{
    mElev = elev;
}
//��ú��θ߶�
double GRader2DLayer::getElev()
{
    return mElev;
}
//�����������Ǹ���������ֵ
void GRader2DLayer::setELs(int surfsnum, vector<double> v)
{
    int size = v.size();
    if (surfsnum != size)
        return;
    for (int i = 0; i < size; i++)
    {
        mELs.push_back(v[i]);
    }
}
//����������Ǹ���������ֵ
vector<double> GRader2DLayer::getELs()
{
    return mELs;
}

//���Ʋ�����ÿһ�����ǵ�ͼƬ
int GRader2DLayer::drawElpng(QString root)
{
    qDebug() << "rootPath" << root << endl;
    const GMapView* mapView = getView();
    int volumesize = mRaderDataList->size();
   
    for (int k = 0; k < volumesize; k++)
    {
        const GRadarVolume* volume = mRaderDataList->at(k);
        int surfsize = volume->surfs.size();
        vector<double> v;
        
        for (int i = 0; i < surfsize; i++)
        {
            v.push_back(volume->surfs[i]->el);
            //qDebug() << "v:" << volume->surfs[i]->el << endl;
        }
        if (cheekisSave == -1)
        {
            double lon = volume->longitude;
            double lat = volume->latitude;
            double mx = 0, my = 0;
            GMapCoordConvert::lonLatToMercator(lon, lat, &mx, &my);
            const QPoint pt = mapView->lpTodp(QPointF(mx, my));
            //qDebug() <<"��ʱ���������Ļ��" << pt.x() << "--" << pt.y() << endl;
            setLonpLatp(pt);
            setElev(volume->elev);
            setELs(surfsize, v);
            cheekisSave = 1;
        }
        for (int i = 0; i < surfsize; i++)
        {
            auto* surface = volume->surfs[i];

            if (!surface->isConvert) {

                //qDebug() << "�����״�׶�����ݻ�δת��Ϊī��������";
                // �ж��Ƿ��Ѿ�ת��Ϊ��Webī��������
                GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
                //return;
            }
            //qDebug() << "����" << endl;
            const vector<GRadialLine*>& radials = surface->radials;
            const int radialSize = radials.size();
            const int pointSize = radials[0]->points.size();
          
            double rgb[3] = { 0 };

            QPainter painter;
            QImage im(920, 920, QImage::Format_RGB32);
            QPen pen;
            painter.begin(&im);
            for (int i = 0; i < radialSize; i++)
            {
                const GRadialLine* radial1 = radials.at(i);
                for (size_t k = 0; k < pointSize; k++)
                {
                    const GRadialPoint& gpt = radial1->points[k];
                    const double value = gpt.value;
                    if (value < mColorRange[0] || value > mColorRange[1]) {
                        // �������Чֵ�Ͳ�����
                        continue;
                    }
                    const QPoint&& pt = mapView->lpTodp(QPointF(gpt.x, gpt.y));
                    //qDebug() << pt.x() << "--" << pt.y() << endl;
                    mColorTF->GetColor(value, rgb);
                    int value1 = value + 35;
                    pen.setColor(QColor::fromRgb(value1, value1, value1));
                    painter.setPen(pen);
                    painter.drawPoint(pt);

                }
            }
            painter.end();

           
            //QString path = "F:\\radar-dataimage\\photo\\";
            QString path = root;
            QString volumepath = volume->path;

            int indexp = volumepath.lastIndexOf('/');
            QString filename = volumepath.mid(indexp + 1, volumepath.size() - indexp - 1);
            //�����ļ���
            path += "/";
            path += filename;
            QString dir_str = path;
            QDir dir;
            if (!dir.exists(dir_str))
            {
                bool res = dir.mkpath(dir_str);
            }
            path += "/";
            QString index;
            index.setNum(i + 1, 10);
            path.append(index);
            path += ".png";
            im.save(path, "PNG");        
        }
    }
    return 1;
}
//�����Ƿ�������Ԥ��״̬
void GRader2DLayer::setPredict(bool mIsPredict)
{
    if (this->mPredict == mIsPredict) {
        return;
    }
    this->mPredict = mIsPredict;
    getView()->update();
}
//��������Ļ����ת��Ϊ�߼�����
void GRader2DLayer::convertdptolp()
{
    int size = mPreImages.size();
    int width = mPreImages[0].width();
    int height = mPreImages[0].height();
    qDebug() << "width:::::" << width << endl;
    qDebug() << "height:::::" << height << endl;
    const GMapView* mapView = getView();
    for (int i = 0; i < size; i++)
    {
        QImage image = mPreImages[i];
        qDebug() <<"cc"<< image;
        qDebug() << "size:" << image.size();
        QVector<float> values;
        QVector<QPointF> pointfs;
        qDebug() << "i:" << i << endl;
        for (int k = 0; k < width; k++)
        {
            for (int j = 0; j < height; j++)
            {
               /*
                QColor color = image.pixel(k,j);
                //qDebug() << "j+1:" << j+1 << endl;
                float value = color.red();
                */
                int value = qRed(image.pixel(k, j));
                values.push_back(value);
                //QPointF pf = cvdptolp(QPoint(i, j));
                QPointF pf = mapView->dpTolp(QPoint(k, j));
                pointfs.push_back(pf);
            }
        }
        mMXYS.push_back(pointfs);
        mValues.push_back(values);
    }
}
//����Ԥ��ͼƬ
void GRader2DLayer::setPreImage(QVector<QImage>&image)
{
    mPreImages = image;
    //qDebug() << "mPreImages.size()" << mPreImages.size() << endl;
}
//��ֵ��
int GRader2DLayer::savePreInfoIn(QVector<QVector<QImage>>& Images)
{
    int volumesize = mRaderDataList->size();
    int imagewidth = imgwidth;
    int imageheight = imgwidth;



    for (int k = 0; k < volumesize; k++)
    {
        const GRadarVolume* volume = mRaderDataList->at(k);
        int surfsize = volume->surfs.size();
        vector<double> v;

        for (int i = 0; i < surfsize; i++)
        {
            v.push_back(volume->surfs[i]->el);
        }
        if (cheekisSave == -1)
        {
            mSLon = volume->longitude;
            mSLat = volume->latitude;
            setElev(volume->elev);
            setELs(surfsize, v);
            cheekisSave = 1;
        }
        QVector<QImage> image;
        for (int i = 0; i < surfsize; i++)
        {
            auto* surface = volume->surfs[i];
            QRectF bound = surface->bound;
            if (bound.isNull()) {
                bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
            }
            QImage im(imagewidth, imageheight, QImage::Format_Grayscale8);
            im.fill(0);
            GRadarAlgorithm::interpolateImageGrayOMP(surface, imagewidth, imagewidth,
                volume->longitude, volume->latitude, volume->elev,
                bound, mColorTF,
                nullptr,
                im.bits());
            image.push_back(im);
        }
        Images.push_back(image);
    }
    return 0;
}
int GRader2DLayer::savePreInfo(QVector<QVector<QImage>>& Images)
{
    const GRadarVolume* tvolume = mRaderDataList->at(0);
    auto* surf = mRaderDataList->at(0)->surfs[0];
    QRectF rectf = surf->bound;
    if (rectf.isEmpty())
    {
        rectf = GRadarAlgorithm::calcMercatorBound(surf, tvolume->longitude, tvolume->latitude, tvolume->elev);
    }
    double minMlon = rectf.left();
    double minMlat = rectf.top();
    double maxMlon = rectf.right();
    double maxMlat = rectf.bottom();

    double minLon, minLat;
    double maxLon, maxLat;
    //double lon = 0, lat = 0;
    GMapCoordConvert::mercatorToLonLat(minMlon, minMlat, &minLon, &minLat);
    GMapCoordConvert::mercatorToLonLat(maxMlon, maxMlat, &maxLon, &maxLat);

    const GMapView* mapView = getView();
    int volumesize = mRaderDataList->size();
    //920/1840
    int imagewidth = imgwidth;
    int imageheight = imgwidth;

    int num = 0;
    mScaleLon = imagewidth / (maxLon - minLon);
    mScaleLat = imageheight / (maxLat - minLat);

    mCenterX = imagewidth / 2;
    mCenterY = imageheight / 2;

    double slon = tvolume->longitude;
    double slat = tvolume->latitude;

    for (int k = 0; k < volumesize; k++)
    {

        const GRadarVolume* volume = mRaderDataList->at(k);
        int surfsize = volume->surfs.size();
        vector<double> v;

        for (int i = 0; i < surfsize; i++)
        {
            v.push_back(volume->surfs[i]->el);
            qDebug() << "v:" << volume->surfs[i]->el << endl;
        }
        if (cheekisSave == -1)
        {
            mSLon = volume->longitude;
            mSLat = volume->latitude;
            //setLonpLatp(QPoint(mCenterX,mCenterY));
            setElev(volume->elev);
            setELs(surfsize, v);
            cheekisSave = 1;
        }
        QVector<QImage> image;
        for (int i = 0; i < surfsize; i++)
        {
            auto* surface = volume->surfs[i];
            if (!surface->isConvert) {
                GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
            }

            if (surface->isConvert)
            {
                const vector<GRadialLine*>& radials = surface->radials;
                const int radialSize = radials.size();
                const int pointSize = radials[0]->points.size();
                QPainter painter;
                QImage im(imgwidth, imgwidth, QImage::Format_Grayscale8);
                im.fill(0);

                uchar* p = im.bits();
                for (int i = 0; i < radialSize; i++)
                {
                    const GRadialLine* radial1 = radials.at(i);
                    for (size_t k = 0; k < pointSize; k++)
                    {
                        const GRadialPoint& gpt = radial1->points[k];
                        const double value = gpt.value;
                        if (value < mColorRange[0] || value > mColorRange[1]) {
                            continue;
                        }
                        double tlon;
                        double tlat;
                        GMapCoordConvert::mercatorToLonLat(gpt.x, gpt.y, &tlon, &tlat);
                        int x = mCenterX + (tlon - slon) * mScaleLon;
                        int y = mCenterY + (tlat - slat) * mScaleLat;
                        int value1 = (int)value + 120;
                        if (x >= 0 && x <= imagewidth && y >= 0 && y <= imageheight)
                        {
                            int index = y * imageheight + x;
                            p[index] = value1;
                        }
                    }
                }
               
                image.push_back(im);
                /*
                static int nNum = 0;
                QString strFileName = "d:/testImage/" + QString::number(nNum).sprintf("%05d", nNum++)+".bmp";
                im.save(strFileName, "BMP");
                */
            }

        }
        Images.push_back(image);
        num++;
    }
    qDebug() << "ִ����:" << num << endl;
    return 1;

}
int GRader2DLayer::savePreInfo(QVector<QVector<QImage>>& Images,int PredictNum)
{

    const GMapView* mapView = getView();
   // double sclae = mpView->scale();
   // qDebug() << "scale:" << sclae << endl;
    int volumesize = mRaderDataList->size();
    int start = volumesize - PredictNum;
    int end = volumesize;
   
    int num = 0;
    //for (int k = start; k < end; k++)
    for (int k = 0; k < volumesize; k++)
    {
        qDebug() << "�������:" << k << endl;
        const GRadarVolume* volume = mRaderDataList->at(k);
        int surfsize = volume->surfs.size();
        vector<double> v;
        for (int i = 0; i < surfsize; i++)
        {
            v.push_back(volume->surfs[i]->el);
            
        }
        if (cheekisSave == -1)
        {
            double lon = volume->longitude;
            double lat = volume->latitude;
            double mx = 0, my = 0;
            GMapCoordConvert::lonLatToMercator(lon, lat, &mx, &my);
            const QPoint pt = mapView->lpTodpc(QPointF(mx, my));
            qDebug() << "��ʱ���������Ļ��" << pt.x() << "--" << pt.y() << endl;
            setLonpLatp(pt);
            //Lonp = lon;
            //Latp = lat;
            setElev(volume->elev);
            setELs(surfsize, v);
            cheekisSave = 1;
        }
        QVector<QImage> image;
        //vector<vector<double>> tlonsp;
       // vector<vector<double>> tlatsp;
        for (int i = 0; i < surfsize; i++)
        {
            auto* surface = volume->surfs[i];
            if (!surface->isConvert) {
                GRadarAlgorithm::surfToMercator(surface, volume->longitude, volume->latitude, volume->elev);
            }
            const vector<GRadialLine*>& radials = surface->radials;
            const int radialSize = radials.size();
            const int pointSize = radials[0]->points.size();
            //QPainter painter;
           // QImage im(920, 920, QImage::Format_RGB32);
            QImage im(920, 920, QImage::Format_Grayscale8);
            im.fill(0);
            uchar* p = im.bits();
            int vectorsize = 920 * 920;
           // vector<double> lonsp(vectorsize, 0);
          //  vector<double> latsp(vectorsize, 0);
            //QPen pen;
            //painter.begin(&im);
            for (int i = 0; i < radialSize; i++)
            {
                const GRadialLine* radial1 = radials.at(i);
                for (size_t k = 0; k < pointSize; k++)
                {
                    const GRadialPoint& gpt = radial1->points[k];
                    const double value = gpt.value;
                    if (value < mColorRange[0] || value > mColorRange[1]) {
                        // �������Чֵ�Ͳ�����
                        continue;
                    }
                    const QPoint&& pt = mapView->lpTodpc(QPointF(gpt.x, gpt.y));
                   /*
                    int value1 = (int)value + 35;
                    im.setPixel(pt.x(), pt.y(), qRgb(value1, value1, value1));
                  */
                    int value1 = (int)value + 35.0;
                    if (pt.x() >= 0 && pt.x() <= 920 && pt.y() >= 0 && pt.y() <= 920)
                   {
                        int index = pt.y() * 920 + pt.x();
                        p[index] = value1;
                       // lonsp[index] = gpt.x;
                       // latsp[index] = gpt.y;
                   }

                }
            }
            //painter.end();
          //  tlonsp.push_back(lonsp);
           // tlatsp.push_back(latsp);
           // lonsp.clear();
           // latsp.clear();4
            /*image.push_back(im);
            static int nFileNum = 0;
            QString strFileName="d:/"+QString::number(nFileNum).sprintf("%04d",nFileNum) + ".bmp";
            
            im.save(strFileName,"BMP");
            nFileNum++;*/
        }
        Images.push_back(image);
       

        //mLonsP.push_back(tlonsp);
     //   mLatsP.push_back(tlatsp);
    }
    qDebug() << "ִ����:" << num << endl;
    return 1;
}
void GRader2DLayer::setPredictValue(double value, double* mvalue)
{
    *mvalue = value - 120.0;
}
GRadarVolume* GRader2DLayer::CompPredictVolumeP(int oldsize, int index)
{
    GRadarVolume* volume = mRaderDataList->at(0);;
    int sursize = volume->surfs.size();
    double sLon = toRadian(volume->longitude);
    double sLat = toRadian(volume->latitude);
    double sin_slat = sin(sLat);
    double cos_slat = cos(sLat);
    double elev = volume->elev;
    GRadarVolume* tvolume = new GRadarVolume();
    GRadialSurf* tsurfs = nullptr;
    qDebug() << "MarkA";
    for (int i = 0; i < sursize; i++)
    {
        tsurfs = nullptr;
        tsurfs = new GRadialSurf();
        int tlinesNum = volume->surfs[i]->radials.size();
        for (int j = 0; j < tlinesNum; j++)
        {
            int tpointsNum = volume->surfs[i]->radials[j]->points.size();
            GRadialLine* tlines = new GRadialLine();
            tlines->az = volume->surfs[i]->radials[j]->az;
            tlines->azRadian = volume->surfs[i]->radials[j]->azRadian;
            tlines->el = volume->surfs[i]->radials[j]->el;
            tlines->elRadian = volume->surfs[i]->radials[j]->elRadian;
            for (int k = 0; k < tpointsNum; k++)
            {
                double value = volume->surfs[i]->radials[j]->points[k].value;
                double lon = volume->surfs[i]->radials[j]->points[k].x;
                double lat = volume->surfs[i]->radials[j]->points[k].y;
                //tlines->points[k].value = value;
                tlines->points.push_back(GRadialPoint(value));
                tlines->points[k].x = lon;
                tlines->points[k].y = lat;
            }
            tsurfs->radials.push_back(tlines);
        }
        tsurfs->interval = volume->surfs[i]->interval;
        tsurfs->el = volume->surfs[i]->el;
        tsurfs->elRadian = volume->surfs[i]->elRadian;
        tsurfs->bound = volume->surfs[i]->bound;
        tsurfs->isConvert = volume->surfs[i]->isConvert;

        tvolume->surfs.push_back(tsurfs);
    }
    qDebug() << "MarkB";
    tvolume->surfNum = volume->surfNum;
    tvolume->elev = volume->elev;
    tvolume->latitude = volume->latitude;
    tvolume->longitude = volume->longitude;
    tvolume->siteCode = volume->siteCode;
    tvolume->siteName = volume->siteName;
    //tvolume->startTime = volume->startTime;
    // tvolume->minEl = volume->minEl;
    // tvolume->maxEl = volume->maxEl;
    //tvolume->maxPointCount = volume->maxPointCount;
    tvolume->bound = volume->bound;
    
    int timeindex1 = oldsize - 1;
    int timeindex2 = timeindex1 - 1;
    QDateTime datetime1 = QDateTime::fromString(mRaderDataList->at(timeindex1)->startTime, "yyyy-MM-dd hh:mm:ss");
    int timestamp1 = datetime1.toSecsSinceEpoch();

    QDateTime datetime2 = QDateTime::fromString(mRaderDataList->at(timeindex2)->startTime, "yyyy-MM-dd hh:mm:ss");
    int timestamp2 = datetime2.toSecsSinceEpoch();

    int timestamp = timestamp1 + (timestamp1 - timestamp2) * index;

    tvolume->startTime = QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd hh:mm:ss");

    for (int i = 0; i < sursize; i++)
    {
        //GRadialSurf* surface = volume->surfs[i];
        GRadialSurf* surface = tvolume->surfs[i];
        double elRadian = surface->elRadian;
        double el = surface->el;
       
        for (GRadialLine* line : surface->radials)
        {
            double azRadian = line->azRadian;
            size_t size = line->points.size();

#pragma omp parallel for
            for (int j = 0; j < size; j++) {
                    //qDebug() << "j " << j;
                auto& item = line->points[j];
                //setPredictValue(0, &item.value);
                double r = (j + 1) * surface->interval;
                double el1 = r * 0.5 / RN;
                double el0 = elRadian - el1;
                double r_ = 2 * RN * sin(el1);
                double gl = r_ * cos(el0);

                double gx = gl * sin(azRadian);
                double gy = gl * cos(azRadian);
                double gz = r_ * sin(el0);
                double v = NearinteP(el, gx, gy, gz, 1000,1000, 1500);
                setPredictValue(v, &item.value);
                //qDebug() << "value:" << item.value << endl;
                // gz + ���ĵ��״����
                const double l = gz + elev + RE;
                // ���ĵ㵽���ƽ��߶�
                const double ch = l * sin_slat;
                // ���ĵ����ĵ��ڳ��ƽ����ͶӰ�����
                const double cl = l * cos_slat;

                const double dcl = gy * sin_slat;
                const double dch = gy * cos_slat;
                const double cl0 = cl - dcl;

                double lon = atan2(gx, cl0) + sLon;
                double lat = atan2(ch + dch, sqrt(gx * gx + cl0 * cl0));
                lon = toAngle(lon);
                lat = toAngle(lat);

                // TODO ����������ľ�γ��
                if (lon > 180.0) {
                    lon -= 360.0;
                }
                else if (lon <= -180.0) {
                    lon += 360.0;
                }
                // ��γ��תī����ͶӰ
                GMapCoordConvert::lonLatToMercator(lon, lat, &item.x, &item.y);
            }
        }
        surface->bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
        surface->isConvert = true;
        //file.close();
    }
    qDebug() << "this 4:" << endl;
    return tvolume;
}
void GRader2DLayer::CompPredictVolume()
{

    GRadarVolume* volume = mRaderDataList->at(0);;
    int sursize = volume->surfs.size();
    double sLon = toRadian(volume->longitude);
    double sLat = toRadian(volume->latitude);
    double sin_slat = sin(sLat);
    double cos_slat = cos(sLat);
    double elev = volume->elev;
    GRadarVolume* tvolume = new GRadarVolume();
    GRadialSurf* tsurfs = nullptr;
    qDebug() << "this 1:" << endl;
    for (int i = 0; i < sursize; i++)
    {
        tsurfs = nullptr;
        tsurfs = new GRadialSurf();
        int tlinesNum = volume->surfs[i]->radials.size();
        for (int j = 0; j < tlinesNum; j++)
        {
            int tpointsNum = volume->surfs[i]->radials[j]->points.size();
            GRadialLine* tlines = new GRadialLine();
            tlines->az = volume->surfs[i]->radials[j]->az;
            tlines->azRadian = volume->surfs[i]->radials[j]->azRadian;
            tlines->el = volume->surfs[i]->radials[j]->el;
            tlines->elRadian = volume->surfs[i]->radials[j]->elRadian;
            for (int k = 0; k < tpointsNum; k++)
            {
                double value = volume->surfs[i]->radials[j]->points[k].value;
                double lon = volume->surfs[i]->radials[j]->points[k].x;
                double lat = volume->surfs[i]->radials[j]->points[k].y;
                //tlines->points[k].value = value;
                tlines->points.push_back(GRadialPoint(value));
                tlines->points[k].x = lon;
                tlines->points[k].y = lat;
            }
            tsurfs->radials.push_back(tlines);
        }
        tsurfs->interval = volume->surfs[i]->interval;
        tsurfs->el = volume->surfs[i]->el;
        tsurfs->elRadian = volume->surfs[i]->elRadian;
        tsurfs->bound = volume->surfs[i]->bound;
        tsurfs->isConvert = volume->surfs[i]->isConvert;

        tvolume->surfs.push_back(tsurfs);
    }
    qDebug() << "this 2:" << endl;
    ///////////////
   // auto start_time = std::chrono::system_clock::now();

    tvolume->surfNum = volume->surfNum;
    tvolume->elev = volume->elev;
    tvolume->latitude = volume->latitude;
    tvolume->longitude = volume->longitude;
    tvolume->siteCode = volume->siteCode;
    tvolume->siteName = volume->siteName;
    tvolume->startTime = volume->startTime;
    // tvolume->minEl = volume->minEl;
    // tvolume->maxEl = volume->maxEl;
    //tvolume->maxPointCount = volume->maxPointCount;
    tvolume->bound = volume->bound;
    qDebug() << "this 3:" << endl;
    for (int i = 0; i < sursize; i++)
    {
        //GRadialSurf* surface = volume->surfs[i];
        GRadialSurf* surface = tvolume->surfs[i];
        double elRadian = surface->elRadian;
        double el = surface->el;
        qDebug() << "i;" << i << endl;
        for (GRadialLine* line : surface->radials)
        {
            double azRadian = line->azRadian;
            size_t size = line->points.size();
           
#pragma omp parallel for
            for (int j = 0; j < size; j++) {
                    //qDebug() << "j " << j;
                auto& item = line->points[j];
                //setPredictValue(0, &item.value);
                double r = (j + 1) * surface->interval;
                double el1 = r * 0.5 / RN;
                double el0 = elRadian - el1;
                double r_ = 2 * RN * sin(el1);
                double gl = r_ * cos(el0);

                double gx = gl * sin(azRadian);
                double gy = gl * cos(azRadian);
                double gz = r_ * sin(el0);
                double v = NearinteP(el, gx, gy, gz, 2500, 2500, 2500);
                setPredictValue(v, &item.value);

                // gz + ���ĵ��״����
                const double l = gz + elev + RE;
                // ���ĵ㵽���ƽ��߶�
                const double ch = l * sin_slat;
                // ���ĵ����ĵ��ڳ��ƽ����ͶӰ�����
                const double cl = l * cos_slat;

                const double dcl = gy * sin_slat;
                const double dch = gy * cos_slat;
                const double cl0 = cl - dcl;

                double lon = atan2(gx, cl0) + sLon;
                double lat = atan2(ch + dch, sqrt(gx * gx + cl0 * cl0));
                lon = toAngle(lon);
                lat = toAngle(lat);

                // TODO ����������ľ�γ��
                if (lon > 180.0) {
                    lon -= 360.0;
                }
                else if (lon <= -180.0) {
                    lon += 360.0;
                }
                // ��γ��תī����ͶӰ
                GMapCoordConvert::lonLatToMercator(lon, lat, &item.x, &item.y);
            }
        }
        surface->bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
        surface->isConvert = true;
        //file.close();
    }
    qDebug() << "this 4:" << endl;
    /*
    auto end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    qDebug() << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    std::cout << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den << std::endl;
    */
    setPredictVolume(tvolume);
 
}
void GRader2DLayer::CompleteVolume()
{
  
    qDebug() << "a" << endl;
    int indexh = 0;
    int vsum = 0;
    int elsum = 0;
    double maxlon = 0;
    double maxlat = 0;

    qDebug() << "mvalues.size():" << mvalues.size() << endl; 
    qDebug() << "mlon.size():" << mlons.size() << endl;
    qDebug() << "mlats.size():" << mlats.size() << endl;
    for (int i = 0; i < mSurnums; i++)
    {
        const double te = mELs[i];
        const double el = toRadian(te);
        elsum = 0;
        for (int j = 0; j < mImageWideth; j++)
        {
            for (int k = 0; k < mImageHeight; k++)
            {
                //if (values[indexh] != 0||values[indexh]!=255)

                //if (mvalues[indexh] != 255)
                if (mvalues[indexh] != 0)
                {
                    double gx = 0;
                    double gy = 0;
                    //if (maxlon < mlons[indexh])
                      //  maxlon = mlons[indexh];
                  //  //if (maxlat < mlats[indexh])
                     //   maxlat = mlats[indexh];
                    GRadarAlgorithm::lonLatToGrid(mlons[indexh], mlats[indexh], 0, mSLon, mSLat, mElev, &gx, &gy);
                    mGXS.push_back(gx);
                    mGYS.push_back(gy);
                    double r = sqrt(gx * gx + gy * gy);
                    double gz = r * tan(el);
                    mGZS.push_back(gz);
                    mgvalues.push_back(mvalues[indexh]);
                    vsum++;
                    elsum++;
                }

                indexh++;
            }

        }
        moneELnums.push_back(elsum);
        qDebug() << "--elsum:" << elsum << endl;
    }
    qDebug() << "maxlon:" << maxlon << endl;
    qDebug() << "maxlat:" << maxlat << endl;
    qDebug() << "vsum:" << vsum << endl;
    qDebug() << "elsum:" << moneELnums.size() << endl;
    qDebug() << "Els.size:" << mELs.size() << endl;

    qDebug() << "c" << endl;
}
int GRader2DLayer::load_PredictImageIn(QVector<QImage> Image, int surfindex)
{
    const GRadarVolume* volume = mRaderDataList->at(0);;
    int sursize = volume->surfs.size();
    //�״�վ�������Ϣ
    double sLon = toRadian(volume->longitude);
    double sLat = toRadian(volume->latitude);
    double elev = volume->elev;

    double SElev = mElev;
    int Sufnums = mELs.size();
    mSurnums = mELs.size();

    for (int k = 0; k < Sufnums; k++)
    {
        QImage image = Image[k];
        mImageWideth = image.width();
        mImageHeight = image.height();
        uchar* timagevalue = image.bits();

        auto* surface = volume->surfs[k];
        QRectF bound = surface->bound;

        if (bound.isNull()) {
            bound = GRadarAlgorithm::calcMercatorBound(surface, volume->longitude, volume->latitude, volume->elev);
        }
        double dx = bound.width() / (mImageWideth - 1);
        double dy = bound.height() / (mImageHeight - 1);

        for (int row = 0; row < mImageHeight; row++)
        {
            const double dpY = bound.top() + row * dy;
         
            double dpX = bound.left() - dx;
            for (int col = 0; col < mImageWideth; col++)
            {
                dpX += dx;
               
                // ī����ֱ��ת��γ�Ȼ���
                //double lon = dpX / EHC * PI - sLon;
                //double lat = atan(exp(dpY * PI / EHC)) * 2.0 - PI / 2.0;
                double lon = 0;
                double lat = 0;
                GMapCoordConvert::mercatorToLonLat(dpX, dpY, &lon, &lat);
                int index = row * mImageHeight + col;
                uchar pixelvalue = timagevalue[index];

                int value = (int)pixelvalue;
                mvalues.push_back(value);
                mlons.push_back(lon);
                mlats.push_back(lat);
            }
        }

    }
    return 1;
}
int GRader2DLayer::load_PredictImage(QVector<QImage> Image, int surfindex)
{
    /*
    QPoint p = getLonpLatp();
    
    QPointF tpf = cvdptolp(QPoint(p.x(), p.y()));
    double lon1 = 0, lat1 = 0;
    GMapCoordConvert::mercatorToLonLat(tpf.x(), tpf.y(), &lon1, &lat1);
  
   mSLon = lon1;
   mSLat = lat1;
   */
   
    double SElev = mElev;
    int Sufnums = mELs.size();
    mSurnums = mELs.size();

    QVector<QPointF> pointfs;
    QVector<QPoint> vpoint;
    QVector<QPointF> vfpoints;
    for (int k = 0; k < Sufnums; k++)
    {
        QImage image = Image[k];
        mImageWideth = image.width();
        mImageHeight = image.height();
        uchar* timagevalue = image.bits();
        for (int i = 0; i < mImageWideth; i++)
        {
            for (int j = 0; j < mImageHeight; j++)
            {
                int index = i * mImageHeight + j;
                uchar pixelvalue = timagevalue[i * mImageHeight + j];
                int value = (int)pixelvalue;
                mvalues.push_back(value);
                //double lons = mLonsP[surfindex][k][index];
                //double lats = mLatsP[surfindex][k][index];
                //vfpoints.push_back(QPointF(lons, lats));
                vpoint.push_back(QPoint(j, i));
            }
        }
    }
    //������ת��Ϊ��γ��
    for (int i = 0; i < vpoint.size(); i++)
    {
        /*
        QPointF pf = cvdptolp(vpoint[i]);
        //QPointF pf = vfpoints[i];
        pointfs.push_back(pf);
        double lon = 0, lat = 0;
        GMapCoordConvert::mercatorToLonLat(pf.x(), pf.y(), &lon, &lat);
        */
        double lon = 0, lat = 0;
        lon = (vpoint[i].x() - mCenterX) / mScaleLon + mSLon;
        lat = (vpoint[i].y() - mCenterY) / mScaleLat + mSLat;
        mlons.push_back(lon);
        mlats.push_back(lat);

    }
  
    return 1;
}
int GRader2DLayer::findel(double el)
{
    const int size = mELs.size() - 1;
    //qDebug() << "mEls.size(0" << ELs.size() << endl;
    //��β�����
    if (el < mELs[0] || el >= mELs[size])
    {
        return -1;
    }
    //�м����
    for (int i = 1; i <= size - 1; i++)
    {
        if (el < mELs[i])
        {
            return i - 1;
        }
    }
    return size - 1;
}
//����ھӷ���
double GRader2DLayer::NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz)
{
    //vector<double> els = ELs;

    int iel = findel(e);
    if (iel == -1)
        return -1000;


    int size1 = moneELnums[iel] - 1;
    //int size2 = oneELnums[iel + 1];
    int index1 = 0;
    int index2 = 0;
    //int index3 = 0;
    if (iel == 0)
    {
        index1 = 0;
        index2 = size1;
        //index3 = index2 + size2;
    }
    else
    {
        for (int i = 0; i < iel; i++)
        {
            index1 = index1 + moneELnums[i] - 1;
        }
        index2 = index1 + size1;
        //index3 = index2 + size2;
    }
    for (int i = index1; i < index2 - 1; i++)
    {
        //if (i < 0 || i >= mGYS.size()) break;
        if (abs(gx - mGXS[i]) < 2 * dx && abs(gy - mGYS[i]) < 2 * dy && abs(gz - mGZS[i]) < 2 * dz)
            //if (abs(gx - GXS[i]) < 4*dx && abs(gy - GYS[i]) < 4 * dy && abs(gz - GZS[i]) < 4 * dz)
            return mgvalues[i];
    }
    return -1000;
}
//����������ǲ�
int GRader2DLayer::findel(double el, vector<double>& ELs)
{

    const int size = ELs.size() - 1;
    //qDebug() << "mEls.size(0" << ELs.size() << endl;
    //��β�����
    if (el < ELs[0] || el >= ELs[size])
    {
        return -1;
    }
    //�м����
    for (int i = 1; i <= size - 1; i++)
    {
        if (el < ELs[i])
        {
            return i - 1;
        }
    }
    return size - 1;
}
//����ھӷ���
double GRader2DLayer::NearinteP(double e, double gx, double gy, double gz, double dx, double dy, double dz
    , vector<double>& lons, vector<double>& lats,
    vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
    vector<int>& oneELnums, vector<double>& ELs)
{
    vector<double> els = ELs;
    //qDebug() << "1" << endl;
    int iel = findel(e, els);
    //qDebug() << "1" << endl;
    if (iel == -1)
        return -1000;


    int size1 = oneELnums[iel];
    //int size2 = oneELnums[iel + 1];
    int index1 = 0;
    int index2 = 0;
    //int index3 = 0;
    if (iel == 0)
    {
        index1 = 0;
        index2 = size1;
        //index3 = index2 + size2;
    }
    else
    {
        for (int i = 0; i < iel; i++)
        {
            index1 = index1 + oneELnums[i];
        }
        index2 = index1 + size1;
        //index3 = index2 + size2;
    }
    for (int i = index1; i < index2; i++)
    {
        if (abs(gx - GXS[i]) < 2 * dx && abs(gy - GYS[i]) < 2 * dy && abs(gz - GZS[i]) < 2 * dz)
            //if (abs(gx - GXS[i]) < 4*dx && abs(gy - GYS[i]) < 4 * dy && abs(gz - GZS[i]) < 4 * dz)
            return gvalues[i];
    }
    return -1000;
}
void GRader2DLayer::PredictinterpolateImageOMP(const GRadialSurf* surface, const size_t width, const size_t height,
    const double longitude, const double latitude, const double elev,
    const QRectF& bound, vtkColorTransferFunction* mColorTF,
    double* points, uchar* imageData, vector<double>& lons, vector<double>& lats,
    vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
    vector<int>& oneELnums, vector<double>& ELs)
{
    const double dx = bound.width() / (width - 1);
    const double dy = bound.height() / (height - 1);
    
    // �״�վ������
    const double slon = toRadian(longitude);
    const double slat = toRadian(latitude);
    const double elev_add_RE_div_RN = (elev + RE) / RN;

    // ׶������
    const double tel = surface->el;
    const double el = toRadian(surface->el);
    const double minR = surface->interval;
    const double maxR = minR * surface->radials[0]->points.size();
#pragma omp parallel
    {
#pragma omp for
        for (int row = 0; row < height; row++) {

            const double dpY = bound.top() + row * dy;
            double rgb[3] = {};
            double dpX = bound.left() - dx;
            for (int col = 0; col < width; col++) {
                dpX += dx;
                
                const double dlon = dpX / EHC * PI - slon;
                const double lat = atan(exp(dpY * PI / EHC)) * 2.0 - PI / 2.0;

                const double cos_lat = cos(lat);
                const double a = acos(sin(slat) * sin(lat) +
                    cos(slat) * cos_lat * cos(dlon));
                const double sin_a = sin(a);

                const double r = fabs(RN * (a + el + asin(fma(elev_add_RE_div_RN, sin_a, -sin(a + el)))));
                const double gz = r * sin(el);

                double tlon = 0;
                double tlat = 0;
               
                GMapCoordConvert::mercatorToLonLat(dpX, dpY, &tlon, &tlat);
                double gx = 0;
                double gy = 0;
               
                GRadarAlgorithm::lonLatToGrid(tlon, tlat, 0,
                    longitude, latitude, elev,
                    &gx, &gy);
               
                double value = NearinteP(tel, gx, gy, gz, dx, dy, 1000, lons, lats,
                    GXS, GYS, GZS, values, gvalues, oneELnums, ELs) - 35.0;
               
                if (value < -5.0 || value > 70.0) continue;
                mColorTF->GetColor(value, rgb);
                uchar* current = &imageData[((height - row - 1) * width + col) * 4];
                current[0] = 255 * rgb[0];
                current[1] = 255 * rgb[1];
                current[2] = 255 * rgb[2];
                current[3] = 255;

            }
        }

    }
}
void GRader2DLayer::PredictinterpolateSurfaceRealTime(const GRadarVolume* body, const GRadialSurf* surface,
    const QRectF& bound, size_t width, size_t height, vector<double>& lons, vector<double>& lats,
    vector<double>& GXS, vector<double>& GYS, vector<double>& GZS, vector<double>& values, vector<double>& gvalues,
    vector<int>& oneELnums, vector<double>& ELs)
{
    const size_t n = width * height;
    // ʹ��std::unique_ptr�Զ��ͷ��ڴ�
    PredictimageDataRealTime.reset(new uchar[width * height * 4]{ 0 });
    // TODO ����ֵͼ������, �Զ�ѡ��ʹ��CPU����GPU����
    auto t0 = std::chrono::steady_clock::now();
   
    GRader2DLayer::PredictinterpolateImageOMP(surface, width, height,
        body->longitude, body->latitude, body->elev,
        bound, mColorTF,
        nullptr,
        PredictimageDataRealTime.get(), lons, lats,
        GXS, GYS, GZS, values, gvalues, oneELnums, ELs);
   
    auto t1 = std::chrono::steady_clock::now();
    // ������ƶ����캯��ת����Դ
    this->PredictsurfImageRealTime = QImage(PredictimageDataRealTime.get(), width, height, QImage::Format::Format_RGBA8888);
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}
//����Ԥ����ֵ���״�����
void GRader2DLayer::drawPredictInterpolation(QPainter* painter, const QRect& rect)
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
    
    PredictinterpolateSurfaceRealTime(volume, surface, bound, drawRect.width(), drawRect.height(), mlons, mlats,
        mGXS, mGYS, mGZS, mvalues, mgvalues, moneELnums, mELs);
   
    painter->drawImage(drawRect, PredictsurfImageRealTime);

}