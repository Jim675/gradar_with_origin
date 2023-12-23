#pragma once

#include <QDialog>
#include "ui_tanimationconfigdlg.h"

// 二维动画参数设置对话框
class TAnimationConfigDlg : public QDialog
{
	Q_OBJECT

private:
	Ui::TAnimationConfigDlgClass ui;

public:
	TAnimationConfigDlg(QWidget *parent = nullptr);
	~TAnimationConfigDlg();

	int getInterval();

	bool isSkipEmpty();

private slots:

	void on_mOkBtn_clicked();

	void on_mCancelBtn_clicked();
};
