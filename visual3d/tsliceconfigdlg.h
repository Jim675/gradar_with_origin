#pragma once

#include <QDialog>
#include "ui_tsliceconfigdlg.h"

class TSliceConfigDlg : public QDialog
{
	Q_OBJECT

private:
	Ui::TSliceConfigDlgClass	ui;
	bool						mIsUpdateUI;

public:
	TSliceConfigDlg(QWidget *parent = nullptr);
	~TSliceConfigDlg();

	void setSliceEnabled(bool enableX, bool enableY, bool enableZ);

	void setSliceRange(int maxX, int maxY, int maxZ);

	void setSlicePos(int ix, int iy, int iz);

signals:

	void sliceEnableChanged(int axis, bool enable);

	void sliceChanged(int axis, int pos);

private slots:

	void on_mXCBox_stateChanged(int state);

	void on_mYCBox_stateChanged(int state);

	void on_mZCBox_stateChanged(int state);

	void on_mXSlider_valueChanged(int value);

	void on_mXSBox_valueChanged(int value);

	void on_mYSlider_valueChanged(int value);

	void on_mYSBox_valueChanged(int value);

	void on_mZSlider_valueChanged(int value);

	void on_mZSBox_valueChanged(int value);
};
