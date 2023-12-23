#include "topacitydlg.h"

TOpacityDlg::TOpacityDlg(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    mIsUpdateUI = false;

    connect(ui.mOpacityView, SIGNAL(valueChanged(int)), this, SLOT(opacityViewValueChanged(int)));
}

TOpacityDlg::~TOpacityDlg()
{
}

void TOpacityDlg::setOpacityTable(const GOpacityTable& table)
{
    ui.mOpacityView->setOpacityTable(table);
    ui.mFuncTypeCBox->setCurrentIndex(table.getFuncType());
    //ui.mCbClamping->setChecked(table.getClamping());
}

const GOpacityTable& TOpacityDlg::getOpacityTable()
{
    return ui.mOpacityView->getOpacityTable();
}

void TOpacityDlg::on_mFuncTypeCBox_currentIndexChanged(int index)
{
    if (mIsUpdateUI) return;

    ui.mOpacityView->setFuncType(index);
    emit valueChanged(0);
}

void TOpacityDlg::on_mCbClamping_clicked(bool checked)
{
    if (mIsUpdateUI) return;

    ui.mOpacityView->setClamping(checked);
    emit valueChanged(0);
}

void TOpacityDlg::on_mOkBtn_clicked()
{
    accept();
}

void TOpacityDlg::on_mCancelBtn_clicked()
{
    reject();
}

void TOpacityDlg::opacityViewValueChanged(int type)
{
    emit valueChanged(type);
}
