#pragma once

constexpr double PI = 3.1415926535897932384626433832795;

// ����뾶(webī���г���뾶)
//constexpr double RE = 6371393;
constexpr double RE = 6378137;

// ��Ч����뾶
//constexpr double RM = RE * 4.0 / 3.0;

// �״ﴫ��Բ��·���İ뾶Ϊ����뾶4��
constexpr double RN = RE * 4;

// 2����Ч����뾶
// constexpr double RM_2 = RM * 2;

// ������ܳ�Equatorial half circumference
constexpr double EHC = 20037508.3427892430765884088807;

// �Ƕ�ת����
constexpr double toRadian(double angle)
{
    return angle * (PI / 180.0);
}

// ����ת�Ƕ�
constexpr double toAngle(double radian)
{
    return radian * (180.0 / PI);
}