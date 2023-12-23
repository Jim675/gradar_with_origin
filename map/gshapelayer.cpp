#include "gshapelayer.h"

#include <fstream>

#include <QByteArray>
#include <QPointF>
#include <QDebug>
#include <QPainter>
#include <QString>
#include <qpainterpath.h>
#include <qpaintengine.h>

#include <gmapcoordconvert.h>
#include <ggpscorrect.h>

#include "gmapview.h"

#include "gshapereader.h"
#include "grader2dlayer.h"
#include "gdbfreader.h"

using std::ofstream;

ShapeDraw::ShapeDraw(const QString& shapePath, const QString& addressPath)
{
    mShape = ShapeReader::read(shapePath);
    if (mShape != nullptr) { // ��Ҫ�ж��ļ��Ƿ��ȡ�ɹ�
       // ת��ΪWebī��������
        double mx = 0, my = 0;
        auto& points = mShape->mPoints;
        for (int i = 0; i < points.size(); i++) {
            double lon = points[i].x();
            double lat = points[i].y();
            // WGS84ת��Ϊ��������(��ƫ)
            GGPSCorrect::wgsTogcj2(lon, lat, lon, lat);
            GMapCoordConvert::lonLatToMercator(lon, lat, &mx, &my);
            points[i].setX(mx);
            points[i].setY(my);
        }

        mShapeList = this->mShape->getPolygonList();
        for (const auto& item : mShapeList) {
            mBoundList.append(item.boundingRect());
        }
    }
    mAddressList = ShapeDraw::readAddressList(addressPath);
    //for (int i = 0; i < mAddressList.size(); i++) {
    //    auto& item = mAddressList[i];
    //    double lon = item.x;
    //    double lat = item.y;
    //    //GGPSCorrect::gcj2Towgs(lon, lat, lon0, lat0);
    //    GGPSCorrect::wgsTogcj2(lon, lat, lon, lat);
    //    Convert::lonLatToMercator(lon, lat, &item.x, &item.y);
    //}
}

// ��ȡ��ַ�б�
QVector<AddressNode> ShapeDraw::readAddressList(const QString& path)
{
    QVector<AddressNode> list;
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return list;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        auto tuple = line.split(',');
        if (tuple.size() != 3) {
            qDebug("��ȡ��ַ�ı�����");
            continue;
        }
        double x = tuple[1].toDouble();
        double y = tuple[2].toDouble();
        list.append({tuple[0],x,y});
    }
    file.close();
    return list;
}

void merge(Shape* shape, Dbf* dbf, const string& path)
{
    ofstream file(path, ios::out | ios::trunc);
    if (file.fail()) {
        return;
    }
    file.precision(2);
    //file.unsetf(ios::se)
    file.setf(ios::fixed, ios::floatfield);
    for (int i = 0; i < dbf->mTable[0]->mFieldList.size(); i++) {
        QPointF& point = shape->mPoints[i];
        string& name = dbf->mTable[0]->mFieldList[i];
        int j = name.size() - 1;
        for (; j >= 0 && name[j] == ' '; j--) {
        }
        QPointF p;
        GGPSCorrect::wgsTogcj2(point.x(), point.y(), p.rx(), p.ry());
        p = GMapCoordConvert::lonLatToMercator(p);
        file << name.substr(0, j + 1) << "," << p.x() << "," << p.y() << std::endl;
    }
    file.close();
}

GShapeLayer::GShapeLayer()
{
    QString dir = "./shp";
    // ��ȡshape�ļ�
    mShapeProvince = new ShapeDraw(dir + "/Province.shp", dir + "/ProvinceName.txt");
    mShapeCity = new ShapeDraw(dir + "/City.shp", dir + "/CityName.txt");
    mShapeCounty = new ShapeDraw(dir + "/County.shp", dir + "/CountyName.txt");

    //Shape* a = ShapeReader::read(dir + "/ProvinceName.shp");
    //Dbf* b = DbfReader::read("./shp/ProvinceName.dbf");
    //merge(a, b, "./shp/ProvinceName.txt");
    //delete a;
    //delete b;

    //a = ShapeReader::read(dir + "/CityName.shp");
    //b = DbfReader::read("./shp/CityName.dbf");
    //merge(a, b, "./shp/CityName.txt");
    //delete a;
    //delete b;

    //a = ShapeReader::read(dir + "/CountyName.shp");
    //b = DbfReader::read("./shp/CountyName.dbf");
    //merge(a, b, "./shp/CountyName.txt");
    //delete a;
    //delete b;

    //qDebug("д���ļ����");
}

