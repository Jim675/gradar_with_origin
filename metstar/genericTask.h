// **********************************************************************
//
// Copyright (c) 1996-2016 Beijing Metstar Radar, Inc. All rights reserved.
//
// This copy of the source code is licensed to you under the terms described in the
// METSTAR_LICENSE file included in this distribution.
//
// **********************************************************************
#ifndef RADAR_GENERIC_TASK_H
#define RADAR_GENERIC_TASK_H

#pragma pack(push,1)
#include <vector>

using std::vector;

//#include"taskConst.h"
//not speed of light in vacuum,refractrive index of air is included.
const float SPEED_OF_LIGHT = (float)299735000.0;//m/s

enum
{
    SCAN_SYNC_AUTO = 0,
    SCAN_SYNC_MANU = 1
};

enum
{
    WF_CS = 0,//cs mode
    WF_CD = 1,//doppler mode
    WF_CDX = 2,//cdx mode
    WF_RX_TEST = 3,
    WF_BATCH = 4,//batch mode
    WF_DPRF = 5, //dual prf
    WF_RPHASE = 6,//random phase
    WF_SZ = 7,//sz 8/64 code
    WF_SPRT = 8 //staggered prt	
};

enum
{
    SP_VOL = 0, //mulit ppi is included,marked as VOL_START,CUT_END,CUT_START,CUT_END....VOL_END 
    SP_PPI = 1, // single ppi, marked as VOL_START,VOL_END
    SP_RHI = 2,//  single rhi, marked as VOL_START,VOL_END
    SP_SECTOR = 3,// single ppi sector  ,marked as VOL_START,VOL_END
    SP_VOL_SECT = 4, //mulit sector scan is included ,marked same as volume scan
    SP_VOL_RHI = 5,//mulit rhi scan is included ,marked same as volume scan
    SP_MAN = 6 //mannual scan,or fixed scan,marked as PPI,radial num is 360/angluar_reso 
};

bool isPPIFull(int sc);
bool isPPISector(int sc);
bool isRHI(int sc);
bool isVolScan(int sc);
bool isPPI(int sc);

enum
{
    PRF_SINGLE = 1,
    PRF_2V3 = 2,
    PRF_3V4 = 3,
    PRF_4V5 = 4
};

//process mode
enum
{
    PM_PPP = 1,
    PM_FFT = 2,
    PM_RP = 3
};

enum
{
    POL_HORI = 0,// horizontal only
    POL_VERT = 1,//vertical only
    POL_ALT = 2,
    POL_SIMU = 3// simu star mode
};
#define  MAX_POL_TYPE_NUM 4 

#define  MAX_TSC_NUM 16
#define  TASK_NAME_LENGTH 32
#define  TASK_DESP_LENGTH 128
#define  MAX_MOM_NUM 64  //maxinum moment data number in raw product
#define  MAX_CUT_NUM 32 //maximum cut number for one task
#define  MAX_RAD_NUM 1000 //maximum radial number for one cut
#define  MAX_BIN_NUM 4000 //maximum bin number for one radial

enum
{
    QC_LOG = 1,//LOG THRESH CONTROL BIT
    QC_SQI = 2,//SQI THRESH CONTROL BIT
    QC_CCR = 4,//CCR THRESH CONTROL BIT
    QC_SIG = 8,//SIG THRESH CONTROL BIT
    QC_PMI = 16,
    QC_DPLOG = 32,
};

#define QC_MAX (QC_DPLOG+1)
#define QC_MASK_NUM 6
#define QC_MASK_CONJ "&"

//description of antenna rotation direction
enum
{
    ROT_CW = 1, //clockwise
    ROT_CCW = 2 //counter-clockwise
};

//ground clutter classfier
enum
{
    GCC_AP = 1,//all pass,treat all bins as weather signal,filter no bins then
    GCC_NP = 2,//none pass, treat all bins as ground clutter, filter all bins then
    GCC_RT = 3,//use real time ground clutter classfier CMD algorithm
    GCC_PRE = 4 // use predefined by pass map
};

