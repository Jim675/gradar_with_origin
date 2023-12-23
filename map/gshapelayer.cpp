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
    if (mShape != nullptr) { // 需要判断文件是否读取成功
       // 转换为Web墨卡托坐标
        double mx = 0, my = 0;
        auto& points = mShape->mPoints;
        for (int i = 0; i < points.size(); i++) {
            double lon = points[i].x();
            double lat = points[i].y();
            // WGS84转换为火星坐标(加偏)
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

// 读取地址列表
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
            qDebug("读取地址文本错误");
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
    // 读取shape文件
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

    //qDebug("写入文件完毕");
}

GShapeLayer::~GShapeLayer()
{
    delete mShapeProvince;
    delete mShapeCity;
    delete mShapeCounty;
}


//打印时间
static void printDTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char* msg)
{
    auto dtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    qDebug() << msg << ": " << dtime.count() << "ms";
}

// 绘制图层
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

//绘制shp文件
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

// 绘制点
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

// 绘制线
void GShapeLayer::drawLine(QPainter* painter, const QRect& rect, const Shape* shape)
{
    //QPen pen(QColor(0, 0, 0), Qt::SolidLine);
    //使用画刷
    //painter->setPen(pen);
    const GMapView* view = getView();

    QVector<QPointF> points;
    // 当前环在所有环数组中起始偏移
    int allPartsStartOffset = 0;
    // 当前环起始点在所有点数组中起始偏移
    int allPointsStartOffset = 0;
    //开始画点的距离
    const int drawDistanceSqrt = 8;
    for (int i = 0; i < shape->mRecordParts.size() - 1; i++)//对每条记录内容进行处理
    {
        int partsNum = shape->mRecordParts[i];//获得当前面状目标子环个数
        int pointsNum = shape->mRecordPoints[i];//获得构成当前子环的点的个数

        int allPartsEndIndex = allPartsStartOffset + partsNum;
        for (; allPartsStartOffset < allPartsEndIndex; allPartsStartOffset++) { // 绘制每个环
            // 当前环中起始点在当前环所拥有的点数组的偏移
            int curPointsStartOffset = shape->mPartsIndexList[allPartsStartOffset];
            //qDebug() << allPartsStartOffset << "," << curPointsStartOffset << "," << pointsNum;
            int curPointsNum = 0; // 当前环中包含点的个数
            if (allPartsStartOffset == shape->mPartsIndexList.size() - 1 || shape->mPartsIndexList[allPartsStartOffset + 1] == 0) {
                // 防止越界
                curPointsNum = pointsNum - curPointsStartOffset;
            } else {
                curPointsNum = shape->mPartsIndexList[allPartsStartOffset + 1] - curPointsStartOffset;
            }
            points.clear();
            points.reserve(curPointsNum);
            //QPainterPath path;
            // 记录上一个点
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

// 绘制多边形
void GShapeLayer::drawPolygon(QPainter* painter, const QRect& rect, const ShapeDraw* shape)
{
    //开始画点的距离
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

// 绘制多边形
void GShapeLayer::drawAddress(QPainter* painter, const QRect& rect, const ShapeDraw* shape)
{
    // 白色半透明
    QBrush brush1(QColor(255, 255, 255, 125), Qt::SolidPattern);
    // 红色
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
    //字符串高度
    const int height = metrics.height();
    auto* view = getView();
    for (int i = 0; i < shape->mAddressList.size(); i++) {
        auto& item = shape->mAddressList[i];
        //字符串宽度
        int width = metrics.width(item.name);
        QPoint point = view->lpTodp({item.x, item.y});

        // 点与文字包围盒子
        int top = point.y() - 10;
        int bottom = point.y() + 6 + height;

        int left = point.x() - width / 2;
        int right = left + width;

        // 判断文本框是否与rect相交
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