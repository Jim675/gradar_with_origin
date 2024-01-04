#pragma once

#include <QDialog>
#include "ui_tgriddingconfigdlg.h"

class TGriddingConfigDlg : public QDialog
{
	Q_OBJECT

public:
	TGriddingConfigDlg(QWidget *parent = nullptr);
	~TGriddingConfigDlg();

	// ?????????
	void setRect(const QRectF& rect);

private:
	Ui::TGriddingConfigDlgClass ui;

private slots:

	void on_mOkBtn_clicked();

	void on_mCancelBtn_clicked();
};
