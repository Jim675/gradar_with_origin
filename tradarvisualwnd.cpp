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

TRadarVisualWnd::TRadarVisualWnd(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags)
{
    ui.setupUi(this);

    // 颜色传输函数
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

    // 文件列表
    ui.mFileListWidget->setAlternatingRowColors(true);
    ui.mFileListDock->setWidget(ui.mFileListWidget);

    // 仰角按钮组
    mELBtnGroups = new QButtonGroup(this);
    mELIndex = 0;
    connect(mELBtnGroups, SIGNAL(buttonClicked(int)), this, SLOT(onELButtonClicked(int)));

    // 二维主视图
    mpView = new GMapView(this);
    setCentralWidget(mpView);
    QLabel* infoLabel = new QLabel(this);
    statusBar()->addPermanentWidget(infoLabel);
    mpView->setInfoLabel(infoLabel);
	// 限制用户可视范围
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
	// 连接选择矩形框回调函数
	connect(mpView, &GMapView::onSelectedRect, this, &TRadarVisualWnd::onSelectedRect);
    
    //预测
    QObject::connect(ui.actionPredict, &QAction::triggered, this, &TRadarVisualWnd::on_actPredict_triggered);
    //停止预测
    QObject::connect(ui.actionStop, &QAction::triggered, this, &TRadarVisualWnd::on_actStop_triggered);

    //保存
   // QObject::connect(ui.actionSave, &QAction::triggered, this, &TRadarVisualWnd::on_actSave_triggered);
   
    // 添加图层, 先添加的图层先绘制
    // 添加雷达图层
    mpRaderLayer = new GRader2DLayer();
    mpRaderLayer->setLayerName("Rader2DLayer");
    mpRaderLayer->setRaderDataList(&mRadarList); // 设置雷达列表
    mpRaderLayer->setColorTransferFunction(mColorTF);
    mpView->layerSet()->addLayer(mpRaderLayer);

    // 添加Shape图层
    mpShapeLayer = new GShapeLayer();
    mpShapeLayer->setLayerName("ShapeLayer");
    mpView->layerSet()->addLayer(mpShapeLayer);

    // 添加雷达色标图层
    mpColorBarLayer = new GColorBarLayer();
    mpColorBarLayer->setLayerName("ColorBarLayer");
    mpColorBarLayer->setColorTransferFunction(mColorTF);
    mpView->layerSet()->addLayer(mpColorBarLayer);

	// 新增两个控制雷达图层的CheckBox
	ui.mInterpCBox->setCheckState(Qt::CheckState::Unchecked);
	ui.statusBar->addPermanentWidget(ui.mInterpCBox);
    ui.mInterpCBox->setChecked(true);
	ui.mRadarCBox->setCheckState(Qt::CheckState::Checked);
	ui.statusBar->addPermanentWidget(ui.mRadarCBox);

    ui.mInterpCBox->setVisible(false);
    ui.mRadarCBox->setVisible(false);

	// 默认关闭动画
	mIsAnimate = true;
	this->setAnimate(false);
    // 初始化Timer
    mAnimateTimer.setSingleShot(false);
    connect(&mAnimateTimer, &QTimer::timeout, this, &TRadarVisualWnd::onAnimateTimeout);

    // 设置主窗口大小
    
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

//设置主窗口大小
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
void TRadarVisualWnd::centerOn(const QRectF& rc)
{
    mpView->centerOn(rc);
}

void TRadarVisualWnd::on_actFileOpen_triggered()
{
    if (mLastRadar3DWnd)
    {
        mLastRadar3DWnd->close();
        mLastRadar3DWnd = nullptr;
    }

	QString path = GConfig::mLastSelectPath;
	QFileDialog dialog;
	dialog.setWindowTitle(QString::fromLocal8Bit("请选择雷达数据文件"));
	dialog.setDirectory(path);
	dialog.setFilter(QDir::Filter::Files);
	dialog.setNameFilter(QString::fromLocal8Bit("雷达数据文件(*.AR2)"));
	dialog.setFileMode(QFileDialog::FileMode::ExistingFiles);
	if (dialog.exec() != QDialog::Accepted) return;

	GConfig::mLastSelectPath = dialog.directory().absolutePath();
	QStringList fileList = dialog.selectedFiles();
	if (fileList.isEmpty()) return;

    fileList.sort(Qt::CaseInsensitive);

    on_actClearFileList_triggered();
    // 打开文件列表
	loadRadarFiles(fileList);
}

void TRadarVisualWnd::on_actFileSave_triggered() {
    int find = -1;
    // 获取当前选中的有数据的雷达数据体索引
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
            QString::fromLocal8Bit("错误"),
            QString::fromLocal8Bit("没有雷达数据可写入！"),
            QMessageBox::StandardButton::Abort);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }

    //获取上次打开的路径
    QString path = GConfig::mLastSelectPath;

    //dir_path即为选择的文件夹的绝对路径，第二形参为对话框标题，第三个为对话框打开后默认的路径。
    QString dir_path = QFileDialog::getExistingDirectory(this, "choose directory", path);

    //qDebug() << "====================================";
    //qDebug() << dir_path;
    //qDebug() << find;
    //qDebug() << "====================================";

    // 保存文件
    saveRadarFile(dir_path, find);
}