//ground clutter filter
enum
{
    GCF_NONE = 0,//no filter,comp
    GCF_ADAPTIVE = 1,//fft adaptive
    GCF_FIXED = 2,// fft fixed
    GCF_VARI = 3,//fft varied
    GCF_VARSQ = 4,
    GCF_IIR = 5// IIR filter
};

enum
{
    WINDOW_NONE = -1,
    WINDOW_RECT = 0,//rectangle window
    WINDOW_HAMMING = 1,//hamming window
    WINDOW_BLACKMAN = 2,//blackman window
    WINDOW_ADAPTIVE = 3,//adaptive window
    WINDOW_CHEB80 = 4,//DOLPH-CHEBYSHEV window
    WINDOW_FUNC_NUM = 5// number of window function
};

//phase code type
enum
{
    PC_FIXED = 0,//fix phase
    PC_RANDOM = 1,//random phase 
    PC_SZ64 = 2,//sz 8/64 code 
    PC_TEST = 3, // test code ,download phasecode 0,1,2,3....127
    PC_POLY = 4
};

struct geneQcThresh
{
    float sqi;//signal quality index
    float sig;//weather signal thresh
    float csr;//cluter signal ratio
    float log;//singal noise ratio
    float cpa;//const phase aligment
    float pmi;//polarimetric meteo index
    float dplog;
    float spare[1];
};

struct geneQcMask
{
    int dbt; // 滤波前反射率
    int dbz; // 滤波后反射率
    int vel; // 径向速度
    int wid; // 谱宽
    int dpvar;//thresh apply to dual pol variable
    int spare[3];
};
struct geneFilterMask
{
    unsigned int interFilter : 1;
    unsigned int censorFilter : 1;
    unsigned int spekFilter1DLog : 1;
    unsigned int spekFilter1DDop : 1;
    unsigned int spekFilter2DLog : 1;
    unsigned int spekFilter2DDop : 1;
    unsigned int nebor : 1;//1 for enable noise estimation
    unsigned int gccal : 1;//1 for enable ground clutter calibration 
    unsigned int spared : 24;
};

int WaveformIndex(const char* swf);
const char* WaveformDesp(int wf);
int ScanTypeIndex(const char* st);
const char* ScanTypeDesp(int st);
const char* ProcessModeDesp(int pm);
const char* unfoldModeDesp(int um);
const char* PolDesp(int pol);
int PolType(const char* pol);
const char* QcMaskDesp(int qm);
int QcMaskType(const char* desp);
int parseQCMask(const char* str);
void formatQcMask(int mask, char* str);
const char* RadStateDesp(int s);
int RadStateType(const char* desp);

const char* scanSyncDesp(int);

struct geneCutConfig
{
    /** although pol scantype and pulsewidth should not changed within a VCP,we still put it
      in cut configuration ,make controlword easy.
    */
    int processMode;
    int waveForm;
    //for cut have one prf, prf2 and  maxrange2 should be ignored or same as prf and maxrange
    // for cut have two prfs prf <-> maxrange  ,prf2 <->maxrange2	
    // prf2 is used in dprf and batch mode, prf2 is the low prf (cs prf in batch mode)
    float prf;// in hz 脉冲重复频率 1
    float prf2;//in hz 脉冲重复频率 2
    int unfoldMode;
    float az;
    float el;
    float startAngle;
    float endAngle;
    float angleReso;
    float scanSpeed;// use sign as cw/rcw
    int logReso;
    int dopReso;
    int maxRange; // 最大距离 1
    //maxrange2 will be enabled for CD mode with random phase code or SZ phase code,
    //which means the max unfold range,2 times for RP and 3 times for SZ 
    int maxRange2; // 最大距离 2
    int startRange; // 起始距离
    int samples;
    int samples2;// for batch modes'sur samples
    int phaseMode; // 相位编码模式
    float atmos; // 大气衰减
    float nyquist; // 最大不模糊速度
    long long momMask;//moments available in this cut, bit
    long long momSizeMask;//mask for moment data size 1 for 16bit 0 for 8bit
    geneFilterMask filterMask; // 滤波设置掩码
    geneQcThresh qcThresh; // 各种门限
    geneQcMask qcMask; // 各种掩码
    int scanSync; // 扫描同步标志
    int direction; // antenna rotate direction, CW or CCW // 天线运行方向
    //all for ground clutter filter
    short gcCf;// 地物杂波图类型
    short gcFilter; // 滤波窗口类型
    short gcNw; // 地物滤波宽度
    short gcWin; // 滤波窗口类型

