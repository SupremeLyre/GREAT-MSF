
/* ----------------------------------------------------------------------
 * G-Nut - GNSS software development library
 *
  (c) 2018 G-Nut Software s.r.o. (software@gnutsoftware.com)

  This file is part of the G-Nut C++ library.

-*/

#include "gmodels/ggpt.h"
#include "gutils/gconst.h"

namespace gnut
{

double t_gpt::a_geoid[55] = {
    -5.6195e-001, -6.0794e-002, -2.0125e-001, -6.4180e-002, -3.6997e-002, +1.0098e+001, +1.6436e+001, +1.4065e+001,
    +1.9881e+000, +6.4414e-001, -4.7482e+000, -3.2290e+000, +5.0652e-001, +3.8279e-001, -2.6646e-002, +1.7224e+000,
    -2.7970e-001, +6.8177e-001, -9.6658e-002, -1.5113e-002, +2.9206e-003, -3.4621e+000, -3.8198e-001, +3.2306e-002,
    +6.9915e-003, -2.3068e-003, -1.3548e-003, +4.7324e-006, +2.3527e+000, +1.2985e+000, +2.1232e-001, +2.2571e-002,
    -3.7855e-003, +2.9449e-005, -1.6265e-004, +1.1711e-007, +1.6732e+000, +1.9858e-001, +2.3975e-002, -9.0013e-004,
    -2.2475e-003, -3.3095e-005, -1.2040e-005, +2.2010e-006, -1.0083e-006, +8.6297e-001, +5.8231e-001, +2.0545e-002,
    -7.8110e-003, -1.4085e-004, -8.8459e-006, +5.7256e-006, -1.5068e-006, +4.0095e-007, -2.4185e-008};

double t_gpt::b_geoid[55] = {
    +0.0000e+000, +0.0000e+000, -6.5993e-002, +0.0000e+000, +6.5364e-002, -5.8320e+000, +0.0000e+000, +1.6961e+000,
    -1.3557e+000, +1.2694e+000, +0.0000e+000, -2.9310e+000, +9.4805e-001, -7.6243e-002, +4.1076e-002, +0.0000e+000,
    -5.1808e-001, -3.4583e-001, -4.3632e-002, +2.2101e-003, -1.0663e-002, +0.0000e+000, +1.0927e-001, -2.9463e-001,
    +1.4371e-003, -1.1452e-002, -2.8156e-003, -3.5330e-004, +0.0000e+000, +4.4049e-001, +5.5653e-002, -2.0396e-002,
    -1.7312e-003, +3.5805e-005, +7.2682e-005, +2.2535e-006, +0.0000e+000, +1.9502e-002, +2.7919e-002, -8.1812e-003,
    +4.4540e-004, +8.8663e-005, +5.5596e-005, +2.4826e-006, +1.0279e-006, +0.0000e+000, +6.0529e-002, -3.5824e-002,
    -5.1367e-003, +3.0119e-005, -2.9911e-005, +1.9844e-005, -1.2349e-006, -7.6756e-009, +5.0100e-008};

double t_gpt::ap_mean[55] = {
    +1.0108e+003, +8.4886e+000, +1.4799e+000, -1.3897e+001, +3.7516e-003, -1.4936e-001, +1.2232e+001, -7.6615e-001,
    -6.7699e-002, +8.1002e-003, -1.5874e+001, +3.6614e-001, -6.7807e-002, -3.6309e-003, +5.9966e-004, +4.8163e+000,
    -3.7363e-001, -7.2071e-002, +1.9998e-003, -6.2385e-004, -3.7916e-004, +4.7609e+000, -3.9534e-001, +8.6667e-003,
    +1.1569e-002, +1.1441e-003, -1.4193e-004, -8.5723e-005, +6.5008e-001, -5.0889e-001, -1.5754e-002, -2.8305e-003,
    +5.7458e-004, +3.2577e-005, -9.6052e-006, -2.7974e-006, +1.3530e+000, -2.7271e-001, -3.0276e-004, +3.6286e-003,
    -2.0398e-004, +1.5846e-005, -7.7787e-006, +1.1210e-006, +9.9020e-008, +5.5046e-001, -2.7312e-001, +3.2532e-003,
    -2.4277e-003, +1.1596e-004, +2.6421e-007, -1.3263e-006, +2.7322e-007, +1.4058e-007, +4.9414e-009};

double t_gpt::bp_mean[55] = {
    +0.0000e+000, +0.0000e+000, -1.2878e+000, +0.0000e+000, +7.0444e-001, +3.3222e-001, +0.0000e+000, -2.9636e-001,
    +7.2248e-003, +7.9655e-003, +0.0000e+000, +1.0854e+000, +1.1145e-002, -3.6513e-002, +3.1527e-003, +0.0000e+000,
    -4.8434e-001, +5.2023e-002, -1.3091e-002, +1.8515e-003, +1.5422e-004, +0.0000e+000, +6.8298e-001, +2.5261e-003,
    -9.9703e-004, -1.0829e-003, +1.7688e-004, -3.1418e-005, +0.0000e+000, -3.7018e-001, +4.3234e-002, +7.2559e-003,
    +3.1516e-004, +2.0024e-005, -8.0581e-006, -2.3653e-006, +0.0000e+000, +1.0298e-001, -1.5086e-002, +5.6186e-003,
    +3.2613e-005, +4.0567e-005, -1.3925e-006, -3.6219e-007, -2.0176e-008, +0.0000e+000, -1.8364e-001, +1.8508e-002,
    +7.5016e-004, -9.6139e-005, -3.1995e-006, +1.3868e-007, -1.9486e-007, +3.0165e-010, -6.4376e-010};

double t_gpt::ap_amp[55] = {
    -1.0444e-001, +1.6618e-001, -6.3974e-002, +1.0922e+000, +5.7472e-001, -3.0277e-001, -3.5087e+000, +7.1264e-003,
    -1.4030e-001, +3.7050e-002, +4.0208e-001, -3.0431e-001, -1.3292e-001, +4.6746e-003, -1.5902e-004, +2.8624e+000,
    -3.9315e-001, -6.4371e-002, +1.6444e-002, -2.3403e-003, +4.2127e-005, +1.9945e+000, -6.0907e-001, -3.5386e-002,
    -1.0910e-003, -1.2799e-004, +4.0970e-005, +2.2131e-005, -5.3292e-001, -2.9765e-001, -3.2877e-002, +1.7691e-003,
    +5.9692e-005, +3.1725e-005, +2.0741e-005, -3.7622e-007, +2.6372e+000, -3.1165e-001, +1.6439e-002, +2.1633e-004,
    +1.7485e-004, +2.1587e-005, +6.1064e-006, -1.3755e-008, -7.8748e-008, -5.9152e-001, -1.7676e-001, +8.1807e-003,
    +1.0445e-003, +2.3432e-004, +9.3421e-006, +2.8104e-006, -1.5788e-007, -3.0648e-008, +2.6421e-010};

double t_gpt::bp_amp[55] = {
    +0.0000e+000, +0.0000e+000, +9.3340e-001, +0.0000e+000, +8.2346e-001, +2.2082e-001, +0.0000e+000, +9.6177e-001,
    -1.5650e-002, +1.2708e-003, +0.0000e+000, -3.9913e-001, +2.8020e-002, +2.8334e-002, +8.5980e-004, +0.0000e+000,
    +3.0545e-001, -2.1691e-002, +6.4067e-004, -3.6528e-005, -1.1166e-004, +0.0000e+000, -7.6974e-002, -1.8986e-002,
    +5.6896e-003, -2.4159e-004, -2.3033e-004, -9.6783e-006, +0.0000e+000, -1.0218e-001, -1.3916e-002, -4.1025e-003,
    -5.1340e-005, -7.0114e-005, -3.3152e-007, +1.6901e-006, +0.0000e+000, -1.2422e-002, +2.5072e-003, +1.1205e-003,
    -1.3034e-004, -2.3971e-005, -2.6622e-006, +5.7852e-007, +4.5847e-008, +0.0000e+000, +4.4777e-002, -3.0421e-003,
    +2.6062e-005, -7.2421e-005, +1.9119e-006, +3.9236e-007, +2.2390e-007, +2.9765e-009, -4.6452e-009};

double t_gpt::at_mean[55] = {
    +1.6257e+001, +2.1224e+000, +9.2569e-001, -2.5974e+001, +1.4510e+000, +9.2468e-002, -5.3192e-001, +2.1094e-001,
    -6.9210e-002, -3.4060e-002, -4.6569e+000, +2.6385e-001, -3.6093e-002, +1.0198e-002, -1.8783e-003, +7.4983e-001,
    +1.1741e-001, +3.9940e-002, +5.1348e-003, +5.9111e-003, +8.6133e-006, +6.3057e-001, +1.5203e-001, +3.9702e-002,
    +4.6334e-003, +2.4406e-004, +1.5189e-004, +1.9581e-007, +5.4414e-001, +3.5722e-001, +5.2763e-002, +4.1147e-003,
    -2.7239e-004, -5.9957e-005, +1.6394e-006, -7.3045e-007, -2.9394e+000, +5.5579e-002, +1.8852e-002, +3.4272e-003,
    -2.3193e-005, -2.9349e-005, +3.6397e-007, +2.0490e-006, -6.4719e-008, -5.2225e-001, +2.0799e-001, +1.3477e-003,
    +3.1613e-004, -2.2285e-004, -1.8137e-005, -1.5177e-007, +6.1343e-007, +7.8566e-008, +1.0749e-009};

double t_gpt::bt_mean[55] = {
    +0.0000e+000, +0.0000e+000, +1.0210e+000, +0.0000e+000, +6.0194e-001, +1.2292e-001, +0.0000e+000, -4.2184e-001,
    +1.8230e-001, +4.2329e-002, +0.0000e+000, +9.3312e-002, +9.5346e-002, -1.9724e-003, +5.8776e-003, +0.0000e+000,
    -2.0940e-001, +3.4199e-002, -5.7672e-003, -2.1590e-003, +5.6815e-004, +0.0000e+000, +2.2858e-001, +1.2283e-002,
    -9.3679e-003, -1.4233e-003, -1.5962e-004, +4.0160e-005, +0.0000e+000, +3.6353e-002, -9.4263e-004, -3.6762e-003,
    +5.8608e-005, -2.6391e-005, +3.2095e-006, -1.1605e-006, +0.0000e+000, +1.6306e-001, +1.3293e-002, -1.1395e-003,
    +5.1097e-005, +3.3977e-005, +7.6449e-006, -1.7602e-007, -7.6558e-008, +0.0000e+000, -4.5415e-002, -1.8027e-002,
    +3.6561e-004, -1.1274e-004, +1.3047e-005, +2.0001e-006, -1.5152e-007, -2.7807e-008, +7.7491e-009};

double t_gpt::at_amp[55] = {
    -1.8654e+000, -9.0041e+000, -1.2974e-001, -3.6053e+000, +2.0284e-002, +2.1872e-001, -1.3015e+000, +4.0355e-001,
    +2.2216e-001, -4.0605e-003, +1.9623e+000, +4.2887e-001, +2.1437e-001, -1.0061e-002, -1.1368e-003, -6.9235e-002,
    +5.6758e-001, +1.1917e-001, -7.0765e-003, +3.0017e-004, +3.0601e-004, +1.6559e+000, +2.0722e-001, +6.0013e-002,
    +1.7023e-004, -9.2424e-004, +1.1269e-005, -6.9911e-006, -2.0886e+000, -6.7879e-002, -8.5922e-004, -1.6087e-003,
    -4.5549e-005, +3.3178e-005, -6.1715e-006, -1.4446e-006, -3.7210e-001, +1.5775e-001, -1.7827e-003, -4.4396e-004,
    +2.2844e-004, -1.1215e-005, -2.1120e-006, -9.6421e-007, -1.4170e-008, +7.8720e-001, -4.4238e-002, -1.5120e-003,
    -9.4119e-004, +4.0645e-006, -4.9253e-006, -1.8656e-006, -4.0736e-007, -4.9594e-008, +1.6134e-009};

double t_gpt::bt_amp[55] = {
    +0.0000e+000, +0.0000e+000, -8.9895e-001, +0.0000e+000, -1.0790e+000, -1.2699e-001, +0.0000e+000, -5.9033e-001,
    +3.4865e-002, -3.2614e-002, +0.0000e+000, -2.4310e-002, +1.5607e-002, -2.9833e-002, -5.9048e-003, +0.0000e+000,
    +2.8383e-001, +4.0509e-002, -1.8834e-002, -1.2654e-003, -1.3794e-004, +0.0000e+000, +1.3306e-001, +3.4960e-002,
    -3.6799e-003, -3.5626e-004, +1.4814e-004, +3.7932e-006, +0.0000e+000, +2.0801e-001, +6.5640e-003, -3.4893e-003,
    -2.7395e-004, +7.4296e-005, -7.9927e-006, -1.0277e-006, +0.0000e+000, +3.6515e-002, -7.4319e-003, -6.2873e-004,
    -8.2461e-005, +3.1095e-005, -5.3860e-007, -1.2055e-007, -1.1517e-007, +0.0000e+000, +3.1404e-002, +1.5580e-002,
    -1.1428e-003, +3.3529e-005, +1.0387e-005, -1.9378e-006, -2.7327e-007, +7.5833e-009, -9.2323e-009};

// GPT empirical model v1
int t_gpt::gpt_v1(double dmjd, double dlat, double dlon, double dhgt, double &pres, double &temp, double &undu)
{

    pres = temp = undu = 0.0;
    int i = 0;
    int m = 0;
    int n = 0;

    // reference day is 28 January
    // this is taken from Niell (1996) to be consistent
    // for constant values use: doy = 91.3125
    double doy = dmjd - 44239.0 + 1 - 28;

    // degree n and order m (=n)
    int nmax = 9;

    // unit vector
    double x = cos(dlat) * cos(dlon);
    double y = cos(dlat) * sin(dlon);
    double z = sin(dlat);

    double V[10][10];
    double W[10][10];

    // Legendre polynomials
    V[0][0] = 1.0;
    W[0][0] = 0.0;
    V[1][0] = z * V[0][0];
    W[1][0] = 0.0;

    for (n = 2; n <= nmax; n++)
    {
        V[n][0] = ((2 * n - 1) * z * V[n - 1][0] - (n - 1) * V[n - 2][0]) / n;
        W[n][0] = 0.0;
    }

    for (m = 1; m <= nmax; m++)
    {
        V[m][m] = (2 * m - 1) * (x * V[m - 1][m - 1] - y * W[m - 1][m - 1]);
        W[m][m] = (2 * m - 1) * (x * W[m - 1][m - 1] + y * V[m - 1][m - 1]);

        if (m < nmax)
        {
            V[m + 1][m] = (2 * m + 1) * z * V[m][m];
            W[m + 1][m] = (2 * m + 1) * z * W[m][m];
        }

        for (n = m + 2; n <= nmax; n++)
        {
            V[n][m] = ((2 * n - 1) * z * V[n - 1][m] - (n + m - 1) * V[n - 2][m]) / (n - m);
            W[n][m] = ((2 * n - 1) * z * W[n - 1][m] - (n + m - 1) * W[n - 2][m]) / (n - m);
        }
    }

    // Geoidal height
    i = 0;
    for (n = 0; n <= nmax; n++)
    {
        for (m = 0; m <= n; m++)
        {
            undu += a_geoid[i] * V[n][m] + b_geoid[i] * W[n][m];
            i++;
        }
    }

    // Orthometric height
    double hort = dhgt - undu;

    // Surface temperature on the geoid
    double atm = 0.0;
    double ata = 0.0;
    i = 0;
    for (n = 0; n <= nmax; n++)
    {
        for (m = 0; m <= n; m++)
        {
            atm += at_mean[i] * V[n][m] + bt_mean[i] * W[n][m];
            ata += at_amp[i] * V[n][m] + bt_amp[i] * W[n][m];
            i++;
        }
    }
    double temp0 = atm + ata * cos(doy / 365.25 * 2.0 * G_PI);

    // Height correction for temperature
    temp = temp0 - 0.0065 * hort;

    // Surface pressure on the geoid
    double apm = 0.0;
    double apa = 0.0;
    i = 0;
    for (n = 0; n <= nmax; n++)
    {
        for (m = 0; m <= n; m++)
        {
            apm += ap_mean[i] * V[n][m] + bp_mean[i] * W[n][m];
            apa += ap_amp[i] * V[n][m] + bp_amp[i] * W[n][m];
            i++;
        }
    }
    double pres0 = apm + apa * cos(doy / 365.25 * 2.0 * G_PI);

    // Height correction for pressure (Berg)
    pres = pres0 * pow(1.0 - 0.0000226 * hort, 5.225);

    // Height correction for pressure (P+T dependant)
    //   pres = pres0*exp(-9.80665*hort/287.0/(temp+273.15));

    return 0;
}

} // namespace gnut