void TRadarVisualWnd::on_actClearFileList_triggered()
{
    if (mLastRadar3DWnd)
	{
        mLastRadar3DWnd->close();
		mLastRadar3DWnd = nullptr;
	}

	// 更新文件列表
	mpRaderLayer->clear();
    qDeleteAll(mRadarList);
    mELIndex = 0;
	mRadarList.clear();
    mRadarPathSet.clear();
	ui.mFileListWidget->clear();
	onRaderIndexChange(-1);
}

void TRadarVisualWnd::on_actExit_triggered()
{
    if (mLastRadar3DWnd)
	{
        mLastRadar3DWnd->close();
		mLastRadar3DWnd = nullptr;
	}

    close();
}

void TRadarVisualWnd::on_actAnimationStart_triggered()
{
    this->setAnimate(true);
}

void TRadarVisualWnd::on_actAnimationStop_triggered()
{
    this->setAnimate(false);
}

void TRadarVisualWnd::on_actAnimationSettings_triggered()
{
	TAnimationConfigDlg configDlg;
	configDlg.exec();
}

void TRadarVisualWnd::on_actViewReset_triggered()
{
	ui.mFileListDock->show();
	ui.mRadarDock->show();
}

void TRadarVisualWnd::on_mFileListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    // 如果当前在动画就忽略
	if (mIsAnimate) return;

	//auto msg = QString("clicked:") + QString::number(index.row());
    int row = ui.mFileListWidget->currentRow();
	mpRaderLayer->setRaderDataIndex(row);
    mpRaderLayer->setSurfaceIndex(mELIndex);
	// 通知自己
	this->onRaderIndexChange(row);
}

void TRadarVisualWnd::on_mInterpCBox_stateChanged(int state)
{
    bool isCheck = state == Qt::Checked;
    mpRaderLayer->setInterpolate(isCheck);
    ui.actInterpRadarData->setChecked(isCheck);
}

