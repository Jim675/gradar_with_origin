#pragma once

#include <QDialog>
#include "ui_tpredictdlg.h"

class TPredictDlg : public QDialog
{
	Q_OBJECT

public:
	TPredictDlg(const std::vector<std::string>& model_list, QWidget *parent = nullptr);
	~TPredictDlg();

	void getInfo(int& chooseindex,int& predictnum);
	void setInfo();

	int mChooseIndex = 0;
	int mPredictNum = 0;
private:
	Ui::tpredictdlgClass ui;


private slots:
	void on_mOkBtn_clicked();

	void on_mCancelBtn_clicked();
};
