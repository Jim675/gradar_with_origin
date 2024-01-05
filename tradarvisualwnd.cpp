#include "tradarvisualwnd.h"

#include <QLabel>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox> 
#include <QInputDialog>
#include <QDesktopWidget>
#include <QSettings>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QVector>
#include <QDateTime>
#include <QRadioButton>
#include <QInputDialog>
#include <thread>
#include "gmapview.h"
#include "gmaplayerset.h"
#include "ggpscorrect.h"
#include "gmaptile.h"
#include "gutils.h"
#include "gmapdownloader.h"
#include "grader2dlayer.h"
#include "gshapelayer.h"
#include "tgriddingconfigdlg.h"
#include "tanimationconfigdlg.h"
#include "gconfig.h"
#include "gradaralgorithm.h"
#include "gvtkutil.h"
#include "tradar3dwnd.h"
#include "tpredictdlg.h"
//#include "gpredstart.h"
#include "gpuinfo.h"
#include <iostream>
#include <QTime>
#include <filesystem>
// TODO(yx): use your predrnn
//#include "gpredrnn.h"
//#include "gpredrnn.h"
#include "qdebug.h"
#include <qdir.h>
#include <QImage>
#include <QVector>
#include "gpredrnn.h"

//  ��ҳ��Ĵ�С,��ʾ,����,��ť�ĳ�ʼ����
TRadarVisualWnd::TRadarVisualWnd(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags)
{
    ui.setupUi(this);

    // ��ɫ���亯��
	mColorTF = vtkColorTransferFunction::New();
	if (!VTKUtil::readColorMap(R"(./config/cinrad.cs)", "dBZ", mColorTF))
	{
        VTKUtil::setDefaultColorMap(mColorTF);
	}
    double crRange[2];
    mColorTF->GetRange(crRange);

    GConfig::mRenderConfig.loadFromIni("./config/render.ini");
    GConfig::mRenderConfig.mOpacityTable.loadFromIni("./config/opacity.ini");
    GConfig::mRenderConfig.mOpacityTable.setValueRange(crRange[0], crRange[1]);

    // �ļ��б�
    ui.mFileListWidget->setAlternatingRowColors(true);
    ui.mFileListDock->setWidget(ui.mFileListWidget);

    // ���ǰ�ť��
    mELBtnGroups = new QButtonGroup(this);
    mELIndex = 0;
    connect(mELBtnGroups, SIGNAL(buttonClicked(int)), this, SLOT(onELButtonClicked(int)));

    // ��ά����ͼ
    mpView = new GMapView(this);
    setCentralWidget(mpView);
    QLabel* infoLabel = new QLabel(this);
    statusBar()->addPermanentWidget(infoLabel);
    mpView->setInfoLabel(infoLabel);
	// �����û����ӷ�Χ
	QRectF worldRect;
    //worldRect.setLeft(0);
    worldRect.setLeft(-20037508.3427892);
	worldRect.setRight(20037508.3427892);
    //worldRect.setTop(0);
    worldRect.setTop(-20037508.3427892);
    //worldRect.setBottom(10000000.0);
    worldRect.setBottom(20037508.3427892);
	mpView->setBoundingRect(worldRect);
	mpView->setScaleGrade(5);
	mpView->setSelectMode(SELECT_RECT);
	// ����ѡ����ο�ص�����
	connect(mpView, &GMapView::onSelectedRect, this, &TRadarVisualWnd::onSelectedRect);
    
    //Ԥ��
    QObject::connect(ui.actionPredict, &QAction::triggered, this, &TRadarVisualWnd::on_actPredict_triggered);
    //ֹͣԤ��
    QObject::connect(ui.actionStop, &QAction::triggered, this, &TRadarVisualWnd::on_actStop_triggered);

    //����
    // QObject::connect(ui.actionSave, &QAction::triggered, this, &TRadarVisualWnd::on_actSave_triggered);
   
    // ���ͼ��, ����ӵ�ͼ���Ȼ���
    // ����״�ͼ��
    mpRaderLayer = new GRader2DLayer();
    mpRaderLayer->setLayerName("Rader2DLayer");
    mpRaderLayer->setRaderDataList(&mRadarList); // �����״��б�
    mpRaderLayer->setColorTransferFunction(mColorTF);
    mpView->layerSet()->addLayer(mpRaderLayer);

    // ���Shapeͼ��
    mpShapeLayer = new GShapeLayer();
    mpShapeLayer->setLayerName("ShapeLayer");
    mpView->layerSet()->addLayer(mpShapeLayer);

    // ����״�ɫ��ͼ��
    mpColorBarLayer = new GColorBarLayer();
    mpColorBarLayer->setLayerName("ColorBarLayer");
    mpColorBarLayer->setColorTransferFunction(mColorTF);
    mpView->layerSet()->addLayer(mpColorBarLayer);

	// �������������״�ͼ���CheckBox
	ui.mInterpCBox->setCheckState(Qt::CheckState::Unchecked);
	ui.statusBar->addPermanentWidget(ui.mInterpCBox);
    ui.mInterpCBox->setChecked(true);
	ui.mRadarCBox->setCheckState(Qt::CheckState::Checked);
	ui.statusBar->addPermanentWidget(ui.mRadarCBox);

    ui.mInterpCBox->setVisible(false);
    ui.mRadarCBox->setVisible(false);

	// Ĭ�Ϲرն���
	mIsAnimate = true;
	this->setAnimate(false);
    // ��ʼ��Timer
    mAnimateTimer.setSingleShot(false);
    connect(&mAnimateTimer, &QTimer::timeout, this, &TRadarVisualWnd::onAnimateTimeout);

    // ���������ڴ�С
    
    QRect desk = QApplication::desktop()->availableGeometry();
    int width = desk.width() * 0.9;
    int height = desk.height() * 0.9;
    int left = (desk.width() - width) / 2;
    int top = (desk.height() - height) / 2;
   
    setGeometry(left, top, width, height);
    //move((desk.width() - this->width()) / 2, (desk.height() - this->height()) / 2);
    mTranslator = new QTranslator;
}

