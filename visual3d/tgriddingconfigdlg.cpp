#include "tgriddingconfigdlg.h"
#include "gmapcoordconvert.h"
#include "gconfig.h"

TGriddingConfigDlg::TGriddingConfigDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.mNXSBox->setValue(GConfig::mGriddingConfig.mNX);
	ui.mNYSBox->setValue(GConfig::mGriddingConfig.mNY);
	ui.mNZSBox->setValue(GConfig::mGriddingConfig.mNZ);
}

TGriddingConfigDlg::~TGriddingConfigDlg()
{}

void TGriddingConfigDlg::setRect(const QRectF& rect)
{
	QRectF lonLat = GMapCoordConvert::mercatorToLonLat(rect);

	ui.mLon0SBox->setValue(lonLat.left());
	ui.mLon1SBox->setValue(lonLat.right());

	ui.mLat0SBox->setValue(lonLat.top());
	ui.mLat1SBox->setValue(lonLat.bottom());

	ui.mX0SBox->setValue(rect.left());
	ui.mX1SBox->setValue(rect.right());

	ui.mY0SBox->setValue(rect.top());
	ui.mY1SBox->setValue(rect.bottom());
}

void TGriddingConfigDlg::on_mOkBtn_clicked()
{
	GConfig::mGriddingConfig.mNX = ui.mNXSBox->value();
	GConfig::mGriddingConfig.mNY = ui.mNYSBox->value();
	GConfig::mGriddingConfig.mNZ = ui.mNZSBox->value();
	accept();
}

void TGriddingConfigDlg::on_mCancelBtn_clicked()
{
	reject();
}
