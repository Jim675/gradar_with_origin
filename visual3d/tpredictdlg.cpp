#include "tpredictdlg.h"
#include "qdebug.h"
TPredictDlg::TPredictDlg(const std::vector<std::string>& model_list, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	ui.spinBox->setValue(5);
	ui.spinBox->setMaximum(1);
	ui.spinBox->setMaximum(50);
	ui.spinBox->setSingleStep(1);
	this->setWindowTitle(QString::fromLocal8Bit("Ԥ�����ô���"));
	for (auto model : model_list) {
		ui.comboBox->addItem(model.c_str());
	}
	ui.comboBox->setCurrentIndex(0);

	connect(ui.pushButtonOK, &QPushButton::clicked, this, &TPredictDlg::on_mOkBtn_clicked);
	connect(ui.pushButtonCancel, &QPushButton::clicked, this, &TPredictDlg::on_mCancelBtn_clicked);
}
void TPredictDlg::setInfo()
{
	mPredictNum = ui.spinBox->value();
	mChooseIndex = ui.comboBox->currentIndex();
	qDebug() << "����ˣ�" << ui.comboBox->itemText(mChooseIndex);
}
void TPredictDlg::getInfo(int& chooseindex, int& predictnum)
{
	chooseindex = mChooseIndex;
	predictnum = mPredictNum;
}
void TPredictDlg::on_mOkBtn_clicked()
{
	setInfo();
	accept();
}

void TPredictDlg::on_mCancelBtn_clicked()
{
	reject();
}
TPredictDlg::~TPredictDlg()
{}
