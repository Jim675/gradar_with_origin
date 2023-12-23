#include "trenderconfigdlg.h"
#include "gconfig.h"
#include "topacitydlg.h"
#include "gvisualwidget.h"

TRenderConfigDlg::TRenderConfigDlg(QWidget *parent)
	: QDialog(parent)
{
	//setAttribute(Qt::WA_DeleteOnClose);
	ui.setupUi(this);

	mpVisualWidget = NULL;

	mIsLockUI = true;
	const GRenderConfig& config = GConfig::mRenderConfig;

	ui.mZScaleSBox->setValue(config.mZScale);
	ui.mAutoSampCBox->setChecked(config.mAutoSampleDistance);
	ui.mSampDistSBox->setValue(config.mSampleDistance);
	ui.mSampDistSBox->setEnabled(!config.mAutoSampleDistance);
	mOpacityTable = config.mOpacityTable;
	ui.mInterpMethodCBox->setCurrentIndex(config.mInterpolationMethod);
	ui.mUseJitterCBox->setChecked(config.mUseJittering);

	ui.mVolShadeCBox->setChecked(config.mVolumeShade);
	ui.mVolAmbientSBox->setValue(config.mVolumeAmbient);
	ui.mVolAmbientSBox->setEnabled(config.mVolumeShade);
	ui.mVolDiffuseSBox->setValue(config.mVolumeDiffuse);
	ui.mVolDiffuseSBox->setEnabled(config.mVolumeShade);
	ui.mVolSpecularSBox->setValue(config.mVolumeSpecular);
	ui.mVolSpecularSBox->setEnabled(config.mVolumeShade);
	ui.mVolSpecularPowerSBox->setValue(config.mVolumeSpecularPower);
	ui.mVolSpecularPowerSBox->setEnabled(config.mVolumeShade);

	ui.mTerrainShadeCBox->setChecked(config.mTerrainShade);
	ui.mTerrainAmbientSBox->setValue(config.mTerrainAmbient);
	ui.mTerrainAmbientSBox->setEnabled(config.mTerrainShade);
	ui.mTerrainDiffuseSBox->setValue(config.mTerrainDiffuse);
	ui.mTerrainDiffuseSBox->setEnabled(config.mTerrainShade);
	ui.mTerrainSpecularSBox->setValue(config.mTerrainSpecular);
	ui.mTerrainSpecularSBox->setEnabled(config.mTerrainShade);
	ui.mTerrainSpecularPowerSBox->setValue(config.mTerrainSpecularPower);
	ui.mTerrainSpecularPowerSBox->setEnabled(config.mTerrainShade);

	ui.mCrBarWidthScaleSBox->setValue(config.mScalarBarWidthRate);
	ui.mCrBarHeightScaleSBox->setValue(config.mScalarBarHeightRate);
	ui.mCrBarMaxWidthSBox->setValue(config.mScalarBarMaxWidthPixels);
	ui.mCrBarLabelCountSBox->setValue(config.mScalarBarLabelsCount);
	//ui.mAxesFontSizeSBox->setValue(config.mCubeAxesFontSize);
	ui.mShowCoordCBox->setChecked(config.mCubeAxesVisibility);
	mIsLockUI = false;

	mOpacityDlg = new TOpacityDlg(this);
	mOpacityDlg->setOpacityTable(mOpacityTable);
	connect(mOpacityDlg, SIGNAL(valueChanged(int)), this, SLOT(opacityValueChanged(int)));
}

TRenderConfigDlg::~TRenderConfigDlg()
{
	delete mOpacityDlg;
}


void TRenderConfigDlg::setVisualWidget(GVisualWidget* pVisualWidget)
{
	mpVisualWidget = pVisualWidget;
}

