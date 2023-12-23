#pragma once

#include <QDialog>
#include "ui_trenderconfigdlg.h"
#include "gopacitytable.h"

class GVisualWidget;
class TOpacityDlg;

class TRenderConfigDlg : public QDialog
{
	Q_OBJECT

private:
	Ui::TRenderConfigDlgClass	ui;
	bool						mIsLockUI;
	GVisualWidget *				mpVisualWidget;
	GOpacityTable				mOpacityTable;
	TOpacityDlg *				mOpacityDlg;

protected:
	void updateData();

	void applyUpdate();

public:
	TRenderConfigDlg(QWidget *parent = nullptr);
	~TRenderConfigDlg();

	void setVisualWidget(GVisualWidget* pVisualWidget);

private slots:

	void on_mZScaleSBox_valueChanged(double);

	void on_mSampDistSBox_valueChanged(double);

	void on_mAutoSampCBox_stateChanged(int state);

	void on_mInterpMethodCBox_currentIndexChanged(int);

	void on_mVolShadeCBox_stateChanged(int state);

	void on_mVolAmbientSBox_valueChanged(double);

	void on_mVolDiffuseSBox_valueChanged(double);

	void on_mVolSpecularSBox_valueChanged(double);

	void on_mVolSpecularPowerSBox_valueChanged(double);

	void on_mTerrainAmbientSBox_valueChanged(double);

	void on_mTerrainDiffuseSBox_valueChanged(double);

	void on_mTerrainSpecularSBox_valueChanged(double);

	void on_mTerrainSpecularPowerSBox_valueChanged(double);

	void on_mTerrainShadeCBox_stateChanged(int state);

	void on_mCrBarWidthScaleSBox_valueChanged(double);

	void on_mCrBarHeightScaleSBox_valueChanged(double);

	void on_mCrBarMaxWidthSBox_valueChanged(int);

	void on_mCrBarLabelCountSBox_valueChanged(int);

	void on_mAxesFontSizeSBox_valueChanged(int);

	void on_mShowCoordCBox_stateChanged(int state);

	void opacityValueChanged(int);

	void on_mOpacityBtn_clicked();
	
	void on_mOkBtn_clicked();

	void on_mCancelBtn_clicked();

	void on_mSaveBtn_clicked();
};