TRadarVisualWnd::~TRadarVisualWnd()
{
    if (mLastRadar3DWnd)
    {
        mLastRadar3DWnd->close();
    }    
    ui.mFileListWidget->clear();
    qDeleteAll(mRadarList);
    mRadarList.clear();
    mRadarPathSet.clear();
}

//  ���������ڴ�С
void TRadarVisualWnd::setDeskTopWidget()
{
    QRect desk = QApplication::desktop()->availableGeometry();
    int width = desk.width() * 0.9;
    int height = desk.height() * 0.9;
    int left = (desk.width() - width) / 2;
    int top = (desk.height() - height) / 2;
    width = 2228;
    height = 1507;
    // int left = 96;
    // int top = 51;
    setGeometry(left, top, width, height);
    move((desk.width() - this->width()) / 2, (desk.height() - this->height()) / 2);
}
//  ����ͼ�ƶ���ָ�����η�Χ������
void TRadarVisualWnd::centerOn(const QRectF& rc)
{
    mpView->centerOn(rc);
}
//  ���ļ�
void TRadarVisualWnd::on_actFileOpen_triggered()
{
    if (mLastRadar3DWnd)
    {
        mLastRadar3DWnd->close();
        mLastRadar3DWnd = nullptr;
    }

	QString path = GConfig::mLastSelectPath;
	QFileDialog dialog;
	dialog.setWindowTitle(QString::fromLocal8Bit("��ѡ���״������ļ�"));
	dialog.setDirectory(path);
	dialog.setFilter(QDir::Filter::Files);
	dialog.setNameFilter(QString::fromLocal8Bit("�״������ļ�(*.AR2)"));
	dialog.setFileMode(QFileDialog::FileMode::ExistingFiles);
	if (dialog.exec() != QDialog::Accepted) return;

	GConfig::mLastSelectPath = dialog.directory().absolutePath();
	QStringList fileList = dialog.selectedFiles();
	if (fileList.isEmpty()) return;

    fileList.sort(Qt::CaseInsensitive);

    on_actClearFileList_triggered();
    // ���ļ��б�
	loadRadarFiles(fileList);
}
//  �����ļ�
void TRadarVisualWnd::on_actFileSave_triggered() {
    int find = -1;
    // ��ȡ��ǰѡ�е������ݵ��״�����������
    //qDebug() << "================================";
    //qDebug() << "currentRow = " << ui.mFileListWidget->currentRow();
    //qDebug() << "================================";
    if (ui.mFileListWidget->currentRow() != -1 && mRadarList.at(ui.mFileListWidget->currentRow())->surfs.size() != 0)
    {
        find = ui.mFileListWidget->currentRow();
    }
    else
    {
        const int code = QMessageBox::critical(this,
            QString::fromLocal8Bit("����"),
            QString::fromLocal8Bit("û���״����ݿ�д�룡"),
            QMessageBox::StandardButton::Abort);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }

    //��ȡ�ϴδ򿪵�·��
    QString path = GConfig::mLastSelectPath;

    //dir_path��Ϊѡ����ļ��еľ���·�����ڶ��β�Ϊ�Ի�����⣬������Ϊ�Ի���򿪺�Ĭ�ϵ�·����
    QString dir_path = QFileDialog::getExistingDirectory(this, "choose directory", path);

    //qDebug() << "====================================";
    //qDebug() << dir_path;
    //qDebug() << find;
    //qDebug() << "====================================";

    // �����ļ�
    saveRadarFile(dir_path, find);
}
//  ����ļ��б�
void TRadarVisualWnd::on_actClearFileList_triggered()
{
    if (mLastRadar3DWnd)
	{
        mLastRadar3DWnd->close();
		mLastRadar3DWnd = nullptr;
	}

	// �����ļ��б�
	mpRaderLayer->clear();
    qDeleteAll(mRadarList);
    mELIndex = 0;
	mRadarList.clear();
    mRadarPathSet.clear();
	ui.mFileListWidget->clear();
	onRaderIndexChange(-1);
}
//  �˳�
void TRadarVisualWnd::on_actExit_triggered()
{
    if (mLastRadar3DWnd)
	{
        mLastRadar3DWnd->close();
		mLastRadar3DWnd = nullptr;
	}

    close();
}
//  ��ʼ���Ŷ���
void TRadarVisualWnd::on_actAnimationStart_triggered()
{
    this->setAnimate(true);
}
//  ֹͣ���Ŷ���
void TRadarVisualWnd::on_actAnimationStop_triggered()
{
    this->setAnimate(false);
}
//  ���ö������Ų���
void TRadarVisualWnd::on_actAnimationSettings_triggered()
{
	TAnimationConfigDlg configDlg;
	configDlg.exec();
}
//  ���ô�����ͼ
void TRadarVisualWnd::on_actViewReset_triggered()
{
	ui.mFileListDock->show();
	ui.mRadarDock->show();
}
//  ��ǰ�ļ��仯
void TRadarVisualWnd::on_mFileListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    // �����ǰ�ڶ����ͺ���
	if (mIsAnimate) return;

	//auto msg = QString("clicked:") + QString::number(index.row());
    int row = ui.mFileListWidget->currentRow();
	mpRaderLayer->setRaderDataIndex(row);
    mpRaderLayer->setSurfaceIndex(mELIndex);
	// ֪ͨ�Լ�
	this->onRaderIndexChange(row);
}
//  ��άͼ���ֵ״̬�仯
void TRadarVisualWnd::on_mInterpCBox_stateChanged(int state)
{
    bool isCheck = state == Qt::Checked;
    mpRaderLayer->setInterpolate(isCheck);
    ui.actInterpRadarData->setChecked(isCheck);
}
//  
void TRadarVisualWnd::on_actInterpRadarData_toggled(bool checked)
{
    ui.mInterpCBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}
