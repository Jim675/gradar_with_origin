// **********************************************************************
//
// Copyright (c) 1996-2016 Beijing Metstar Radar, Inc. All rights reserved.
//
// This copy of the source code is licensed to you under the terms described in the
// METSTAR_LICENSE file included in this distribution.
//
// **********************************************************************
#pragma once

#ifndef RADAR_DATA_TYPE_H
#define RADAR_DATA_TYPE_H

// number in 64 is reserved for RDA use, it's the data type may be in basedata file
const short MAX_RESV_RDT = 64;
const short MIN_RESV_RDT = 0;

// first class, data type generated by RDA in basedata directly
// 数据类型的掩码
const short RDT_DBT = 1;
const short RDT_DBZ = 2;
const short RDT_VEL = 3;
const short RDT_WID = 4;
const short RDT_SQI = 5;
const short RDT_CPA = 6;        //clutter phase alignment
const short RDT_ZDR = 7;
const short RDT_LDR = 8;        //ldrh and ldrv
const short RDT_CC = 9;         //phhv, correlation coffient
const short RDT_PDP = 10;
const short RDT_KDP = 11;
const short RDT_CP = 12;        //clutter probability 0-1
const short RDT_FLAG = 13;      //rvp flag data
const short RDT_HCL = 14;       // hydro class
const short RDT_CF = 15;        //clutter flag
const short RDT_SNR = 16;       //ground clutter filtered SNR for h channel
const short RDT_SNRV = 17;      //ground clutter filtered SNR for v channel
const short RDT_SNRT = 18;      //unfiltered SNR for h channel used by mainb for cs/cd 
const short RDT_POTS = 19;    //phase of time series
const short RDT_N = 20;     // refractivity from ground clutter 
const short RDT_COP = 21; //change of phase from POTS
//const short RDT_SDDBZ = 22;
const short RDT_DEG = 23;       // for wind field
const short RDT_PROB = 24;      // probability 0-1
const short RDT_SPEC = 25;      //spectrum


//second class,data type  generated  by RPG QC alrigthm
const short RDT_DBZC = 32;
const short RDT_VELC = 33;
const short RDT_WIDC = 34;
const short RDT_ZDRC = 35;
const short RDT_PDPC = 36;
const short RDT_KDPC = 37;
const short RDT_CCC = 38;
const short RDT_LDRC = 39;

//data type will not be in basedata
const short RDT_RR = 71;        //rain rate  mm/hr
const short RDT_HGT = 72;       //height m
const short RDT_VIL = 73;       //
const short RDT_SHR = 74;       //shear
const short RDT_RAIN = 75;      //rain mm
const short RDT_RMS = 76;       // root mean square
const short RDT_CTR = 77;       // contour
const short RDT_SWIS = 78;      //swis product for TianJin University
const short RDT_RDD = 79;       //mean diameter of rain drop in mm
const short RDT_RDN = 80;       //density ofrain drops 1/m**3
const short RDT_ML = 81;
const short RDT_OCR = 82;
const short RDT_TURB = 83;
const short RDT_DSD = 84;
#endif
