#include "gdbfreader.h"

#include <string>
#include <fstream>

#include <QDebug>
#include <qstring.h>
#include <QTextCodec>

using std::ifstream;
using std::ios;

//��ȡdbf�ļ�
Dbf* DbfReader::read(const char* path)
{
    ifstream ifs;
    ifs.open(path, ios::binary | ios::out);
    if (ifs.fail()) {
        ifs.close();
        return nullptr;
    }
    Dbf* dbfHead = DbfReader::readDbfHead(ifs);//���ļ�ͷ
    Dbf* dbf = readDbfRecord(ifs, dbfHead);//�ļ���¼����
    ifs.close();
    return dbf;
}

//��ȡdbf�ļ�ͷ
Dbf* DbfReader::readDbfHead(ifstream& ifs)
{
    Dbf* dbfHead = new Dbf();
    ifs.seekg(4, ios::beg);
    ifs.read((char*)&dbfHead->mRecordNum, sizeof(int));//��ȡ�ļ�ͷ�еļ�¼����
    ifs.read((char*)&dbfHead->mBytesOfFileHead, sizeof(short));//��ȡ�ļ�ͷ���ֽ���
    ifs.read((char*)&dbfHead->mBytesOfEachRecord, sizeof(short));//��ȡһ����¼���ļ�����
    dbfHead->mFieldNum = (dbfHead->mBytesOfFileHead - 33) / 32;//�����ֶ���
    for (int i = 0; i < dbfHead->mFieldNum; i++)//��ȡ�ļ�ͷ�й����ֶε�����
    {
        ifs.seekg(32 + 32 * i, ios::beg);

        dbfHead->dbfField = new DbfField();

        for (int j = 0; j < 11; j++) {//��ȡ�ֶ���
            ifs.read(dbfHead->dbfField->mName + j, sizeof(char));
        }

        ifs.read(&dbfHead->dbfField->mDataType, sizeof(char));//��ȡ�ֶε�����
        ifs.seekg(4, ios::cur);
        ifs.read((char*)&dbfHead->dbfField->mFieldLength, sizeof(char));//��ȡ�ֶεĳ���
        dbfHead->mTable.push_back(dbfHead->dbfField);//����ȡ���ֶ����ݴ���		
    }
    char terminator;
    return dbfHead;
}

//��ȡdbf�ļ���¼����
Dbf* DbfReader::readDbfRecord(ifstream& ifs, Dbf* dbfHead)
{
    ifs.seekg(dbfHead->mBytesOfFileHead, ios::beg);
    char deleteTag;
    for (int i = 0; i < dbfHead->mRecordNum; i++)//��ȡÿһ��������Ϣ
    {
        ifs.read(&deleteTag, sizeof(char));
        for (unsigned int j = 0; j < dbfHead->mTable.size(); j++)//��ȡÿһ���ֶε���Ϣ
        {
            const size_t len = dbfHead->mTable[j]->mFieldLength;
            //char* record = new char[len] {0};
            string record(len, 0);
            for (int k = 0; k < len; k++) {
                ifs.read(&record[k], sizeof(char));
            }
            dbfHead->mTable[j]->mFieldList.push_back(std::move(record));
        }
    }
    return dbfHead;
}