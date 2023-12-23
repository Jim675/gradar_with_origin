#pragma once
#include <qdialog.h>

class BaseDialog:public QDialog
{
    Q_OBJECT
public:
    BaseDialog(QWidget* parent = nullptr);
    virtual ~BaseDialog();

    // 设置在关闭时是否自动delete
    void setAutoDelete(bool autoDelete);
};
