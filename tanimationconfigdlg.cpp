#include "tanimationconfigdlg.h"
#include "gconfig.h"

TAnimationConfigDlg::TAnimationConfigDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.mIntervalSBox->setValue(GConfig::mAnimationConfig.mInterval);
	ui.mSkipCBox->setChecked(GConfig::mAnimationConfig.mIsSkipEmpty);
}

TAnimationConfigDlg::~TAnimationConfigDlg()
{}

int TAnimationConfigDlg::getInterval()
{
	return ui.mIntervalSBox->value();
}

bool TAnimationConfigDlg::isSkipEmpty()
{
	return ui.mSkipCBox->isChecked();
}

void TAnimationConfigDlg::on_mOkBtn_clicked()
{
	GConfig::mAnimationConfig.mInterval = ui.mIntervalSBox->value();
	GConfig::mAnimationConfig.mIsSkipEmpty = ui.mSkipCBox->isChecked();

	accept();
}

void TAnimationConfigDlg::on_mCancelBtn_clicked()
{
	reject();
}
