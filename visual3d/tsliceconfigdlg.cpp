#include "tsliceconfigdlg.h"

TSliceConfigDlg::TSliceConfigDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	mIsUpdateUI = false;
}

TSliceConfigDlg::~TSliceConfigDlg()
{}

void TSliceConfigDlg::setSliceEnabled(bool enableX, bool enableY, bool enableZ)
{
	mIsUpdateUI = true;
	ui.mXCBox->setChecked(enableX);
	ui.mYCBox->setChecked(enableY);
	ui.mZCBox->setChecked(enableZ);
	mIsUpdateUI = false;
}

void TSliceConfigDlg::setSliceRange(int maxX, int maxY, int maxZ)
{
	mIsUpdateUI = true;
	ui.mXSlider->setMinimum(1);
	ui.mXSlider->setMaximum(maxX + 1);
	ui.mXSBox->setMinimum(1);
	ui.mXSBox->setMaximum(maxX + 1);

	ui.mYSlider->setMinimum(1);
	ui.mYSlider->setMaximum(maxY + 1);
	ui.mYSBox->setMinimum(1);
	ui.mYSBox->setMaximum(maxY + 1);

	ui.mZSlider->setMinimum(1);
	ui.mZSlider->setMaximum(maxZ + 1);
	ui.mZSBox->setMinimum(1);
	ui.mZSBox->setMaximum(maxZ + 1);
	mIsUpdateUI = false;
}

void TSliceConfigDlg::setSlicePos(int ix, int iy, int iz)
{
	mIsUpdateUI = true;
	ui.mXSBox->setValue(ix + 1);
	ui.mXSlider->setValue(ix + 1);
	ui.mYSBox->setValue(iy + 1);
	ui.mYSlider->setValue(iy + 1);
	ui.mZSBox->setValue(iz + 1);
	ui.mZSlider->setValue(iz + 1);
	mIsUpdateUI = false;
}

void TSliceConfigDlg::on_mXCBox_stateChanged(int state)
{
	if (mIsUpdateUI) return;

	bool isChecked = state == Qt::Checked;

	ui.mXSlider->setEnabled(isChecked);
	ui.mXSBox->setEnabled(isChecked);

	emit sliceEnableChanged(0, isChecked);
}

void TSliceConfigDlg::on_mYCBox_stateChanged(int state)
{
	if (mIsUpdateUI) return;

	bool isChecked = state == Qt::Checked;

	ui.mYSlider->setEnabled(isChecked);
	ui.mYSBox->setEnabled(isChecked);

	emit sliceEnableChanged(1, isChecked);
}

void TSliceConfigDlg::on_mZCBox_stateChanged(int state)
{
	if (mIsUpdateUI) return;

	bool isChecked = state == Qt::Checked;

	ui.mZSlider->setEnabled(isChecked);
	ui.mZSBox->setEnabled(isChecked);

	emit sliceEnableChanged(2, isChecked);
}

void TSliceConfigDlg::on_mXSlider_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mXSBox->setValue(value);
	emit sliceChanged(0, value - 1);
}

void TSliceConfigDlg::on_mXSBox_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mXSlider->setValue(value);
}

void TSliceConfigDlg::on_mYSlider_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mYSBox->setValue(value);
	emit sliceChanged(1, value - 1);
}

void TSliceConfigDlg::on_mYSBox_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mYSlider->setValue(value);
}

void TSliceConfigDlg::on_mZSlider_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mZSBox->setValue(value);
	emit sliceChanged(2, value - 1);
}

void TSliceConfigDlg::on_mZSBox_valueChanged(int value)
{
	if (mIsUpdateUI) return;

	ui.mZSlider->setValue(value);
}