GShapeLayer::~GShapeLayer()
{
    delete mShapeProvince;
    delete mShapeCity;
    delete mShapeCounty;
}


//��ӡʱ��
static void printDTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char* msg)
{
    auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    qDebug() << msg << ": " << dtime.count() << "ms";
}

// ����ͼ��
void GShapeLayer::draw(QPainter* painter, const QRect& rect)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    auto t1 = std::chrono::steady_clock::now();
    const GMapView* view = getView();
    int scaleGrade = view->scaleGrade();

    int width = 1;
    const ShapeDraw* shape = nullptr;
    if (scaleGrade <= 6) {
        shape = mShapeProvince;
        width = 2;
    } else if (scaleGrade <= 9) {
        shape = mShapeCity;
        width = 2;
    } else {
        shape = mShapeCounty;
        width = 3;
    }

    QPen pen(Qt::PenStyle::SolidLine);
    pen.setColor(QColor(0, 0, 0));
    pen.setWidth(width);
    painter->setPen(pen);

    drawPolygon(painter, rect, shape);
    auto t2 = std::chrono::steady_clock::now();

    //QBrush brush(QColor(255, 0, 0), Qt::SolidPattern);
    //painter->setBrush(brush);
    drawAddress(painter, rect, shape);
    //auto t3 = std::chrono::steady_clock::now();

    //printDTime(t1, t2, "drawPolygon spend time");
    //printDTime(t2, t3, "drawAddress spend time");
    painter->setRenderHint(QPainter::Antialiasing, false);
}

//����shp�ļ�
//void ShapeLayer::drawShape(QPainter* painter, const QRect& rect, QVector<QPolygonF>* shapeList)
//{
//    switch (shape->mShapeType) {
//        case Shape::TYPE_POINT:
//            ShapeLayer::drawPoint(painter, rect, shape);
//            break;
//        case Shape::TYPE_LINE:
//            ShapeLayer::drawLine(painter, rect, shape);
//            break;
//        case Shape::TYPE_POLYGON:
//            ShapeLayer::drawPolygon(painter, rect, m);
//            break;
//    }
//}

// ���Ƶ�
void GShapeLayer::drawPoint(QPainter* painter, const QRect& rect, const Shape* shape)
{
    double lon = 0, lat = 0;
    double mx = 0, my = 0;
    GMapView* view = getView();
    for (int i = 0; i < shape->mPoints.size(); i++) {
        lon = shape->mPoints[i].x();
        lat = shape->mPoints[i].y();
        view->lpTodp({mx,my});
        painter->drawPoint(QPointF(mx, my));
    }
}

// ������
void GShapeLayer::drawLine(QPainter* painter, const QRect& rect, const Shape* shape)
{
    //QPen pen(QColor(0, 0, 0), Qt::SolidLine);
    //ʹ�û�ˢ
    //painter->setPen(pen);
    const GMapView* view = getView();

    QVector<QPointF> points;
    // ��ǰ�������л���������ʼƫ��
    int allPartsStartOffset = 0;
    // ��ǰ����ʼ�������е���������ʼƫ��
    int allPointsStartOffset = 0;
    //��ʼ����ľ���
    const int drawDistanceSqrt = 8;
    for (int i = 0; i < shape->mRecordParts.size() - 1; i++)//��ÿ����¼���ݽ��д���
    {
        int partsNum = shape->mRecordParts[i];//��õ�ǰ��״Ŀ���ӻ�����
        int pointsNum = shape->mRecordPoints[i];//��ù��ɵ�ǰ�ӻ��ĵ�ĸ���

        int allPartsEndIndex = allPartsStartOffset + partsNum;
        for (; allPartsStartOffset < allPartsEndIndex; allPartsStartOffset++) { // ����ÿ����
            // ��ǰ������ʼ���ڵ�ǰ����ӵ�еĵ������ƫ��
            int curPointsStartOffset = shape->mPartsIndexList[allPartsStartOffset];
            //qDebug() << allPartsStartOffset << "," << curPointsStartOffset << "," << pointsNum;
            int curPointsNum = 0; // ��ǰ���а�����ĸ���
            if (allPartsStartOffset == shape->mPartsIndexList.size() - 1 || shape->mPartsIndexList[allPartsStartOffset + 1] == 0) {
                // ��ֹԽ��
                curPointsNum = pointsNum - curPointsStartOffset;
            } else {
                curPointsNum = shape->mPartsIndexList[allPartsStartOffset + 1] - curPointsStartOffset;
            }
            points.clear();
            points.reserve(curPointsNum);
            //QPainterPath path;
            // ��¼��һ����
            QPoint lastDrawPointDp;
            bool next = false;
            for (int j = 0; j < curPointsNum; j++) {
                int index = allPointsStartOffset + j;
                QPointF point = shape->mPoints[index];
                QPoint pointDp = view->lpTodp({point.x(),point.y()});
                if (next) {
                    const int dx = pointDp.x() - lastDrawPointDp.x();
                    const int dy = pointDp.y() - lastDrawPointDp.y();
                    const int disSqrt = dx * dx + dy * dy;
                    if (disSqrt >= drawDistanceSqrt) {
                        //path.lineTo(pointDp);
                        points.append(pointDp);
                        points.append(pointDp);
                        lastDrawPointDp = pointDp;
                    }
                } else {
                    next = true;
                    points.append(pointDp);
                    lastDrawPointDp = pointDp;
                }
            }
            //QVector<QPointF> points;
            points.remove(points.size() - 1);
            painter->drawLines(points);
            //painter->drawPolygon(points);
            allPointsStartOffset += curPointsNum;
        }
    }
}

