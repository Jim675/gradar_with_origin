#pragma once

constexpr double PI = 3.1415926535897932384626433832795;

// 地球半径(web墨卡托赤道半径)
//constexpr double RE = 6371393;
constexpr double RE = 6378137;

// 等效地球半径
//constexpr double RM = RE * 4.0 / 3.0;

// 雷达传播圆弧路径的半径为地球半径4倍
constexpr double RN = RE * 4;

// 2倍等效地球半径
// constexpr double RM_2 = RM * 2;

// 赤道半周长Equatorial half circumference
constexpr double EHC = 20037508.3427892430765884088807;

// 角度转弧度
constexpr double toRadian(double angle)
{
    return angle * (PI / 180.0);
}

// 弧度转角度
constexpr double toAngle(double radian)
{
    return radian * (180.0 / PI);
}