    char twins;
    //ground clutter filter related options
    unsigned char gcfMinWidth;
    unsigned char gcfEdgePoints;
    unsigned char gcfSlopePoints;
    int spare[17];
};

typedef vector<unsigned short> shortVec;

short getMomNum(long long momMask);
short getMomList(long long momMask, shortVec& seq);
long long getMomMask(shortVec seq);
/**  get moment mask from predefined waveform*/
long long getMomMaskFromWF(int wf);
bool isDataAvail(long long momMask, short dt);
void clrMomMask(short dt, long long& momMask);
void setMomMask(short dt, long long& momMask);
void getMomSizeList(long long msm, shortVec& seq);
int getMomDataSize(long long msm, int rdt);
int GCFilterIndex(const char* desp);
const char* GCFilterDesp(int gcf);
int GCWinIndex(const char* desp);
const char* GCWinDesp(int win);
void formatMomMask(long long momMask, long long momSizeMask, char* buffer);
void finalUpdateCut(geneCutConfig* pcc);
int compAmbRange(int prf);

struct geneTaskConfig
{
    char name[TASK_NAME_LENGTH];
    char desp[TASK_DESP_LENGTH];
    int pol; // 极化方式
    int scanType; // 扫描任务类型
    int pulseWidth;
    int startTime;
    int cutNum;
    float horNoise;
    float verNoise;
    float horCalib;
    float verCalib;
    float horNoiseTemp;
    float verNoiseTemp;
    float zdrCal;
    float phaseCal;
    float ldrCal;
    int spare[10];
};

struct geneTscConfig
{
    char taskname[TASK_NAME_LENGTH];
    int startTime;  //seconds of task start time in one day
    int stopTime;   //seconds of task stop time in one day 
    int period; // seconds of task schedule period 
    char spared[20];
};

struct geneTaskSchedule
{
    char name[TASK_NAME_LENGTH];
    char desp[TASK_DESP_LENGTH];
    int num;
    //	geneTscConfig tscConfig[MAX_TSC_NUM];
    char spared[12];
};

#define SITE_CODE_LENGTH 8
#define SITE_NAME_LENGTH 32

//ALL CINRAD RADAR MODEL 
//S BAND
#define MODEL_SA 1
#define MODEL_SB 2
#define MODEL_SC 3
//C BAND
#define MODEL_CA 33
#define MODEL_CB 34
#define MODEL_CC 35
#define MODEL_CCJ 36
#define MODEL_CD 37
//X BAND
#define MODEL_XA 65 

struct geneSiteConfig
{
    char code[SITE_CODE_LENGTH];
    char name[SITE_NAME_LENGTH];
    float lat;
    float lon;
    int height;
    int ground;
    float freq;
    float beamwidth;
    float beamwidthVert;
    int rdaVersion;//rdasc source code svn version
    short model;// refer to MODEL_xx
    char spared[2];
    int spare[13];
};

void getWaveformExplain(char* buf);
//void getPolAvailMoms(int pol, int chan, shortVec& dt);
const char* PCDesp(int pc);
int PCIndex(const char* desp);
const char* GCCDesp(int gcc);
int GCCIndex(const char* desp);

#pragma pack(pop)
#endif
