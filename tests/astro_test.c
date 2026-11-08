/* astro_test.c: check astro.c on a PC against published times.
 * Build: cc -I../src -o astro_test astro_test.c ../src/astro.c -lm && ./astro_test
 * References: US Naval Observatory "Earth's Seasons" table, UT, to the minute. */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "astro.h"

static int fails = 0;

static void event(int y, int which, int M, int D, int H, int Mi)
{
    static const char *names[] = { "March equinox", "June solstice", "September equinox", "December solstice" };
    int yy, mm, dd, hh, mi;
    double jd = AsEventJD(y, which), want = AsJD(y, M, D, H + Mi / 60.0);
    double err = (jd - want) * 1440.0;
    AsCalendar(jd, &yy, &mm, &dd, &hh, &mi);
    printf("%s %d %-18s %04d-%02d-%02d %02d:%02d UT  (USNO %02d:%02d, %+.1f min)\n", fabs(err) <= 2 ? "ok  " : "FAIL",
           y, names[which], yy, mm, dd, hh, mi, H, Mi, err);
    if (fabs(err) > 2) fails++;
}

int main(void)
{
    AsSun s;
    double rise, set, alt;
    int y, m, d, hh, mi;

    /* USNO: 2024 Mar 20 03:06, Jun 20 20:51, Sep 22 12:44, Dec 21 09:20; 2025 Mar 20 09:01, Jun 21 02:42,
       Sep 22 18:19, Dec 21 15:03 */
    event(2024, AS_MARCH, 3, 20, 3, 6);   event(2024, AS_JUNE, 6, 20, 20, 51);
    event(2024, AS_SEPT, 9, 22, 12, 44);  event(2024, AS_DEC, 12, 21, 9, 20);
    event(2025, AS_MARCH, 3, 20, 9, 1);   event(2025, AS_JUNE, 6, 21, 2, 42);
    event(2025, AS_SEPT, 9, 22, 18, 19);  event(2025, AS_DEC, 12, 21, 15, 3);

    /* the Sun at the June 2026 solstice: declination +23.44, longitude 90 */
    AsSunAt(AsEventJD(2026, AS_JUNE), &s);
    printf("%s June 2026 solstice: lambda %.3f, dec %.3f\n", fabs(s.lambda - 90) < 0.02 && fabs(s.dec - 23.44) < 0.02 ? "ok  " : "FAIL", s.lambda, s.dec);
    if (!(fabs(s.lambda - 90) < 0.02)) fails++;

    /* Cincinnati (39.10 N, 84.51 W), 21 June 2026, local noon EDT (16:00 UT): NOAA calculator sunrise 06:14 EDT, sunset 21:09 EDT */
    AsSunTimes(AsJD(2026, 6, 21, 16.0), 39.10, -84.51, &rise, &set, &alt);
    AsCalendar(rise - 4 / 24.0, &y, &m, &d, &hh, &mi);
    printf("Cincinnati 2026-06-21 sunrise %02d:%02d EDT", hh, mi);
    if (abs(hh * 60 + mi - (6 * 60 + 14)) > 3) { printf("  FAIL"); fails++; }
    AsCalendar(set - 4 / 24.0, &y, &m, &d, &hh, &mi);
    printf(", sunset %02d:%02d EDT", hh, mi);
    if (abs(hh * 60 + mi - (21 * 60 + 9)) > 3) { printf("  FAIL"); fails++; }
    printf(", noon altitude %.1f (expect 74.3)\n", alt);
    if (fabs(alt - 74.34) > 0.2) fails++;

    /* the terminator: at the equinox it runs near the meridians 90 degrees from the subsolar point */
    AsSunAt(AsEventJD(2026, AS_MARCH), &s);
    printf("%s March 2026 equinox: dec %.3f, terminator lat at subsolar+90 = %.1f\n", fabs(s.dec) < 0.02 ? "ok  " : "FAIL", s.dec,
           AsTerminatorLat(s.subLon + 90, &s));

    printf("%s (%d failed)\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