//  �״���ʾ״̬�仯
void TRadarVisualWnd::on_mRadarCBox_stateChanged(int state)
{
	ui.mSurfsLayout->invalidate();
	if (mpRaderLayer == nullptr) return;

    bool isCheck = state == Qt::Checked;
	mpRaderLayer->setVisible(isCheck);
	ui.mInterpCBox->setEnabled(isCheck);
    ui.actInterpRadarData->setEnabled(isCheck);
    ui.actShowRadarData->setChecked(isCheck);
	mpView->update();
}

void TRadarVisualWnd::on_actShowRadarData_toggled(bool checked)
{
    ui.mRadarCBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}
//  ����ѡ��仯
void TRadarVisualWnd::onELButtonClicked(int id)
{
    mpRaderLayer->setSurfaceIndex(id);
    mELIndex = id;
}

void TRadarVisualWnd::on_actTestServer_triggered()
{
    
}
//  ѡ����ο�
void TRadarVisualWnd::on_actSelectRect_triggered(bool checked)
{
    if (checked) {
        mpView->setSelectMode(SELECT_RECT);
    } else {
        mpView->setSelectMode(SELECT_NONE);
    }
}
/*
//��ImageDataÿ����벢����
void TRadarVisualWnd::on_actSave_triggered()
{
    //QString rootpath = QFileDialog::getExistingDirectory(this, "ѡ�񱣴�λ��""./", "ͼ���ļ� (*.jpg *.png)");
   
    //int ret=mpRaderLayer->drawElpng(rootpath);
    
    
    const GRadarVolume* data = mpRaderLayer->getCurRaderData();
    if (data == nullptr)
    {
        QMessageBox::information(this, windowTitle(), "���ȼ����״�����!");
        return;
    }
    if (data->surfs.size() <= 0)
    {
        return;
    }

    TGriddingConfigDlg configDlg(this);
    //configDlg.setRect(mktRect);
    if (configDlg.exec() != QDialog::Accepted) return;

    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    // ��������
    radarVisualDlg->setRadarDataList(mpRaderLayer->getRaderDataList());
    radarVisualDlg->setRadarDataIndex(mpRaderLayer->getRaderDataIndex());
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    radarVisualDlg->renderAll();
   
}*/
//  ֹͣԤ��
void TRadarVisualWnd::on_actStop_triggered()
{
    mpRaderLayer->justCenter();
}

void TRadarVisualWnd::on_actCN_triggered() {
    qDebug() << "CN";
    QApplication::instance()->removeTranslator(mTranslator);
    mTranslator->load("zh_CN.qm");
    QApplication::instance()->installTranslator(mTranslator);
    ui.retranslateUi(this);
}

void TRadarVisualWnd::on_actEN_triggered() {
    qDebug() << "EN";
    QApplication::instance()->removeTranslator(mTranslator);
    mTranslator->load("en_US.qm");
    QApplication::instance()->installTranslator(mTranslator);
    ui.retranslateUi(this);
}

