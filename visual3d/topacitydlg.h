#pragma once

#include <QDialog>
#include "ui_topacitydlg.h"

class GVolumn;

class TOpacityDlg : public QDialog
{
	Q_OBJECT

public:
	TOpacityDlg(QWidget *parent = Q_NULLPTR);
	~TOpacityDlg();

	void setOpacityTable(const GOpacityTable& table);

	const GOpacityTable& getOpacityTable();

private:
	Ui::TOpacityDlg			ui;
	bool					mIsUpdateUI;

signals:

	void valueChanged(int type);

private slots:

	void on_mFuncTypeCBox_currentIndexChanged(int index);

	void on_mCbClamping_clicked(bool checked);

	void on_mOkBtn_clicked();

	void on_mCancelBtn_clicked();

	void opacityViewValueChanged(int type);
};
