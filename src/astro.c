/* astro.c: see astro.h. */
#include <math.h>
#include "astro.h"

#define PI  3.14159265358979323846
#define RAD (PI / 180.0)

static double norm360(double a) { a = fmod(a, 360.0); return a < 0 ? a + 360.0 : a; }
static double norm180(double a) { a = norm360(a); return a > 180.0 ? a - 360.0 : a; }

/* Meeus table 27.C: A, B, C for the periodic terms */
static const float kTerms[24][3] = {
    {485, 324.96, 1934.136}, {203, 337.23, 32964.467}, {199, 342.08, 20.186}, {182, 27.85, 445267.112},
    {156, 73.14, 45036.886}, {136, 171.52, 22518.443}, {77, 222.54, 65928.934}, {74, 296.72, 3034.906},
    {70, 243.58, 9037.513}, {58, 119.81, 33718.147}, {52, 297.17, 150.678}, {50, 21.02, 2281.226},
    {45, 247.54, 29929.562}, {44, 325.15, 31555.956}, {29, 60.93, 4443.417}, {18, 155.12, 67555.328},
    {17, 288.79, 4562.452}, {16, 198.04, 62894.029}, {14, 199.76, 31436.921}, {12, 95.39, 14577.848},
    {12, 287.11, 31931.756}, {12, 320.81, 34777.259}, {9, 227.73, 1222.114}, {8, 15.45, 16859.074}
};

/* Meeus table 27.B (years 1000 to 3000) */
static const double kMean[4][5] = {
    {2451623.80984, 365242.37404,  0.05169, -0.00411, -0.00057},
    {2451716.56767, 365241.62603,  0.00325,  0.00888, -0.00030},
    {2451810.21715, 365242.01767, -0.11575,  0.00337,  0.00078},
    {2451900.05952, 365242.74049, -0.06223, -0.00823,  0.00032}
};

double AsDeltaT(int year)
{
    double t = year - 2000.0;                     /* NASA (Espenak and Meeus) polynomial, 2005-2050 */
    if (year < 2005 || year > 2050) t = year < 2005 ? 5 : 50;
    return 62.92 + 0.32217 * t + 0.005589 * t * t;
}

double AsEventJD(int year, int which)
{
    double Y = (year - 2000) / 1000.0, jde0, T, W, dl, S = 0;
    const double *m = kMean[which];
    int i;
    jde0 = m[0] + Y * (m[1] + Y * (m[2] + Y * (m[3] + Y * m[4])));
    T = (jde0 - 2451545.0) / 36525.0;
    W = (35999.373 * T - 2.47) * RAD;
    dl = 1 + 0.0334 * cos(W) + 0.0007 * cos(2 * W);
    for (i = 0; i < 24; i++) S += kTerms[i][0] * cos((kTerms[i][1] + kTerms[i][2] * T) * RAD);
    return jde0 + 0.00001 * S / dl - AsDeltaT(year) / 86400.0;
}

void AsSunAt(double jd, AsSun *s)
{
    double n = jd - 2451545.0;
    double L = norm360(280.460 + 0.9856474 * n);
    double g = norm360(357.528 + 0.9856003 * n) * RAD;
    double lam = norm360(L + 1.915 * sin(g) + 0.020 * sin(2 * g));
    double eps = (23.439 - 0.0000004 * n) * RAD;
    double ra = atan2(cos(eps) * sin(lam * RAD), cos(lam * RAD)) / RAD;
    double gmst = norm360(280.46061837 + 360.98564736629 * n);
    s->lambda = lam;
    s->dec = asin(sin(eps) * sin(lam * RAD)) / RAD;
    s->subLon = norm180(ra - gmst);
}

int AsSunTimes(double jdLocalNoon, double lat, double lon, double *rise, double *set, double *noonAlt)
{
    AsSun s;
    double t = jdLocalNoon, c, h0;
    int i;
    for (i = 0; i < 3; i++) {                     /* transit: the Sun overhead at this longitude */
        AsSunAt(t, &s);
        t += norm180(s.subLon - lon) / 360.98564736629;
    }
    AsSunAt(t, &s);
    *noonAlt = 90.0 - fabs(lat - s.dec);
    c = (sin(-0.833 * RAD) - sin(lat * RAD) * sin(s.dec * RAD)) / (cos(lat * RAD) * cos(s.dec * RAD));
    if (c <= -1) { *rise = *set = t; return 1; }
    if (c >= 1)  { *rise = *set = t; return -1; }
    h0 = acos(c) / RAD;
    *rise = t - h0 / 360.98564736629;
    *set = t + h0 / 360.98564736629;
    return 0;
}

double AsTerminatorLat(double lon, const AsSun *s)
{
    double H = (lon - s->subLon) * RAD, d = s->dec;
    if (fabs(d) < 0.01) d = d < 0 ? -0.01 : 0.01;  /* at an equinox the line runs pole to pole */
    return atan(-cos(H) / tan(d * RAD)) / RAD;
}

double AsJD(int y, int m, int d, double hours)
{
    int a, b;
    if (m <= 2) { y--; m += 12; }
    a = y / 100; b = 2 - a + a / 4;
    return floor(365.25 * (y + 4716)) + floor(30.6001 * (m + 1)) + d + b - 1524.5 + hours / 24.0;
}

void AsCalendar(double jd, int *y, int *m, int *d, int *hh, int *mi)
{
    double z, f, a, alpha, b, c, dd, e, dayf;
    long mins;
    jd += 0.5 / 1440.0;                           /* round to the minute */
    z = floor(jd + 0.5); f = jd + 0.5 - z;
    if (z < 2299161) a = z;
    else { alpha = floor((z - 1867216.25) / 36524.25); a = z + 1 + alpha - floor(alpha / 4); }
    b = a + 1524; c = floor((b - 122.1) / 365.25); dd = floor(365.25 * c); e = floor((b - dd) / 30.6001);
    dayf = b - dd - floor(30.6001 * e) + f;
    *d = (int)dayf;
    *m = (int)(e < 14 ? e - 1 : e - 13);
    *y = (int)(*m > 2 ? c - 4716 : c - 4715);
    mins = (long)((dayf - *d) * 1440.0);
    *hh = (int)(mins / 60); *mi = (int)(mins % 60);
}
