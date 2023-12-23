#include "BaseDialog.h"

BaseDialog::BaseDialog(QWidget* parent):QDialog(parent)
{
    // 去掉右上角问号
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, false);
}

BaseDialog::~BaseDialog()
{

}


// 设置在关闭时是否自动delete
void BaseDialog::setAutoDelete(bool autoDelete)
{
    setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, autoDelete);
}