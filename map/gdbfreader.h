#pragma once

#include <string>
#include <fstream>

#include <qpoint.h>
#include <qvector.h>
#include <qstringlist.h>
#include <qstring.h>

using std::string;
using std::vector;
using std::ifstream;

class DbfField
{
public:
    char mName[11] = {0};//�ֶε�����
    char mDataType = 0;//�ֶε�����
    unsigned char mFieldLength = 0;//�ֶεĳ���
    char mFieldContent[100];//�ֶε�����
    vector<string> mFieldList;//�洢�ֶε�����
};

class Dbf
{
public:
    int mRecordNum = 0; //dbf�ļ��еļ�¼��������������
    int mFieldNum = 0; //dbf�ļ��е��ֶ�������������
    short mBytesOfFileHead = 0; //dbf�ļ�ͷ�е��ֽ���
    short mBytesOfEachRecord = 0; //ÿһ����¼���ֽ���
    DbfField* dbfField = nullptr; //����ĳ���ֶ�
    vector<DbfField*> mTable;//�洢�����ֶε�����

    ~Dbf()
    {
        for (auto item : mTable) {
            delete item;
        }
    }
};

class DbfReader
{
public:
    //��ȡdbf�ļ�
    static Dbf* read(const char* path);
private:
    //��ȡdbf�ļ�ͷ
    static Dbf* readDbfHead(ifstream& ifs);
    //��ȡdbf��¼����
    static Dbf* readDbfRecord(ifstream& ifs, Dbf* dbfHead);
};


