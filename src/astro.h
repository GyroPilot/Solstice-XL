/* astro.h: the Sun for Solstice.  Portable C (doubles), built into the Mac application and into the host test.
 * Equinoxes and solstices: Meeus, Astronomical Algorithms, 2nd ed., chapter 27 (about a minute's accuracy for
 * 1000-3000).  The Sun's place: the low-precision formulas of the Astronomical Almanac (about 0.01 degree).
 * Times are Julian Days in Universal Time unless the name says TT. */
#ifndef ASTRO_H
#define ASTRO_H

#define AS_MARCH 0
#define AS_JUNE  1
#define AS_SEPT  2
#define AS_DEC   3

double AsEventJD(int year, int which);            /* the equinox or solstice, JD (UT) */
double AsDeltaT(int year);                        /* TT - UT in seconds */

typedef struct {
    double lambda;                                /* the Sun's ecliptic longitude, degrees 0..360 */
    double dec;                                   /* declination, degrees: the latitude where it is overhead */
    double subLon;                                /* longitude where it is overhead, degrees east -180..180 */
} AsSun;
void AsSunAt(double jd, AsSun *s);

/* Sunrise and sunset for the civil day containing jdLocalNoon (JD of local noon), at latitude/longitude in
 * degrees (east positive).  Returns 0 normally, 1 for sun up all day, -1 for down all day.  Times are JD (UT). */
int AsSunTimes(double jdLocalNoon, double lat, double lon, double *rise, double *set, double *noonAlt);

/* For the world map: the latitude of the day/night line at a longitude, for a Sun at subLon/dec. */
double AsTerminatorLat(double lon, const AsSun *s);

/* Calendar helpers (Gregorian, UT). */
double AsJD(int y, int m, int d, double hours);
void AsCalendar(double jd, int *y, int *m, int *d, int *hh, int *mi);

#endif