// ���ƶ����
void GShapeLayer::drawPolygon(QPainter* painter, const QRect& rect, const ShapeDraw* shape)
{
    //��ʼ����ľ���
    const int drawDistanceSqr = 12;

    //painter->setRenderHint(QPainter::Antialiasing);
    QPen pen = painter->pen();
    pen.setColor(QColor(80, 80, 80));
    painter->setPen(pen);

    const GMapView* view = getView();

    QVector<QPoint> points;
    for (int i = 0; i < shape->mShapeList.size(); i++) {
        const QRectF& boundRect = shape->mBoundList.at(i);
        QRect boundRectDp = view->lpTodp(boundRect);;
        if (!rect.intersects(boundRectDp)) {
            continue;
        }
        const QPolygonF& polygon = shape->mShapeList.at(i);

        QPoint pt0 = view->lpTodp(polygon[0]);
        QPoint pt1;

        points.clear();
        for (int j = 1; j < polygon.size(); j++) {
            pt1 = view->lpTodp(polygon[j]);
            if (rect.contains(pt0) || rect.contains(pt1)) {
                const int dx = pt1.x() - pt0.x();
                const int dy = pt1.y() - pt0.y();
                if (dx * dx + dy * dy >= drawDistanceSqr) {
                    points.append(pt0);
                    points.append(pt1);
                    //painter->drawLine(pt0, pt1);
                    pt0 = pt1;
                }
            } else {
                pt0 = pt1;
            }
        }
        //poly.append(view->lpTodp(polygon[polygon.size() - 1]));
        //painter->drawPolygon(poly);
        //painter->drawPolyline(poly);
        //const char* name = typeid(painter->paintEngine()).name();
        painter->drawLines(points);
    }
    //painter->setRenderHint(QPainter::Antialiasing, false);
}

// ���ƶ����
void GShapeLayer::drawAddress(QPainter* painter, const QRect& rect, const ShapeDraw* shape)
{
    // ��ɫ��͸��
    QBrush brush1(QColor(255, 255, 255, 125), Qt::SolidPattern);
    // ��ɫ
    QBrush brush2(QColor(255, 0, 0), Qt::SolidPattern);

    QPen pen1 = painter->pen();
    pen1.setStyle(Qt::PenStyle::NoPen);

    QPen pen2 = painter->pen();
    pen2.setColor(QColor(0, 0, 0));
    pen2.setStyle(Qt::PenStyle::SolidLine);

    QFont font;
    font.setPointSize(10);
    painter->setFont(font);

    const QFontMetrics metrics = painter->fontMetrics();
    //�ַ����߶�
    const int height = metrics.height();
    auto* view = getView();
    for (int i = 0; i < shape->mAddressList.size(); i++) {
        auto& item = shape->mAddressList[i];
        //�ַ������
        int width = metrics.width(item.name);
        QPoint point = view->lpTodp({item.x, item.y});

        // �������ְ�Χ����
        int top = point.y() - 10;
        int bottom = point.y() + 6 + height;

        int left = point.x() - width / 2;
        int right = left + width;

        // �ж��ı����Ƿ���rect�ཻ
        if (top > rect.bottom() || bottom < rect.top() || left > rect.right() || right < rect.left()) {
            continue;
        }
        painter->setPen(pen1);
        painter->setBrush(brush1);
        painter->drawEllipse(point, 10, 10);

        painter->setBrush(brush2);
        painter->drawEllipse(point, 4, 4);

        painter->setPen(pen2);
        painter->drawText(left, bottom, item.name);
    }
}