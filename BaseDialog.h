#pragma once
#include <qdialog.h>

class BaseDialog:public QDialog
{
    Q_OBJECT
public:
    BaseDialog(QWidget* parent = nullptr);
    virtual ~BaseDialog();

    // �����ڹر�ʱ�Ƿ��Զ�delete
    void setAutoDelete(bool autoDelete);
};
