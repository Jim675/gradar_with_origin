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
#include <QDateTime>
#include <QRadioButton>
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
	dialog.setWindowTitle("��ѡ���״������ļ�");
	dialog.setDirectory(path);
	dialog.setFilter(QDir::Filter::Files);
	dialog.setNameFilter("�״������ļ�(*.ar2)");
	dialog.setFileMode(QFileDialog::FileMode::ExistingFiles);
	if (dialog.exec() != QDialog::Accepted) return;

	GConfig::mLastSelectPath = dialog.directory().absolutePath();
	QStringList fileList = dialog.selectedFiles();
	if (fileList.isEmpty()) return;

    fileList.sort(Qt::CaseInsensitive);

    // ���ļ��б�
	loadRadarFiles(fileList);
}

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
    // �����ǰ�ڶ����ͺ���
	if (mIsAnimate) return;

	//auto msg = QString("clicked:") + QString::number(index.row());
    int row = ui.mFileListWidget->currentRow();
	mpRaderLayer->setRaderDataIndex(row);
    mpRaderLayer->setSurfaceIndex(mELIndex);
	// ֪ͨ�Լ�
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

// ѡ����ο�
void TRadarVisualWnd::on_actSelectRect_triggered(bool checked)
{
    if (checked) {
        mpView->setSelectMode(SELECT_RECT);
    } else {
        mpView->setSelectMode(SELECT_NONE);
    }
}

// ѡ����ο�ص� rectΪ��Ļ����
void TRadarVisualWnd::onSelectedRect(const QRect& rect)
{
    QRect lastRect;
	if (mLastRadar3DWnd)
	{
        lastRect = mLastRadar3DWnd->geometry();
        delete mLastRadar3DWnd;
		mLastRadar3DWnd = nullptr;
	}

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
    // ��������
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

// �ļ�->�� ���״��ļ��Ļص�����
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
        dialog->setWindowTitle("���ڼ����״������ļ�");
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
        std::string stdPath = qstrPath.toLocal8Bit();
        basedataImage bdi;
        const int ret = loadBasedataImage(stdPath.c_str(), &bdi);
        if (ret != 1) {
            const int code = QMessageBox::critical(this,
                "����",
                QString("%1 ��ʧ�ܣ�").arg(qstrPath),
                QMessageBox::StandardButton::Abort,
                QMessageBox::StandardButton::Ignore);
            if (code == QMessageBox::StandardButton::Abort) {
                break;
            }
        }
        // ��ȡ�״�����
        GRadarVolume* volume = new GRadarVolume();
        volume->extractData(bdi, RDT_DBZ, -1000);

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

// �״������ı�ص�
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
        QString hint = QString::number(pSurf->el, 'f', 2) + "��";

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

// �����״�ͼ���Ƿ񲥷Ŷ���
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

// �״�2D������ʱ�ص�
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