std::string trim(std::string str) {
    if (str.empty()) return str;
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::vector<std::string> list_model_files(const std::string& model_root) {
    std::vector<std::string> model_list;
    {
        for (auto f : std::filesystem::directory_iterator(model_root)) {
            std::string name = f.path().filename().string();
            if (name.substr(name.find_last_of(".")) == ".pt") {
                model_list.push_back(name);
                qDebug() << "find model: " << name.c_str();
            }
        }
    }
    return model_list;
}

std::map<std::string, std::string> read_model_parameters(const std::string& config_file_path, bool& has_config_file) {
    std::map<std::string, std::string> parameters;

    qDebug() << "open model config file: " << config_file_path.c_str();

    std::ifstream config;
    config.open(config_file_path);

    if (!config.is_open()) {
        QMessageBox::warning(nullptr, "Error", QString("miss model config file: %1").arg(QString::fromStdString(config_file_path)));
        has_config_file = false;
        return {};
    }

    // read parameter

    char buffer[1024];

    while (config.getline(buffer, 1024)) {
        std::string line(buffer);
        std::string key = line.substr(0, line.find("="));
        std::string value = line.substr(line.find("=") + 1);
        key = trim(key);
        value = trim(value);

        qDebug() << "parameter key: " << key.c_str() << " value: " << value.c_str();
        parameters[key] = value;
    }
    has_config_file = true;
    return parameters;
}
//  Ԥ��
void TRadarVisualWnd::on_actPredict_triggered()
{
    // �ж�GPU�ڴ��Ƿ��㹻
    if (!is_gpu_memory_suitable()) {
        QMessageBox::warning(this, QString::fromLocal8Bit("����"), QString::fromLocal8Bit("�Դ治��"));
        return;
    }
    // ����QT�Ķ�ʱ��
    QTime timer;
    timer.start();
    int oldsize = mRadarList.size();
    // �ж��Ƿ����㹻���״�����
    if (mRadarList.size() < 5)
    {
        QMessageBox::information(this, "��ʾ", QString::fromLocal8Bit("�����ٴ�����״��ļ�"));
        return;
    }
    // ������һ�������ַ���model_root���������ʼ��Ϊ"models"
    const std::string model_root("models");

    // ʹ����C++17���ļ�ϵͳ�������model_root·���Ƿ����
    if (!std::filesystem::exists(std::filesystem::path(model_root))) {
        QMessageBox::warning(this, "Error", "Miss prediction models!");
            return;
    }
    // list_model_files����ҪĿ�����ҵ�model_root·����������չ��Ϊ".pt"���ļ������������ǵ��ļ���
    auto model_list = list_model_files(model_root);
    // ��ʾ�Ի���
    TPredictDlg predictDialog(model_list, this);
    // ��⵽�û��رնԻ�����˳�����
    int code = predictDialog.exec();
    if (code != QDialog::Accepted)
    {
        return;
    }
    // ��ȡ�û�ѡ���ģ��������Ԥ�����
    int predictNum = 0;
    int chooseIndex = 0;
    predictDialog.getInfo(chooseIndex, predictNum);

    if (predictNum > mRadarList.size())
    {
        QMessageBox::information(this, QString::fromLocal8Bit("����"), QString::fromLocal8Bit("�����������״��ļ�������"));
        return;
    }
    if (predictNum > 5)
    {
        QMessageBox::information(this, QString::fromLocal8Bit("����"), QString::fromLocal8Bit("--������Ӧ<=5--"));
        return;
    }
    //setDeskTopWidget();
    //mpRaderLayer->justCenter();
    string path;
    int image_width = 920;
    // ���ָ������С��Ŀ�Ⱥ͸߶�
    int patchsize = 5;
    float maxmin = 1;
    bool predictIn = true;//�Ƿ��ֵ
    bool usesimvp = false;

    // ѡ��Ԥ���ģ��,�ϳɵ�ַ
    path = model_root + "/" + model_list[chooseIndex];
    qDebug() << "model path" << QString::fromStdString(path) << endl;
    // Ԥ�����������ļ�txt��ʽ
    std::string model_name = model_list[chooseIndex];
    std::string config_file_path = model_root + "/" + model_name.substr(0, model_name.find_last_of(".")) + ".txt";

    // ��ȡ�����ļ�,���������õĲ������Ƿ���������ļ�
    bool has_config_file;
    auto parameters = read_model_parameters(config_file_path, has_config_file);
    // ���û�������ļ�,���˳�
    if (!has_config_file) { return; }
    // ����һ������if/else������������ļ���û��ָ������,��ʹ��Ĭ��ֵ
    if (parameters.find("image_width") == parameters.end()) {
        QMessageBox::warning(this, "Warning", QString("parameter image_width is empty, use default value: %1").arg(image_width));
    }
    else {
        image_width = atoi(parameters["image_width"].c_str());
    }

    if (parameters.find("patchsize") == parameters.end()) {
        QMessageBox::warning(this, "Warning", QString("parameter patchsize is empty, use default value: %1").arg(patchsize));
    }
    else {
        patchsize = atoi(parameters["patchsize"].c_str());
    }

    if (parameters.find("maxmin") == parameters.end()) {
        QMessageBox::warning(this, "Warning", QString("parameter maxmin is empty, use default value: %1").arg(maxmin));
    }
    else {
        maxmin = atoi(parameters["maxmin"].c_str());
    }

    if (parameters.find("predictIn") == parameters.end()) {
        QMessageBox::warning(this, "Warning", QString("parameter predictIn is empty, use default value: %1").arg(predictIn));
    }
    else {
        predictIn = parameters["predictIn"] == "true";
    }

    if (parameters.find("usesimvp") == parameters.end()) {
        QMessageBox::warning(this, "Warning", QString("parameter usesimvp is empty, use default value: %1").arg(usesimvp));
    }
    else {
        usesimvp = parameters["usesimvp"] == "true";
    }

    QProgressDialog* dialog = new QProgressDialog(this);
    dialog->setWindowTitle(QString::fromLocal8Bit("����Ԥ��"));
    dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    // dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    dialog->setMinimumDuration(0);
    // dialog->setMinimum(0);
    // dialog->setMaximum(100);
    dialog->setRange(0, 100);
    dialog->setModal(true);
    dialog->show();
    QCoreApplication::processEvents();

    dialog->setValue(0);
    QVector<QVector<QImage>> Images;
    // int ret = mpRaderLayer->savePreInfo(Images, predictNum);
    // int ret = mpRaderLayer->savePreInfo(Images);
    // 920��1840��ֵ

    // ������ʾ�������İٷֱ�
    int progressValue = 0;
    // �����ж��߳��Ƿ�ִ�����
    bool complete = false;

    /** ������һ���µ��̣߳���������ʼִ��,thread����������:
        1. ��ȡ�״�����
        2. Ԥ��
        3. ��ʾԤ����,�����µ��״�������
    **/
    std::thread thread([&]() {
        int ret = 0;
        // mpRaderLayer���״�2Dͼ��
        mpRaderLayer->imgwidth = image_width;
        // Images���״�����ת����2Dͼ����Ҫ��Ԥ����,��һ����ά����,�м����״������ж೤
        if (predictIn) ret = mpRaderLayer->savePreInfoIn(Images); //��ֵ
        else ret = mpRaderLayer->savePreInfo(Images); //����ֵ

        //  mpRaderLayer->setPreImage(Images[0]);
        //  mpRaderLayer->convertdptolp();
        progressValue = 10;
        //  dialog->setValue(10);

        auto pred = IPredict::load(path);

        pred->setProperties(predictNum, image_width, patchsize, maxmin);

        // res[�״�][׶��],����������Ÿ�׶����״������
        QVector<QVector<QImage>> res = pred->predict(Images, usesimvp);

        progressValue = 30;
        //  dialog->setValue(30);
        //  dialog->setWindowTitle("���ݲ�ֵ����");

        QString fileName = "Predict";

        /**
            1. predictNum���û��������Ԥ������ɵ�Ԥ�����ĸ���,�����5,��СҪ����������״����
            2. ϵͳ��ǿ��Ҫ����ص��״����������,����predictNum��С�ڵ�����
            3. Ԥ��Ľ�����Ƿ�������������Ÿ�׶����״������
        **/
        int space = 70 / predictNum;
        for (int i = 0; i < predictNum; i++)
        {
            //  ��ֵ
            //  progressValue = 40;
            //  Ԥ������������״��ͬһ��׶����Ԥ��!!!,���洫���res[i]��һά����
            if (predictIn) mpRaderLayer->load_PredictImageIn(res[i], i);
            else mpRaderLayer->load_PredictImage(res[i], i);
            //  ����ֵ
            //  mpRaderLayer->load_PredictImage(res[i], i);
            //  mpRaderLayer->load_PredictImage(Images[i],i);
            //  progressValue = 50;
            //  mpRaderLayer->load_PredictImageIn(Images[i], i);

            //  ����������
            mpRaderLayer->CompleteVolume();
            //  GRadarVolume���״�������
            GRadarVolume* volume = new GRadarVolume();
            //  progressValue = 60;

            //  oldsize��ȫ��������״�������ĸ���
            volume = mpRaderLayer->CompPredictVolumeP(oldsize, i + 1);
            //  mRadarList���û����ص��״���������б�
            mRadarList.push_back(volume);
            //  progressValue = 70;
            QString number = QString::number(i + 1);
            QString name = fileName + number;
            mRadarPathSet.insert(name);
            ui.mFileListWidget->addItem(name);
            mpRaderLayer->clearConten();
            //  nt showvalue = 30 + space * (i + 1);
            //  dialog->setValue(showvalue);
            progressValue = 30 + space * (i + 1);

        }
        delete pred;
        complete = true;
    });

    while (!complete) {
        dialog->setValue(progressValue);
        QApplication::processEvents();

    }
    thread.join();

    dialog->close();
    delete dialog;
    

    /*
    int predictNum = 5;
    bool isok;
    QInputDialog inputdialog;
    inputdialog.setOption(QInputDialog::UsePlainTextEditForTextInput);
    predictNum = inputdialog.getInt(this, "������", "������:", 5, 0, 50, 1, &isok);
    if (isok)
    {    
        qDebug() << "predictNum:" << predictNum << endl;
        
        if (predictNum < 0)
        {
            QMessageBox::information(this, "��ʾ", "������������");
            return;
        }
        if (predictNum > mRadarList.size())
        {
            QMessageBox::information(this, "����", "�����������״��ļ�������");
            return;
        }
        mpRaderLayer->justCenter();

        QProgressDialog* dialog = nullptr;
        dialog = new QProgressDialog(this);
        dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
        //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
        dialog->setWindowTitle("����Ԥ��");
        dialog->setMinimumDuration(0);
        dialog->setRange(0, 100);
        dialog->setModal(true);
        dialog->show();
        QCoreApplication::processEvents();

        mIsPredict = false;
        dialog->setValue(0);
        QVector<QVector<QImage>> Images;
        int ret = mpRaderLayer->savePreInfo(Images,predictNum);
        dialog->setValue(10);
        
        PredRNN pred = PredRNN("D:/Data/predrnn_cuda/predrnn_cuda.pt");
        pred.thepredictNum = predictNum;
        QVector<QVector<QImage>> res = pred.predict(Images);
        dialog->setValue(30);
        
        //mpRaderLayer->setPreImage(res);

       // mpRaderLayer->setPreImage(Images[0]);
        ////ԭ��ͼ
        
        //qDebug() << "this1" << endl;
       // mpRaderLayer->convertdptolp();
       // qDebug() << "this2" << endl;
        //mpRaderLayer->setPredict(true);
        
        QString fileName = "Predict";
        int space = 70 / predictNum;
        for (int i = 0; i < predictNum; i++)
        {
            mpRaderLayer->load_PredictImage(res[i],i);
            //mpRaderLayer->load_PredictImage(Images[i]);
            mpRaderLayer->CompleteVolume();
            GRadarVolume* volume = new GRadarVolume();
            volume = mpRaderLayer->CompPredictVolumeP(oldsize, i + 1);
            mRadarList.push_back(volume);

            QString number = QString::number(i + 1);
            QString name = fileName + number;
            mRadarPathSet.insert(name);
            ui.mFileListWidget->addItem(name);
            mpRaderLayer->clearConten();
            int showvalue = 30 + space * (i + 1);
            dialog->setValue(showvalue);
        }
        dialog->close();
        //////////////////////////
        /*
        ///�ֻ�ͼ
        mpRaderLayer->load_PredictImage(res);
        //mpRaderLayer->load_PredictImage(Images[0]);
        dialog->setValue(60);
        mpRaderLayer->CompleteVolume();
        dialog->setValue(80);
        mpRaderLayer->CompPredictVolume();
        // mpRaderLayer->setPredict(true);
         //dialog->setValue(80);

        GRadarVolume* volume = new GRadarVolume();
        volume = mpRaderLayer->getPredictVolume();

        mRadarList.push_back(volume);
        qDebug() << "mRadarList.size():" << mRadarList.size() << endl;
        QString snumber = QString::number(predictNum);
        QString fileName = "Predict" + snumber;
        mRadarPathSet.insert(fileName);
        ui.mFileListWidget->addItem(fileName);
        dialog->setValue(100);
        dialog->close();
        QMessageBox::information(this, "��ʾ", "Ԥ��ɹ�");
        qDebug() << "ALL Time:" << timer.elapsed();
        
    }
    else
    {
        return;
    }
    */
    /*
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    //QVector<QImage> res1 = mpRaderLayer->getPreImage();
    int Flag = radarVisualDlg->load_PredictImage(mpRaderLayer, res);

    radarVisualDlg->CompleteVolume();
    qDebug() << "�������������" << endl;
   
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    radarVisualDlg->renderPrerdict();
    dialog->close();
    qDebug() << "��Ⱦ���" << endl;
    */
   // QRectF mktRect = mpView->dpTolp(rect);
    /*
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    //radarVisualDlg->setGridRect(mktRect);
    int Flag = radarVisualDlg->load_PredictImage(mpRaderLayer, res);

    radarVisualDlg->CompleteVolume();
    qDebug() << "�������������" << endl;

    TGriddingConfigDlg configDlg(this);
    if (configDlg.exec() != QDialog::Accepted) return;
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    // radarVisualDlg->renderSPredict();
    radarVisualDlg->renderPrerdict();
    qDebug() << "��Ⱦ���" << endl;
    */
    
   
   
    /*
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

   // int Flag = radarVisualDlg->load_PredictImage(mpRaderLayer, outputrootpath);
    if (Flag == -1)
    {
        QMessageBox::information(this, "��ʾ", "����ͼƬʧ��");
        return;
    }
    radarVisualDlg->CompleteVolume();
    qDebug() << "�������������" << endl;

    TGriddingConfigDlg configDlg(this);
    if (configDlg.exec() != QDialog::Accepted) return;
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();

    QProgressDialog* dialog = nullptr;
    dialog = new QProgressDialog(this);
    dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    dialog->setWindowTitle("����Ԥ��");
    dialog->setMinimumDuration(0);
    dialog->setRange(0, 3);
    dialog->setModal(true);
    dialog->show();
    radarVisualDlg->renderPrerdict();
    QCoreApplication::processEvents();
    dialog->close();
    qDebug() << "��Ⱦ���" << endl;
    */
  
}
//  ѡ����ο�ص� rectΪ��Ļ����
void TRadarVisualWnd::onSelectedRect(const QRect& rect)
{
    // QRect�������ڴ洢���ε�λ�úͳߴ���Ϣ,�����˾��ε����Ͻ����ꡢ��Ⱥ͸߶ȵ���Ϣ
    // QRect(int x, int y, int width, int height)
    QRect lastRect;
    // mLastRadar3DWnd����һ���״�������ά��С��������,����ϴ����״�������ά���ӻ�С���ھ͹رղ������
	if (mLastRadar3DWnd)
	{
        // ��ȡ�ϴε��״���ӻ�����Ĵ��ڵ�λ�úʹ�С�� OR �ϴ�ѡ��ľ��ο��λ�úʹ�С��   ????
        lastRect = mLastRadar3DWnd->geometry();
        delete mLastRadar3DWnd;
		mLastRadar3DWnd = nullptr;
	}
    int ret = -1;
    /**
        1. mpRaderLayer����ҳ���״�2dͼ��
        2. ���к������Ի�ȡ��ǰ��ʾ���״�������,����,����2Dͼ��ȹ���
        3. ��Ԥ�����ݾ�����ʹ��Ԥ������
    **/
    ret = mpRaderLayer->getPredict();
    const GRadarVolume* data = nullptr;
    if (ret == -1)
        data = mpRaderLayer->getCurRaderData();
    else
        data = mpRaderLayer->getPredictVolume();

    //  const GRadarVolume* data = mpRaderLayer->getCurRaderData();
    if (data == nullptr)
    {
        QMessageBox::information(this, windowTitle(), QString::fromLocal8Bit("���ȼ����״�����!"));
        return;
    }
    if (data->surfs.size() <= 0)
    {
        return;
    }
    // �¾�����ˮƽ�ʹ�ֱ�����϶���ԭʼ���ε����Ͻ��ƶ��˳�������10%,���ҿ�Ⱥ͸߶ȶ����ӵ���ԭʼ��120%
    QRect rect1(rect.left() - rect.width() * 0.1, rect.top() - rect.height() * 0.1,
        rect.width() * 1.2, rect.height() * 1.2);
    //  ����ѡ��ľ��ο�ĵ�ͼͼƬ,mpView������ͼ,�ǵ�ͼ��ͼ��,����ҳ
    QImage mapImage = mpView->saveSelectRectImage(rect1);
    //  ��ȡ��ǰѡ��ľ��ο�ĸ߳�ͼ��,�߳�ͼ�����ڱ�ʾ�ر�ĸߵͱ仯������Ƭ��һ����֯��ͼ��������߼������ܵķ�ʽ
    QImage elevImage = mpView->getElevationImage(rect1);
    //  ��Ļ����ת�����߼�����,����һ���µ��߼�����ľ���
    QRectF mktRect = mpView->dpTolp(rect);
    TGriddingConfigDlg configDlg(this);
    //  ���߼�����ľ��εľ��ȡ�γ�ȡ�x�����y�����ֵ�ֱ����õ��û������϶�Ӧ��SpinBox��,�Ա��ڽ�������ʾ���޸���Щֵ��
    configDlg.setRect(mktRect);
    //  ����"���񻯲�������"�ĶԻ���,�û��������ò�������Ϣ
    if (configDlg.exec() != QDialog::Accepted) return;

    /*�����ռ��׶�,�״��ά���ӻ�*/

    //  radarVisualDlg���µ��״�������ά���ӻ�С����
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    //  �û��ر� radarVisualDlg ����ʱ����Ӧ�Ķ���ᱻ�Զ��ͷź�ɾ��
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    //  �� radarVisualDlg ��������ʱ,mLastRadar3DWnd = nullptr;
    connect(radarVisualDlg, SIGNAL(destroyed(QObject*)), this, SLOT(radar3DWndDestroyed(QObject*)));

    //  ��ȡ�״������б�(����ҳ��2d�״������б�ֵ���״�������ά���ӻ�С���ڵ��״������б�)
    radarVisualDlg->setRadarDataList(mpRaderLayer->getRaderDataList());
    //  ��ȡ��ǰ��ʾ���״�����(ͬ��)
    radarVisualDlg->setRadarDataIndex(mpRaderLayer->getRaderDataIndex());
    // 	���õ�ͼͼ��
    radarVisualDlg->setMapImage(&mapImage);
    // 	���ø߳�ͼ��
    radarVisualDlg->setElevImage(&elevImage);
    // 	�������񻯷�Χ��webī����ͶӰ���꣩,ʹ�õĲ���rect���ʼѡ���û������ľ��ο��С
    radarVisualDlg->setGridRect(mktRect);
    //  ��ȡ��ɫ��
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    //radarDialog->setGridSize(dialog.nx, dialog.ny, dialog.nz);
    if (!lastRect.isEmpty())
    {
        //  lastRect���״����ݿ��ӻ�С����Ĵ��ڵ�λ�úʹ�С
        radarVisualDlg->setGeometry(lastRect);
    }
    
    //  ��ʾradarVisualDlg����
    radarVisualDlg->show();
    //  ��Ⱦ����
    radarVisualDlg->render();
    //  ���µ��״�������ά���ӻ�С����չʾ�����ݸ�ֵ��mLastRadar3DWnd
    mLastRadar3DWnd = radarVisualDlg;
}

void TRadarVisualWnd::radar3DWndDestroyed(QObject*)
{
    mLastRadar3DWnd = nullptr;
}
//  �ļ�->�� ���״��ļ��Ļص�����
void TRadarVisualWnd::loadRadarFiles(const QStringList& fileList)
{
    int oldRadarNum = mRadarList.size();
    int oldIndex = mpRaderLayer->getRaderDataIndex();

    QProgressDialog* dialog = nullptr;
    if (fileList.size() >= 2) 
    {
        dialog = new QProgressDialog(this);
        dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
        //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
        dialog->setWindowTitle(QString::fromLocal8Bit("���ڼ����״������ļ�"));
        dialog->setMinimumDuration(0);
        dialog->setRange(0, fileList.size());
        dialog->setModal(true);
        dialog->show();
    }
    QCoreApplication::processEvents();

    int i = 0;
    // �����ļ��б���ȡ��ԭʼ�״����ݴ��뵱ǰ�򿪵��״��ļ��б�openRadarList
    for (const QString& qstrPath : fileList) 
    {
        if (dialog != nullptr) 
        {
            dialog->setLabelText(qstrPath);
            dialog->setValue(i++);
            QCoreApplication::processEvents();
            if (dialog->wasCanceled()) 
            {
                break;
            }
        }

        // ����Ѿ����ع��ͺ���
        if (mRadarPathSet.contains(qstrPath)) {
            if (dialog != nullptr) {
                dialog->setValue(++i);
                QCoreApplication::processEvents();
            }
            continue;
        }
        std::string stdPath = qstrPath.toStdString();
        basedataImage* bdi = new basedataImage();
        const int ret = loadBasedataImage(stdPath.c_str(), bdi);
        if (ret != 1) {
            const int code = QMessageBox::critical(this,
                QString::fromLocal8Bit("����"),
                QString("%1 ��ʧ�ܣ�").arg(qstrPath),
                QMessageBox::StandardButton::Abort,
                QMessageBox::StandardButton::Ignore);
            if (code == QMessageBox::StandardButton::Abort) {
                break;
            }
        }
        // ��ȡ�״�����
        GRadarVolume* volume = new GRadarVolume();
        volume->extractData(*bdi, RDT_DBZ, -1000);

        if (volume->surfs.empty()) {
            // �򿪵��״��ļ�û��׶������
            qDebug() << qstrPath << ": no Surf data";
        }
        volume->path = qstrPath;
        // ���������ھ����Ͻ���ƽ��
        GRadarAlgorithm::smoothRadial(volume);
        // ƽ������
        GRadarAlgorithm::smoothRadial(volume);

        // ���뵱ǰ�򿪵��״��ļ��б�
        mRadarList.push_back(volume);

        // ���뵱ǰ�򿪵��״Ｐ�����б�
        mBdiList.push_back(bdi);

        // ���뵱ǰ�򿪵��״��ļ�·������
        mRadarPathSet.insert(qstrPath);

        // ����б�item
        int index = qstrPath.lastIndexOf('/');
        QString fileName = qstrPath.mid(index + 1, qstrPath.size() - index - 1);
        ui.mFileListWidget->addItem(fileName);
    }

    if (dialog != nullptr) 
    {
        dialog->setValue(i);
        dialog->close();
        QCoreApplication::processEvents();
        delete dialog;
    }

    // ���û���״��ļ����ؽ���
    if (mRadarList.size() == oldRadarNum) return;

    // Ѱ�ұ��δ򿪵ĵ�һ�������ݵ��״�����������
    int find = 0; // Ĭ���ǵ�һ��
    for (int i = oldRadarNum; i < mRadarList.size(); i++) 
    {
        const GRadarVolume* body = mRadarList.at(i);
        if (body->surfs.size() != 0) 
        {
            find = i;
            break;
        }
    }

    if (find == mpRaderLayer->getRaderDataIndex()) {
        return;
    }

    mpRaderLayer->setRaderDataIndex(find);
    ui.mFileListWidget->setCurrentRow(find);
}
//  �����޸ĺ���״�����
void TRadarVisualWnd::saveRadarFile(const QString& filepath, int mIndex) {

    //��Ԥ�����ݲ��ܱ���
    if (mIndex < mBdiList.size()) {
        const int code = QMessageBox::critical(this,
            QString::fromLocal8Bit("����"),
            QString::fromLocal8Bit("�����ݷ�Ԥ������, ��ѡ��Ԥ�����ݱ���"),
            QMessageBox::StandardButton::Abort);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }

    //qDebug() << "===============================================";
    //qDebug() << "====================����д��: mIndex =" << mIndex << "===================";
    //qDebug() << "====================mBdiList.size() =" << mBdiList.size() << "===================";
    //qDebug() << "===============================================";

    //����·��
    std::string stdPath = filepath.toStdString();

    //���һ֡��bdi
    basedataImage bdi_modify = *(mBdiList.at(mBdiList.size() - 1));

    //�޸�bdi�е�����
    mRadarList[mIndex]->modifyData(bdi_modify, RDT_DBZ);

    //GRadarVolume* modify_volume = new GRadarVolume();
    //modify_volume->extractData(bdi_modify, RDT_DBZ, -1000);

    ////�Ա��޸�ǰ���ȡ��volume�е����ݵ�ֵ
    //for (auto mSurf = (*mRadarList[mIndex]).surfs.begin(), Surf = (*modify_volume).surfs.begin(); mSurf != (*mRadarList[mIndex]).surfs.end(); mSurf++, Surf++) {
    //    for (auto mradial = (*mSurf)->radials.begin(), radial = (*Surf)->radials.begin(); mradial != (*mSurf)->radials.end(); mradial++, radial++) {
    //        for (auto mpoint = (*mradial)->points.begin(), point = (*radial)->points.begin(); mpoint != (*mradial)->points.end(); mpoint++, point++) {
    //            qDebug() << "==============================================";
    //            qDebug() << "before_modify = " << (*mpoint).value;
    //            qDebug() << "after_modify = " << (*point).value;
    //            qDebug() << "==============================================";
    //        }
    //    }
    //}

    //д���޸ĺ���ļ�
    //վ������
    std::string siteCode = std::string(bdi_modify.siteInfo.code);
    //ɨ�迪ʼʱ��
    std::string startTime = QDateTime::fromSecsSinceEpoch(bdi_modify.taskConf.startTime).toString("yyyyMMdd.hhmmss").toStdString();
    //Ԥ�����ݱ���·��������
    std::string stdPath_modify = stdPath + "/" + siteCode + "." + startTime + "_predict.AR2";
    //д��Ԥ����bdi�ļ�
    const int ret2 = writeBasedataImage(stdPath_modify.c_str(), &bdi_modify);
    if (ret2 != 1) {
        const int code = QMessageBox::critical(this,
            QString::fromLocal8Bit("����"),
            QString::fromLocal8Bit("%1 д��ʧ�ܣ�").arg(filepath),
            QMessageBox::StandardButton::Abort,
            QMessageBox::StandardButton::Ignore);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }
    else
    {
        const int code = QMessageBox::information(this,
            "��Ϣ",
            QString::fromLocal8Bit("����ɹ���"),
            QMessageBox::StandardButton::Ok);
        if (code == QMessageBox::StandardButton::Ok) {
            return;
        }
    }
}
//  �״������ı�ص�
void TRadarVisualWnd::onRaderIndexChange(int index)
{
    // ���RadioButtons
	QList<QAbstractButton*> buttons = mELBtnGroups->buttons();
	for (int i = 0; i < buttons.count(); i++)
	{
		ui.mSurfsLayout->removeWidget(buttons[i]);
		mELBtnGroups->removeButton(buttons[i]);
        delete buttons[i];
	}

    if (index == -1) 
    {
        // ���֮ǰ����ʾ����
        ui.mSiteNameLabel->setText(QString());
        ui.mSiteCodeLabel->setText(QString());
        ui.mLonLabel->setText(QString());
        ui.mLatLabel->setText(QString());
        ui.mTimeLabel->setText(QString());
        ui.mSurfsCountLabel->setText(QString());
        mELIndex = 0;
        return;
    }

    const auto& volume = mRadarList.at(index);

    ui.mSiteNameLabel->setText(volume->siteName);
    ui.mSiteCodeLabel->setText(volume->siteCode);

    ui.mLonLabel->setText(QString::number(volume->longitude, 'f', 4));
    ui.mLatLabel->setText(QString::number(volume->latitude, 'f', 4));
    ui.mTimeLabel->setText(volume->startTime);
    ui.mSurfsCountLabel->setText(QString::number(volume->surfs.size()));

    for (int i = 0; i < volume->surfs.size(); ++i) 
    {
        const GRadialSurf* pSurf = volume->surfs[i];
        QString hint = QString::number(pSurf->el, 'f', 2) + QString::fromLocal8Bit("��");

        QRadioButton* rb = new QRadioButton(hint, this);

        if (i == mELIndex)
        {
            rb->setChecked(true);
        }

        mELBtnGroups->addButton(rb);
        mELBtnGroups->setId(rb, i);

        ui.mSurfsLayout->addWidget(rb, i / 4, i % 4);
    }
}
//  �����״�ͼ���Ƿ񲥷Ŷ���
void TRadarVisualWnd::setAnimate(bool isAnimate)
{
    if (this->mIsAnimate == isAnimate) {
        return;
    }

    this->mIsAnimate = isAnimate;

    ui.actAnimationStart->setEnabled(!isAnimate);
    ui.actAnimationStop->setEnabled(isAnimate);
    // ����ʱ��ֹ����״��б�
    ui.mFileListWidget->setEnabled(!isAnimate);
    ui.mSurfsLayout->setEnabled(!isAnimate); // Ϊʲô��Ч��
    ui.actSelectRect->setEnabled(!isAnimate);

    if (isAnimate) {
        mAnimateTimer.setInterval(GConfig::mAnimationConfig.mInterval);
        mAnimateTimer.start();
    } else {
        mAnimateTimer.stop();
    }
}
//  �״�2D������ʱ�ص�
void TRadarVisualWnd::onAnimateTimeout()
{
    size_t radarNum = mRadarList.size();
    if (radarNum <= 1) {
        return;
    }

    int oldIndex = mpRaderLayer->getRaderDataIndex();
    int find = oldIndex;
    // �ҵ���һ����ʾ���״�����
    // ����������
    if (GConfig::mAnimationConfig.mIsSkipEmpty) {
        int start = oldIndex;
        do {
            ++find;
            if (find >= radarNum) find = 0;
            if (mRadarList.at(find)->surfs.size() > 0) {
                break;
            }
        } while (find != start);
    } else {
        find = oldIndex + 1;
        if (find >= mRadarList.size()) {
            find = 0;
        }
    }
    if (find == oldIndex) {
        return;
    }
    mpRaderLayer->setRaderDataIndex(find);

    //onRaderIndexChange(find);
    ui.mFileListWidget->setCurrentRow(find);
}