void TRadarVisualWnd::on_actInterpRadarData_toggled(bool checked)
{
    ui.mInterpCBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

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

void TRadarVisualWnd::onELButtonClicked(int id)
{
    mpRaderLayer->setSurfaceIndex(id);
    mELIndex = id;
}

void TRadarVisualWnd::on_actTestServer_triggered()
{
    
}

// 选择矩形框
void TRadarVisualWnd::on_actSelectRect_triggered(bool checked)
{
    if (checked) {
        mpView->setSelectMode(SELECT_RECT);
    } else {
        mpView->setSelectMode(SELECT_NONE);
    }
}

/*
//将ImageData每层分离并保存
void TRadarVisualWnd::on_actSave_triggered()
{
    //QString rootpath = QFileDialog::getExistingDirectory(this, "选择保存位置""./", "图像文件 (*.jpg *.png)");
   
    //int ret=mpRaderLayer->drawElpng(rootpath);
    
    
    const GRadarVolume* data = mpRaderLayer->getCurRaderData();
    if (data == nullptr)
    {
        QMessageBox::information(this, windowTitle(), "请先加载雷达数据!");
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
    // 设置数据
    radarVisualDlg->setRadarDataList(mpRaderLayer->getRaderDataList());
    radarVisualDlg->setRadarDataIndex(mpRaderLayer->getRaderDataIndex());
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    radarVisualDlg->renderAll();
   
}*/


//停止预测
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


//预测
void TRadarVisualWnd::on_actPredict_triggered()
{

    if (!is_gpu_memory_suitable()) {
        QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("显存不足"));
        return;
    }
    QTime timer;
    timer.start();
    int oldsize = mRadarList.size();
    if (mRadarList.size() < 5)
    {
        QMessageBox::information(this, "提示", QString::fromLocal8Bit("请至少打开五个雷达文件"));
        return;
    }

    const std::string model_root("models");

    if (!std::filesystem::exists(std::filesystem::path(model_root))) {
        QMessageBox::warning(this, "Error", "Miss prediction models!");
            return;
    }

    // scan models from dir
    auto model_list = list_model_files(model_root);

    TPredictDlg predictDialog(model_list, this);
    int code = predictDialog.exec();
    if (code != QDialog::Accepted)
    {
        return;
    }

    int predictNum = 0;
    int chooseIndex = 0;
    predictDialog.getInfo(chooseIndex, predictNum);

    if (predictNum > mRadarList.size())
    {
        QMessageBox::information(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("输入数大于雷达文件个数！"));
        return;
    }
    if (predictNum > 5)
    {
        QMessageBox::information(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("--输入数应<=5--"));
        return;
    }
    //setDeskTopWidget();
    //mpRaderLayer->justCenter();
    string path;
    int image_width = 920;
    int patchsize = 5;
    float maxmin = 1;
    bool predictIn = true;//是否插值
    bool usesimvp = false;

    // model path
    path = model_root + "/" + model_list[chooseIndex];
    qDebug() << "model path" << QString::fromStdString(path) << endl;
    // read model config
    std::string model_name = model_list[chooseIndex];
    std::string config_file_path = model_root + "/" + model_name.substr(0, model_name.find_last_of(".")) + ".txt";

    bool has_config_file;
    auto parameters = read_model_parameters(config_file_path, has_config_file);

    if (!has_config_file) { return; }

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
    dialog->setWindowTitle(QString::fromLocal8Bit("正在预测"));
    dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    dialog->setMinimumDuration(0);
    //dialog->setMinimum(0);
    // dialog->setMaximum(100);
    dialog->setRange(0, 100);
    dialog->setModal(true);
    dialog->show();
    QCoreApplication::processEvents();

    dialog->setValue(0);
    QVector<QVector<QImage>> Images;
    // int ret = mpRaderLayer->savePreInfo(Images, predictNum);
     //int ret = mpRaderLayer->savePreInfo(Images);
     // 
     //920或1840插值

    int progressValue = 0;
    bool complete = false;
    std::thread thread([&]() {
        int ret = 0;
        mpRaderLayer->imgwidth = image_width;
        if (predictIn) ret = mpRaderLayer->savePreInfoIn(Images);
        else ret = mpRaderLayer->savePreInfo(Images);

        //mpRaderLayer->setPreImage(Images[0]);
        //mpRaderLayer->convertdptolp();
        progressValue = 10;
        //dialog->setValue(10);

        auto pred = IPredict::load(path);

        pred->setProperties(predictNum, image_width, patchsize, maxmin);

        QVector<QVector<QImage>> res = pred->predict(Images, usesimvp);

        progressValue = 30;
        //dialog->setValue(30);
        //dialog->setWindowTitle("数据插值整合");

        QString fileName = "Predict";
        int space = 70 / predictNum;
        for (int i = 0; i < predictNum; i++)
        {
            //插值
        //    progressValue = 40;
            if (predictIn) mpRaderLayer->load_PredictImageIn(res[i], i);
            else mpRaderLayer->load_PredictImage(res[i], i);
            //不插值
            //mpRaderLayer->load_PredictImage(res[i], i);
            //mpRaderLayer->load_PredictImage(Images[i],i);
         //   progressValue = 50;
            //mpRaderLayer->load_PredictImageIn(Images[i], i);
            mpRaderLayer->CompleteVolume();
            GRadarVolume* volume = new GRadarVolume();
        //    progressValue = 60;
            volume = mpRaderLayer->CompPredictVolumeP(oldsize, i + 1);
            mRadarList.push_back(volume);
          //  progressValue = 70;
            QString number = QString::number(i + 1);
            QString name = fileName + number;
            mRadarPathSet.insert(name);
            ui.mFileListWidget->addItem(name);
            mpRaderLayer->clearConten();
           // int showvalue = 30 + space * (i + 1);
           // dialog->setValue(showvalue);
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
    predictNum = inputdialog.getInt(this, "请输入", "请输入:", 5, 0, 50, 1, &isok);
    if (isok)
    {    
        qDebug() << "predictNum:" << predictNum << endl;
        
        if (predictNum < 0)
        {
            QMessageBox::information(this, "提示", "输入数大于零");
            return;
        }
        if (predictNum > mRadarList.size())
        {
            QMessageBox::information(this, "错误", "输入数大于雷达文件个数！");
            return;
        }
        mpRaderLayer->justCenter();

        QProgressDialog* dialog = nullptr;
        dialog = new QProgressDialog(this);
        dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
        //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
        dialog->setWindowTitle("正在预测");
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
        ////原画图
        
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
        ///现画图
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
        QMessageBox::information(this, "提示", "预测成功");
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
    qDebug() << "整合数据体完成" << endl;
   
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    radarVisualDlg->renderPrerdict();
    dialog->close();
    qDebug() << "渲染完成" << endl;
    */
   // QRectF mktRect = mpView->dpTolp(rect);
    /*
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    //radarVisualDlg->setGridRect(mktRect);
    int Flag = radarVisualDlg->load_PredictImage(mpRaderLayer, res);

    radarVisualDlg->CompleteVolume();
    qDebug() << "整合数据体完成" << endl;

    TGriddingConfigDlg configDlg(this);
    if (configDlg.exec() != QDialog::Accepted) return;
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();
    // radarVisualDlg->renderSPredict();
    radarVisualDlg->renderPrerdict();
    qDebug() << "渲染完成" << endl;
    */
    
   
   
    /*
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);

   // int Flag = radarVisualDlg->load_PredictImage(mpRaderLayer, outputrootpath);
    if (Flag == -1)
    {
        QMessageBox::information(this, "提示", "加载图片失败");
        return;
    }
    radarVisualDlg->CompleteVolume();
    qDebug() << "整合数据体完成" << endl;

    TGriddingConfigDlg configDlg(this);
    if (configDlg.exec() != QDialog::Accepted) return;
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    radarVisualDlg->show();

    QProgressDialog* dialog = nullptr;
    dialog = new QProgressDialog(this);
    dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    //dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    dialog->setWindowTitle("正在预测");
    dialog->setMinimumDuration(0);
    dialog->setRange(0, 3);
    dialog->setModal(true);
    dialog->show();
    radarVisualDlg->renderPrerdict();
    QCoreApplication::processEvents();
    dialog->close();
    qDebug() << "渲染完成" << endl;
    */
  
}
// 选择矩形框回调 rect为屏幕坐标
void TRadarVisualWnd::onSelectedRect(const QRect& rect)
{
    QRect lastRect;
	if (mLastRadar3DWnd)
	{
        lastRect = mLastRadar3DWnd->geometry();
        delete mLastRadar3DWnd;
		mLastRadar3DWnd = nullptr;
	}
    

    //此次为添加
    int ret = -1;
    ret = mpRaderLayer->getPredict();
    const GRadarVolume* data = nullptr;
    if (ret == -1)
        data = mpRaderLayer->getCurRaderData();
    else
        data = mpRaderLayer->getPredictVolume();

    //const GRadarVolume* data = mpRaderLayer->getCurRaderData();
    if (data == nullptr)
    {
        QMessageBox::information(this, windowTitle(), QString::fromLocal8Bit("请先加载雷达数据!"));
        return;
    }
    if (data->surfs.size() <= 0)
    {
        return;
    }
    QRect rect1(rect.left() - rect.width() * 0.1, rect.top() - rect.height() * 0.1,
        rect.width() * 1.2, rect.height() * 1.2);
    QImage mapImage = mpView->saveSelectRectImage(rect1);
    QImage elevImage = mpView->getElevationImage(rect1);
    QRectF mktRect = mpView->dpTolp(rect);
    TGriddingConfigDlg configDlg(this);
    configDlg.setRect(mktRect);
    if (configDlg.exec() != QDialog::Accepted) return;

    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    connect(radarVisualDlg, SIGNAL(destroyed(QObject*)), this, SLOT(radar3DWndDestroyed(QObject*)));
    // 设置数据
    radarVisualDlg->setRadarDataList(mpRaderLayer->getRaderDataList());
    radarVisualDlg->setRadarDataIndex(mpRaderLayer->getRaderDataIndex());
    radarVisualDlg->setMapImage(&mapImage);
    radarVisualDlg->setElevImage(&elevImage);
    radarVisualDlg->setGridRect(mktRect);
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    //radarDialog->setGridSize(dialog.nx, dialog.ny, dialog.nz);
    if (!lastRect.isEmpty())
    {
        radarVisualDlg->setGeometry(lastRect);
    }
    radarVisualDlg->show();
    radarVisualDlg->render();

    mLastRadar3DWnd = radarVisualDlg;
}


void TRadarVisualWnd::radar3DWndDestroyed(QObject*)
{
    mLastRadar3DWnd = nullptr;
}

// 文件->打开 打开雷达文件的回调函数
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
        dialog->setWindowTitle(QString::fromLocal8Bit("正在加载雷达数据文件"));
        dialog->setMinimumDuration(0);
        dialog->setRange(0, fileList.size());
        dialog->setModal(true);
        dialog->show();
    }
    QCoreApplication::processEvents();

    int i = 0;
    // 遍历文件列表提取出原始雷达数据存入当前打开的雷达文件列表openRadarList
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

        // 如果已经加载过就忽略
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
                QString::fromLocal8Bit("错误"),
                QString("%1 打开失败！").arg(qstrPath),
                QMessageBox::StandardButton::Abort,
                QMessageBox::StandardButton::Ignore);
            if (code == QMessageBox::StandardButton::Abort) {
                break;
            }
        }
        // 提取雷达数据
        GRadarVolume* volume = new GRadarVolume();
        volume->extractData(*bdi, RDT_DBZ, -1000);

        if (volume->surfs.empty()) {
            // 打开的雷达文件没有锥面数据
            qDebug() << qstrPath << ": no Surf data";
        }
        volume->path = qstrPath;
        // 对数据体在径向上进行平滑
        GRadarAlgorithm::smoothRadial(volume);
        // 平滑两次
        GRadarAlgorithm::smoothRadial(volume);

        // 存入当前打开的雷达文件列表
        mRadarList.push_back(volume);

        // 存入当前打开的雷达及数据列表
        mBdiList.push_back(bdi);

        // 存入当前打开的雷达文件路径集合
        mRadarPathSet.insert(qstrPath);

        // 添加列表item
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

    // 如果没有雷达文件加载进来
    if (mRadarList.size() == oldRadarNum) return;

    // 寻找本次打开的第一个有数据的雷达数据体索引
    int find = 0; // 默认是第一个
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

// 保存修改后的雷达数据
void TRadarVisualWnd::saveRadarFile(const QString& filepath, int mIndex) {

    //非预测数据不能保存
    if (mIndex < mBdiList.size()) {
        const int code = QMessageBox::critical(this,
            QString::fromLocal8Bit("错误"),
            QString::fromLocal8Bit("该数据非预测数据, 请选择预测数据保存"),
            QMessageBox::StandardButton::Abort);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }

    //qDebug() << "===============================================";
    //qDebug() << "====================正在写入: mIndex =" << mIndex << "===================";
    //qDebug() << "====================mBdiList.size() =" << mBdiList.size() << "===================";
    //qDebug() << "===============================================";

    //保存路径
    std::string stdPath = filepath.toStdString();

    //最后一帧的bdi
    basedataImage bdi_modify = *(mBdiList.at(mBdiList.size() - 1));

    //修改bdi中的数据
    mRadarList[mIndex]->modifyData(bdi_modify, RDT_DBZ);

    //GRadarVolume* modify_volume = new GRadarVolume();
    //modify_volume->extractData(bdi_modify, RDT_DBZ, -1000);

    ////对比修改前后读取的volume中点数据的值
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

    //写入修改后的文件
    //站点名称
    std::string siteCode = std::string(bdi_modify.siteInfo.code);
    //扫描开始时间
    std::string startTime = QDateTime::fromSecsSinceEpoch(bdi_modify.taskConf.startTime).toString("yyyyMMdd.hhmmss").toStdString();
    //预测数据保存路径及名字
    std::string stdPath_modify = stdPath + "/" + siteCode + "." + startTime + "_predict.AR2";
    //写入预测后的bdi文件
    const int ret2 = writeBasedataImage(stdPath_modify.c_str(), &bdi_modify);
    if (ret2 != 1) {
        const int code = QMessageBox::critical(this,
            QString::fromLocal8Bit("错误"),
            QString::fromLocal8Bit("%1 写入失败！").arg(filepath),
            QMessageBox::StandardButton::Abort,
            QMessageBox::StandardButton::Ignore);
        if (code == QMessageBox::StandardButton::Abort) {
            return;
        }
    }
    else
    {
        const int code = QMessageBox::information(this,
            "信息",
            QString::fromLocal8Bit("保存成功！"),
            QMessageBox::StandardButton::Ok);
        if (code == QMessageBox::StandardButton::Ok) {
            return;
        }
    }
}

// 雷达索引改变回调
void TRadarVisualWnd::onRaderIndexChange(int index)
{
    // 清空RadioButtons
	QList<QAbstractButton*> buttons = mELBtnGroups->buttons();
	for (int i = 0; i < buttons.count(); i++)
	{
		ui.mSurfsLayout->removeWidget(buttons[i]);
		mELBtnGroups->removeButton(buttons[i]);
        delete buttons[i];
	}

    if (index == -1) 
    {
        // 清空之前的显示数据
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
        QString hint = QString::number(pSurf->el, 'f', 2) + QString::fromLocal8Bit("°");

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

// 设置雷达图层是否播放动画
void TRadarVisualWnd::setAnimate(bool isAnimate)
{
    if (this->mIsAnimate == isAnimate) {
        return;
    }

    this->mIsAnimate = isAnimate;

    ui.actAnimationStart->setEnabled(!isAnimate);
    ui.actAnimationStop->setEnabled(isAnimate);
    // 动画时禁止点击雷达列表
    ui.mFileListWidget->setEnabled(!isAnimate);
    ui.mSurfsLayout->setEnabled(!isAnimate); // 为什么无效？
    ui.actSelectRect->setEnabled(!isAnimate);

    if (isAnimate) {
        mAnimateTimer.setInterval(GConfig::mAnimationConfig.mInterval);
        mAnimateTimer.start();
    } else {
        mAnimateTimer.stop();
    }
}

// 雷达2D动画超时回调
void TRadarVisualWnd::onAnimateTimeout()
{
    size_t radarNum = mRadarList.size();
    if (radarNum <= 1) {
        return;
    }

    int oldIndex = mpRaderLayer->getRaderDataIndex();
    int find = oldIndex;
    // 找到下一个显示的雷达索引
    // 跳过空数据
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

