#include "gshapereader.h"

#include <fstream>
#include <string>

#include <qdebug.h>
#include <QString.h>

using std::ifstream;
using std::ios;
using std::wstring;


//��ȡ.shp�ļ�
//Shape* ShapeReader::read(const char* path)
Shape* ShapeReader::read(const QString& path)
{
    int index = path.lastIndexOf('.');
    if (index == -1) {
        return nullptr;
    }
    const QString suffix = path.mid(index + 1, path.size() - index - 1);
    if (suffix != QString("shp")) {//����Ϊ.shp�ļ���ֱ�ӷ��أ���������һ������;
        return nullptr;
    }

    ifstream ifs;
    ifs.open(path.toStdString(), ios::binary | ios::out);
    int shpType = 1;
    //��ȡShp�ļ�ͷ
    if (!ifs) {
        qDebug() << "Open Failed" << endl;
        return nullptr;
    } else {
        shpType = ShapeReader::readFileHead(ifs);
    }

    Shape* shape = nullptr;
    switch (shpType) {
        case Shape::TYPE_POINT:
            shape = ShapeReader::readPoint(ifs);
            break;
        case Shape::TYPE_LINE:
            shape = ShapeReader::readLine(ifs);
            break;
        case Shape::TYPE_POLYGON:
            shape = ShapeReader::readPolygon(ifs);
            break;
    }
    ifs.close();
    return shape;
}


//��ȡ�ļ�ͷ
int ShapeReader::readFileHead(ifstream& ifs)
{

    ShapeFileHead rs;
    ifs.seekg(0, ios::beg);
    ifs.read((char*)&rs.mfilecode, 4);
    for (int i = 0; i < 5; i++) {
        ifs.read((char*)&rs.munused[i], 4);
    }
    ifs.read((char*)&rs.mfilelength, 4);
    ifs.read((char*)&rs.mversion, 4);
    ifs.read((char*)&rs.mShapeType, 4);
    ifs.read((char*)&rs.mxmin, 8);
    ifs.read((char*)&rs.mymin, 8);
    ifs.read((char*)&rs.mxmax, 8);
    ifs.read((char*)&rs.mymax, 8);
    ifs.read((char*)&rs.mzmin, 8);
    ifs.read((char*)&rs.mzmax, 8);
    ifs.read((char*)&rs.mmmin, 8);
    ifs.read((char*)&rs.mmmax, 8);
    return rs.mShapeType;
}

//��ȡ��״Ŀ��
Shape* ShapeReader::readPoint(ifstream& ifs)
{
    Shape* shape = new Shape();
    ifs.seekg(100, ios::beg);
    while (!ifs.eof()) {
        //ifs.seekg(12, ios::cur);
        ifs.read((char*)&shape->mrecordnumber, 4);
        ifs.read((char*)&shape->mcontentlength, 4);
        ifs.read((char*)&shape->mShapeType, 4);
        double x, y;//�ݴ� X Y
        ifs.read((char*)&x, 8);
        ifs.read((char*)&y, 8);
        shape->mPoints.append({x,y});
    }
    return shape;
}

//��ȡ��״Ŀ��
Shape* ShapeReader::readLine(ifstream& ifs)
{
    Shape* shape = new Shape();
    ifs.seekg(100, ios::beg);
    while (!ifs.eof()) {
        ifs.read((char*)&shape->mrecordnumber, 4);
        ifs.read((char*)&shape->mcontentlength, 4);
        ifs.read((char*)&shape->mShapeType, 4);
        ifs.read((char*)&shape->mbox[0], 8);
        ifs.read((char*)&shape->mbox[1], 8);
        ifs.read((char*)&shape->mbox[2], 8);
        ifs.read((char*)&shape->mbox[3], 8);
        ifs.read((char*)&shape->mPartsNum, 4);
        ifs.read((char*)&shape->mPointsNum, 4);
        shape->mRecordParts.append(shape->mPartsNum);//1
        shape->mRecordPoints.append(shape->mPointsNum);//2
        int nIndex = 0;
        for (int i = 0; i < shape->mPartsNum; i++) {
            ifs.read((char*)&nIndex, 4);
            shape->mPartsIndexList.push_back(nIndex);//3
        }
        double x;
        double y;
        for (int i = 0; i < shape->mPointsNum; i++) {
            ifs.read((char*)&x, 8);
            ifs.read((char*)&y, 8);
            shape->mPoints.append({x,y});//4
        }
    }
    return shape;
}

//��ȡ��״Ŀ��
Shape* ShapeReader::readPolygon(ifstream& ifs)
{
    Shape* shape = new Shape();
    ifs.seekg(100, ios::beg);
    while (!ifs.eof()) {
        ifs.read((char*)&shape->mrecordnumber, 4);
        ifs.read((char*)&shape->mcontentlength, 4);
        ifs.read((char*)&shape->mShapeType, 4);
        ifs.read((char*)&shape->mbox[0], 8);
        ifs.read((char*)&shape->mbox[1], 8);
        ifs.read((char*)&shape->mbox[2], 8);
        ifs.read((char*)&shape->mbox[3], 8);
        ifs.read((char*)&shape->mPartsNum, 4);
        shape->mRecordParts.append(shape->mPartsNum);
        ifs.read((char*)&shape->mPointsNum, 4);
        shape->mRecordPoints.append(shape->mPointsNum);
        int nIndex = 0;
        for (int i = 0; i < shape->mPartsNum; i++) {
            ifs.read((char*)&nIndex, 4);
            shape->mPartsIndexList.push_back(nIndex);
        }
        double x;
        double y;
        for (int i = 0; i < shape->mPointsNum; i++) {
            ifs.read((char*)&x, 8);
            ifs.read((char*)&y, 8);
            shape->mPoints.append({x,y});
        }
    }
    return shape;
}

QVector<QPolygonF> Shape::getPolygonList() const
{
    QVector<QPolygonF> list;
    if (mShapeType != TYPE_POLYGON) {
        return list;
    }

    // ��ǰ�������л���������ʼƫ��
    int allPartsStartOffset = 0;
    // ��ǰ����ʼ�������е���������ʼƫ��
    int allPointsStartOffset = 0;

    for (int i = 0; i < mRecordParts.size() - 1; i++)//��ÿ����¼���ݽ��д���
    {
        int partsNum = mRecordParts[i];//��õ�ǰ��״Ŀ���ӻ�����
        int pointsNum = mRecordPoints[i];//��ù��ɵ�ǰ�ӻ��ĵ�ĸ���

        int allPartsEndIndex = allPartsStartOffset + partsNum;
        for (; allPartsStartOffset < allPartsEndIndex; allPartsStartOffset++) {
            // ����ÿ����
            // ��ǰ������ʼ���ڵ�ǰ����ӵ�еĵ������ƫ��
            int curPointsStartOffset = mPartsIndexList[allPartsStartOffset];
            int curPointsNum = 0; // ��ǰ���а�����ĸ���
            if (allPartsStartOffset == mPartsIndexList.size() - 1 || mPartsIndexList[allPartsStartOffset + 1] == 0) {
                curPointsNum = pointsNum - curPointsStartOffset;
            } else {
                curPointsNum = mPartsIndexList[allPartsStartOffset + 1] - curPointsStartOffset;
            }
            QPolygonF polygon;
            for (int j = 0; j < curPointsNum; j++) {
                polygon.append(mPoints[allPointsStartOffset + j]);
            }
            if (polygon.size() > 0) {
                list.append(std::move(polygon));
            }
            allPointsStartOffset += curPointsNum;
        }
    }
    return list;
}