void TRenderConfigDlg::updateData()
{
	GRenderConfig& config = GConfig::mRenderConfig;

	config.mZScale = ui.mZScaleSBox->value();
	config.mAutoSampleDistance = ui.mAutoSampCBox->isChecked();
	config.mSampleDistance = ui.mSampDistSBox->value();
	config.mOpacityTable = mOpacityTable;
	config.mInterpolationMethod = ui.mInterpMethodCBox->currentIndex();
	config.mUseJittering = ui.mUseJitterCBox->isChecked();

	config.mVolumeShade = ui.mVolShadeCBox->isChecked();
	config.mVolumeAmbient = ui.mVolAmbientSBox->value();
	config.mVolumeDiffuse = ui.mVolDiffuseSBox->value();
	config.mVolumeSpecular = ui.mVolSpecularSBox->value();
	config.mVolumeSpecularPower = ui.mVolSpecularPowerSBox->value();

	config.mTerrainShade = ui.mTerrainShadeCBox->isChecked();
	config.mTerrainAmbient = ui.mTerrainAmbientSBox->value();
	config.mTerrainDiffuse = ui.mTerrainDiffuseSBox->value();
	config.mTerrainSpecular = ui.mTerrainSpecularSBox->value();
	config.mTerrainSpecularPower = ui.mTerrainSpecularPowerSBox->value();

	config.mScalarBarWidthRate = ui.mCrBarWidthScaleSBox->value();
	config.mScalarBarHeightRate = ui.mCrBarHeightScaleSBox->value();
	config.mScalarBarMaxWidthPixels = ui.mCrBarMaxWidthSBox->value();
	config.mScalarBarLabelsCount = ui.mCrBarLabelCountSBox->value();
	config.mCubeAxesVisibility = ui.mShowCoordCBox->isChecked();
	//config.mCubeAxesFontSize = ui.mAxesFontSizeSBox->value();
}

void TRenderConfigDlg::applyUpdate()
{
	if (mIsLockUI) return; 

	updateData();
	GVisualWidget* pVisualWidget = (GVisualWidget*)parentWidget();
	mpVisualWidget->applyConfig(GConfig::mRenderConfig);
	mpVisualWidget->renderScene();
}

void TRenderConfigDlg::on_mZScaleSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mSampDistSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mAutoSampCBox_stateChanged(int state)
{
	ui.mSampDistSBox->setEnabled(!ui.mAutoSampCBox->isChecked());
	applyUpdate();
}

void TRenderConfigDlg::on_mInterpMethodCBox_currentIndexChanged(int)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mVolShadeCBox_stateChanged(int state)
{
	ui.mVolAmbientSBox->setEnabled(ui.mVolShadeCBox->isChecked());
	ui.mVolDiffuseSBox->setEnabled(ui.mVolShadeCBox->isChecked());
	ui.mVolSpecularSBox->setEnabled(ui.mVolShadeCBox->isChecked());
	ui.mVolSpecularPowerSBox->setEnabled(ui.mVolShadeCBox->isChecked());
	applyUpdate();
}

void TRenderConfigDlg::on_mVolAmbientSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mVolDiffuseSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mVolSpecularSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mVolSpecularPowerSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mTerrainAmbientSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mTerrainDiffuseSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mTerrainSpecularSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mTerrainSpecularPowerSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mTerrainShadeCBox_stateChanged(int state)
{
	ui.mTerrainAmbientSBox->setEnabled(ui.mTerrainShadeCBox->isChecked());
	ui.mTerrainDiffuseSBox->setEnabled(ui.mTerrainShadeCBox->isChecked());
	ui.mTerrainSpecularSBox->setEnabled(ui.mTerrainShadeCBox->isChecked());
	ui.mTerrainSpecularPowerSBox->setEnabled(ui.mTerrainShadeCBox->isChecked());
	applyUpdate();
}

void TRenderConfigDlg::on_mCrBarWidthScaleSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mCrBarHeightScaleSBox_valueChanged(double)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mCrBarMaxWidthSBox_valueChanged(int)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mCrBarLabelCountSBox_valueChanged(int)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mAxesFontSizeSBox_valueChanged(int)
{
	applyUpdate();
}

void TRenderConfigDlg::on_mShowCoordCBox_stateChanged(int state)
{
	applyUpdate();
}

void TRenderConfigDlg::opacityValueChanged(int)
{
	mOpacityTable = mOpacityDlg->getOpacityTable();
	applyUpdate();
}

void TRenderConfigDlg::on_mOpacityBtn_clicked()
{
	mOpacityDlg->setOpacityTable(mOpacityTable);
	mOpacityDlg->exec();
	/*if (opacityDlg.exec() == QDialog::Accepted)
	{
		mOpacityTable = opacityDlg.getOpacityTable();
	}*/
}

void TRenderConfigDlg::on_mOkBtn_clicked()
{
	
	//accept();
}

void TRenderConfigDlg::on_mCancelBtn_clicked()
{
	reject();
}

void TRenderConfigDlg::on_mSaveBtn_clicked()
{
	updateData();
	GConfig::mRenderConfig.saveToIni("./config/render.ini");
	GConfig::mRenderConfig.mOpacityTable.saveToIni("./config/opacity.ini");
}

