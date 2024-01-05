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

//  主页面的大小,显示,功能,按钮的初始配置
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

//  设置主窗口大小
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
//  将视图移动到指定矩形范围的中心
void TRadarVisualWnd::centerOn(const QRectF& rc)
{
    mpView->centerOn(rc);
}
//  打开文件
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
//  保存文件
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
//  清空文件列表
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
//  退出
void TRadarVisualWnd::on_actExit_triggered()
{
    if (mLastRadar3DWnd)
	{
        mLastRadar3DWnd->close();
		mLastRadar3DWnd = nullptr;
	}

    close();
}
//  开始播放动画
void TRadarVisualWnd::on_actAnimationStart_triggered()
{
    this->setAnimate(true);
}
//  停止播放动画
void TRadarVisualWnd::on_actAnimationStop_triggered()
{
    this->setAnimate(false);
}
//  设置动画播放参数
void TRadarVisualWnd::on_actAnimationSettings_triggered()
{
	TAnimationConfigDlg configDlg;
	configDlg.exec();
}
//  重置窗口视图
void TRadarVisualWnd::on_actViewReset_triggered()
{
	ui.mFileListDock->show();
	ui.mRadarDock->show();
}
//  当前文件变化
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
//  二维图层插值状态变化
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
//  雷达显示状态变化
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
//  仰角选择变化
void TRadarVisualWnd::onELButtonClicked(int id)
{
    mpRaderLayer->setSurfaceIndex(id);
    mELIndex = id;
}

