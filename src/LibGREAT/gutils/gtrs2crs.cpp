/**
 * @file         gtrs2crs.cpp
 * @author       GREAT-WHU (https://github.com/GREAT-WHU)
 * @brief        calculate the rotation matrix from TRS to CRS and the corresponding partial derivation matrix
 * @version      1.0
 * @date         2024-08-29
 *
 * @copyright Copyright (c) 2024, Wuhan University. All rights reserved.
 *
 */
#include "gutils/gtrs2crs.h"
#include "gutils/gsysconv.h"

using namespace std;

namespace great
{

t_pudaily::t_pudaily()
{
    xpole = 0.0;
    ypole = 0.0;
    ut1 = 0.0;
}

t_EpochNutation::t_EpochNutation()
{
    T = 0;
    Pi = 0;
    Ep = 0;
}

t_zonaltide::t_zonaltide()
{
    ut1 = 0.0;
    lod = 0.0;
    omega = 0.0;
}

t_gtrs2crs::t_gtrs2crs() : _poleut1(nullptr)
{
    _cver = "06";
}

t_gtrs2crs::t_gtrs2crs(string cver)
{
    _cver = cver;
}

t_gtrs2crs::t_gtrs2crs(bool erptab, t_gpoleut1 *poleut1)
{
    _erptab = erptab;
    _poleut1 = poleut1;
    _cver = "06";
}

t_gtrs2crs::t_gtrs2crs(bool erptab, t_gpoleut1 *poleut1, string cver)
{
    _erptab = erptab;
    _poleut1 = poleut1;
    _cver = cver;
}

t_gtrs2crs &t_gtrs2crs::operator=(const t_gtrs2crs &Other)
{
    _erptab = Other._erptab;
    _poleut1 = Other._poleut1;
    _erptab = Other._erptab;
    _epo = Other._epo;
    _pudata = Other._pudata;
    _tdt = Other._tdt;
    _qmat = Other._qmat;
    _epsa = Other._epsa;
    _xpole = Other._xpole;
    _ypole = Other._ypole;
    _gmst = Other._gmst;
    _rotmat = Other._rotmat;
    _rotdu = Other._rotdu;
    _rotdx = Other._rotdx;
    _rotdy = Other._rotdy;

    for (int i = 0; i < 5; i++)
    {
        _arg[i] = Other._arg[i];
    }
    return *this;
}

// return the values needed by other functions
Matrix &t_gtrs2crs::getRotMat()
{
    return _rotmat;
};
Matrix &t_gtrs2crs::getMatDu()
{
    return _rotdu;
};
Matrix &t_gtrs2crs::getMatDx()
{
    return _rotdx;
};
Matrix &t_gtrs2crs::getMatDy()
{
    return _rotdy;
};
Matrix &t_gtrs2crs::getMatDp()
{
    return _rotdpsi;
};
Matrix &t_gtrs2crs::getMatDe()
{
    return _rotdeps;
};
double t_gtrs2crs::getXpole()
{
    return _xpole;
};
double t_gtrs2crs::getYpole()
{
    return _ypole;
};
double t_gtrs2crs::getGmst()
{
    return _gmst;
};
t_gtime t_gtrs2crs::getCurtEpoch()
{
    return _tdt;
};

void t_gtrs2crs::calcRotMat(const t_gtime &epoch, const bool &ldxdpole, const bool &ldydpole, const bool &ldudpole,
                            const bool &ldX, const bool &ldY)
{
    double era; // earth rotation angle in radian
    double sp;
    double psi, eps;

    double dUt1_tai = 0.0;
    string strTemp1;
    string strTemp2;
    double dXhelp[6] = {0.0};
    double dX = 0.0;
    double dY = 0.0;

    _tdt = epoch;
    _xpole = 0.0;
    _ypole = 0.0;
    dUt1_tai = 0.0;
    if (!_erptab)
    {
        strTemp1 = "";
        strTemp2 = "";
        _calPoleut1(_tdt, dXhelp, _poleut1, strTemp2);

        _xpole = dXhelp[0] / RAD2SEC;
        _ypole = dXhelp[1] / RAD2SEC;
        dUt1_tai = dXhelp[2];

        dX = dXhelp[3] / RAD2SEC;
        dY = dXhelp[4] / RAD2SEC;
        if (abs(dX) < 10e-10 || abs(dY) < 10e-10)
        {
            double sigdX = 0.0;
            double sigdY = 0.0;
            _nutFCN(_tdt.dmjd(), &dX, &dY, &sigdX, &sigdY);
            dX = dX / 1000000 * dAS2R;
            dY = dY / 1000000 * dAS2R;
        }
    }
    dX = 0.0;
    dY = 0.0;

    t_gtime sTUT1;
    sTUT1.from_mjd(_tdt.mjd(), int(_tdt.sod() + _tdt.dsec() + (dUt1_tai - 32.184)),
                   (_tdt.sod() + _tdt.dsec() + (dUt1_tai - 32.184)) -
                       int(_tdt.sod() + _tdt.dsec() + (dUt1_tai - 32.184)));

    sp = _sp2000(_tdt.mjd(), (_tdt.sod() + _tdt.dsec()) / 86400.0);

    era = _era2000(sTUT1.mjd() * 1.0, (sTUT1.sod() + sTUT1.dsec()) / 86400.0);

    // IAU2000A nutation model
    _nutInt(_tdt.dmjd(), &psi, &eps, 0.0625);

    // for both IAU2000 and IAU2006
    if (_cver.find("00") != string::npos)
    {
        _process2000(_tdt.dmjd(), psi, eps, _qmat, false, false);
        _gst2000(_tdt.dmjd(), era, psi);
    }
    else
    {
        _process2006(_tdt.dmjd(), psi, eps, _qmat, false, false);
        _gst2006(_tdt.dmjd(), era, psi);
    }

    double X = 0.0;
    double Y = 0.0;
    _iau_bpn2xy(_qmat.t(), &X, &Y);
    if (_cver.find("00") != string::npos)
    {
        _iau_XY00(_tdt.mjd(), (_tdt.sod() + _tdt.dsec()) / 86400.0, &X, &Y);
    }
    else
    {
        _iau_XY06(_tdt.mjd(), (_tdt.sod() + _tdt.dsec()) / 86400.0, &X, &Y);
    }
    double S = _iau_CIO_locator(_tdt.mjd(), (_tdt.sod() + _tdt.dsec()) / 86400.0, X, Y);
    X += dX;
    Y += dY;

    double r2 = X * X + Y * Y;
    double E = 0.0;
    if (r2 != 0)
    {
        E = atan2(Y, X);
    }
    double V = sqrt(r2);
    double Z = sqrt(1 - r2);
    double D = atan(V / Z);

    // partial of S/E/D w.r.t. X and Y
    double dSdX = -Y / 2;
    double dSdY = -X / 2;
    double dEdX = -Y / r2;
    double dEdY = X / r2;
    double dDdX = X / (Z * V);
    double dDdY = Y / (Z * V);

    vector<Matrix> rotx, roty, rotsp, rotu;
    /* Y-Pole matrix */
    calcProcMat(ldydpole, 1, _ypole, roty);
    /* X-Pole matrix */
    calcProcMat(ldxdpole, 2, _xpole, rotx);
    /* SP matrix */
    calcProcMat(false, 3, -sp, rotsp);
    /* ERA matrix */
    calcProcMat(ldudpole, 3, -era, rotu);

    /* Q matrix */
    vector<Matrix> rotdS, rotdEp, rotdD, rotdE;
    calcProcMat((ldX || ldY), 3, S, rotdS);
    calcProcMat((ldX || ldY), 3, E, rotdEp);
    calcProcMat((ldX || ldY), 2, -D, rotdD);
    calcProcMat((ldX || ldY), 3, -E, rotdE);

    /* calculate _rotmat */
    Matrix mathlp;
    mathlp = rotdE[0] * rotdD[0];
    mathlp = mathlp * rotdEp[0];
    mathlp = mathlp * rotdS[0];

    if (ldudpole)
    {
        rotu[1] = mathlp * rotu[1];
    }
    mathlp = mathlp * rotu[0];
    mathlp = mathlp * rotsp[0];
    if (ldudpole)
    {
        rotu[1] = rotu[1] * rotsp[0];
    }
    if (ldxdpole)
    {
        rotx[1] = mathlp * rotx[1];
    }
    mathlp = mathlp * rotx[0];
    if (ldudpole)
    {
        rotu[1] = rotu[1] * rotx[0];
    }
    if (ldydpole)
    {
        roty[1] = mathlp * roty[1];
    }
    _rotmat = mathlp * roty[0];
    if (ldudpole)
    {
        rotu[1] = rotu[1] * roty[0];
        rotu[1] = rotu[1] * _rr;
    }
    if (ldxdpole)
    {
        rotx[1] = rotx[1] * roty[0];
    }

    Matrix dRdX, dRdY;
    if (ldX)
    {
        dRdX = rotdE[1] * rotdD[0] * rotdEp[0] * rotdS[0] * (-dEdX) +
               rotdE[0] * rotdD[1] * rotdEp[0] * rotdS[0] * (-dDdX) +
               rotdE[0] * rotdD[0] * rotdEp[1] * rotdS[0] * (dEdX) + rotdE[0] * rotdD[0] * rotdEp[0] * rotdS[1] * dSdX;
        dRdX = dRdX * rotu[0] * rotsp[0] * rotx[0] * roty[0];
    }
    if (ldY)
    {
        dRdY = rotdE[1] * rotdD[0] * rotdEp[0] * rotdS[0] * (-dEdY) +
               rotdE[0] * rotdD[1] * rotdEp[0] * rotdS[0] * (-dDdY) +
               rotdE[0] * rotdD[0] * rotdEp[1] * rotdS[0] * (dEdY) + rotdE[0] * rotdD[0] * rotdEp[0] * rotdS[1] * dSdY;
        dRdY = dRdY * rotu[0] * rotsp[0] * rotx[0] * roty[0];
    }

    _xpole = _xpole * RAD2SEC;
    _ypole = _ypole * RAD2SEC;
    _rotdx = rotx[1];
    _rotdy = roty[1];
    _rotdu = rotu[1];
    _rotdpsi = dRdX;
    _rotdeps = dRdY;
}

void t_gtrs2crs::calcProcMat(const bool &partial, const int &axis, const double &angle, vector<Matrix> &rot)
{
    rot.clear();
    Matrix rotmat(0.0, 3, 3);
    Matrix drotmat(0.0, 3, 3);

    int i, j;
    double sina, cosa;
    cosa = cos(angle);
    sina = sin(angle);
    rotmat(axis, axis) = 1.0;
    i = axis + 1;
    j = axis + 2;
    if (i > 3)
        i = i - 3;
    if (j > 3)
        j = j - 3;

    rotmat(i, i) = cosa;
    rotmat(j, j) = cosa;
    rotmat(i, j) = sina;
    rotmat(j, i) = -sina;
    if (partial)
    {
        drotmat(i, i) = -sina;
        drotmat(j, j) = -sina;
        drotmat(i, j) = cosa;
        drotmat(j, i) = -cosa;
    }
    rot.push_back(rotmat);
    rot.push_back(drotmat);
    return;
}

void t_gtrs2crs::_calPoleut1(t_gtime &t, double *x, t_gpoleut1 *poleut1, string type)
{
    // tt to tai
    double rmjd_utc = t.dmjd() - 32.184 / 86400.0;
    // tai to utc
    int leap_sec = t.leapsec();
    rmjd_utc -= leap_sec * 1.0 / 86400.0;
    t_gtime utc(t_gtime::UTC);
    utc.from_dmjd(rmjd_utc);

    int i;
    double dutlzonaltide, alpha;
    t_gtriple xyu;
    t_gtriple pmut1_ocean;
    t_gtime t0;
    t0.from_mjd(_tdt.mjd(), 0, 0.0);

    double dut1_zonaltide = 0.0;
    t_gtime t1;
    t1.from_mjd(_tdt.mjd() + 1, 0, 0.0);

    vector<t_pudaily> rec;
    t_pudaily rec0, rec1;
    rec0.time = t0;
    rec1.time = t1;
    map<string, double> data0, data1;
    data0 = (*(poleut1->getPoleUt1DataMap()))[rec0.time];
    rec0.xpole = data0["XPOLE"];
    rec0.ypole = data0["YPOLE"];
    rec0.ut1 = data0["UT1-TAI"];
    rec.push_back(rec0);
    data1 = (*(poleut1->getPoleUt1DataMap()))[rec1.time];
    rec1.xpole = data1["XPOLE"];
    rec1.ypole = data1["YPOLE"];
    rec1.ut1 = data1["UT1-TAI"];
    rec.push_back(rec1);

    for (i = 0; i < 2; i++)
    {
        if ((*poleut1).getUt1Mode() != "UT1R")
        {
            _tide_corrections(rec[i].time, xyu);
            dutlzonaltide = _tideCor2(rec[i].time.mjd());
        }
        rec[i].xpole = rec[i].xpole - xyu[0];
        rec[i].ypole = rec[i].ypole - xyu[1];
        rec[i].ut1 = rec[i].ut1 - xyu[2] - dut1_zonaltide;
    }
    x[0] = 0;
    x[1] = 0;
    x[2] = 0;
    alpha = (t.dmjd() - rec[0].time.mjd()) / (*poleut1).getIntv();
    x[0] = rec[0].xpole + alpha * (rec[1].xpole - rec[0].xpole);
    x[1] = rec[0].ypole + alpha * (rec[1].ypole - rec[0].ypole);
    x[2] = rec[0].ut1 + alpha * (rec[1].ut1 - rec[0].ut1);
    x[3] = data0["DPSI"] + alpha * (data1["DPSI"] - data0["DPSI"]);
    x[4] = data0["DEPSI"] + alpha * (data1["DEPSI"] - data0["DEPSI"]);

    if (type != "UT1R")
    {
        _tide_corrections(t, xyu);
        dutlzonaltide = _tideCor2(t.dmjd());
        x[0] = x[0] + xyu[0];
        x[1] = x[1] + xyu[1];
        x[2] = x[2] + xyu[2] + dutlzonaltide;
    }
}

void t_gtrs2crs::_tide_corrections(t_gtime &t, t_gtriple &xyu)
{
    double stepsize = 0.015 * 86400.0; // unit sec
    double rmjd = t.dmjd();
    static bool isfirst = true;
    static t_pudaily tb0, tb1;
    if (isfirst)
    {
        tb0.time = LAST_TIME;
        tb1.time = FIRST_TIME;
        isfirst = false;
    }

    while (t > tb1.time || t < tb0.time)
    {
        if (t > (tb1.time + stepsize) || t < (tb0.time - stepsize))
        {

            tb0.time = t - stepsize;
            tb1.time = t + stepsize;

            tb0 = _tideCor1Cal(tb0);
            tb1 = _tideCor1Cal(tb1);
        }

        else if (t < tb0.time)
        {
            tb1 = tb0;
            tb0.time = tb0.time - stepsize;
            tb0 = _tideCor1Cal(tb0);
        }
        else
        {
            tb0 = tb1;
            tb1.time = tb1.time + stepsize;
            tb1 = _tideCor1Cal(tb1);
        }
    }

    double mjds[2] = {tb0.time.dmjd(), tb1.time.dmjd()};
    double xpoles[2] = {tb0.xpole, tb1.xpole};
    double ypoles[2] = {tb0.ypole, tb1.ypole};
    double ut1s[2] = {tb0.ut1, tb1.ut1};

    xyu[0] = _interpolation(1, 2, mjds, xpoles, rmjd);
    xyu[1] = _interpolation(1, 2, mjds, ypoles, rmjd);
    xyu[2] = _interpolation(1, 2, mjds, ut1s, rmjd);

    return;
}

t_pudaily &t_gtrs2crs::_tideCor1Cal(t_pudaily &b)
{
    t_gtriple eop;
    _PMUT1_OCEANS(b.time, eop);
    b.xpole = eop[0];
    b.ypole = eop[1];
    b.ut1 = eop[2];

    double xtemp[2];
    _PMSDNUT2(b.time, xtemp);
    b.xpole = b.xpole + xtemp[0];
    b.ypole = b.ypole + xtemp[1];

    _UTLIBR(b.time, xtemp);
    b.ut1 = b.ut1 + xtemp[0];

    b.xpole = b.xpole * 1e-6;
    b.ypole = b.ypole * 1e-6;
    b.ut1 = b.ut1 * 1e-6;

    return b;
}

void t_gtrs2crs::_PMUT1_OCEANS(t_gtime &t, t_gtriple &eop)
{
    double table[71][12] = {{1.0, -1.0, 0.0, -2.0, -2.0, -2.0, -0.05, 0.94, -0.94, -0.05, 0.396, -0.078},
                            {1.0, -2.0, 0.0, -2.0, 0.0, -1.0, 0.06, 0.64, -0.64, 0.06, 0.195, -0.059},
                            {1.0, -2.0, 0.0, -2.0, 0.0, -2.0, 0.30, 3.42, -3.42, 0.30, 1.034, -0.314},
                            {1.0, 0.0, 0.0, -2.0, -2.0, -1.0, 0.08, 0.78, -0.78, 0.08, 0.224, -0.073},
                            {1.0, 0.0, 0.0, -2.0, -2.0, -2.0, 0.46, 4.15, -4.15, 0.45, 1.187, -0.387},
                            {1.0, -1.0, 0.0, -2.0, 0.0, -1.0, 1.19, 4.96, -4.96, 1.19, 0.966, -0.474},
                            {1.0, -1.0, 0.0, -2.0, 0.0, -2.0, 6.24, 26.31, -26.31, 6.23, 5.118, -2.499},
                            {1.0, 1.0, 0.0, -2.0, -2.0, -1.0, 0.24, 0.94, -0.94, 0.24, 0.172, -0.090},
                            {1.0, 1.0, 0.0, -2.0, -2.0, -2.0, 1.28, 4.99, -4.99, 1.28, 0.911, -0.475},
                            {1.0, 0.0, 0.0, -2.0, 0.0, 0.0, -0.28, -0.77, 0.77, -0.28, -0.093, 0.070},
                            {1.0, 0.0, 0.0, -2.0, 0.0, -1.0, 9.22, 25.06, -25.06, 9.22, 3.025, -2.280},
                            {1.0, 0.0, 0.0, -2.0, 0.0, -2.0, 48.82, 132.91, -132.90, 48.82, 16.020, -12.069},
                            {1.0, -2.0, 0.0, 0.0, 0.0, 0.0, -0.32, -0.86, 0.86, -0.32, -0.103, 0.078},
                            {1.0, 0.0, 0.0, 0.0, -2.0, 0.0, -0.66, -1.72, 1.72, -0.66, -0.194, 0.154},
                            {1.0, -1.0, 0.0, -2.0, 2.0, -2.0, -0.42, -0.92, 0.92, -0.42, -0.083, 0.074},
                            {1.0, 1.0, 0.0, -2.0, 0.0, -1.0, -0.30, -0.64, 0.64, -0.30, -0.057, 0.050},
                            {1.0, 1.0, 0.0, -2.0, 0.0, -2.0, -1.61, -3.46, 3.46, -1.61, -0.308, 0.271},
                            {1.0, -1.0, 0.0, 0.0, 0.0, 0.0, -4.48, -9.61, 9.61, -4.48, -0.856, 0.751},
                            {1.0, -1.0, 0.0, 0.0, 0.0, -1.0, -0.90, -1.93, 1.93, -0.90, -0.172, 0.151},
                            {1.0, 1.0, 0.0, 0.0, -2.0, 0.0, -0.86, -1.81, 1.81, -0.86, -0.161, 0.137},
                            {1.0, 0.0, -1.0, -2.0, 2.0, -2.0, 1.54, 3.03, -3.03, 1.54, 0.315, -0.189},
                            {1.0, 0.0, 0.0, -2.0, 2.0, -1.0, -0.29, -0.58, 0.58, -0.29, -0.062, 0.035},
                            {1.0, 0.0, 0.0, -2.0, 2.0, -2.0, 26.13, 51.25, -51.25, 26.13, 5.512, -3.095},
                            {1.0, 0.0, 1.0, -2.0, 2.0, -2.0, -0.22, -0.42, 0.42, -0.22, -0.047, 0.025},
                            {1.0, 0.0, -1.0, 0.0, 0.0, 0.0, -0.61, -1.20, 1.20, -0.61, -0.134, 0.070},
                            {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.54, 3.00, -3.00, 1.54, 0.348, -0.171},
                            {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -77.48, -151.74, 151.74, -77.48, -17.620, 8.548},
                            {1.0, 0.0, 0.0, 0.0, 0.0, -1.0, -10.52, -20.56, 20.56, -10.52, -2.392, 1.159},
                            {1.0, 0.0, 0.0, 0.0, 0.0, -2.0, 0.23, 0.44, -0.44, 0.23, 0.052, -0.025},
                            {1.0, 0.0, 1.0, 0.0, 0.0, 0.0, -0.61, -1.19, 1.19, -0.61, -0.144, 0.065},
                            {1.0, 0.0, 0.0, 2.0, -2.0, 2.0, -1.09, -2.11, 2.11, -1.09, -0.267, 0.111},
                            {1.0, -1.0, 0.0, 0.0, 2.0, 0.0, -0.69, -1.43, 1.43, -0.69, -0.288, 0.043},
                            {1.0, 1.0, 0.0, 0.0, 0.0, 0.0, -3.46, -7.28, 7.28, -3.46, -1.610, 0.187},
                            {1.0, 1.0, 0.0, 0.0, 0.0, -1.0, -0.69, -1.44, 1.44, -0.69, -0.320, 0.037},
                            {1.0, 0.0, 0.0, 0.0, 2.0, 0.0, -0.37, -1.06, 1.06, -0.37, -0.407, -0.005},
                            {1.0, 2.0, 0.0, 0.0, 0.0, 0.0, -0.17, -0.51, 0.51, -0.17, -0.213, -0.005},
                            {1.0, 0.0, 0.0, 2.0, 0.0, 2.0, -1.10, -3.42, 3.42, -1.09, -1.436, -0.037},
                            {1.0, 0.0, 0.0, 2.0, 0.0, 1.0, -0.70, -2.19, 2.19, -0.70, -0.921, -0.023},
                            {1.0, 0.0, 0.0, 2.0, 0.0, 0.0, -0.15, -0.46, 0.46, -0.15, -0.193, -0.005},
                            {1.0, 1.0, 0.0, 2.0, 0.0, 2.0, -0.03, -0.59, 0.59, -0.03, -0.396, -0.024},
                            {1.0, 1.0, 0.0, 2.0, 0.0, 1.0, -0.02, -0.38, 0.38, -0.02, -0.253, -0.015},
                            {2.0, -3.0, 0.0, -2.0, 0.0, -2.0, -0.49, -0.04, 0.63, 0.24, -0.089, -0.011},
                            {2.0, -1.0, 0.0, -2.0, -2.0, -2.0, -1.33, -0.17, 1.53, 0.68, -0.224, -0.032},
                            {2.0, -2.0, 0.0, -2.0, 0.0, -2.0, -6.08, -1.61, 3.13, 3.35, -0.637, -0.177},
                            {2.0, 0.0, 0.0, -2.0, -2.0, -2.0, -7.59, -2.05, 3.44, 4.23, -0.745, -0.222},
                            {2.0, 0.0, 1.0, -2.0, -2.0, -2.0, -0.52, -0.14, 0.22, 0.29, -0.049, -0.015},
                            {2.0, -1.0, -1.0, -2.0, 0.0, -2.0, 0.47, 0.11, -0.10, -0.27, 0.033, 0.013},
                            {2.0, -1.0, 0.0, -2.0, 0.0, -1.0, 2.12, 0.49, -0.41, -1.23, 0.141, 0.058},
                            {2.0, -1.0, 0.0, -2.0, 0.0, -2.0, -56.87, -12.93, 11.15, 32.88, -3.795, -1.556},
                            {2.0, -1.0, 1.0, -2.0, 0.0, -2.0, -0.54, -0.12, 0.10, 0.31, -0.035, -0.015},
                            {2.0, 1.0, 0.0, -2.0, -2.0, -2.0, -11.01, -2.40, 1.89, 6.41, -0.698, -0.298},
                            {2.0, 1.0, 1.0, -2.0, -2.0, -2.0, -0.51, -0.11, 0.08, 0.30, -0.032, -0.014},
                            {2.0, -2.0, 0.0, -2.0, 2.0, -2.0, 0.98, 0.11, -0.11, -0.58, 0.050, 0.022},
                            {2.0, 0.0, -1.0, -2.0, 0.0, -2.0, 1.13, 0.11, -0.13, -0.67, 0.056, 0.025},
                            {2.0, 0.0, 0.0, -2.0, 0.0, -1.0, 12.32, 1.00, -1.41, -7.31, 0.605, 0.266},
                            {2.0, 0.0, 0.0, -2.0, 0.0, -2.0, -330.15, -26.96, 37.58, 195.92, -16.195, -7.140},
                            {2.0, 0.0, 1.0, -2.0, 0.0, -2.0, -1.01, -0.07, 0.11, 0.60, -0.049, -0.021},
                            {2.0, -1.0, 0.0, -2.0, 2.0, -2.0, 2.47, -0.28, -0.44, -1.48, 0.111, 0.034},
                            {2.0, 1.0, 0.0, -2.0, 0.0, -2.0, 9.40, -1.44, -1.88, -5.65, 0.425, 0.117},
                            {2.0, -1.0, 0.0, 0.0, 0.0, 0.0, -2.35, 0.37, 0.47, 1.41, -0.106, -0.029},
                            {2.0, -1.0, 0.0, 0.0, 0.0, -1.0, -1.04, 0.17, 0.21, 0.62, -0.047, -0.013},
                            {2.0, 0.0, -1.0, -2.0, 2.0, -2.0, -8.51, 3.50, 3.29, 5.11, -0.437, -0.019},
                            {2.0, 0.0, 0.0, -2.0, 2.0, -2.0, -144.13, 63.56, 59.23, 86.56, -7.547, -0.159},
                            {2.0, 0.0, 1.0, -2.0, 2.0, -2.0, 1.19, -0.56, -0.52, -0.72, 0.064, 0.000},
                            {2.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.49, -0.25, -0.23, -0.29, 0.027, -0.001},
                            {2.0, 0.0, 0.0, 0.0, 0.0, 0.0, -38.48, 19.14, 17.72, 23.11, -2.104, 0.041},
                            {2.0, 0.0, 0.0, 0.0, 0.0, -1.0, -11.44, 5.75, 5.32, 6.87, -0.627, 0.015},
                            {2.0, 0.0, 0.0, 0.0, 0.0, -2.0, -1.24, 0.63, 0.58, 0.75, -0.068, 0.002},
                            {2.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.77, 1.79, 1.71, 1.04, -0.146, 0.037},
                            {2.0, 1.0, 0.0, 0.0, 0.0, -1.0, -0.77, 0.78, 0.75, 0.45, -0.064, 0.017},
                            {2.0, 0.0, 0.0, 2.0, 0.0, 2.0, -0.33, 0.62, 0.65, 0.19, -0.049, 0.018}};

    double dt = (t.dmjd() - dJ0) / dJC;

    double arg[6];
    arg[0] =
        fmod((67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * dt + 0.093104 * dt * dt - 6.2e-6 * dt * dt * dt) *
                     15.0 +
                 648000.0,
             dTURNAS) *
        dAS2R;
    arg[1] = _iauFal03(dt);
    arg[2] = _iauFalp03(dt);
    arg[3] = _iauFaf03(dt);
    arg[4] = _iauFad03(dt);
    arg[5] = _iauFaom03(dt);

    double darg[6];
    darg[0] = (876600.0 * 3600.0 + 8640184.812866 + 2.0 * 0.093104 * dt - 3.0 * 6.2e-6 * dt * dt) * 15.0 * dAS2R / dJC;
    darg[1] = (-4.0 * 0.00024470 * dt * dt * dt + 3.0 * 0.051635 * dt * dt + 2.0 * 31.8792 * dt + 1717915923.2178) *
              dAS2R / dJC;
    darg[2] = (-4.0 * 0.00001149 * dt * dt * dt + 3.0 * 0.000136 * dt * dt - 2.0 * 0.5532 * dt + 129596581.0481) *
              dAS2R / dJC;
    darg[3] = (4.0 * 0.00000417 * dt * dt * dt - 3.0 * 0.001037 * dt * dt - 2.0 * 12.7512 * dt + 1739527262.8478) *
              dAS2R / dJC;
    darg[4] = (-4.0 * 0.00003169 * dt * dt * dt + 3.0 * 0.006593 * dt * dt - 2.0 * 6.3706 * dt + 1602961601.2090) *
              dAS2R / dJC;
    darg[5] =
        (-4.0 * 0.00005939 * dt * dt * dt + 3.0 * 0.007702 * dt * dt + 2.0 * 7.4722 * dt - 6962890.5431) * dAS2R / dJC;

    double cor_x = 0.0;
    double cor_y = 0.0;
    double cor_ut1 = 0.0;
    double cor_lod = 0.0;
    for (int j = 0; j <= 70; j++)
    {
        double ag = 0.0;
        double dag = 0.0;
        for (int i = 0; i <= 5; i++)
        {
            ag += table[j][i] * arg[i];
            dag += table[j][i] * darg[i];
        }
        ag = fmod(ag, d2PI);
        cor_x += table[j][7] * cos(ag) + table[j][6] * sin(ag);
        cor_y += table[j][9] * cos(ag) + table[j][8] * sin(ag);
        cor_ut1 += table[j][11] * cos(ag) + table[j][10] * sin(ag);
        cor_lod -= (table[j][11] * (-sin(ag)) + table[j][10] * cos(ag)) * dag;
    }
    eop[0] = cor_x;
    eop[1] = cor_y;
    eop[2] = cor_ut1;
}

void t_gtrs2crs::_PMSDNUT2(t_gtime &t, double *pm)
{
    double table[10][10] = {{1.0, -1.0, 0.0, -2.0, 0.0, -1.0, -0.44, 0.25, -0.25, -0.44},
                            {1.0, -1.0, 0.0, -2.0, 0.0, -2.0, -2.31, 1.32, -1.32, -2.31},
                            {1.0, 1.0, 0.0, -2.0, -2.0, -2.0, -0.44, 0.25, -0.25, -0.44},
                            {1.0, 0.0, 0.0, -2.0, 0.0, -1.0, -2.14, 1.23, -1.23, -2.14},
                            {1.0, 0.0, 0.0, -2.0, 0.0, -2.0, -11.36, 6.52, -6.52, -11.36},
                            {1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.84, -0.48, 0.48, 0.84},
                            {1.0, 0.0, 0.0, -2.0, 2.0, -2.0, -4.76, 2.73, -2.73, -4.76},
                            {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 14.27, -8.19, 8.19, 14.27},
                            {1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 1.93, -1.11, 1.11, 1.93},
                            {1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.76, -0.43, 0.43, 0.76}};

    pm[0] = 0;
    pm[1] = 0;
    double dmjd = t.dmjd();
    double dt = (dmjd - dJ0) / dJC;
    double arg[6];
    arg[0] =
        fmod((67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * dt + 0.093104 * dt * dt - 6.2e-6 * dt * dt * dt) *
                     15.0 +
                 648000.0,
             dTURNAS) *
        dAS2R;
    arg[1] = _iauFal03(dt);
    arg[2] = _iauFalp03(dt);
    arg[3] = _iauFaf03(dt);
    arg[4] = _iauFad03(dt);
    arg[5] = _iauFaom03(dt);

    for (int j = 0; j < 10; j++)
    {
        double angle = 0.0;
        for (int i = 0; i < 6; i++)
        {
            angle += table[j][i] * arg[i];
        }
        angle = fmod(angle, d2PI);
        pm[0] = pm[0] + table[j][6] * sin(angle) + table[j][7] * cos(angle);
        pm[1] = pm[1] + table[j][8] * sin(angle) + table[j][9] * cos(angle);
    }
    return;
}

void t_gtrs2crs::_UTLIBR(t_gtime &t, double *temp)
{
    double table[11][10] = {{2.0, -2.0, 0.0, -2.0, 0.0, -2.0, 0.05, -0.03, -0.3, -0.6},
                            {2.0, 0.0, 0.0, -2.0, -2.0, -2.0, 0.06, -0.03, -0.4, -0.7},
                            {2.0, -1.0, 0.0, -2.0, 0.0, -2.0, 0.35, -0.20, -2.4, -4.2},
                            {2.0, 1.0, 0.0, -2.0, -2.0, -2.0, 0.07, -0.04, -0.5, -0.8},
                            {2.0, 0.0, 0.0, -2.0, 0.0, -1.0, -0.07, 0.04, 0.5, 0.8},
                            {2.0, 0.0, 0.0, -2.0, 0.0, -2.0, 1.75, -1.01, -12.2, -21.3},
                            {2.0, 1.0, 0.0, -2.0, 0.0, -2.0, -0.05, 0.03, 0.3, 0.6},
                            {2.0, 0.0, -1.0, -2.0, 2.0, -2.0, 0.05, -0.03, -0.3, -0.6},
                            {2.0, 0.0, 0.0, -2.0, 2.0, -2.0, 0.76, -0.44, -5.5, -9.5},
                            {2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.21, -0.12, -1.5, -2.6},
                            {2.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.06, -0.04, -0.4, -0.8}};
    double dt = (t.dmjd() - rmjd0) / 36525.0;
    double arg[6];
    arg[0] =
        fmod((67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * dt + 0.093104 * dt * dt - 6.2e-6 * dt * dt * dt) *
                     15.0 +
                 648000.0,
             dTURNAS) *
        dAS2R;
    arg[1] = _iauFal03(dt);
    arg[2] = _iauFalp03(dt);
    arg[3] = _iauFaf03(dt);
    arg[4] = _iauFad03(dt);
    arg[5] = _iauFaom03(dt);
    temp[0] = 0;
    temp[1] = 0;
    for (int j = 0; j <= 10; j++)
    {
        double angle = 0;
        for (int i = 0; i <= 5; i++)
        {
            angle += table[j][i] * arg[i];
        }
        angle = fmod(angle, d2PI);
        temp[0] = temp[0] + table[j][6] * sin(angle) + table[j][7] * cos(angle);
        temp[1] = temp[1] + table[j][8] * sin(angle) + table[j][9] * cos(angle);
    }
    return;
}

double t_gtrs2crs::_tideCor2(const double &dRmjd)
{
    static t_zonaltide sTZB[3];
    static double gdStepsize = 0.05;
    static bool isFirst = true;
    double dT;
    double pdUt1;
    if (isFirst)
    {
        sTZB[0].time = LAST_TIME;
        sTZB[2].time = FIRST_TIME;
        isFirst = false;
    }

    while ((dRmjd > sTZB[2].time.dmjd()) || (dRmjd < sTZB[0].time.dmjd()))
    {
        if ((dRmjd > sTZB[2].time.dmjd() + gdStepsize) || (dRmjd < sTZB[0].time.dmjd() - gdStepsize))
        {

            sTZB[0].time.from_mjd((int)(dRmjd - gdStepsize),
                                  (int)((dRmjd - gdStepsize - (int)(dRmjd - gdStepsize)) * 86400),
                                  ((dRmjd - gdStepsize - (int)(dRmjd - gdStepsize)) * 86400) -
                                      (int)(((dRmjd - gdStepsize - (int)(dRmjd - gdStepsize)) * 86400)));

            sTZB[1].time.from_mjd((int)(dRmjd), (int)((dRmjd - (int)dRmjd) * 86400),
                                  ((dRmjd - (int)dRmjd) * 86400) - (int)((dRmjd - (int)dRmjd) * 86400));

            sTZB[2].time.from_mjd((int)(dRmjd + gdStepsize),
                                  (int)((dRmjd + gdStepsize - (int)(dRmjd + gdStepsize)) * 86400),
                                  ((dRmjd + gdStepsize - (int)(dRmjd + gdStepsize)) * 86400) -
                                      (int)(((dRmjd + gdStepsize - (int)(dRmjd + gdStepsize)) * 86400)));
            for (int i = 0; i < 3; i++)
            {
                dT = (sTZB[i].time.dmjd() - 51544.5) / 36525.0;
                _RG_ZONT2(dT, &sTZB[i].ut1, &sTZB[i].lod, &sTZB[i].omega);
            }
        }

        else if (dRmjd < sTZB[0].time.dmjd())
        {
            sTZB[2] = sTZB[1];
            sTZB[1] = sTZB[0];
            sTZB[0].time.from_mjd(
                (int)(sTZB[0].time.dmjd() - gdStepsize),
                (int)(((sTZB[0].time.dmjd() - gdStepsize) - (int)(sTZB[0].time.dmjd() - gdStepsize)) * 86400),
                (((sTZB[0].time.dmjd() - gdStepsize) - (int)(sTZB[0].time.dmjd() - gdStepsize)) * 86400) -
                    (int)(((sTZB[0].time.dmjd() - gdStepsize) - (int)(sTZB[0].time.dmjd() - gdStepsize)) * 86400));
            dT = (sTZB[0].time.dmjd() - 51544.5) / 36525.0;
            _RG_ZONT2(dT, &sTZB[0].ut1, &sTZB[0].lod, &sTZB[0].omega);
        }

        else
        {
            sTZB[0] = sTZB[1];
            sTZB[1] = sTZB[2];

            sTZB[2].time.from_mjd(
                (int)(sTZB[2].time.dmjd() + gdStepsize),
                (int)(((sTZB[2].time.dmjd() + gdStepsize) - (int)(sTZB[2].time.dmjd() + gdStepsize)) * 86400),
                (((sTZB[2].time.dmjd() + gdStepsize) - (int)(sTZB[2].time.dmjd() + gdStepsize)) * 86400) -
                    (int)(((sTZB[2].time.dmjd() + gdStepsize) - (int)(sTZB[2].time.dmjd() + gdStepsize)) * 86400));
            dT = (sTZB[2].time.dmjd() - 51544.5) / 36525.0;
            _RG_ZONT2(dT, &sTZB[2].ut1, &sTZB[2].lod, &sTZB[2].omega);
        }
    }

    double dTemp1[3];
    double dTemp2[3];
    for (int i = 0; i < 3; i++)
    {
        dTemp1[i] = sTZB[i].time.dmjd();
        dTemp2[i] = sTZB[i].ut1;
    }

    pdUt1 = _interpolation(2, 3, dTemp1, dTemp2, dRmjd);
    return pdUt1;
}

void t_gtrs2crs::_cal_dPole_dut1(t_gtime &t, double *x, t_gpoleut1 *poleut1, string type)
{
    double dut1zonaltide = 0.0;
    x[0] = 0.0;
    x[1] = 0.0;
    x[2] = 0.0;

    if (type != "UT1R")
    {
        t_gtriple xyu;
        _tide_corrections(t, xyu);
        dut1zonaltide = _tideCor2(t.dmjd());
        x[0] = xyu[0];
        x[1] = xyu[1];
        x[2] = xyu[2] + dut1zonaltide;
    }
}

void t_gtrs2crs::_RG_ZONT2(const double &dT, double *DUT, double *DLOD, double *DOMEGA)
{
    int I;
    double dL;
    double dLP;
    double dF;
    double dD;
    double dOM;
    double dARG;

    /*  ----------------------
     *  Zonal Earth tide model
     *  ----------------------*/
    //*  Number of terms in the zonal Earth tide model
    //  INTEGER NZONT
    const int iNZONT = 62;
    //*  Coefficients for the fundamental arguments
    int iNFUND[5][iNZONT];
    //*  Zonal tide term coefficients
    double dTIDE[6][iNZONT];
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     *  --------------------------------------------------
     *  Tables of multiples of arguments and coefficients
     *  --------------------------------------------------
     *  Luni-Solar argument multipliers*/

    int iNFUNDSUB1[20][5] = {1,  0, 2, 2, 2, 2, 0, 2, 0, 1, 2, 0, 2, 0,  2,  0, 0, 2, 2, 1, 0,  0, 2, 2, 2,
                             1,  0, 2, 0, 0, 1, 0, 2, 0, 1, 1, 0, 2, 0,  2,  3, 0, 0, 0, 0, -1, 0, 2, 2, 1,
                             -1, 0, 2, 2, 2, 1, 0, 0, 2, 0, 2, 0, 2, -2, 2,  0, 1, 2, 0, 2, 0,  0, 2, 0, 0,
                             0,  0, 2, 0, 1, 0, 0, 2, 0, 2, 2, 0, 0, 0,  -1, 2, 0, 0, 0, 0, 2,  0, 0, 0, 1};
    int i = 0;
    int j = 0;
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 4; j++)
        {
            iNFUND[j][i] = iNFUNDSUB1[i][j];
        }
    }

    int iNFUNDSUB2[20][5] = {0,  -1, 2, 0,  2, 0,  0, 0, 2,  -1, 0,  0, 0, 2, 0, 0,  0, 0, 2, 1, 0,  -1, 0,  2, 0,
                             1,  0,  2, -2, 1, 1,  0, 2, -2, 2,  1,  1, 0, 0, 0, -1, 0, 2, 0, 0, -1, 0,  2,  0, 1,
                             -1, 0,  2, 0,  2, 1,  0, 0, 0,  -1, 1,  0, 0, 0, 0, 1,  0, 0, 0, 1, 0,  0,  0,  1, 0,
                             1,  -1, 0, 0,  0, -1, 0, 0, 2,  -1, -1, 0, 0, 2, 0, -1, 0, 0, 2, 1, 1,  0,  -2, 2, -1};
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 4; j++)
        {
            iNFUND[j][i + 20] = iNFUNDSUB2[i][j];
        }
    }

    int iNFUNDSUB3[20][5] = {-1, -1, 0, 2,  0, 0, 2,  2, -2, 2, 0, 1, 2,  -2, 1,  0,  1,  2, -2, 2,  0,  0, 2, -2, 0,
                             0,  0,  2, -2, 1, 0, 0,  2, -2, 2, 0, 2, 0,  0,  0,  2,  0,  0, -2, -1, 2,  0, 0, -2, 0,
                             2,  0,  0, -2, 1, 0, -1, 2, -2, 1, 0, 1, 0,  0,  -1, 0,  -1, 2, -2, 2,  0,  1, 0, 0,  0,
                             0,  1,  0, 0,  1, 1, 0,  0, -1, 0, 2, 0, -2, 0,  0,  -2, 0,  2, 0,  1,  -1, 1, 0, 1,  0};
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 4; j++)
        {
            iNFUND[j][i + 40] = iNFUNDSUB3[i][j];
        }
    }

    int iNFUNDSUB4[2][5] = {0, 0, 0, 0, 2, 0, 0, 0, 0, 1};
    for (i = 0; i <= 1; i++)
    {
        for (j = 0; j <= 4; j++)
        {
            iNFUND[j][i + 60] = iNFUNDSUB4[i][j];
        }
    }

    double dTIDESUB1[20][6] = {
        -0.0235, 0.0000, 0.2617,  0.0000, -0.2209,  0.0000,  -0.0404, 0.0000, 0.3706,  0.0000, -0.3128,  0.0000,
        -0.0987, 0.0000, 0.9041,  0.0000, -0.7630,  0.0000,  -0.0508, 0.0000, 0.4499,  0.0000, -0.3797,  0.0000,
        -0.1231, 0.0000, 1.0904,  0.0000, -0.9203,  0.0000,  -0.0385, 0.0000, 0.2659,  0.0000, -0.2244,  0.0000,
        -0.4108, 0.0000, 2.8298,  0.0000, -2.3884,  0.0000,  -0.9926, 0.0000, 6.8291,  0.0000, -5.7637,  0.0000,
        -0.0179, 0.0000, 0.1222,  0.0000, -0.1031,  0.0000,  -0.0818, 0.0000, 0.5384,  0.0000, -0.4544,  0.0000,
        -0.1974, 0.0000, 1.2978,  0.0000, -1.0953,  0.0000,  -0.0761, 0.0000, 0.4976,  0.0000, -0.4200,  0.0000,
        0.0216,  0.0000, -0.1060, 0.0000, 0.0895,   0.0000,  0.0254,  0.0000, -0.1211, 0.0000, 0.1022,   0.0000,
        -0.2989, 0.0000, 1.3804,  0.0000, -1.1650,  0.0000,  -3.1873, 0.2010, 14.6890, 0.9266, -12.3974, -0.7820,
        -7.8468, 0.5320, 36.0910, 2.4469, -30.4606, -2.0652, 0.0216,  0.0000, -0.0988, 0.0000, 0.0834,   0.0000,
        -0.3384, 0.0000, 1.5433,  0.0000, -1.3025,  0.0000,  0.0179,  0.0000, -0.0813, 0.0000, 0.0686,   0.0000};
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            dTIDE[j][i] = dTIDESUB1[i][j];
        }
    }

    double dTIDESUB2[20][6] = {
        -0.0244, 0.0000,  0.1082,  0.0000,  -0.0913, 0.0000,  0.0470,  0.0000,  -0.2004, 0.0000,  0.1692,
        0.0000,  -0.7341, 0.0000,  3.1240,  0.0000,  -2.6367, 0.0000,  -0.0526, 0.0000,  0.2235,  0.0000,
        -0.1886, 0.0000,  -0.0508, 0.0000,  0.2073,  0.0000,  -0.1749, 0.0000,  0.0498,  0.0000,  -0.1312,
        0.0000,  0.1107,  0.0000,  0.1006,  0.0000,  -0.2640, 0.0000,  0.2228,  0.0000,  0.0395,  0.0000,
        -0.0968, 0.0000,  0.0817,  0.0000,  0.0470,  0.0000,  -0.1099, 0.0000,  0.0927,  0.0000,  0.1767,
        0.0000,  -0.4115, 0.0000,  0.3473,  0.0000,  0.4352,  0.0000,  -1.0093, 0.0000,  0.8519,  0.0000,
        0.5339,  0.0000,  -1.2224, 0.0000,  1.0317,  0.0000,  -8.4046, 0.2500,  19.1647, 0.5701,  -16.1749,
        -0.4811, 0.5443,  0.0000,  -1.2360, 0.0000,  1.0432,  0.0000,  0.0470,  0.0000,  -0.1000, 0.0000,
        0.0844,  0.0000,  -0.0555, 0.0000,  0.1169,  0.0000,  -0.0987, 0.0000,  0.1175,  0.0000,  -0.2332,
        0.0000,  0.1968,  0.0000,  -1.8236, 0.0000,  3.6018,  0.0000,  -3.0399, 0.0000,  0.1316,  0.0000,
        -0.2587, 0.0000,  0.2183,  0.0000,  0.0179,  0.0000,  -0.0344, 0.0000,  0.0290,  0.0000

    };
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            dTIDE[j][i + 20] = dTIDESUB2[i][j];
        }
    }

    double dTIDESUB3[20][6] = {
        -0.0855,  0.0000, 0.1542,  0.0000, -0.1302,  0.0000,  -0.0573, 0.0000, 0.0395,  0.0000, -0.0333, 0.0000,
        0.0329,   0.0000, -0.0173, 0.0000, 0.0146,   0.0000,  -1.8847, 0.0000, 0.9726,  0.0000, -0.8209, 0.0000,
        0.2510,   0.0000, -0.0910, 0.0000, 0.0768,   0.0000,  1.1703,  0.0000, -0.4135, 0.0000, 0.3490,  0.0000,
        -49.7174, 0.4330, 17.1056, 0.1490, -14.4370, -0.1257, -0.1936, 0.0000, 0.0666,  0.0000, -0.0562, 0.0000,
        0.0489,   0.0000, -0.0154, 0.0000, 0.0130,   0.0000,  -0.5471, 0.0000, 0.1670,  0.0000, -0.1409, 0.0000,
        0.0367,   0.0000, -0.0108, 0.0000, 0.0092,   0.0000,  -0.0451, 0.0000, 0.0082,  0.0000, -0.0069, 0.0000,
        0.0921,   0.0000, -0.0167, 0.0000, 0.0141,   0.0000,  0.8281,  0.0000, -0.1425, 0.0000, 0.1202,  0.0000,
        -15.8887, 0.1530, 2.7332,  0.0267, -2.3068,  -0.0222, -0.1382, 0.0000, 0.0225,  0.0000, -0.0190, 0.0000,
        0.0348,   0.0000, -0.0053, 0.0000, 0.0045,   0.0000,  -0.1372, 0.0000, -0.0079, 0.0000, 0.0066,  0.0000,
        0.4211,   0.0000, -0.0203, 0.0000, 0.0171,   0.0000,  -0.0404, 0.0000, 0.0008,  0.0000, -0.0007, 0.0000};
    for (i = 0; i <= 19; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            dTIDE[j][i + 40] = dTIDESUB3[i][j];
        }
    }

    double dTIDESUB4[2][6] = {7.8998,     0.0000, 0.1460,   0.0000, -0.1232, 0.0000,
                              -1617.2681, 0.0000, -14.9471, 0.0000, 12.6153, 0.0000};
    for (i = 0; i <= 1; i++)
    {
        for (j = 0; j <= 5; j++)
        {
            dTIDE[j][i + 60] = dTIDESUB4[i][j];
        }
    }
    /*  -------------------------------------
     *   Computation of fundamental arguments
     *  -------------------------------------*/

    _FUNDARG(dT, &dL, &dLP, &dF, &dD, &dOM);

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
    // Set initial values to zero.
    *DUT = 0;
    *DLOD = 0;
    *DOMEGA = 0;

    //*  Sum zonal tide terms.
    for (I = 0; I <= 40; I++)
    {
        //*     Formation of multiples of arguments.
        dARG =
            fmod((iNFUND[0][I] * dL + iNFUND[1][I] * dLP + iNFUND[2][I] * dF + iNFUND[3][I] * dD + iNFUND[4][I] * dOM),
                 d2PI);

        if (dARG < 0)
        {
            dARG = dARG + d2PI;
        }
        //*     Evaluate zonal tidal terms.
        *DUT = *DUT + dTIDE[0][I] * sin(dARG) + dTIDE[1][I] * cos(dARG);

        *DLOD = *DLOD + dTIDE[2][I] * cos(dARG) + dTIDE[3][I] * sin(dARG);

        *DOMEGA = *DOMEGA + dTIDE[4][I] * cos(dARG) + dTIDE[5][I] * sin(dARG);
    }
    //*  Rescale corrections so that they are in units involving seconds.

    *DUT = (*DUT) * 1.0E-4;
    *DLOD = (*DLOD) * 1.0E-5;
    *DOMEGA = (*DOMEGA) * 1.0E-14;
    return;
}

void t_gtrs2crs::_FUNDARG(const double &T, double *L, double *LP, double *F, double *D, double *OM)
{
    if (L == NULL || LP == NULL || F == NULL || D == NULL || OM == NULL)
    {
        cout << "?????";
    }

    const double DAS2R = 4.848136811095359935899141e-6;
    const double TURNAS = 1296000;

    *L = fmod(485868.249036 + T * (1717915923.2178 + T * (31.8792 + T * (0.051635 + T * (-0.00024470)))), TURNAS) *
         DAS2R;
    *LP =
        fmod(1287104.79305 + T * (129596581.0481 + T * (-0.5532 + T * (0.000136 + T * (-0.00001149)))), TURNAS) * DAS2R;

    *F = fmod(335779.526232 + T * (1739527262.8478 + T * (-12.7512 + T * (-0.001037 + T * (0.00000417)))), TURNAS) *
         DAS2R;
    *D = fmod(1072260.70369 + T * (1602961601.2090 + T * (-6.3706 + T * (0.006593 + T * (-0.00003169)))), TURNAS) *
         DAS2R;
    *OM = fmod(450160.398036 + T * (-6962890.5431 + T * (7.4722 + T * (0.007702 + T * (-0.00005939)))), TURNAS) * DAS2R;
    return;
}

/** @brief Mean anomaly of the Moon. */
double t_gtrs2crs::_iauFal03(const double &t)
{
    /* Mean anomaly of the Moon (IERS Conventions 2003). */
    return fmod(485868.249036 + t * (1717915923.2178 + t * (31.8792 + t * (0.051635 + t * (-0.00024470)))), dTURNAS) *
           dAS2R;
}

/** @brief Mean anomaly of the Sun. */
double t_gtrs2crs::_iauFalp03(const double &t)
{
    /* Mean anomaly of the Sun (IERS Conventions 2003). */
    return fmod(1287104.793048 + t * (129596581.0481 + t * (-0.5532 + t * (0.000136 + t * (-0.00001149)))), dTURNAS) *
           dAS2R;
}

/** @brief Mean argument of the latitude of the Moon. */
double t_gtrs2crs::_iauFaf03(const double &t)
{
    /* Mean longitude of the Moon minus that of the ascending node */
    /* (IERS Conventions 2003).                                    */
    return fmod(335779.526232 + t * (1739527262.8478 + t * (-12.7512 + t * (-0.001037 + t * (0.00000417)))), dTURNAS) *
           dAS2R;
}

/** @brief Mean elongation of the Moon from the Sun. */
double t_gtrs2crs::_iauFad03(const double &t)
{
    /* Mean elongation of the Moon from the Sun (IERS Conventions 2003). */
    return fmod(1072260.703692 + t * (1602961601.2090 + t * (-6.3706 + t * (0.006593 + t * (-0.00003169)))), dTURNAS) *
           dAS2R;
}

/** @brief Mean longitude of the ascending node of the Moon. */
double t_gtrs2crs::_iauFaom03(const double &t)
{
    /* Mean longitude of the Moon's ascending node */
    /* (IERS Conventions 2003).                    */
    return fmod(450160.398036 + t * (-6962890.5431 + t * (7.4722 + t * (0.007702 + t * (-0.00005939)))), dTURNAS) *
           dAS2R;
}

/** @brief Planetary longitudes, Mercury. */
double t_gtrs2crs::_iauFame03(const double &t)
{
    /* Mean longitude of Mercury (IERS Conventions 2003). */
    return fmod(4.402608842 + 2608.7903141574 * t, d2PI);
}

/** @brief Planetary longitudes, Venus. */
double t_gtrs2crs::_iauFave03(const double &t)
{
    /* Mean longitude of Venus (IERS Conventions 2003). */
    return fmod(3.176146697 + 1021.3285546211 * t, d2PI);
}

/** @brief Planetary longitudes, Earth. */
double t_gtrs2crs::_iauFae03(const double &t)
{
    /* Mean longitude of Earth (IERS Conventions 2003). */
    return fmod(1.753470314 + 628.3075849991 * t, d2PI);
}

/** @brief Planetary longitudes, Mars. */
double t_gtrs2crs::_iauFama03(const double &t)
{
    /* Mean longitude of Mars (IERS Conventions 2003). */
    return fmod(6.203480913 + 334.0612426700 * t, d2PI);
}

/** @brief Planetary longitudes, Jupiter. */
double t_gtrs2crs::_iauFaju03(const double &t)
{
    /* Mean longitude of Jupiter (IERS Conventions 2003). */
    return fmod(0.599546497 + 52.9690962641 * t, d2PI);
}

/** @brief Planetary longitudes, Saturn. */
double t_gtrs2crs::_iauFasa03(const double &t)
{
    /* Mean longitude of Saturn (IERS Conventions 2003). */
    return fmod(0.874016757 + 21.3299104960 * t, d2PI);
}

/** @brief Planetary longitudes, Uranus. */
double t_gtrs2crs::_iauFaur03(const double &t)
{
    /* Mean longitude of Uranus (IERS Conventions 2003). */
    return fmod(5.481293872 + 7.4781598567 * t, d2PI);
}

/** @brief Planetary longitudes, Neptune. */
double t_gtrs2crs::_iauFane03(const double &t)
{
    /* Mean longitude of Neptune (IERS Conventions 2003). */
    return fmod(5.311886287 + 3.8133035638 * t, d2PI);
}

/** @brief General accumulated precession in longitude. */
double t_gtrs2crs::_iauFapa03(const double &t)
{
    /* General accumulated precession in longitude. */
    return (0.024381750 + 0.00000538691 * t) * t;
}

/***********************
*FunctionName:nutation_interpolation
    *Function:
    *InPut:      dRmjd modified JD
    dStepsize stepsize in days, 0.d0 use the default 0.125
    *OutPut:dpi nutation angle of the longitude(radian)
    dep nutation angle of the obliquity(radian)
    *Return:
    *Other:
***********************/
void t_gtrs2crs::_nutInt(const double &dRmjd, double *dpsi, double *deps, const double &step)
{
    static bool isFirst = true;
    static double gdStepsize_used = 0.125;
    static t_EpochNutation sTB1[3];

    if ((isFirst) || fabs(gdStepsize_used - step) > pow(10.0, -10))
    {
        isFirst = false;
        sTB1[0].T = 1.0e20;
        sTB1[2].T = -1.0e-20;
        if (step != 0.0)
        {
            gdStepsize_used = step;
        }
    }

    if ((dRmjd > sTB1[2].T) || (dRmjd < sTB1[0].T))
    {
        if ((dRmjd > (sTB1[2].T + gdStepsize_used)) || (dRmjd < sTB1[0].T - gdStepsize_used))
        {
            sTB1[0].T = dRmjd - gdStepsize_used;
            sTB1[1].T = dRmjd;
            sTB1[2].T = dRmjd + gdStepsize_used;
            if (_cver.find("00") != string::npos)
            {
                _nutCal(sTB1[0].T, 0.0, &sTB1[0].Pi, &sTB1[0].Ep);
                _nutCal(sTB1[1].T, 0.0, &sTB1[1].Pi, &sTB1[1].Ep);
                _nutCal(sTB1[2].T, 0.0, &sTB1[2].Pi, &sTB1[2].Ep);
            }
            else
            {
                _nutCal_06(sTB1[0].T, 0.0, &sTB1[0].Pi, &sTB1[0].Ep);
                _nutCal_06(sTB1[1].T, 0.0, &sTB1[1].Pi, &sTB1[1].Ep);
                _nutCal_06(sTB1[2].T, 0.0, &sTB1[2].Pi, &sTB1[2].Ep);
            }
        }
        else if (dRmjd < sTB1[0].T)
        {
            sTB1[2] = sTB1[1];
            sTB1[1] = sTB1[0];
            sTB1[0].T = sTB1[0].T - gdStepsize_used;
            if (_cver.find("00") != string::npos)
            {
                _nutCal(sTB1[0].T, 0.0, &sTB1[0].Pi, &sTB1[0].Ep);
            }
            else
            {
                _nutCal_06(sTB1[0].T, 0.0, &sTB1[0].Pi, &sTB1[0].Ep);
            }
        }
        else
        {
            sTB1[0] = sTB1[1];
            sTB1[1] = sTB1[2];
            sTB1[2].T = sTB1[2].T + gdStepsize_used;
            if (_cver.find("00") != string::npos)
            {
                _nutCal(sTB1[2].T, 0, &sTB1[2].Pi, &sTB1[2].Ep);
            }
            else
            {
                _nutCal_06(sTB1[2].T, 0, &sTB1[2].Pi, &sTB1[2].Ep);
            }
        }
    }
    double dTemp1[3];
    for (int i = 0; i < 3; i++)
    {
        dTemp1[i] = sTB1[i].T;
    }
    double dTemp2[3];
    for (int i = 0; i < 3; i++)
    {
        dTemp2[i] = sTB1[i].Pi;
    }
    double dTemp3[3];
    for (int i = 0; i < 3; i++)
    {
        dTemp3[i] = sTB1[i].Ep;
    }

    *dpsi = _interpolation(2, 3, dTemp1, dTemp2, dRmjd);
    *deps = _interpolation(2, 3, dTemp1, dTemp3, dRmjd);
}

/***********************
*FunctionName:r_interpolation
*Function:interpolation of linear of 2-order polynomials
*InPut: iOrder 1 or 2 for linear or quadratic
iPoint number of points in the input array (x,y) in case of long table (npoint > norder+1):
*OutPut:pdX
pdX
dXin
*Return:
*Other:
***********************/
double t_gtrs2crs::_interpolation(const int &iOrder, const int &iPoint, double *pdX, double *pdY, const double &dXin)
{
    const string strProgname = "_interpolation";

    int i1;
    int i2;
    int i3;
    double dX;
    double dIntv;
    string strTemp1;
    string strTemp2;

    dIntv = (*(pdX + 1)) - (*(pdX));
    if ((dXin < (*(pdX)-0.1 * dIntv)) || (dXin > (*(pdX + iPoint - 1) + 0.1 * dIntv)))
    {
        cout << " ERROR: input variable out of the table (tbeg,tend,tinput): " << setw(10) << (*pdX)
             << *(pdX + iPoint - 1) << dXin << endl;
    }

    if (iPoint == iOrder + 1)
    {
        i1 = 1;
    }
    else if (iPoint > iOrder + 1)
    {
        i1 = (int)((dXin - *pdX) / dIntv) + 1;
        if (i1 < 1)
        {
            i1 = 1;
        }
        if ((iPoint - iOrder) < i1)
        {
            i1 = iPoint - iOrder;
        }
    }
    else
    {
        cout << " ERROR: table is enough long for interpolation: "
             << " npoint = " << iPoint << " norder = " << iOrder << endl;
        throw(" ERROR: table is enough long for interpolation");
    }

    double dResult;
    if (iOrder == 1)
    {
        i2 = i1 + 1;
        dX = (dXin - *(pdX + i1 - 1)) / (*(pdX + i2 - 1) - *(pdX + i1 - 1));
        dResult = *(pdY + i1 - 1) + dX * (*(pdY + i2 - 1) - *(pdY + i1 - 1));
        return dResult;
    }
    else if (iOrder == 2)
    {
        i2 = i1 + 1;
        i3 = i2 + 1;
        dX = (dXin - *(pdX + i2 - 1)) / (*(pdX + i2 - 1) - *(pdX + i1 - 1));
        dResult = *(pdY + i2 - 1) + dX * (*(pdY + i3 - 1) - *(pdY + i1 - 1)) * 0.5 +
                  dX * dX * ((*(pdY + i3 - 1) + *(pdY + i1 - 1)) * 0.5 - *(pdY + i2 - 1));
        return dResult;
    }
    else
    {
        cout << "***ERROR: norder > 2 is not supported ";
        throw("***ERROR: norder > 2 is not supported ");
    }
}

// calculate rotation matrix of Precession and nutation using IAU2000A precession-nutation model
void t_gtrs2crs::_process2000(const double &dRmjd, const double &dpsi, const double &deps, Matrix &qmat,
                              const bool &ldpsi, const bool &ldeps)
{

    Matrix dMathlp(0.0, 3, 3);
    Matrix dNmat(0.0, 3, 3);
    Matrix dPmat(0.0, 3, 3);
    Matrix dBmat(0.0, 3, 3);

    vector<Matrix> rot;
    rot.reserve(5);

    //! Interval between fundamental epoch J2000.0 and given date in centuries
    double dT = (dRmjd - 51544.5) / 36525.e0;

    //! IAU 1980 mean obliquity of date and  Precession angles (Lieske et al. 1977) ( equ(30))
    double dEpsa80 = dEps0 + (-46.8150 + (-0.00059 + (0.001813) * dT) * dT) * dT / RAD2SEC;
    double dPsia77 = (5038.7784 + (-1.07259 + (-0.001147) * dT) * dT) * dT / RAD2SEC;
    double dOma77 = dEps0 + ((0.05127 + (-0.007726) * dT) * dT) * dT / RAD2SEC;
    double dChia = (10.5526 + (-2.38064 + (-0.001125) * dT) * dT) * dT / RAD2SEC;

    //         ! Apply IAU 2000A precession corrections.
    double dPsia = dPsia77 + dPrecor * dT;
    double dOma = dOma77 + dOblcor * dT;
    _epsa = dEpsa80 + dOblcor * dT;

    Matrix dNdpsi, dNdeps;
    vector<Matrix> rot_epsa, rot_nepsa, rot_deps, rot_dpsi;
    calcProcMat(false, 1, _epsa, rot_epsa);
    calcProcMat(false, 1, -_epsa, rot_nepsa);
    calcProcMat(ldeps, 1, deps, rot_deps);
    calcProcMat(ldpsi, 3, dpsi, rot_dpsi);
    dNmat = rot_nepsa[0] * rot_dpsi[0] * rot_deps[0] * rot_epsa[0];
    if (ldpsi)
    {
        dNdpsi = rot_nepsa[0] * rot_dpsi[1] * rot_deps[0] * rot_epsa[0];
    }
    if (ldeps)
    {
        dNdeps = rot_nepsa[0] * rot_dpsi[0] * rot_deps[1] * rot_epsa[0];
    }
    calcProcMat(false, 3, -dChia, rot);
    dPmat << rot[0];
    calcProcMat(false, 1, dOma, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;
    calcProcMat(false, 3, dPsia, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;
    calcProcMat(false, 1, -dEps0, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;

    // Frame Bias matrix (B-matrix)
    calcProcMat(false, 1, dEpsbi, rot);
    dBmat << rot[0];
    calcProcMat(false, 2, -dPsibi * sin(dEps0), rot);
    dMathlp << rot[0];
    dBmat = dMathlp * dBmat;
    calcProcMat(false, 3, -dRa0, rot);
    dMathlp << rot[0];
    dBmat = dMathlp * dBmat;

    qmat = dBmat * dPmat * dNmat;

    if (ldpsi)
    {
        _rotdpsi = dBmat * dPmat * dNdpsi;
    }
    if (ldeps)
    {
        _rotdeps = dBmat * dPmat * dNdeps;
    }
}

void t_gtrs2crs::_process2006(const double &dRmjd, const double &dpsi, const double &deps, Matrix &qmat,
                              const bool &ldpsi, const bool &ldeps)
{
    Matrix dMathlp(0.0, 3, 3);
    Matrix dNmat(0.0, 3, 3);
    Matrix dPmat(0.0, 3, 3);
    Matrix dBmat(0.0, 3, 3);

    vector<Matrix> rot;
    rot.reserve(5);

    // Interval between fundamental epoch J2000.0 and given date in centuries
    double dT = (dRmjd - dJ0) / dJC;

    double dPsia = (5038.481507 + (-1.0790069 + (-0.00114045 + (0.000132851 + (-0.0000000951) * dT) * dT) * dT) * dT) *
                   dT / RAD2SEC;
    double dOma =
        dEps0_2006 +
        (-0.025754 + (0.0512623 + (-0.00772503 + (-0.000000467 + (0.0000003337) * dT) * dT) * dT) * dT) * dT / RAD2SEC;
    _epsa =
        dEps0_2006 + (-46.836769 + (-0.0001831 + (0.00200340 + (-0.000000576 + (-0.0000000434) * dT) * dT) * dT) * dT) *
                         dT / RAD2SEC;
    double dChia =
        (10.556403 + (-2.3814292 + (-0.00121197 + (0.000170663 + (-0.0000000560) * dT) * dT) * dT) * dT) * dT / RAD2SEC;

    Matrix dNdpsi, dNdeps;
    vector<Matrix> rot_epsa, rot_nepsa, rot_deps, rot_dpsi;
    calcProcMat(false, 1, _epsa, rot_epsa);
    calcProcMat(false, 1, -_epsa, rot_nepsa);
    calcProcMat(ldeps, 1, deps, rot_deps);
    calcProcMat(ldpsi, 3, dpsi, rot_dpsi);
    dNmat = rot_nepsa[0] * rot_dpsi[0] * rot_deps[0] * rot_epsa[0];
    if (ldpsi)
    {
        dNdpsi = rot_nepsa[0] * rot_dpsi[1] * rot_deps[0] * rot_epsa[0];
    }
    if (ldeps)
    {
        dNdeps = rot_nepsa[0] * rot_dpsi[0] * rot_deps[1] * rot_epsa[0];
    }

    // Precession matrix (P-matrix)
    calcProcMat(false, 3, -dChia, rot);
    dPmat << rot[0];
    calcProcMat(false, 1, dOma, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;
    calcProcMat(false, 3, dPsia, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;
    calcProcMat(false, 1, -dEps0_2006, rot);
    dMathlp << rot[0];
    dPmat = dMathlp * dPmat;

    // Frame Bias matrix (B-matrix)
    calcProcMat(false, 1, dEpsbi, rot);
    dBmat << rot[0];
    calcProcMat(false, 2, -dPsibi * sin(dEps0), rot);
    dMathlp << rot[0];
    dBmat = dMathlp * dBmat;
    calcProcMat(false, 3, -dRa0, rot);
    dMathlp << rot[0];
    dBmat = dMathlp * dBmat;

    qmat = dBmat * dPmat * dNmat;

    if (ldpsi)
    {
        _rotdpsi = dBmat * dPmat * dNdpsi;
    }
    if (ldeps)
    {
        _rotdeps = dBmat * dPmat * dNdeps;
    }
}

void t_gtrs2crs::_iau_XY00(double date1, double date2, double *x, double *y)
{
    int NX0, NX1, NX2, NX3, NX4;
    int NY0, NY1, NY2, NY3, NY4;
    NX0 = 1306;
    NX1 = 253;
    NX2 = 36;
    NX3 = 4;
    NX4 = 1;
    NY0 = 962;
    NY1 = 277;
    NY2 = 30;
    NY3 = 5;
    NY4 = 1;

    /* Polynomial coefficients */
    double XP[6] = {-16616.99e-6, 2004191742.88e-6, -427219.05e-6, -198620.54e-6, -46.05e-6, 5.98e-6};
    double YP[6] = {-6950.78e-6, -25381.99e-6, -22407250.99e-6, 1842.28e-6, 1113.06e-6, 0.99e-6};

    int KX0[1306][14] = {/* 1-200 */
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                         {1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -1},
                         {0, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -2},
                         {1, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 0},
                         {1, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {2, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
                         {1, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                         {0, 1, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -8, 3, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
                         {0, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
                         {1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},
                         {3, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -2},
                         {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {0, 3, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                         {1, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},
                         {2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                         {1, 0, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},
                         {1, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                         {0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -2},
                         {1, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},
                         {4, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                         {1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         /* 201-400 */
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
                         {2, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -2},
                         {0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -2},
                         {0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, -2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {0, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
                         {0, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
                         {1, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},
                         {2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, -2},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},
                         {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
                         {1, 0, 0, -2, 0, 0, 19, -21, 3, 0, 0, 0, 0, 0},
                         {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0, -2},
                         {0, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
                         {0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
                         {3, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
                         {1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},
                         {2, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -1},
                         {1, 0, 0, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0},
                         {0, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -2},
                         {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -2},
                         {2, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 1, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0},
                         {0, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -1},
                         {1, -1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, -1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -5, 8, -3, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -8, 13, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, -2, 5, 0, 0, 0},
                         {1, 0, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2},
                         {2, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 6, -16, 4, 5, 0, 0, -2},
                         {1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0, -2},
                         {2, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                         {3, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0},
                         {0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2},
                         {1, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 1, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                         {1, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 2},
                         {1, 1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, -1},
                         {0, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, 0},
                         {3, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                         {0, 2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
                         {2, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                         {0, 0, 1, -1, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0},
                         {3, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, -1},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {1, -2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -2},
                         {0, 0, 1, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         /* 401-600 */
                         {3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 2},
                         {2, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0},
                         {2, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -4, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -2},
                         {1, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, -3, 0, 0, 0, 2},
                         {1, 0, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, -5, 0, 0, 0},
                         {2, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, -1},
                         {0, 0, 1, -1, 0, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 0},
                         {1, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -1},
                         {1, -1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 2},
                         {2, 0, 0, -2, 0, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                         {1, -1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 2, 0},
                         {2, 0, 2, 0, 2, 0, 0, 2, 0, -3, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, -2},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -4, 10, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                         {1, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, -1, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -4, 5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                         {1, -1, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -8, 3, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -2},
                         {1, 0, 0, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -1},
                         {0, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -9, 13, 0, 0, 0, 0, 0},
                         {1, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 0, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, 2, 0, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                         {1, 0, -4, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -2, 4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0},
                         {0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 1},
                         {1, 1, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -1, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
                         {1, 1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                         {2, 0, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2},
                         {1, 1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -2, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 7, -8, 3, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -2},
                         {1, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                         {3, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, -1, 1, 0, 0, 0},
                         {1, 0, 0, -1, 0, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -1},
                         {3, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, -1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {5, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -9, 4, 0, 0, 0, 0, -2},
                         {0, 0, 1, -1, 1, 0, 8, -14, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                         {0, 2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -5, 4, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0},
                         {1, -1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                         {1, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 0},
                         {2, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -1, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, -2},
                         /* 601-800 */
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                         {0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0},
                         {1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -4, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -9, 15, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
                         {1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 5, 0, 0, 2},
                         {0, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, 2, -5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, -5, 0, 0, 2},
                         {1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -15, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 2},
                         {1, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 3, -7, 4, 0, 0, 0, 0, 0},
                         {1, 0, -2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 2, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 4, -3, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, 0, -2, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, 0},
                         {0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 3, -1, 0, 0, 0},
                         {2, -1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {3, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                         {1, 2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 1, 0, 0, 0},
                         {3, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2},
                         {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, -5, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 1, 0, -3, 7, -4, 0, 0, 0, 0, 0},
                         {0, 0, 4, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 3, -5, 0, 2, 0, 0, 0, 0},
                         {0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {0, 0, 2, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 2, -4, 0, -3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 2},
                         {1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                         {1, -2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 3, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1},
                         {1, -1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -2, 0, 0, 5, 0, 0, 0},
                         {3, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 2, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 1, -1, -1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                         {0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, 0},
                         {3, -1, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, -2},
                         {1, 2, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                         {0, 0, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, -1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                         {1, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, 0},
                         {5, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 9, -12, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -5, 0, 2, 0, 0, 0, 0},
                         {1, 1, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                         {1, 0, 0, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, -1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {2, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 3, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 3, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         /* 801-1000 */
                         {1, -1, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -5, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 5, 0, -3, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, -2},
                         {0, 0, 1, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -8, 15, 0, 0, 0, 0, 0},
                         {0, 2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                         {3, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, -2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {1, 2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, 0, -9, 13, 0, 0, 0, 0, 0},
                         {3, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 20, -21, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 2, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -8, 11, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 2, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -12, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 5, -2, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 4, 0, 0, 0},
                         {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, -2},
                         {0, 0, 2, -2, 1, -1, 0, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                         {3, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -9, 17, 0, 0, 0, 0, 0},
                         {1, 1, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {1, 0, -2, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, -2, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -2, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 1, -6, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2},
                         {0, 0, 0, 0, 0, 0, 3, -7, 4, 0, 0, 0, 0, 0},
                         {1, -1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -2, 0, 0, 0},
                         {0, 0, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {1, 1, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, -1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                         {1, -2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {2, 0, -4, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, -1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {4, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 2, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 1, -1, 0, -18, 17, 0, 0, 0, 0, 0, 0},
                         {0, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -12, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 8, -16, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, -7, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2},
                         {0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0},
                         {1, 0, -2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, -2, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {1, 0, 0, -1, 0, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {4, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -2, 0, 4, -5, 0, 0, 0},
                         {2, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 17, -16, 0, -2, 0, 0, 0, 0},
                         {1, 0, -1, -1, -1, 0, 20, -20, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, -2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {0, 3, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 1, -2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, 0, -4, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -4, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, -5, 0, 0, 0, -2},
                         /* 1001-1200 */
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -5, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, -2},
                         {0, 2, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, -1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {0, 1, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 5, -8, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, -1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                         {1, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                         {2, 0, -2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {2, 0, 0, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -4, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                         {1, 1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, -4, 5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 1, -2, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {3, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 2, 2, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -4, 4, 0, 0, 0, 0, 0},
                         {2, -2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -3, 0, 3, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, -5, 5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 1, -3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, -1, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -4, 6, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 8, -9, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -8, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0},
                         {2, 1, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {3, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {3, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -4, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -2, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                         {3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                         {0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, -1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                         {0, 1, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                         {2, -1, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {2, 0, -1, -1, 1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                         {1, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {1, 0, 0, -1, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                         {0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, -1, 0, 0, 0},
                         {2, -1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, 0, 2, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {5, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                         {2, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                         {1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -2, 2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 0, -1, 0, 2, 0, 0, 0, 0, 0, 0},
                         {4, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 1, -4, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, -2, 0, 0, 0},
                         {1, -1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {2, 0, -1, -1, 0, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
                         {4, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 1, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, -1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 1, -3, 1, 0, -6, 7, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 2, -5, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 5, -5, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 1, 5, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 0, 2, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                         {2, 0, -2, 0, -2, 0, 0, 5, -9, 0, 0, 0, 0, 0},
                         {2, 0, -2, -5, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, -1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 3, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                         {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         {1, 0, 0, 0, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                         {1, 0, 0, -2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                         {1, 0, -1, 0, -1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, -1, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                         {1, 0, -1, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, -2, -2, -2, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                         /* 1201-1306 */
                         {0, 0, 2, 2, 2, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -2, 0, 1, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -4, 4, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -7, 9, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 0, -10, 15, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, 1, -4, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, -3, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 1, 0, -4, 6, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                         {0, 0, 0, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 9, -13, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 8, -14, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 5, -6, -4, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 3, -3, 0, 2, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 7, -12, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -8, 1, 5, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, 0, -4, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 5, -13, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 5, -16, 4, 5, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 3, 0, -5, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -1},
                         {0, 0, 0, 0, 0, 0, 0, 3, -7, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 3, -9, 0, 0, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -3, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 2, -8, 1, 5, 0, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, -5, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 2},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -3, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -6, 3, 0, -2},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
                         {1, 1, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -2, 0, 0, 0, -2, 0, 0, 5, 0, 0, 0},
                         {3, 0, -2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 2, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                         {1, 0, -1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 2, -2, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                         {2, 0, 2, -6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {4, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 7, -13, 0, 0, 0, 0, 0},
                         {2, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, -1, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, -1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 2, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},
                         {2, -1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                         {2, 1, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {3, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 0, 0, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {2, 0, 0, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {1, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 0, 0, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                         {1, 0, -2, 4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         {0, 0, 4, -4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KX1[253][14] = {/* 1-200 */
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                        {0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -1},
                        {0, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -2},
                        {1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
                        {1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -1},
                        {1, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},
                        {1, -1, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -8, 3, 0, 0, 0, -2},
                        {1, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
                        {0, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -2},
                        {2, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 3, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                        {1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},
                        {1, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},
                        {1, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                        {1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
                        {2, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -2},
                        {0, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},
                        {1, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
                        {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                        {0, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, -2},
                        {1, 0, -2, 0, -2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
                        {0, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},
                        {1, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -2},
                        {3, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
                        {0, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -1},
                        {1, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},
                        {3, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},
                        /* 201-253 */
                        {2, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
                        {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
                        {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                        {0, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},
                        {3, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -1},
                        {2, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},
                        {0, 0, 1, -1, 1, 0, 0, -5, 8, -3, 0, 0, 0, 0},
                        {1, 0, 0, 0, -1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -2},
                        {1, -1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -2},
                        {1, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
                        {1, -1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -8, 13, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -2},
                        {4, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, -2, 5, 0, 0, 0},
                        {2, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                        {0, 2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                        {2, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                        {1, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KX2[36][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KX3[4][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KX4[1][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    double SX0[1306][2] = {/* 1-200 */
                           {-6844318.44e-6, +1328.67e-6},
                           {-523908.04e-6, -544.76e-6},
                           {-90552.22e-6, +111.23e-6},
                           {+82168.76e-6, -27.64e-6},
                           {+58707.02e-6, +470.05e-6},
                           {+28288.28e-6, -34.69e-6},
                           {-20557.78e-6, -20.84e-6},
                           {-15406.85e-6, +15.12e-6},
                           {-11991.74e-6, +32.46e-6},
                           {-8584.95e-6, +4.42e-6},
                           {-6245.02e-6, -6.68e-6},
                           {+5095.50e-6, +7.19e-6},
                           {-4910.93e-6, +0.76e-6},
                           {+2521.07e-6, -5.97e-6},
                           {+2511.85e-6, +1.07e-6},
                           {+2372.58e-6, +5.93e-6},
                           {+2307.58e-6, -7.52e-6},
                           {-2053.16e-6, +5.13e-6},
                           {+1898.27e-6, -0.72e-6},
                           {-1825.49e-6, +1.23e-6},
                           {-1534.09e-6, +6.29e-6},
                           {-1292.02e-6, +0.00e-6},
                           {-1234.96e-6, +5.21e-6},
                           {+1163.22e-6, -2.94e-6},
                           {+1137.48e-6, -0.04e-6},
                           {+1029.70e-6, -2.63e-6},
                           {-866.48e-6, +0.52e-6},
                           {-813.13e-6, +0.40e-6},
                           {+664.57e-6, -0.40e-6},
                           {-628.24e-6, -0.64e-6},
                           {-603.52e-6, +0.44e-6},
                           {-556.26e-6, +3.16e-6},
                           {-512.37e-6, -1.47e-6},
                           {+506.65e-6, +2.54e-6},
                           {+438.51e-6, -0.56e-6},
                           {+405.91e-6, +0.99e-6},
                           {-122.67e-6, +203.78e-6},
                           {-305.78e-6, +1.75e-6},
                           {+300.99e-6, -0.44e-6},
                           {-292.37e-6, -0.32e-6},
                           {+284.09e-6, +0.32e-6},
                           {-264.02e-6, +0.99e-6},
                           {+261.54e-6, -0.95e-6},
                           {+256.30e-6, -0.28e-6},
                           {-250.54e-6, +0.08e-6},
                           {+230.72e-6, +0.08e-6},
                           {+229.78e-6, -0.60e-6},
                           {-212.82e-6, +0.84e-6},
                           {+196.64e-6, -0.84e-6},
                           {+188.95e-6, -0.12e-6},
                           {+187.95e-6, -0.24e-6},
                           {-160.15e-6, -14.04e-6},
                           {-172.95e-6, -0.40e-6},
                           {-168.26e-6, +0.20e-6},
                           {+161.79e-6, +0.24e-6},
                           {+161.34e-6, +0.20e-6},
                           {+57.44e-6, +95.82e-6},
                           {+142.16e-6, +0.20e-6},
                           {-134.81e-6, +0.20e-6},
                           {+132.81e-6, -0.52e-6},
                           {-130.31e-6, +0.04e-6},
                           {+121.98e-6, -0.08e-6},
                           {-115.40e-6, +0.60e-6},
                           {-114.49e-6, +0.32e-6},
                           {+112.14e-6, +0.28e-6},
                           {+105.29e-6, +0.44e-6},
                           {+98.69e-6, -0.28e-6},
                           {+91.31e-6, -0.40e-6},
                           {+86.74e-6, -0.08e-6},
                           {-18.38e-6, +63.80e-6},
                           {+82.14e-6, +0.00e-6},
                           {+79.03e-6, -0.24e-6},
                           {+0.00e-6, -79.08e-6},
                           {-78.56e-6, +0.00e-6},
                           {+47.73e-6, +23.79e-6},
                           {+66.03e-6, -0.20e-6},
                           {+62.65e-6, -0.24e-6},
                           {+60.50e-6, +0.36e-6},
                           {+59.07e-6, +0.00e-6},
                           {+57.28e-6, +0.00e-6},
                           {-55.66e-6, +0.16e-6},
                           {-54.81e-6, -0.08e-6},
                           {-53.22e-6, -0.20e-6},
                           {-52.95e-6, +0.32e-6},
                           {-52.27e-6, +0.00e-6},
                           {+51.32e-6, +0.00e-6},
                           {-51.00e-6, -0.12e-6},
                           {+51.02e-6, +0.00e-6},
                           {-48.65e-6, -1.15e-6},
                           {+48.29e-6, +0.20e-6},
                           {-46.38e-6, +0.00e-6},
                           {-45.59e-6, -0.12e-6},
                           {-43.76e-6, +0.36e-6},
                           {-40.58e-6, -1.00e-6},
                           {+0.00e-6, -41.53e-6},
                           {+40.54e-6, -0.04e-6},
                           {+40.33e-6, -0.04e-6},
                           {-38.57e-6, +0.08e-6},
                           {+37.75e-6, +0.04e-6},
                           {+37.15e-6, -0.12e-6},
                           {+36.68e-6, -0.04e-6},
                           {-18.30e-6, -17.30e-6},
                           {-17.86e-6, +17.10e-6},
                           {-34.81e-6, +0.04e-6},
                           {-33.22e-6, +0.08e-6},
                           {+32.43e-6, -0.04e-6},
                           {-30.47e-6, +0.04e-6},
                           {-29.53e-6, +0.04e-6},
                           {+28.50e-6, -0.08e-6},
                           {+28.35e-6, -0.16e-6},
                           {-28.00e-6, +0.00e-6},
                           {-27.61e-6, +0.20e-6},
                           {-26.77e-6, +0.08e-6},
                           {+26.54e-6, -0.12e-6},
                           {+26.54e-6, +0.04e-6},
                           {-26.17e-6, +0.00e-6},
                           {-25.42e-6, -0.08e-6},
                           {-16.91e-6, +8.43e-6},
                           {+0.32e-6, +24.42e-6},
                           {-19.53e-6, +5.09e-6},
                           {-23.79e-6, +0.00e-6},
                           {+23.66e-6, +0.00e-6},
                           {-23.47e-6, +0.16e-6},
                           {+23.39e-6, -0.12e-6},
                           {-23.49e-6, +0.00e-6},
                           {-23.28e-6, -0.08e-6},
                           {-22.99e-6, +0.04e-6},
                           {-22.67e-6, -0.08e-6},
                           {+9.35e-6, +13.29e-6},
                           {+22.47e-6, -0.04e-6},
                           {+4.89e-6, -16.55e-6},
                           {+4.89e-6, -16.51e-6},
                           {+21.28e-6, -0.08e-6},
                           {+20.57e-6, +0.64e-6},
                           {+21.01e-6, +0.00e-6},
                           {+1.23e-6, -19.13e-6},
                           {-19.97e-6, +0.12e-6},
                           {+19.65e-6, -0.08e-6},
                           {+19.58e-6, -0.12e-6},
                           {+19.61e-6, -0.08e-6},
                           {-19.41e-6, +0.08e-6},
                           {-19.49e-6, +0.00e-6},
                           {-18.64e-6, +0.00e-6},
                           {+18.58e-6, +0.04e-6},
                           {-18.42e-6, +0.00e-6},
                           {+18.22e-6, +0.00e-6},
                           {-0.72e-6, -17.34e-6},
                           {-18.02e-6, -0.04e-6},
                           {+17.74e-6, +0.08e-6},
                           {+17.46e-6, +0.00e-6},
                           {-17.42e-6, +0.00e-6},
                           {-6.60e-6, +10.70e-6},
                           {+16.43e-6, +0.52e-6},
                           {-16.75e-6, +0.04e-6},
                           {+16.55e-6, -0.08e-6},
                           {+16.39e-6, -0.08e-6},
                           {+13.88e-6, -2.47e-6},
                           {+15.69e-6, +0.00e-6},
                           {-15.52e-6, +0.00e-6},
                           {+3.34e-6, +11.86e-6},
                           {+14.72e-6, -0.32e-6},
                           {+14.92e-6, -0.04e-6},
                           {-3.26e-6, +11.62e-6},
                           {-14.64e-6, +0.00e-6},
                           {+0.00e-6, +14.47e-6},
                           {-14.37e-6, +0.00e-6},
                           {+14.32e-6, -0.04e-6},
                           {-14.10e-6, +0.04e-6},
                           {+10.86e-6, +3.18e-6},
                           {-10.58e-6, -3.10e-6},
                           {-3.62e-6, +9.86e-6},
                           {-13.48e-6, +0.00e-6},
                           {+13.41e-6, -0.04e-6},
                           {+13.32e-6, -0.08e-6},
                           {-13.33e-6, -0.04e-6},
                           {-13.29e-6, +0.00e-6},
                           {-0.20e-6, +13.05e-6},
                           {+0.00e-6, +13.13e-6},
                           {-8.99e-6, +4.02e-6},
                           {-12.93e-6, +0.04e-6},
                           {+2.03e-6, +10.82e-6},
                           {-12.78e-6, +0.04e-6},
                           {+12.24e-6, +0.04e-6},
                           {+8.71e-6, +3.54e-6},
                           {+11.98e-6, -0.04e-6},
                           {-11.38e-6, +0.04e-6},
                           {-11.30e-6, +0.00e-6},
                           {+11.14e-6, -0.04e-6},
                           {+10.98e-6, +0.00e-6},
                           {-10.98e-6, +0.00e-6},
                           {+0.44e-6, -10.38e-6},
                           {+10.46e-6, +0.08e-6},
                           {-10.42e-6, +0.00e-6},
                           {-10.30e-6, +0.08e-6},
                           {+6.92e-6, +3.34e-6},
                           {+10.07e-6, +0.04e-6},
                           {+10.02e-6, +0.00e-6},
                           {-9.75e-6, +0.04e-6},
                           {+9.75e-6, +0.00e-6},
                           {+9.67e-6, -0.04e-6},
                           /* 201-400 */
                           {-1.99e-6, +7.72e-6},
                           {+0.40e-6, +9.27e-6},
                           {-3.42e-6, +6.09e-6},
                           {+0.56e-6, -8.67e-6},
                           {-9.19e-6, +0.00e-6},
                           {+9.11e-6, +0.00e-6},
                           {+9.07e-6, +0.00e-6},
                           {+1.63e-6, +6.96e-6},
                           {-8.47e-6, +0.00e-6},
                           {-8.28e-6, +0.04e-6},
                           {+8.27e-6, +0.04e-6},
                           {-8.04e-6, +0.00e-6},
                           {+7.91e-6, +0.00e-6},
                           {-7.84e-6, -0.04e-6},
                           {-7.64e-6, +0.08e-6},
                           {+5.21e-6, -2.51e-6},
                           {-5.77e-6, +1.87e-6},
                           {+5.01e-6, -2.51e-6},
                           {-7.48e-6, +0.00e-6},
                           {-7.32e-6, -0.12e-6},
                           {+7.40e-6, -0.04e-6},
                           {+7.44e-6, +0.00e-6},
                           {+6.32e-6, -1.11e-6},
                           {-6.13e-6, -1.19e-6},
                           {+0.20e-6, -6.88e-6},
                           {+6.92e-6, +0.04e-6},
                           {+6.48e-6, -0.48e-6},
                           {-6.94e-6, +0.00e-6},
                           {+2.47e-6, -4.46e-6},
                           {-2.23e-6, -4.65e-6},
                           {-1.07e-6, -5.69e-6},
                           {+4.97e-6, -1.71e-6},
                           {+5.57e-6, +1.07e-6},
                           {-6.48e-6, +0.08e-6},
                           {+2.03e-6, +4.53e-6},
                           {+4.10e-6, -2.39e-6},
                           {+0.00e-6, -6.44e-6},
                           {-6.40e-6, +0.00e-6},
                           {+6.32e-6, +0.00e-6},
                           {+2.67e-6, -3.62e-6},
                           {-1.91e-6, -4.38e-6},
                           {-2.43e-6, -3.82e-6},
                           {+6.20e-6, +0.00e-6},
                           {-3.38e-6, -2.78e-6},
                           {-6.12e-6, +0.04e-6},
                           {-6.09e-6, -0.04e-6},
                           {-6.01e-6, -0.04e-6},
                           {+3.18e-6, -2.82e-6},
                           {-5.05e-6, +0.84e-6},
                           {+5.85e-6, +0.00e-6},
                           {+5.69e-6, -0.12e-6},
                           {+5.73e-6, -0.04e-6},
                           {+5.61e-6, +0.00e-6},
                           {+5.49e-6, +0.00e-6},
                           {-5.33e-6, +0.04e-6},
                           {-5.29e-6, +0.00e-6},
                           {+5.25e-6, +0.00e-6},
                           {+0.99e-6, +4.22e-6},
                           {-0.99e-6, +4.22e-6},
                           {+0.00e-6, +5.21e-6},
                           {+5.13e-6, +0.04e-6},
                           {-4.90e-6, +0.00e-6},
                           {-3.10e-6, +1.79e-6},
                           {-4.81e-6, +0.04e-6},
                           {-4.75e-6, +0.00e-6},
                           {+4.70e-6, -0.04e-6},
                           {-4.69e-6, +0.00e-6},
                           {-4.65e-6, +0.00e-6},
                           {+4.65e-6, +0.00e-6},
                           {-4.57e-6, +0.00e-6},
                           {+4.49e-6, -0.04e-6},
                           {-4.53e-6, +0.00e-6},
                           {+0.00e-6, -4.53e-6},
                           {+0.00e-6, -4.53e-6},
                           {-4.53e-6, +0.00e-6},
                           {+4.50e-6, +0.00e-6},
                           {-4.49e-6, +0.00e-6},
                           {+1.83e-6, +2.63e-6},
                           {+4.38e-6, +0.00e-6},
                           {+0.88e-6, -3.46e-6},
                           {-2.70e-6, +1.55e-6},
                           {-4.22e-6, +0.00e-6},
                           {-4.10e-6, -0.12e-6},
                           {+3.54e-6, -0.64e-6},
                           {-3.50e-6, +0.68e-6},
                           {+4.18e-6, +0.00e-6},
                           {+4.14e-6, +0.00e-6},
                           {+4.10e-6, +0.00e-6},
                           {-4.06e-6, +0.00e-6},
                           {+2.70e-6, -1.35e-6},
                           {-4.04e-6, +0.00e-6},
                           {-3.98e-6, -0.04e-6},
                           {-3.98e-6, +0.04e-6},
                           {+4.02e-6, +0.00e-6},
                           {+3.94e-6, +0.00e-6},
                           {+0.84e-6, -3.10e-6},
                           {+3.30e-6, +0.60e-6},
                           {-1.59e-6, +2.27e-6},
                           {-3.66e-6, -0.20e-6},
                           {-3.10e-6, -0.72e-6},
                           {-3.82e-6, +0.00e-6},
                           {-3.62e-6, -0.16e-6},
                           {-3.74e-6, +0.00e-6},
                           {+3.74e-6, +0.00e-6},
                           {-3.74e-6, +0.00e-6},
                           {-3.71e-6, +0.00e-6},
                           {+3.02e-6, +0.68e-6},
                           {+3.70e-6, +0.00e-6},
                           {+3.30e-6, +0.40e-6},
                           {-3.66e-6, +0.04e-6},
                           {+3.66e-6, +0.04e-6},
                           {-3.62e-6, +0.00e-6},
                           {-3.61e-6, +0.00e-6},
                           {-2.90e-6, +0.68e-6},
                           {+0.80e-6, -2.78e-6},
                           {+3.54e-6, +0.00e-6},
                           {-3.54e-6, +0.00e-6},
                           {-3.50e-6, +0.00e-6},
                           {+3.45e-6, +0.00e-6},
                           {+0.00e-6, -3.42e-6},
                           {+3.38e-6, +0.00e-6},
                           {+2.27e-6, -1.11e-6},
                           {-3.34e-6, +0.00e-6},
                           {+3.34e-6, +0.00e-6},
                           {-3.30e-6, +0.01e-6},
                           {+3.31e-6, +0.00e-6},
                           {+3.30e-6, +0.00e-6},
                           {-3.30e-6, +0.00e-6},
                           {-1.39e-6, -1.91e-6},
                           {+3.30e-6, +0.00e-6},
                           {+3.26e-6, +0.00e-6},
                           {+3.26e-6, +0.00e-6},
                           {+3.22e-6, -0.04e-6},
                           {-3.26e-6, +0.00e-6},
                           {+2.51e-6, -0.64e-6},
                           {+3.14e-6, +0.00e-6},
                           {-2.63e-6, -0.48e-6},
                           {+3.10e-6, +0.00e-6},
                           {-3.06e-6, +0.00e-6},
                           {+2.94e-6, -0.12e-6},
                           {+3.06e-6, +0.00e-6},
                           {+0.00e-6, +2.98e-6},
                           {+2.98e-6, +0.00e-6},
                           {+2.07e-6, +0.91e-6},
                           {-2.98e-6, +0.00e-6},
                           {+2.94e-6, +0.00e-6},
                           {-2.94e-6, +0.00e-6},
                           {-2.94e-6, +0.00e-6},
                           {-2.90e-6, +0.00e-6},
                           {-0.56e-6, -2.35e-6},
                           {-1.47e-6, +1.39e-6},
                           {+2.80e-6, +0.00e-6},
                           {-2.74e-6, +0.00e-6},
                           {-0.12e-6, +2.63e-6},
                           {+2.15e-6, -0.60e-6},
                           {-2.70e-6, +0.00e-6},
                           {+1.79e-6, -0.88e-6},
                           {-0.48e-6, +2.19e-6},
                           {+0.44e-6, +2.23e-6},
                           {+0.52e-6, +2.07e-6},
                           {-2.59e-6, +0.00e-6},
                           {+2.55e-6, +0.00e-6},
                           {-1.11e-6, +1.43e-6},
                           {-2.51e-6, +0.00e-6},
                           {-2.51e-6, +0.00e-6},
                           {+2.51e-6, +0.00e-6},
                           {+0.00e-6, -2.50e-6},
                           {+2.47e-6, +0.00e-6},
                           {+2.11e-6, -0.36e-6},
                           {+1.67e-6, +0.80e-6},
                           {+2.46e-6, +0.00e-6},
                           {-2.43e-6, +0.00e-6},
                           {-2.39e-6, +0.00e-6},
                           {-1.83e-6, +0.56e-6},
                           {-0.44e-6, -1.95e-6},
                           {+0.24e-6, +2.15e-6},
                           {+2.39e-6, +0.00e-6},
                           {+2.35e-6, +0.00e-6},
                           {+2.27e-6, +0.00e-6},
                           {-2.22e-6, +0.00e-6},
                           {-1.03e-6, -1.15e-6},
                           {+1.87e-6, +0.32e-6},
                           {-0.32e-6, -1.87e-6},
                           {+2.15e-6, +0.00e-6},
                           {-0.80e-6, +1.35e-6},
                           {+2.11e-6, +0.00e-6},
                           {-2.11e-6, +0.00e-6},
                           {-0.56e-6, -1.55e-6},
                           {+2.11e-6, +0.00e-6},
                           {-0.84e-6, -1.27e-6},
                           {-1.99e-6, +0.12e-6},
                           {-0.24e-6, +1.87e-6},
                           {-0.24e-6, -1.87e-6},
                           {-2.03e-6, +0.00e-6},
                           {+2.03e-6, +0.00e-6},
                           {+2.03e-6, +0.00e-6},
                           {+2.03e-6, +0.00e-6},
                           {-0.40e-6, +1.59e-6},
                           {+1.99e-6, +0.00e-6},
                           {+1.95e-6, +0.00e-6},
                           /* 401-600 */
                           {+1.95e-6, +0.00e-6},
                           {+1.91e-6, +0.00e-6},
                           {+1.19e-6, -0.72e-6},
                           {+1.87e-6, +0.00e-6},
                           {+1.87e-6, +0.00e-6},
                           {-1.27e-6, +0.60e-6},
                           {+0.72e-6, -1.15e-6},
                           {-0.99e-6, +0.88e-6},
                           {+1.87e-6, +0.00e-6},
                           {-1.87e-6, +0.00e-6},
                           {-1.83e-6, +0.00e-6},
                           {-1.79e-6, +0.00e-6},
                           {-1.79e-6, +0.00e-6},
                           {+1.79e-6, +0.00e-6},
                           {+0.00e-6, -1.79e-6},
                           {-1.79e-6, +0.00e-6},
                           {-1.75e-6, +0.00e-6},
                           {-1.75e-6, +0.00e-6},
                           {+1.75e-6, +0.00e-6},
                           {-1.47e-6, -0.28e-6},
                           {-1.71e-6, +0.00e-6},
                           {+1.71e-6, +0.00e-6},
                           {+0.32e-6, +1.39e-6},
                           {+0.28e-6, -1.43e-6},
                           {-0.52e-6, -1.19e-6},
                           {+1.67e-6, +0.00e-6},
                           {-1.67e-6, +0.00e-6},
                           {+0.76e-6, -0.91e-6},
                           {-0.32e-6, +1.35e-6},
                           {-1.39e-6, -0.28e-6},
                           {+1.63e-6, +0.00e-6},
                           {-1.59e-6, +0.00e-6},
                           {+1.03e-6, -0.56e-6},
                           {+1.59e-6, +0.00e-6},
                           {+1.55e-6, +0.00e-6},
                           {-0.28e-6, -1.27e-6},
                           {-0.64e-6, +0.91e-6},
                           {-0.32e-6, -1.23e-6},
                           {-1.55e-6, +0.00e-6},
                           {-1.51e-6, +0.00e-6},
                           {+1.51e-6, +0.00e-6},
                           {-1.51e-6, +0.00e-6},
                           {+1.51e-6, +0.00e-6},
                           {+1.47e-6, +0.00e-6},
                           {+1.47e-6, +0.00e-6},
                           {+0.95e-6, -0.52e-6},
                           {+1.23e-6, -0.24e-6},
                           {+0.60e-6, +0.88e-6},
                           {-1.47e-6, +0.00e-6},
                           {-1.43e-6, +0.00e-6},
                           {+1.43e-6, +0.00e-6},
                           {+1.43e-6, +0.00e-6},
                           {-0.68e-6, -0.76e-6},
                           {+0.95e-6, -0.48e-6},
                           {-0.95e-6, -0.48e-6},
                           {-1.19e-6, -0.24e-6},
                           {+0.36e-6, -1.07e-6},
                           {+0.95e-6, +0.48e-6},
                           {+1.43e-6, +0.00e-6},
                           {+1.39e-6, +0.00e-6},
                           {+1.39e-6, +0.00e-6},
                           {-1.39e-6, +0.00e-6},
                           {-1.39e-6, +0.00e-6},
                           {+0.00e-6, +1.39e-6},
                           {-0.12e-6, -1.27e-6},
                           {+0.56e-6, +0.84e-6},
                           {-0.44e-6, -0.95e-6},
                           {+0.32e-6, -1.07e-6},
                           {+1.03e-6, -0.36e-6},
                           {-0.28e-6, +1.11e-6},
                           {+0.44e-6, +0.95e-6},
                           {-1.35e-6, +0.00e-6},
                           {+0.88e-6, +0.48e-6},
                           {-1.35e-6, +0.00e-6},
                           {+1.35e-6, +0.00e-6},
                           {+1.35e-6, +0.00e-6},
                           {-1.31e-6, +0.00e-6},
                           {+1.31e-6, +0.00e-6},
                           {-1.19e-6, -0.12e-6},
                           {+1.27e-6, +0.00e-6},
                           {+0.40e-6, -0.88e-6},
                           {+1.27e-6, +0.00e-6},
                           {+1.27e-6, +0.00e-6},
                           {-0.16e-6, -1.11e-6},
                           {-0.84e-6, +0.44e-6},
                           {+0.84e-6, -0.44e-6},
                           {+0.84e-6, -0.44e-6},
                           {-1.27e-6, +0.00e-6},
                           {-1.27e-6, +0.00e-6},
                           {+1.27e-6, +0.00e-6},
                           {-0.44e-6, -0.84e-6},
                           {+0.00e-6, -1.27e-6},
                           {-1.27e-6, +0.00e-6},
                           {-1.23e-6, +0.00e-6},
                           {-1.23e-6, +0.00e-6},
                           {+1.23e-6, +0.00e-6},
                           {+0.00e-6, +1.23e-6},
                           {-0.12e-6, +1.11e-6},
                           {+1.22e-6, +0.00e-6},
                           {+1.19e-6, +0.00e-6},
                           {-0.24e-6, +0.95e-6},
                           {-0.76e-6, -0.44e-6},
                           {+0.91e-6, +0.28e-6},
                           {+1.19e-6, +0.00e-6},
                           {+1.19e-6, +0.00e-6},
                           {+0.00e-6, +1.19e-6},
                           {+1.15e-6, +0.00e-6},
                           {+0.00e-6, +1.15e-6},
                           {-1.15e-6, +0.00e-6},
                           {+1.15e-6, +0.00e-6},
                           {-1.15e-6, +0.00e-6},
                           {+1.15e-6, +0.00e-6},
                           {+1.15e-6, +0.00e-6},
                           {-0.95e-6, +0.20e-6},
                           {+0.24e-6, +0.91e-6},
                           {-1.15e-6, +0.00e-6},
                           {-1.12e-6, +0.00e-6},
                           {-1.11e-6, +0.00e-6},
                           {-1.11e-6, +0.00e-6},
                           {+0.16e-6, +0.95e-6},
                           {-1.11e-6, +0.00e-6},
                           {+1.11e-6, +0.00e-6},
                           {+0.20e-6, -0.91e-6},
                           {-0.72e-6, -0.40e-6},
                           {-1.11e-6, +0.00e-6},
                           {-1.11e-6, +0.00e-6},
                           {+1.07e-6, +0.00e-6},
                           {-1.07e-6, +0.00e-6},
                           {+0.76e-6, -0.32e-6},
                           {+0.00e-6, -1.07e-6},
                           {+1.07e-6, +0.00e-6},
                           {+1.07e-6, +0.00e-6},
                           {-1.07e-6, +0.00e-6},
                           {+1.07e-6, +0.00e-6},
                           {-0.84e-6, -0.24e-6},
                           {+0.00e-6, -1.03e-6},
                           {+1.03e-6, +0.00e-6},
                           {-1.03e-6, +0.00e-6},
                           {-0.24e-6, +0.80e-6},
                           {+0.20e-6, +0.84e-6},
                           {-1.03e-6, +0.00e-6},
                           {-1.03e-6, +0.00e-6},
                           {-0.99e-6, +0.00e-6},
                           {+0.24e-6, +0.76e-6},
                           {-0.99e-6, +0.00e-6},
                           {-0.16e-6, +0.84e-6},
                           {-0.99e-6, +0.00e-6},
                           {-0.64e-6, +0.36e-6},
                           {+0.99e-6, +0.00e-6},
                           {+0.36e-6, -0.64e-6},
                           {-0.95e-6, +0.00e-6},
                           {-0.95e-6, +0.00e-6},
                           {+0.00e-6, +0.95e-6},
                           {+0.64e-6, +0.32e-6},
                           {+0.00e-6, -0.95e-6},
                           {+0.84e-6, +0.12e-6},
                           {+0.20e-6, +0.76e-6},
                           {-0.95e-6, +0.00e-6},
                           {+0.95e-6, +0.00e-6},
                           {-0.95e-6, +0.00e-6},
                           {+0.00e-6, +0.92e-6},
                           {+0.91e-6, +0.00e-6},
                           {+0.91e-6, +0.00e-6},
                           {+0.40e-6, +0.52e-6},
                           {-0.91e-6, +0.00e-6},
                           {-0.56e-6, +0.36e-6},
                           {+0.44e-6, -0.48e-6},
                           {-0.91e-6, +0.00e-6},
                           {-0.91e-6, +0.00e-6},
                           {-0.36e-6, -0.56e-6},
                           {+0.91e-6, +0.00e-6},
                           {-0.88e-6, +0.00e-6},
                           {-0.88e-6, +0.00e-6},
                           {+0.60e-6, -0.28e-6},
                           {+0.88e-6, +0.00e-6},
                           {+0.36e-6, -0.52e-6},
                           {-0.52e-6, +0.36e-6},
                           {+0.52e-6, +0.36e-6},
                           {+0.00e-6, +0.88e-6},
                           {+0.56e-6, +0.32e-6},
                           {+0.64e-6, -0.24e-6},
                           {+0.88e-6, +0.00e-6},
                           {+0.88e-6, +0.00e-6},
                           {+0.88e-6, +0.00e-6},
                           {+0.84e-6, +0.00e-6},
                           {-0.68e-6, -0.16e-6},
                           {+0.84e-6, +0.00e-6},
                           {+0.56e-6, +0.28e-6},
                           {-0.16e-6, +0.68e-6},
                           {+0.64e-6, -0.20e-6},
                           {+0.16e-6, +0.68e-6},
                           {+0.72e-6, -0.12e-6},
                           {-0.83e-6, +0.00e-6},
                           {-0.80e-6, +0.00e-6},
                           {+0.80e-6, +0.00e-6},
                           {-0.80e-6, +0.00e-6},
                           {+0.28e-6, +0.52e-6},
                           {+0.68e-6, -0.12e-6},
                           {+0.00e-6, -0.80e-6},
                           {-0.32e-6, +0.48e-6},
                           /* 601-800 */
                           {+0.36e-6, -0.44e-6},
                           {-0.36e-6, -0.44e-6},
                           {-0.80e-6, +0.00e-6},
                           {+0.79e-6, +0.00e-6},
                           {+0.74e-6, -0.04e-6},
                           {-0.76e-6, +0.00e-6},
                           {+0.00e-6, +0.76e-6},
                           {+0.16e-6, +0.60e-6},
                           {-0.76e-6, +0.00e-6},
                           {-0.76e-6, +0.00e-6},
                           {+0.76e-6, +0.00e-6},
                           {-0.76e-6, +0.00e-6},
                           {+0.76e-6, +0.00e-6},
                           {+0.12e-6, +0.64e-6},
                           {+0.76e-6, +0.00e-6},
                           {+0.00e-6, +0.76e-6},
                           {+0.76e-6, +0.00e-6},
                           {+0.64e-6, -0.12e-6},
                           {+0.16e-6, -0.60e-6},
                           {+0.76e-6, +0.00e-6},
                           {+0.00e-6, -0.76e-6},
                           {+0.28e-6, -0.48e-6},
                           {+0.32e-6, +0.44e-6},
                           {-0.76e-6, +0.00e-6},
                           {+0.72e-6, +0.00e-6},
                           {+0.72e-6, +0.00e-6},
                           {+0.48e-6, -0.24e-6},
                           {-0.72e-6, +0.00e-6},
                           {+0.72e-6, +0.00e-6},
                           {-0.72e-6, +0.00e-6},
                           {-0.72e-6, +0.00e-6},
                           {-0.71e-6, +0.00e-6},
                           {-0.68e-6, +0.00e-6},
                           {-0.68e-6, +0.00e-6},
                           {+0.68e-6, +0.00e-6},
                           {+0.68e-6, +0.00e-6},
                           {+0.68e-6, +0.00e-6},
                           {-0.68e-6, +0.00e-6},
                           {+0.56e-6, -0.12e-6},
                           {-0.68e-6, +0.00e-6},
                           {-0.68e-6, +0.00e-6},
                           {+0.20e-6, +0.48e-6},
                           {-0.44e-6, -0.24e-6},
                           {-0.68e-6, +0.00e-6},
                           {+0.64e-6, +0.00e-6},
                           {+0.64e-6, +0.00e-6},
                           {-0.64e-6, +0.00e-6},
                           {+0.64e-6, +0.00e-6},
                           {-0.64e-6, +0.00e-6},
                           {-0.12e-6, +0.52e-6},
                           {-0.12e-6, -0.52e-6},
                           {-0.16e-6, -0.48e-6},
                           {-0.20e-6, -0.44e-6},
                           {-0.44e-6, +0.20e-6},
                           {-0.44e-6, +0.20e-6},
                           {+0.24e-6, -0.40e-6},
                           {-0.20e-6, -0.44e-6},
                           {-0.64e-6, +0.00e-6},
                           {+0.40e-6, -0.24e-6},
                           {-0.64e-6, +0.00e-6},
                           {+0.64e-6, +0.00e-6},
                           {-0.63e-6, +0.00e-6},
                           {-0.60e-6, +0.00e-6},
                           {+0.00e-6, +0.60e-6},
                           {-0.60e-6, +0.00e-6},
                           {-0.60e-6, +0.00e-6},
                           {+0.60e-6, +0.00e-6},
                           {+0.00e-6, +0.60e-6},
                           {+0.24e-6, -0.36e-6},
                           {+0.12e-6, +0.48e-6},
                           {+0.48e-6, -0.12e-6},
                           {+0.12e-6, +0.48e-6},
                           {+0.24e-6, -0.36e-6},
                           {+0.36e-6, +0.24e-6},
                           {+0.12e-6, +0.48e-6},
                           {+0.44e-6, +0.16e-6},
                           {-0.60e-6, +0.00e-6},
                           {-0.60e-6, +0.00e-6},
                           {+0.60e-6, +0.00e-6},
                           {+0.00e-6, +0.60e-6},
                           {+0.59e-6, +0.00e-6},
                           {-0.56e-6, +0.00e-6},
                           {-0.44e-6, -0.12e-6},
                           {+0.56e-6, +0.00e-6},
                           {+0.00e-6, +0.56e-6},
                           {-0.56e-6, +0.00e-6},
                           {-0.56e-6, +0.00e-6},
                           {+0.56e-6, +0.00e-6},
                           {-0.56e-6, +0.00e-6},
                           {+0.16e-6, +0.40e-6},
                           {+0.44e-6, -0.12e-6},
                           {+0.20e-6, -0.36e-6},
                           {-0.36e-6, -0.20e-6},
                           {-0.56e-6, +0.00e-6},
                           {+0.55e-6, +0.00e-6},
                           {+0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {+0.52e-6, +0.00e-6},
                           {+0.52e-6, +0.00e-6},
                           {+0.16e-6, +0.36e-6},
                           {-0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {+0.00e-6, -0.52e-6},
                           {+0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {+0.12e-6, +0.40e-6},
                           {+0.52e-6, +0.00e-6},
                           {-0.52e-6, +0.00e-6},
                           {+0.00e-6, -0.52e-6},
                           {+0.52e-6, +0.00e-6},
                           {+0.52e-6, +0.00e-6},
                           {-0.51e-6, +0.00e-6},
                           {-0.51e-6, +0.00e-6},
                           {+0.48e-6, +0.00e-6},
                           {+0.48e-6, +0.00e-6},
                           {-0.16e-6, +0.32e-6},
                           {-0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {+0.48e-6, +0.00e-6},
                           {+0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {-0.12e-6, -0.36e-6},
                           {-0.32e-6, +0.16e-6},
                           {+0.32e-6, -0.16e-6},
                           {-0.12e-6, -0.36e-6},
                           {+0.16e-6, +0.32e-6},
                           {+0.20e-6, -0.28e-6},
                           {-0.20e-6, -0.28e-6},
                           {-0.36e-6, +0.12e-6},
                           {-0.48e-6, +0.00e-6},
                           {+0.32e-6, -0.16e-6},
                           {+0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {+0.00e-6, -0.48e-6},
                           {+0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {-0.48e-6, +0.00e-6},
                           {+0.00e-6, +0.48e-6},
                           {+0.44e-6, +0.00e-6},
                           {-0.32e-6, -0.12e-6},
                           {-0.44e-6, +0.00e-6},
                           {+0.20e-6, -0.24e-6},
                           {+0.44e-6, +0.00e-6},
                           {-0.44e-6, +0.00e-6},
                           {+0.44e-6, +0.00e-6},
                           {+0.20e-6, -0.24e-6},
                           {+0.12e-6, +0.32e-6},
                           {-0.20e-6, +0.24e-6},
                           {+0.32e-6, -0.12e-6},
                           {+0.00e-6, +0.44e-6},
                           {+0.00e-6, +0.44e-6},
                           {+0.44e-6, +0.00e-6},
                           {-0.44e-6, +0.00e-6},
                           {-0.44e-6, +0.00e-6},
                           {-0.44e-6, +0.00e-6},
                           {+0.44e-6, +0.00e-6},
                           {+0.44e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {-0.40e-6, +0.00e-6},
                           {-0.40e-6, +0.00e-6},
                           {-0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.24e-6, +0.16e-6},
                           {+0.00e-6, -0.40e-6},
                           {+0.12e-6, +0.28e-6},
                           {+0.40e-6, +0.00e-6},
                           {-0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.00e-6, -0.40e-6},
                           {-0.40e-6, +0.00e-6},
                           {+0.00e-6, -0.40e-6},
                           {+0.00e-6, -0.40e-6},
                           {+0.20e-6, -0.20e-6},
                           {-0.40e-6, +0.00e-6},
                           {-0.40e-6, +0.00e-6},
                           {-0.12e-6, -0.28e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.40e-6, +0.00e-6},
                           {+0.00e-6, +0.40e-6},
                           {-0.20e-6, -0.16e-6},
                           {+0.36e-6, +0.00e-6},
                           {+0.36e-6, +0.00e-6},
                           {+0.24e-6, -0.12e-6},
                           {+0.20e-6, -0.16e-6},
                           {+0.00e-6, +0.36e-6},
                           {+0.00e-6, +0.36e-6},
                           {-0.36e-6, +0.00e-6},
                           {+0.12e-6, +0.24e-6},
                           {-0.36e-6, +0.00e-6},
                           {-0.36e-6, +0.00e-6},
                           {-0.36e-6, +0.00e-6},
                           {-0.36e-6, +0.00e-6},
                           /* 801-1000 */
                           {+0.36e-6, +0.00e-6},
                           {+0.00e-6, +0.36e-6},
                           {+0.00e-6, +0.36e-6},
                           {+0.00e-6, +0.36e-6},
                           {-0.36e-6, +0.00e-6},
                           {+0.00e-6, +0.36e-6},
                           {+0.12e-6, -0.24e-6},
                           {-0.24e-6, +0.12e-6},
                           {-0.36e-6, +0.00e-6},
                           {+0.00e-6, +0.36e-6},
                           {+0.36e-6, +0.00e-6},
                           {+0.24e-6, -0.12e-6},
                           {+0.00e-6, -0.36e-6},
                           {-0.36e-6, +0.00e-6},
                           {+0.36e-6, +0.00e-6},
                           {+0.36e-6, +0.00e-6},
                           {-0.36e-6, +0.00e-6},
                           {+0.36e-6, +0.00e-6},
                           {-0.13e-6, +0.22e-6},
                           {-0.32e-6, +0.00e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.32e-6, +0.00e-6},
                           {-0.20e-6, -0.12e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.12e-6, +0.20e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.32e-6, +0.00e-6},
                           {-0.32e-6, +0.00e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.00e-6, -0.32e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.12e-6, -0.20e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.00e-6, -0.32e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.00e-6, +0.32e-6},
                           {+0.00e-6, -0.32e-6},
                           {+0.00e-6, -0.32e-6},
                           {+0.32e-6, +0.00e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.00e-6, +0.32e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.00e-6, +0.32e-6},
                           {+0.00e-6, -0.32e-6},
                           {+0.32e-6, +0.00e-6},
                           {-0.16e-6, +0.16e-6},
                           {-0.16e-6, +0.16e-6},
                           {+0.00e-6, +0.32e-6},
                           {+0.20e-6, +0.12e-6},
                           {+0.20e-6, +0.12e-6},
                           {-0.20e-6, +0.12e-6},
                           {+0.12e-6, +0.20e-6},
                           {+0.12e-6, -0.20e-6},
                           {+0.00e-6, +0.32e-6},
                           {-0.32e-6, +0.00e-6},
                           {+0.32e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.16e-6, +0.12e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {-0.12e-6, -0.16e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.00e-6, +0.28e-6},
                           {+0.00e-6, +0.28e-6},
                           {+0.00e-6, -0.28e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.12e-6, -0.16e-6},
                           {+0.00e-6, +0.28e-6},
                           {+0.00e-6, -0.28e-6},
                           {+0.12e-6, -0.16e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.00e-6, -0.28e-6},
                           {+0.00e-6, +0.28e-6},
                           {+0.00e-6, -0.28e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.12e-6, -0.16e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.28e-6, +0.00e-6},
                           {-0.28e-6, +0.00e-6},
                           {+0.00e-6, -0.28e-6},
                           {+0.00e-6, -0.28e-6},
                           {+0.28e-6, +0.00e-6},
                           {+0.00e-6, +0.24e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.00e-6, -0.24e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.00e-6, -0.24e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.00e-6, +0.24e-6},
                           {+0.12e-6, -0.12e-6},
                           {+0.00e-6, -0.24e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.00e-6, +0.24e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.00e-6, -0.24e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {-0.24e-6, +0.00e-6},
                           {+0.00e-6, +0.24e-6},
                           {+0.00e-6, -0.24e-6},
                           {+0.24e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.00e-6, -0.20e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.00e-6, +0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {+0.00e-6, -0.20e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           /* 1001-1200 */
                           {+0.00e-6, +0.20e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {+0.00e-6, +0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, -0.20e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.00e-6, +0.20e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {-0.20e-6, +0.00e-6},
                           {+0.20e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.04e-6, +0.12e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.00e-6, -0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.00e-6, +0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {-0.16e-6, +0.00e-6},
                           {+0.00e-6, -0.16e-6},
                           {+0.16e-6, +0.00e-6},
                           {+0.16e-6, +0.00e-6},
                           {-0.15e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.08e-6, +0.04e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           /* 1201-1306 */
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, -0.12e-6},
                           {+0.00e-6, +0.12e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {+0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {-0.12e-6, +0.00e-6},
                           {+0.11e-6, +0.00e-6}};
    double SX1[253][2] = {/* 1-200 */
                          {-3328.48e-6, +205833.15e-6},
                          {+197.53e-6, +12814.01e-6},
                          {+41.19e-6, +2187.91e-6},
                          {-35.85e-6, -2004.36e-6},
                          {+59.15e-6, +501.82e-6},
                          {-5.82e-6, +448.76e-6},
                          {-179.56e-6, +164.33e-6},
                          {+5.67e-6, +288.49e-6},
                          {+23.85e-6, -214.50e-6},
                          {+2.87e-6, -154.91e-6},
                          {+2.14e-6, -119.21e-6},
                          {+1.17e-6, -74.33e-6},
                          {+1.47e-6, +70.31e-6},
                          {-0.42e-6, +58.94e-6},
                          {-0.95e-6, +57.12e-6},
                          {-1.08e-6, -54.19e-6},
                          {+0.92e-6, +36.78e-6},
                          {+0.68e-6, -31.01e-6},
                          {+0.74e-6, +29.60e-6},
                          {-0.60e-6, -27.59e-6},
                          {-11.11e-6, -15.07e-6},
                          {-0.40e-6, -24.05e-6},
                          {-0.81e-6, +19.06e-6},
                          {+3.18e-6, +15.32e-6},
                          {-0.08e-6, -17.90e-6},
                          {-0.16e-6, +15.55e-6},
                          {-0.77e-6, +14.40e-6},
                          {-0.25e-6, +11.67e-6},
                          {+6.18e-6, +3.58e-6},
                          {-1.00e-6, -7.27e-6},
                          {-0.99e-6, +6.87e-6},
                          {-0.27e-6, +7.49e-6},
                          {-0.30e-6, +7.31e-6},
                          {+0.20e-6, +7.30e-6},
                          {+0.33e-6, +6.80e-6},
                          {+0.27e-6, -6.81e-6},
                          {+0.35e-6, +6.08e-6},
                          {+0.35e-6, +6.09e-6},
                          {-0.14e-6, -6.19e-6},
                          {+0.14e-6, +6.02e-6},
                          {+2.69e-6, -2.76e-6},
                          {-0.08e-6, -4.93e-6},
                          {+2.85e-6, -1.77e-6},
                          {-0.07e-6, -4.27e-6},
                          {-3.71e-6, +0.38e-6},
                          {+3.75e-6, +0.04e-6},
                          {-0.82e-6, -2.73e-6},
                          {-0.06e-6, +2.93e-6},
                          {-0.04e-6, +2.83e-6},
                          {+0.08e-6, +2.75e-6},
                          {+0.07e-6, +2.75e-6},
                          {-0.07e-6, +2.70e-6},
                          {-0.07e-6, +2.52e-6},
                          {-0.05e-6, -2.53e-6},
                          {-0.04e-6, +2.40e-6},
                          {-0.06e-6, -2.37e-6},
                          {+0.69e-6, -1.45e-6},
                          {-0.04e-6, +2.00e-6},
                          {+1.99e-6, +0.02e-6},
                          {-0.94e-6, +1.07e-6},
                          {+0.04e-6, +1.91e-6},
                          {-0.58e-6, -1.36e-6},
                          {-0.51e-6, -1.25e-6},
                          {-0.04e-6, -1.59e-6},
                          {+0.39e-6, -1.23e-6},
                          {+0.03e-6, -1.57e-6},
                          {-0.03e-6, +1.50e-6},
                          {+0.04e-6, +1.48e-6},
                          {-0.04e-6, +1.45e-6},
                          {+0.02e-6, -1.36e-6},
                          {+0.03e-6, -1.32e-6},
                          {-0.03e-6, -1.24e-6},
                          {-0.02e-6, -1.18e-6},
                          {-0.03e-6, +1.16e-6},
                          {+0.02e-6, +1.13e-6},
                          {+0.04e-6, -1.11e-6},
                          {+0.02e-6, +1.11e-6},
                          {+0.03e-6, -1.10e-6},
                          {+0.03e-6, +1.04e-6},
                          {-0.51e-6, +0.56e-6},
                          {+0.02e-6, -0.98e-6},
                          {-0.02e-6, -0.94e-6},
                          {-0.02e-6, -0.89e-6},
                          {-0.02e-6, -0.88e-6},
                          {+0.31e-6, +0.60e-6},
                          {+0.02e-6, -0.87e-6},
                          {-0.02e-6, -0.87e-6},
                          {-0.01e-6, +0.83e-6},
                          {-0.02e-6, +0.77e-6},
                          {+0.42e-6, -0.36e-6},
                          {-0.01e-6, -0.73e-6},
                          {+0.01e-6, +0.71e-6},
                          {+0.01e-6, +0.68e-6},
                          {+0.02e-6, +0.66e-6},
                          {-0.01e-6, -0.62e-6},
                          {-0.01e-6, +0.62e-6},
                          {-0.58e-6, -0.03e-6},
                          {-0.01e-6, +0.58e-6},
                          {+0.44e-6, +0.14e-6},
                          {+0.02e-6, +0.56e-6},
                          {+0.01e-6, -0.57e-6},
                          {-0.13e-6, -0.45e-6},
                          {+0.01e-6, +0.56e-6},
                          {+0.01e-6, -0.55e-6},
                          {+0.01e-6, +0.55e-6},
                          {-0.52e-6, +0.03e-6},
                          {-0.01e-6, +0.54e-6},
                          {-0.01e-6, -0.51e-6},
                          {-0.41e-6, -0.11e-6},
                          {-0.01e-6, +0.50e-6},
                          {+0.01e-6, +0.48e-6},
                          {+0.45e-6, -0.04e-6},
                          {+0.01e-6, -0.48e-6},
                          {+0.01e-6, +0.46e-6},
                          {-0.23e-6, +0.24e-6},
                          {+0.01e-6, +0.46e-6},
                          {+0.35e-6, -0.11e-6},
                          {+0.01e-6, +0.45e-6},
                          {+0.01e-6, -0.45e-6},
                          {+0.00e-6, -0.45e-6},
                          {-0.01e-6, +0.44e-6},
                          {+0.35e-6, +0.09e-6},
                          {+0.01e-6, +0.42e-6},
                          {-0.01e-6, -0.41e-6},
                          {+0.09e-6, -0.33e-6},
                          {+0.00e-6, +0.41e-6},
                          {+0.01e-6, +0.40e-6},
                          {-0.01e-6, -0.39e-6},
                          {-0.39e-6, -0.01e-6},
                          {+0.01e-6, -0.39e-6},
                          {-0.01e-6, +0.38e-6},
                          {+0.32e-6, -0.07e-6},
                          {-0.01e-6, +0.36e-6},
                          {-0.01e-6, -0.36e-6},
                          {+0.01e-6, -0.34e-6},
                          {+0.01e-6, -0.34e-6},
                          {+0.01e-6, +0.33e-6},
                          {-0.01e-6, -0.32e-6},
                          {+0.01e-6, +0.32e-6},
                          {-0.01e-6, -0.32e-6},
                          {-0.01e-6, -0.31e-6},
                          {-0.31e-6, +0.00e-6},
                          {-0.07e-6, -0.24e-6},
                          {+0.10e-6, -0.21e-6},
                          {-0.01e-6, -0.30e-6},
                          {-0.01e-6, +0.29e-6},
                          {-0.01e-6, -0.29e-6},
                          {+0.00e-6, +0.29e-6},
                          {+0.23e-6, +0.06e-6},
                          {+0.26e-6, +0.02e-6},
                          {+0.00e-6, -0.27e-6},
                          {+0.25e-6, +0.02e-6},
                          {+0.09e-6, -0.18e-6},
                          {+0.01e-6, +0.25e-6},
                          {+0.14e-6, -0.11e-6},
                          {+0.00e-6, -0.25e-6},
                          {+0.01e-6, +0.24e-6},
                          {-0.01e-6, -0.24e-6},
                          {+0.00e-6, +0.23e-6},
                          {+0.01e-6, +0.23e-6},
                          {-0.01e-6, -0.23e-6},
                          {+0.00e-6, -0.23e-6},
                          {+0.00e-6, -0.22e-6},
                          {+0.00e-6, +0.21e-6},
                          {+0.01e-6, +0.21e-6},
                          {-0.17e-6, +0.03e-6},
                          {-0.17e-6, +0.03e-6},
                          {+0.00e-6, -0.19e-6},
                          {+0.14e-6, -0.06e-6},
                          {+0.03e-6, -0.17e-6},
                          {-0.13e-6, +0.06e-6},
                          {+0.00e-6, +0.19e-6},
                          {+0.00e-6, +0.19e-6},
                          {-0.06e-6, -0.13e-6},
                          {+0.00e-6, +0.18e-6},
                          {-0.09e-6, -0.09e-6},
                          {+0.10e-6, -0.09e-6},
                          {+0.06e-6, +0.12e-6},
                          {+0.00e-6, +0.18e-6},
                          {+0.00e-6, -0.18e-6},
                          {+0.00e-6, +0.17e-6},
                          {-0.03e-6, +0.15e-6},
                          {-0.01e-6, -0.16e-6},
                          {+0.00e-6, +0.17e-6},
                          {+0.00e-6, -0.17e-6},
                          {+0.11e-6, +0.06e-6},
                          {+0.00e-6, -0.17e-6},
                          {-0.08e-6, +0.09e-6},
                          {-0.17e-6, +0.00e-6},
                          {+0.00e-6, -0.16e-6},
                          {+0.01e-6, +0.15e-6},
                          {-0.13e-6, -0.03e-6},
                          {+0.00e-6, +0.15e-6},
                          {+0.00e-6, +0.15e-6},
                          {-0.13e-6, +0.03e-6},
                          {+0.10e-6, -0.06e-6},
                          {-0.07e-6, +0.08e-6},
                          {-0.09e-6, -0.06e-6},
                          {+0.00e-6, +0.15e-6},
                          {-0.07e-6, -0.08e-6},
                          /* 201-253 */
                          {+0.00e-6, -0.14e-6},
                          {+0.02e-6, +0.12e-6},
                          {+0.07e-6, +0.08e-6},
                          {-0.03e-6, -0.11e-6},
                          {-0.01e-6, -0.14e-6},
                          {+0.00e-6, -0.14e-6},
                          {+0.02e-6, -0.12e-6},
                          {+0.00e-6, -0.14e-6},
                          {+0.00e-6, +0.14e-6},
                          {+0.00e-6, +0.14e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.08e-6, -0.06e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.01e-6, +0.13e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.00e-6, +0.13e-6},
                          {-0.02e-6, -0.11e-6},
                          {+0.08e-6, -0.04e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.00e-6, +0.13e-6},
                          {+0.01e-6, -0.12e-6},
                          {+0.00e-6, +0.12e-6},
                          {-0.02e-6, -0.11e-6},
                          {+0.00e-6, -0.12e-6},
                          {+0.00e-6, -0.12e-6},
                          {+0.00e-6, -0.12e-6},
                          {+0.04e-6, +0.08e-6},
                          {+0.00e-6, -0.12e-6},
                          {+0.00e-6, +0.12e-6},
                          {+0.00e-6, -0.12e-6},
                          {+0.00e-6, -0.11e-6},
                          {+0.03e-6, -0.09e-6},
                          {+0.00e-6, +0.11e-6},
                          {-0.11e-6, +0.00e-6},
                          {+0.00e-6, +0.11e-6},
                          {+0.00e-6, -0.11e-6},
                          {+0.07e-6, +0.05e-6},
                          {+0.11e-6, +0.00e-6},
                          {+0.00e-6, -0.11e-6},
                          {+0.00e-6, -0.11e-6},
                          {+0.02e-6, -0.09e-6},
                          {+0.00e-6, +0.11e-6},
                          {+0.02e-6, +0.09e-6},
                          {+0.00e-6, -0.11e-6},
                          {+0.00e-6, +0.11e-6},
                          {-0.08e-6, -0.02e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {-0.03e-6, -0.07e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, -0.10e-6}};
    double SX2[36][2] = {{+2038.00e-6, +82.26e-6}, {+155.75e-6, -2.70e-6}, {+26.92e-6, -0.45e-6}, {-24.43e-6, +0.46e-6},
                         {-17.36e-6, -0.50e-6},    {-8.41e-6, +0.01e-6},   {+6.08e-6, -1.36e-6},  {+4.59e-6, +0.17e-6},
                         {+3.57e-6, -0.06e-6},     {+2.54e-6, +0.60e-6},   {+1.86e-6, +0.00e-6},  {-1.52e-6, -0.07e-6},
                         {+1.46e-6, +0.04e-6},     {-0.75e-6, -0.02e-6},   {-0.75e-6, +0.00e-6},  {-0.71e-6, -0.01e-6},
                         {-0.69e-6, +0.02e-6},     {+0.61e-6, +0.02e-6},   {+0.54e-6, -0.04e-6},  {-0.56e-6, +0.00e-6},
                         {+0.46e-6, -0.02e-6},     {+0.38e-6, -0.01e-6},   {+0.37e-6, -0.02e-6},  {-0.34e-6, +0.01e-6},
                         {-0.35e-6, +0.00e-6},     {-0.31e-6, +0.00e-6},   {+0.19e-6, -0.09e-6},  {+0.26e-6, +0.00e-6},
                         {+0.24e-6, -0.01e-6},     {-0.20e-6, +0.00e-6},   {+0.18e-6, -0.01e-6},  {+0.17e-6, +0.00e-6},
                         {+0.15e-6, +0.01e-6},     {-0.15e-6, +0.00e-6},   {-0.13e-6, +0.00e-6},  {-0.12e-6, +0.00e-6}};
    double SX3[4][2] = {{+1.76e-6, -20.39e-6}, {+0.00e-6, -1.27e-6}, {+0.00e-6, -0.22e-6}, {+0.00e-6, +0.20e-6}};
    double SX4[1][2] = {{-0.10e-6, -0.02e-6}};

    int KY0[962][14] = {/* 1-200 */
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                        {0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -1},
                        {0, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -2},
                        {1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
                        {2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -1},
                        {3, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -8, 3, 0, 0, 0, -2},
                        {1, -1, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
                        {0, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -2},
                        {2, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
                        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {1, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -2},
                        {1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 3, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                        {1, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},
                        {1, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},
                        {1, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                        {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                        {1, 0, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
                        {2, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},
                        {1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -2},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -2},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                        {0, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
                        {0, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},
                        {0, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -2},
                        {1, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -1},
                        {2, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0, -2},
                        {1, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},
                        {3, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},
                        {1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
                        {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        /* 201-400 */
                        {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
                        {1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                        {0, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {1, -1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},
                        {2, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -1},
                        {1, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},
                        {1, 0, 0, 0, -1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -5, 8, -3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -2},
                        {0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -8, 13, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -2},
                        {0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, -2, 5, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
                        {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                        {2, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                        {0, 2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                        {3, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                        {2, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -1},
                        {2, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2},
                        {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 2},
                        {1, 1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, -1},
                        {1, -1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2},
                        {2, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
                        {0, 0, 2, -2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 2},
                        {1, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -2},
                        {2, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                        {1, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, -1},
                        {0, 0, 2, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {1, 0, -1, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 6, -16, 4, 5, 0, 0, -2},
                        {2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -1},
                        {2, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, -1},
                        {0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 2},
                        {1, 0, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -1},
                        {1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -4, 5, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -4, 10, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 2, 0},
                        {2, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -3, 0, 0, 0, 2},
                        {0, 0, 0, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -4, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -1},
                        {2, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -9, 13, 0, 0, 0, 0, 0},
                        {0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {1, 0, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 1, 0, 0, -2, 4, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
                        {2, 0, 2, 0, 2, 0, 0, 2, 0, -3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 1},
                        {1, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                        {1, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                        {0, 1, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -1, 0, 0, 0, 2},
                        {0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, -8, 3, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, -2},
                        {1, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        /* 401-600 */
                        {0, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, -1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 8, -14, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 1},
                        {3, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -1, 0, 0, 2},
                        {0, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                        {2, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -2},
                        {0, 0, 1, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2},
                        {1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                        {0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 7, -8, 3, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                        {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -2},
                        {1, -2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0},
                        {1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 1, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -9, 15, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 3, -5, 4, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 3, -9, 4, 0, 0, 0, 0, -2},
                        {1, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {1, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -2},
                        {5, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -1, 0, 0, 2},
                        {1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -1},
                        {0, 0, 4, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, -5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 5, 0, 0, 2},
                        {0, 0, 0, 0, 1, 0, -3, 7, -4, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 3, -7, 4, 0, 0, 0, 0, 0},
                        {0, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                        {1, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                        {0, 1, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 1, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 3, -5, 0, 2, 0, 0, 0, 0},
                        {0, 1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 2, -4, 0, -3, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 6, -15, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, 2, -5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, -2},
                        {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                        {2, -1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {0, 0, 2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, -1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 5, 0, -2, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -1},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 0, 5, 0, 0, 0},
                        {2, 0, -1, -1, -1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 3, 0, 0, 0},
                        {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 1, -5, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1},
                        {4, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                        {1, -2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                        {2, 0, -1, -1, -1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 1, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, -1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 2},
                        {3, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        /* 601-800 */
                        {3, -1, -2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 9, -12, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, -2},
                        {0, 0, 2, -2, 1, -1, 0, 2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 5, 0, -3, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2},
                        {0, 0, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                        {1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -8, 15, 0, 0, 0, 0, 0},
                        {1, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -2, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 4, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2},
                        {2, -2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 6, -5, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 2},
                        {5, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 2, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -8, 11, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 2, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 5, -2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2},
                        {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 0, 2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                        {2, 0, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, -1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {2, -2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                        {1, -2, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0},
                        {2, 0, 0, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                        {2, 0, -1, -1, 1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                        {2, 1, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                        {2, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                        {0, 1, 0, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -9, 17, 0, 0, 0, 0, 0},
                        {1, 1, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -7, 9, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, -2},
                        {4, 0, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 4, -5, 0, 0, 0},
                        {2, 0, 0, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {2, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {1, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 1, -1, 0, -18, 17, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, -1, -1, 0, 20, -20, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 1, -2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 8, -16, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, -2, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -2, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 1, -6, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2},
                        {1, 0, 0, -1, -1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                        {2, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -1, -1, -1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {1, 0, 0, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 5, -8, 0, 0, 0, 0, 0, 0},
                        {1, -2, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {1, -2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -4, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                        {0, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                        {1, 0, -2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                        {2, 0, 0, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                        /* 801-962 */
                        {1, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {1, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {2, 0, -4, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {2, -1, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                        {1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, -1, 0, 0, 0},
                        {1, -1, -4, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -1, -1, -2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {4, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 2, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -3, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -6, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, -1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {1, -1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 3, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -3, 0, 3, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, -4, 4, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -5, 5, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 1, -3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 1, -4, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, -3, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, -1, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -4, 6, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -12, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 3, -8, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -4, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -5, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 2, -7, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 5, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -5, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 2},
                        {1, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {4, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 7, -13, 0, 0, 0, 0, 0},
                        {2, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {1, 2, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -6, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -2, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, -3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                        {2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                        {0, 1, 2, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 2, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 1, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 4, -4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                        {3, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                        {2, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {1, 2, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KY1[277][14] = {/* 1-200 */
                        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                        {1, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -1},
                        {0, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -2},
                        {1, 0, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 0},
                        {1, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 2, -2, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                        {2, 0, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
                        {1, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                        {0, 1, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -8, 3, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
                        {0, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
                        {1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},
                        {3, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -2},
                        {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 3, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                        {1, -1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                        {1, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},
                        {2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                        {1, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                        {1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 0, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                        {0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, 0, 0, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                        {0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -2},
                        {1, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},
                        {4, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                        {1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        /* 201-277 */
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                        {0, 0, 2, -2, 0, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
                        {2, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -2},
                        {0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, -2, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -2},
                        {0, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, -4, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                        {1, 0, -2, 0, -2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                        {0, 0, 2, -3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
                        {0, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
                        {1, 1, -2, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},
                        {2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -2},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, -2},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},
                        {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
                        {1, 0, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -2, 0, 0, 19, -21, 3, 0, 0, 0, 0, 0},
                        {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0, -2},
                        {0, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 2, -4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
                        {3, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
                        {1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 4, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {3, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},
                        {2, 1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, 0, 0, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -1},
                        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -1},
                        {0, 2, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0},
                        {0, 0, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {3, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -2},
                        {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -2},
                        {2, 1, 0, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
                        {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -2},
                        {4, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {2, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {1, -1, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                        {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -2}};
    int KY2[30][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 1, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {1, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 1, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 0, 2, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, -2, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {2, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {1, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {0, 2, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},    {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},  {1, 0, 0, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},   {1, 0, -2, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                       {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0}, {0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KY3[5][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int KY4[1][14] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    double SY0[962][2] = {/* 1-200 */
                          {+1538.18e-6, +9205236.26e-6},
                          {-458.66e-6, +573033.42e-6},
                          {+137.41e-6, +97846.69e-6},
                          {-29.05e-6, -89618.24e-6},
                          {-17.40e-6, +22438.42e-6},
                          {+31.80e-6, +20069.50e-6},
                          {+36.70e-6, +12902.66e-6},
                          {-13.20e-6, -9592.72e-6},
                          {-192.40e-6, +7387.02e-6},
                          {+3.92e-6, -6918.22e-6},
                          {+0.40e-6, -5331.13e-6},
                          {-0.90e-6, -3323.89e-6},
                          {+7.50e-6, +3143.98e-6},
                          {+7.80e-6, +2636.13e-6},
                          {-6.60e-6, +2554.51e-6},
                          {-2.00e-6, -2423.59e-6},
                          {+6.80e-6, +1645.01e-6},
                          {+0.00e-6, -1387.00e-6},
                          {+5.90e-6, +1323.81e-6},
                          {-0.30e-6, -1233.89e-6},
                          {+0.30e-6, -1075.60e-6},
                          {-4.48e-6, +852.85e-6},
                          {+0.10e-6, -800.34e-6},
                          {+35.80e-6, -674.99e-6},
                          {-1.40e-6, +695.54e-6},
                          {-0.50e-6, +684.99e-6},
                          {-2.62e-6, +643.75e-6},
                          {-1.50e-6, +522.11e-6},
                          {+273.50e-6, +164.70e-6},
                          {+1.40e-6, +335.24e-6},
                          {+1.90e-6, +326.60e-6},
                          {+0.40e-6, +327.11e-6},
                          {-0.50e-6, -325.03e-6},
                          {-0.40e-6, +307.03e-6},
                          {+0.50e-6, +304.17e-6},
                          {-0.10e-6, -304.46e-6},
                          {-0.40e-6, -276.81e-6},
                          {+0.90e-6, +272.05e-6},
                          {+0.30e-6, +272.22e-6},
                          {+1.20e-6, +269.45e-6},
                          {+0.10e-6, -220.67e-6},
                          {+128.60e-6, -77.10e-6},
                          {+0.10e-6, -190.79e-6},
                          {+167.90e-6, +0.00e-6},
                          {-8.20e-6, -123.48e-6},
                          {+0.10e-6, +131.04e-6},
                          {+0.40e-6, +126.64e-6},
                          {+2.90e-6, -122.28e-6},
                          {+0.70e-6, +123.20e-6},
                          {+0.40e-6, +123.20e-6},
                          {-0.30e-6, +120.70e-6},
                          {-0.50e-6, +112.90e-6},
                          {-0.20e-6, -112.94e-6},
                          {+0.20e-6, +107.31e-6},
                          {-0.30e-6, -106.20e-6},
                          {+31.90e-6, -64.10e-6},
                          {+0.00e-6, +89.50e-6},
                          {+89.10e-6, +0.00e-6},
                          {+0.00e-6, +85.32e-6},
                          {-0.20e-6, -71.00e-6},
                          {+0.00e-6, -70.01e-6},
                          {+13.90e-6, -55.30e-6},
                          {+0.00e-6, +67.25e-6},
                          {+0.40e-6, +66.29e-6},
                          {-0.40e-6, +64.70e-6},
                          {+1.30e-6, -60.90e-6},
                          {-0.20e-6, -60.92e-6},
                          {+0.20e-6, -59.20e-6},
                          {+1.10e-6, -55.55e-6},
                          {+0.00e-6, -55.60e-6},
                          {-0.10e-6, -52.69e-6},
                          {-0.20e-6, +51.80e-6},
                          {+1.00e-6, -49.51e-6},
                          {+0.00e-6, +50.50e-6},
                          {+2.50e-6, +47.70e-6},
                          {+0.10e-6, +49.59e-6},
                          {+0.10e-6, -49.00e-6},
                          {-23.20e-6, +24.60e-6},
                          {+0.40e-6, +46.50e-6},
                          {-0.10e-6, -44.04e-6},
                          {-0.10e-6, -42.19e-6},
                          {+13.30e-6, +26.90e-6},
                          {-0.10e-6, -39.90e-6},
                          {-0.10e-6, -39.50e-6},
                          {+0.00e-6, -39.11e-6},
                          {-0.10e-6, -38.92e-6},
                          {+0.10e-6, +36.95e-6},
                          {-0.10e-6, +34.59e-6},
                          {+0.20e-6, -32.55e-6},
                          {-0.10e-6, +31.61e-6},
                          {+0.00e-6, +30.40e-6},
                          {+0.20e-6, +29.40e-6},
                          {+0.00e-6, -27.91e-6},
                          {+0.10e-6, +27.50e-6},
                          {-25.70e-6, -1.70e-6},
                          {+19.90e-6, +5.90e-6},
                          {+0.00e-6, +25.80e-6},
                          {+0.20e-6, +25.20e-6},
                          {+0.00e-6, -25.31e-6},
                          {+0.20e-6, +25.00e-6},
                          {-0.10e-6, +24.40e-6},
                          {+0.10e-6, -24.40e-6},
                          {-23.30e-6, +0.90e-6},
                          {-0.10e-6, +24.00e-6},
                          {-18.00e-6, -5.30e-6},
                          {-0.10e-6, -22.80e-6},
                          {-0.10e-6, +22.50e-6},
                          {+0.10e-6, +21.60e-6},
                          {+0.00e-6, -21.30e-6},
                          {+0.10e-6, +20.70e-6},
                          {+0.70e-6, -20.10e-6},
                          {+0.00e-6, +20.51e-6},
                          {+15.90e-6, -4.50e-6},
                          {+0.20e-6, -19.94e-6},
                          {+0.00e-6, +20.11e-6},
                          {+15.60e-6, +4.40e-6},
                          {+0.00e-6, -20.00e-6},
                          {+0.00e-6, +19.80e-6},
                          {+0.00e-6, +18.91e-6},
                          {+4.30e-6, -14.60e-6},
                          {-0.10e-6, -18.50e-6},
                          {-0.10e-6, +18.40e-6},
                          {+0.00e-6, +18.10e-6},
                          {+1.00e-6, +16.81e-6},
                          {-0.10e-6, -17.60e-6},
                          {-17.60e-6, +0.00e-6},
                          {-1.30e-6, -16.26e-6},
                          {+0.00e-6, -17.41e-6},
                          {+14.50e-6, -2.70e-6},
                          {+0.00e-6, +17.08e-6},
                          {+0.00e-6, +16.21e-6},
                          {+0.00e-6, -16.00e-6},
                          {+0.00e-6, -15.31e-6},
                          {+0.00e-6, -15.10e-6},
                          {+0.00e-6, +14.70e-6},
                          {+0.00e-6, +14.40e-6},
                          {-0.10e-6, -14.30e-6},
                          {+0.00e-6, -14.40e-6},
                          {+0.00e-6, -13.81e-6},
                          {+4.50e-6, -9.30e-6},
                          {-13.80e-6, +0.00e-6},
                          {+0.00e-6, -13.38e-6},
                          {-0.10e-6, +13.10e-6},
                          {+10.30e-6, +2.70e-6},
                          {+0.00e-6, +12.80e-6},
                          {+0.00e-6, -12.80e-6},
                          {+11.70e-6, +0.80e-6},
                          {+0.00e-6, -12.00e-6},
                          {+11.30e-6, +0.50e-6},
                          {+0.00e-6, +11.40e-6},
                          {+0.00e-6, -11.20e-6},
                          {+0.10e-6, +10.90e-6},
                          {+0.10e-6, -10.77e-6},
                          {+0.00e-6, -10.80e-6},
                          {-0.20e-6, +10.47e-6},
                          {+0.00e-6, +10.50e-6},
                          {+0.00e-6, -10.40e-6},
                          {+0.00e-6, +10.40e-6},
                          {+0.00e-6, -10.20e-6},
                          {+0.00e-6, -10.00e-6},
                          {+0.00e-6, +9.60e-6},
                          {+0.10e-6, +9.40e-6},
                          {-7.60e-6, +1.70e-6},
                          {-7.70e-6, +1.40e-6},
                          {+1.40e-6, -7.50e-6},
                          {+6.10e-6, -2.70e-6},
                          {+0.00e-6, -8.70e-6},
                          {-5.90e-6, +2.60e-6},
                          {+0.00e-6, +8.40e-6},
                          {-0.20e-6, -8.11e-6},
                          {-2.60e-6, -5.70e-6},
                          {+0.00e-6, +8.30e-6},
                          {+2.70e-6, +5.50e-6},
                          {+4.20e-6, -4.00e-6},
                          {-0.10e-6, +8.00e-6},
                          {+0.00e-6, +8.09e-6},
                          {-1.30e-6, +6.70e-6},
                          {+0.00e-6, -7.90e-6},
                          {+0.00e-6, +7.80e-6},
                          {-7.50e-6, -0.20e-6},
                          {-0.50e-6, -7.20e-6},
                          {+4.90e-6, +2.70e-6},
                          {+0.00e-6, +7.50e-6},
                          {+0.00e-6, -7.50e-6},
                          {+0.00e-6, -7.49e-6},
                          {+0.00e-6, -7.20e-6},
                          {+0.10e-6, +6.90e-6},
                          {-5.60e-6, +1.40e-6},
                          {-5.70e-6, -1.30e-6},
                          {+0.00e-6, +6.90e-6},
                          {+4.20e-6, -2.70e-6},
                          {+0.00e-6, +6.90e-6},
                          {-3.10e-6, +3.70e-6},
                          {-3.90e-6, -2.90e-6},
                          {+0.00e-6, +6.60e-6},
                          {-3.10e-6, -3.50e-6},
                          {+1.10e-6, -5.39e-6},
                          {+0.00e-6, -6.40e-6},
                          {+0.90e-6, +5.50e-6},
                          {+0.00e-6, -6.30e-6},
                          /* 201-400 */
                          {-0.10e-6, -6.20e-6},
                          {+0.00e-6, -6.10e-6},
                          {+0.00e-6, +6.10e-6},
                          {+0.00e-6, +6.10e-6},
                          {+3.50e-6, -2.50e-6},
                          {+0.00e-6, +6.00e-6},
                          {+0.00e-6, +5.90e-6},
                          {-0.90e-6, -4.80e-6},
                          {+0.00e-6, +5.70e-6},
                          {+0.10e-6, +5.60e-6},
                          {+0.00e-6, +5.70e-6},
                          {+0.00e-6, +5.70e-6},
                          {+0.00e-6, +5.60e-6},
                          {+0.00e-6, +5.60e-6},
                          {+0.20e-6, -5.40e-6},
                          {-0.90e-6, -4.70e-6},
                          {-0.40e-6, -5.10e-6},
                          {+0.00e-6, +5.50e-6},
                          {+0.00e-6, -5.40e-6},
                          {+0.00e-6, -5.40e-6},
                          {+1.80e-6, +3.60e-6},
                          {+0.00e-6, +5.30e-6},
                          {+0.00e-6, -5.30e-6},
                          {+0.00e-6, -5.20e-6},
                          {+0.00e-6, -5.19e-6},
                          {+3.00e-6, +2.10e-6},
                          {+0.00e-6, -5.10e-6},
                          {+0.00e-6, +5.07e-6},
                          {+0.90e-6, -4.10e-6},
                          {-5.00e-6, +0.00e-6},
                          {+0.00e-6, -5.00e-6},
                          {+0.00e-6, +5.00e-6},
                          {+0.00e-6, -5.00e-6},
                          {+0.00e-6, -4.90e-6},
                          {+4.90e-6, +0.00e-6},
                          {+0.00e-6, -4.90e-6},
                          {+0.90e-6, +3.90e-6},
                          {+0.00e-6, +4.80e-6},
                          {-3.70e-6, -1.10e-6},
                          {+0.00e-6, -4.72e-6},
                          {+0.00e-6, +4.71e-6},
                          {+0.00e-6, -4.50e-6},
                          {-1.50e-6, -3.00e-6},
                          {+0.00e-6, -4.50e-6},
                          {+0.30e-6, -4.11e-6},
                          {+0.00e-6, +4.40e-6},
                          {+0.00e-6, -4.40e-6},
                          {+0.00e-6, +4.39e-6},
                          {+0.00e-6, -4.30e-6},
                          {+0.00e-6, +4.30e-6},
                          {+0.00e-6, -4.30e-6},
                          {+0.20e-6, +4.03e-6},
                          {+0.20e-6, +4.00e-6},
                          {-0.60e-6, +3.50e-6},
                          {+0.00e-6, +4.10e-6},
                          {+0.00e-6, +4.00e-6},
                          {+0.00e-6, -4.00e-6},
                          {+0.00e-6, -3.91e-6},
                          {+1.90e-6, +2.00e-6},
                          {+0.00e-6, +3.90e-6},
                          {+0.00e-6, +3.90e-6},
                          {+0.00e-6, -3.90e-6},
                          {+3.10e-6, -0.80e-6},
                          {+0.00e-6, +3.90e-6},
                          {+0.00e-6, +3.90e-6},
                          {+0.00e-6, +3.80e-6},
                          {-0.20e-6, +3.51e-6},
                          {+0.00e-6, -3.60e-6},
                          {-2.10e-6, +1.50e-6},
                          {+0.00e-6, -3.60e-6},
                          {+0.70e-6, +2.80e-6},
                          {-2.80e-6, +0.70e-6},
                          {+0.00e-6, -3.50e-6},
                          {-2.90e-6, -0.60e-6},
                          {+0.00e-6, -3.40e-6},
                          {+0.00e-6, +3.40e-6},
                          {+0.00e-6, +3.36e-6},
                          {+0.50e-6, +2.80e-6},
                          {+2.60e-6, -0.70e-6},
                          {+1.00e-6, -2.30e-6},
                          {+0.00e-6, -3.30e-6},
                          {+0.00e-6, +3.30e-6},
                          {+0.00e-6, +3.23e-6},
                          {+0.00e-6, +3.20e-6},
                          {+0.00e-6, -3.20e-6},
                          {+0.00e-6, -3.20e-6},
                          {+0.00e-6, +3.20e-6},
                          {+2.90e-6, -0.30e-6},
                          {+0.08e-6, +3.05e-6},
                          {-0.70e-6, -2.40e-6},
                          {+0.00e-6, -3.08e-6},
                          {+0.00e-6, +3.00e-6},
                          {-1.60e-6, +1.40e-6},
                          {-2.90e-6, -0.10e-6},
                          {+0.00e-6, -2.90e-6},
                          {-2.50e-6, +0.40e-6},
                          {+0.40e-6, -2.50e-6},
                          {+0.00e-6, -2.90e-6},
                          {+0.00e-6, +2.89e-6},
                          {+0.00e-6, -2.80e-6},
                          {-2.50e-6, +0.30e-6},
                          {-2.50e-6, -0.30e-6},
                          {+0.00e-6, -2.70e-6},
                          {+2.70e-6, +0.00e-6},
                          {+0.00e-6, -2.60e-6},
                          {+0.00e-6, -2.60e-6},
                          {+0.00e-6, +2.60e-6},
                          {+2.10e-6, +0.50e-6},
                          {+0.00e-6, +2.50e-6},
                          {+0.80e-6, +1.70e-6},
                          {+1.90e-6, -0.60e-6},
                          {+0.00e-6, -2.50e-6},
                          {+0.00e-6, -2.40e-6},
                          {+0.00e-6, +2.40e-6},
                          {+0.00e-6, -2.40e-6},
                          {+0.00e-6, +2.40e-6},
                          {-1.90e-6, +0.50e-6},
                          {-0.10e-6, -2.30e-6},
                          {+0.00e-6, +2.30e-6},
                          {+0.00e-6, -2.30e-6},
                          {-1.40e-6, +0.90e-6},
                          {-0.10e-6, -2.20e-6},
                          {-0.20e-6, -2.00e-6},
                          {+0.00e-6, +2.20e-6},
                          {+0.00e-6, -2.20e-6},
                          {+0.00e-6, +2.20e-6},
                          {+0.00e-6, +2.20e-6},
                          {-1.80e-6, -0.40e-6},
                          {+0.00e-6, +2.20e-6},
                          {+0.00e-6, +2.20e-6},
                          {-1.70e-6, +0.40e-6},
                          {-0.80e-6, -1.30e-6},
                          {-1.30e-6, -0.80e-6},
                          {+0.00e-6, +2.10e-6},
                          {+0.00e-6, +2.10e-6},
                          {+0.00e-6, -2.10e-6},
                          {+0.00e-6, -2.10e-6},
                          {+0.00e-6, +2.10e-6},
                          {+0.00e-6, -2.00e-6},
                          {+0.00e-6, +2.00e-6},
                          {+0.00e-6, +2.00e-6},
                          {+0.00e-6, +2.00e-6},
                          {+0.00e-6, -2.00e-6},
                          {+2.00e-6, +0.00e-6},
                          {+1.10e-6, -0.90e-6},
                          {+1.60e-6, -0.40e-6},
                          {+0.00e-6, -1.91e-6},
                          {+0.00e-6, -1.90e-6},
                          {+0.00e-6, +1.90e-6},
                          {+0.00e-6, -1.90e-6},
                          {+0.00e-6, +1.90e-6},
                          {+1.50e-6, +0.40e-6},
                          {-1.50e-6, -0.40e-6},
                          {-1.40e-6, -0.50e-6},
                          {-1.00e-6, +0.90e-6},
                          {+0.00e-6, -1.90e-6},
                          {-0.30e-6, +1.60e-6},
                          {+0.00e-6, +1.90e-6},
                          {+0.00e-6, +1.90e-6},
                          {+0.00e-6, -1.80e-6},
                          {+0.00e-6, -1.80e-6},
                          {-1.10e-6, +0.70e-6},
                          {+0.20e-6, -1.60e-6},
                          {+0.00e-6, +1.80e-6},
                          {+0.00e-6, -1.71e-6},
                          {-1.20e-6, -0.50e-6},
                          {+1.50e-6, +0.20e-6},
                          {-0.60e-6, -1.10e-6},
                          {+0.60e-6, +1.10e-6},
                          {-0.60e-6, -1.10e-6},
                          {-1.10e-6, +0.60e-6},
                          {-1.70e-6, +0.00e-6},
                          {+0.00e-6, +1.60e-6},
                          {+0.00e-6, -1.60e-6},
                          {+0.00e-6, -1.60e-6},
                          {+1.20e-6, -0.40e-6},
                          {-0.50e-6, -1.10e-6},
                          {+0.60e-6, +1.00e-6},
                          {-1.30e-6, -0.30e-6},
                          {+0.30e-6, -1.30e-6},
                          {+0.00e-6, +1.60e-6},
                          {+0.00e-6, -1.60e-6},
                          {+0.00e-6, -1.60e-6},
                          {+1.10e-6, -0.50e-6},
                          {+0.00e-6, -1.50e-6},
                          {+0.00e-6, -1.50e-6},
                          {+0.00e-6, +1.50e-6},
                          {+0.00e-6, -1.50e-6},
                          {+0.00e-6, -1.50e-6},
                          {+1.50e-6, +0.00e-6},
                          {+0.00e-6, -1.50e-6},
                          {+1.30e-6, -0.20e-6},
                          {+0.00e-6, -1.50e-6},
                          {-1.20e-6, -0.30e-6},
                          {-1.40e-6, +0.10e-6},
                          {-0.50e-6, +1.00e-6},
                          {-0.50e-6, +1.00e-6},
                          {+0.20e-6, -1.30e-6},
                          {+0.00e-6, +1.50e-6},
                          {+0.00e-6, +1.50e-6},
                          /* 401-600 */
                          {+0.00e-6, +1.50e-6},
                          {+0.00e-6, +1.49e-6},
                          {+0.00e-6, -1.41e-6},
                          {+0.00e-6, +1.41e-6},
                          {+0.00e-6, -1.40e-6},
                          {+0.00e-6, -1.40e-6},
                          {+0.00e-6, +1.40e-6},
                          {+0.00e-6, -1.40e-6},
                          {+1.10e-6, -0.30e-6},
                          {+0.00e-6, -1.40e-6},
                          {+0.00e-6, +1.40e-6},
                          {+1.40e-6, +0.00e-6},
                          {-0.30e-6, +1.10e-6},
                          {+0.20e-6, +1.20e-6},
                          {-1.30e-6, +0.00e-6},
                          {+0.00e-6, -1.30e-6},
                          {+0.00e-6, +1.30e-6},
                          {-0.70e-6, -0.60e-6},
                          {-0.80e-6, +0.50e-6},
                          {-0.20e-6, -1.10e-6},
                          {+1.10e-6, +0.20e-6},
                          {+0.00e-6, -1.30e-6},
                          {+0.00e-6, -1.30e-6},
                          {+0.00e-6, -1.30e-6},
                          {+0.00e-6, -1.30e-6},
                          {+0.00e-6, -1.29e-6},
                          {+0.00e-6, +1.20e-6},
                          {+0.00e-6, -1.20e-6},
                          {-0.40e-6, -0.80e-6},
                          {+0.00e-6, +1.20e-6},
                          {+1.20e-6, +0.00e-6},
                          {-0.70e-6, -0.50e-6},
                          {-1.00e-6, +0.20e-6},
                          {-1.00e-6, +0.20e-6},
                          {+0.20e-6, -1.00e-6},
                          {+0.40e-6, +0.80e-6},
                          {-0.40e-6, +0.80e-6},
                          {+0.00e-6, -1.20e-6},
                          {+0.00e-6, +1.15e-6},
                          {+0.00e-6, +1.10e-6},
                          {-0.20e-6, +0.90e-6},
                          {-1.10e-6, +0.00e-6},
                          {+0.00e-6, -1.10e-6},
                          {-1.10e-6, +0.00e-6},
                          {+0.00e-6, +1.10e-6},
                          {+0.00e-6, +1.10e-6},
                          {+0.00e-6, +1.10e-6},
                          {+0.60e-6, -0.50e-6},
                          {-0.90e-6, -0.20e-6},
                          {-0.40e-6, -0.70e-6},
                          {-0.50e-6, +0.60e-6},
                          {+0.00e-6, +1.10e-6},
                          {+0.00e-6, -1.10e-6},
                          {+0.00e-6, +1.00e-6},
                          {+1.00e-6, +0.00e-6},
                          {+0.80e-6, -0.20e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, -1.00e-6},
                          {-1.00e-6, +0.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+1.00e-6, +0.00e-6},
                          {+1.00e-6, +0.00e-6},
                          {-0.80e-6, -0.20e-6},
                          {+0.40e-6, +0.60e-6},
                          {-0.40e-6, -0.60e-6},
                          {+0.00e-6, -1.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, +1.00e-6},
                          {+0.00e-6, -1.00e-6},
                          {+0.00e-6, +0.91e-6},
                          {+0.10e-6, +0.80e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, -0.90e-6},
                          {+0.00e-6, -0.90e-6},
                          {-0.70e-6, -0.20e-6},
                          {+0.70e-6, -0.20e-6},
                          {-0.30e-6, +0.60e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, -0.90e-6},
                          {-0.50e-6, -0.40e-6},
                          {-0.90e-6, +0.00e-6},
                          {+0.00e-6, -0.90e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, +0.90e-6},
                          {+0.00e-6, -0.90e-6},
                          {+0.00e-6, -0.90e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.00e-6, +0.80e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.10e-6, +0.70e-6},
                          {-0.70e-6, +0.10e-6},
                          {-0.60e-6, +0.20e-6},
                          {+0.20e-6, +0.60e-6},
                          {+0.00e-6, +0.80e-6},
                          {-0.50e-6, +0.30e-6},
                          {-0.50e-6, -0.30e-6},
                          {-0.50e-6, -0.30e-6},
                          {+0.00e-6, -0.80e-6},
                          {-0.30e-6, +0.50e-6},
                          {-0.80e-6, +0.00e-6},
                          {-0.30e-6, -0.50e-6},
                          {-0.30e-6, +0.50e-6},
                          {-0.30e-6, -0.50e-6},
                          {+0.00e-6, +0.80e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.00e-6, +0.80e-6},
                          {+0.00e-6, +0.80e-6},
                          {+0.00e-6, -0.80e-6},
                          {+0.00e-6, +0.76e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.10e-6, -0.60e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.70e-6, +0.00e-6},
                          {+0.00e-6, -0.70e-6},
                          {+0.00e-6, -0.70e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.00e-6, -0.70e-6},
                          {-0.70e-6, +0.00e-6},
                          {-0.50e-6, +0.20e-6},
                          {-0.20e-6, -0.50e-6},
                          {+0.50e-6, -0.20e-6},
                          {+0.20e-6, +0.50e-6},
                          {-0.20e-6, -0.50e-6},
                          {+0.50e-6, -0.20e-6},
                          {-0.50e-6, +0.20e-6},
                          {+0.00e-6, -0.70e-6},
                          {+0.00e-6, -0.70e-6},
                          {+0.70e-6, +0.00e-6},
                          {-0.60e-6, -0.10e-6},
                          {+0.60e-6, -0.10e-6},
                          {+0.40e-6, +0.30e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.70e-6, +0.00e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.00e-6, +0.70e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.10e-6, -0.50e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.40e-6, +0.20e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.50e-6, +0.10e-6},
                          {-0.50e-6, -0.10e-6},
                          {-0.10e-6, -0.50e-6},
                          {+0.10e-6, +0.50e-6},
                          {+0.50e-6, -0.10e-6},
                          {-0.10e-6, +0.50e-6},
                          {+0.00e-6, -0.60e-6},
                          {-0.40e-6, +0.20e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.60e-6, +0.00e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.20e-6, +0.40e-6},
                          {-0.40e-6, +0.20e-6},
                          {+0.30e-6, +0.30e-6},
                          {+0.40e-6, -0.20e-6},
                          {-0.40e-6, -0.20e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.40e-6, +0.20e-6},
                          {-0.20e-6, -0.40e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, +0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, -0.60e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.10e-6, +0.40e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.10e-6, +0.40e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.30e-6, -0.20e-6},
                          {-0.20e-6, +0.30e-6},
                          {+0.20e-6, +0.30e-6},
                          {+0.40e-6, -0.10e-6},
                          {+0.40e-6, +0.10e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.30e-6, +0.20e-6},
                          {-0.30e-6, +0.20e-6},
                          {+0.20e-6, +0.30e-6},
                          {-0.30e-6, +0.20e-6},
                          {+0.00e-6, +0.50e-6},
                          /* 601-800 */
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {-0.50e-6, +0.00e-6},
                          {+0.50e-6, +0.00e-6},
                          {+0.00e-6, -0.50e-6},
                          {-0.50e-6, +0.00e-6},
                          {-0.50e-6, +0.00e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.40e-6, +0.10e-6},
                          {-0.40e-6, -0.10e-6},
                          {+0.40e-6, -0.10e-6},
                          {-0.40e-6, +0.10e-6},
                          {+0.10e-6, +0.40e-6},
                          {+0.10e-6, +0.40e-6},
                          {-0.50e-6, +0.00e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.50e-6, +0.00e-6},
                          {+0.00e-6, -0.50e-6},
                          {+0.00e-6, +0.50e-6},
                          {+0.00e-6, -0.40e-6},
                          {-0.20e-6, +0.20e-6},
                          {-0.10e-6, +0.30e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.40e-6, +0.00e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {-0.10e-6, +0.30e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {-0.20e-6, -0.20e-6},
                          {+0.20e-6, -0.20e-6},
                          {+0.20e-6, +0.20e-6},
                          {-0.10e-6, +0.30e-6},
                          {-0.30e-6, +0.10e-6},
                          {+0.10e-6, +0.30e-6},
                          {-0.10e-6, +0.30e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.40e-6, +0.00e-6},
                          {-0.40e-6, +0.00e-6},
                          {-0.40e-6, +0.00e-6},
                          {-0.40e-6, +0.00e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {-0.40e-6, +0.00e-6},
                          {+0.40e-6, +0.00e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.40e-6, +0.00e-6},
                          {+0.00e-6, -0.40e-6},
                          {-0.40e-6, +0.00e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.40e-6, +0.00e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {-0.10e-6, +0.30e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.00e-6, -0.40e-6},
                          {+0.40e-6, +0.00e-6},
                          {+0.00e-6, +0.40e-6},
                          {+0.21e-6, +0.10e-6},
                          {+0.00e-6, +0.30e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.20e-6, +0.10e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {-0.20e-6, +0.10e-6},
                          {-0.10e-6, -0.20e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.10e-6, -0.20e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {-0.20e-6, +0.10e-6},
                          {+0.00e-6, -0.30e-6},
                          {-0.10e-6, -0.20e-6},
                          {-0.10e-6, +0.20e-6},
                          {+0.20e-6, -0.10e-6},
                          {-0.10e-6, -0.20e-6},
                          {+0.20e-6, +0.10e-6},
                          {+0.20e-6, -0.10e-6},
                          {-0.20e-6, -0.10e-6},
                          {-0.10e-6, -0.20e-6},
                          {+0.20e-6, -0.10e-6},
                          {+0.20e-6, +0.10e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, +0.30e-6},
                          {-0.30e-6, +0.00e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.00e-6, +0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {-0.20e-6, -0.10e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.30e-6},
                          {+0.00e-6, -0.21e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          /* 801-962 */
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.10e-6, -0.10e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.10e-6, -0.10e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {-0.20e-6, +0.00e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, +0.20e-6},
                          {+0.00e-6, -0.19e-6},
                          {+0.00e-6, +0.17e-6},
                          {+0.00e-6, +0.11e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {-0.10e-6, +0.00e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, -0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, +0.10e-6}};
    double SY1[277][2] = {/* 1-200 */
                          {+153041.82e-6, +878.89e-6},
                          {+11714.49e-6, -289.32e-6},
                          {+2024.68e-6, -50.99e-6},
                          {-1837.33e-6, +47.75e-6},
                          {-1312.21e-6, -28.91e-6},
                          {-632.54e-6, +0.78e-6},
                          {+459.68e-6, -67.23e-6},
                          {+344.50e-6, +1.46e-6},
                          {+268.14e-6, -7.03e-6},
                          {+192.06e-6, +29.80e-6},
                          {+139.64e-6, +0.15e-6},
                          {-113.94e-6, -1.06e-6},
                          {+109.81e-6, +3.18e-6},
                          {-56.37e-6, +0.13e-6},
                          {-56.17e-6, -0.02e-6},
                          {-53.05e-6, -1.23e-6},
                          {-51.60e-6, +0.17e-6},
                          {+45.91e-6, -0.11e-6},
                          {-42.45e-6, +0.02e-6},
                          {+40.82e-6, -1.03e-6},
                          {+34.30e-6, -1.24e-6},
                          {+28.89e-6, +0.00e-6},
                          {+27.61e-6, -1.22e-6},
                          {-25.43e-6, +1.00e-6},
                          {-26.01e-6, +0.07e-6},
                          {-23.02e-6, +0.06e-6},
                          {+19.37e-6, -0.01e-6},
                          {+14.05e-6, -4.19e-6},
                          {+18.18e-6, -0.01e-6},
                          {-14.86e-6, -0.09e-6},
                          {+13.49e-6, -0.01e-6},
                          {+12.44e-6, -0.27e-6},
                          {+11.46e-6, +0.03e-6},
                          {-11.33e-6, -0.06e-6},
                          {-9.81e-6, +0.01e-6},
                          {-9.08e-6, -0.02e-6},
                          {+2.74e-6, -4.56e-6},
                          {+6.84e-6, -0.04e-6},
                          {-6.73e-6, +0.01e-6},
                          {+6.54e-6, +0.01e-6},
                          {-6.35e-6, -0.01e-6},
                          {+5.90e-6, -0.02e-6},
                          {-5.85e-6, +0.02e-6},
                          {-5.73e-6, +0.01e-6},
                          {+5.60e-6, +0.00e-6},
                          {-5.16e-6, +0.00e-6},
                          {-5.14e-6, +0.01e-6},
                          {+4.76e-6, -0.02e-6},
                          {-4.40e-6, +0.02e-6},
                          {-4.22e-6, +0.00e-6},
                          {-4.20e-6, +0.01e-6},
                          {+3.58e-6, +0.31e-6},
                          {+3.87e-6, +0.01e-6},
                          {+3.76e-6, +0.00e-6},
                          {-3.62e-6, -0.01e-6},
                          {-3.61e-6, +0.00e-6},
                          {-1.28e-6, -2.14e-6},
                          {-3.18e-6, +0.00e-6},
                          {+3.01e-6, +0.00e-6},
                          {-2.97e-6, +0.01e-6},
                          {+2.91e-6, +0.00e-6},
                          {-2.73e-6, +0.00e-6},
                          {+2.58e-6, -0.01e-6},
                          {+2.56e-6, -0.01e-6},
                          {-2.51e-6, -0.01e-6},
                          {-2.35e-6, -0.01e-6},
                          {-2.21e-6, +0.01e-6},
                          {-2.04e-6, +0.01e-6},
                          {-1.94e-6, +0.00e-6},
                          {+0.41e-6, -1.43e-6},
                          {-1.84e-6, +0.00e-6},
                          {-1.77e-6, +0.01e-6},
                          {+0.00e-6, +1.77e-6},
                          {+1.76e-6, +0.00e-6},
                          {-1.07e-6, -0.53e-6},
                          {-1.48e-6, +0.00e-6},
                          {-1.40e-6, +0.01e-6},
                          {-1.35e-6, -0.01e-6},
                          {-1.32e-6, +0.00e-6},
                          {-1.28e-6, +0.00e-6},
                          {+1.24e-6, +0.00e-6},
                          {+1.23e-6, +0.00e-6},
                          {+1.19e-6, +0.00e-6},
                          {+1.18e-6, -0.01e-6},
                          {+1.17e-6, +0.00e-6},
                          {-1.15e-6, +0.00e-6},
                          {+1.14e-6, +0.00e-6},
                          {-1.14e-6, +0.00e-6},
                          {+1.09e-6, +0.03e-6},
                          {-1.08e-6, +0.00e-6},
                          {+1.04e-6, +0.00e-6},
                          {+1.02e-6, +0.00e-6},
                          {+0.98e-6, -0.01e-6},
                          {+0.91e-6, +0.02e-6},
                          {+0.00e-6, +0.93e-6},
                          {-0.91e-6, +0.00e-6},
                          {-0.90e-6, +0.00e-6},
                          {+0.86e-6, +0.00e-6},
                          {-0.84e-6, +0.00e-6},
                          {-0.83e-6, +0.00e-6},
                          {-0.82e-6, +0.00e-6},
                          {+0.41e-6, +0.39e-6},
                          {+0.40e-6, -0.38e-6},
                          {+0.78e-6, +0.00e-6},
                          {+0.74e-6, +0.00e-6},
                          {-0.73e-6, +0.00e-6},
                          {+0.68e-6, +0.00e-6},
                          {+0.66e-6, +0.00e-6},
                          {-0.64e-6, +0.00e-6},
                          {-0.63e-6, +0.00e-6},
                          {+0.63e-6, +0.00e-6},
                          {+0.62e-6, +0.00e-6},
                          {+0.60e-6, +0.00e-6},
                          {-0.59e-6, +0.00e-6},
                          {-0.59e-6, +0.00e-6},
                          {+0.59e-6, +0.00e-6},
                          {+0.57e-6, +0.00e-6},
                          {+0.38e-6, -0.19e-6},
                          {-0.01e-6, -0.55e-6},
                          {+0.44e-6, -0.11e-6},
                          {+0.53e-6, +0.00e-6},
                          {-0.53e-6, +0.00e-6},
                          {+0.52e-6, +0.00e-6},
                          {-0.52e-6, +0.00e-6},
                          {+0.53e-6, +0.00e-6},
                          {+0.52e-6, +0.00e-6},
                          {+0.51e-6, +0.00e-6},
                          {+0.51e-6, +0.00e-6},
                          {-0.21e-6, -0.30e-6},
                          {-0.50e-6, +0.00e-6},
                          {-0.11e-6, +0.37e-6},
                          {-0.11e-6, +0.37e-6},
                          {-0.48e-6, +0.00e-6},
                          {-0.46e-6, -0.01e-6},
                          {-0.47e-6, +0.00e-6},
                          {-0.03e-6, +0.43e-6},
                          {+0.45e-6, +0.00e-6},
                          {-0.44e-6, +0.00e-6},
                          {-0.44e-6, +0.00e-6},
                          {-0.44e-6, +0.00e-6},
                          {+0.43e-6, +0.00e-6},
                          {+0.44e-6, +0.00e-6},
                          {+0.42e-6, +0.00e-6},
                          {-0.42e-6, +0.00e-6},
                          {+0.41e-6, +0.00e-6},
                          {-0.41e-6, +0.00e-6},
                          {+0.02e-6, +0.39e-6},
                          {+0.40e-6, +0.00e-6},
                          {-0.40e-6, +0.00e-6},
                          {-0.39e-6, +0.00e-6},
                          {+0.39e-6, +0.00e-6},
                          {+0.15e-6, -0.24e-6},
                          {-0.37e-6, -0.01e-6},
                          {+0.37e-6, +0.00e-6},
                          {-0.37e-6, +0.00e-6},
                          {-0.37e-6, +0.00e-6},
                          {-0.31e-6, +0.06e-6},
                          {-0.35e-6, +0.00e-6},
                          {+0.35e-6, +0.00e-6},
                          {-0.07e-6, -0.27e-6},
                          {-0.33e-6, +0.01e-6},
                          {-0.33e-6, +0.00e-6},
                          {+0.07e-6, -0.26e-6},
                          {+0.33e-6, +0.00e-6},
                          {+0.00e-6, -0.32e-6},
                          {+0.32e-6, +0.00e-6},
                          {-0.32e-6, +0.00e-6},
                          {+0.32e-6, +0.00e-6},
                          {-0.24e-6, -0.07e-6},
                          {+0.24e-6, +0.07e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.08e-6, -0.22e-6},
                          {-0.30e-6, +0.00e-6},
                          {-0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.30e-6, +0.00e-6},
                          {+0.00e-6, -0.29e-6},
                          {+0.00e-6, -0.29e-6},
                          {+0.20e-6, -0.09e-6},
                          {+0.29e-6, +0.00e-6},
                          {-0.05e-6, -0.24e-6},
                          {+0.29e-6, +0.00e-6},
                          {-0.27e-6, +0.00e-6},
                          {-0.19e-6, -0.08e-6},
                          {-0.27e-6, +0.00e-6},
                          {+0.25e-6, +0.00e-6},
                          {+0.25e-6, +0.00e-6},
                          {-0.25e-6, +0.00e-6},
                          {+0.25e-6, +0.00e-6},
                          {-0.25e-6, +0.00e-6},
                          {-0.01e-6, +0.23e-6},
                          {-0.23e-6, +0.00e-6},
                          {+0.23e-6, +0.00e-6},
                          {+0.23e-6, +0.00e-6},
                          {-0.15e-6, -0.07e-6},
                          {-0.23e-6, +0.00e-6},
                          {-0.22e-6, +0.00e-6},
                          {+0.22e-6, +0.00e-6},
                          {-0.22e-6, +0.00e-6},
                          {-0.22e-6, +0.00e-6},
                          /* 201-277 */
                          {+0.04e-6, -0.17e-6},
                          {-0.01e-6, -0.21e-6},
                          {+0.08e-6, -0.14e-6},
                          {-0.01e-6, +0.19e-6},
                          {+0.21e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.20e-6, +0.00e-6},
                          {-0.04e-6, -0.16e-6},
                          {+0.19e-6, +0.00e-6},
                          {+0.19e-6, +0.00e-6},
                          {-0.19e-6, +0.00e-6},
                          {+0.18e-6, +0.00e-6},
                          {-0.18e-6, +0.00e-6},
                          {+0.18e-6, +0.00e-6},
                          {+0.17e-6, +0.00e-6},
                          {-0.12e-6, +0.06e-6},
                          {+0.13e-6, -0.04e-6},
                          {-0.11e-6, +0.06e-6},
                          {+0.17e-6, +0.00e-6},
                          {+0.16e-6, +0.00e-6},
                          {-0.17e-6, +0.00e-6},
                          {-0.17e-6, +0.00e-6},
                          {-0.14e-6, +0.02e-6},
                          {+0.14e-6, +0.03e-6},
                          {+0.00e-6, +0.15e-6},
                          {-0.15e-6, +0.00e-6},
                          {-0.14e-6, +0.01e-6},
                          {+0.16e-6, +0.00e-6},
                          {-0.06e-6, +0.10e-6},
                          {+0.05e-6, +0.10e-6},
                          {+0.02e-6, +0.13e-6},
                          {-0.11e-6, +0.04e-6},
                          {-0.12e-6, -0.02e-6},
                          {-0.05e-6, -0.10e-6},
                          {+0.14e-6, +0.00e-6},
                          {-0.09e-6, +0.05e-6},
                          {+0.00e-6, +0.14e-6},
                          {+0.14e-6, +0.00e-6},
                          {-0.14e-6, +0.00e-6},
                          {+0.04e-6, +0.10e-6},
                          {-0.06e-6, +0.08e-6},
                          {+0.05e-6, +0.09e-6},
                          {-0.14e-6, +0.00e-6},
                          {+0.08e-6, +0.06e-6},
                          {+0.14e-6, +0.00e-6},
                          {+0.14e-6, +0.00e-6},
                          {+0.13e-6, +0.00e-6},
                          {-0.07e-6, +0.06e-6},
                          {+0.11e-6, -0.02e-6},
                          {-0.13e-6, +0.00e-6},
                          {-0.13e-6, +0.00e-6},
                          {-0.13e-6, +0.00e-6},
                          {-0.13e-6, +0.00e-6},
                          {-0.12e-6, +0.00e-6},
                          {+0.12e-6, +0.00e-6},
                          {+0.12e-6, +0.00e-6},
                          {-0.12e-6, +0.00e-6},
                          {+0.00e-6, -0.12e-6},
                          {-0.02e-6, -0.09e-6},
                          {+0.02e-6, -0.09e-6},
                          {-0.11e-6, +0.00e-6},
                          {+0.11e-6, +0.00e-6},
                          {+0.07e-6, -0.04e-6},
                          {+0.11e-6, +0.00e-6},
                          {+0.11e-6, +0.00e-6},
                          {-0.11e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {-0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {+0.00e-6, +0.10e-6},
                          {+0.00e-6, +0.10e-6},
                          {-0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6},
                          {-0.10e-6, +0.00e-6},
                          {+0.10e-6, +0.00e-6}};
    double SY2[30][2] = {
        {+121.15e-6, -2301.27e-6}, {-0.98e-6, -143.27e-6}, {-0.27e-6, -24.46e-6}, {+0.24e-6, +22.41e-6},
        {-1.19e-6, -5.61e-6},      {+3.57e-6, -1.83e-6},   {+0.24e-6, -5.02e-6},  {-0.04e-6, -3.23e-6},
        {-0.48e-6, +2.40e-6},      {-0.10e-6, +1.73e-6},   {-0.01e-6, +1.33e-6},  {-0.04e-6, +0.83e-6},
        {-0.05e-6, -0.79e-6},      {+0.03e-6, -0.66e-6},   {+0.00e-6, -0.64e-6},  {+0.04e-6, +0.61e-6},
        {-0.01e-6, -0.41e-6},      {-0.01e-6, +0.35e-6},   {-0.01e-6, -0.33e-6},  {+0.01e-6, +0.31e-6},
        {+0.01e-6, +0.27e-6},      {-0.07e-6, -0.17e-6},   {+0.07e-6, +0.17e-6},  {+0.02e-6, -0.21e-6},
        {+0.01e-6, +0.20e-6},      {+0.01e-6, -0.17e-6},   {+0.01e-6, -0.16e-6},  {+0.00e-6, -0.13e-6},
        {-0.07e-6, -0.04e-6},      {+0.02e-6, +0.08e-6}};
    double SY3[5][2] = {
        {-15.23e-6, -1.62e-6}, {-1.16e-6, -0.01e-6}, {-0.20e-6, +0.00e-6}, {+0.18e-6, +0.00e-6}, {+0.13e-6, +0.00e-6}};
    double SY4[1][2] = {{-0.01e-6, +0.11e-6}};

    /*Interval between fundamental epoch J2000.0 and current date(JC). */
    double t = ((date1 - dJ0) + date2) / dJC;

    /*Fundamental Arguments */
    double FA[14];
    /* Mean anomaly of the Moon. */
    FA[0] = _iauFal03(t);
    /* Mean anomaly of the Sun. */
    FA[1] = _iauFalp03(t);
    /* Mean argument of the latitude of the Moon. */
    FA[2] = _iauFaf03(t);
    /* Mean elongation of the Moon from the Sun. */
    FA[3] = _iauFad03(t);
    /* Mean longitude of the ascending node of the Moon. */
    FA[4] = _iauFaom03(t);
    /* Planetary longitudes, Mercury through Neptune. */
    FA[5] = _iauFame03(t);
    FA[6] = _iauFave03(t);
    FA[7] = _iauFae03(t);
    FA[8] = _iauFama03(t);
    FA[9] = _iauFaju03(t);
    FA[10] = _iauFasa03(t);
    FA[11] = _iauFaur03(t);
    FA[12] = _iauFane03(t);
    /* General accumulated precession in longitude. */
    FA[13] = _iauFapa03(t);

    double S0 = XP[0];
    double S1 = XP[1];
    double S2 = XP[2];
    double S3 = XP[3];
    double S4 = XP[4];
    double S5 = XP[5];
    int I, J;
    double A;
    for (I = NX0 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KX0[I][J] * FA[J];
        }
        S0 += (SX0[I][0] * sin(A) + SX0[I][1] * cos(A));
    }
    for (I = NX1 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KX1[I][J] * FA[J];
        }
        S1 += (SX1[I][0] * sin(A) + SX1[I][1] * cos(A));
    }
    for (I = NX2 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KX2[I][J] * FA[J];
        }
        S2 += (SX2[I][0] * sin(A) + SX2[I][1] * cos(A));
    }
    for (I = NX3 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KX3[I][J] * FA[J];
        }
        S3 += (SX3[I][0] * sin(A) + SX3[I][1] * cos(A));
    }
    for (I = NX4 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KX4[I][J] * FA[J];
        }
        S4 += (SX4[I][0] * sin(A) + SX4[I][1] * cos(A));
    }
    double tmp_x = (S0 + (S1 + (S2 + (S3 + (S4 + S5 * t) * t) * t) * t) * t) * dAS2R;

    S0 = YP[0];
    S1 = YP[1];
    S2 = YP[2];
    S3 = YP[3];
    S4 = YP[4];
    S5 = YP[5];
    for (I = NY0 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KY0[I][J] * FA[J];
        }
        S0 += (SY0[I][0] * sin(A) + SY0[I][1] * cos(A));
    }
    for (I = NY1 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KY1[I][J] * FA[J];
        }
        S1 += (SY1[I][0] * sin(A) + SY1[I][1] * cos(A));
    }
    for (I = NY2 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KY2[I][J] * FA[J];
        }
        S2 += (SY2[I][0] * sin(A) + SY2[I][1] * cos(A));
    }
    for (I = NY3 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KY3[I][J] * FA[J];
        }
        S3 += (SY3[I][0] * sin(A) + SY3[I][1] * cos(A));
    }
    for (I = NY4 - 1; I >= 0; I--)
    {
        A = 0.0;
        for (J = 0; J <= 13; J++)
        {
            A = A + KY4[I][J] * FA[J];
        }
        S4 += (SY4[I][0] * sin(A) + SY4[I][1] * cos(A));
    }
    double tmp_y = (S0 + (S1 + (S2 + (S3 + (S4 + S5 * t) * t) * t) * t) * t) * dAS2R;

    *x = tmp_x;
    *y = tmp_y;
}

/** @brief calculate X/Y coordinates of celestial intermediate pole            */
/** from series based on IAU 2006 precession and IAU 2000A nutation.           */
/** Created: yqyuan 2021.08.20 from SOFA package                               */
/**  Tested: yqyuan 2021.XX.XX                                                 */
void t_gtrs2crs::_iau_XY06(double date1, double date2, double *x, double *y)
{
    /* Maximum power of T in the polynomials for X and Y */
    int MAXPT = 5;
    /* Numbers of frequencies : luni - solar, planetary, total */
    int NFLS, NFPL;
    NFLS = 653;
    NFPL = 656;
    /* Number of amplitude coefficients */
    int NA = 4755;

    /* Polynomial coefficients (arcsec, X then Y). */
    double xyp[2][6] = {{-0.016617, 2004.191898, -0.4297829, -0.19861834, 0.000007578, 0.0000059285},
                        {-0.006951, -0.025896, -22.4072747, 0.00190059, 0.001112526, 0.0000001358}};
    /* Fundamental-argument multipliers:  luni-solar terms */
    int mfals[653][5] = {/* 1-200 */
                         {0, 0, 0, 0, 1},
                         {0, 0, 2, -2, 2},
                         {0, 0, 2, 0, 2},
                         {0, 0, 0, 0, 2},
                         {0, 1, 0, 0, 0},
                         {0, 1, 2, -2, 2},
                         {1, 0, 0, 0, 0},
                         {0, 0, 2, 0, 1},
                         {1, 0, 2, 0, 2},
                         {0, 1, -2, 2, -2},
                         {0, 0, 2, -2, 1},
                         {1, 0, -2, 0, -2},
                         {1, 0, 0, -2, 0},
                         {1, 0, 0, 0, 1},
                         {1, 0, 0, 0, -1},
                         {1, 0, -2, -2, -2},
                         {1, 0, 2, 0, 1},
                         {2, 0, -2, 0, -1},
                         {0, 0, 0, 2, 0},
                         {0, 0, 2, 2, 2},
                         {2, 0, 0, -2, 0},
                         {0, 2, -2, 2, -2},
                         {2, 0, 2, 0, 2},
                         {1, 0, 2, -2, 2},
                         {1, 0, -2, 0, -1},
                         {2, 0, 0, 0, 0},
                         {0, 0, 2, 0, 0},
                         {0, 1, 0, 0, 1},
                         {1, 0, 0, -2, -1},
                         {0, 2, 2, -2, 2},
                         {0, 0, 2, -2, 0},
                         {1, 0, 0, -2, 1},
                         {0, 1, 0, 0, -1},
                         {0, 2, 0, 0, 0},
                         {1, 0, -2, -2, -1},
                         {1, 0, 2, 2, 2},
                         {0, 1, 2, 0, 2},
                         {2, 0, -2, 0, 0},
                         {0, 0, 2, 2, 1},
                         {0, 1, -2, 0, -2},
                         {0, 0, 0, 2, 1},
                         {1, 0, 2, -2, 1},
                         {2, 0, 0, -2, -1},
                         {2, 0, 2, -2, 2},
                         {2, 0, 2, 0, 1},
                         {0, 0, 0, 2, -1},
                         {0, 1, -2, 2, -1},
                         {1, 1, 0, -2, 0},
                         {2, 0, 0, -2, 1},
                         {1, 0, 0, 2, 0},
                         {0, 1, 2, -2, 1},
                         {1, -1, 0, 0, 0},
                         {0, 1, -1, 1, -1},
                         {2, 0, -2, 0, -2},
                         {0, 1, 0, -2, 0},
                         {1, 0, 0, -1, 0},
                         {3, 0, 2, 0, 2},
                         {0, 0, 0, 1, 0},
                         {1, -1, 2, 0, 2},
                         {1, 1, -2, -2, -2},
                         {1, 0, -2, 0, 0},
                         {2, 0, 0, 0, -1},
                         {0, 1, -2, -2, -2},
                         {1, 1, 2, 0, 2},
                         {2, 0, 0, 0, 1},
                         {1, 1, 0, 0, 0},
                         {1, 0, -2, 2, -1},
                         {1, 0, 2, 0, 0},
                         {1, -1, 0, -1, 0},
                         {1, 0, 0, 0, 2},
                         {1, 0, -1, 0, -1},
                         {0, 0, 2, 1, 2},
                         {1, 0, -2, -4, -2},
                         {1, -1, 0, -1, -1},
                         {1, 0, 2, 2, 1},
                         {0, 2, -2, 2, -1},
                         {1, 0, 0, 0, -2},
                         {2, 0, -2, -2, -2},
                         {1, 1, 2, -2, 2},
                         {2, 0, -2, -4, -2},
                         {1, 0, -4, 0, -2},
                         {2, 0, 2, -2, 1},
                         {1, 0, 0, -1, -1},
                         {2, 0, 2, 2, 2},
                         {3, 0, 0, 0, 0},
                         {1, 0, 0, 2, 1},
                         {0, 0, 2, -2, -1},
                         {3, 0, 2, -2, 2},
                         {0, 0, 4, -2, 2},
                         {1, 0, 0, -4, 0},
                         {0, 1, 2, 0, 1},
                         {2, 0, 0, -4, 0},
                         {1, 1, 0, -2, -1},
                         {2, 0, -2, 0, 1},
                         {0, 0, 2, 0, -1},
                         {0, 1, -2, 0, -1},
                         {0, 1, 0, 0, 2},
                         {0, 0, 2, -1, 2},
                         {0, 0, 2, 4, 2},
                         {2, 1, 0, -2, 0},
                         {1, 1, 0, -2, 1},
                         {1, -1, 0, -2, 0},
                         {1, -1, 0, -1, -2},
                         {1, -1, 0, 0, 1},
                         {0, 1, -2, 2, 0},
                         {0, 1, 0, 0, -2},
                         {1, -1, 2, 2, 2},
                         {1, 0, 0, 2, -1},
                         {1, -1, -2, -2, -2},
                         {3, 0, 2, 0, 1},
                         {0, 1, 2, 2, 2},
                         {1, 0, 2, -2, 0},
                         {1, 1, -2, -2, -1},
                         {1, 0, 2, -4, 1},
                         {0, 1, -2, -2, -1},
                         {2, -1, 2, 0, 2},
                         {0, 0, 0, 2, 2},
                         {1, -1, 2, 0, 1},
                         {1, -1, -2, 0, -2},
                         {0, 1, 0, 2, 0},
                         {0, 1, 2, -2, 0},
                         {0, 0, 0, 1, 1},
                         {1, 0, -2, -2, 0},
                         {0, 3, 2, -2, 2},
                         {2, 1, 2, 0, 2},
                         {1, 1, 0, 0, 1},
                         {2, 0, 0, 2, 0},
                         {1, 1, 2, 0, 1},
                         {1, 0, 0, -2, -2},
                         {1, 0, -2, 2, 0},
                         {1, 0, -1, 0, -2},
                         {0, 1, 0, -2, 1},
                         {0, 1, 0, 1, 0},
                         {0, 0, 0, 1, -1},
                         {1, 0, -2, 2, -2},
                         {1, -1, 0, 0, -1},
                         {0, 0, 0, 4, 0},
                         {1, -1, 0, 2, 0},
                         {1, 0, 2, 1, 2},
                         {1, 0, 2, -1, 2},
                         {0, 0, 2, 1, 1},
                         {1, 0, 0, -2, 2},
                         {1, 0, -2, 0, 1},
                         {1, 0, -2, -4, -1},
                         {0, 0, 2, 2, 0},
                         {1, 1, 2, -2, 1},
                         {1, 0, -2, 1, -1},
                         {0, 0, 1, 0, 1},
                         {2, 0, -2, -2, -1},
                         {4, 0, 2, 0, 2},
                         {2, -1, 0, 0, 0},
                         {2, 1, 2, -2, 2},
                         {0, 1, 2, 1, 2},
                         {1, 0, 4, -2, 2},
                         {1, 1, 0, 0, -1},
                         {2, 0, 2, 0, 0},
                         {2, 0, -2, -4, -1},
                         {1, 0, -1, 0, 0},
                         {1, 0, 0, 1, 0},
                         {0, 1, 0, 2, 1},
                         {1, 0, -4, 0, -1},
                         {1, 0, 0, -4, -1},
                         {2, 0, 2, 2, 1},
                         {2, 1, 0, 0, 0},
                         {0, 0, 2, -3, 2},
                         {1, 2, 0, -2, 0},
                         {0, 3, 0, 0, 0},
                         {0, 0, 4, 0, 2},
                         {0, 0, 2, -4, 1},
                         {2, 0, 0, -2, -2},
                         {1, 1, -2, -4, -2},
                         {0, 1, 0, -2, -1},
                         {0, 0, 0, 4, 1},
                         {3, 0, 2, -2, 1},
                         {1, 0, 2, 4, 2},
                         {1, 1, -2, 0, -2},
                         {0, 0, 4, -2, 1},
                         {2, -2, 0, -2, 0},
                         {2, 1, 0, -2, -1},
                         {0, 2, 0, -2, 0},
                         {1, 0, 0, -1, 1},
                         {1, 1, 2, 2, 2},
                         {3, 0, 0, 0, -1},
                         {2, 0, 0, -4, -1},
                         {3, 0, 2, 2, 2},
                         {0, 0, 2, 4, 1},
                         {0, 2, -2, -2, -2},
                         {1, -1, 0, -2, -1},
                         {0, 0, 2, -1, 1},
                         {2, 0, 0, 2, 1},
                         {1, -1, -2, 2, -1},
                         {0, 0, 0, 2, -2},
                         {2, 0, 0, -4, 1},
                         {1, 0, 0, -4, 1},
                         {2, 0, 2, -4, 1},
                         {4, 0, 2, -2, 2},
                         {2, 1, -2, 0, -1},
                         {2, 1, -2, -4, -2},
                         {3, 0, 0, -4, 0},
                         {1, -1, 2, 2, 1},
                         /* 201-400 */
                         {1, -1, -2, 0, -1},
                         {0, 2, 0, 0, 1},
                         {1, 2, -2, -2, -2},
                         {1, 1, 0, -4, 0},
                         {2, 0, 0, -2, 2},
                         {0, 2, 2, -2, 1},
                         {1, 0, 2, 0, -1},
                         {2, 1, 0, -2, 1},
                         {2, -1, -2, 0, -1},
                         {1, -1, -2, -2, -1},
                         {0, 1, -2, 1, -2},
                         {1, 0, -4, 2, -2},
                         {0, 1, 2, 2, 1},
                         {3, 0, 0, 0, 1},
                         {2, -1, 2, 2, 2},
                         {0, 1, -2, -4, -2},
                         {1, 0, -2, -3, -2},
                         {2, 0, 0, 0, 2},
                         {1, -1, 0, -2, -2},
                         {2, 0, -2, 2, -1},
                         {0, 2, -2, 0, -2},
                         {3, 0, -2, 0, -1},
                         {2, -1, 2, 0, 1},
                         {1, 0, -2, -1, -2},
                         {0, 0, 2, 0, 3},
                         {2, 0, -4, 0, -2},
                         {2, 1, 0, -4, 0},
                         {1, 1, -2, 1, -1},
                         {0, 2, 2, 0, 2},
                         {1, -1, 2, -2, 2},
                         {1, -1, 0, -2, 1},
                         {2, 1, 2, 0, 1},
                         {1, 0, 2, -4, 2},
                         {1, 1, -2, 0, -1},
                         {1, 1, 0, 2, 0},
                         {1, 0, 0, -3, 0},
                         {2, 0, 2, -1, 2},
                         {0, 2, 0, 0, -1},
                         {2, -1, 0, -2, 0},
                         {4, 0, 0, 0, 0},
                         {2, 1, -2, -2, -2},
                         {0, 2, -2, 2, 0},
                         {1, 0, 2, 1, 1},
                         {1, 0, -1, 0, -3},
                         {3, -1, 2, 0, 2},
                         {2, 0, 2, -2, 0},
                         {1, -2, 0, 0, 0},
                         {2, 0, 0, 0, -2},
                         {1, 0, 0, 4, 0},
                         {0, 1, 0, 1, 1},
                         {1, 0, 2, 2, 0},
                         {0, 1, 0, 2, -1},
                         {0, 1, 0, 1, -1},
                         {0, 0, 2, -2, 3},
                         {3, 1, 2, 0, 2},
                         {1, 1, 2, 1, 2},
                         {1, 1, -2, 2, -1},
                         {2, -1, 2, -2, 2},
                         {1, -2, 2, 0, 2},
                         {1, 0, 2, -4, 0},
                         {0, 0, 1, 0, 0},
                         {1, 0, 2, -3, 1},
                         {1, -2, 0, -2, 0},
                         {2, 0, 0, 2, -1},
                         {1, 1, 2, -4, 1},
                         {4, 0, 2, 0, 1},
                         {0, 1, 2, 1, 1},
                         {1, 2, 2, -2, 2},
                         {2, 0, 2, 1, 2},
                         {2, 1, 2, -2, 1},
                         {1, 0, 2, -1, 1},
                         {1, 0, 4, -2, 1},
                         {1, -1, 2, -2, 1},
                         {0, 1, 0, -4, 0},
                         {3, 0, -2, -2, -2},
                         {0, 0, 4, -4, 2},
                         {2, 0, -4, -2, -2},
                         {2, -2, 0, -2, -1},
                         {1, 0, 2, -2, -1},
                         {2, 0, -2, -6, -2},
                         {1, 0, -2, 1, -2},
                         {1, 0, -2, 2, 1},
                         {1, -1, 0, 2, -1},
                         {1, 0, -2, 1, 0},
                         {2, -1, 0, -2, 1},
                         {1, -1, 0, 2, 1},
                         {2, 0, -2, -2, 0},
                         {1, 0, 2, -3, 2},
                         {0, 0, 0, 4, -1},
                         {2, -1, 0, 0, 1},
                         {2, 0, 4, -2, 2},
                         {0, 0, 2, 3, 2},
                         {0, 1, 4, -2, 2},
                         {0, 1, -2, 2, 1},
                         {1, 1, 0, 2, 1},
                         {1, 0, 0, 4, 1},
                         {0, 0, 4, 0, 1},
                         {2, 0, 0, -3, 0},
                         {1, 0, 0, -1, -2},
                         {1, -2, -2, -2, -2},
                         {3, 0, 0, 2, 0},
                         {2, 0, 2, -4, 2},
                         {1, 1, -2, -4, -1},
                         {1, 0, -2, -6, -2},
                         {2, -1, 0, 0, -1},
                         {2, -1, 0, 2, 0},
                         {0, 1, 2, -2, -1},
                         {1, 1, 0, 1, 0},
                         {1, 2, 0, -2, -1},
                         {1, 0, 0, 1, -1},
                         {0, 0, 1, 0, 2},
                         {3, 1, 2, -2, 2},
                         {1, 0, -4, -2, -2},
                         {1, 0, 2, 4, 1},
                         {1, -2, 2, 2, 2},
                         {1, -1, -2, -4, -2},
                         {0, 0, 2, -4, 2},
                         {0, 0, 2, -3, 1},
                         {2, 1, -2, 0, 0},
                         {3, 0, -2, -2, -1},
                         {2, 0, 2, 4, 2},
                         {0, 0, 0, 0, 3},
                         {2, -1, -2, -2, -2},
                         {2, 0, 0, -1, 0},
                         {3, 0, 2, -4, 2},
                         {2, 1, 2, 2, 2},
                         {0, 0, 3, 0, 3},
                         {1, 1, 2, 2, 1},
                         {2, 1, 0, 0, -1},
                         {1, 2, 0, -2, 1},
                         {3, 0, 2, 2, 1},
                         {1, -1, -2, 2, -2},
                         {1, 1, 0, -1, 0},
                         {1, 2, 0, 0, 0},
                         {1, 0, 4, 0, 2},
                         {1, -1, 2, 4, 2},
                         {2, 1, 0, 0, 1},
                         {1, 0, 0, 2, 2},
                         {1, -1, -2, 2, 0},
                         {0, 2, -2, -2, -1},
                         {2, 0, -2, 0, 2},
                         {5, 0, 2, 0, 2},
                         {3, 0, -2, -6, -2},
                         {1, -1, 2, -1, 2},
                         {3, 0, 0, -4, -1},
                         {1, 0, 0, 1, 1},
                         {1, 0, -4, 2, -1},
                         {0, 1, 2, -4, 1},
                         {1, 2, 2, 0, 2},
                         {0, 1, 0, -2, -2},
                         {0, 0, 2, -1, 0},
                         {1, 0, 1, 0, 1},
                         {0, 2, 0, -2, 1},
                         {3, 0, 2, 0, 0},
                         {1, 1, -2, 1, 0},
                         {2, 1, -2, -4, -1},
                         {3, -1, 0, 0, 0},
                         {2, -1, -2, 0, 0},
                         {4, 0, 2, -2, 1},
                         {2, 0, -2, 2, 0},
                         {1, 1, 2, -2, 0},
                         {1, 0, -2, 4, -1},
                         {1, 0, -2, -2, 1},
                         {2, 0, 2, -4, 0},
                         {1, 1, 0, -2, -2},
                         {1, 1, -2, -2, 0},
                         {1, 0, 1, -2, 1},
                         {2, -1, -2, -4, -2},
                         {3, 0, -2, 0, -2},
                         {0, 1, -2, -2, 0},
                         {3, 0, 0, -2, -1},
                         {1, 0, -2, -3, -1},
                         {0, 1, 0, -4, -1},
                         {1, -2, 2, -2, 1},
                         {0, 1, -2, 1, -1},
                         {1, -1, 0, 0, 2},
                         {2, 0, 0, 1, 0},
                         {1, -2, 0, 2, 0},
                         {1, 2, -2, -2, -1},
                         {0, 0, 4, -4, 1},
                         {0, 1, 2, 4, 2},
                         {0, 1, -4, 2, -2},
                         {3, 0, -2, 0, 0},
                         {2, -1, 2, 2, 1},
                         {0, 1, -2, -4, -1},
                         {4, 0, 2, 2, 2},
                         {2, 0, -2, -3, -2},
                         {2, 0, 0, -6, 0},
                         {1, 0, 2, 0, 3},
                         {3, 1, 0, 0, 0},
                         {3, 0, 0, -4, 1},
                         {1, -1, 2, 0, 0},
                         {1, -1, 0, -4, 0},
                         {2, 0, -2, 2, -2},
                         {1, 1, 0, -2, 2},
                         {4, 0, 0, -2, 0},
                         {2, 2, 0, -2, 0},
                         {0, 1, 2, 0, 0},
                         {1, 1, 0, -4, 1},
                         {1, 0, 0, -4, -2},
                         /* 401-600 */
                         {0, 0, 0, 1, 2},
                         {3, 0, 0, 2, 1},
                         {1, 1, 0, -4, -1},
                         {0, 0, 2, 2, -1},
                         {1, 1, 2, 0, 0},
                         {1, -1, 2, -4, 1},
                         {1, 1, 0, 0, 2},
                         {0, 0, 2, 6, 2},
                         {4, 0, -2, -2, -1},
                         {2, 1, 0, -4, -1},
                         {0, 0, 0, 3, 1},
                         {1, -1, -2, 0, 0},
                         {0, 0, 2, 1, 0},
                         {1, 0, 0, 2, -2},
                         {3, -1, 2, 2, 2},
                         {3, -1, 2, -2, 2},
                         {1, 0, 0, -1, 2},
                         {1, -2, 2, -2, 2},
                         {0, 1, 0, 2, 2},
                         {0, 1, -2, -1, -2},
                         {1, 1, -2, 0, 0},
                         {0, 2, 2, -2, 0},
                         {3, -1, -2, -1, -2},
                         {1, 0, 0, -6, 0},
                         {1, 0, -2, -4, 0},
                         {2, 1, 0, -4, 1},
                         {2, 0, 2, 0, -1},
                         {2, 0, -4, 0, -1},
                         {0, 0, 3, 0, 2},
                         {2, 1, -2, -2, -1},
                         {1, -2, 0, 0, 1},
                         {2, -1, 0, -4, 0},
                         {0, 0, 0, 3, 0},
                         {5, 0, 2, -2, 2},
                         {1, 2, -2, -4, -2},
                         {1, 0, 4, -4, 2},
                         {0, 0, 4, -1, 2},
                         {3, 1, 0, -4, 0},
                         {3, 0, 0, -6, 0},
                         {2, 0, 0, 2, 2},
                         {2, -2, 2, 0, 2},
                         {1, 0, 0, -3, 1},
                         {1, -2, -2, 0, -2},
                         {1, -1, -2, -3, -2},
                         {0, 0, 2, -2, -2},
                         {2, 0, -2, -4, 0},
                         {1, 0, -4, 0, 0},
                         {0, 1, 0, -1, 0},
                         {4, 0, 0, 0, -1},
                         {3, 0, 2, -1, 2},
                         {3, -1, 2, 0, 1},
                         {2, 0, 2, -1, 1},
                         {1, 2, 2, -2, 1},
                         {1, 1, 0, 2, -1},
                         {0, 2, 2, 0, 1},
                         {3, 1, 2, 0, 1},
                         {1, 1, 2, 1, 1},
                         {1, 1, 0, -1, 1},
                         {1, -2, 0, -2, -1},
                         {4, 0, 0, -4, 0},
                         {2, 1, 0, 2, 0},
                         {1, -1, 0, 4, 0},
                         {0, 1, 0, -2, 2},
                         {0, 0, 2, 0, -2},
                         {1, 0, -1, 0, 1},
                         {3, 0, 2, -2, 0},
                         {2, 0, 2, 2, 0},
                         {1, 2, 0, -4, 0},
                         {1, -1, 0, -3, 0},
                         {0, 1, 0, 4, 0},
                         {0, 1, -2, 0, 0},
                         {2, 2, 2, -2, 2},
                         {0, 0, 0, 1, -2},
                         {0, 2, -2, 0, -1},
                         {4, 0, 2, -4, 2},
                         {2, 0, -4, 2, -2},
                         {2, -1, -2, 0, -2},
                         {1, 1, 4, -2, 2},
                         {1, 1, 2, -4, 2},
                         {1, 0, 2, 3, 2},
                         {1, 0, 0, 4, -1},
                         {0, 0, 0, 4, 2},
                         {2, 0, 0, 4, 0},
                         {1, 1, -2, 2, 0},
                         {2, 1, 2, 1, 2},
                         {2, 1, 2, -4, 1},
                         {2, 0, 2, 1, 1},
                         {2, 0, -4, -2, -1},
                         {2, 0, -2, -6, -1},
                         {2, -1, 2, -1, 2},
                         {1, -2, 2, 0, 1},
                         {1, -2, 0, -2, 1},
                         {1, -1, 0, -4, -1},
                         {0, 2, 2, 2, 2},
                         {0, 2, -2, -4, -2},
                         {0, 1, 2, 3, 2},
                         {0, 1, 0, -4, 1},
                         {3, 0, 0, -2, 1},
                         {2, 1, -2, 0, 1},
                         {2, 0, 4, -2, 1},
                         {2, 0, 0, -3, -1},
                         {2, -2, 0, -2, 1},
                         {2, -1, 2, -2, 1},
                         {1, 0, 0, -6, -1},
                         {1, -2, 0, 0, -1},
                         {1, -2, -2, -2, -1},
                         {0, 1, 4, -2, 1},
                         {0, 0, 2, 3, 1},
                         {2, -1, 0, -1, 0},
                         {1, 3, 0, -2, 0},
                         {0, 3, 0, -2, 0},
                         {2, -2, 2, -2, 2},
                         {0, 0, 4, -2, 0},
                         {4, -1, 2, 0, 2},
                         {2, 2, -2, -4, -2},
                         {4, 1, 2, 0, 2},
                         {4, -1, -2, -2, -2},
                         {2, 1, 0, -2, -2},
                         {2, 1, -2, -6, -2},
                         {2, 0, 0, -1, 1},
                         {2, -1, -2, 2, -1},
                         {1, 1, -2, 2, -2},
                         {1, 1, -2, -3, -2},
                         {1, 0, 3, 0, 3},
                         {1, 0, -2, 1, 1},
                         {1, 0, -2, 0, 2},
                         {1, -1, 2, 1, 2},
                         {1, -1, 0, 0, -2},
                         {1, -1, -4, 2, -2},
                         {0, 3, -2, -2, -2},
                         {0, 1, 0, 4, 1},
                         {0, 0, 4, 2, 2},
                         {3, 0, -2, -2, 0},
                         {2, -2, 0, 0, 0},
                         {1, 1, 2, -4, 0},
                         {1, 1, 0, -3, 0},
                         {1, 0, 2, -3, 0},
                         {1, -1, 2, -2, 0},
                         {0, 2, 0, 2, 0},
                         {0, 0, 2, 4, 0},
                         {1, 0, 1, 0, 0},
                         {3, 1, 2, -2, 1},
                         {3, 0, 4, -2, 2},
                         {3, 0, 2, 1, 2},
                         {3, 0, 0, 2, -1},
                         {3, 0, 0, 0, 2},
                         {3, 0, -2, 2, -1},
                         {2, 0, 4, -4, 2},
                         {2, 0, 2, -3, 2},
                         {2, 0, 0, 4, 1},
                         {2, 0, 0, -3, 1},
                         {2, 0, -4, 2, -1},
                         {2, 0, -2, -2, 1},
                         {2, -2, 2, 2, 2},
                         {2, -2, 0, -2, -2},
                         {2, -1, 0, 2, 1},
                         {2, -1, 0, 2, -1},
                         {1, 1, 2, 4, 2},
                         {1, 1, 0, 1, 1},
                         {1, 1, 0, 1, -1},
                         {1, 1, -2, -6, -2},
                         {1, 0, 0, -3, -1},
                         {1, 0, -4, -2, -1},
                         {1, 0, -2, -6, -1},
                         {1, -2, 2, 2, 1},
                         {1, -2, -2, 2, -1},
                         {1, -1, -2, -4, -1},
                         {0, 2, 0, 0, 2},
                         {0, 1, 2, -4, 2},
                         {0, 1, -2, 4, -1},
                         {5, 0, 0, 0, 0},
                         {3, 0, 0, -3, 0},
                         {2, 2, 0, -4, 0},
                         {1, -1, 2, 2, 0},
                         {0, 1, 0, 3, 0},
                         {4, 0, -2, 0, -1},
                         {3, 0, -2, -6, -1},
                         {3, 0, -2, -1, -1},
                         {2, 1, 2, 2, 1},
                         {2, 1, 0, 2, 1},
                         {2, 0, 2, 4, 1},
                         {2, 0, 2, -6, 1},
                         {2, 0, 2, -2, -1},
                         {2, 0, 0, -6, -1},
                         {2, -1, -2, -2, -1},
                         {1, 2, 2, 0, 1},
                         {1, 2, 0, 0, 1},
                         {1, 0, 4, 0, 1},
                         {1, 0, 2, -6, 1},
                         {1, 0, 2, -4, -1},
                         {1, 0, -1, -2, -1},
                         {1, -1, 2, 4, 1},
                         {1, -1, 2, -3, 1},
                         {1, -1, 0, 4, 1},
                         {1, -1, -2, 1, -1},
                         {0, 1, 2, -2, 3},
                         {3, 0, 0, -2, 0},
                         {1, 0, 1, -2, 0},
                         {0, 2, 0, -4, 0},
                         {0, 0, 2, -4, 0},
                         /* 601-653 */
                         {0, 0, 1, -1, 0},
                         {0, 0, 0, 6, 0},
                         {0, 2, 0, 0, -2},
                         {0, 1, -2, 2, -3},
                         {4, 0, 0, 2, 0},
                         {3, 0, 0, -1, 0},
                         {3, -1, 0, 2, 0},
                         {2, 1, 0, 1, 0},
                         {2, 1, 0, -6, 0},
                         {2, -1, 2, 0, 0},
                         {1, 0, 2, -1, 0},
                         {1, -1, 0, 1, 0},
                         {1, -1, -2, -2, 0},
                         {0, 1, 2, 2, 0},
                         {0, 0, 2, -3, 0},
                         {2, 2, 0, -2, -1},
                         {2, -1, -2, 0, 1},
                         {1, 2, 2, -4, 1},
                         {0, 1, 4, -4, 2},
                         {0, 0, 0, 3, 2},
                         {5, 0, 2, 0, 1},
                         {4, 1, 2, -2, 2},
                         {4, 0, -2, -2, 0},
                         {3, 1, 2, 2, 2},
                         {3, 1, 0, -2, 0},
                         {3, 1, -2, -6, -2},
                         {3, 0, 0, 0, -2},
                         {3, 0, -2, -4, -2},
                         {3, -1, 0, -3, 0},
                         {3, -1, 0, -2, 0},
                         {2, 1, 2, 0, 0},
                         {2, 1, 2, -4, 2},
                         {2, 1, 2, -2, 0},
                         {2, 1, 0, -3, 0},
                         {2, 1, -2, 0, -2},
                         {2, 0, 0, -4, 2},
                         {2, 0, 0, -4, -2},
                         {2, 0, -2, -5, -2},
                         {2, -1, 2, 4, 2},
                         {2, -1, 0, -2, 2},
                         {1, 3, -2, -2, -2},
                         {1, 1, 0, 0, -2},
                         {1, 1, 0, -6, 0},
                         {1, 1, -2, 1, -2},
                         {1, 1, -2, -1, -2},
                         {1, 0, 2, 1, 0},
                         {1, 0, 0, 3, 0},
                         {1, 0, 0, -4, 2},
                         {1, 0, -2, 4, -2},
                         {1, -2, 0, -1, 0},
                         {0, 1, -4, 2, -1},
                         {1, 0, -2, 0, -3},
                         {0, 0, 4, -4, 4}};
    /* Fundamental-argument multipliers:  planetary terms */
    int mfapl[656][14] = {/* 1-200 */
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, -8, 3, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},
                          {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
                          {1, 0, 0, 0, 0, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},
                          {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},
                          {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
                          {1, 0, -2, 0, -2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},
                          {0, 0, 1, -1, 1, 0, 0, -5, 8, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, -1},
                          {2, 0, 0, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 8, -13, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, -2, 5, 0, 0, 0},
                          {1, 0, 0, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2},
                          {1, 0, 0, 0, -1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, -5, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 19, -21, 3, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -8, 13, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2},
                          {1, 0, 0, 0, 1, 0, -18, 16, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 6, -16, 4, 5, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
                          {2, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 1, 0, 0, 1, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2},
                          {0, 0, 2, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -2},
                          {1, 0, 0, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, -3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 1, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -4, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, -1, 0, 0, 0, 2},
                          {1, 0, 0, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -4, 10, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, -2},
                          {0, 0, 2, -2, 1, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -1, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 2},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 2, 0},
                          {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -9, 13, 0, 0, 0, 0, 0},
                          {2, 0, 2, 0, 2, 0, 0, 2, 0, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                          {1, 0, 0, -1, -1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},
                          {1, 0, 2, 0, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {1, 0, -2, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -2, 4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
                          {0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -8, 3, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 7, -8, 3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -2},
                          {1, 0, 0, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, -5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 0, -9, 15, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0},
                          /* 201-400 */
                          {0, 0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -1, 0, 0, 2},
                          {2, 0, 0, -2, 1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 8, -14, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 2},
                          {2, 0, -1, -1, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -9, 4, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                          {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 3, -5, 4, 0, 0, 0, 0, 2},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, -1, 1, 0, 0, 0},
                          {2, 0, 0, 0, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 2, -4, 0, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 3, -5, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 2},
                          {0, 0, 2, -2, 2, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -2, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, -5, 0, 0, 2},
                          {2, 0, 0, -2, -1, 0, 0, -2, 0, 0, 5, 0, 0, 0},
                          {2, 0, 0, -2, -1, 0, -6, 8, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, 2, -5, 0, 0, 2},
                          {0, 0, 0, 0, 1, 0, 3, -7, 4, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -15, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 2},
                          {1, 0, 0, -1, 0, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -3, 7, -4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, 0, -2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 1},
                          {0, 0, 2, -2, 2, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 9, -12, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 1},
                          {0, 0, 1, -1, 0, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, -1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -5, 0, 0, 0, 0, -2},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 3, -1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 3, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 2, -4, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2},
                          {0, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, 0, -3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
                          {2, 0, -1, -1, -1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 1},
                          {0, 0, 2, -2, 1, -1, 0, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0},
                          {0, 0, 0, 0, 1, 0, 3, -5, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -8, 11, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 2, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 2, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 5, -2, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 2},
                          {0, 0, 2, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 8, -10, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, 0},
                          {2, 0, 0, -2, -1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, -2},
                          {2, 0, -1, -1, -1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 4, -3, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -5, 0, 0, 0, 0, 2},
                          {1, 0, -2, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, -2, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -2, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 1, -6, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2},
                          {0, 0, 0, 0, 0, 0, 3, -5, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 2},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, 2, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -8, 15, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, -2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {2, 0, -1, -1, -1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {1, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {1, 0, -1, 1, -1, 0, -18, 17, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 2, -2, -1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -16, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2},
                          {0, 0, 0, 0, 2, 0, 0, -1, 2, 0, 0, 0, 0, 0},
                          {2, 0, -1, -1, -2, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 4, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2},
                          {2, 0, 0, -2, -1, 0, 0, -2, 0, 4, -5, 0, 0, 0},
                          /* 401-600 */
                          {2, 0, 0, -2, -1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {2, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -1, -1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {1, 0, -1, -1, -1, 0, 20, -20, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 1, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 5, -8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 9, -11, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1},
                          {0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -2},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -2, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, -2, 5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, -6, 0, 0, 0, 0, -2},
                          {1, 0, 0, -2, 0, 0, 20, -21, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -12, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -12, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, -7, 0, 0, 0, 0, -2},
                          {1, 0, 0, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                          {1, 0, -2, 0, -2, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -9, 17, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -2},
                          {1, 0, -2, -2, -2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {1, 0, -1, 1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -10, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, -4, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, -5, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -5, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 1},
                          {1, 0, 0, -2, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -7, 4, 0, 0, 0, 0, 0},
                          {2, 0, 2, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 1, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, -2},
                          {1, 0, 0, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -3, 0, 3, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, -5, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 1, -3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -4, 6, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, -1, 0, 0},
                          {0, 0, 1, -1, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 3, -4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -10, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 3, -8, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, -1},
                          {2, 0, 0, -2, -1, 0, 0, -6, 8, 0, 0, 0, 0, 0},
                          {2, 0, -1, -1, 1, 0, 0, 3, -7, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -7, 9, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, -1},
                          {0, 0, 1, -1, 2, 0, -8, 12, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 1, 0, 0, -5, 6, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, -1, 0, 0, -2, 0, 3, -1, 0, 0, 0},
                          {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {1, 0, 0, -2, -1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {1, 0, 0, -1, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
                          {1, 0, -1, 0, -1, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -4, 4, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, -8, 11, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, 0, 0, -9, 13, 0, 0, 0, 0, 0},
                          {0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, 1, -4, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, -3, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 7, -13, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 7, -11, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 1, -4, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 17, -16, 0, -2, 0, 0, 0, 0},
                          {1, 0, 0, -1, 0, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -4, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 2},
                          {2, 0, 0, -2, 0, 0, 0, -4, 4, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 2, 2, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -2, 2, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, -4, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 2, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 8, -9, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0},
                          {2, 0, -2, -2, -2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
                          {1, 0, 0, 0, 1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, -1, 0, -10, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 2, -3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, 2, -2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, -2, 3, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, 0, 2, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},
                          {2, 0, 2, -2, 2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {2, 0, 1, -3, 1, 0, -6, 7, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 2, -5, 0, 0, 0, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 5, -5, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 1, 5, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 0, 5, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, 0, -2, 0, 0, 2, 0, 0, 0},
                          {2, 0, 0, -2, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0},
                          {2, 0, -2, 0, -2, 0, 0, 5, -9, 0, 0, 0, 0, 0},
                          {2, 0, -1, -1, 0, 0, 0, -1, 0, 3, 0, 0, 0, 0},
                          {1, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
                          {1, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
                          {1, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
                          {1, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {1, 0, 0, 0, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0},
                          {1, 0, 0, -2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                          {1, 0, -2, -2, -2, 0, 0, 1, 0, -1, 0, 0, 0, 0},
                          {1, 0, -1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {1, 0, -1, -1, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},
                          {0, 0, 2, 2, 2, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -2, 0, 1, 0, 0, 0, 0},
                          {0, 0, 2, -2, 1, 0, 0, -10, 15, 0, 0, 0, 0, 0},
                          {0, 0, 2, -2, 0, -1, 0, 2, 0, 0, 0, 0, 0, 0},
                          /* 601-656 */
                          {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, -1, 0, 0, 0},
                          {0, 0, 1, -1, 2, 0, -3, 4, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -4, 6, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, 0, -1, 0, 0, -2, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
                          {0, 0, 1, -1, -1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
                          {0, 0, 0, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 2, 0, -3, 5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 9, -13, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 8, -14, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 8, -11, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 5, -6, -4, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 3, -3, 0, 2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 7, -12, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -8, 1, 5, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 6, -10, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, 0, -4, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, -1},
                          {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 5, -16, 4, 5, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 5, -13, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, 0, -5, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -9, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 3, -7, 0, 0, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -3, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 2, -8, 1, 5, 0, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, -5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -3, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -6, 3, 0, -2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
                          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
                          {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}};
    /* Pointers into amplitudes array, one pointer per frequency */
    int nc[1309] = {
        /* 1-200 */
        1, 21, 37, 51, 65, 79, 91, 103, 115, 127, 139, 151, 163, 172, 184, 196, 207, 219, 231, 240, 252, 261, 273, 285,
        297, 309, 318, 327, 339, 351, 363, 372, 384, 396, 405, 415, 423, 435, 444, 452, 460, 467, 474, 482, 490, 498,
        506, 513, 521, 528, 536, 543, 551, 559, 566, 574, 582, 590, 597, 605, 613, 620, 628, 636, 644, 651, 658, 666,
        674, 680, 687, 695, 702, 710, 717, 725, 732, 739, 746, 753, 760, 767, 774, 782, 790, 798, 805, 812, 819, 826,
        833, 840, 846, 853, 860, 867, 874, 881, 888, 895, 901, 908, 914, 921, 928, 934, 941, 948, 955, 962, 969, 976,
        982, 989, 996, 1003, 1010, 1017, 1024, 1031, 1037, 1043, 1050, 1057, 1064, 1071, 1078, 1084, 1091, 1098, 1104,
        1112, 1118, 1124, 1131, 1138, 1145, 1151, 1157, 1164, 1171, 1178, 1185, 1192, 1199, 1205, 1212, 1218, 1226,
        1232, 1239, 1245, 1252, 1259, 1266, 1272, 1278, 1284, 1292, 1298, 1304, 1310, 1316, 1323, 1329, 1335, 1341,
        1347, 1353, 1359, 1365, 1371, 1377, 1383, 1389, 1396, 1402, 1408, 1414, 1420, 1426, 1434, 1440, 1446, 1452,
        1459, 1465, 1471, 1477, 1482, 1488, 1493, 1499, 1504, 1509, 1514, 1520, 1527, 1532, 1538,
        /* 201-400 */
        1543, 1548, 1553, 1558, 1564, 1569, 1574, 1579, 1584, 1589, 1594, 1596, 1598, 1600, 1602, 1605, 1608, 1610,
        1612, 1617, 1619, 1623, 1625, 1627, 1629, 1632, 1634, 1640, 1642, 1644, 1646, 1648, 1650, 1652, 1654, 1658,
        1660, 1662, 1664, 1668, 1670, 1672, 1673, 1675, 1679, 1681, 1683, 1684, 1686, 1688, 1690, 1693, 1695, 1697,
        1701, 1703, 1705, 1707, 1709, 1711, 1712, 1715, 1717, 1721, 1723, 1725, 1727, 1729, 1731, 1733, 1735, 1737,
        1739, 1741, 1743, 1745, 1747, 1749, 1751, 1753, 1755, 1757, 1759, 1761, 1762, 1764, 1766, 1768, 1769, 1771,
        1773, 1775, 1777, 1779, 1781, 1783, 1785, 1787, 1788, 1790, 1792, 1794, 1796, 1798, 1800, 1802, 1804, 1806,
        1807, 1809, 1811, 1815, 1817, 1819, 1821, 1823, 1825, 1827, 1829, 1831, 1833, 1835, 1837, 1839, 1840, 1842,
        1844, 1848, 1850, 1852, 1854, 1856, 1858, 1859, 1860, 1862, 1864, 1866, 1868, 1869, 1871, 1873, 1875, 1877,
        1879, 1881, 1883, 1885, 1887, 1889, 1891, 1892, 1896, 1898, 1900, 1901, 1903, 1905, 1907, 1909, 1910, 1911,
        1913, 1915, 1919, 1921, 1923, 1927, 1929, 1931, 1933, 1935, 1937, 1939, 1943, 1945, 1947, 1948, 1949, 1951,
        1953, 1955, 1957, 1958, 1960, 1962, 1964, 1966, 1968, 1970, 1971, 1973, 1974, 1975, 1977, 1979, 1980, 1981,
        1982, 1984,
        /* 401-600 */
        1986, 1988, 1990, 1992, 1994, 1995, 1997, 1999, 2001, 2003, 2005, 2007, 2008, 2009, 2011, 2013, 2015, 2017,
        2019, 2021, 2023, 2024, 2025, 2027, 2029, 2031, 2033, 2035, 2037, 2041, 2043, 2045, 2046, 2047, 2049, 2051,
        2053, 2055, 2056, 2057, 2059, 2061, 2063, 2065, 2067, 2069, 2070, 2071, 2072, 2074, 2076, 2078, 2080, 2082,
        2084, 2086, 2088, 2090, 2092, 2094, 2095, 2096, 2097, 2099, 2101, 2105, 2106, 2107, 2108, 2109, 2110, 2111,
        2113, 2115, 2119, 2121, 2123, 2125, 2127, 2129, 2131, 2133, 2135, 2136, 2137, 2139, 2141, 2143, 2145, 2147,
        2149, 2151, 2153, 2155, 2157, 2159, 2161, 2163, 2165, 2167, 2169, 2171, 2173, 2175, 2177, 2179, 2181, 2183,
        2185, 2186, 2187, 2188, 2192, 2193, 2195, 2197, 2199, 2201, 2203, 2205, 2207, 2209, 2211, 2213, 2217, 2219,
        2221, 2223, 2225, 2227, 2229, 2231, 2233, 2234, 2235, 2236, 2237, 2238, 2239, 2240, 2241, 2244, 2246, 2248,
        2250, 2252, 2254, 2256, 2258, 2260, 2262, 2264, 2266, 2268, 2270, 2272, 2274, 2276, 2278, 2280, 2282, 2284,
        2286, 2288, 2290, 2292, 2294, 2296, 2298, 2300, 2302, 2303, 2304, 2305, 2306, 2307, 2309, 2311, 2313, 2315,
        2317, 2319, 2321, 2323, 2325, 2327, 2329, 2331, 2333, 2335, 2337, 2341, 2343, 2345, 2347, 2349, 2351, 2352,
        2355, 2356,
        /* 601-800 */
        2357, 2358, 2359, 2361, 2363, 2364, 2365, 2366, 2367, 2368, 2369, 2370, 2371, 2372, 2373, 2374, 2376, 2378,
        2380, 2382, 2384, 2385, 2386, 2387, 2388, 2389, 2390, 2391, 2392, 2393, 2394, 2395, 2396, 2397, 2398, 2399,
        2400, 2401, 2402, 2403, 2404, 2405, 2406, 2407, 2408, 2409, 2410, 2411, 2412, 2413, 2414, 2415, 2417, 2418,
        2430, 2438, 2445, 2453, 2460, 2468, 2474, 2480, 2488, 2496, 2504, 2512, 2520, 2527, 2535, 2543, 2550, 2558,
        2566, 2574, 2580, 2588, 2596, 2604, 2612, 2619, 2627, 2634, 2642, 2648, 2656, 2664, 2671, 2679, 2685, 2693,
        2701, 2709, 2717, 2725, 2733, 2739, 2747, 2753, 2761, 2769, 2777, 2785, 2793, 2801, 2809, 2817, 2825, 2833,
        2841, 2848, 2856, 2864, 2872, 2878, 2884, 2892, 2898, 2906, 2914, 2922, 2930, 2938, 2944, 2952, 2958, 2966,
        2974, 2982, 2988, 2996, 3001, 3009, 3017, 3025, 3032, 3039, 3045, 3052, 3059, 3067, 3069, 3076, 3083, 3090,
        3098, 3105, 3109, 3111, 3113, 3120, 3124, 3128, 3132, 3136, 3140, 3144, 3146, 3150, 3158, 3161, 3165, 3166,
        3168, 3172, 3176, 3180, 3182, 3185, 3189, 3193, 3194, 3197, 3200, 3204, 3208, 3212, 3216, 3219, 3221, 3222,
        3226, 3230, 3234, 3238, 3242, 3243, 3247, 3251, 3254, 3258, 3262, 3266, 3270, 3274, 3275, 3279, 3283, 3287,
        3289, 3293,
        /* 801-1000 */
        3296, 3300, 3303, 3307, 3311, 3315, 3319, 3321, 3324, 3327, 3330, 3334, 3338, 3340, 3342, 3346, 3350, 3354,
        3358, 3361, 3365, 3369, 3373, 3377, 3381, 3385, 3389, 3393, 3394, 3398, 3402, 3406, 3410, 3413, 3417, 3421,
        3425, 3429, 3433, 3435, 3439, 3443, 3446, 3450, 3453, 3457, 3458, 3461, 3464, 3468, 3472, 3476, 3478, 3481,
        3485, 3489, 3493, 3497, 3501, 3505, 3507, 3511, 3514, 3517, 3521, 3524, 3525, 3527, 3529, 3533, 3536, 3540,
        3541, 3545, 3548, 3551, 3555, 3559, 3563, 3567, 3569, 3570, 3574, 3576, 3578, 3582, 3586, 3590, 3593, 3596,
        3600, 3604, 3608, 3612, 3616, 3620, 3623, 3626, 3630, 3632, 3636, 3640, 3643, 3646, 3648, 3652, 3656, 3660,
        3664, 3667, 3669, 3671, 3675, 3679, 3683, 3687, 3689, 3693, 3694, 3695, 3699, 3703, 3705, 3707, 3710, 3713,
        3717, 3721, 3725, 3729, 3733, 3736, 3740, 3744, 3748, 3752, 3754, 3757, 3759, 3763, 3767, 3770, 3773, 3777,
        3779, 3783, 3786, 3790, 3794, 3798, 3801, 3805, 3809, 3813, 3817, 3821, 3825, 3827, 3831, 3835, 3836, 3837,
        3840, 3844, 3848, 3852, 3856, 3859, 3863, 3867, 3869, 3871, 3875, 3879, 3883, 3887, 3890, 3894, 3898, 3901,
        3905, 3909, 3913, 3917, 3921, 3922, 3923, 3924, 3926, 3930, 3932, 3936, 3938, 3940, 3944, 3948, 3952, 3956,
        3959, 3963,
        /* 1001-1200 */
        3965, 3969, 3973, 3977, 3979, 3981, 3982, 3986, 3989, 3993, 3997, 4001, 4004, 4006, 4009, 4012, 4016, 4020,
        4024, 4026, 4028, 4032, 4036, 4040, 4044, 4046, 4050, 4054, 4058, 4060, 4062, 4063, 4064, 4068, 4071, 4075,
        4077, 4081, 4083, 4087, 4089, 4091, 4095, 4099, 4101, 4103, 4105, 4107, 4111, 4115, 4119, 4123, 4127, 4129,
        4131, 4135, 4139, 4141, 4143, 4145, 4149, 4153, 4157, 4161, 4165, 4169, 4173, 4177, 4180, 4183, 4187, 4191,
        4195, 4198, 4201, 4205, 4209, 4212, 4213, 4216, 4217, 4221, 4223, 4226, 4230, 4234, 4236, 4240, 4244, 4248,
        4252, 4256, 4258, 4262, 4264, 4266, 4268, 4270, 4272, 4276, 4279, 4283, 4285, 4287, 4289, 4293, 4295, 4299,
        4300, 4301, 4305, 4309, 4313, 4317, 4319, 4323, 4325, 4329, 4331, 4333, 4335, 4337, 4341, 4345, 4349, 4351,
        4353, 4357, 4361, 4365, 4367, 4369, 4373, 4377, 4381, 4383, 4387, 4389, 4391, 4395, 4399, 4403, 4407, 4411,
        4413, 4414, 4415, 4418, 4419, 4421, 4423, 4427, 4429, 4431, 4433, 4435, 4437, 4439, 4443, 4446, 4450, 4452,
        4456, 4458, 4460, 4462, 4466, 4469, 4473, 4477, 4481, 4483, 4487, 4489, 4491, 4493, 4497, 4499, 4501, 4504,
        4506, 4510, 4513, 4514, 4515, 4518, 4521, 4522, 4525, 4526, 4527, 4530, 4533, 4534, 4537, 4541, 4542, 4543,
        4544, 4545,
        /* 1201-1309 */
        4546, 4547, 4550, 4553, 4554, 4555, 4558, 4561, 4564, 4567, 4568, 4571, 4574, 4575, 4578, 4581, 4582, 4585,
        4586, 4588, 4590, 4592, 4596, 4598, 4602, 4604, 4608, 4612, 4613, 4616, 4619, 4622, 4623, 4624, 4625, 4626,
        4629, 4632, 4633, 4636, 4639, 4640, 4641, 4642, 4643, 4644, 4645, 4648, 4649, 4650, 4651, 4652, 4653, 4656,
        4657, 4660, 4661, 4664, 4667, 4670, 4671, 4674, 4675, 4676, 4677, 4678, 4681, 4682, 4683, 4684, 4687, 4688,
        4689, 4692, 4693, 4696, 4697, 4700, 4701, 4702, 4703, 4704, 4707, 4708, 4711, 4712, 4715, 4716, 4717, 4718,
        4719, 4720, 4721, 4722, 4723, 4726, 4729, 4730, 4733, 4736, 4737, 4740, 4741, 4742, 4745, 4746, 4749, 4752,
        4753};
    /* Amplitude coefficients (microarcsec);  indexed using the nc array. */
    static const double a[4755] = {
        /* 1-105 */
        -6844318.44, 9205236.26, 1328.67, 1538.18, 205833.11, 153041.79, -3309.73, 853.32, 2037.98, -2301.27, 81.46,
        120.56, -20.39, -15.22, 1.73, -1.61, -0.10, 0.11, -0.02, -0.02, -523908.04, 573033.42, -544.75, -458.66,
        12814.01, 11714.49, 198.97, -290.91, 155.74, -143.27, -2.75, -1.03, -1.27, -1.16, 0.00, -0.01, -90552.22,
        97846.69, 111.23, 137.41, 2187.91, 2024.68, 41.44, -51.26, 26.92, -24.46, -0.46, -0.28, -0.22, -0.20, 82168.76,
        -89618.24, -27.64, -29.05, -2004.36, -1837.32, -36.07, 48.00, -24.43, 22.41, 0.47, 0.24, 0.20, 0.18, 58707.02,
        7387.02, 470.05, -192.40, 164.33, -1312.21, -179.73, -28.93, -17.36, -1.83, -0.50, 3.57, 0.00, 0.13, -20557.78,
        22438.42, -20.84, -17.40, 501.82, 459.68, 59.20, -67.30, 6.08, -5.61, -1.36, -1.19, 28288.28, -674.99, -34.69,
        35.80, -15.07, -632.54, -11.19, 0.78, -8.41, 0.17, 0.01, 0.07, -15406.85, 20069.50, 15.12,
        /* 106-219 */
        31.80, 448.76, 344.50, -5.77, 1.41, 4.59, -5.02, 0.17, 0.24, -11991.74, 12902.66, 32.46, 36.70, 288.49, 268.14,
        5.70, -7.06, 3.57, -3.23, -0.06, -0.04, -8584.95, -9592.72, 4.42, -13.20, -214.50, 192.06, 23.87, 29.83, 2.54,
        2.40, 0.60, -0.48, 5095.50, -6918.22, 7.19, 3.92, -154.91, -113.94, 2.86, -1.04, -1.52, 1.73, -0.07, -0.10,
        -4910.93, -5331.13, 0.76, 0.40, -119.21, 109.81, 2.16, 3.20, 1.46, 1.33, 0.04, -0.02, -6245.02, -123.48, -6.68,
        -8.20, -2.76, 139.64, 2.71, 0.15, 1.86, 2511.85, -3323.89, 1.07, -0.90, -74.33, -56.17, 1.16, -0.01, -0.75,
        0.83, -0.02, -0.04, 2307.58, 3143.98, -7.52, 7.50, 70.31, -51.60, 1.46, 0.16, -0.69, -0.79, 0.02, -0.05,
        2372.58, 2554.51, 5.93, -6.60, 57.12, -53.05, -0.96, -1.24, -0.71, -0.64, -0.01, -2053.16, 2636.13, 5.13, 7.80,
        58.94, 45.91, -0.42, -0.12, 0.61, -0.66, 0.02, 0.03, -1825.49,
        /* 220-339 */
        -2423.59, 1.23, -2.00, -54.19, 40.82, -1.07, -1.02, 0.54, 0.61, -0.04, 0.04, 2521.07, -122.28, -5.97, 2.90,
        -2.73, -56.37, -0.82, 0.13, -0.75, -1534.09, 1645.01, 6.29, 6.80, 36.78, 34.30, 0.92, -1.25, 0.46, -0.41, -0.02,
        -0.01, 1898.27, 47.70, -0.72, 2.50, 1.07, -42.45, -0.94, 0.02, -0.56, -1292.02, -1387.00, 0.00, 0.00, -31.01,
        28.89, 0.68, 0.00, 0.38, 0.35, -0.01, -0.01, -1234.96, 1323.81, 5.21, 5.90, 29.60, 27.61, 0.74, -1.22, 0.37,
        -0.33, -0.02, -0.01, 1137.48, -1233.89, -0.04, -0.30, -27.59, -25.43, -0.61, 1.00, -0.34, 0.31, 0.01, 0.01,
        -813.13, -1075.60, 0.40, 0.30, -24.05, 18.18, -0.40, -0.01, 0.24, 0.27, -0.01, 0.01, 1163.22, -60.90, -2.94,
        1.30, -1.36, -26.01, -0.58, 0.07, -0.35, 1029.70, -55.55, -2.63, 1.10, -1.25, -23.02, -0.52, 0.06, -0.31,
        -556.26, 852.85, 3.16, -4.48, 19.06, 12.44, -0.81, -0.27, 0.17, -0.21, 0.00, 0.02, -603.52,
        /* 340-659 */
        -800.34, 0.44, 0.10, -17.90, 13.49, -0.08, -0.01, 0.18, 0.20, -0.01, 0.01, -628.24, 684.99, -0.64, -0.50, 15.32,
        14.05, 3.18, -4.19, 0.19, -0.17, -0.09, -0.07, -866.48, -16.26, 0.52, -1.30, -0.36, 19.37, 0.43, -0.01, 0.26,
        -512.37, 695.54, -1.47, -1.40, 15.55, 11.46, -0.16, 0.03, 0.15, -0.17, 0.01, 0.01, 506.65, 643.75, 2.54, -2.62,
        14.40, -11.33, -0.77, -0.06, -0.15, -0.16, 0.00, 0.01, 664.57, 16.81, -0.40, 1.00, 0.38, -14.86, -3.71, -0.09,
        -0.20, 405.91, 522.11, 0.99, -1.50, 11.67, -9.08, -0.25, -0.02, -0.12, -0.13, -305.78, 326.60, 1.75, 1.90, 7.30,
        6.84, 0.20, -0.04, 300.99, -325.03, -0.44, -0.50, -7.27, -6.73, -1.01, 0.01, 0.00, 0.08, 0.00, 0.02, 438.51,
        10.47, -0.56, -0.20, 0.24, -9.81, -0.24, 0.01, -0.13, -264.02, 335.24, 0.99, 1.40, 7.49, 5.90, -0.27, -0.02,
        284.09, 307.03, 0.32, -0.40, 6.87, -6.35, -0.99, -0.01, -250.54, 327.11, 0.08, 0.40, 7.31, 5.60, -0.30, 230.72,
        -304.46, 0.08, -0.10, -6.81, -5.16, 0.27, 229.78, 304.17, -0.60, 0.50, 6.80, -5.14, 0.33, 0.01, 256.30, -276.81,
        -0.28, -0.40, -6.19, -5.73, -0.14, 0.01, -212.82, 269.45, 0.84, 1.20, 6.02, 4.76, 0.14, -0.02, 196.64, 272.05,
        -0.84, 0.90, 6.08, -4.40, 0.35, 0.02, 188.95, 272.22, -0.12, 0.30, 6.09, -4.22, 0.34, -292.37, -5.10, -0.32,
        -0.40, -0.11, 6.54, 0.14, 0.01, 161.79, -220.67, 0.24, 0.10, -4.93, -3.62, -0.08, 261.54, -19.94, -0.95, 0.20,
        -0.45, -5.85, -0.13, 0.02, 142.16, -190.79, 0.20, 0.10, -4.27, -3.18, -0.07, 187.95, -4.11, -0.24, 0.30, -0.09,
        -4.20, -0.09, 0.01, 0.00, 0.00, -79.08, 167.90, 0.04, 0.00, 3.75, 1.77, 121.98, 131.04, -0.08, 0.10, 2.93,
        -2.73, -0.06, -172.95, -8.11, -0.40, -0.20, -0.18, 3.87, 0.09, 0.01, -160.15, -55.30, -14.04, 13.90, -1.23,
        3.58, 0.40, 0.31, -115.40, 123.20, 0.60, 0.70, 2.75, 2.58, 0.08, -0.01, -168.26, -2.00, 0.20, -0.20, -0.04,
        3.76, 0.08, -114.49, 123.20, 0.32, 0.40, 2.75, 2.56, 0.07, -0.01, 112.14, 120.70, 0.28, -0.30, 2.70, -2.51,
        -0.07, -0.01, 161.34, 4.03, 0.20, 0.20, 0.09, -3.61, -0.08, 91.31, 126.64, -0.40, 0.40, 2.83, -2.04, -0.04,
        0.01, 105.29, 112.90, 0.44, -0.50, 2.52, -2.35, -0.07, -0.01, 98.69, -106.20, -0.28, -0.30, -2.37, -2.21, -0.06,
        0.01, 86.74, -112.94, -0.08, -0.20, -2.53, -1.94, -0.05, -134.81, 3.51, 0.20, -0.20, 0.08, 3.01, 0.07, 79.03,
        107.31,
        /* 660-979 */
        -0.24, 0.20, 2.40, -1.77, -0.04, 0.01, 132.81, -10.77, -0.52, 0.10, -0.24, -2.97, -0.07, 0.01, -130.31, -0.90,
        0.04, 0.00, 0.00, 2.91, -78.56, 85.32, 0.00, 0.00, 1.91, 1.76, 0.04, 0.00, 0.00, -41.53, 89.10, 0.02, 0.00,
        1.99, 0.93, 66.03, -71.00, -0.20, -0.20, -1.59, -1.48, -0.04, 60.50, 64.70, 0.36, -0.40, 1.45, -1.35, -0.04,
        -0.01, -52.27, -70.01, 0.00, 0.00, -1.57, 1.17, 0.03, -52.95, 66.29, 0.32, 0.40, 1.48, 1.18, 0.04, -0.01, 51.02,
        67.25, 0.00, 0.00, 1.50, -1.14, -0.03, -55.66, -60.92, 0.16, -0.20, -1.36, 1.24, 0.03, -54.81, -59.20, -0.08,
        0.20, -1.32, 1.23, 0.03, 51.32, -55.60, 0.00, 0.00, -1.24, -1.15, -0.03, 48.29, 51.80, 0.20, -0.20, 1.16, -1.08,
        -0.03, -45.59, -49.00, -0.12, 0.10, -1.10, 1.02, 0.03, 40.54, -52.69, -0.04, -0.10, -1.18, -0.91, -0.02, -40.58,
        -49.51, -1.00, 1.00, -1.11, 0.91, 0.04, 0.02, -43.76, 46.50, 0.36, 0.40, 1.04, 0.98, 0.03, -0.01, 62.65, -5.00,
        -0.24, 0.00, -0.11, -1.40, -0.03, 0.01, -38.57, 49.59, 0.08, 0.10, 1.11, 0.86, 0.02, -33.22, -44.04, 0.08,
        -0.10, -0.98, 0.74, 0.02, 37.15, -39.90, -0.12, -0.10, -0.89, -0.83, -0.02, 36.68, -39.50, -0.04, -0.10, -0.88,
        -0.82, -0.02, -53.22, -3.91, -0.20, 0.00, -0.09, 1.19, 0.03, 32.43, -42.19, -0.04, -0.10, -0.94, -0.73, -0.02,
        -51.00, -2.30, -0.12, -0.10, 0.00, 1.14, -29.53, -39.11, 0.04, 0.00, -0.87, 0.66, 0.02, 28.50, -38.92, -0.08,
        -0.10, -0.87, -0.64, -0.02, 26.54, 36.95, -0.12, 0.10, 0.83, -0.59, -0.01, 26.54, 34.59, 0.04, -0.10, 0.77,
        -0.59, -0.02, 28.35, -32.55, -0.16, 0.20, -0.73, -0.63, -0.01, -28.00, 30.40, 0.00, 0.00, 0.68, 0.63, 0.01,
        -27.61, 29.40, 0.20, 0.20, 0.66, 0.62, 0.02, 40.33, 0.40, -0.04, 0.10, 0.00, -0.90, -23.28, 31.61, -0.08, -0.10,
        0.71, 0.52, 0.01, 37.75, 0.80, 0.04, 0.10, 0.00, -0.84, 23.66, 25.80, 0.00, 0.00, 0.58, -0.53, -0.01, 21.01,
        -27.91, 0.00, 0.00, -0.62, -0.47, -0.01, -34.81, 2.89, 0.04, 0.00, 0.00, 0.78, -23.49, -25.31, 0.00, 0.00,
        -0.57, 0.53, 0.01, -23.47, 25.20, 0.16, 0.20, 0.56, 0.52, 0.02, 19.58, 27.50, -0.12, 0.10, 0.62, -0.44, -0.01,
        -22.67, -24.40, -0.08, 0.10, -0.55, 0.51, 0.01, -19.97, 25.00, 0.12, 0.20, 0.56, 0.45, 0.01, 21.28, -22.80,
        -0.08, -0.10, -0.51, -0.48, -0.01, -30.47, 0.91, 0.04, 0.00,
        /* 980-1299 */
        0.00, 0.68, 18.58, 24.00, 0.04, -0.10, 0.54, -0.42, -0.01, -18.02, 24.40, -0.04, -0.10, 0.55, 0.40, 0.01, 17.74,
        22.50, 0.08, -0.10, 0.50, -0.40, -0.01, -19.41, 20.70, 0.08, 0.10, 0.46, 0.43, 0.01, -18.64, 20.11, 0.00, 0.00,
        0.45, 0.42, 0.01, -16.75, 21.60, 0.04, 0.10, 0.48, 0.37, 0.01, -18.42, -20.00, 0.00, 0.00, -0.45, 0.41, 0.01,
        -26.77, 1.41, 0.08, 0.00, 0.00, 0.60, -26.17, -0.19, 0.00, 0.00, 0.00, 0.59, -15.52, 20.51, 0.00, 0.00, 0.46,
        0.35, 0.01, -25.42, -1.91, -0.08, 0.00, -0.04, 0.57, 0.45, -17.42, 18.10, 0.00, 0.00, 0.40, 0.39, 0.01, 16.39,
        -17.60, -0.08, -0.10, -0.39, -0.37, -0.01, -14.37, 18.91, 0.00, 0.00, 0.42, 0.32, 0.01, 23.39, -2.40, -0.12,
        0.00, 0.00, -0.52, 14.32, -18.50, -0.04, -0.10, -0.41, -0.32, -0.01, 15.69, 17.08, 0.00, 0.00, 0.38, -0.35,
        -0.01, -22.99, 0.50, 0.04, 0.00, 0.00, 0.51, 0.00, 0.00, 14.47, -17.60, -0.01, 0.00, -0.39, -0.32, -13.33,
        18.40, -0.04, -0.10, 0.41, 0.30, 22.47, -0.60, -0.04, 0.00, 0.00, -0.50, -12.78, -17.41, 0.04, 0.00, -0.39,
        0.29, 0.01, -14.10, -15.31, 0.04, 0.00, -0.34, 0.32, 0.01, 11.98, 16.21, -0.04, 0.00, 0.36, -0.27, -0.01, 19.65,
        -1.90, -0.08, 0.00, 0.00, -0.44, 19.61, -1.50, -0.08, 0.00, 0.00, -0.44, 13.41, -14.30, -0.04, -0.10, -0.32,
        -0.30, -0.01, -13.29, 14.40, 0.00, 0.00, 0.32, 0.30, 0.01, 11.14, -14.40, -0.04, 0.00, -0.32, -0.25, -0.01,
        12.24, -13.38, 0.04, 0.00, -0.30, -0.27, -0.01, 10.07, -13.81, 0.04, 0.00, -0.31, -0.23, -0.01, 10.46, 13.10,
        0.08, -0.10, 0.29, -0.23, -0.01, 16.55, -1.71, -0.08, 0.00, 0.00, -0.37, 9.75, -12.80, 0.00, 0.00, -0.29, -0.22,
        -0.01, 9.11, 12.80, 0.00, 0.00, 0.29, -0.20, 0.00, 0.00, -6.44, -13.80, 0.00, 0.00, -0.31, 0.14, -9.19, -12.00,
        0.00, 0.00, -0.27, 0.21, -10.30, 10.90, 0.08, 0.10, 0.24, 0.23, 0.01, 14.92, -0.80, -0.04, 0.00, 0.00, -0.33,
        10.02, -10.80, 0.00, 0.00, -0.24, -0.22, -0.01, -9.75, 10.40, 0.04, 0.00, 0.23, 0.22, 0.01, 9.67, -10.40, -0.04,
        0.00, -0.23, -0.22, -0.01, -8.28, -11.20, 0.04, 0.00, -0.25, 0.19, 13.32, -1.41, -0.08, 0.00, 0.00, -0.30, 8.27,
        10.50, 0.04, 0.00, 0.23, -0.19, 0.00, 0.00, 13.13, 0.00, 0.00, 0.00, 0.00, -0.29, -12.93, 0.70, 0.04, 0.00,
        0.00, 0.29, 7.91, -10.20,
        /*1300-1619 */
        0.00, 0.00, -0.23, -0.18, -7.84, -10.00, -0.04, 0.00, -0.22, 0.18, 7.44, 9.60, 0.00, 0.00, 0.21, -0.17, -7.64,
        9.40, 0.08, 0.10, 0.21, 0.17, 0.01, -11.38, 0.60, 0.04, 0.00, 0.00, 0.25, -7.48, 8.30, 0.00, 0.00, 0.19, 0.17,
        -10.98, -0.20, 0.00, 0.00, 0.00, 0.25, 10.98, 0.20, 0.00, 0.00, 0.00, -0.25, 7.40, -7.90, -0.04, 0.00, -0.18,
        -0.17, -6.09, 8.40, -0.04, 0.00, 0.19, 0.14, -6.94, -7.49, 0.00, 0.00, -0.17, 0.16, 6.92, 7.50, 0.04, 0.00,
        0.17, -0.15, 6.20, 8.09, 0.00, 0.00, 0.18, -0.14, -6.12, 7.80, 0.04, 0.00, 0.17, 0.14, 5.85, -7.50, 0.00, 0.00,
        -0.17, -0.13, -6.48, 6.90, 0.08, 0.10, 0.15, 0.14, 0.01, 6.32, 6.90, 0.00, 0.00, 0.15, -0.14, 5.61, -7.20, 0.00,
        0.00, -0.16, -0.13, 9.07, 0.00, 0.00, 0.00, 0.00, -0.20, 5.25, 6.90, 0.00, 0.00, 0.15, -0.12, -8.47, -0.40,
        0.00, 0.00, 0.00, 0.19, 6.32, -5.39, -1.11, 1.10, -0.12, -0.14, 0.02, 0.02, 5.73, -6.10, -0.04, 0.00, -0.14,
        -0.13, 4.70, 6.60, -0.04, 0.00, 0.15, -0.11, -4.90, -6.40, 0.00, 0.00, -0.14, 0.11, -5.33, 5.60, 0.04, 0.10,
        0.13, 0.12, 0.01, -4.81, 6.00, 0.04, 0.00, 0.13, 0.11, 5.13, 5.50, 0.04, 0.00, 0.12, -0.11, 4.50, 5.90, 0.00,
        0.00, 0.13, -0.10, -4.22, 6.10, 0.00, 0.00, 0.14, -4.53, 5.70, 0.00, 0.00, 0.13, 0.10, 4.18, 5.70, 0.00, 0.00,
        0.13, -4.75, -5.19, 0.00, 0.00, -0.12, 0.11, -4.06, 5.60, 0.00, 0.00, 0.13, -3.98, 5.60, -0.04, 0.00, 0.13,
        4.02, -5.40, 0.00, 0.00, -0.12, 4.49, -4.90, -0.04, 0.00, -0.11, -0.10, -3.62, -5.40, -0.16, 0.20, -0.12, 0.00,
        0.01, 4.38, 4.80, 0.00, 0.00, 0.11, -6.40, -0.10, 0.00, 0.00, 0.00, 0.14, -3.98, 5.00, 0.04, 0.00, 0.11, -3.82,
        -5.00, 0.00, 0.00, -0.11, -3.71, 5.07, 0.00, 0.00, 0.11, 4.14, 4.40, 0.00, 0.00, 0.10, -6.01, -0.50, -0.04,
        0.00, 0.00, 0.13, -4.04, 4.39, 0.00, 0.00, 0.10, 3.45, -4.72, 0.00, 0.00, -0.11, 3.31, 4.71, 0.00, 0.00, 0.11,
        3.26, -4.50, 0.00, 0.00, -0.10, -3.26, -4.50, 0.00, 0.00, -0.10, -3.34, -4.40, 0.00, 0.00, -0.10, -3.74, -4.00,
        3.70, 4.00, 3.34, -4.30, 3.30, -4.30, -3.66, 3.90, 0.04, 3.66, 3.90, 0.04, -3.62, -3.90, -3.61, 3.90, -0.20,
        5.30, 0.00, 0.00, 0.12, 3.06, 4.30, 3.30,
        /*1620-1939 */
        4.00, 0.40, 0.20, 3.10, 4.10, -3.06, 3.90, -3.30, -3.60, -3.30, 3.36, 0.01, 3.14, 3.40, -4.57, -0.20, 0.00,
        0.00, 0.00, 0.10, -2.70, -3.60, 2.94, -3.20, -2.90, 3.20, 2.47, -3.40, 2.55, -3.30, 2.80, -3.08, 2.51, 3.30,
        -4.10, 0.30, -0.12, -0.10, 4.10, 0.20, -2.74, 3.00, 2.46, 3.23, -3.66, 1.20, -0.20, 0.20, 3.74, -0.40, -2.51,
        -2.80, -3.74, 2.27, -2.90, 0.00, 0.00, -2.50, 2.70, -2.51, 2.60, -3.50, 0.20, 3.38, -2.22, -2.50, 3.26, -0.40,
        1.95, -2.60, 3.22, -0.40, -0.04, -1.79, -2.60, 1.91, 2.50, 0.74, 3.05, -0.04, 0.08, 2.11, -2.30, -2.11, 2.20,
        -1.87, -2.40, 2.03, -2.20, -2.03, 2.20, 2.98, 0.00, 0.00, 2.98, -1.71, 2.40, 2.94, -0.10, -0.12, 0.10, 1.67,
        2.40, -1.79, 2.30, -1.79, 2.20, -1.67, 2.20, 1.79, -2.00, 1.87, -1.90, 1.63, -2.10, -1.59, 2.10, 1.55, -2.10,
        -1.55, 2.10, -2.59, -0.20, -1.75, -1.90, -1.75, 1.90, -1.83, -1.80, 1.51, 2.00, -1.51, -2.00, 1.71, 1.80, 1.31,
        2.10, -1.43, 2.00, 1.43, 2.00, -2.43, -1.51, 1.90, -1.47, 1.90, 2.39, 0.20, -2.39, 1.39, 1.90, 1.39, -1.80,
        1.47, -1.60, 1.47, -1.60, 1.43, -1.50, -1.31, 1.60, 1.27, -1.60, -1.27, 1.60, 1.27, -1.60, 2.03, 1.35, 1.50,
        -1.39, -1.40, 1.95, -0.20, -1.27, 1.49, 1.19, 1.50, 1.27, 1.40, 1.15, 1.50, 1.87, -0.10, -1.12, -1.50, 1.87,
        -1.11, -1.50, -1.11, -1.50, 0.00, 0.00, 1.19, 1.40, 1.27, -1.30, -1.27, -1.30, -1.15, 1.40, -1.23, 1.30, -1.23,
        -1.30, 1.22, -1.29, 1.07, -1.40, 1.75, -0.20, -1.03, -1.40, -1.07, 1.20, -1.03, 1.15, 1.07, 1.10, 1.51, -1.03,
        1.10, 1.03, -1.10, 0.00, 0.00, -1.03, -1.10, 0.91, -1.20, -0.88, -1.20, -0.88, 1.20, -0.95, 1.10, -0.95, -1.10,
        1.43, -1.39, 0.95, -1.00, -0.95, 1.00, -0.80, 1.10, 0.91, -1.00, -1.35, 0.88, 1.00, -0.83, 1.00, -0.91, 0.90,
        0.91, 0.90, 0.88, -0.90, -0.76, -1.00, -0.76, 1.00, 0.76, 1.00, -0.72, 1.00, 0.84, -0.90, 0.84, 0.90, 1.23,
        0.00, 0.00, -0.52, -1.10, -0.68, 1.00, 1.19, -0.20, 1.19, 0.76, 0.90, 1.15, -0.10, 1.15, -0.10, 0.72, -0.90,
        -1.15, -1.15, 0.68, 0.90, -0.68, 0.90, -1.11, 0.00, 0.00, 0.20, 0.79, 0.80, -1.11, -0.10, 0.00, 0.00, -0.48,
        -1.00, -0.76, -0.80, -0.72, -0.80, -1.07, -0.10, 0.64, 0.80, -0.64, -0.80, 0.64, 0.80, 0.40,
        /*1940-2259 */
        0.60, 0.52, -0.50, -0.60, -0.80, -0.71, 0.70, -0.99, 0.99, 0.56, 0.80, -0.56, 0.80, 0.68, -0.70, 0.68, 0.70,
        -0.95, -0.64, 0.70, 0.64, 0.70, -0.60, 0.70, -0.60, -0.70, -0.91, -0.10, -0.51, 0.76, -0.91, -0.56, 0.70, 0.88,
        0.88, -0.63, -0.60, 0.55, -0.60, -0.80, 0.80, -0.80, -0.52, 0.60, 0.52, 0.60, 0.52, -0.60, -0.48, 0.60, 0.48,
        0.60, 0.48, 0.60, -0.76, 0.44, -0.60, 0.52, -0.50, -0.52, 0.50, 0.40, 0.60, -0.40, -0.60, 0.40, -0.60, 0.72,
        -0.72, -0.51, -0.50, -0.48, 0.50, 0.48, -0.50, -0.48, 0.50, -0.48, 0.50, 0.48, -0.50, -0.48, -0.50, -0.68,
        -0.68, 0.44, 0.50, -0.64, -0.10, -0.64, -0.10, -0.40, 0.50, 0.40, 0.50, 0.40, 0.50, 0.00, 0.00, -0.40, -0.50,
        -0.36, -0.50, 0.36, -0.50, 0.60, -0.60, 0.40, -0.40, 0.40, 0.40, -0.40, 0.40, -0.40, 0.40, -0.56, -0.56, 0.36,
        -0.40, -0.36, 0.40, 0.36, -0.40, -0.36, -0.40, 0.36, 0.40, 0.36, 0.40, -0.52, 0.52, 0.52, 0.32, 0.40, -0.32,
        0.40, -0.32, 0.40, -0.32, 0.40, 0.32, -0.40, -0.32, -0.40, 0.32, -0.40, 0.28, -0.40, -0.28, 0.40, 0.28, -0.40,
        0.28, 0.40, 0.48, -0.48, 0.48, 0.36, -0.30, -0.36, -0.30, 0.00, 0.00, 0.20, 0.40, -0.44, 0.44, -0.44, -0.44,
        -0.44, -0.44, 0.32, -0.30, 0.32, 0.30, 0.24, 0.30, -0.12, -0.10, -0.28, 0.30, 0.28, 0.30, 0.28, 0.30, 0.28,
        -0.30, 0.28, -0.30, 0.28, -0.30, 0.28, 0.30, -0.28, 0.30, 0.40, 0.40, -0.24, 0.30, 0.24, -0.30, 0.24, -0.30,
        -0.24, -0.30, 0.24, 0.30, 0.24, -0.30, -0.24, 0.30, 0.24, -0.30, -0.24, -0.30, 0.24, -0.30, 0.24, 0.30, -0.24,
        0.30, -0.24, 0.30, 0.20, -0.30, 0.20, -0.30, 0.20, -0.30, 0.20, 0.30, 0.20, -0.30, 0.20, -0.30, 0.20, 0.30,
        0.20, 0.30, -0.20, -0.30, 0.20, -0.30, 0.20, -0.30, -0.36, -0.36, -0.36, -0.04, 0.30, 0.12, -0.10, -0.32, -0.24,
        0.20, 0.24, 0.20, 0.20, -0.20, -0.20, -0.20, -0.20, -0.20, 0.20, 0.20, 0.20, -0.20, 0.20, 0.20, 0.20, 0.20,
        -0.20, -0.20, 0.00, 0.00, -0.20, -0.20, -0.20, 0.20, -0.20, 0.20, 0.20, -0.20, -0.20, -0.20, 0.20, 0.20, 0.20,
        0.20, 0.20, -0.20, 0.20, -0.20, 0.28, 0.28, 0.28, 0.28, 0.28, 0.28, -0.28, 0.28, 0.12, 0.00, 0.24, 0.16, -0.20,
        0.16, -0.20, 0.16, -0.20, 0.16, 0.20, -0.16, 0.20, 0.16, 0.20, -0.16, 0.20, -0.16, 0.20,
        /*2260-2579 */
        -0.16, 0.20, 0.16, -0.20, 0.16, 0.20, 0.16, -0.20, -0.16, 0.20, -0.16, -0.20, -0.16, 0.20, 0.16, 0.20, 0.16,
        -0.20, 0.16, -0.20, 0.16, 0.20, 0.16, 0.20, 0.16, 0.20, -0.16, -0.20, 0.16, 0.20, -0.16, 0.20, 0.16, 0.20,
        -0.16, -0.20, 0.16, -0.20, 0.16, -0.20, -0.16, -0.20, 0.24, -0.24, -0.24, 0.24, 0.24, 0.12, 0.20, 0.12, 0.20,
        -0.12, -0.20, 0.12, -0.20, 0.12, -0.20, -0.12, 0.20, -0.12, 0.20, -0.12, -0.20, 0.12, 0.20, 0.12, 0.20, 0.12,
        -0.20, -0.12, 0.20, 0.12, -0.20, -0.12, 0.20, 0.12, 0.20, 0.00, 0.00, -0.12, 0.20, -0.12, 0.20, 0.12, -0.20,
        -0.12, 0.20, 0.12, 0.20, 0.00, -0.21, -0.20, 0.00, 0.00, 0.20, -0.20, -0.20, -0.20, 0.20, -0.16, -0.10, 0.00,
        0.17, 0.16, 0.16, 0.16, 0.16, -0.16, 0.16, 0.16, -0.16, 0.16, -0.16, 0.16, 0.12, 0.10, 0.12, -0.10, -0.12, 0.10,
        -0.12, 0.10, 0.12, -0.10, -0.12, 0.12, -0.12, 0.12, -0.12, 0.12, -0.12, -0.12, -0.12, -0.12, -0.12, -0.12,
        -0.12, 0.12, 0.12, 0.12, 0.12, -0.12, -0.12, 0.12, 0.12, 0.12, -0.12, 0.12, -0.12, -0.12, -0.12, 0.12, -0.12,
        -0.12, 0.12, 0.00, 0.11, 0.11, -122.67, 164.70, 203.78, 273.50, 3.58, 2.74, 6.18, -4.56, 0.00, -0.04, 0.00,
        -0.07, 57.44, -77.10, 95.82, 128.60, -1.77, -1.28, 2.85, -2.14, 82.14, 89.50, 0.00, 0.00, 2.00, -1.84, -0.04,
        47.73, -64.10, 23.79, 31.90, -1.45, -1.07, 0.69, -0.53, -46.38, 50.50, 0.00, 0.00, 1.13, 1.04, 0.02, -18.38,
        0.00, 63.80, 0.00, 0.00, 0.41, 0.00, -1.43, 59.07, 0.00, 0.00, 0.00, 0.00, -1.32, 57.28, 0.00, 0.00, 0.00, 0.00,
        -1.28, -48.65, 0.00, -1.15, 0.00, 0.00, 1.09, 0.00, 0.03, -18.30, 24.60, -17.30, -23.20, 0.56, 0.41, -0.51,
        0.39, -16.91, 26.90, 8.43, 13.30, 0.60, 0.38, 0.31, -0.19, 1.23, -1.70, -19.13, -25.70, -0.03, -0.03, -0.58,
        0.43, -0.72, 0.90, -17.34, -23.30, 0.03, 0.02, -0.52, 0.39, -19.49, -21.30, 0.00, 0.00, -0.48, 0.44, 0.01,
        20.57, -20.10, 0.64, 0.70, -0.45, -0.46, 0.00, -0.01, 4.89, 5.90, -16.55, 19.90, 0.14, -0.11, 0.44, 0.37, 18.22,
        19.80, 0.00, 0.00, 0.44, -0.41, -0.01, 4.89, -5.30, -16.51, -18.00, -0.11, -0.11, -0.41, 0.37, -17.86, 0.00,
        17.10, 0.00, 0.00, 0.40, 0.00, -0.38, 0.32, 0.00, 24.42, 0.00, 0.00, -0.01, 0.00, -0.55, -23.79, 0.00, 0.00,
        0.00, 0.00, 0.53,
        /*2580-2899 */
        14.72, -16.00, -0.32, 0.00, -0.36, -0.33, -0.01, 0.01, 3.34, -4.50, 11.86, 15.90, -0.11, -0.07, 0.35, -0.27,
        -3.26, 4.40, 11.62, 15.60, 0.09, 0.07, 0.35, -0.26, -19.53, 0.00, 5.09, 0.00, 0.00, 0.44, 0.00, -0.11, -13.48,
        14.70, 0.00, 0.00, 0.33, 0.30, 0.01, 10.86, -14.60, 3.18, 4.30, -0.33, -0.24, 0.09, -0.07, -11.30, -15.10, 0.00,
        0.00, -0.34, 0.25, 0.01, 2.03, -2.70, 10.82, 14.50, -0.07, -0.05, 0.32, -0.24, 17.46, 0.00, 0.00, 0.00, 0.00,
        -0.39, 16.43, 0.00, 0.52, 0.00, 0.00, -0.37, 0.00, -0.01, 9.35, 0.00, 13.29, 0.00, 0.00, -0.21, 0.00, -0.30,
        -10.42, 11.40, 0.00, 0.00, 0.25, 0.23, 0.01, 0.44, 0.50, -10.38, 11.30, 0.02, -0.01, 0.25, 0.23, -14.64, 0.00,
        0.00, 0.00, 0.00, 0.33, 0.56, 0.80, -8.67, 11.70, 0.02, -0.01, 0.26, 0.19, 13.88, 0.00, -2.47, 0.00, 0.00,
        -0.31, 0.00, 0.06, -1.99, 2.70, 7.72, 10.30, 0.06, 0.04, 0.23, -0.17, -0.20, 0.00, 13.05, 0.00, 0.00, 0.00,
        0.00, -0.29, 6.92, -9.30, 3.34, 4.50, -0.21, -0.15, 0.10, -0.07, -6.60, 0.00, 10.70, 0.00, 0.00, 0.15, 0.00,
        -0.24, -8.04, -8.70, 0.00, 0.00, -0.19, 0.18, -10.58, 0.00, -3.10, 0.00, 0.00, 0.24, 0.00, 0.07, -7.32, 8.00,
        -0.12, -0.10, 0.18, 0.16, 1.63, 1.70, 6.96, -7.60, 0.03, -0.04, -0.17, -0.16, -3.62, 0.00, 9.86, 0.00, 0.00,
        0.08, 0.00, -0.22, 0.20, -0.20, -6.88, -7.50, 0.00, 0.00, -0.17, 0.15, -8.99, 0.00, 4.02, 0.00, 0.00, 0.20,
        0.00, -0.09, -1.07, 1.40, -5.69, -7.70, 0.03, 0.02, -0.17, 0.13, 6.48, -7.20, -0.48, -0.50, -0.16, -0.14, -0.01,
        0.01, 5.57, -7.50, 1.07, 1.40, -0.17, -0.12, 0.03, -0.02, 8.71, 0.00, 3.54, 0.00, 0.00, -0.19, 0.00, -0.08,
        0.40, 0.00, 9.27, 0.00, 0.00, -0.01, 0.00, -0.21, -6.13, 6.70, -1.19, -1.30, 0.15, 0.14, -0.03, 0.03, 5.21,
        -5.70, -2.51, -2.60, -0.13, -0.12, -0.06, 0.06, 5.69, -6.20, -0.12, -0.10, -0.14, -0.13, -0.01, 2.03, -2.70,
        4.53, 6.10, -0.06, -0.05, 0.14, -0.10, 5.01, 5.50, -2.51, 2.70, 0.12, -0.11, 0.06, 0.06, -1.91, 2.60, -4.38,
        -5.90, 0.06, 0.04, -0.13, 0.10, 4.65, -6.30, 0.00, 0.00, -0.14, -0.10, -5.29, 5.70, 0.00, 0.00, 0.13, 0.12,
        -2.23, -4.00, -4.65, 4.20, -0.09, 0.05, 0.10, 0.10, -4.53, 6.10, 0.00, 0.00, 0.14, 0.10, 2.47, 2.70,
        /*2900-3219 */
        -4.46, 4.90, 0.06, -0.06, 0.11, 0.10, -5.05, 5.50, 0.84, 0.90, 0.12, 0.11, 0.02, -0.02, 4.97, -5.40, -1.71,
        0.00, -0.12, -0.11, 0.00, 0.04, -0.99, -1.30, 4.22, -5.70, -0.03, 0.02, -0.13, -0.09, 0.99, 1.40, 4.22, -5.60,
        0.03, -0.02, -0.13, -0.09, -4.69, -5.20, 0.00, 0.00, -0.12, 0.10, -3.42, 0.00, 6.09, 0.00, 0.00, 0.08, 0.00,
        -0.14, -4.65, -5.10, 0.00, 0.00, -0.11, 0.10, 0.00, 0.00, -4.53, -5.00, 0.00, 0.00, -0.11, 0.10, -2.43, -2.70,
        -3.82, 4.20, -0.06, 0.05, 0.10, 0.09, 0.00, 0.00, -4.53, 4.90, 0.00, 0.00, 0.11, 0.10, -4.49, -4.90, 0.00, 0.00,
        -0.11, 0.10, 2.67, -2.90, -3.62, -3.90, -0.06, -0.06, -0.09, 0.08, 3.94, -5.30, 0.00, 0.00, -0.12, -3.38, 3.70,
        -2.78, -3.10, 0.08, 0.08, -0.07, 0.06, 3.18, -3.50, -2.82, -3.10, -0.08, -0.07, -0.07, 0.06, -5.77, 0.00, 1.87,
        0.00, 0.00, 0.13, 0.00, -0.04, 3.54, -4.80, -0.64, -0.90, -0.11, 0.00, -0.02, -3.50, -4.70, 0.68, -0.90, -0.11,
        0.00, -0.02, 5.49, 0.00, 0.00, 0.00, 0.00, -0.12, 1.83, -2.50, 2.63, 3.50, -0.06, 0.00, 0.08, 3.02, -4.10, 0.68,
        0.90, -0.09, 0.00, 0.02, 0.00, 0.00, 5.21, 0.00, 0.00, 0.00, 0.00, -0.12, -3.54, 3.80, 2.70, 3.60, -1.35, 1.80,
        0.08, 0.00, 0.04, -2.90, 3.90, 0.68, 0.90, 0.09, 0.00, 0.02, 0.80, -1.10, -2.78, -3.70, -0.02, 0.00, -0.08,
        4.10, 0.00, -2.39, 0.00, 0.00, -0.09, 0.00, 0.05, -1.59, 2.10, 2.27, 3.00, 0.05, 0.00, 0.07, -2.63, 3.50, -0.48,
        -0.60, -2.94, -3.20, -2.94, 3.20, 2.27, -3.00, -1.11, -1.50, -0.07, 0.00, -0.03, -0.56, -0.80, -2.35, 3.10,
        0.00, -0.60, -3.42, 1.90, -0.12, -0.10, 2.63, -2.90, 2.51, 2.80, -0.64, 0.70, -0.48, -0.60, 2.19, -2.90, 0.24,
        -0.30, 2.15, 2.90, 2.15, -2.90, 0.52, 0.70, 2.07, -2.80, -3.10, 0.00, 1.79, 0.00, 0.00, 0.07, 0.00, -0.04, 0.88,
        0.00, -3.46, 2.11, 2.80, -0.36, 0.50, 3.54, -0.20, -3.50, -1.39, 1.50, -1.91, -2.10, -1.47, 2.00, 1.39, 1.90,
        2.07, -2.30, 0.91, 1.00, 1.99, -2.70, 3.30, 0.00, 0.60, -0.44, -0.70, -1.95, 2.60, 2.15, -2.40, -0.60, -0.70,
        3.30, 0.84, 0.00, -3.10, -3.10, 0.00, -0.72, -0.32, 0.40, -1.87, -2.50, 1.87, -2.50, 0.32, 0.40, -0.24, 0.30,
        -1.87, -2.50, -0.24, -0.30, 1.87, -2.50, -2.70, 0.00, 1.55, 2.03,
        /*3220-3539 */
        2.20, -2.98, -1.99, -2.20, 0.12, -0.10, -0.40, 0.50, 1.59, 2.10, 0.00, 0.00, -1.79, 2.00, -1.03, 1.40, -1.15,
        -1.60, 0.32, 0.50, 1.39, -1.90, 2.35, -1.27, 1.70, 0.60, 0.80, -0.32, -0.40, 1.35, -1.80, 0.44, 0.00, 2.23,
        -0.84, 0.90, -1.27, -1.40, -1.47, 1.60, -0.28, -0.30, -0.28, 0.40, -1.27, -1.70, 0.28, -0.40, -1.43, -1.50,
        0.00, 0.00, -1.27, -1.70, 2.11, -0.32, -0.40, -1.23, 1.60, 1.19, -1.30, -0.72, -0.80, 0.72, -0.80, -1.15, -1.30,
        -1.35, -1.50, -1.19, -1.60, -0.12, 0.20, 1.79, 0.00, -0.88, -0.28, 0.40, 1.11, 1.50, -1.83, 0.00, 0.56, -0.12,
        0.10, -1.27, -1.40, 0.00, 0.00, 1.15, 1.50, -0.12, 0.20, 1.11, 1.50, 0.36, -0.50, -1.07, -1.40, -1.11, 1.50,
        1.67, 0.00, 0.80, -1.11, 0.00, 1.43, 1.23, -1.30, -0.24, -1.19, -1.30, -0.24, 0.20, -0.44, -0.90, -0.95, 1.10,
        1.07, -1.40, 1.15, -1.30, 1.03, -1.10, -0.56, -0.60, -0.68, 0.90, -0.76, -1.00, -0.24, -0.30, 0.95, -1.30, 0.56,
        0.70, 0.84, -1.10, -0.56, 0.00, -1.55, 0.91, -1.30, 0.28, 0.30, 0.16, -0.20, 0.95, 1.30, 0.40, -0.50, -0.88,
        -1.20, 0.95, -1.10, -0.48, -0.50, 0.00, 0.00, -1.07, 1.20, 0.44, -0.50, 0.95, 1.10, 0.00, 0.00, 0.92, -1.30,
        0.95, 1.00, -0.52, 0.60, 1.59, 0.24, -0.40, 0.91, 1.20, 0.84, -1.10, -0.44, -0.60, 0.84, 1.10, -0.44, 0.60,
        -0.44, 0.60, -0.84, -1.10, -0.80, 0.00, 1.35, 0.76, 0.20, -0.91, -1.00, 0.20, -0.30, -0.91, -1.20, -0.95, 1.00,
        -0.48, -0.50, 0.88, 1.00, 0.48, -0.50, -0.95, -1.10, 0.20, -0.20, -0.99, 1.10, -0.84, 1.10, -0.24, -0.30, 0.20,
        -0.30, 0.84, 1.10, -1.39, 0.00, -0.28, -0.16, 0.20, 0.84, 1.10, 0.00, 0.00, 1.39, 0.00, 0.00, -0.95, 1.00, 1.35,
        -0.99, 0.00, 0.88, -0.52, 0.00, -1.19, 0.20, 0.20, 0.76, -1.00, 0.00, 0.00, 0.76, 1.00, 0.00, 0.00, 0.76, 1.00,
        -0.76, 1.00, 0.00, 0.00, 1.23, 0.76, 0.80, -0.32, 0.40, -0.72, 0.80, -0.40, -0.40, 0.00, 0.00, -0.80, -0.90,
        -0.68, 0.90, -0.16, -0.20, -0.16, -0.20, 0.68, -0.90, -0.36, 0.50, -0.56, -0.80, 0.72, -0.90, 0.44, -0.60,
        -0.48, -0.70, -0.16, 0.00, -1.11, 0.32, 0.00, -1.07, 0.60, -0.80, -0.28, -0.40, -0.64, 0.00, 0.91, 1.11, 0.64,
        -0.90, 0.76, -0.80, 0.00, 0.00, -0.76, -0.80, 1.03, 0.00, -0.36, -0.64, -0.70, 0.36, -0.40,
        /*3540-3859 */
        1.07, 0.36, -0.50, -0.52, -0.70, 0.60, 0.00, 0.88, 0.95, 0.00, 0.48, 0.16, -0.20, 0.60, 0.80, 0.16, -0.20,
        -0.60, -0.80, 0.00, -1.00, 0.12, 0.20, 0.16, -0.20, 0.68, 0.70, 0.59, -0.80, -0.99, -0.56, -0.60, 0.36, -0.40,
        -0.68, -0.70, -0.68, -0.70, -0.36, -0.50, -0.44, 0.60, 0.64, 0.70, -0.12, 0.10, -0.52, 0.60, 0.36, 0.40, 0.00,
        0.00, 0.95, -0.84, 0.00, 0.44, 0.56, 0.60, 0.32, -0.30, 0.00, 0.00, 0.60, 0.70, 0.00, 0.00, 0.60, 0.70, -0.12,
        -0.20, 0.52, -0.70, 0.00, 0.00, 0.56, 0.70, -0.12, 0.10, -0.52, -0.70, 0.00, 0.00, 0.88, -0.76, 0.00, -0.44,
        0.00, 0.00, -0.52, -0.70, 0.52, -0.70, 0.36, -0.40, -0.44, -0.50, 0.00, 0.00, 0.60, 0.60, 0.84, 0.00, 0.12,
        -0.24, 0.00, 0.80, -0.56, 0.60, -0.32, -0.30, 0.48, -0.50, 0.28, -0.30, -0.48, -0.50, 0.12, 0.20, 0.48, -0.60,
        0.48, 0.60, -0.12, 0.20, 0.24, 0.00, 0.76, -0.52, -0.60, -0.52, 0.60, 0.48, -0.50, -0.24, -0.30, 0.12, -0.10,
        0.48, 0.60, 0.52, -0.20, 0.36, 0.40, -0.44, 0.50, -0.24, -0.30, -0.48, -0.60, -0.44, -0.60, -0.12, 0.10, 0.76,
        0.76, 0.20, -0.20, 0.48, 0.50, 0.40, -0.50, -0.24, -0.30, 0.44, -0.60, 0.44, -0.60, 0.36, 0.00, -0.64, 0.72,
        0.00, -0.12, 0.00, -0.10, -0.40, -0.60, -0.20, -0.20, -0.44, 0.50, -0.44, 0.50, 0.20, 0.20, -0.44, -0.50, 0.20,
        -0.20, -0.20, 0.20, -0.44, -0.50, 0.64, 0.00, 0.32, -0.36, 0.50, -0.20, -0.30, 0.12, -0.10, 0.48, 0.50, -0.12,
        0.30, -0.36, -0.50, 0.00, 0.00, 0.48, 0.50, -0.48, 0.50, 0.68, 0.00, -0.12, 0.56, -0.40, 0.44, -0.50, -0.12,
        -0.10, 0.24, 0.30, -0.40, 0.40, 0.64, 0.00, -0.24, 0.64, 0.00, -0.20, 0.00, 0.00, 0.44, -0.50, 0.44, 0.50,
        -0.12, 0.20, -0.36, -0.50, 0.12, 0.00, 0.64, -0.40, 0.50, 0.00, 0.10, 0.00, 0.00, -0.40, 0.50, 0.00, 0.00,
        -0.40, -0.50, 0.56, 0.00, 0.28, 0.00, 0.10, 0.36, 0.50, 0.00, -0.10, 0.36, -0.50, 0.36, 0.50, 0.00, -0.10, 0.24,
        -0.20, -0.36, -0.40, 0.16, 0.20, 0.40, -0.40, 0.00, 0.00, -0.36, -0.50, -0.36, -0.50, -0.32, -0.50, -0.12, 0.10,
        0.20, 0.20, -0.36, 0.40, -0.60, 0.60, 0.28, 0.00, 0.52, 0.12, -0.10, 0.40, 0.40, 0.00, -0.50, 0.20, -0.20,
        -0.32, 0.40, 0.16, 0.20, -0.16, 0.20, 0.32, 0.40, 0.56, 0.00, -0.12, 0.32,
        /*3860-4179 */
        -0.40, -0.16, -0.20, 0.00, 0.00, 0.40, 0.40, -0.40, -0.40, -0.40, 0.40, -0.36, 0.40, 0.12, 0.10, 0.00, 0.10,
        0.36, 0.40, 0.00, -0.10, 0.36, 0.40, -0.36, 0.40, 0.00, 0.10, 0.32, 0.00, 0.44, 0.12, 0.20, 0.28, -0.40, 0.00,
        0.00, 0.36, 0.40, 0.32, -0.40, -0.16, 0.12, 0.10, 0.32, -0.40, 0.20, 0.30, -0.24, 0.30, 0.00, 0.10, 0.32, 0.40,
        0.00, -0.10, -0.32, -0.40, -0.32, 0.40, 0.00, 0.10, -0.52, -0.52, 0.52, 0.32, -0.40, 0.00, 0.00, 0.32, 0.40,
        0.32, -0.40, 0.00, 0.00, -0.32, -0.40, -0.32, 0.40, 0.32, 0.40, 0.00, 0.00, 0.32, 0.40, 0.00, 0.00, -0.32,
        -0.40, 0.00, 0.00, 0.32, 0.40, 0.16, 0.20, 0.32, -0.30, -0.16, 0.00, -0.48, -0.20, 0.20, -0.28, -0.30, 0.28,
        -0.40, 0.00, 0.00, 0.28, -0.40, 0.00, 0.00, 0.28, -0.40, 0.00, 0.00, -0.28, -0.40, 0.28, 0.40, -0.28, -0.40,
        -0.48, -0.20, 0.20, 0.24, 0.30, 0.44, 0.00, 0.16, 0.24, 0.30, 0.16, -0.20, 0.24, 0.30, -0.12, 0.20, 0.20, 0.30,
        -0.16, 0.20, 0.00, 0.00, 0.44, -0.32, 0.30, 0.24, 0.00, -0.36, 0.36, 0.00, 0.24, 0.12, -0.20, 0.20, 0.30, -0.12,
        0.00, -0.28, 0.30, -0.24, 0.30, 0.12, 0.10, -0.28, -0.30, -0.28, 0.30, 0.00, 0.00, -0.28, -0.30, 0.00, 0.00,
        -0.28, -0.30, 0.00, 0.00, 0.28, 0.30, 0.00, 0.00, -0.28, -0.30, -0.28, 0.30, 0.00, 0.00, -0.28, -0.30, 0.00,
        0.00, 0.28, 0.30, 0.00, 0.00, -0.28, 0.30, 0.28, -0.30, -0.28, 0.30, 0.40, 0.40, -0.24, 0.30, 0.00, -0.10, 0.16,
        0.00, 0.36, -0.20, 0.30, -0.12, -0.10, -0.24, -0.30, 0.00, 0.00, -0.24, 0.30, -0.24, 0.30, 0.00, 0.00, -0.24,
        0.30, -0.24, 0.30, 0.24, -0.30, 0.00, 0.00, 0.24, -0.30, 0.00, 0.00, 0.24, 0.30, 0.24, -0.30, 0.24, 0.30, -0.24,
        0.30, -0.24, 0.30, -0.20, 0.20, -0.16, -0.20, 0.00, 0.00, -0.32, 0.20, 0.00, 0.10, 0.20, -0.30, 0.20, -0.20,
        0.12, 0.20, -0.16, 0.20, 0.16, 0.20, 0.20, 0.30, 0.20, 0.30, 0.00, 0.00, -0.20, 0.30, 0.00, 0.00, 0.20, 0.30,
        -0.20, -0.30, -0.20, -0.30, 0.20, -0.30, 0.00, 0.00, 0.20, 0.30, 0.00, 0.00, 0.20, 0.30, 0.00, 0.00, 0.20, 0.30,
        0.00, 0.00, 0.20, 0.30, 0.00, 0.00, 0.20, -0.30, 0.00, 0.00, -0.20, -0.30, 0.00, 0.00, -0.20, 0.30, 0.00, 0.00,
        -0.20, 0.30, 0.00, 0.00, 0.36,
        /*4180-4499 */
        0.00, 0.00, 0.36, 0.12, 0.10, -0.24, 0.20, 0.12, -0.20, -0.16, -0.20, -0.13, 0.10, 0.22, 0.21, 0.20, 0.00,
        -0.28, 0.32, 0.00, -0.12, -0.20, -0.20, 0.12, -0.10, 0.12, 0.10, -0.20, 0.20, 0.00, 0.00, -0.32, 0.32, 0.00,
        0.00, 0.32, 0.32, 0.00, 0.00, -0.24, -0.20, 0.24, 0.20, 0.20, 0.00, -0.24, 0.00, 0.00, -0.24, -0.20, 0.00, 0.00,
        0.24, 0.20, -0.24, -0.20, 0.00, 0.00, -0.24, 0.20, 0.16, -0.20, 0.12, 0.10, 0.20, 0.20, 0.00, -0.10, -0.12,
        0.10, -0.16, -0.20, -0.12, -0.10, -0.16, 0.20, 0.20, 0.20, 0.00, 0.00, -0.20, 0.20, -0.20, 0.20, -0.20, 0.20,
        -0.20, 0.20, 0.20, -0.20, -0.20, -0.20, 0.00, 0.00, -0.20, 0.20, 0.20, 0.00, -0.20, 0.00, 0.00, -0.20, 0.20,
        -0.20, 0.20, -0.20, -0.20, -0.20, -0.20, 0.00, 0.00, 0.20, 0.20, 0.20, 0.20, 0.12, -0.20, -0.12, -0.10, 0.28,
        -0.28, 0.16, -0.20, 0.00, -0.10, 0.00, 0.10, -0.16, 0.20, 0.00, -0.10, -0.16, -0.20, 0.00, -0.10, 0.16, -0.20,
        0.16, -0.20, 0.00, 0.00, 0.16, 0.20, -0.16, 0.20, 0.00, 0.00, 0.16, 0.20, 0.16, -0.20, 0.16, -0.20, -0.16, 0.20,
        0.16, -0.20, 0.00, 0.00, 0.16, 0.20, 0.00, 0.00, 0.16, 0.20, 0.00, 0.00, -0.16, -0.20, 0.16, -0.20, -0.16,
        -0.20, 0.00, 0.00, -0.16, -0.20, 0.00, 0.00, -0.16, 0.20, 0.00, 0.00, 0.16, -0.20, 0.16, 0.20, 0.16, 0.20, 0.00,
        0.00, -0.16, -0.20, 0.00, 0.00, -0.16, -0.20, 0.00, 0.00, 0.16, 0.20, 0.16, 0.20, 0.00, 0.00, 0.16, 0.20, 0.16,
        -0.20, 0.16, 0.20, 0.00, 0.00, -0.16, 0.20, 0.00, 0.10, 0.12, -0.20, 0.12, -0.20, 0.00, -0.10, 0.00, -0.10,
        0.12, 0.20, 0.00, -0.10, -0.12, 0.20, -0.15, 0.20, -0.24, 0.24, 0.00, 0.00, 0.24, 0.24, 0.12, -0.20, -0.12,
        -0.20, 0.00, 0.00, 0.12, 0.20, 0.12, -0.20, 0.12, 0.20, 0.12, 0.20, 0.12, 0.20, 0.12, -0.20, -0.12, 0.20, 0.00,
        0.00, 0.12, 0.20, 0.12, 0.00, -0.20, 0.00, 0.00, -0.12, -0.20, 0.12, -0.20, 0.00, 0.00, 0.12, 0.20, -0.12, 0.20,
        -0.12, 0.20, 0.12, -0.20, 0.00, 0.00, 0.12, 0.20, 0.20, 0.00, 0.12, 0.00, 0.00, -0.12, 0.20, 0.00, 0.00, -0.12,
        -0.20, 0.00, 0.00, -0.12, -0.20, -0.12, -0.20, 0.00, 0.00, 0.12, -0.20, 0.12, -0.20, 0.12, 0.20, -0.12, -0.20,
        0.00, 0.00, 0.12, -0.20, 0.12, -0.20, 0.12,
        /*4500-4755 */
        0.20, 0.12, 0.00, 0.20, -0.12, -0.20, 0.00, 0.00, 0.12, 0.20, -0.16, 0.00, 0.16, -0.20, 0.20, 0.00, 0.00, -0.20,
        0.00, 0.00, -0.20, 0.20, 0.00, 0.00, 0.20, 0.20, -0.20, 0.00, 0.00, -0.20, 0.12, 0.00, -0.16, 0.20, 0.00, 0.00,
        0.20, 0.12, -0.10, 0.00, 0.10, 0.16, -0.16, -0.16, -0.16, -0.16, -0.16, 0.00, 0.00, -0.16, 0.00, 0.00, -0.16,
        -0.16, -0.16, 0.00, 0.00, -0.16, 0.00, 0.00, 0.16, 0.00, 0.00, 0.16, 0.00, 0.00, 0.16, 0.16, 0.00, 0.00, -0.16,
        0.00, 0.00, -0.16, -0.16, 0.00, 0.00, 0.16, 0.00, 0.00, -0.16, -0.16, 0.00, 0.00, -0.16, -0.16, 0.12, 0.10,
        0.12, -0.10, 0.12, 0.10, 0.00, 0.00, 0.12, 0.10, -0.12, 0.10, 0.00, 0.00, 0.12, 0.10, 0.12, -0.10, 0.00, 0.00,
        -0.12, -0.10, 0.00, 0.00, 0.12, 0.10, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, -0.12, 0.00, 0.00, 0.12, 0.12, 0.12,
        0.12, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, 0.12, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, 0.12, -0.12, -0.12, 0.12,
        0.12, -0.12, -0.12, 0.00, 0.00, 0.12, -0.12, 0.12, 0.12, -0.12, -0.12, 0.00, 0.00, -0.12, -0.12, 0.00, 0.00,
        -0.12, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, 0.12, -0.12, 0.00, 0.00, -0.12, 0.12, -0.12, -0.12,
        0.12, 0.00, 0.00, 0.12, 0.12, 0.12, -0.12, 0.00, 0.00, -0.12, -0.12, -0.12, 0.00, 0.00, -0.12, -0.12, 0.00,
        0.00, 0.12, 0.12, 0.00, 0.00, -0.12, -0.12, -0.12, -0.12, 0.12, 0.00, 0.00, 0.12, -0.12, 0.00, 0.00, -0.12,
        -0.12, 0.00, 0.00, 0.12, -0.12, -0.12, -0.12, -0.12, 0.12, 0.12, -0.12, -0.12, 0.00, 0.00, -0.12, 0.00, 0.00,
        -0.12, 0.12, 0.00, 0.00, 0.12, 0.00, 0.00, -0.12, -0.12, 0.00, 0.00, -0.12, -0.12, 0.12, 0.00, 0.00, 0.12, 0.12,
        0.00, 0.00, 0.12, 0.00, 0.00, 0.12, 0.12, 0.08, 0.00, 0.04};

    /* Amplitude usage: X or Y, sin or cos, power of T. */
    int jaxy[20] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    int jasc[20] = {0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0};
    int japt[20] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4};

    /* Miscellaneous */
    double t, w, pt[6], fa[14], xypr[2], xypl[2], xyls[2], arg, sc[2];
    int jpt, i, j, jxy, ialast, ifreq, m, ia, jsc;

    /* ------------------------------------------------------------------ */

    /* Interval between fundamental date J2000.0 and given date (JC). */
    t = ((date1 - dJ0) + date2) / dJC;

    /* Powers of T. */
    w = 1.0;
    for (jpt = 0; jpt <= MAXPT; jpt++)
    {
        pt[jpt] = w;
        w *= t;
    }

    /* Initialize totals in X and Y:  polynomial, luni-solar, planetary. */
    for (jxy = 0; jxy < 2; jxy++)
    {
        xypr[jxy] = 0.0;
        xyls[jxy] = 0.0;
        xypl[jxy] = 0.0;
    }

    /* --------------------------------- */
    /* Fundamental arguments (IERS 2003) */
    /* --------------------------------- */

    /* Mean anomaly of the Moon. */
    fa[0] = _iauFal03(t);
    /* Mean anomaly of the Sun. */
    fa[1] = _iauFalp03(t);
    /* Mean argument of the latitude of the Moon. */
    fa[2] = _iauFaf03(t);
    /* Mean elongation of the Moon from the Sun. */
    fa[3] = _iauFad03(t);
    /* Mean longitude of the ascending node of the Moon. */
    fa[4] = _iauFaom03(t);
    /* Planetary longitudes, Mercury through Neptune. */
    fa[5] = _iauFame03(t);
    fa[6] = _iauFave03(t);
    fa[7] = _iauFae03(t);
    fa[8] = _iauFama03(t);
    fa[9] = _iauFaju03(t);
    fa[10] = _iauFasa03(t);
    fa[11] = _iauFaur03(t);
    fa[12] = _iauFane03(t);
    /* General accumulated precession in longitude. */
    fa[13] = _iauFapa03(t);

    /* -------------------------------------- */
    /* Polynomial part of precession-nutation */
    /* -------------------------------------- */

    for (jxy = 0; jxy < 2; jxy++)
    {
        for (j = MAXPT; j >= 0; j--)
        {
            xypr[jxy] += xyp[jxy][j] * pt[j];
        }
    }

    /* ---------------------------------- */
    /* Nutation periodic terms, planetary */
    /* ---------------------------------- */

    /* Work backwards through the coefficients per frequency list. */
    ialast = NA;
    for (ifreq = NFPL - 1; ifreq >= 0; ifreq--)
    {

        /* Obtain the argument functions. */
        arg = 0.0;
        for (i = 0; i < 14; i++)
        {
            m = mfapl[ifreq][i];
            if (m != 0)
                arg += (double)m * fa[i];
        }
        sc[0] = sin(arg);
        sc[1] = cos(arg);

        /* Work backwards through the amplitudes at this frequency. */
        ia = nc[ifreq + NFLS];
        for (i = ialast; i >= ia; i--)
        {

            /* Coefficient number (0 = 1st). */
            j = i - ia;

            /* X or Y. */
            jxy = jaxy[j];

            /* Sin or cos. */
            jsc = jasc[j];

            /* Power of T. */
            jpt = japt[j];

            /* Accumulate the component. */
            xypl[jxy] += a[i - 1] * sc[jsc] * pt[jpt];
        }
        ialast = ia - 1;
    }

    /* ----------------------------------- */
    /* Nutation periodic terms, luni-solar */
    /* ----------------------------------- */

    /* Continue working backwards through the number of coefficients list. */
    for (ifreq = NFLS - 1; ifreq >= 0; ifreq--)
    {

        /* Obtain the argument functions. */
        arg = 0.0;
        for (i = 0; i < 5; i++)
        {
            m = mfals[ifreq][i];
            if (m != 0)
                arg += (double)m * fa[i];
        }
        sc[0] = sin(arg);
        sc[1] = cos(arg);

        /* Work backwards through the amplitudes at this frequency. */
        ia = nc[ifreq];
        for (i = ialast; i >= ia; i--)
        {

            /* Coefficient number (0 = 1st). */
            j = i - ia;

            /* X or Y. */
            jxy = jaxy[j];

            /* Sin or cos. */
            jsc = jasc[j];

            /* Power of T. */
            jpt = japt[j];

            /* Accumulate the component. */
            xyls[jxy] += a[i - 1] * sc[jsc] * pt[jpt];
        }
        ialast = ia - 1;
    }

    /* ------------------------------------ */
    /* Results:  CIP unit vector components */
    /* ------------------------------------ */

    *x = dDAS2R * (xypr[0] + (xyls[0] + xypl[0]) / 1e6);
    *y = dDAS2R * (xypr[1] + (xyls[1] + xypl[1]) / 1e6);

    return;
}

double t_gtrs2crs::_sp2000(const double &dDATE1, const double &dDATE2)
{

    double dT = ((dDATE1 - dJ0) + dDATE2) / dJC;
    double dTemp = (-47) * 1.0e-6 * dT * dAS2R;
    return dTemp;
}

double t_gtrs2crs::_era2000(const double &dJ1, const double &dJ2)
{
    double d1 = (dJ1 < dJ2) ? dJ1 : dJ2;
    double d2 = (dJ1 < dJ2) ? dJ2 : dJ1;
    double dT = d1 + (d2 - dJ0);
    double dF = fmod((d1 + 0.5), 1.0) + fmod(d2, 1.0);
    return _iau_anp(d2PI * (dF + 0.7790572732640 + 0.00273781191135448 * dT));
}

double t_gtrs2crs::_iau_anp(const double &dA)
{
    double dW = fmod(dA, d2PI);
    if (dW < 0)
    {
        dW = dW + d2PI;
    }
    return dW;
}

double t_gtrs2crs::_gst2000(const double &dRmjd, const double &era, const double &dpsi)
{
    double dT = (dRmjd - 51544.5) / 36525.0;
    _gmst =
        era + (0.014506 + (4612.15739966 + (1.39667721 + (-0.00009344 + 0.00001882 * dT) * dT) * dT) * dT) / RAD2SEC;
    double dTemp = _gmst + dpsi * cos(_epsa) + _eect2000(dRmjd);
    return dTemp;
}

double t_gtrs2crs::_gst2006(const double &dRmjd, const double &era, const double &dpsi)
{
    double dT = (dRmjd - 51544.5) / 36525.0;
    _gmst = era +
            (0.014506 +
             (4612.156534 + (1.3915817 + (-0.00000044 + (-0.000029956 + (-0.0000000368) * dT) * dT) * dT) * dT) * dT) /
                RAD2SEC;
    double dTemp = _gmst + dpsi * cos(_epsa) + _eect2000(dRmjd);
    return dTemp;
}

void t_gtrs2crs::_iau_bpn2xy(Matrix qmat, double *X, double *Y)
{
    *X = qmat(3, 1);
    *Y = qmat(3, 2);
}

double t_gtrs2crs::_iau_anpm(const double &dA)
{
    double dW = fmod(dA, d2PI);
    if (fabs(dW) >= dPI)
    {
        if (dA > 0)
        {
            dW = dW - d2PI;
        }
        else
        {
            dW = dW + d2PI;
        }
    }

    return dW;
}

double t_gtrs2crs::_eect2000(const double &dRmjd)
{
    //         *  Time since J2000, in Julian centuries
    double dT;
    //         *  Miscellaneous
    int I;
    int J;
    double dA;
    double dS0;
    double dS1;
    //         *  Fundamental arguments
    double dFA[14];
    //         *  -----------------------------------------
    //         *  The series for the EE complementary terms
    //         *  -----------------------------------------
    //
    //         *  Number of terms in the series
    //               INTEGER NE0, NE1
    //               PARAMETER ( NE0=  33, NE1=  1 )
    int const iNE0 = 33;
    int const iNE1 = 1;

    //         *  Coefficients of l,l',F,D,Om,LMe,LVe,LE,LMa,LJu,LSa,LU,LN,pA
    //               INTEGER KE0 ( 14, NE0 ),
    //              :        KE1 ( 14, NE1 )
    int iKE0[14][iNE0];

    //
    //         *  Sine and cosine coefficients
    //               DOUBLE PRECISION SE0 ( 2, NE0 ),
    //              :                 SE1 ( 2, NE1 )
    double dSE0[2][iNE0];

    //         *  Argument coefficients for t^0
    int iTemp1[10][14] = {0, 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  2,  0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 2, -2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 1,  0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 2, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,  3,  0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 2, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  3,  0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,  -1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 14; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            iKE0[i][j] = iTemp1[j][i];
        }
    }

    int iTemp2[10][14] = {1, 0, 0, 0,  -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  1, 0, 0,  0,  0, 0, 0, 0, 0, 0,
                          0, 1, 2, -2, 3,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -2, 1, 0, 0,  0,  0, 0, 0, 0, 0, 0,
                          0, 0, 4, -4, 4,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0,
                          0, 0, 2, 0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,  2, 0, 0,  0,  0, 0, 0, 0, 0, 0,
                          1, 0, 2, 0,  3,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0,  1, 0, 0,  0,  0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 14; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            iKE0[i][j + 10] = iTemp2[j][i];
        }
    }

    int iTemp3[10][14] = {0, 0, 2,  -2, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, 2,  -3, 0, 0, 0,   0, 0, 0, 0, 0, 0,
                          0, 1, -2, 2,  -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 8, -13, 0, 0, 0, 0, 0, -1,
                          0, 0, 0,  2,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0,  -1, 0, 0, 0,   0, 0, 0, 0, 0, 0,
                          1, 0, 0,  -2, 1,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2,  -2, 2,  0, 0, 0,   0, 0, 0, 0, 0, 0,
                          1, 0, 0,  -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,  -2, 4,  0, 0, 0,   0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 14; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            iKE0[i][j + 20] = iTemp3[j][i];
        }
    }

    int iTemp4[3][14] = {0, 0, 2, -2, 4, 0, 0, 0, 0, 0,  0, 0,  0, 0, 1, 0, -2, 0, -3, 0, 0,
                         0, 0, 0, 0,  0, 0, 0, 1, 0, -2, 0, -1, 0, 0, 0, 0, 0,  0, 0,  0, 0};
    for (int i = 0; i < 14; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            iKE0[i][j + 30] = iTemp4[j][i];
        }
    }

    //         *  Argument coefficients for t^1
    int iKE1[14][iNE1] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //
    //         *  Sine and cosine coefficients for t^0
    double dTemp1[10][2] = {

        +2640.96e-6, -0.39e-6, +63.52e-6, -0.02e-6, +11.75e-6, +0.01e-6, +11.21e-6, +0.01e-6, -4.55e-6, +0.00e-6,
        +2.02e-6,    +0.00e-6, +1.98e-6,  +0.00e-6, -1.72e-6,  +0.00e-6, -1.41e-6,  -0.01e-6, -1.26e-6, -0.01e-6};

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            dSE0[i][j] = dTemp1[j][i];
        }
    }

    double dTemp2[10][2] = {

        -0.63e-6, +0.00e-6, -0.63e-6, +0.00e-6, +0.46e-6, +0.00e-6, +0.45e-6, +0.00e-6, +0.36e-6, +0.00e-6,
        -0.24e-6, -0.12e-6, +0.32e-6, +0.00e-6, +0.28e-6, +0.00e-6, +0.27e-6, +0.00e-6, +0.26e-6, +0.00e-6};

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            dSE0[i][j + 10] = dTemp2[j][i];
        }
    }

    double dTemp3[10][2] = {

        -0.21e-6, +0.00e-6, +0.19e-6, +0.00e-6, +0.18e-6, +0.00e-6, -0.10e-6, +0.05e-6, +0.15e-6, +0.00e-6,
        -0.14e-6, +0.00e-6, +0.14e-6, +0.00e-6, -0.14e-6, +0.00e-6, +0.14e-6, +0.00e-6, +0.13e-6, +0.00e-6};

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            dSE0[i][j + 20] = dTemp3[j][i];
        }
    }

    double dTemp4[3][2] = {

        -0.11e-6, +0.00e-6, +0.11e-6, +0.00e-6, +0.11e-6, +0.00e-6};
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            dSE0[i][j + 30] = dTemp4[j][i];
        }
    }
    //         *  Sine and cosine coefficients for t^1
    double dSE1[2][iNE1] = {-0.87 * pow(10, -6), +0.00 * pow(10, -6)};

    //         * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //
    //         *  Interval between fundamental epoch J2000.0 and current date (JC).
    dT = (dRmjd - 51544.5) / dJC;
    //         *  Fundamental Arguments (from IERS Conventions 2000)
    //
    //         *  Mean Anomaly of the Moon.

    dFA[0] =
        _iau_anpm((485868.249036 + (715923.2178 + (31.8792 + (0.051635 + (-0.00024470) * dT) * dT) * dT) * dT) * dAS2R +
                  fmod(1325 * dT, 1.0) * d2PI);
    //         *  Mean Anomaly of the Sun.
    dFA[1] = _iau_anpm((1287104.793048 + (1292581.0481 + (-0.5532 + (+0.000136 + (-0.00001149) * dT) * dT) * dT) * dT) *
                           dAS2R +
                       fmod(99 * dT, 1.0) * d2PI);
    //         *  Mean Longitude of the Moon minus Mean Longitude of the Ascending
    //         *  Node of the Moon.

    dFA[2] = _iau_anpm((335779.526232 + (295262.8478 + (-12.7512 + (-0.001037 + (0.00000417) * dT) * dT) * dT) * dT) *
                           dAS2R +
                       fmod(1342 * dT, 1.0) * d2PI);

    dFA[3] = _iau_anpm((1072260.703692 + (1105601.2090 + (-6.3706 + (0.006593 + (-0.00003169) * dT) * dT) * dT) * dT) *
                           dAS2R +
                       fmod(1236 * dT, 1.0) * d2PI);

    //         *  Mean Longitude of the Ascending Node of the Moon.

    dFA[4] =
        _iau_anpm((450160.398036 + (-482890.5431 + (7.4722 + (0.007702 + (-0.00005939) * dT) * dT) * dT) * dT) * dAS2R +
                  fmod(-5 * dT, 1.0) * d2PI);

    dFA[5] = _iau_anpm(4.402608842 + 2608.7903141574 * dT);
    dFA[6] = _iau_anpm(3.176146697 + 1021.3285546211 * dT);
    dFA[7] = _iau_anpm(1.753470314 + 628.3075849991 * dT);
    dFA[8] = _iau_anpm(6.203480913 + 334.0612426700 * dT);
    dFA[9] = _iau_anpm(0.599546497 + 52.9690962641 * dT);
    dFA[10] = _iau_anpm(0.874016757 + 21.3299104960 * dT);
    dFA[11] = _iau_anpm(5.481293872 + 7.4781598567 * dT);
    dFA[12] = _iau_anpm(5.311886287 + 3.8133035638 * dT);
    dFA[13] = (0.024381750 + 0.00000538691 * dT) * dT;

    //
    //         *  Evaluate the EE complementary terms.

    dS0 = 0.0;
    dS1 = 0.0;

    for (I = iNE0 - 1; I >= 0; I--)
    {
        dA = 0.0;
        for (J = 0; J < 14; J++)
        {
            dA = dA + iKE0[J][I] * dFA[J];
        }
        dS0 = dS0 + (dSE0[0][I] * sin(dA) + dSE0[1][I] * cos(dA));
    }

    for (I = iNE1 - 1; I >= 0; I--)
    {
        dA = 0.0;
        for (J = 0; J < 14; J++)
        {
            dA = dA + iKE1[J][I] * dFA[J];
        }
        dS1 = dS1 + (dSE1[0][I] * sin(dA) + dSE1[1][I] * cos(dA));
    }

    return (dS0 + dS1 * dT) * dAS2R;
}

void t_gtrs2crs::_nutCal(const double &dATE1, const double &dATE2, double *dPSI, double *dEPS)
{
    double dT, dEL, dELP, dF, dD, dOM, dARG, dDP, dDE, dSARG, dCARG, dDPSILS, dDEPSLS, dAL, dALSU, dAF, dAD, dAOM,
        dALME, dALVE, dALEA, dALMA, dALJU, dALSA, dALUR, dALNE, dAPA, dDPSIPL, dDEPSPL;
    int I, J;

    /*** Luni-Solar nutation model***/

    const int iNLS = 678;
    // int i;
    int(*iNALS)[iNLS] = new int[5][iNLS];
    double(*dCLS)[iNLS] = new double[6][iNLS];

    /*** Planetary terms***/

    const int iNPL = 687;
    int(*iNAPL)[iNPL] = new int[14][iNPL];
    int(*iICPL)[iNPL] = new int[4][iNPL];

    /***Tables of argument and term coefficients***/
    /*  Luni-Solar argument multipliers:
        L     L'    F     D     Om  */
    // NALST is the transposition of NALS
    int iNALST[iNLS][5] = {
        {0, 0, 0, 0, 1},    {0, 0, 2, -2, 2},   {0, 0, 2, 0, 2},    {0, 0, 0, 0, 2},    {0, 1, 0, 0, 0},
        {0, 1, 2, -2, 2},   {1, 0, 0, 0, 0},    {0, 0, 2, 0, 1},    {1, 0, 2, 0, 2},    {0, -1, 2, -2, 2},
        {0, 0, 2, -2, 1},   {-1, 0, 2, 0, 2},   {-1, 0, 0, 2, 0},   {1, 0, 0, 0, 1},    {-1, 0, 0, 0, 1},
        {-1, 0, 2, 2, 2},   {1, 0, 2, 0, 1},    {-2, 0, 2, 0, 1},   {0, 0, 0, 2, 0},    {0, 0, 2, 2, 2},
        {0, -2, 2, -2, 2},  {-2, 0, 0, 2, 0},   {2, 0, 2, 0, 2},    {1, 0, 2, -2, 2},   {-1, 0, 2, 0, 1},
        {2, 0, 0, 0, 0},    {0, 0, 2, 0, 0},    {0, 1, 0, 0, 1},    {-1, 0, 0, 2, 1},   {0, 2, 2, -2, 2},
        {0, 0, -2, 2, 0},   {1, 0, 0, -2, 1},   {0, -1, 0, 0, 1},   {-1, 0, 2, 2, 1},   {0, 2, 0, 0, 0},
        {1, 0, 2, 2, 2},    {-2, 0, 2, 0, 0},   {0, 1, 2, 0, 2},    {0, 0, 2, 2, 1},    {0, -1, 2, 0, 2},
        {0, 0, 0, 2, 1},    {1, 0, 2, -2, 1},   {2, 0, 2, -2, 2},   {-2, 0, 0, 2, 1},   {2, 0, 2, 0, 1},
        {0, -1, 2, -2, 1},  {0, 0, 0, -2, 1},   {-1, -1, 0, 2, 0},  {2, 0, 0, -2, 1},   {1, 0, 0, 2, 0},
        {0, 1, 2, -2, 1},   {1, -1, 0, 0, 0},   {-2, 0, 2, 0, 2},   {3, 0, 2, 0, 2},    {0, -1, 0, 2, 0},
        {1, -1, 2, 0, 2},   {0, 0, 0, 1, 0},    {-1, -1, 2, 2, 2},  {-1, 0, 2, 0, 0},   {0, -1, 2, 2, 2},
        {-2, 0, 0, 0, 1},   {1, 1, 2, 0, 2},    {2, 0, 0, 0, 1},    {-1, 1, 0, 1, 0},   {1, 1, 0, 0, 0},
        {1, 0, 2, 0, 0},    {-1, 0, 2, -2, 1},  {1, 0, 0, 0, 2},    {-1, 0, 0, 1, 0},   {0, 0, 2, 1, 2},
        {-1, 0, 2, 4, 2},   {-1, 1, 0, 1, 1},   {0, -2, 2, -2, 1},  {1, 0, 2, 2, 1},    {-2, 0, 2, 2, 2},
        {-1, 0, 0, 0, 2},   {1, 1, 2, -2, 2},   {-2, 0, 2, 4, 2},   {-1, 0, 4, 0, 2},   {2, 0, 2, -2, 1},
        {2, 0, 2, 2, 2},    {1, 0, 0, 2, 1},    {3, 0, 0, 0, 0},    {3, 0, 2, -2, 2},   {0, 0, 4, -2, 2},
        {0, 1, 2, 0, 1},    {0, 0, -2, 2, 1},   {0, 0, 2, -2, 3},   {-1, 0, 0, 4, 0},   {2, 0, -2, 0, 1},
        {-2, 0, 0, 4, 0},   {-1, -1, 0, 2, 1},  {-1, 0, 0, 1, 1},   {0, 1, 0, 0, 2},    {0, 0, -2, 0, 1},
        {0, -1, 2, 0, 1},   {0, 0, 2, -1, 2},   {0, 0, 2, 4, 2},    {-2, -1, 0, 2, 0},  {1, 1, 0, -2, 1},
        {-1, 1, 0, 2, 0},   {-1, 1, 0, 1, 2},   {1, -1, 0, 0, 1},   {1, -1, 2, 2, 2},   {-1, 1, 2, 2, 2},
        {3, 0, 2, 0, 1},    {0, 1, -2, 2, 0},   {-1, 0, 0, -2, 1},  {0, 1, 2, 2, 2},    {-1, -1, 2, 2, 1},
        {0, -1, 0, 0, 2},   {1, 0, 2, -4, 1},   {-1, 0, -2, 2, 0},  {0, -1, 2, 2, 1},   {2, -1, 2, 0, 2},
        {0, 0, 0, 2, 2},    {1, -1, 2, 0, 1},   {-1, 1, 2, 0, 2},   {0, 1, 0, 2, 0},    {0, -1, -2, 2, 0},
        {0, 3, 2, -2, 2},   {0, 0, 0, 1, 1},    {-1, 0, 2, 2, 0},   {2, 1, 2, 0, 2},    {1, 1, 0, 0, 1},
        {1, 1, 2, 0, 1},    {2, 0, 0, 2, 0},    {1, 0, -2, 2, 0},   {-1, 0, 0, 2, 2},   {0, 1, 0, 1, 0},
        {0, 1, 0, -2, 1},   {-1, 0, 2, -2, 2},  {0, 0, 0, -1, 1},   {-1, 1, 0, 0, 1},   {1, 0, 2, -1, 2},
        {1, -1, 0, 2, 0},   {0, 0, 0, 4, 0},    {1, 0, 2, 1, 2},    {0, 0, 2, 1, 1},    {1, 0, 0, -2, 2},
        {-1, 0, 2, 4, 1},   {1, 0, -2, 0, 1},   {1, 1, 2, -2, 1},   {0, 0, 2, 2, 0},    {-1, 0, 2, -1, 1},
        {-2, 0, 2, 2, 1},   {4, 0, 2, 0, 2},    {2, -1, 0, 0, 0},   {2, 1, 2, -2, 2},   {0, 1, 2, 1, 2},
        {1, 0, 4, -2, 2},   {-1, -1, 0, 0, 1},  {0, 1, 0, 2, 1},    {-2, 0, 2, 4, 1},   {2, 0, 2, 0, 0},
        {1, 0, 0, 1, 0},    {-1, 0, 0, 4, 1},   {-1, 0, 4, 0, 1},   {2, 0, 2, 2, 1},    {0, 0, 2, -3, 2},
        {-1, -2, 0, 2, 0},  {2, 1, 0, 0, 0},    {0, 0, 4, 0, 2},    {0, 0, 0, 0, 3},    {0, 3, 0, 0, 0},
        {0, 0, 2, -4, 1},   {0, -1, 0, 2, 1},   {0, 0, 0, 4, 1},    {-1, -1, 2, 4, 2},  {1, 0, 2, 4, 2},
        {-2, 2, 0, 2, 0},   {-2, -1, 2, 0, 1},  {-2, 0, 0, 2, 2},   {-1, -1, 2, 0, 2},  {0, 0, 4, -2, 1},
        {3, 0, 2, -2, 1},   {-2, -1, 0, 2, 1},  {1, 0, 0, -1, 1},   {0, -2, 0, 2, 0},   {-2, 0, 0, 4, 1},
        {-3, 0, 0, 0, 1},   {1, 1, 2, 2, 2},    {0, 0, 2, 4, 1},    {3, 0, 2, 2, 2},    {-1, 1, 2, -2, 1},
        {2, 0, 0, -4, 1},   {0, 0, 0, -2, 2},   {2, 0, 2, -4, 1},   {-1, 1, 0, 2, 1},   {0, 0, 2, -1, 1},
        {0, -2, 2, 2, 2},   {2, 0, 0, 2, 1},    {4, 0, 2, -2, 2},   {2, 0, 0, -2, 2},   {0, 2, 0, 0, 1},
        {1, 0, 0, -4, 1},   {0, 2, 2, -2, 1},   {-3, 0, 0, 4, 0},   {-1, 1, 2, 0, 1},   {-1, -1, 0, 4, 0},
        {-1, -2, 2, 2, 2},  {-2, -1, 2, 4, 2},  {1, -1, 2, 2, 1},   {-2, 1, 0, 2, 0},   {-2, 1, 2, 0, 1},
        {2, 1, 0, -2, 1},   {-3, 0, 2, 0, 1},   {-2, 0, 2, -2, 1},  {-1, 1, 0, 2, 2},   {0, -1, 2, -1, 2},
        {-1, 0, 4, -2, 2},  {0, -2, 2, 0, 2},   {-1, 0, 2, 1, 2},   {2, 0, 0, 0, 2},    {0, 0, 2, 0, 3},
        {-2, 0, 4, 0, 2},   {-1, 0, -2, 0, 1},  {-1, 1, 2, 2, 1},   {3, 0, 0, 0, 1},    {-1, 0, 2, 3, 2},
        {2, -1, 2, 0, 1},   {0, 1, 2, 2, 1},    {0, -1, 2, 4, 2},   {2, -1, 2, 2, 2},   {0, 2, -2, 2, 0},
        {-1, -1, 2, -1, 1}, {0, -2, 0, 0, 1},   {1, 0, 2, -4, 2},   {1, -1, 0, -2, 1},  {-1, -1, 2, 0, 1},
        {1, -1, 2, -2, 2},  {-2, -1, 0, 4, 0},  {-1, 0, 0, 3, 0},   {-2, -1, 2, 2, 2},  {0, 2, 2, 0, 2},
        {1, 1, 0, 2, 0},    {2, 0, 2, -1, 2},   {1, 0, 2, 1, 1},    {4, 0, 0, 0, 0},    {2, 1, 2, 0, 1},
        {3, -1, 2, 0, 2},   {-2, 2, 0, 2, 1},   {1, 0, 2, -3, 1},   {1, 1, 2, -4, 1},   {-1, -1, 2, -2, 1},
        {0, -1, 0, -1, 1},  {0, -1, 0, -2, 1},  {-2, 0, 0, 0, 2},   {-2, 0, -2, 2, 0},  {-1, 0, -2, 4, 0},
        {1, -2, 0, 0, 0},   {0, 1, 0, 1, 1},    {-1, 2, 0, 2, 0},   {1, -1, 2, -2, 1},  {1, 2, 2, -2, 2},
        {2, -1, 2, -2, 2},  {1, 0, 2, -1, 1},   {2, 1, 2, -2, 1},   {-2, 0, 0, -2, 1},  {1, -2, 2, 0, 2},
        {0, 1, 2, 1, 1},    {1, 0, 4, -2, 1},   {-2, 0, 4, 2, 2},   {1, 1, 2, 1, 2},    {1, 0, 0, 4, 0},
        {1, 0, 2, 2, 0},    {2, 0, 2, 1, 2},    {3, 1, 2, 0, 2},    {4, 0, 2, 0, 1},    {-2, -1, 2, 0, 0},
        {0, 1, -2, 2, 1},   {1, 0, -2, 1, 0},   {0, -1, -2, 2, 1},  {2, -1, 0, -2, 1},  {-1, 0, 2, -1, 2},
        {1, 0, 2, -3, 2},   {0, 1, 2, -2, 3},   {0, 0, 2, -3, 1},   {-1, 0, -2, 2, 1},  {0, 0, 2, -4, 2},
        {-2, 1, 0, 0, 1},   {-1, 0, 0, -1, 1},  {2, 0, 2, -4, 2},   {0, 0, 4, -4, 4},   {0, 0, 4, -4, 2},
        {-1, -2, 0, 2, 1},  {-2, 0, 0, 3, 0},   {1, 0, -2, 2, 1},   {-3, 0, 2, 2, 2},   {-3, 0, 2, 2, 1},
        {-2, 0, 2, 2, 0},   {2, -1, 0, 0, 1},   {-2, 1, 2, 2, 2},   {1, 1, 0, 1, 0},    {0, 1, 4, -2, 2},
        {-1, 1, 0, -2, 1},  {0, 0, 0, -4, 1},   {1, -1, 0, 2, 1},   {1, 1, 0, 2, 1},    {-1, 2, 2, 2, 2},
        {3, 1, 2, -2, 2},   {0, -1, 0, 4, 0},   {2, -1, 0, 2, 0},   {0, 0, 4, 0, 1},    {2, 0, 4, -2, 2},
        {-1, -1, 2, 4, 1},  {1, 0, 0, 4, 1},    {1, -2, 2, 2, 2},   {0, 0, 2, 3, 2},    {-1, 1, 2, 4, 2},
        {3, 0, 0, 2, 0},    {-1, 0, 4, 2, 2},   {1, 1, 2, 2, 1},    {-2, 0, 2, 6, 2},   {2, 1, 2, 2, 2},
        {-1, 0, 2, 6, 2},   {1, 0, 2, 4, 1},    {2, 0, 2, 4, 2},    {1, 1, -2, 1, 0},   {-3, 1, 2, 1, 2},
        {2, 0, -2, 0, 2},   {-1, 0, 0, 1, 2},   {-4, 0, 2, 2, 1},   {-1, -1, 0, 1, 0},  {0, 0, -2, 2, 2},
        {1, 0, 0, -1, 2},   {0, -1, 2, -2, 3},  {-2, 1, 2, 0, 0},   {0, 0, 2, -2, 4},   {-2, -2, 0, 2, 0},
        {-2, 0, -2, 4, 0},  {0, -2, -2, 2, 0},  {1, 2, 0, -2, 1},   {3, 0, 0, -4, 1},   {-1, 1, 2, -2, 2},
        {1, -1, 2, -4, 1},  {1, 1, 0, -2, 2},   {-3, 0, 2, 0, 0},   {-3, 0, 2, 0, 2},   {-2, 0, 0, 1, 0},
        {0, 0, -2, 1, 0},   {-3, 0, 0, 2, 1},   {-1, -1, -2, 2, 0}, {0, 1, 2, -4, 1},   {2, 1, 0, -4, 1},
        {0, 2, 0, -2, 1},   {1, 0, 0, -3, 1},   {-2, 0, 2, -2, 2},  {-2, -1, 0, 0, 1},  {-4, 0, 0, 2, 0},
        {1, 1, 0, -4, 1},   {-1, 0, 2, -4, 1},  {0, 0, 4, -4, 1},   {0, 3, 2, -2, 2},   {-3, -1, 0, 4, 0},
        {-3, 0, 0, 4, 1},   {1, -1, -2, 2, 0},  {-1, -1, 0, 2, 2},  {1, -2, 0, 0, 1},   {1, -1, 0, 0, 2},
        {0, 0, 0, 1, 2},    {-1, -1, 2, 0, 0},  {1, -2, 2, -2, 2},  {0, -1, 2, -1, 1},  {-1, 0, 2, 0, 3},
        {1, 1, 0, 0, 2},    {-1, 1, 2, 0, 0},   {1, 2, 0, 0, 0},    {-1, 2, 2, 0, 2},   {-1, 0, 4, -2, 1},
        {3, 0, 2, -4, 2},   {1, 2, 2, -2, 1},   {1, 0, 4, -4, 2},   {-2, -1, 0, 4, 1},  {0, -1, 0, 2, 2},
        {-2, 1, 0, 4, 0},   {-2, -1, 2, 2, 1},  {2, 0, -2, 2, 0},   {1, 0, 0, 1, 1},    {0, 1, 0, 2, 2},
        {1, -1, 2, -1, 2},  {-2, 0, 4, 0, 1},   {2, 1, 0, 0, 1},    {0, 1, 2, 0, 0},    {0, -1, 4, -2, 2},
        {0, 0, 4, -2, 4},   {0, 2, 2, 0, 1},    {-3, 0, 0, 6, 0},   {-1, -1, 0, 4, 1},  {1, -2, 0, 2, 0},
        {-1, 0, 0, 4, 2},   {-1, -2, 2, 2, 1},  {-1, 0, 0, -2, 2},  {1, 0, -2, -2, 1},  {0, 0, -2, -2, 1},
        {-2, 0, -2, 0, 1},  {0, 0, 0, 3, 1},    {0, 0, 0, 3, 0},    {-1, 1, 0, 4, 0},   {-1, -1, 2, 2, 0},
        {-2, 0, 2, 3, 2},   {1, 0, 0, 2, 2},    {0, -1, 2, 1, 2},   {3, -1, 0, 0, 0},   {2, 0, 0, 1, 0},
        {1, -1, 2, 0, 0},   {0, 0, 2, 1, 0},    {1, 0, 2, 0, 3},    {3, 1, 0, 0, 0},    {3, -1, 2, -2, 2},
        {2, 0, 2, -1, 1},   {1, 1, 2, 0, 0},    {0, 0, 4, -1, 2},   {1, 2, 2, 0, 2},    {-2, 0, 0, 6, 0},
        {0, -1, 0, 4, 1},   {-2, -1, 2, 4, 1},  {0, -2, 2, 2, 1},   {0, -1, 2, 2, 0},   {-1, 0, 2, 3, 1},
        {-2, 1, 2, 4, 2},   {2, 0, 0, 2, 2},    {2, -2, 2, 0, 2},   {-1, 1, 2, 3, 2},   {3, 0, 2, -1, 2},
        {4, 0, 2, -2, 1},   {-1, 0, 0, 6, 0},   {-1, -2, 2, 4, 2},  {-3, 0, 2, 6, 2},   {-1, 0, 2, 4, 0},
        {3, 0, 0, 2, 1},    {3, -1, 2, 0, 1},   {3, 0, 2, 0, 0},    {1, 0, 4, 0, 2},    {5, 0, 2, -2, 2},
        {0, -1, 2, 4, 1},   {2, -1, 2, 2, 1},   {0, 1, 2, 4, 2},    {1, -1, 2, 4, 2},   {3, -1, 2, 2, 2},
        {3, 0, 2, 2, 1},    {5, 0, 2, 0, 2},    {0, 0, 2, 6, 2},    {4, 0, 2, 2, 2},    {0, -1, 1, -1, 1},
        {-1, 0, 1, 0, 3},   {0, -2, 2, -2, 3},  {1, 0, -1, 0, 1},   {2, -2, 0, -2, 1},  {-1, 0, 1, 0, 2},
        {-1, 0, 1, 0, 1},   {-1, -1, 2, -1, 2}, {-2, 2, 0, 2, 2},   {-1, 0, 1, 0, 0},   {-4, 1, 2, 2, 2},
        {-3, 0, 2, 1, 1},   {-2, -1, 2, 0, 2},  {1, 0, -2, 1, 1},   {2, -1, -2, 0, 1},  {-4, 0, 2, 2, 0},
        {-3, 1, 0, 3, 0},   {-1, 0, -1, 2, 0},  {0, -2, 0, 0, 2},   {0, -2, 0, 0, 2},   {-3, 0, 0, 3, 0},
        {-2, -1, 0, 2, 2},  {-1, 0, -2, 3, 0},  {-4, 0, 0, 4, 0},   {2, 1, -2, 0, 1},   {2, -1, 0, -2, 2},
        {0, 0, 1, -1, 0},   {-1, 2, 0, 1, 0},   {-2, 1, 2, 0, 2},   {1, 1, 0, -1, 1},   {1, 0, 1, -2, 1},
        {0, 2, 0, 0, 2},    {1, -1, 2, -3, 1},  {-1, 1, 2, -1, 1},  {-2, 0, 4, -2, 2},  {-2, 0, 4, -2, 1},
        {-2, -2, 0, 2, 1},  {-2, 0, -2, 4, 0},  {1, 2, 2, -4, 1},   {1, 1, 2, -4, 2},   {-1, 2, 2, -2, 1},
        {2, 0, 0, -3, 1},   {-1, 2, 0, 0, 1},   {0, 0, 0, -2, 0},   {-1, -1, 2, -2, 2}, {-1, 1, 0, 0, 2},
        {0, 0, 0, -1, 2},   {-2, 1, 0, 1, 0},   {1, -2, 0, -2, 1},  {1, 0, -2, 0, 2},   {-3, 1, 0, 2, 0},
        {-1, 1, -2, 2, 0},  {-1, -1, 0, 0, 2},  {-3, 0, 0, 2, 0},   {-3, -1, 0, 2, 0},  {2, 0, 2, -6, 1},
        {0, 1, 2, -4, 2},   {2, 0, 0, -4, 2},   {-2, 1, 2, -2, 1},  {0, -1, 2, -4, 1},  {0, 1, 0, -2, 2},
        {-1, 0, 0, -2, 0},  {2, 0, -2, -2, 1},  {-4, 0, 2, 0, 1},   {-1, -1, 0, -1, 1}, {0, 0, -2, 0, 2},
        {-3, 0, 0, 1, 0},   {-1, 0, -2, 1, 0},  {-2, 0, -2, 2, 1},  {0, 0, -4, 2, 0},   {-2, -1, -2, 2, 0},
        {1, 0, 2, -6, 1},   {-1, 0, 2, -4, 2},  {1, 0, 0, -4, 2},   {2, 1, 2, -4, 2},   {2, 1, 2, -4, 1},
        {0, 1, 4, -4, 4},   {0, 1, 4, -4, 2},   {-1, -1, -2, 4, 0}, {-1, -3, 0, 2, 0},  {-1, 0, -2, 4, 1},
        {-2, -1, 0, 3, 0},  {0, 0, -2, 3, 0},   {-2, 0, 0, 3, 1},   {0, -1, 0, 1, 0},   {-3, 0, 2, 2, 0},
        {1, 1, -2, 2, 0},   {-1, 1, 0, 2, 2},   {1, -2, 2, -2, 1},  {0, 0, 1, 0, 2},    {0, 0, 1, 0, 1},
        {0, 0, 1, 0, 0},    {-1, 2, 0, 2, 1},   {0, 0, 2, 0, 2},    {-2, 0, 2, 0, 2},   {2, 0, 0, -1, 1},
        {3, 0, 0, -2, 1},   {1, 0, 2, -2, 3},   {1, 2, 0, 0, 1},    {2, 0, 2, -3, 2},   {-1, 1, 4, -2, 2},
        {-2, -2, 0, 4, 0},  {0, -3, 0, 2, 0},   {0, 0, -2, 4, 0},   {-1, -1, 0, 3, 0},  {-2, 0, 0, 4, 2},
        {-1, 0, 0, 3, 1},   {2, -2, 0, 0, 0},   {1, -1, 0, 1, 0},   {-1, 0, 0, 2, 0},   {0, -2, 2, 0, 1},
        {-1, 0, 1, 2, 1},   {-1, 1, 0, 3, 0},   {-1, -1, 2, 1, 2},  {0, -1, 2, 0, 0},   {-2, 1, 2, 2, 1},
        {2, -2, 2, -2, 2},  {1, 1, 0, 1, 1},    {1, 0, 1, 0, 1},    {1, 0, 1, 0, 0},    {0, 2, 0, 2, 0},
        {2, -1, 2, -2, 1},  {0, -1, 4, -2, 1},  {0, 0, 4, -2, 3},   {0, 1, 4, -2, 1},   {4, 0, 2, -4, 2},
        {2, 2, 2, -2, 2},   {2, 0, 4, -4, 2},   {-1, -2, 0, 4, 0},  {-1, -3, 2, 2, 2},  {-3, 0, 2, 4, 2},
        {-3, 0, 2, -2, 1},  {-1, -1, 0, -2, 1}, {-3, 0, 0, 0, 2},   {-3, 0, -2, 2, 0},  {0, 1, 0, -4, 1},
        {-2, 1, 0, -2, 1},  {-4, 0, 0, 0, 1},   {-1, 0, 0, -4, 1},  {-3, 0, 0, -2, 1},  {0, 0, 0, 3, 2},
        {-1, 1, 0, 4, 1},   {1, -2, 2, 0, 1},   {0, 1, 0, 3, 0},    {-1, 0, 2, 2, 3},   {0, 0, 2, 2, 2},
        {-2, 0, 2, 2, 2},   {-1, 1, 2, 2, 0},   {3, 0, 0, 0, 2},    {2, 1, 0, 1, 0},    {2, -1, 2, -1, 2},
        {0, 0, 2, 0, 1},    {0, 0, 3, 0, 3},    {0, 0, 3, 0, 2},    {-1, 2, 2, 2, 1},   {-1, 0, 4, 0, 0},
        {1, 2, 2, 0, 1},    {3, 1, 2, -2, 1},   {1, 1, 4, -2, 2},   {-2, -1, 0, 6, 0},  {0, -2, 0, 4, 0},
        {-2, 0, 0, 6, 1},   {-2, -2, 2, 4, 2},  {0, -3, 2, 2, 2},   {0, 0, 0, 4, 2},    {-1, -1, 2, 3, 2},
        {-2, 0, 2, 4, 0},   {2, -1, 0, 2, 1},   {1, 0, 0, 3, 0},    {0, 1, 0, 4, 1},    {0, 1, 0, 4, 0},
        {1, -1, 2, 1, 2},   {0, 0, 2, 2, 3},    {1, 0, 2, 2, 2},    {-1, 0, 2, 2, 2},   {-2, 0, 4, 2, 1},
        {2, 1, 0, 2, 1},    {2, 1, 0, 2, 0},    {2, -1, 2, 0, 0},   {1, 0, 2, 1, 0},    {0, 1, 2, 2, 0},
        {2, 0, 2, 0, 3},    {3, 0, 2, 0, 2},    {1, 0, 2, 0, 2},    {1, 0, 3, 0, 3},    {1, 1, 2, 1, 1},
        {0, 2, 2, 2, 2},    {2, 1, 2, 0, 0},    {2, 0, 4, -2, 1},   {4, 1, 2, -2, 2},   {-1, -1, 0, 6, 0},
        {-3, -1, 2, 6, 2},  {-1, 0, 0, 6, 1},   {-3, 0, 2, 6, 1},   {1, -1, 0, 4, 1},   {1, -1, 0, 4, 0},
        {-2, 0, 2, 5, 2},   {1, -2, 2, 2, 1},   {3, -1, 0, 2, 0},   {1, -1, 2, 2, 0},   {0, 0, 2, 3, 1},
        {-1, 1, 2, 4, 1},   {0, 1, 2, 3, 2},    {-1, 0, 4, 2, 1},   {2, 0, 2, 1, 1},    {5, 0, 0, 0, 0},
        {2, 1, 2, 1, 2},    {1, 0, 4, 0, 1},    {3, 1, 2, 0, 1},    {3, 0, 4, -2, 2},   {-2, -1, 2, 6, 2},
        {0, 0, 0, 6, 0},    {0, -2, 2, 4, 2},   {-2, 0, 2, 6, 1},   {2, 0, 0, 4, 1},    {2, 0, 0, 4, 0},
        {2, -2, 2, 2, 2},   {0, 0, 2, 4, 0},    {1, 0, 2, 3, 2},    {4, 0, 0, 2, 0},    {2, 0, 2, 2, 0},
        {0, 0, 4, 2, 2},    {4, -1, 2, 0, 2},   {3, 0, 2, 1, 2},    {2, 1, 2, 2, 1},    {4, 1, 2, 0, 2},
        {-1, -1, 2, 6, 2},  {-1, 0, 2, 6, 1},   {1, -1, 2, 4, 1},   {1, 1, 2, 4, 2},    {3, 1, 2, 2, 2},
        {5, 0, 2, 0, 1},    {2, -1, 2, 4, 2},   {2, 0, 2, 4, 1}};
    for (I = 0; I < iNLS; I++)
        for (J = 0; J < 5; J++)
        {
            iNALS[J][I] = iNALST[I][J];
        }
    // CLST is the transposition of CLS
    double dCLST[iNLS][6] = {{-172064161e0, -174666e0, 33386e0, 92052331e0, 9086e0, 15377e0},
                             {-13170906e0, -1675e0, -13696e0, 5730336e0, -3015e0, -4587e0},
                             {-2276413e0, -234e0, 2796e0, 978459e0, -485e0, 1374e0},
                             {2074554e0, 207e0, -698e0, -897492e0, 470e0, -291e0},
                             {1475877e0, -3633e0, 11817e0, 73871e0, -184e0, -1924e0},
                             {-516821e0, 1226e0, -524e0, 224386e0, -677e0, -174e0},
                             {711159e0, 73e0, -872e0, -6750e0, 0e0, 358e0},
                             {-387298e0, -367e0, 380e0, 200728e0, 18e0, 318e0},
                             {-301461e0, -36e0, 816e0, 129025e0, -63e0, 367e0},
                             {215829e0, -494e0, 111e0, -95929e0, 299e0, 132e0},
                             {128227e0, 137e0, 181e0, -68982e0, -9e0, 39e0},
                             {123457e0, 11e0, 19e0, -53311e0, 32e0, -4e0},
                             {156994e0, 10e0, -168e0, -1235e0, 0e0, 82e0},
                             {63110e0, 63e0, 27e0, -33228e0, 0e0, -9e0},
                             {-57976e0, -63e0, -189e0, 31429e0, 0e0, -75e0},
                             {-59641e0, -11e0, 149e0, 25543e0, -11e0, 66e0},
                             {-51613e0, -42e0, 129e0, 26366e0, 0e0, 78e0},
                             {45893e0, 50e0, 31e0, -24236e0, -10e0, 20e0},
                             {63384e0, 11e0, -150e0, -1220e0, 0e0, 29e0},
                             {-38571e0, -1e0, 158e0, 16452e0, -11e0, 68e0},
                             {32481e0, 0e0, 0e0, -13870e0, 0e0, 0e0},
                             {-47722e0, 0e0, -18e0, 477e0, 0e0, -25e0},
                             {-31046e0, -1e0, 131e0, 13238e0, -11e0, 59e0},
                             {28593e0, 0e0, -1e0, -12338e0, 10e0, -3e0},
                             {20441e0, 21e0, 10e0, -10758e0, 0e0, -3e0},
                             {29243e0, 0e0, -74e0, -609e0, 0e0, 13e0},
                             {25887e0, 0e0, -66e0, -550e0, 0e0, 11e0},
                             {-14053e0, -25e0, 79e0, 8551e0, -2e0, -45e0},
                             {15164e0, 10e0, 11e0, -8001e0, 0e0, -1e0},
                             {-15794e0, 72e0, -16e0, 6850e0, -42e0, -5e0},
                             {21783e0, 0e0, 13e0, -167e0, 0e0, 13e0},
                             {-12873e0, -10e0, -37e0, 6953e0, 0e0, -14e0},
                             {-12654e0, 11e0, 63e0, 6415e0, 0e0, 26e0},
                             {-10204e0, 0e0, 25e0, 5222e0, 0e0, 15e0},
                             {16707e0, -85e0, -10e0, 168e0, -1e0, 10e0},
                             {-7691e0, 0e0, 44e0, 3268e0, 0e0, 19e0},
                             {-11024e0, 0e0, -14e0, 104e0, 0e0, 2e0},
                             {7566e0, -21e0, -11e0, -3250e0, 0e0, -5e0},
                             {-6637e0, -11e0, 25e0, 3353e0, 0e0, 14e0},
                             {-7141e0, 21e0, 8e0, 3070e0, 0e0, 4e0},
                             {-6302e0, -11e0, 2e0, 3272e0, 0e0, 4e0},
                             {5800e0, 10e0, 2e0, -3045e0, 0e0, -1e0},
                             {6443e0, 0e0, -7e0, -2768e0, 0e0, -4e0},
                             {-5774e0, -11e0, -15e0, 3041e0, 0e0, -5e0},
                             {-5350e0, 0e0, 21e0, 2695e0, 0e0, 12e0},
                             {-4752e0, -11e0, -3e0, 2719e0, 0e0, -3e0},
                             {-4940e0, -11e0, -21e0, 2720e0, 0e0, -9e0},
                             {7350e0, 0e0, -8e0, -51e0, 0e0, 4e0},
                             {4065e0, 0e0, 6e0, -2206e0, 0e0, 1e0},
                             {6579e0, 0e0, -24e0, -199e0, 0e0, 2e0},
                             {3579e0, 0e0, 5e0, -1900e0, 0e0, 1e0},
                             {4725e0, 0e0, -6e0, -41e0, 0e0, 3e0},
                             {-3075e0, 0e0, -2e0, 1313e0, 0e0, -1e0},
                             {-2904e0, 0e0, 15e0, 1233e0, 0e0, 7e0},
                             {4348e0, 0e0, -10e0, -81e0, 0e0, 2e0},
                             {-2878e0, 0e0, 8e0, 1232e0, 0e0, 4e0},
                             {-4230e0, 0e0, 5e0, -20e0, 0e0, -2e0},
                             {-2819e0, 0e0, 7e0, 1207e0, 0e0, 3e0},
                             {-4056e0, 0e0, 5e0, 40e0, 0e0, -2e0},
                             {-2647e0, 0e0, 11e0, 1129e0, 0e0, 5e0},
                             {-2294e0, 0e0, -10e0, 1266e0, 0e0, -4e0},
                             {2481e0, 0e0, -7e0, -1062e0, 0e0, -3e0},
                             {2179e0, 0e0, -2e0, -1129e0, 0e0, -2e0},
                             {3276e0, 0e0, 1e0, -9e0, 0e0, 0e0},
                             {-3389e0, 0e0, 5e0, 35e0, 0e0, -2e0},
                             {3339e0, 0e0, -13e0, -107e0, 0e0, 1e0},
                             {-1987e0, 0e0, -6e0, 1073e0, 0e0, -2e0},
                             {-1981e0, 0e0, 0e0, 854e0, 0e0, 0e0},
                             {4026e0, 0e0, -353e0, -553e0, 0e0, -139e0},
                             {1660e0, 0e0, -5e0, -710e0, 0e0, -2e0},
                             {-1521e0, 0e0, 9e0, 647e0, 0e0, 4e0},
                             {1314e0, 0e0, 0e0, -700e0, 0e0, 0e0},
                             {-1283e0, 0e0, 0e0, 672e0, 0e0, 0e0},
                             {-1331e0, 0e0, 8e0, 663e0, 0e0, 4e0},
                             {1383e0, 0e0, -2e0, -594e0, 0e0, -2e0},
                             {1405e0, 0e0, 4e0, -610e0, 0e0, 2e0},
                             {1290e0, 0e0, 0e0, -556e0, 0e0, 0e0},
                             {-1214e0, 0e0, 5e0, 518e0, 0e0, 2e0},
                             {1146e0, 0e0, -3e0, -490e0, 0e0, -1e0},
                             {1019e0, 0e0, -1e0, -527e0, 0e0, -1e0},
                             {-1100e0, 0e0, 9e0, 465e0, 0e0, 4e0},
                             {-970e0, 0e0, 2e0, 496e0, 0e0, 1e0},
                             {1575e0, 0e0, -6e0, -50e0, 0e0, 0e0},
                             {934e0, 0e0, -3e0, -399e0, 0e0, -1e0},
                             {922e0, 0e0, -1e0, -395e0, 0e0, -1e0},
                             {815e0, 0e0, -1e0, -422e0, 0e0, -1e0},
                             {834e0, 0e0, 2e0, -440e0, 0e0, 1e0},
                             {1248e0, 0e0, 0e0, -170e0, 0e0, 1e0},
                             {1338e0, 0e0, -5e0, -39e0, 0e0, 0e0},
                             {716e0, 0e0, -2e0, -389e0, 0e0, -1e0},
                             {1282e0, 0e0, -3e0, -23e0, 0e0, 1e0},
                             {742e0, 0e0, 1e0, -391e0, 0e0, 0e0},
                             {1020e0, 0e0, -25e0, -495e0, 0e0, -10e0},
                             {715e0, 0e0, -4e0, -326e0, 0e0, 2e0},
                             {-666e0, 0e0, -3e0, 369e0, 0e0, -1e0},
                             {-667e0, 0e0, 1e0, 346e0, 0e0, 1e0},
                             {-704e0, 0e0, 0e0, 304e0, 0e0, 0e0},
                             {-694e0, 0e0, 5e0, 294e0, 0e0, 2e0},
                             {-1014e0, 0e0, -1e0, 4e0, 0e0, -1e0},
                             {-585e0, 0e0, -2e0, 316e0, 0e0, -1e0},
                             {-949e0, 0e0, 1e0, 8e0, 0e0, -1e0},
                             {-595e0, 0e0, 0e0, 258e0, 0e0, 0e0},
                             {528e0, 0e0, 0e0, -279e0, 0e0, 0e0},
                             {-590e0, 0e0, 4e0, 252e0, 0e0, 2e0},
                             {570e0, 0e0, -2e0, -244e0, 0e0, -1e0},
                             {-502e0, 0e0, 3e0, 250e0, 0e0, 2e0},
                             {-875e0, 0e0, 1e0, 29e0, 0e0, 0e0},
                             {-492e0, 0e0, -3e0, 275e0, 0e0, -1e0},
                             {535e0, 0e0, -2e0, -228e0, 0e0, -1e0},
                             {-467e0, 0e0, 1e0, 240e0, 0e0, 1e0},
                             {591e0, 0e0, 0e0, -253e0, 0e0, 0e0},
                             {-453e0, 0e0, -1e0, 244e0, 0e0, -1e0},
                             {766e0, 0e0, 1e0, 9e0, 0e0, 0e0},
                             {-446e0, 0e0, 2e0, 225e0, 0e0, 1e0},
                             {-488e0, 0e0, 2e0, 207e0, 0e0, 1e0},
                             {-468e0, 0e0, 0e0, 201e0, 0e0, 0e0},
                             {-421e0, 0e0, 1e0, 216e0, 0e0, 1e0},
                             {463e0, 0e0, 0e0, -200e0, 0e0, 0e0},
                             {-673e0, 0e0, 2e0, 14e0, 0e0, 0e0},
                             {658e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-438e0, 0e0, 0e0, 188e0, 0e0, 0e0},
                             {-390e0, 0e0, 0e0, 205e0, 0e0, 0e0},
                             {639e0, -11e0, -2e0, -19e0, 0e0, 0e0},
                             {412e0, 0e0, -2e0, -176e0, 0e0, -1e0},
                             {-361e0, 0e0, 0e0, 189e0, 0e0, 0e0},
                             {360e0, 0e0, -1e0, -185e0, 0e0, -1e0},
                             {588e0, 0e0, -3e0, -24e0, 0e0, 0e0},
                             {-578e0, 0e0, 1e0, 5e0, 0e0, 0e0},
                             {-396e0, 0e0, 0e0, 171e0, 0e0, 0e0},
                             {565e0, 0e0, -1e0, -6e0, 0e0, 0e0},
                             {-335e0, 0e0, -1e0, 184e0, 0e0, -1e0},
                             {357e0, 0e0, 1e0, -154e0, 0e0, 0e0},
                             {321e0, 0e0, 1e0, -174e0, 0e0, 0e0},
                             {-301e0, 0e0, -1e0, 162e0, 0e0, 0e0},
                             {-334e0, 0e0, 0e0, 144e0, 0e0, 0e0},
                             {493e0, 0e0, -2e0, -15e0, 0e0, 0e0},
                             {494e0, 0e0, -2e0, -19e0, 0e0, 0e0},
                             {337e0, 0e0, -1e0, -143e0, 0e0, -1e0},
                             {280e0, 0e0, -1e0, -144e0, 0e0, 0e0},
                             {309e0, 0e0, 1e0, -134e0, 0e0, 0e0},
                             {-263e0, 0e0, 2e0, 131e0, 0e0, 1e0},
                             {253e0, 0e0, 1e0, -138e0, 0e0, 0e0},
                             {245e0, 0e0, 0e0, -128e0, 0e0, 0e0},
                             {416e0, 0e0, -2e0, -17e0, 0e0, 0e0},
                             {-229e0, 0e0, 0e0, 128e0, 0e0, 0e0},
                             {231e0, 0e0, 0e0, -120e0, 0e0, 0e0},
                             {-259e0, 0e0, 2e0, 109e0, 0e0, 1e0},
                             {375e0, 0e0, -1e0, -8e0, 0e0, 0e0},
                             {252e0, 0e0, 0e0, -108e0, 0e0, 0e0},
                             {-245e0, 0e0, 1e0, 104e0, 0e0, 0e0},
                             {243e0, 0e0, -1e0, -104e0, 0e0, 0e0},
                             {208e0, 0e0, 1e0, -112e0, 0e0, 0e0},
                             {199e0, 0e0, 0e0, -102e0, 0e0, 0e0},
                             {-208e0, 0e0, 1e0, 105e0, 0e0, 0e0},
                             {335e0, 0e0, -2e0, -14e0, 0e0, 0e0},
                             {-325e0, 0e0, 1e0, 7e0, 0e0, 0e0},
                             {-187e0, 0e0, 0e0, 96e0, 0e0, 0e0},
                             {197e0, 0e0, -1e0, -100e0, 0e0, 0e0},
                             {-192e0, 0e0, 2e0, 94e0, 0e0, 1e0},
                             {-188e0, 0e0, 0e0, 83e0, 0e0, 0e0},
                             {276e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-286e0, 0e0, 1e0, 6e0, 0e0, 0e0},
                             {186e0, 0e0, -1e0, -79e0, 0e0, 0e0},
                             {-219e0, 0e0, 0e0, 43e0, 0e0, 0e0},
                             {276e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-153e0, 0e0, -1e0, 84e0, 0e0, 0e0},
                             {-156e0, 0e0, 0e0, 81e0, 0e0, 0e0},
                             {-154e0, 0e0, 1e0, 78e0, 0e0, 0e0},
                             {-174e0, 0e0, 1e0, 75e0, 0e0, 0e0},
                             {-163e0, 0e0, 2e0, 69e0, 0e0, 1e0},
                             {-228e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {91e0, 0e0, -4e0, -54e0, 0e0, -2e0},
                             {175e0, 0e0, 0e0, -75e0, 0e0, 0e0},
                             {-159e0, 0e0, 0e0, 69e0, 0e0, 0e0},
                             {141e0, 0e0, 0e0, -72e0, 0e0, 0e0},
                             {147e0, 0e0, 0e0, -75e0, 0e0, 0e0},
                             {-132e0, 0e0, 0e0, 69e0, 0e0, 0e0},
                             {159e0, 0e0, -28e0, -54e0, 0e0, 11e0},
                             {213e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {123e0, 0e0, 0e0, -64e0, 0e0, 0e0},
                             {-118e0, 0e0, -1e0, 66e0, 0e0, 0e0},
                             {144e0, 0e0, -1e0, -61e0, 0e0, 0e0},
                             {-121e0, 0e0, 1e0, 60e0, 0e0, 0e0},
                             {-134e0, 0e0, 1e0, 56e0, 0e0, 1e0},
                             {-105e0, 0e0, 0e0, 57e0, 0e0, 0e0},
                             {-102e0, 0e0, 0e0, 56e0, 0e0, 0e0},
                             {120e0, 0e0, 0e0, -52e0, 0e0, 0e0},
                             {101e0, 0e0, 0e0, -54e0, 0e0, 0e0},
                             {-113e0, 0e0, 0e0, 59e0, 0e0, 0e0},
                             {-106e0, 0e0, 0e0, 61e0, 0e0, 0e0},
                             {-129e0, 0e0, 1e0, 55e0, 0e0, 0e0},
                             {-114e0, 0e0, 0e0, 57e0, 0e0, 0e0},
                             {113e0, 0e0, -1e0, -49e0, 0e0, 0e0},
                             {-102e0, 0e0, 0e0, 44e0, 0e0, 0e0},
                             {-94e0, 0e0, 0e0, 51e0, 0e0, 0e0},
                             {-100e0, 0e0, -1e0, 56e0, 0e0, 0e0},
                             {87e0, 0e0, 0e0, -47e0, 0e0, 0e0},
                             {161e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {96e0, 0e0, 0e0, -50e0, 0e0, 0e0},
                             {151e0, 0e0, -1e0, -5e0, 0e0, 0e0},
                             {-104e0, 0e0, 0e0, 44e0, 0e0, 0e0},
                             {-110e0, 0e0, 0e0, 48e0, 0e0, 0e0},
                             {-100e0, 0e0, 1e0, 50e0, 0e0, 0e0},
                             {92e0, 0e0, -5e0, 12e0, 0e0, -2e0},
                             {82e0, 0e0, 0e0, -45e0, 0e0, 0e0},
                             {82e0, 0e0, 0e0, -45e0, 0e0, 0e0},
                             {-78e0, 0e0, 0e0, 41e0, 0e0, 0e0},
                             {-77e0, 0e0, 0e0, 43e0, 0e0, 0e0},
                             {2e0, 0e0, 0e0, 54e0, 0e0, 0e0},
                             {94e0, 0e0, 0e0, -40e0, 0e0, 0e0},
                             {-93e0, 0e0, 0e0, 40e0, 0e0, 0e0},
                             {-83e0, 0e0, 10e0, 40e0, 0e0, -2e0},
                             {83e0, 0e0, 0e0, -36e0, 0e0, 0e0},
                             {-91e0, 0e0, 0e0, 39e0, 0e0, 0e0},
                             {128e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-79e0, 0e0, 0e0, 34e0, 0e0, 0e0},
                             {-83e0, 0e0, 0e0, 47e0, 0e0, 0e0},
                             {84e0, 0e0, 0e0, -44e0, 0e0, 0e0},
                             {83e0, 0e0, 0e0, -43e0, 0e0, 0e0},
                             {91e0, 0e0, 0e0, -39e0, 0e0, 0e0},
                             {-77e0, 0e0, 0e0, 39e0, 0e0, 0e0},
                             {84e0, 0e0, 0e0, -43e0, 0e0, 0e0},
                             {-92e0, 0e0, 1e0, 39e0, 0e0, 0e0},
                             {-92e0, 0e0, 1e0, 39e0, 0e0, 0e0},
                             {-94e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {68e0, 0e0, 0e0, -36e0, 0e0, 0e0},
                             {-61e0, 0e0, 0e0, 32e0, 0e0, 0e0},
                             {71e0, 0e0, 0e0, -31e0, 0e0, 0e0},
                             {62e0, 0e0, 0e0, -34e0, 0e0, 0e0},
                             {-63e0, 0e0, 0e0, 33e0, 0e0, 0e0},
                             {-73e0, 0e0, 0e0, 32e0, 0e0, 0e0},
                             {115e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-103e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {63e0, 0e0, 0e0, -28e0, 0e0, 0e0},
                             {74e0, 0e0, 0e0, -32e0, 0e0, 0e0},
                             {-103e0, 0e0, -3e0, 3e0, 0e0, -1e0},
                             {-69e0, 0e0, 0e0, 30e0, 0e0, 0e0},
                             {57e0, 0e0, 0e0, -29e0, 0e0, 0e0},
                             {94e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {64e0, 0e0, 0e0, -33e0, 0e0, 0e0},
                             {-63e0, 0e0, 0e0, 26e0, 0e0, 0e0},
                             {-38e0, 0e0, 0e0, 20e0, 0e0, 0e0},
                             {-43e0, 0e0, 0e0, 24e0, 0e0, 0e0},
                             {-45e0, 0e0, 0e0, 23e0, 0e0, 0e0},
                             {47e0, 0e0, 0e0, -24e0, 0e0, 0e0},
                             {-48e0, 0e0, 0e0, 25e0, 0e0, 0e0},
                             {45e0, 0e0, 0e0, -26e0, 0e0, 0e0},
                             {56e0, 0e0, 0e0, -25e0, 0e0, 0e0},
                             {88e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-75e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {85e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {49e0, 0e0, 0e0, -26e0, 0e0, 0e0},
                             {-74e0, 0e0, -3e0, -1e0, 0e0, -1e0},
                             {-39e0, 0e0, 0e0, 21e0, 0e0, 0e0},
                             {45e0, 0e0, 0e0, -20e0, 0e0, 0e0},
                             {51e0, 0e0, 0e0, -22e0, 0e0, 0e0},
                             {-40e0, 0e0, 0e0, 21e0, 0e0, 0e0},
                             {41e0, 0e0, 0e0, -21e0, 0e0, 0e0},
                             {-42e0, 0e0, 0e0, 24e0, 0e0, 0e0},
                             {-51e0, 0e0, 0e0, 22e0, 0e0, 0e0},
                             {-42e0, 0e0, 0e0, 22e0, 0e0, 0e0},
                             {39e0, 0e0, 0e0, -21e0, 0e0, 0e0},
                             {46e0, 0e0, 0e0, -18e0, 0e0, 0e0},
                             {-53e0, 0e0, 0e0, 22e0, 0e0, 0e0},
                             {82e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {81e0, 0e0, -1e0, -4e0, 0e0, 0e0},
                             {47e0, 0e0, 0e0, -19e0, 0e0, 0e0},
                             {53e0, 0e0, 0e0, -23e0, 0e0, 0e0},
                             {-45e0, 0e0, 0e0, 22e0, 0e0, 0e0},
                             {-44e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-33e0, 0e0, 0e0, 16e0, 0e0, 0e0},
                             {-61e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {28e0, 0e0, 0e0, -15e0, 0e0, 0e0},
                             {-38e0, 0e0, 0e0, 19e0, 0e0, 0e0},
                             {-33e0, 0e0, 0e0, 21e0, 0e0, 0e0},
                             {-60e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {48e0, 0e0, 0e0, -10e0, 0e0, 0e0},
                             {27e0, 0e0, 0e0, -14e0, 0e0, 0e0},
                             {38e0, 0e0, 0e0, -20e0, 0e0, 0e0},
                             {31e0, 0e0, 0e0, -13e0, 0e0, 0e0},
                             {-29e0, 0e0, 0e0, 15e0, 0e0, 0e0},
                             {28e0, 0e0, 0e0, -15e0, 0e0, 0e0},
                             {-32e0, 0e0, 0e0, 15e0, 0e0, 0e0},
                             {45e0, 0e0, 0e0, -8e0, 0e0, 0e0},
                             {-44e0, 0e0, 0e0, 19e0, 0e0, 0e0},
                             {28e0, 0e0, 0e0, -15e0, 0e0, 0e0},
                             {-51e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-36e0, 0e0, 0e0, 20e0, 0e0, 0e0},
                             {44e0, 0e0, 0e0, -19e0, 0e0, 0e0},
                             {26e0, 0e0, 0e0, -14e0, 0e0, 0e0},
                             {-60e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {35e0, 0e0, 0e0, -18e0, 0e0, 0e0},
                             {-27e0, 0e0, 0e0, 11e0, 0e0, 0e0},
                             {47e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {36e0, 0e0, 0e0, -15e0, 0e0, 0e0},
                             {-36e0, 0e0, 0e0, 20e0, 0e0, 0e0},
                             {-35e0, 0e0, 0e0, 19e0, 0e0, 0e0},
                             {-37e0, 0e0, 0e0, 19e0, 0e0, 0e0},
                             {32e0, 0e0, 0e0, -16e0, 0e0, 0e0},
                             {35e0, 0e0, 0e0, -14e0, 0e0, 0e0},
                             {32e0, 0e0, 0e0, -13e0, 0e0, 0e0},
                             {65e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {47e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {32e0, 0e0, 0e0, -16e0, 0e0, 0e0},
                             {37e0, 0e0, 0e0, -16e0, 0e0, 0e0},
                             {-30e0, 0e0, 0e0, 15e0, 0e0, 0e0},
                             {-32e0, 0e0, 0e0, 16e0, 0e0, 0e0},
                             {-31e0, 0e0, 0e0, 13e0, 0e0, 0e0},
                             {37e0, 0e0, 0e0, -16e0, 0e0, 0e0},
                             {31e0, 0e0, 0e0, -13e0, 0e0, 0e0},
                             {49e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {32e0, 0e0, 0e0, -13e0, 0e0, 0e0},
                             {23e0, 0e0, 0e0, -12e0, 0e0, 0e0},
                             {-43e0, 0e0, 0e0, 18e0, 0e0, 0e0},
                             {26e0, 0e0, 0e0, -11e0, 0e0, 0e0},
                             {-32e0, 0e0, 0e0, 14e0, 0e0, 0e0},
                             {-29e0, 0e0, 0e0, 14e0, 0e0, 0e0},
                             {-27e0, 0e0, 0e0, 12e0, 0e0, 0e0},
                             {30e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-11e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-21e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {-34e0, 0e0, 0e0, 15e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {-36e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-9e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-21e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-29e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-15e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-20e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {28e0, 0e0, 0e0, 0e0, 0e0, -2e0},
                             {17e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-22e0, 0e0, 0e0, 12e0, 0e0, 0e0},
                             {-14e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {24e0, 0e0, 0e0, -11e0, 0e0, 0e0},
                             {11e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {14e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {24e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {18e0, 0e0, 0e0, -8e0, 0e0, 0e0},
                             {-38e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-31e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-16e0, 0e0, 0e0, 8e0, 0e0, 0e0},
                             {29e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-18e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-17e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {16e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {22e0, 0e0, 0e0, -12e0, 0e0, 0e0},
                             {20e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-13e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {-17e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {-14e0, 0e0, 0e0, 8e0, 0e0, 0e0},
                             {0e0, 0e0, 0e0, -7e0, 0e0, 0e0},
                             {14e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {19e0, 0e0, 0e0, -10e0, 0e0, 0e0},
                             {-34e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-20e0, 0e0, 0e0, 8e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {-18e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {13e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {17e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {15e0, 0e0, 0e0, -8e0, 0e0, 0e0},
                             {-11e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {13e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {-18e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-35e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {-19e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {-26e0, 0e0, 0e0, 11e0, 0e0, 0e0},
                             {8e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {10e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {-21e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {-15e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {-29e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-19e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {12e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {22e0, 0e0, 0e0, -9e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-20e0, 0e0, 0e0, 11e0, 0e0, 0e0},
                             {-20e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-17e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {15e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {8e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {14e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {25e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-13e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {-14e0, 0e0, 0e0, 8e0, 0e0, 0e0},
                             {13e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {-17e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {10e0, 0e0, 0e0, -6e0, 0e0, 0e0},
                             {-15e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-22e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {28e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {15e0, 0e0, 0e0, -7e0, 0e0, 0e0},
                             {23e0, 0e0, 0e0, -10e0, 0e0, 0e0},
                             {12e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {29e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-25e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {22e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-18e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {15e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-23e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {12e0, 0e0, 0e0, -5e0, 0e0, 0e0},
                             {-8e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-19e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {21e0, 0e0, 0e0, -9e0, 0e0, 0e0},
                             {23e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-16e0, 0e0, 0e0, 8e0, 0e0, 0e0},
                             {-19e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {-22e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {27e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {16e0, 0e0, 0e0, -8e0, 0e0, 0e0},
                             {19e0, 0e0, 0e0, -8e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {-9e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-9e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-8e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {18e0, 0e0, 0e0, -9e0, 0e0, 0e0},
                             {16e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-10e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-23e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {16e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 6e0, 0e0, 0e0},
                             {-8e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {30e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {24e0, 0e0, 0e0, -10e0, 0e0, 0e0},
                             {10e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {-16e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {-16e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {17e0, 0e0, 0e0, -7e0, 0e0, 0e0},
                             {-24e0, 0e0, 0e0, 10e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-24e0, 0e0, 0e0, 11e0, 0e0, 0e0},
                             {-23e0, 0e0, 0e0, 9e0, 0e0, 0e0},
                             {-13e0, 0e0, 0e0, 5e0, 0e0, 0e0},
                             {-15e0, 0e0, 0e0, 7e0, 0e0, 0e0},
                             {0e0, 0e0, -1988e0, 0e0, 0e0, -1679e0},
                             {0e0, 0e0, -63e0, 0e0, 0e0, -27e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {0e0, 0e0, 5e0, 0e0, 0e0, 4e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {0e0, 0e0, 364e0, 0e0, 0e0, 176e0},
                             {0e0, 0e0, -1044e0, 0e0, 0e0, -891e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {0e0, 0e0, 330e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {0e0, 0e0, 5e0, 0e0, 0e0, 0e0},
                             {0e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {0e0, 0e0, -12e0, 0e0, 0e0, -10e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {0e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-8e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {8e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-13e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {10e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {10e0, 0e0, 13e0, 6e0, 0e0, -5e0},
                             {0e0, 0e0, 30e0, 0e0, 0e0, 14e0},
                             {0e0, 0e0, -162e0, 0e0, 0e0, -138e0},
                             {0e0, 0e0, 75e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {9e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-6e0, 0e0, -3e0, 3e0, 0e0, 1e0},
                             {0e0, 0e0, -3e0, 0e0, 0e0, -2e0},
                             {11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-1e0, 0e0, 3e0, 3e0, 0e0, -1e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {0e0, 0e0, -13e0, 0e0, 0e0, -11e0},
                             {3e0, 0e0, 6e0, 0e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {8e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {8e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-8e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {0e0, 0e0, -26e0, 0e0, 0e0, -11e0},
                             {0e0, 0e0, -10e0, 0e0, 0e0, -5e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-13e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-7e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {13e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-12e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {0e0, 0e0, -5e0, 0e0, 0e0, -2e0},
                             {-7e0, 0e0, 0e0, 4e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {12e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {6e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, -4e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-5e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 3e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {10e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {7e0, 0e0, 0e0, -3e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {11e0, 0e0, 0e0, 0e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-6e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {5e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-4e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0},
                             {4e0, 0e0, 0e0, -2e0, 0e0, 0e0},
                             {3e0, 0e0, 0e0, -1e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 1e0, 0e0, 0e0},
                             {-3e0, 0e0, 0e0, 2e0, 0e0, 0e0}};
    for (I = 0; I < iNLS; I++)
        for (J = 0; J < 6; J++)
        {
            dCLS[J][I] = dCLST[I][J];
        }
    // NAPLT is the transposition of NAPL
    int iNAPLT[iNPL][14] = {
        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -8, 16, -4, -5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 8, -16, 4, 5, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 2, 2},
        {0, 0, 0, 0, 0, 0, 0, -4, 8, -1, -5, 0, 0, 2},   {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, 0, 3, -8, 3, 0, 0, 0, 0},    {-1, 0, 0, 0, 0, 0, 10, -3, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 6, -3, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -5, 8, -3, 0, 0, 0, 0},   {0, 0, 0, 0, 0, 0, 0, -4, 8, -3, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 4, -8, 1, 5, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -5, 6, 4, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, -5, 0, 0, 0},   {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -5, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 5, 0, 0, 0},   {0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 5, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 5, 0, 0, 2},     {2, 0, -1, -1, 0, 0, 0, 3, -7, 0, 0, 0, 0, 0},
        {1, 0, 0, -2, 0, 0, 19, -21, 3, 0, 0, 0, 0, 0},  {0, 0, 1, -1, 1, 0, 2, -4, 0, -3, 0, 0, 0, 0},
        {1, 0, 0, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},    {0, 0, 1, -1, 1, 0, 0, -1, 0, -4, 10, 0, 0, 0},
        {-2, 0, 0, 2, 1, 0, 0, 2, 0, 0, -5, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 3, -7, 4, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0},    {-2, 0, 0, 2, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {-1, 0, 0, 0, 0, 0, 18, -16, 0, 0, 0, 0, 0, 0},  {-2, 0, 1, 1, 2, 0, 0, 1, 0, -2, 0, 0, 0, 0},
        {-1, 0, 1, -1, 1, 0, 18, -17, 0, 0, 0, 0, 0, 0}, {-1, 0, 0, 1, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -8, 13, 0, 0, 0, 0, 0, 2},    {0, 0, 2, -2, 2, 0, -8, 11, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -8, 13, 0, 0, 0, 0, 0, 1},    {0, 0, 1, -1, 1, 0, -8, 12, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 0},    {0, 0, 1, -1, 1, 0, 8, -14, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 8, -13, 0, 0, 0, 0, 0, 1},    {-2, 0, 0, 2, 1, 0, 0, 2, 0, -4, 5, 0, 0, 0},
        {-2, 0, 0, 2, 2, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 2, 0, -3, 1, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 3, -5, 0, 2, 0, 0, 0, 0},     {-2, 0, 0, 2, 0, 0, 0, 2, 0, -4, 3, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, -1, 2, 0, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, 0, -2, 2, 0, 0, 0, 0, 0},    {-1, 0, 1, 0, 1, 0, 3, -5, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 1, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 2, 0, -2, -2, 0, 0, 0},
        {-2, 0, 2, 0, 2, 0, 0, -5, 9, 0, 0, 0, 0, 0},    {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, -1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},      {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 2, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2},
        {-1, 0, 0, 1, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0},    {0, 0, -1, 1, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 2, 0, 0, 0},    {0, 0, 0, 0, 1, 0, 0, -9, 17, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 2, 0, -3, 5, 0, 0, 0, 0, 0, 0},     {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 2, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0},     {1, 0, 0, -2, 0, 0, 17, -16, 0, -2, 0, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, -3, 0, 0, 0},   {-2, 0, 0, 2, 1, 0, 0, 5, -6, 0, 0, 0, 0, 0},
        {0, 0, -2, 2, 0, 0, 0, 9, -13, 0, 0, 0, 0, 0},   {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},      {0, 0, -1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
        {0, 0, -2, 2, 0, 0, 5, -6, 0, 0, 0, 0, 0, 0},    {0, 0, -1, 1, 1, 0, 5, -7, 0, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 0, 0, 6, -8, 0, 0, 0, 0, 0, 0},    {2, 0, 1, -3, 1, 0, -6, 7, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0},      {0, 0, -1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2},      {0, 0, 0, 0, 0, 0, 0, -8, 15, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -8, 15, 0, 0, 0, 0, 1},    {0, 0, 1, -1, 1, 0, 0, -9, 15, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},    {1, 0, -1, -1, 0, 0, 0, 8, -15, 0, 0, 0, 0, 0},
        {2, 0, 0, -2, 0, 0, 2, -5, 0, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 2, 0, -5, 5, 0, 0, 0},
        {2, 0, 0, -2, 1, 0, 0, -6, 8, 0, 0, 0, 0, 0},    {2, 0, 0, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
        {-2, 0, 1, 1, 0, 0, 0, 1, 0, -3, 0, 0, 0, 0},    {-2, 0, 1, 1, 1, 0, 0, 1, 0, -3, 0, 0, 0, 0},
        {-2, 0, 0, 2, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 0, 0, 0, 2, 0, -1, -5, 0, 0, 0},   {-1, 0, 0, 1, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {-1, 0, 1, 1, 1, 0, -20, 20, 0, 0, 0, 0, 0, 0},  {1, 0, 0, -2, 0, 0, 20, -21, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 8, -15, 0, 0, 0, 0, 0},    {0, 0, 2, -2, 1, 0, 0, -10, 15, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},    {0, 0, 1, -1, 1, 0, 0, -1, 0, -2, 4, 0, 0, 0},
        {2, 0, 0, -2, 1, 0, -6, 8, 0, 0, 0, 0, 0, 0},    {0, 0, -2, 2, 1, 0, 5, -6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},      {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2},
        {0, 0, 2, -2, 1, 0, 0, -9, 13, 0, 0, 0, 0, 0},   {0, 0, 0, 0, 1, 0, 0, 7, -13, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 9, -17, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -9, 17, 0, 0, 0, 0, 2},    {1, 0, 0, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},
        {1, 0, 0, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 2, 0, 0, -1, 2, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0},     {0, 0, -2, 2, 0, 1, 0, -2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 3, -5, 0, 2, 0, 0, 0, 0},     {-2, 0, 0, 2, 1, 0, 0, 2, 0, -3, 1, 0, 0, 0},
        {-2, 0, 0, 2, 1, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 1, 0, 8, -13, 0, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 8, -12, 0, 0, 0, 0, 0, 0},   {0, 0, 2, -2, 1, 0, -8, 11, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 1, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},    {-1, 0, 0, 0, 1, 0, 18, -16, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 1, 0, 0, 0},   {0, 0, 0, 0, 1, 0, 3, -7, 4, 0, 0, 0, 0, 0},
        {-2, 0, 1, 1, 1, 0, 0, -3, 7, 0, 0, 0, 0, 0},    {0, 0, 1, -1, 2, 0, 0, -1, 0, -2, 5, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, -2, 5, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, -10, 3, 0, 0, 0, 0, 0, 0},    {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 0, 1, 0, 10, -3, 0, 0, 0, 0, 0, 0},   {0, 0, 0, 0, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 2, -5, 0, 0, 0},     {0, 0, -1, 1, 0, 0, 0, 1, 0, 2, -5, 0, 0, 0},
        {2, 0, -1, -1, 1, 0, 0, 3, -7, 0, 0, 0, 0, 0},   {-2, 0, 0, 2, 0, 0, 0, 2, 0, 0, -5, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, -3, 7, -4, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, -18, 16, 0, 0, 0, 0, 0, 0},   {-2, 0, 1, 1, 1, 0, 0, 1, 0, -2, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, -8, 12, 0, 0, 0, 0, 0, 0},   {0, 0, 0, 0, 1, 0, -8, 13, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 0, 0, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0},     {0, 0, 1, -1, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -1, 2, 0, 0, 0, 0, 1},     {-1, 0, 0, 1, 1, 0, 3, -4, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 1, 1, 0, 0, 3, -4, 0, 0, 0, 0, 0},    {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, -2, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 2, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2},      {0, 0, 1, -1, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0, 0, 0, 0},     {0, 0, 1, -1, 2, 0, -3, 4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, -2, 4, 0, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 5, -7, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 1, 0, 5, -8, 0, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 1, 0, 6, -8, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 1, 0, 0, -8, 15, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 1, 0, 0, 2, 0, -3, 0, 0, 0, 0},    {-2, 0, 0, 2, 1, 0, 0, 6, -8, 0, 0, 0, 0, 0},
        {1, 0, 0, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0},   {0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2},      {0, 0, 1, -1, 2, 0, 0, -1, 0, 0, -1, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0},     {0, 0, -1, 1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -7, 13, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 7, -13, 0, 0, 0, 0, 0},
        {2, 0, 0, -2, 1, 0, 0, -5, 6, 0, 0, 0, 0, 0},    {0, 0, 2, -2, 1, 0, 0, -8, 11, 0, 0, 0, 0, 0},
        {0, 0, 2, -2, 1, -1, 0, 2, 0, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0},     {0, 0, 1, -1, 1, 0, 0, -1, 0, 0, 3, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2},
        {-2, 0, 0, 2, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
        {0, 0, 0, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},     {2, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, 0, -1, 0, 2, 0, 0, 0, 0},    {0, 0, 1, -1, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 1, -2, 0, 0, 0, 0, 0},     {0, 0, -1, 1, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 0, 1, 0, 0, -2, 0, 0, 0},    {0, 0, 2, -2, 1, 0, 0, -2, 0, 0, 2, 0, 0, 0},
        {0, 0, 1, -1, 1, 0, 3, -6, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0, 0},     {0, 0, 1, -1, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -3, 5, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, -3, 5, 0, 0, 0, 0, 0, 2},
        {0, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, -3, 5, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 0, 1, -4, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, -2, 4, 0, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, 0, -3, 4, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -2, 4, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, -2, 4, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -5, 8, 0, 0, 0, 0, 0, 2},
        {0, 0, 2, -2, 2, 0, -5, 6, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, -5, 8, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -5, 8, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, -5, 7, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -5, 8, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, 0, -1, 0, -1, 0, 0, 0, 0},   {0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},    {0, 0, 2, -2, 1, 0, 0, -2, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -6, 11, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 6, -11, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, -1, 0, 4, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0, 0, 0},
        {2, 0, 0, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0},
        {0, 0, 2, -2, 1, 0, 0, -7, 9, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2},      {0, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 2},      {0, 0, 0, 0, 1, 0, 3, -5, 0, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},    {0, 0, 2, -2, 1, 0, -3, 3, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 2, -4, 0, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, 0, -4, 4, 0, 0, 0, 0, 0},
        {0, 0, 1, -1, 2, 0, -5, 7, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -3, 6, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 0, -4, 6, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -3, 6, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, -3, 6, 0, 0, 0, 0, 2},
        {0, 0, -1, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 1, 0, 2, -3, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -5, 9, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, -5, 9, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 5, -9, 0, 0, 0, 0, 0},     {0, 0, -1, 1, 0, 0, 0, 1, 0, -2, 0, 0, 0, 0},
        {0, 0, 2, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},    {-2, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, -2, 2, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, -6, 10, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, -6, 10, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -2, 3, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -2, 3, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},      {0, 0, 1, -1, 1, 0, 0, -1, 0, 3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1},      {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 4, -8, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, -4, 8, 0, 0, 0, 0, 2},
        {0, 0, -2, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -4, 7, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -4, 7, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 4, -7, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -5, 10, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, -3, 5, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -3, 5, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 3, -5, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 1, -3, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -1, 2, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, -1, 2, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -7, 11, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -7, 11, 0, 0, 0, 0, 0, 1},    {0, 0, -2, 2, 0, 0, 4, -4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, -3, 0, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, -4, 4, 0, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 4, -5, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -4, 7, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, -4, 6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -4, 7, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -4, 6, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -4, 6, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, -4, 5, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -4, 6, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0, 0},
        {-2, 0, 0, 2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
        {0, 0, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -1, 0, 5, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -1, 3, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, -7, 12, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, 1, -2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -2, 5, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, -1, 0, 4, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, -4, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, -1, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -6, 10, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, -6, 10, 0, 0, 0, 0, 0},
        {0, 0, 2, -2, 1, 0, 0, -3, 0, 3, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -3, 7, 0, 0, 0, 0, 2},
        {-2, 0, 0, 2, 0, 0, 4, -4, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -5, 8, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 5, -8, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, -1, 0, 3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -1, 0, 3, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 2, -4, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -2, 4, 0, 0, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, -2, 3, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, -2, 4, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -6, 9, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -6, 9, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, 1, 0, -2, 0, 0, 0, 0},
        {0, 0, 2, -2, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 0, -4, 6, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 4, -6, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 3, -4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -1, 0, 2, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -5, 9, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -3, 4, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -3, 4, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 3, -4, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 1, 0, 0, 2, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, -1, 0, 2, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -3, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, -5, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, -3, 5, 0, 0, 0},     {0, 0, 0, 0, 1, 0, -3, 4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -2, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, -2, 2, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -8, 14, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, -5, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 5, -8, 3, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 3, -8, 3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -3, 8, -3, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -8, 12, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -8, 12, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, -2, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 2},      {0, 0, 2, -2, 1, 0, -5, 5, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 3, -6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -3, 6, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, -3, 6, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -1, 4, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -5, 7, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -5, 7, 0, 0, 0, 0, 0, 1},     {0, 0, 1, -1, 1, 0, -5, 6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, 0, -1, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 0},     {0, 0, 0, 0, 0, -1, 0, 3, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, -2, 6, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 1, 0, 2, -2, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, -6, 9, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 6, -9, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -2, 2, 0, 0, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, -2, 1, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -5, 7, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 5, -7, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, -2, 2, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, -3, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -1, 3, 0, 0, 0, 0, 0, 1},
        {0, 0, 1, -1, 1, 0, -1, 2, 0, 0, 0, 0, 0, 0},    {0, 0, 0, 0, 0, 0, -1, 3, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -7, 10, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -7, 10, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, -4, 8, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -4, 5, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -4, 5, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 4, -5, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -2, 0, 5, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -9, 13, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, -1, 5, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -2, 0, 4, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, -4, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -2, 7, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -2, 5, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, -2, 5, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -6, 8, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -6, 8, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 6, -8, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -3, 9, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -5, 10, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -3, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, -5, 13, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, -1, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, -1, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -6, 15, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -8, 15, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -3, 9, -4, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, -5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -2, 8, -1, -5, 0, 0, 2},   {0, 0, 0, 0, 0, 0, 0, 6, -8, 3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},      {0, 0, 1, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},      {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -6, 16, -4, -5, 0, 0, 2},  {0, 0, 0, 0, 0, 0, 0, -2, 8, -3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -2, 8, -3, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 6, -8, 1, 5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 5, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 3, -5, 4, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -8, 11, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -8, 11, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, -8, 11, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 3, -3, 0, 2, 0, 0, 0, 2},
        {0, 0, 2, -2, 1, 0, 0, 4, -8, 3, 0, 0, 0, 0},    {0, 0, 1, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 2, -2, 1, 0, 0, -4, 8, -3, 0, 0, 0, 0},   {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, -3, 7, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, -5, 6, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -5, 6, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 5, -6, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, -1, 6, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 7, -9, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -1, 4, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, -1, 4, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -7, 9, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -7, 9, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -4, 4, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 4, -4, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, -3, 0, 5, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, -9, 12, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, -4, 0, 0, 0, 0},     {0, 0, 2, -2, 1, 0, 1, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, -3, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -2, 6, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -6, 7, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 6, -7, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, -2, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 3, -2, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 3, 0, -1, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -2, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, -1, 0, 0, 2},     {0, 0, 2, -2, 1, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, -8, 16, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 3, 0, 2, -5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 7, -8, 3, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, -5, 16, -4, -5, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, -1, 8, -3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -8, 10, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -8, 10, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, -8, 10, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, -3, 8, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -5, 5, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 5, -5, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 6, -5, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 7, -8, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 4, -3, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -9, 11, 0, 0, 0, 0, 0, 2},    {0, 0, 0, 0, 0, 0, -9, 11, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 4, 0, -4, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 4, 0, -3, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, -6, 6, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 0, 4, 0, -2, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, 3, -1, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 4, 0, -1, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, -2, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 5, -2, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 8, -9, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 5, -4, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1},      {0, 0, 0, 0, 0, 0, -7, 7, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 7, -7, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 4, -2, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 5, 0, -4, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 5, 0, -3, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, 0, 5, 0, -2, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, -8, 8, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 8, -8, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 5, -3, 0, 0, 0, 0, 0, 2},     {0, 0, 0, 0, 0, 0, -9, 9, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, -9, 9, 0, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 0, 0, -9, 9, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 9, -9, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 6, -4, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},      {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},      {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 1},      {0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},      {1, 0, 0, -2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {1, 0, 0, -2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},    {1, 0, 0, -2, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {1, 0, 0, -2, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},    {-1, 0, 0, 0, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},    {-1, 0, 0, 2, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {1, 0, 0, -2, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},    {-2, 0, 0, 2, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {-1, 0, 0, 0, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},    {-1, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {-1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},    {-1, 0, 0, 2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},
        {1, 0, -1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},     {-1, 0, 0, 2, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},
        {-2, 0, 0, 0, 0, 0, 0, 2, 0, -3, 0, 0, 0, 0},    {1, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {-1, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0},   {1, 0, 1, -1, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0},
        {-1, 0, 0, 0, 0, 0, 0, 4, -8, 3, 0, 0, 0, 0},    {-1, 0, 0, 2, 1, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},     {-1, 0, 0, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {-1, 0, 0, 2, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {1, 0, 0, -2, 1, 0, 0, -2, 0, 2, 0, 0, 0, 0},
        {1, 0, 2, -2, 2, 0, -3, 3, 0, 0, 0, 0, 0, 0},    {1, 0, 2, -2, 2, 0, 0, -2, 0, 2, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0},     {1, 0, 0, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {0, 0, 0, -2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0},    {0, 0, 0, -2, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0},
        {0, 0, 2, 0, 2, 0, -2, 2, 0, 0, 0, 0, 0, 0},     {0, 0, 2, 0, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},
        {0, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},     {0, 0, 2, 0, 2, 0, -2, 3, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 2, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0},     {0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},      {-1, 0, 2, 0, 2, 0, 10, -3, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},      {1, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},     {0, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},
        {-1, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},   {2, 0, 2, -2, 2, 0, 0, -2, 0, 3, 0, 0, 0, 0},
        {1, 0, 2, 0, 1, 0, 0, -2, 0, 3, 0, 0, 0, 0},     {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {-1, 0, 2, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},     {-2, 0, 2, 2, 2, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {0, 0, 2, 0, 2, 0, 2, -3, 0, 0, 0, 0, 0, 0},     {0, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 2, 0, 2, 0, 0, 1, 0, -1, 0, 0, 0, 0},     {0, 0, 2, 0, 2, 0, 2, -2, 0, 0, 0, 0, 0, 0},
        {-1, 0, 2, 2, 2, 0, 0, -1, 0, 1, 0, 0, 0, 0},    {1, 0, 2, 0, 2, 0, -1, 1, 0, 0, 0, 0, 0, 0},
        {-1, 0, 2, 2, 2, 0, 0, 2, 0, -3, 0, 0, 0, 0},    {2, 0, 2, 0, 2, 0, 0, 2, 0, -3, 0, 0, 0, 0},
        {1, 0, 2, 0, 2, 0, 0, -4, 8, -3, 0, 0, 0, 0},    {1, 0, 2, 0, 2, 0, 0, 4, -8, 3, 0, 0, 0, 0},
        {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},      {0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {2, 0, 2, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},      {-1, 0, 2, 2, 2, 0, 0, 2, 0, -2, 0, 0, 0, 0},
        {-1, 0, 2, 2, 2, 0, 3, -3, 0, 0, 0, 0, 0, 0},    {1, 0, 2, 0, 2, 0, 1, -1, 0, 0, 0, 0, 0, 0},
        {0, 0, 2, 2, 2, 0, 0, 2, 0, -2, 0, 0, 0, 0}};
    for (I = 0; I < iNPL; I++)
        for (J = 0; J < 14; J++)
        {
            iNAPL[J][I] = iNAPLT[I][J];
        }

    // ICPLT is the transposition of ICPL
    int iICPLT[iNPL][4] = {{1440, 0, 0, 0},
                           {56, -117, -42, -40},
                           {125, -43, 0, -54},
                           {0, 5, 0, 0},
                           {3, -7, -3, 0},
                           {3, 0, 0, -2},
                           {-114, 0, 0, 61},
                           {-219, 89, 0, 0},
                           {-3, 0, 0, 0},
                           {-462, 1604, 0, 0},
                           {99, 0, 0, -53},
                           {-3, 0, 0, 2},
                           {0, 6, 2, 0},
                           {3, 0, 0, 0},
                           {-12, 0, 0, 0},
                           {14, -218, 117, 8},
                           {31, -481, -257, -17},
                           {-491, 128, 0, 0},
                           {-3084, 5123, 2735, 1647},
                           {-1444, 2409, -1286, -771},
                           {11, -24, -11, -9},
                           {26, -9, 0, 0},
                           {103, -60, 0, 0},
                           {0, -13, -7, 0},
                           {-26, -29, -16, 14},
                           {9, -27, -14, -5},
                           {12, 0, 0, -6},
                           {-7, 0, 0, 0},
                           {0, 24, 0, 0},
                           {284, 0, 0, -151},
                           {226, 101, 0, 0},
                           {0, -8, -2, 0},
                           {0, -6, -3, 0},
                           {5, 0, 0, -3},
                           {-41, 175, 76, 17},
                           {0, 15, 6, 0},
                           {425, 212, -133, 269},
                           {1200, 598, 319, -641},
                           {235, 334, 0, 0},
                           {11, -12, -7, -6},
                           {5, -6, 3, 3},
                           {-5, 0, 0, 3},
                           {6, 0, 0, -3},
                           {15, 0, 0, 0},
                           {13, 0, 0, -7},
                           {-6, -9, 0, 0},
                           {266, -78, 0, 0},
                           {-460, -435, -232, 246},
                           {0, 15, 7, 0},
                           {-3, 0, 0, 2},
                           {0, 131, 0, 0},
                           {4, 0, 0, 0},
                           {0, 3, 0, 0},
                           {0, 4, 2, 0},
                           {0, 3, 0, 0},
                           {-17, -19, -10, 9},
                           {-9, -11, 6, -5},
                           {-6, 0, 0, 3},
                           {-16, 8, 0, 0},
                           {0, 3, 0, 0},
                           {11, 24, 11, -5},
                           {-3, -4, -2, 1},
                           {3, 0, 0, -1},
                           {0, -8, -4, 0},
                           {0, 3, 0, 0},
                           {0, 5, 0, 0},
                           {0, 3, 2, 0},
                           {-6, 4, 2, 3},
                           {-3, -5, 0, 0},
                           {-5, 0, 0, 2},
                           {4, 24, 13, -2},
                           {-42, 20, 0, 0},
                           {-10, 233, 0, 0},
                           {-3, 0, 0, 1},
                           {78, -18, 0, 0},
                           {0, 3, 1, 0},
                           {0, -3, -1, 0},
                           {0, -4, -2, 1},
                           {0, -8, -4, -1},
                           {0, -5, 3, 0},
                           {-7, 0, 0, 3},
                           {-14, 8, 3, 6},
                           {0, 8, -4, 0},
                           {0, 19, 10, 0},
                           {45, -22, 0, 0},
                           {-3, 0, 0, 0},
                           {0, -3, 0, 0},
                           {0, 3, 0, 0},
                           {3, 5, 3, -2},
                           {89, -16, -9, -48},
                           {0, 3, 0, 0},
                           {-3, 7, 4, 2},
                           {-349, -62, 0, 0},
                           {-15, 22, 0, 0},
                           {-3, 0, 0, 0},
                           {-53, 0, 0, 0},
                           {5, 0, 0, -3},
                           {0, -8, 0, 0},
                           {15, -7, -4, -8},
                           {-3, 0, 0, 1},
                           {-21, -78, 0, 0},
                           {20, -70, -37, -11},
                           {0, 6, 3, 0},
                           {5, 3, 2, -2},
                           {-17, -4, -2, 9},
                           {0, 6, 3, 0},
                           {32, 15, -8, 17},
                           {174, 84, 45, -93},
                           {11, 56, 0, 0},
                           {-66, -12, -6, 35},
                           {47, 8, 4, -25},
                           {0, 8, 4, 0},
                           {10, -22, -12, -5},
                           {-3, 0, 0, 2},
                           {-24, 12, 0, 0},
                           {5, -6, 0, 0},
                           {3, 0, 0, -2},
                           {4, 3, 1, -2},
                           {0, 29, 15, 0},
                           {-5, -4, -2, 2},
                           {8, -3, -1, -5},
                           {0, -3, 0, 0},
                           {10, 0, 0, 0},
                           {3, 0, 0, -2},
                           {-5, 0, 0, 3},
                           {46, 66, 35, -25},
                           {-14, 7, 0, 0},
                           {0, 3, 2, 0},
                           {-5, 0, 0, 0},
                           {-68, -34, -18, 36},
                           {0, 14, 7, 0},
                           {10, -6, -3, -5},
                           {-5, -4, -2, 3},
                           {-3, 5, 2, 1},
                           {76, 17, 9, -41},
                           {84, 298, 159, -45},
                           {3, 0, 0, -1},
                           {-3, 0, 0, 2},
                           {-3, 0, 0, 1},
                           {-82, 292, 156, 44},
                           {-73, 17, 9, 39},
                           {-9, -16, 0, 0},
                           {3, 0, -1, -2},
                           {-3, 0, 0, 0},
                           {-9, -5, -3, 5},
                           {-439, 0, 0, 0},
                           {57, -28, -15, -30},
                           {0, -6, -3, 0},
                           {-4, 0, 0, 2},
                           {-40, 57, 30, 21},
                           {23, 7, 3, -13},
                           {273, 80, 43, -146},
                           {-449, 430, 0, 0},
                           {-8, -47, -25, 4},
                           {6, 47, 25, -3},
                           {0, 23, 13, 0},
                           {-3, 0, 0, 2},
                           {3, -4, -2, -2},
                           {-48, -110, -59, 26},
                           {51, 114, 61, -27},
                           {-133, 0, 0, 57},
                           {0, 4, 0, 0},
                           {-21, -6, -3, 11},
                           {0, -3, -1, 0},
                           {-11, -21, -11, 6},
                           {-18, -436, -233, 9},
                           {35, -7, 0, 0},
                           {0, 5, 3, 0},
                           {11, -3, -1, -6},
                           {-5, -3, -1, 3},
                           {-53, -9, -5, 28},
                           {0, 3, 2, 1},
                           {4, 0, 0, -2},
                           {0, -4, 0, 0},
                           {-50, 194, 103, 27},
                           {-13, 52, 28, 7},
                           {-91, 248, 0, 0},
                           {6, 49, 26, -3},
                           {-6, -47, -25, 3},
                           {0, 5, 3, 0},
                           {52, 23, 10, -23},
                           {-3, 0, 0, 1},
                           {0, 5, 3, 0},
                           {-4, 0, 0, 0},
                           {-4, 8, 3, 2},
                           {10, 0, 0, 0},
                           {3, 0, 0, -2},
                           {0, 8, 4, 0},
                           {0, 8, 4, 1},
                           {-4, 0, 0, 0},
                           {-4, 0, 0, 0},
                           {-8, 4, 2, 4},
                           {8, -4, -2, -4},
                           {0, 15, 7, 0},
                           {-138, 0, 0, 0},
                           {0, -7, -3, 0},
                           {0, -7, -3, 0},
                           {54, 0, 0, -29},
                           {0, 10, 4, 0},
                           {-7, 0, 0, 3},
                           {-37, 35, 19, 20},
                           {0, 4, 0, 0},
                           {-4, 9, 0, 0},
                           {8, 0, 0, -4},
                           {-9, -14, -8, 5},
                           {-3, -9, -5, 3},
                           {-145, 47, 0, 0},
                           {-10, 40, 21, 5},
                           {11, -49, -26, -7},
                           {-2150, 0, 0, 932},
                           {-12, 0, 0, 5},
                           {85, 0, 0, -37},
                           {4, 0, 0, -2},
                           {3, 0, 0, -2},
                           {-86, 153, 0, 0},
                           {-6, 9, 5, 3},
                           {9, -13, -7, -5},
                           {-8, 12, 6, 4},
                           {-51, 0, 0, 22},
                           {-11, -268, -116, 5},
                           {0, 12, 5, 0},
                           {0, 7, 3, 0},
                           {31, 6, 3, -17},
                           {140, 27, 14, -75},
                           {57, 11, 6, -30},
                           {-14, -39, 0, 0},
                           {0, -6, -2, 0},
                           {4, 15, 8, -2},
                           {0, 4, 0, 0},
                           {-3, 0, 0, 1},
                           {0, 11, 5, 0},
                           {9, 6, 0, 0},
                           {-4, 10, 4, 2},
                           {5, 3, 0, 0},
                           {16, 0, 0, -9},
                           {-3, 0, 0, 0},
                           {0, 3, 2, -1},
                           {7, 0, 0, -3},
                           {-25, 22, 0, 0},
                           {42, 223, 119, -22},
                           {-27, -143, -77, 14},
                           {9, 49, 26, -5},
                           {-1166, 0, 0, 505},
                           {-5, 0, 0, 2},
                           {-6, 0, 0, 3},
                           {-8, 0, 1, 4},
                           {0, -4, 0, 0},
                           {117, 0, 0, -63},
                           {-4, 8, 4, 2},
                           {3, 0, 0, -2},
                           {-5, 0, 0, 2},
                           {0, 31, 0, 0},
                           {-5, 0, 1, 3},
                           {4, 0, 0, -2},
                           {-4, 0, 0, 2},
                           {-24, -13, -6, 10},
                           {3, 0, 0, 0},
                           {0, -32, -17, 0},
                           {8, 12, 5, -3},
                           {3, 0, 0, -1},
                           {7, 13, 0, 0},
                           {-3, 16, 0, 0},
                           {50, 0, 0, -27},
                           {0, -5, -3, 0},
                           {13, 0, 0, 0},
                           {0, 5, 3, 1},
                           {24, 5, 2, -11},
                           {5, -11, -5, -2},
                           {30, -3, -2, -16},
                           {18, 0, 0, -9},
                           {8, 614, 0, 0},
                           {3, -3, -1, -2},
                           {6, 17, 9, -3},
                           {-3, -9, -5, 2},
                           {0, 6, 3, -1},
                           {-127, 21, 9, 55},
                           {3, 5, 0, 0},
                           {-6, -10, -4, 3},
                           {5, 0, 0, 0},
                           {16, 9, 4, -7},
                           {3, 0, 0, -2},
                           {0, 22, 0, 0},
                           {0, 19, 10, 0},
                           {7, 0, 0, -4},
                           {0, -5, -2, 0},
                           {0, 3, 1, 0},
                           {-9, 3, 1, 4},
                           {17, 0, 0, -7},
                           {0, -3, -2, -1},
                           {-20, 34, 0, 0},
                           {-10, 0, 1, 5},
                           {-4, 0, 0, 2},
                           {22, -87, 0, 0},
                           {-4, 0, 0, 2},
                           {-3, -6, -2, 1},
                           {-16, -3, -1, 7},
                           {0, -3, -2, 0},
                           {4, 0, 0, 0},
                           {-68, 39, 0, 0},
                           {27, 0, 0, -14},
                           {0, -4, 0, 0},
                           {-25, 0, 0, 0},
                           {-12, -3, -2, 6},
                           {3, 0, 0, -1},
                           {3, 66, 29, -1},
                           {490, 0, 0, -213},
                           {-22, 93, 49, 12},
                           {-7, 28, 15, 4},
                           {-3, 13, 7, 2},
                           {-46, 14, 0, 0},
                           {-5, 0, 0, 0},
                           {2, 1, 0, 0},
                           {0, -3, 0, 0},
                           {-28, 0, 0, 15},
                           {5, 0, 0, -2},
                           {0, 3, 0, 0},
                           {-11, 0, 0, 5},
                           {0, 3, 1, 0},
                           {-3, 0, 0, 1},
                           {25, 106, 57, -13},
                           {5, 21, 11, -3},
                           {1485, 0, 0, 0},
                           {-7, -32, -17, 4},
                           {0, 5, 3, 0},
                           {-6, -3, -2, 3},
                           {30, -6, -2, -13},
                           {-4, 4, 0, 0},
                           {-19, 0, 0, 10},
                           {0, 4, 2, -1},
                           {0, 3, 0, 0},
                           {4, 0, 0, -2},
                           {0, -3, -1, 0},
                           {-3, 0, 0, 0},
                           {5, 3, 1, -2},
                           {0, 11, 0, 0},
                           {118, 0, 0, -52},
                           {0, -5, -3, 0},
                           {-28, 36, 0, 0},
                           {5, -5, 0, 0},
                           {14, -59, -31, -8},
                           {0, 9, 5, 1},
                           {-458, 0, 0, 198},
                           {0, -45, -20, 0},
                           {9, 0, 0, -5},
                           {0, -3, 0, 0},
                           {0, -4, -2, -1},
                           {11, 0, 0, -6},
                           {6, 0, 0, -2},
                           {-16, 23, 0, 0},
                           {0, -4, -2, 0},
                           {-5, 0, 0, 2},
                           {-166, 269, 0, 0},
                           {15, 0, 0, -8},
                           {10, 0, 0, -4},
                           {-78, 45, 0, 0},
                           {0, -5, -2, 0},
                           {7, 0, 0, -4},
                           {-5, 328, 0, 0},
                           {3, 0, 0, -2},
                           {5, 0, 0, -2},
                           {0, 3, 1, 0},
                           {-3, 0, 0, 0},
                           {-3, 0, 0, 0},
                           {0, -4, -2, 0},
                           {-1223, -26, 0, 0},
                           {0, 7, 3, 0},
                           {3, 0, 0, 0},
                           {0, 3, 2, 0},
                           {-6, 20, 0, 0},
                           {-368, 0, 0, 0},
                           {-75, 0, 0, 0},
                           {11, 0, 0, -6},
                           {3, 0, 0, -2},
                           {-3, 0, 0, 1},
                           {-13, -30, 0, 0},
                           {21, 3, 0, 0},
                           {-3, 0, 0, 1},
                           {-4, 0, 0, 2},
                           {8, -27, 0, 0},
                           {-19, -11, 0, 0},
                           {-4, 0, 0, 2},
                           {0, 5, 2, 0},
                           {-6, 0, 0, 2},
                           {-8, 0, 0, 0},
                           {-1, 0, 0, 0},
                           {-14, 0, 0, 6},
                           {6, 0, 0, 0},
                           {-74, 0, 0, 32},
                           {0, -3, -1, 0},
                           {4, 0, 0, -2},
                           {8, 11, 0, 0},
                           {0, 3, 2, 0},
                           {-262, 0, 0, 114},
                           {0, -4, 0, 0},
                           {-7, 0, 0, 4},
                           {0, -27, -12, 0},
                           {-19, -8, -4, 8},
                           {202, 0, 0, -87},
                           {-8, 35, 19, 5},
                           {0, 4, 2, 0},
                           {16, -5, 0, 0},
                           {5, 0, 0, -3},
                           {0, -3, 0, 0},
                           {1, 0, 0, 0},
                           {-35, -48, -21, 15},
                           {-3, -5, -2, 1},
                           {6, 0, 0, -3},
                           {3, 0, 0, -1},
                           {0, -5, 0, 0},
                           {12, 55, 29, -6},
                           {0, 5, 3, 0},
                           {-598, 0, 0, 0},
                           {-3, -13, -7, 1},
                           {-5, -7, -3, 2},
                           {3, 0, 0, -1},
                           {5, -7, 0, 0},
                           {4, 0, 0, -2},
                           {16, -6, 0, 0},
                           {8, -3, 0, 0},
                           {8, -31, -16, -4},
                           {0, 3, 1, 0},
                           {113, 0, 0, -49},
                           {0, -24, -10, 0},
                           {4, 0, 0, -2},
                           {27, 0, 0, 0},
                           {-3, 0, 0, 1},
                           {0, -4, -2, 0},
                           {5, 0, 0, -2},
                           {0, -3, 0, 0},
                           {-13, 0, 0, 6},
                           {5, 0, 0, -2},
                           {-18, -10, -4, 8},
                           {-4, -28, 0, 0},
                           {-5, 6, 3, 2},
                           {-3, 0, 0, 1},
                           {-5, -9, -4, 2},
                           {17, 0, 0, -7},
                           {11, 4, 0, 0},
                           {0, -6, -2, 0},
                           {83, 15, 0, 0},
                           {-4, 0, 0, 2},
                           {0, -114, -49, 0},
                           {117, 0, 0, -51},
                           {-5, 19, 10, 2},
                           {-3, 0, 0, 0},
                           {-3, 0, 0, 2},
                           {0, -3, -1, 0},
                           {3, 0, 0, 0},
                           {0, -6, -2, 0},
                           {393, 3, 0, 0},
                           {-4, 21, 11, 2},
                           {-6, 0, -1, 3},
                           {-3, 8, 4, 1},
                           {8, 0, 0, 0},
                           {18, -29, -13, -8},
                           {8, 34, 18, -4},
                           {89, 0, 0, 0},
                           {3, 12, 6, -1},
                           {54, -15, -7, -24},
                           {0, 3, 0, 0},
                           {3, 0, 0, -1},
                           {0, 35, 0, 0},
                           {-154, -30, -13, 67},
                           {15, 0, 0, 0},
                           {0, 4, 2, 0},
                           {0, 9, 0, 0},
                           {80, -71, -31, -35},
                           {0, -20, -9, 0},
                           {11, 5, 2, -5},
                           {61, -96, -42, -27},
                           {14, 9, 4, -6},
                           {-11, -6, -3, 5},
                           {0, -3, -1, 0},
                           {123, -415, -180, -53},
                           {0, 0, 0, -35},
                           {-5, 0, 0, 0},
                           {7, -32, -17, -4},
                           {0, -9, -5, 0},
                           {0, -4, 2, 0},
                           {-89, 0, 0, 38},
                           {0, -86, -19, -6},
                           {0, 0, -19, 6},
                           {-123, -416, -180, 53},
                           {0, -3, -1, 0},
                           {12, -6, -3, -5},
                           {-13, 9, 4, 6},
                           {0, -15, -7, 0},
                           {3, 0, 0, -1},
                           {-62, -97, -42, 27},
                           {-11, 5, 2, 5},
                           {0, -19, -8, 0},
                           {-3, 0, 0, 1},
                           {0, 4, 2, 0},
                           {0, 3, 0, 0},
                           {0, 4, 2, 0},
                           {-85, -70, -31, 37},
                           {163, -12, -5, -72},
                           {-63, -16, -7, 28},
                           {-21, -32, -14, 9},
                           {0, -3, -1, 0},
                           {3, 0, 0, -2},
                           {0, 8, 0, 0},
                           {3, 10, 4, -1},
                           {3, 0, 0, -1},
                           {0, -7, -3, 0},
                           {0, -4, -2, 0},
                           {6, 19, 0, 0},
                           {5, -173, -75, -2},
                           {0, -7, -3, 0},
                           {7, -12, -5, -3},
                           {-3, 0, 0, 2},
                           {3, -4, -2, -1},
                           {74, 0, 0, -32},
                           {-3, 12, 6, 2},
                           {26, -14, -6, -11},
                           {19, 0, 0, -8},
                           {6, 24, 13, -3},
                           {83, 0, 0, 0},
                           {0, -10, -5, 0},
                           {11, -3, -1, -5},
                           {3, 0, 1, -1},
                           {3, 0, 0, -1},
                           {-4, 0, 0, 0},
                           {5, -23, -12, -3},
                           {-339, 0, 0, 147},
                           {0, -10, -5, 0},
                           {5, 0, 0, 0},
                           {3, 0, 0, -1},
                           {0, -4, -2, 0},
                           {18, -3, 0, 0},
                           {9, -11, -5, -4},
                           {-8, 0, 0, 4},
                           {3, 0, 0, -1},
                           {0, 9, 0, 0},
                           {6, -9, -4, -2},
                           {-4, -12, 0, 0},
                           {67, -91, -39, -29},
                           {30, -18, -8, -13},
                           {0, 0, 0, 0},
                           {0, -114, -50, 0},
                           {0, 0, 0, 23},
                           {517, 16, 7, -224},
                           {0, -7, -3, 0},
                           {143, -3, -1, -62},
                           {29, 0, 0, -13},
                           {-4, 0, 0, 2},
                           {-6, 0, 0, 3},
                           {5, 12, 5, -2},
                           {-25, 0, 0, 11},
                           {-3, 0, 0, 1},
                           {0, 4, 2, 0},
                           {-22, 12, 5, 10},
                           {50, 0, 0, -22},
                           {0, 7, 4, 0},
                           {0, 3, 1, 0},
                           {-4, 4, 2, 2},
                           {-5, -11, -5, 2},
                           {0, 4, 2, 0},
                           {4, 17, 9, -2},
                           {59, 0, 0, 0},
                           {0, -4, -2, 0},
                           {-8, 0, 0, 4},
                           {-3, 0, 0, 0},
                           {4, -15, -8, -2},
                           {370, -8, 0, -160},
                           {0, 0, -3, 0},
                           {0, 3, 1, 0},
                           {-6, 3, 1, 3},
                           {0, 6, 0, 0},
                           {-10, 0, 0, 4},
                           {0, 9, 4, 0},
                           {4, 17, 7, -2},
                           {34, 0, 0, -15},
                           {0, 5, 3, 0},
                           {-5, 0, 0, 2},
                           {-37, -7, -3, 16},
                           {3, 13, 7, -2},
                           {40, 0, 0, 0},
                           {0, -3, -2, 0},
                           {-184, -3, -1, 80},
                           {-3, 0, 0, 1},
                           {-3, 0, 0, 0},
                           {0, -10, -6, -1},
                           {31, -6, 0, -13},
                           {-3, -32, -14, 1},
                           {-7, 0, 0, 3},
                           {0, -8, -4, 0},
                           {3, -4, 0, 0},
                           {0, 4, 0, 0},
                           {0, 3, 1, 0},
                           {19, -23, -10, 2},
                           {0, 0, 0, -10},
                           {0, 3, 2, 0},
                           {0, 9, 5, -1},
                           {28, 0, 0, 0},
                           {0, -7, -4, 0},
                           {8, -4, 0, -4},
                           {0, 0, -2, 0},
                           {0, 3, 0, 0},
                           {-3, 0, 0, 1},
                           {-9, 0, 1, 4},
                           {3, 12, 5, -1},
                           {17, -3, -1, 0},
                           {0, 7, 4, 0},
                           {19, 0, 0, 0},
                           {0, -5, -3, 0},
                           {14, -3, 0, -1},
                           {0, 0, -1, 0},
                           {0, 0, 0, -5},
                           {0, 5, 3, 0},
                           {13, 0, 0, 0},
                           {0, -3, -2, 0},
                           {2, 9, 4, 3},
                           {0, 0, 0, -4},
                           {8, 0, 0, 0},
                           {0, 4, 2, 0},
                           {6, 0, 0, -3},
                           {6, 0, 0, 0},
                           {0, 3, 1, 0},
                           {5, 0, 0, -2},
                           {3, 0, 0, -1},
                           {-3, 0, 0, 0},
                           {6, 0, 0, 0},
                           {7, 0, 0, 0},
                           {-4, 0, 0, 0},
                           {4, 0, 0, 0},
                           {6, 0, 0, 0},
                           {0, -4, 0, 0},
                           {0, -4, 0, 0},
                           {5, 0, 0, 0},
                           {-3, 0, 0, 0},
                           {4, 0, 0, 0},
                           {-5, 0, 0, 0},
                           {4, 0, 0, 0},
                           {0, 3, 0, 0},
                           {13, 0, 0, 0},
                           {21, 11, 0, 0},
                           {0, -5, 0, 0},
                           {0, -5, -2, 0},
                           {0, 5, 3, 0},
                           {0, -5, 0, 0},
                           {-3, 0, 0, 2},
                           {20, 10, 0, 0},
                           {-34, 0, 0, 0},
                           {-19, 0, 0, 0},
                           {3, 0, 0, -2},
                           {-3, 0, 0, 1},
                           {-6, 0, 0, 3},
                           {-4, 0, 0, 0},
                           {3, 0, 0, 0},
                           {3, 0, 0, 0},
                           {4, 0, 0, 0},
                           {3, 0, 0, -1},
                           {6, 0, 0, -3},
                           {-8, 0, 0, 3},
                           {0, 3, 1, 0},
                           {-3, 0, 0, 0},
                           {0, -3, -2, 0},
                           {126, -63, -27, -55},
                           {-5, 0, 1, 2},
                           {-3, 28, 15, 2},
                           {5, 0, 1, -2},
                           {0, 9, 4, 1},
                           {0, 9, 4, -1},
                           {-126, -63, -27, 55},
                           {3, 0, 0, -1},
                           {21, -11, -6, -11},
                           {0, -4, 0, 0},
                           {-21, -11, -6, 11},
                           {-3, 0, 0, 1},
                           {0, 3, 1, 0},
                           {8, 0, 0, -4},
                           {-6, 0, 0, 3},
                           {-3, 0, 0, 1},
                           {3, 0, 0, -1},
                           {-3, 0, 0, 1},
                           {-5, 0, 0, 2},
                           {24, -12, -5, -11},
                           {0, 3, 1, 0},
                           {0, 3, 1, 0},
                           {0, 3, 2, 0},
                           {-24, -12, -5, 10},
                           {4, 0, -1, -2},
                           {13, 0, 0, -6},
                           {7, 0, 0, -3},
                           {3, 0, 0, -1},
                           {3, 0, 0, -1}};
    for (I = 0; I < iNPL; I++)
        for (J = 0; J < 4; J++)
        {
            iICPL[J][I] = iICPLT[I][J];
        }
    dT = ((dATE1 - dDJ0) + dATE2) / dDJC;

    /***LUNI-SOLAR NUTATION***/

    dEL = fmod(485868.249036e0 + dT * (1717915923.2178e0 + dT * (31.8792e0 + dT * (0.051635e0 + dT * (-0.00024470e0)))),
               dTURNAS) *
          dDAS2R;
    dELP = fmod(1287104.79305e0 + dT * (129596581.0481e0 + dT * (-0.5532e0 + dT * (0.000136e0 + dT * (-0.00001149e0)))),
                dTURNAS) *
           dDAS2R;
    dF = fmod(335779.526232e0 + dT * (1739527262.8478e0 + dT * (-12.7512e0 + dT * (-0.001037e0 + dT * (0.00000417e0)))),
              dTURNAS) *
         dDAS2R;
    dD = fmod(1072260.70369e0 + dT * (1602961601.2090e0 + dT * (-6.3706e0 + dT * (0.006593e0 + dT * (-0.00003169e0)))),
              dTURNAS) *
         dDAS2R;
    dOM = fmod(450160.398036e0 + dT * (-6962890.5431e0 + dT * (7.4722e0 + dT * (0.007702e0 + dT * (-0.00005939e0)))),
               dTURNAS) *
          dDAS2R;
    dDP = 0e0;
    dDE = 0e0;
    for (I = iNLS - 1; I >= 0; I--)
    {
        dARG = fmod(iNALS[0][I] * dEL + iNALS[1][I] * dELP + iNALS[2][I] * dF + iNALS[3][I] * dD + iNALS[4][I] * dOM,
                    d2PI);
        dSARG = sin(dARG);
        dCARG = cos(dARG);

        dDP = dDP + (dCLS[0][I] + dCLS[1][I] * dT) * dSARG + dCLS[2][I] * dCARG;
        dDE = dDE + (dCLS[3][I] + dCLS[4][I] * dT) * dCARG + dCLS[5][I] * dSARG;
    }
    dDPSILS = dDP * dU2R;
    dDEPSLS = dDE * dU2R;
    dAL = fmod(2.35555598e0 + 8328.6914269554e0 * dT, d2PI);
    dALSU = fmod(6.24006013e0 + 628.301955e0 * dT, d2PI);
    dAF = fmod(1.627905234e0 + 8433.466158131e0 * dT, d2PI);
    dAD = fmod(5.198466741e0 + 7771.3771468121e0 * dT, d2PI);
    dAOM = fmod(2.18243920e0 - 33.757045e0 * dT, d2PI);
    dAPA = (0.02438175e0 + 0.00000538691e0 * dT) * dT;
    dALME = fmod(4.402608842e0 + 2608.7903141574e0 * dT, d2PI);
    dALVE = fmod(3.176146697e0 + 1021.3285546211e0 * dT, d2PI);
    dALEA = fmod(1.753470314e0 + 628.3075849991e0 * dT, d2PI);
    dALMA = fmod(6.203480913e0 + 334.0612426700e0 * dT, d2PI);
    dALJU = fmod(0.599546497e0 + 52.9690962641e0 * dT, d2PI);
    dALSA = fmod(0.874016757e0 + 21.3299104960e0 * dT, d2PI);
    dALUR = fmod(5.481293871e0 + 7.4781598567e0 * dT, d2PI);
    dALNE = fmod(5.321159000e0 + 3.8127774000e0 * dT, d2PI);

    dDP = 0e0;
    dDE = 0e0;

    for (I = iNPL - 1; I >= 0; I--)
    {
        dARG = fmod(iNAPL[0][I] * dAL + iNAPL[1][I] * dALSU + iNAPL[2][I] * dAF + iNAPL[3][I] * dAD +
                        iNAPL[4][I] * dAOM + iNAPL[5][I] * dALME + iNAPL[6][I] * dALVE + iNAPL[7][I] * dALEA +
                        iNAPL[8][I] * dALMA + iNAPL[9][I] * dALJU + iNAPL[10][I] * dALSA + iNAPL[11][I] * dALUR +
                        iNAPL[12][I] * dALNE + iNAPL[13][I] * dAPA,
                    d2PI);
        dSARG = sin(dARG);
        dCARG = cos(dARG);

        dDP = dDP + iICPL[0][I] * dSARG + iICPL[1][I] * dCARG;
        dDE = dDE + iICPL[2][I] * dSARG + iICPL[3][I] * dCARG;
    }
    dDPSIPL = dDP * dU2R;
    dDEPSPL = dDE * dU2R;

    double temp1 = dDPSIPL + dDPSILS;
    *dPSI = temp1;
    double temp2 = dDEPSPL + dDEPSLS;
    *dEPS = temp2;
    delete[] iNALS;
    delete[] dCLS;
    delete[] iNAPL;
    delete[] iICPL; // modified by zhuyt
}

void t_gtrs2crs::_nutCal_06(double dATE1, double dATE2, double *dPSI, double *dEPS)
{
    // Obtain IAU 2000A nutation.
    _nutCal(dATE1, dATE2, dPSI, dEPS);

    // Interval between fundamental date J2000.0 and given date(JC).
    double T = ((dATE1 - dJ0) + dATE2) / dJC;
    // Factor correcting for secular variation of J2.
    double FJ2 = -2.7774e-6 * T;

    // Apply P03 adjustments(Wallace & Capitaine, 2006, Eqs.5).
    double DP = *dPSI;
    double DE = *dEPS;
    *dPSI = DP + DP * (0.4697e-6 + FJ2);
    *dEPS = DE + DE * FJ2;
}

void t_gtrs2crs::_nutFCN(const double &dRmjd, double *dX, double *dY, double *sigdX, double *sigdY)
{
    double MJD0 = 51544.5;

    // Mean prediction error
    double MPE = 0.1325;                             // microarcseconds per day
                                                     // FCN parameters
    double PER = -430.21;                            // period in days
    double PHI = (2.0 * dPI / PER) * (dRmjd - MJD0); // phase in radians

    // Table for the coefficients of the empirical model of the retrograde FCN during 1984-2013 (uas)
    const int N = 30;
    double TABLE[N][4] = {
        {45700.0, 4.55, -36.58, 19.72},     // 1984.0
        {46066.0, -141.82, -105.35, 11.12}, // 1985.0
        {46431.0, -246.56, -170.21, 9.47},  // 1986.0
        {46796.0, -281.89, -159.24, 8.65},  // 1987.0
        {47161.0, -255.05, -43.58, 8.11},   // 1988.0
        {47527.0, -210.46, -88.56, 7.31},   // 1989.0
        {47892.0, -187.79, -57.35, 6.41},   // 1990.0
        {48257.0, -163.01, 26.26, 5.52},    // 1991.0
        {48622.0, -145.53, 44.65, 4.80},    // 1992.0
        {48988.0, -145.12, 51.49, 5.95},    // 1993.0
        {49353.0, -109.93, 16.87, 9.45},    // 1994.0
        {49718.0, -87.30, 5.36, 8.25},      // 1995.0
        {50083.0, -90.61, 1.52, 7.67},      // 1996.0
        {50449.0, -94.73, 35.35, 4.40},     // 1997.0
        {50814.0, -67.52, 27.57, 3.40},     // 1998.0
        {51179.0, -44.11, -14.31, 3.45},    // 1999.0
        {51544.0, 5.21, -74.87, 3.26},      // 2000.0
        {51910.0, 70.37, -129.66, 2.86},    // 2001.0
        {52275.0, 86.47, -127.84, 2.75},    // 2002.0
        {52640.0, 110.44, -42.73, 2.59},    // 2003.0
        {53005.0, 114.78, -0.13, 2.53},     // 2004.0
        {53371.0, 132.96, -4.78, 2.72},     // 2005.0
        {53736.0, 157.36, 28.63, 2.19},     // 2006.0
        {54101.0, 160.40, 58.87, 1.87},     // 2007.0
        {54466.0, 156.76, 101.24, 1.74},    // 2008.0
        {54832.0, 142.99, 143.01, 1.89},    // 2009.0
        {55197.0, 33.70, 184.46, 1.95},     // 2010.0
        {55562.0, 0.76, 253.70, 1.14},      // 2011.0
        {55927.0, 25.47, 271.66, 1.07},     // 2012.0
        {56293.0, 113.42, 256.50, 1.86}     // 2013.0
    };

    // Amplitudes extracted from the table
    double DATE[N], XC[N], XS[N], YC[N], YS[N], SX[N], SY[N];
    for (int I = 0; I < N; I++)
    {
        DATE[I] = TABLE[I][0];
        XC[I] = TABLE[I][1];
        XS[I] = TABLE[I][2];
        SX[I] = TABLE[I][3];
        YC[I] = XS[I];
        YS[I] = -XC[I];
        SY[I] = SX[I];
    }

    // Prediction of the amplitude at the input date
    double AXC, AXS, AYC, AYS;
    if (dRmjd <= DATE[0])
    {
        AXC = XC[0];
        AXS = XS[0];
        AYC = YC[0];
        AYS = YS[0];
    }
    else if (dRmjd >= DATE[N - 1])
    {
        AXC = XC[N - 1];
        AXS = XS[N - 1];
        AYC = YC[N - 1];
        AYS = YS[N - 1];
    }
    else
    {
        for (int I = 0; I <= N - 2; I++)
        {
            if (dRmjd >= DATE[I] && dRmjd < DATE[I + 1])
            {
                double T = dRmjd - DATE[I];
                double DT = DATE[I + 1] - DATE[I];
                double DAXC = XC[I + 1] - XC[I];
                double DAXS = XS[I + 1] - XS[I];
                double DAYC = YC[I + 1] - YC[I];
                double DAYS = YS[I + 1] - YS[I];
                AXC = XC[I] + (DAXC / DT) * T;
                AXS = XS[I] + (DAXS / DT) * T;
                AYC = YC[I] + (DAYC / DT) * T;
                AYS = YS[I] + (DAYS / DT) * T;
            }
        }
    }

    // Computation of X and Y
    double X = AXC * cos(PHI) - AXS * sin(PHI); // in uasec
    double Y = AYC * cos(PHI) - AYS * sin(PHI); // in uasec

    // Prediction of the uncertainty at the input date
    if (dRmjd <= DATE[0])
    {
        AXC = SX[0] + MPE * (DATE[0] - dRmjd);
        AXS = SX[0] + MPE * (DATE[0] - dRmjd);
        AYC = SY[0] + MPE * (DATE[0] - dRmjd);
        AYS = SY[0] + MPE * (DATE[0] - dRmjd);
    }
    else if (dRmjd >= DATE[N - 1])
    {
        AXC = SX[N - 1] + MPE * (dRmjd - DATE[N - 1]);
        AXS = SX[N - 1] + MPE * (dRmjd - DATE[N - 1]);
        AYC = SY[N - 1] + MPE * (dRmjd - DATE[N - 1]);
        AYS = SY[N - 1] + MPE * (dRmjd - DATE[N - 1]);
    }
    else
    {
        for (int I = 0; I <= N - 2; I++)
        {
            if (dRmjd >= DATE[I] && dRmjd < DATE[I + 1])
            {
                double T = dRmjd - DATE[I];
                double DT = DATE[I + 1] - DATE[I];
                double DAXC = SX[I + 1] - SX[I];
                double DAXS = SX[I + 1] - SX[I];
                double DAYC = SY[I + 1] - SY[I];
                double DAYS = SY[I + 1] - SY[I];
                AXC = abs(SX[I] + (DAXC / DT) * T);
                AXS = abs(SX[I] + (DAXS / DT) * T);
                AYC = abs(SY[I] + (DAYC / DT) * T);
                AYS = abs(SY[I] + (DAYS / DT) * T);
            }
        }
    }

    // Computation of the uncertainties
    double Xuncertainty = AXC + AXS; // uasec
    double Yuncertainty = AYC + AYS; // uasec

    *dX = X;
    *dY = Y;
    *sigdX = Xuncertainty;
    *sigdY = Yuncertainty;
}

double t_gtrs2crs::_iau_CIO_locator(double date1, double date2, double x, double y)
{
    /* Time since J2000.0, in Julian centuries */
    double t;

    /* Miscellaneous */
    int i, j;
    double a, w0, w1, w2, w3, w4, w5;

    /* Fundamental arguments */
    double fa[8] = {0.0};

    /* Returned value */
    double s;

    /* --------------------- */
    /* The series for s+XY/2 */
    /* --------------------- */
    /* Polynomial coefficients */
    double sp[6] = {/* 1-6 */
                    94.00e-6, 3808.35e-6, -119.94e-6, -72574.09e-6, 27.70e-6, 15.61e-6};
    /* Terms of order t^0 */
    double s0[33][10] = {/* 1-20 */
                         {0, 0, 0, 0, 1, 0, 0, 0, -2640.73e-6, 0.39e-6},
                         {0, 0, 0, 0, 2, 0, 0, 0, -63.53e-6, 0.02e-6},
                         {0, 0, 2, -2, 3, 0, 0, 0, -11.75e-6, -0.01e-6},
                         {0, 0, 2, -2, 1, 0, 0, 0, -11.21e-6, -0.01e-6},
                         {0, 0, 2, -2, 2, 0, 0, 0, 4.57e-6, 0.00e-6},
                         {0, 0, 2, 0, 3, 0, 0, 0, -2.02e-6, 0.00e-6},
                         {0, 0, 2, 0, 1, 0, 0, 0, -1.98e-6, 0.00e-6},
                         {0, 0, 0, 0, 3, 0, 0, 0, 1.72e-6, 0.00e-6},
                         {0, 1, 0, 0, 1, 0, 0, 0, 1.41e-6, 0.01e-6},
                         {0, 1, 0, 0, -1, 0, 0, 0, 1.26e-6, 0.01e-6},
                         {1, 0, 0, 0, -1, 0, 0, 0, 0.63e-6, 0.00e-6},
                         {1, 0, 0, 0, 1, 0, 0, 0, 0.63e-6, 0.00e-6},
                         {0, 1, 2, -2, 3, 0, 0, 0, -0.46e-6, 0.00e-6},
                         {0, 1, 2, -2, 1, 0, 0, 0, -0.45e-6, 0.00e-6},
                         {0, 0, 4, -4, 4, 0, 0, 0, -0.36e-6, 0.00e-6},
                         {0, 0, 1, -1, 1, -8, 12, 0, 0.24e-6, 0.12e-6},
                         {0, 0, 2, 0, 0, 0, 0, 0, -0.32e-6, 0.00e-6},
                         {0, 0, 2, 0, 2, 0, 0, 0, -0.28e-6, 0.00e-6},
                         {1, 0, 2, 0, 3, 0, 0, 0, -0.27e-6, 0.00e-6},
                         {1, 0, 2, 0, 1, 0, 0, 0, -0.26e-6, 0.00e-6},
                         /* 21-33 */
                         {0, 0, 2, -2, 0, 0, 0, 0, 0.21e-6, 0.00e-6},
                         {0, 1, -2, 2, -3, 0, 0, 0, -0.19e-6, 0.00e-6},
                         {0, 1, -2, 2, -1, 0, 0, 0, -0.18e-6, 0.00e-6},
                         {0, 0, 0, 0, 0, 8, -13, -1, 0.10e-6, -0.05e-6},
                         {0, 0, 0, 2, 0, 0, 0, 0, -0.15e-6, 0.00e-6},
                         {2, 0, -2, 0, -1, 0, 0, 0, 0.14e-6, 0.00e-6},
                         {0, 1, 2, -2, 2, 0, 0, 0, 0.14e-6, 0.00e-6},
                         {1, 0, 0, -2, 1, 0, 0, 0, -0.14e-6, 0.00e-6},
                         {1, 0, 0, -2, -1, 0, 0, 0, -0.14e-6, 0.00e-6},
                         {0, 0, 4, -2, 4, 0, 0, 0, -0.13e-6, 0.00e-6},
                         {0, 0, 2, -2, 4, 0, 0, 0, 0.11e-6, 0.00e-6},
                         {1, 0, -2, 0, -3, 0, 0, 0, -0.11e-6, 0.00e-6},
                         {1, 0, -2, 0, -1, 0, 0, 0, -0.11e-6, 0.00e-6}};
    /* Terms of order t^1 */
    double s1[3][10] = {/* 1-3 */
                        {0, 0, 0, 0, 2, 0, 0, 0, -0.07e-6, 3.57e-6},
                        {0, 0, 0, 0, 1, 0, 0, 0, 1.71e-6, -0.03e-6},
                        {0, 0, 2, -2, 3, 0, 0, 0, 0.00e-6, 0.48e-6}};
    /* Terms of order t^2 */
    double s2[25][10] = {/* 1-20 */
                         {0, 0, 0, 0, 1, 0, 0, 0, 743.53e-6, -0.17e-6},
                         {0, 0, 2, -2, 2, 0, 0, 0, 56.91e-6, 0.06e-6},
                         {0, 0, 2, 0, 2, 0, 0, 0, 9.84e-6, -0.01e-6},
                         {0, 0, 0, 0, 2, 0, 0, 0, -8.85e-6, 0.01e-6},
                         {0, 1, 0, 0, 0, 0, 0, 0, -6.38e-6, -0.05e-6},
                         {1, 0, 0, 0, 0, 0, 0, 0, -3.07e-6, 0.00e-6},
                         {0, 1, 2, -2, 2, 0, 0, 0, 2.23e-6, 0.00e-6},
                         {0, 0, 2, 0, 1, 0, 0, 0, 1.67e-6, 0.00e-6},
                         {1, 0, 2, 0, 2, 0, 0, 0, 1.30e-6, 0.00e-6},
                         {0, 1, -2, 2, -2, 0, 0, 0, 0.93e-6, 0.00e-6},
                         {1, 0, 0, -2, 0, 0, 0, 0, 0.68e-6, 0.00e-6},
                         {0, 0, 2, -2, 1, 0, 0, 0, -0.55e-6, 0.00e-6},
                         {1, 0, -2, 0, -2, 0, 0, 0, 0.53e-6, 0.00e-6},
                         {0, 0, 0, 2, 0, 0, 0, 0, -0.27e-6, 0.00e-6},
                         {1, 0, 0, 0, 1, 0, 0, 0, -0.27e-6, 0.00e-6},
                         {1, 0, -2, -2, -2, 0, 0, 0, -0.26e-6, 0.00e-6},
                         {1, 0, 0, 0, -1, 0, 0, 0, -0.25e-6, 0.00e-6},
                         {1, 0, 2, 0, 1, 0, 0, 0, 0.22e-6, 0.00e-6},
                         {2, 0, 0, -2, 0, 0, 0, 0, -0.21e-6, 0.00e-6},
                         {2, 0, -2, 0, -1, 0, 0, 0, 0.20e-6, 0.00e-6},
                         /* 21-25 */
                         {0, 0, 2, 2, 2, 0, 0, 0, 0.17e-6, 0.00e-6},
                         {2, 0, 2, 0, 2, 0, 0, 0, 0.13e-6, 0.00e-6},
                         {2, 0, 0, 0, 0, 0, 0, 0, -0.13e-6, 0.00e-6},
                         {1, 0, 2, -2, 2, 0, 0, 0, -0.12e-6, 0.00e-6},
                         {0, 0, 2, 0, 0, 0, 0, 0, -0.11e-6, 0.00e-6}};
    /* Terms of order t^3 */
    double s3[4][10] = {/* 1-4 */
                        {0, 0, 0, 0, 1, 0, 0, 0, 0.30e-6, -23.51e-6},
                        {0, 0, 2, -2, 2, 0, 0, 0, -0.03e-6, -1.39e-6},
                        {0, 0, 2, 0, 2, 0, 0, 0, -0.01e-6, -0.24e-6},
                        {0, 0, 0, 0, 2, 0, 0, 0, 0.00e-6, 0.22e-6}};
    /* Terms of order t^4 */
    double s4[1][10] = {/* 1-1 */
                        {0, 0, 0, 0, 1, 0, 0, 0, -0.26e-6, -0.01e-6}};

    /* Modification for IAU2006 */
    if (_cver.find("06") != string::npos)
    {
        sp[0] = 94.00e-6;
        sp[1] = 3808.65e-6;
        sp[2] = -122.68e-6;
        sp[3] = -72574.11e-6;
        sp[4] = 27.98e-6;
        sp[5] = 15.62e-6;
        s1[1][8] = 1.73e-6;
        s2[0][8] = 743.52e-6;
        s3[0][9] = -23.42e-6;
        s3[1][9] = -1.46e-6;
        s3[2][9] = -0.25e-6;
        s3[3][9] = 0.23e-6;
    }

    /* Number of terms in the series */
    int NS0 = 33;
    int NS1 = 3;
    int NS2 = 25;
    int NS3 = 4;
    int NS4 = 1;

    /* Interval between fundamental epoch J2000.0 and current date (JC). */
    t = ((date1 - dJ0) + date2) / dJC;

    /* Fundamental Arguments (from IERS Conventions 2003) */
    /* Mean anomaly of the Moon. */
    fa[0] = _iauFal03(t);
    /* Mean anomaly of the Sun. */
    fa[1] = _iauFalp03(t);
    /* Mean longitude of the Moon minus that of the ascending node. */
    fa[2] = _iauFaf03(t);
    /* Mean elongation of the Moon from the Sun. */
    fa[3] = _iauFad03(t);
    /* Mean longitude of the ascending node of the Moon. */
    fa[4] = _iauFaom03(t);
    /* Mean longitude of Venus. */
    fa[5] = _iauFave03(t);
    /* Mean longitude of Earth. */
    fa[6] = _iauFae03(t);
    /* General precession in longitude. */
    fa[7] = _iauFapa03(t);

    /* Evaluate s. */
    w0 = sp[0];
    w1 = sp[1];
    w2 = sp[2];
    w3 = sp[3];
    w4 = sp[4];
    w5 = sp[5];

    for (i = NS0 - 1; i >= 0; i--)
    {
        a = 0.0;
        for (j = 0; j < 8; j++)
        {
            a += s0[i][j] * fa[j];
        }
        w0 += s0[i][8] * sin(a) + s0[i][9] * cos(a);
    }
    for (i = NS1 - 1; i >= 0; i--)
    {
        a = 0.0;
        for (j = 0; j < 8; j++)
        {
            a += s1[i][j] * fa[j];
        }
        w1 += s1[i][8] * sin(a) + s1[i][9] * cos(a);
    }
    for (i = NS2 - 1; i >= 0; i--)
    {
        a = 0.0;
        for (j = 0; j < 8; j++)
        {
            a += s2[i][j] * fa[j];
        }
        w2 += s2[i][8] * sin(a) + s2[i][9] * cos(a);
    }
    for (i = NS3 - 1; i >= 0; i--)
    {
        a = 0.0;
        for (j = 0; j < 8; j++)
        {
            a += s3[i][j] * fa[j];
        }
        w3 += s3[i][8] * sin(a) + s3[i][9] * cos(a);
    }
    for (i = NS4 - 1; i >= 0; i--)
    {
        a = 0.0;
        for (j = 0; j < 8; j++)
        {
            a += s4[i][j] * fa[j];
        }
        w4 += s4[i][8] * sin(a) + s4[i][9] * cos(a);
    }

    s = (w0 + (w1 + (w2 + (w3 + (w4 + w5 * t) * t) * t) * t) * t) * dAS2R - x * y / 2.0;
    return s;
}

double t_gtrs2crs::_iau_Eors(Matrix rnpb, double s)
{
    double x, ax, xs, ys, zs, p, q, eo;

    /* Evaluate Wallace & Capitaine (2006) expression (16). */
    x = rnpb(3, 1);
    ax = x / (1.0 + rnpb(3, 3));
    xs = 1.0 - ax * x;
    ys = -ax * rnpb(3, 2);
    zs = -x;
    p = rnpb(1, 1) * xs + rnpb(1, 2) * ys + rnpb(1, 3) * zs;
    q = rnpb(2, 1) * xs + rnpb(2, 2) * ys + rnpb(2, 3) * zs;
    eo = ((p != 0) || (q != 0)) ? s - atan2(q, p) : s;

    return eo;
}

} // namespace great