void TRadarVisualWnd::on_actTestServer_triggered()
{
    
}
//  选择矩形框
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
//  停止预测
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
//  预测
void TRadarVisualWnd::on_actPredict_triggered()
{
    // 判断GPU内存是否足够
    if (!is_gpu_memory_suitable()) {
        QMessageBox::warning(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("显存不足"));
        return;
    }
    // 设置QT的定时器
    QTime timer;
    timer.start();
    int oldsize = mRadarList.size();
    // 判断是否有足够的雷达数据
    if (mRadarList.size() < 5)
    {
        QMessageBox::information(this, "提示", QString::fromLocal8Bit("请至少打开五个雷达文件"));
        return;
    }
    // 定义了一个常量字符串model_root，并将其初始化为"models"
    const std::string model_root("models");

    // 使用了C++17的文件系统库来检查model_root路径是否存在
    if (!std::filesystem::exists(std::filesystem::path(model_root))) {
        QMessageBox::warning(this, "Error", "Miss prediction models!");
            return;
    }
    // list_model_files的主要目的是找到model_root路径下所有扩展名为".pt"的文件，并返回它们的文件名
    auto model_list = list_model_files(model_root);
    // 显示对话框
    TPredictDlg predictDialog(model_list, this);
    // 检测到用户关闭对话框就退出程序
    int code = predictDialog.exec();
    if (code != QDialog::Accepted)
    {
        return;
    }
    // 获取用户选择的模型索引和预测个数
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
    // 被分割出来的小块的宽度和高度
    int patchsize = 5;
    float maxmin = 1;
    bool predictIn = true;//是否插值
    bool usesimvp = false;

    // 选择预测的模型,合成地址
    path = model_root + "/" + model_list[chooseIndex];
    qDebug() << "model path" << QString::fromStdString(path) << endl;
    // 预先生成配置文件txt格式
    std::string model_name = model_list[chooseIndex];
    std::string config_file_path = model_root + "/" + model_name.substr(0, model_name.find_last_of(".")) + ".txt";

    // 读取配置文件,并返回配置的参数和是否存在配置文件
    bool has_config_file;
    auto parameters = read_model_parameters(config_file_path, has_config_file);
    // 如果没有配置文件,则退出
    if (!has_config_file) { return; }
    // 下面一连串的if/else都是如果配置文件中没有指定参数,则使用默认值
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
    // 920或1840插值

    // 用于显示进度条的百分比
    int progressValue = 0;
    // 用于判断线程是否执行完毕
    bool complete = false;

    /** 创建了一个新的线程，并立即开始执行,thread的流程如下:
        1. 获取雷达数据
        2. 预测
        3. 显示预测结果,生成新的雷达数据体
    **/
    std::thread thread([&]() {
        int ret = 0;
        // mpRaderLayer是雷达2D图层
        mpRaderLayer->imgwidth = image_width;
        // Images是雷达数据转换成2D图层需要的预数据,是一个二维数组,有几个雷达他就有多长
        if (predictIn) ret = mpRaderLayer->savePreInfoIn(Images); //插值
        else ret = mpRaderLayer->savePreInfo(Images); //不插值

        //  mpRaderLayer->setPreImage(Images[0]);
        //  mpRaderLayer->convertdptolp();
        progressValue = 10;
        //  dialog->setValue(10);

        auto pred = IPredict::load(path);

        pred->setProperties(predictNum, image_width, patchsize, maxmin);

        // res[雷达][锥面],有五个包含九个锥面的雷达的数据
        QVector<QVector<QImage>> res = pred->predict(Images, usesimvp);

        progressValue = 30;
        //  dialog->setValue(30);
        //  dialog->setWindowTitle("数据插值整合");

        QString fileName = "Predict";

        /**
            1. predictNum是用户输入的在预测后生成的预测结果的个数,最大是5,最小要大于载入的雷达个数
            2. 系统有强制要求加载的雷达个数大于五,所以predictNum恒小于等于五
            3. 预测的结果就是返回有五个包含九个锥面的雷达的数据
        **/
        int space = 70 / predictNum;
        for (int i = 0; i < predictNum; i++)
        {
            //  插值
            //  progressValue = 40;
            //  预测就是用所有雷达的同一个锥面来预测!!!,下面传入的res[i]是一维数组
            if (predictIn) mpRaderLayer->load_PredictImageIn(res[i], i);
            else mpRaderLayer->load_PredictImage(res[i], i);
            //  不插值
            //  mpRaderLayer->load_PredictImage(res[i], i);
            //  mpRaderLayer->load_PredictImage(Images[i],i);
            //  progressValue = 50;
            //  mpRaderLayer->load_PredictImageIn(Images[i], i);

            //  整合数据体
            mpRaderLayer->CompleteVolume();
            //  GRadarVolume是雷达数据体
            GRadarVolume* volume = new GRadarVolume();
            //  progressValue = 60;

            //  oldsize是全部载入的雷达数据体的个数
            volume = mpRaderLayer->CompPredictVolumeP(oldsize, i + 1);
            //  mRadarList是用户加载的雷达数据体的列表
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
//  选择矩形框回调 rect为屏幕坐标
void TRadarVisualWnd::onSelectedRect(const QRect& rect)
{
    // QRect类型用于存储矩形的位置和尺寸信息,包含了矩形的左上角坐标、宽度和高度等信息
    // QRect(int x, int y, int width, int height)
    QRect lastRect;
    // mLastRadar3DWnd是上一个雷达数据三维化小窗口数据,如果上次有雷达数据三维可视化小窗口就关闭并且清除
	if (mLastRadar3DWnd)
	{
        // 获取上次的雷达可视化对象的窗口的位置和大小√ OR 上次选择的矩形框的位置和大小×   ????
        lastRect = mLastRadar3DWnd->geometry();
        delete mLastRadar3DWnd;
		mLastRadar3DWnd = nullptr;
	}
    int ret = -1;
    /**
        1. mpRaderLayer是主页面雷达2d图层
        2. 他有函数可以获取当前显示的雷达数据体,索引,绘制2D图像等功能
        3. 有预测数据就优先使用预测数据
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
        QMessageBox::information(this, windowTitle(), QString::fromLocal8Bit("请先加载雷达数据!"));
        return;
    }
    if (data->surfs.size() <= 0)
    {
        return;
    }
    // 新矩形在水平和垂直方向上都向原始矩形的左上角移动了长宽距离的10%,并且宽度和高度都增加到了原始的120%
    QRect rect1(rect.left() - rect.width() * 0.1, rect.top() - rect.height() * 0.1,
        rect.width() * 1.2, rect.height() * 1.2);
    //  保存选择的矩形框的地图图片,mpView是主视图,是地图视图类,在主页
    QImage mapImage = mpView->saveSelectRectImage(rect1);
    //  获取当前选择的矩形框的高程图像,高程图像用于表示地表的高低变化，而瓦片是一种组织地图数据以提高加载性能的方式
    QImage elevImage = mpView->getElevationImage(rect1);
    //  屏幕坐标转换到逻辑坐标,构建一个新的逻辑坐标的矩形
    QRectF mktRect = mpView->dpTolp(rect);
    TGriddingConfigDlg configDlg(this);
    //  将逻辑坐标的矩形的经度、纬度、x坐标和y坐标的值分别设置到用户界面上对应的SpinBox中,以便在界面上显示和修改这些值。
    configDlg.setRect(mktRect);
    //  弹出"网格化参数设置"的对话框,用户可以设置参数等信息
    if (configDlg.exec() != QDialog::Accepted) return;

    /*进入终极阶段,雷达二维可视化*/

    //  radarVisualDlg是新的雷达数据三维可视化小窗口
    TRadar3DWnd* radarVisualDlg = new TRadar3DWnd(this);
    //  用户关闭 radarVisualDlg 窗口时，相应的对象会被自动释放和删除
    radarVisualDlg->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    //  当 radarVisualDlg 对象被销毁时,mLastRadar3DWnd = nullptr;
    connect(radarVisualDlg, SIGNAL(destroyed(QObject*)), this, SLOT(radar3DWndDestroyed(QObject*)));

    //  获取雷达数据列表(将主页的2d雷达数据列表赋值给雷达数据三维可视化小窗口的雷达数据列表)
    radarVisualDlg->setRadarDataList(mpRaderLayer->getRaderDataList());
    //  获取当前显示的雷达索引(同上)
    radarVisualDlg->setRadarDataIndex(mpRaderLayer->getRaderDataIndex());
    // 	设置地图图像
    radarVisualDlg->setMapImage(&mapImage);
    // 	设置高程图像
    radarVisualDlg->setElevImage(&elevImage);
    // 	设置网格化范围（web墨卡托投影坐标）,使用的参数rect是最开始选择的没有扩大的矩形框大小
    radarVisualDlg->setGridRect(mktRect);
    //  获取颜色表
    radarVisualDlg->setColorTransferFunction(mpRaderLayer->getColorTransferFunction());
    //radarDialog->setGridSize(dialog.nx, dialog.ny, dialog.nz);
    if (!lastRect.isEmpty())
    {
        //  lastRect是雷达数据可视化小对象的窗口的位置和大小
        radarVisualDlg->setGeometry(lastRect);
    }
    
    //  显示radarVisualDlg窗口
    radarVisualDlg->show();
    //  渲染数据
    radarVisualDlg->render();
    //  将新的雷达数据三维可视化小窗口展示的数据赋值给mLastRadar3DWnd
    mLastRadar3DWnd = radarVisualDlg;
}

void TRadarVisualWnd::radar3DWndDestroyed(QObject*)
{
    mLastRadar3DWnd = nullptr;
}
//  文件->打开 打开雷达文件的回调函数
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
//  保存修改后的雷达数据
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
//  雷达索引改变回调
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
//  设置雷达图层是否播放动画
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
//  雷达2D动画超时回调
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