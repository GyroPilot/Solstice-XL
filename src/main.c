/* Solstice 0.1d8: how long until the next solstice or equinox, and where the Sun and the Earth are now.
 * The classic Mac companion to LisaSolstice (the banana's, on the Lisa), 2 Nov 2026.
 *
 * 0.1d4 lines up with LisaSolstice 0.1 (Tim: "I would like the two apps to largly align"): it opens on Tim's
 * solargraph, and the Seasons menu has the Lisa's pages, names and keys: Days to the Next Season (D), The Earth's
 * Orbit (O), The Earth's Tilt (E), Where the Sun Is Now (W), Your Town (Y), A Fun Fact (F), Solstice Traditions (L),
 * The Solargraph (G).  0.1d5: the Lisa's 25 facts and Tim and Connie's timeline (the banana, night of 2 Nov 2026).
 * 0.1d6: Pictures from History (P), the three Tim chose for both apps, 3 Nov 2026; 0.1d7: six (LisaSolstice REV C),
 * oldest site first, the two CC BY-SA photos credited on their page and in About.
 * The Mac's extras: the next five events on the countdown page, and the World Map (M).  The letter keys work with
 * or without Command.  Everything is computed on the Mac (astro.c: Meeus ch. 27, about a minute); no network.
 *
 * 68000 code, System 6 and 7, built with Retro68.  Your town comes from Set Your Town (kept in "Solstice Settings"
 * beside the application), else the Map control panel, else Cincinnati.  World map: Natural Earth, public domain.
 */
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "astro.h"

#define PI 3.14159265358979323846

enum { mApple = 128, mFile, mEdit, mSeasons };
enum { iAbout = 1 };
enum { iQuit = 1 };
/* Seasons menu items; the pages use the same numbers */
enum { vCount = 1, vOrbit, vTilt, vSun, vTown, iSetTown = 7, iSetZone, vFact = 10, vTrad, vPics, vSplash, vMap = 15 };

extern const unsigned char kWorldMap[];
extern const unsigned char kSplash[];
extern const char *const kFacts[];
extern const short kNumFacts;
extern const unsigned char *const kPicBits[];
extern const short kPicW[], kPicRow[], kPicH, kNumPics;
extern const char *const kPicCap1[], *const kPicCap2[];

/* ReadLocation: Retro68 has no glue; it is _ReadXPRam for 12 bytes at $E4 (Inside Macintosh: OS Utilities). */
#pragma parameter __D0 ReadXPRamTrap(__A0, __D0)
pascal OSErr ReadXPRamTrap(Ptr buf, long countAndAddr) M68K_INLINE(0xA051);

typedef struct {
    char   town[32];
    double lat, lon;                     /* degrees, north and east positive */
    double utcHours;                     /* local time minus UT */
    Boolean fromMap;                     /* came from the Map control panel */
} Place;

static WindowPtr gWin;
static Boolean   gQuit, gHasWNE;
static short     gView = vSplash;
static short     gFact;                        /* the fact shown last, 1-based; 0 = none yet */
static short     gPic;                         /* the picture shown last, 1-based */
static Place     gPlace;
static unsigned long gLastMinute;

/* ---------- small things ---------- */

#ifdef SOLSTICE_TRACE
static void Trace(const char *what)              /* debug: append a line to "Solstice Trace" beside the app */
{
    static short ref = 0;
    long n = (long)strlen(what);
    if (!ref) {
        unsigned char name[] = "\pSolstice Trace";
        short vref; GetVol(NULL, &vref);
        Create(name, vref, 'ttxt', 'TEXT');
        if (FSOpen(name, vref, &ref) != noErr) { ref = 0; return; }
        SetEOF(ref, 0);
    }
    FSWrite(ref, &n, what); n = 1; FSWrite(ref, &n, "\r");
    { short vref; GetVol(NULL, &vref); FlushVol(NULL, vref); }
}
#else
#define Trace(x)
#endif

static Boolean TrapAvailable(short trap)
{
    return NGetTrapAddress(trap, kToolboxTrapType) != NGetTrapAddress(0xA89F, kToolboxTrapType);
}

/* 0.1d8: an OS trap (A0xx) is there if its address isn't _Unimplemented's.  MacWorks on the Macintosh XL has no
 * _ReadXPRam (the Lisa has no parameter RAM): Solstice stopped with "unimplemented trap" at startup (Tim, 4 Nov 2026). */
static Boolean OSTrapAvailable(short trap)
{
    return NGetTrapAddress(trap, kOSTrapType) != NGetTrapAddress(0xA89F, kToolboxTrapType);
}

static void CtoP(const char *c, unsigned char *p)
{
    size_t n = strlen(c);
    if (n > 255) n = 255;
    p[0] = (unsigned char)n;
    memcpy(p + 1, c, n);
}

static void PtoC(const unsigned char *p, char *c) { memcpy(c, p + 1, p[0]); c[p[0]] = 0; }

static void DrawC(short x, short y, const char *s)
{
    Str255 p;
    CtoP(s, p);
    MoveTo(x, y);
    DrawString(p);
}

static void DrawCentred(short cx, short y, const char *s)
{
    Str255 p;
    CtoP(s, p);
    MoveTo(cx - StringWidth(p) / 2, y);
    DrawString(p);
}

/* "21.3" without printf's %f (smaller and safe on a 68000 without an FPU) */
static void Fixed1(char *out, double v)
{
    long t = (long)(fabs(v) * 10 + 0.5);
    sprintf(out, "%s%ld.%ld", v < 0 ? "-" : "", t / 10, t % 10);
}

static void LatLon(char *out, double lat, double lon)
{
    char a[16], b[16];
    Fixed1(a, fabs(lat)); Fixed1(b, fabs(lon));
    sprintf(out, "%s\xA1 %c, %s\xA1 %c", a, lat >= 0 ? 'N' : 'S', b, lon >= 0 ? 'E' : 'W');   /* \xA1 = degree sign */
}

/* A System 7 Mac told "26" in Date & Time sits in 1926: a year before 1984 gets a century (as in Sherpa 1.0.1). */
static unsigned long NowLocalSecs(void)
{
    unsigned long secs;
    DateTimeRec d;
    GetDateTime(&secs);
    SecondsToDate(secs, &d);
    if (d.year < 1984) { d.year += 100; DateToSeconds(&d, &secs); }
    return secs;
}

static double UTOffsetSecs(void) { return gPlace.utcHours * 3600.0; }

static double NowJD(void) { return 2416480.5 + ((double)NowLocalSecs() - UTOffsetSecs()) / 86400.0; }

static unsigned long JDToLocalSecs(double jd) { return (unsigned long)((jd - 2416480.5) * 86400.0 + UTOffsetSecs() + 0.5); }

static void LocalDateTime(double jd, char *out, Boolean longForm)
{
    Str255 d, t;
    char dc[100], tc[40];
    unsigned long s = JDToLocalSecs(jd);
    IUDateString(s, longForm ? longDate : abbrevDate, d);
    IUTimeString(s, false, t);
    PtoC(d, dc); PtoC(t, tc);
    sprintf(out, "%s, %s", dc, tc);
}

static void LocalTime(double jd, char *out)
{
    Str255 t;
    IUTimeString(JDToLocalSecs(jd), false, t);
    PtoC(t, out);
}

/* ---------- your town ---------- */

static void SaveSettings(void)
{
    short ref, vref;
    long n;
    char s[160];
    unsigned char name[] = "\pSolstice Settings";
    char la[16], lo[16], ut[16];
    GetVol(NULL, &vref);
    if (FSOpen(name, vref, &ref) == fnfErr) {
        Create(name, vref, 'SlSt', 'TEXT');
        if (FSOpen(name, vref, &ref) != noErr) return;
    }
    sprintf(la, "%ld", (long)(gPlace.lat * 1000)); sprintf(lo, "%ld", (long)(gPlace.lon * 1000)); sprintf(ut, "%ld", (long)(gPlace.utcHours * 100));
    sprintf(s, "town=%s\rlat1000=%s\rlon1000=%s\rutc100=%s\r", gPlace.town, la, lo, ut);
    n = (long)strlen(s);
    SetEOF(ref, 0);
    FSWrite(ref, &n, s);
    FSClose(ref);
}

static Boolean LoadSettings(void)
{
    short ref, vref;
    long n = 200;
    char s[201], *p;
    long v;
    unsigned char name[] = "\pSolstice Settings";
    GetVol(NULL, &vref);
    if (FSOpen(name, vref, &ref) != noErr) return false;
    FSRead(ref, &n, s);
    FSClose(ref);
    s[n] = 0;
    for (p = s; *p; p++) if (*p == '\r') *p = '\n';
    if ((p = strstr(s, "town=")) != NULL) { sscanf(p + 5, "%31[^\n]", gPlace.town); }
    if ((p = strstr(s, "lat1000=")) != NULL && sscanf(p + 8, "%ld", &v) == 1) gPlace.lat = v / 1000.0;
    if ((p = strstr(s, "lon1000=")) != NULL && sscanf(p + 8, "%ld", &v) == 1) gPlace.lon = v / 1000.0;
    if ((p = strstr(s, "utc100=")) != NULL && sscanf(p + 7, "%ld", &v) == 1) gPlace.utcHours = v / 100.0;
    gPlace.fromMap = false;
    return true;
}

/* The Map control panel's place: Fract latitude and longitude (2.30, fraction of 90 degrees), GMT offset. */
static Boolean ReadMapPlace(void)
{
    struct { long lat, lon, gmt; } loc;
    long g;
    if (!OSTrapAvailable(0xA051)) return false;          /* no _ReadXPRam: Cincinnati, or Set Your Town */
    if (ReadXPRamTrap((Ptr)&loc, 0x000C00E4L) != noErr) return false;
    if (loc.lat == 0 && loc.lon == 0) return false;
    gPlace.lat = (double)loc.lat / 1073741824.0 * 90.0;
    gPlace.lon = (double)loc.lon / 1073741824.0 * 90.0;
    g = loc.gmt & 0x00FFFFFFL;
    if (g & 0x00800000L) g |= 0xFF000000L;            /* sign-extend the 24-bit offset */
    gPlace.utcHours = g / 3600.0;
    strcpy(gPlace.town, "your Map place");
    gPlace.fromMap = true;
    return true;
}

static void SetupPlace(void)
{
    strcpy(gPlace.town, "Cincinnati");                /* Tim's lab, until told otherwise */
    gPlace.lat = 39.10; gPlace.lon = -84.51; gPlace.utcHours = -5;
    if (!LoadSettings()) { Trace("xpram"); ReadMapPlace(); Trace("xpram done"); }
}

static void AskTown(short field)                 /* field: 4 = the town (Set Your Town), 10 = hours from UT (Set Your Time Zone) */
{
    DialogPtr d = GetNewDialog(128, NULL, (WindowPtr)-1);
    short item = 0, type, i;
    Handle h[4];
    Rect r;
    char s[64];
    Str255 p;
    if (!d) return;
    for (i = 0; i < 4; i++) GetDialogItem(d, 4 + 2 * i, &type, &h[i], &r);
    CtoP(gPlace.town, p); SetDialogItemText(h[0], p);
    { char a[20]; long v = (long)(gPlace.lat * 100 + (gPlace.lat < 0 ? -0.5 : 0.5)); sprintf(a, "%s%ld.%02ld", v < 0 ? "-" : "", labs(v) / 100, labs(v) % 100); CtoP(a, p); SetDialogItemText(h[1], p); }
    { char a[20]; long v = (long)(gPlace.lon * 100 + (gPlace.lon < 0 ? -0.5 : 0.5)); sprintf(a, "%s%ld.%02ld", v < 0 ? "-" : "", labs(v) / 100, labs(v) % 100); CtoP(a, p); SetDialogItemText(h[2], p); }
    { char a[20]; long v = (long)(gPlace.utcHours * 100 + (gPlace.utcHours < 0 ? -0.5 : 0.5)); sprintf(a, "%s%ld.%02ld", v < 0 ? "-" : "", labs(v) / 100, labs(v) % 100); CtoP(a, p); SetDialogItemText(h[3], p); }
    SelectDialogItemText(d, field, 0, 255);
    ShowWindow(d);
    while (item != 1 && item != 2) ModalDialog(NULL, &item);
    if (item == 1) {
        double v[3];
        int ok = 1;
        GetDialogItemText(h[0], p); PtoC(p, s); s[31] = 0; strcpy(gPlace.town, s);
        for (i = 0; i < 3; i++) {
            long whole = 0, frac = 0, scale = 1;
            char *q;
            int neg = 0;
            GetDialogItemText(h[i + 1], p); PtoC(p, s);
            q = s;
            while (*q == ' ') q++;
            if (*q == '-') { neg = 1; q++; } else if (*q == '+') q++;
            while (*q >= '0' && *q <= '9') whole = whole * 10 + (*q++ - '0');
            if (*q == '.') { q++; while (*q >= '0' && *q <= '9' && scale < 10000) { frac = frac * 10 + (*q++ - '0'); scale *= 10; } }
            v[i] = (whole + (double)frac / scale) * (neg ? -1 : 1);
        }
        if (v[0] < -90 || v[0] > 90 || v[1] < -180 || v[1] > 180 || v[2] < -14 || v[2] > 14) ok = 0;
        if (ok) { gPlace.lat = v[0]; gPlace.lon = v[1]; gPlace.utcHours = v[2]; gPlace.fromMap = false; SaveSettings(); }
        else SysBeep(10);
    }
    DisposeDialog(d);
    SetPort(gWin);                               /* the dialog was the current port */
    InvalRect(&gWin->portRect);
}

/* ---------- the views ---------- */

static const char *kEventName[4] = { "March equinox", "June solstice", "September equinox", "December solstice" };

static void NextEvent(double now, int *year, int *which, double *jd)
{
    int y, k, w;
    AsCalendar(now, &y, &k, &k, &k, &k);
    for (; y <= 2100; y++)
        for (w = 0; w < 4; w++) {
            double j = AsEventJD(y, w);
            if (j > now) { *year = y; *which = w; *jd = j; return; }
        }
    *year = y; *which = 0; *jd = now;
}

static void SunLine(short x, short y, const char *label, double jdNoonLocal)
{
    double rise, set, alt, len;
    char s[200], a[24], b[24], c[16];
    int kind = AsSunTimes(jdNoonLocal, gPlace.lat, gPlace.lon, &rise, &set, &alt);
    Fixed1(c, alt);
    if (kind == 1) sprintf(s, "%s: the Sun stays up all day; at noon it is %s\xA1 high.", label, c);
    else if (kind == -1) sprintf(s, "%s: the Sun does not rise.", label);
    else {
        long mins;
        LocalTime(rise, a); LocalTime(set, b);
        len = (set - rise) * 24.0;
        mins = (long)(len * 60 + 0.5);
        sprintf(s, "%s: sunrise %s, sunset %s, %ld h %02ld min of daylight, noon Sun %s\xA1 high.", label, a, b, mins / 60, mins % 60, c);
    }
    DrawC(x, y, s);
}

/* "UTC-5", "UTC+5:30" */
static void ZoneName(char *out)
{
    long m = (long)(gPlace.utcHours * 60 + (gPlace.utcHours < 0 ? -0.5 : 0.5));
    if (m % 60) sprintf(out, "UTC%c%ld:%02ld", m < 0 ? '-' : '+', labs(m) / 60, labs(m) % 60);
    else sprintf(out, "UTC%c%ld", m < 0 ? '-' : '+', labs(m) / 60);
}

/* "49 days, 3 hours and 28 minutes", zero parts left off (as LisaSolstice) */
static void LeftWords(double minutes, char *out)
{
    long days = (long)(minutes / 1440.0), hours, mins;
    char part[3][24];
    int n = 0;
    hours = (long)((minutes - days * 1440.0) / 60.0);
    mins = (long)(minutes - days * 1440.0 - hours * 60.0);
    if (days)  sprintf(part[n++], "%ld day%s", days, days == 1 ? "" : "s");
    if (hours) sprintf(part[n++], "%ld hour%s", hours, hours == 1 ? "" : "s");
    if (mins || !n) sprintf(part[n++], "%ld minute%s", mins, mins == 1 ? "" : "s");
    if (n == 1) sprintf(out, "%s", part[0]);
    else if (n == 2) sprintf(out, "%s and %s", part[0], part[1]);
    else sprintf(out, "%s, %s and %s", part[0], part[1], part[2]);
}

static void DrawCountdown(Rect *r)
{
    double now = NowJD(), jd, prev;
    int year, which, w, y, m, d, hh, mi;
    char s[200], t[120], z[16];
    short x = r->left + 16, cx = (r->left + r->right) / 2, yy = r->top + 22, size;
    static const char *kSeason[4] = { "Spring in the north, autumn in the south", "Summer in the north, winter in the south",
                                      "Autumn in the north, spring in the south", "Winter in the north, summer in the south" };

    NextEvent(now, &year, &which, &jd);
    prev = which ? AsEventJD(year, which - 1) : AsEventJD(year - 1, AS_DEC);

    TextFont(0); TextSize(12);
    LocalDateTime(now, t, true); ZoneName(z);
    sprintf(s, "Now: %s (%s)", t, z);
    DrawCentred(cx, yy, s);
    sprintf(s, "Next: the %s", kEventName[which]);
    TextFace(bold); DrawCentred(cx, yy + 26, s); TextFace(0);
    LocalDateTime(jd, t, true);
    DrawCentred(cx, yy + 44, t);
    LeftWords((jd - now) * 1440.0, t);
    sprintf(s, "in %s", t);
    TextFont(2); TextFace(bold);                  /* New York, as large as fits */
    for (size = 24; size > 12; size -= 2) {
        Str255 p;
        TextSize(size); CtoP(s, p);
        if (StringWidth(p) < r->right - r->left - 24) break;
    }
    DrawCentred(cx, yy + 78, s);
    TextFace(0); TextFont(3); TextSize(10);       /* Geneva */
    {
        long pc = (long)((now - prev) / (jd - prev) * 100.0);
        sprintf(s, "%s: %ld percent of the way from the %s to the %s.", kSeason[(which + 3) % 4], pc,
                kEventName[(which + 3) % 4], kEventName[which]);
        DrawCentred(cx, yy + 102, s);
    }

    yy += 128;
    TextFace(bold); DrawC(x, yy, "Coming up (your time, from the Mac's clock and time zone):"); TextFace(0);
    {
        /* 0.1d2 (Tim, 2 Nov): the next five events, rolling into the following years, each with its countdown */
        int yr, n = 0;
        AsCalendar(now, &y, &m, &d, &hh, &mi);
        for (yr = y; yr <= y + 2 && n < 5; yr++)
            for (w = 0; w < 4 && n < 5; w++) {
                double j = AsEventJD(yr, w), daysLeft;
                char nm[40], in[32];
                if (j <= now) continue;
                n++;
                LocalDateTime(j, t, false);
                sprintf(nm, "%s %d", kEventName[w], yr);
                daysLeft = j - now;
                if (daysLeft < 1.0) sprintf(in, "today");
                else sprintf(in, "in %ld day%s", (long)daysLeft, (long)daysLeft == 1 ? "" : "s");
                if (n == 1) TextFace(bold);
                DrawC(x + 12, yy + 15 * n, nm);
                DrawC(x + 170, yy + 15 * n, t);
                DrawC(x + 380, yy + 15 * n, in);
                if (n == 1) TextFace(0);
            }
    }
    TextFont(0); TextSize(12);
}

/* Your Town (as LisaSolstice): the place and zone; today; the day lengths at the solstices and equinoxes */
static void DrawTown(Rect *r)
{
    double now = NowJD();
    char s[160], ll[48], z[16];
    short x = r->left + 16, cx = (r->left + r->right) / 2, yy = r->top + 22;
    int y, m, d, hh, mi, w;
    static const int kOrder[4] = { AS_JUNE, AS_DEC, AS_MARCH, AS_SEPT };

    AsCalendar(now, &y, &m, &d, &hh, &mi);
    TextFont(0); TextSize(12);
    sprintf(s, "Your Town: %s", gPlace.town);
    TextFace(bold); DrawCentred(cx, yy, s); TextFace(0);
    LatLon(ll, gPlace.lat, gPlace.lon); ZoneName(z);
    sprintf(s, "%s, %s", ll, z);
    DrawCentred(cx, yy + 18, s);
    TextFont(3); TextSize(10);
    yy += 50;
    SunLine(x, yy, "Today", floor(now + gPlace.utcHours / 24.0 + 0.5) - gPlace.utcHours / 24.0);
    yy += 30;
    sprintf(s, "The length of the day in %d:", y);
    TextFace(bold); DrawC(x, yy, s); TextFace(0);
    for (w = 0; w < 4; w++) {
        double j = AsEventJD(y, kOrder[w]);
        SunLine(x + 12, yy + 16 * (w + 1), kEventName[kOrder[w]], floor(j + gPlace.utcHours / 24.0 + 0.5) - gPlace.utcHours / 24.0);
    }
    yy += 16 * 5 + 18;
    DrawC(x, yy, "Set Your Town and Set Your Time Zone are in the Seasons menu (hours from UT: -5 for Eastern");
    DrawC(x, yy + 14, "Standard Time, -4 in summer).");
    if (gPlace.fromMap) DrawC(x, yy + 28, "This place and zone come from the Map control panel.");
    TextFont(0); TextSize(12);
}

static void Sun(short x, short y, short rad)
{
    Rect c;
    int i;
    SetRect(&c, x - rad, y - rad, x + rad, y + rad);
    PaintOval(&c);
    for (i = 0; i < 8; i++) {
        double a = i * PI / 4;
        MoveTo(x + (short)((rad + 3) * cos(a)), y + (short)((rad + 3) * sin(a)));
        LineTo(x + (short)((rad + 7) * cos(a)), y + (short)((rad + 7) * sin(a)));
    }
}

static void DrawOrbit(Rect *r)
{
    double now = NowJD();
    AsSun s;
    short cx = (r->left + r->right) / 2, cy = (r->top + r->bottom) / 2 + 6;
    short R = ((r->bottom - r->top) < (r->right - r->left) ? (r->bottom - r->top) : (r->right - r->left)) / 2 - 46;
    Rect o;
    int w;
    char s1[120];
    static const char *lab[4] = { "March equinox", "June solstice", "September equinox", "December solstice" };

    AsSunAt(now, &s);
    TextFont(3); TextSize(10);
    SetRect(&o, cx - R, cy - R, cx + R, cy + R);
    FrameOval(&o);
    Sun(cx, cy, 10);
    for (w = 0; w < 4; w++) {                     /* the Earth's place at each: opposite the Sun's longitude */
        double a = (w * 90 + 180) * PI / 180;
        short x = cx + (short)(R * cos(a)), y = cy - (short)(R * sin(a));
        Rect e;
        Str255 p;
        SetRect(&e, x - 4, y - 4, x + 4, y + 4);
        EraseOval(&e); FrameOval(&e);
        CtoP(lab[w], p);
        if (w == 0) MoveTo(x - StringWidth(p) - 8, y + 4);
        else if (w == 2) MoveTo(x + 8, y + 4);
        else if (w == 1) MoveTo(x - StringWidth(p) / 2, y + 18);
        else MoveTo(x - StringWidth(p) / 2, y - 9);
        DrawString(p);
    }
    {
        double a = (s.lambda + 180) * PI / 180;
        short x = cx + (short)(R * cos(a)), y = cy - (short)(R * sin(a));
        Rect e;
        SetRect(&e, x - 7, y - 7, x + 7, y + 7);
        PaintOval(&e);
        DrawC(x + 10, y - 8, "Earth now");
    }
    TextFont(0); TextSize(12);
    DrawCentred(cx, r->top + 16, "The Earth's Orbit");
    TextFont(3); TextSize(10);
    strcpy(s1, "Seen from above the North Pole, the Earth goes counterclockwise.");
    DrawCentred(cx, r->bottom - 8, s1);
    TextFont(0); TextSize(12);
}

static void DrawTilt(Rect *r)
{
    double now = NowJD(), lean;
    AsSun s;
    short cx = r->left + (r->right - r->left) * 3 / 5, cy = (r->top + r->bottom) / 2 - 8;
    short rad = (r->bottom - r->top) / 2 - 86;      /* room for the axis, the title and two lines of text */
    Rect e;
    short px, py;
    char s1[160], a[16];

    AsSunAt(now, &s);
    lean = 23.44 * sin(s.lambda * PI / 180);       /* + = the North Pole leans toward the Sun (left) */
    SetRect(&e, cx - rad, cy - rad, cx + rad, cy + rad);
    PenPat(&qd.gray); PaintArc(&e, 0, 180); PenPat(&qd.black);     /* the night half, away from the Sun */
    FrameOval(&e);
    px = (short)(rad * 1.22 * sin(lean * PI / 180)); py = (short)(rad * 1.22 * cos(lean * PI / 180));
    PenSize(2, 2);
    MoveTo(cx - px, cy - py); LineTo(cx + px, cy + py);            /* the axis: North end toward the Sun (left) when lean > 0 */
    PenSize(1, 1);
    MoveTo(cx - (short)(rad * cos(lean * PI / 180)), cy + (short)(rad * sin(lean * PI / 180)));
    LineTo(cx + (short)(rad * cos(lean * PI / 180)), cy - (short)(rad * sin(lean * PI / 180)));   /* the equator, square to the axis */
    {
        /* the tropics (23.44) and the polar circles (66.56), parallel to the equator, dotted */
        static const double kLat[4] = { 23.44, -23.44, 66.56, -66.56 };
        double sl = sin(lean * PI / 180), cl = cos(lean * PI / 180);
        int k, i;
        for (k = 0; k < 4; k++) {
            double up = rad * sin(kLat[k] * PI / 180), half = rad * cos(kLat[k] * PI / 180);
            double mx = cx - up * sl, my = cy - up * cl;
            for (i = -(int)half; i <= (int)half; i += 3) {
                short qx = (short)(mx + i * cl), qy = (short)(my - i * sl);
                PenMode(patXor); MoveTo(qx, qy); LineTo(qx, qy); PenNormal();
            }
        }
    }
    TextFont(0); TextSize(12);
    DrawC(cx - px - 4, cy - py - 6, "N");
    DrawC(cx + px - 4, cy + py + 16, "S");
    Sun(r->left + 34, cy, 14);
    TextFont(3); TextSize(10);
    DrawC(r->left + 14, cy + 36, "sunlight");
    MoveTo(r->left + 60, cy); LineTo(cx - rad - 12, cy);
    MoveTo(cx - rad - 18, cy - 4); LineTo(cx - rad - 12, cy); LineTo(cx - rad - 18, cy + 4);

    TextFont(0); TextSize(12);
    DrawCentred((r->left + r->right) / 2, r->top + 16, "The Earth's Tilt");
    TextFont(3); TextSize(10);
    Fixed1(a, fabs(s.dec));
    if (fabs(s.dec) < 0.5) strcpy(s1, "Neither pole leans toward the Sun: day and night are about equal everywhere.");
    else sprintf(s1, "The north leans %s degrees %s the Sun: %s days up north.", a, s.dec > 0 ? "toward" : "away from", s.dec > 0 ? "longer" : "shorter");
    DrawCentred((r->left + r->right) / 2, r->bottom - 36, s1);
    strcpy(s1, "Dotted: the tropics (23.44 degrees) and the polar circles (66.56). The axis always leans the same way in space.");
    DrawCentred((r->left + r->right) / 2, r->bottom - 20, s1);
    TextFont(0); TextSize(12);
}

static void DrawMap(Rect *r)
{
    double now = NowJD();
    AsSun s;
    BitMap bm;
    Rect src, dst;
    short ox, oy, x;
    char s1[160], a[48];
    int yy, mm, dd, hh, mi;

    AsSunAt(now, &s);
    ox = (r->left + r->right) / 2 - 180; oy = r->top + 34;
    bm.baseAddr = (Ptr)kWorldMap; bm.rowBytes = 46;
    SetRect(&bm.bounds, 0, 0, 360, 180);
    SetRect(&src, 0, 0, 360, 180);
    SetRect(&dst, ox, oy, ox + 360, oy + 180);
    CopyBits(&bm, &gWin->portBits, &src, &dst, srcCopy, NULL);
    FrameRect(&dst);
    PenPat(&qd.gray); PenMode(patOr);
    for (x = 0; x < 360; x++) {                   /* night: grey over the sea, the land stays black */
        double lon = x - 179.5, t = AsTerminatorLat(lon, &s);
        short ty = oy + (short)(90 - t);
        if (s.dec >= 0) { MoveTo(ox + x, ty); LineTo(ox + x, oy + 179); }
        else { MoveTo(ox + x, oy); LineTo(ox + x, ty); }
    }
    PenNormal();
    {
        short sx = ox + (short)(s.subLon + 180), sy = oy + (short)(90 - s.dec);
        Rect c;
        SetRect(&c, sx - 5, sy - 5, sx + 5, sy + 5);
        EraseOval(&c);
        Sun(sx, sy, 3);
    }
    {
        short hx = ox + (short)(gPlace.lon + 180), hy = oy + (short)(90 - gPlace.lat);
        PenMode(patXor);
        MoveTo(hx - 5, hy); LineTo(hx + 5, hy); MoveTo(hx, hy - 5); LineTo(hx, hy + 5);
        PenNormal();
    }
    TextFont(0); TextSize(12);
    DrawCentred((r->left + r->right) / 2, r->top + 18, "Day and night now");
    TextFont(3); TextSize(10);
    AsCalendar(now, &yy, &mm, &dd, &hh, &mi);
    LatLon(a, s.dec, s.subLon);
    sprintf(s1, "At %02d:%02d UT the Sun is overhead at %s (the sun mark). The cross is %s.", hh, mi, a, gPlace.town);
    DrawCentred((r->left + r->right) / 2, oy + 200, s1);
    DrawCentred((r->left + r->right) / 2, oy + 216, "Grey sea is night. Map: Natural Earth (public domain).");
    TextFont(0); TextSize(12);
}

/* Where the Sun Is Now (as LisaSolstice): the overhead point, the declination, the equation of time, the orbit so far */
static void DrawSunNow(Rect *r)
{
    double now = NowJD(), ut, eot;
    AsSun s;
    char s1[160], a[48], b[16];
    short x = r->left + 24, cx = (r->left + r->right) / 2, yy = r->top + 22;
    int y, m, d, hh, mi;

    AsSunAt(now, &s);
    AsCalendar(now, &y, &m, &d, &hh, &mi);
    TextFont(0); TextSize(12);
    TextFace(bold); DrawCentred(cx, yy, "Where the Sun Is Now"); TextFace(0);
    LatLon(a, s.dec, s.subLon);
    sprintf(s1, "Overhead at %s", a);
    TextSize(14); DrawCentred(cx, yy + 34, s1); TextSize(12);
    sprintf(s1, "(at %02d:%02d UT; your time is in the menu bar clock)", hh, mi);
    TextFont(3); TextSize(10); DrawCentred(cx, yy + 52, s1);
    yy += 84;
    Fixed1(b, fabs(s.dec));
    sprintf(s1, "Declination (the Sun's latitude): %s degrees %s of the equator.", b, s.dec >= 0 ? "north" : "south");
    DrawC(x, yy, s1);
    /* the equation of time: apparent solar time at Greenwich (from the overhead point) minus the clock (UT) */
    ut = (now + 0.5 - floor(now + 0.5)) * 24.0;
    eot = (12.0 - s.subLon / 15.0) - ut;
    while (eot > 12) eot -= 24;
    while (eot < -12) eot += 24;
    eot *= 60.0;
    Fixed1(b, fabs(eot));
    if (fabs(eot) < 0.05) strcpy(s1, "A sundial agrees with the clock today.");
    else sprintf(s1, "A sundial is %s minutes %s the clock today (the equation of time).", b, eot > 0 ? "AHEAD of" : "BEHIND");
    DrawC(x, yy + 20, s1);
    Fixed1(b, s.lambda);
    sprintf(s1, "Since the March equinox the Earth has gone %s degrees round the Sun: %ld percent of the orbit.", b, (long)(s.lambda / 3.6));
    DrawC(x, yy + 40, s1);
    DrawC(x, yy + 70, "World Map (Apple-M) shows the overhead point with day and night.");
    TextFont(0); TextSize(12);
}

/* A Fun Fact: kFacts (facts.c, the same list as LisaSolstice), one at a time, in turn */
static void DrawFact(Rect *r)
{
    short cx = (r->left + r->right) / 2;
    const char *f;
    char s[40];
    Rect box;
    if (gFact < 1 || gFact > kNumFacts) gFact = 1;
    f = kFacts[gFact - 1];
    TextFont(0); TextSize(12);
    sprintf(s, "A Fun Fact (%d of %d)", gFact, kNumFacts);
    TextFace(bold); DrawCentred(cx, r->top + 22, s); TextFace(0);
    SetRect(&box, r->left + 30, r->top + 44, r->right - 30, r->bottom - 34);
    TextFont(2); TextSize(r->bottom - r->top > 300 ? 14 : 12);   /* New York; smaller on a Plus */
    TETextBox((Ptr)f, strlen(f), &box, teFlushLeft);
    TextFont(3); TextSize(9);
    DrawCentred(cx, r->bottom - 20, "Solstice history from Tim and Connie O'Connor's Winter Solstice talk.");
    DrawCentred(cx, r->bottom - 8, "Press F (or Apple-F) for the next one.");
    TextFont(0); TextSize(12);
}

/* Solstice Traditions (as LisaSolstice, from Tim and Connie's talk, checked and updated by the banana):
 * how many winter solstices each custom has seen, newest at the top, a bar from its start to now. */
static void DrawTraditions(Rect *r)
{
    static const struct { const char *what; short start, unit; } kRows[] = {
        { "Family Christmas: US holiday", 1870, 10 },
        { "Christmas returns to Massachusetts", 1681, 10 },
        { "Christmas trees, Germany", 1500, 100 },
        { "Dec 25 in Rome's calendar", 336, 10 },
        { "Roman Saturnalia: gifts, feasting", -217, 100 },
        { "Sun chariot, Denmark (Trundholm)", -1400, 100 },
        { "Stonehenge: midwinter sunset", -2500, 100 },
        { "Newgrange, Ireland: midwinter sunrise", -3200, 100 },
        { "Goseck circle, Germany: solstice gates", -4900, 100 },
        { "Evergreens indoors at midwinter", -32000, 0 }        /* older than written history */
    };
    int y, m, d, hh, mi, i, nRows = sizeof kRows / sizeof kRows[0];
    short lw, x0, x1, top = r->top + 34, rowH, cx = (r->left + r->right) / 2;
    char s[80];
    AsCalendar(NowJD(), &y, &m, &d, &hh, &mi);
    TextFont(0); TextSize(12);
    sprintf(s, "Solstice Traditions: how many winter solstices old (December %d)", y);
    if (r->right - r->left < 560) sprintf(s, "Solstice Traditions (December %d)", y);
    TextFace(bold); DrawCentred(cx, r->top + 18, s); TextFace(0);
    TextFont(3); TextSize(9);
    lw = (r->right - r->left) < 560 ? 150 : 190;
    x0 = r->left + 8 + lw; x1 = r->right - 14;
    rowH = (r->bottom - top - 34) / nRows;
#define TX(yr) (x0 + (short)((long)((yr) + 5000) * (x1 - x0) / (y + 5000)))
    for (i = 0; i < nRows; i++) {
        short ry = top + rowH * i + rowH / 2, bx;
        DrawC(r->left + 8, ry + 3, kRows[i].what);
        PenSize(1, 3);
        if (kRows[i].unit) {
            long n = (long)(y - kRows[i].start) / kRows[i].unit * kRows[i].unit;
            Str255 p;
            bx = TX(kRows[i].start);
            MoveTo(bx, ry); LineTo(x1 - 5, ry);
            if (n >= 1000) sprintf(s, "over %ld,%03ld", n / 1000, n % 1000); else sprintf(s, "over %ld", n);
            CtoP(s, p);
            MoveTo(bx - StringWidth(p) - 4 > r->left + lw ? bx - StringWidth(p) - 4 : bx + 2, ry - (bx - StringWidth(p) - 4 > r->left + lw ? -3 : 3));
            DrawString(p);
        } else {
            short x;
            for (x = x0; x < x0 + 40; x += 4) { MoveTo(x, ry); LineTo(x, ry); }   /* fades in from before 5000 BCE */
            MoveTo(x0 + 40, ry); LineTo(x1 - 5, ry);
            DrawC(x0 + 46, ry - 4, "older than written history: thousands of solstices");
        }
        PenSize(1, 1);
        MoveTo(x1 - 6, ry - 3); LineTo(x1, ry); LineTo(x1 - 6, ry + 3);                      /* to now */
    }
    {
        static const short kTicks[] = { -5000, -4000, -3000, -2000, -1000, 1, 1000, 2000 };
        short ay = top + rowH * nRows + 4, k;
        MoveTo(x0, ay); LineTo(x1, ay);
        for (k = 0; k < 8; k++) {
            short tx = TX(kTicks[k]);
            Str255 p;
            MoveTo(tx, ay); LineTo(tx, ay + 3);
            if (kTicks[k] < 0) sprintf(s, "%d%s", -kTicks[k], kTicks[k] == -5000 || kTicks[k] == -1000 ? " BCE" : "");
            else sprintf(s, "%s%d", kTicks[k] == 1 ? "" : "", kTicks[k] == 1 ? 1 : kTicks[k]);
            if (kTicks[k] == 1) strcpy(s, "1 CE");
            CtoP(s, p); MoveTo(tx - StringWidth(p) / 2, ay + 13); DrawString(p);
        }
    }
#undef TX
    TextFont(0); TextSize(12);
}

/* Pictures from History (as LisaSolstice REV B): Tim's three, one at a time, in turn, with the banana's captions */
static void DrawPics(Rect *r)
{
    BitMap bm;
    Rect src, dst;
    short i, cx = (r->left + r->right) / 2, w, h = kPicH, room = r->bottom - r->top - 76;
    char s[60];
    if (gPic < 1 || gPic > kNumPics) gPic = 1;
    i = gPic - 1;
    w = kPicW[i];
    TextFont(0); TextSize(12);
    sprintf(s, "Pictures from History (%d of %d)", gPic, kNumPics);
    TextFace(bold); DrawCentred(cx, r->top + 16, s); TextFace(0);
    bm.baseAddr = (Ptr)kPicBits[i]; bm.rowBytes = kPicRow[i];
    SetRect(&bm.bounds, 0, 0, kPicW[i], kPicH);
    SetRect(&src, 0, 0, kPicW[i], kPicH);
    /* 0.1d8: a little short of room (the XL: about 244 of 250 rows): trim rows top and bottom.  Shrinking a dithered
     * photo by a few percent drops every 40th row and column and draws a grid (Stonehenge on Tim's XL).
     * Much shorter (a Plus): shrink. */
    if (h > room && (long)room * 100 / h >= 85) { short cut = (short)((h - room) / 2); src.top = cut; src.bottom = cut + room; h = room; }
    else if (h > room) { w = (short)((long)w * room / h); h = room; }
    SetRect(&dst, cx - w / 2, r->top + 24, cx - w / 2 + w, r->top + 24 + h);
    CopyBits(&bm, &gWin->portBits, &src, &dst, srcCopy, NULL);
    TextFont(3); TextSize(10);
    DrawCentred(cx, dst.bottom + 14, kPicCap1[i]);
    DrawCentred(cx, dst.bottom + 27, kPicCap2[i]);
    TextSize(9);
    DrawCentred(cx, r->bottom - 6, "Press P (or Apple-P) for the next one.");
    TextFont(0); TextSize(12);
}

/* The Solargraph: Tim's picture, the opening page; scaled down only when the window is smaller (a Plus) */
static void DrawSplash(Rect *r)
{
    BitMap bm;
    Rect src, dst;
    short cx = (r->left + r->right) / 2, w = 512, h = 325, room = r->bottom - r->top - 30;
    TextFont(3); TextSize(10);
    DrawCentred(cx, r->top + 14, "The Sun over Cincinnati, June to December solstice, in a pop-can pinhole camera");
    short wide = r->right - r->left - 8;
    bm.baseAddr = (Ptr)kSplash; bm.rowBytes = 64;
    SetRect(&bm.bounds, 0, 0, 512, 325);
    SetRect(&src, 0, 0, 512, 325);
    /* 0.1d8: a little short of room, trim the edges instead of shrinking: a few percent smaller drops every 20th row
     * or column of the dithered picture and draws a grid (Tim's XL).  Much smaller (a Plus): shrink as before. */
    if (w > wide && (long)wide * 100 / w >= 85) { short cut = (short)((w - wide) / 2); src.left = cut; src.right = cut + wide; w = wide; }
    if (h > room && (long)room * 100 / h >= 85) { short cut = (short)((h - room) / 2); src.top = cut; src.bottom = cut + room; h = room; }
    if (w > wide) { h = (short)((long)h * wide / w); w = wide; }
    if (h > room) { w = (short)((long)w * room / h); h = room; }
    SetRect(&dst, cx - w / 2, r->top + 22, cx - w / 2 + w, r->top + 22 + h);
    CopyBits(&bm, &gWin->portBits, &src, &dst, srcCopy, NULL);
    DrawCentred(cx, r->bottom - 2, "Press any key or click to go on.");
    TextFont(0); TextSize(12);
}

static void DrawWindow(void)
{
    Rect r = gWin->portRect;
    SetPort(gWin);
    EraseRect(&r);
    Trace("draw");
    switch (gView) {
    case vCount: DrawCountdown(&r); break;
    case vOrbit: DrawOrbit(&r); break;
    case vTilt:  DrawTilt(&r); break;
    case vMap:   DrawMap(&r); break;
    case vSun:   DrawSunNow(&r); break;
    case vTown:  DrawTown(&r); break;
    case vFact:  DrawFact(&r); break;
    case vTrad:  DrawTraditions(&r); break;
    case vPics:  DrawPics(&r); break;
    case vSplash: DrawSplash(&r); break;
    }
}

/* ---------- the usual Mac application ---------- */

static void SetView(short v)
{
    MenuHandle m = GetMenuHandle(mSeasons);
    short i;
    if (v == vFact) gFact++;                     /* each time: the next fact */
    if (v == vPics) gPic++;                      /* and the next picture */
    gView = v;
    if (m) for (i = 1; i <= vMap; i++) if (i != iSetTown && i != iSetZone) CheckItem(m, i, i == v);
    SetPort(gWin);                               /* InvalRect works on the current port: at launch there is none yet */
    InvalRect(&gWin->portRect);
}

static void About(void)
{
    DialogPtr d = GetNewDialog(129, NULL, (WindowPtr)-1);
    short item = 0;
    if (!d) return;
    ShowWindow(d);
    while (item != 1) ModalDialog(NULL, &item);
    DisposeDialog(d);
}

static void DoMenu(long mi)
{
    short menu = (short)(mi >> 16), item = (short)mi;
    switch (menu) {
    case mApple:
        if (item == iAbout) About();
        else { Str255 name; GetMenuItemText(GetMenuHandle(mApple), item, name); OpenDeskAcc(name); }
        break;
    case mFile: if (item == iQuit) gQuit = true; break;
    case mEdit: SystemEdit(item - 1); break;
    case mSeasons:
        if (item == iSetTown) AskTown(4);
        else if (item == iSetZone) AskTown(10);
        else SetView(item);
        break;
    }
    HiliteMenu(0);
}

static void DoMouse(EventRecord *e)
{
    WindowPtr w;
    switch (FindWindow(e->where, &w)) {
    case inMenuBar: DoMenu(MenuSelect(e->where)); break;
    case inSysWindow: SystemClick(e, w); break;
    case inDrag: DragWindow(w, e->where, &qd.screenBits.bounds); break;
    case inContent:
        if (w != FrontWindow()) SelectWindow(w);
        else if (gView == vSplash) SetView(vCount);   /* a click goes on from the opening picture */
        break;
    }
}

static void Setup(void)
{
    Rect r;
    short w, h;
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
    FlushEvents(everyEvent, 0);
    gHasWNE = TrapAvailable(0xA860);
    Trace("menus");
    SetMenuBar(GetNewMBar(128));
    AppendResMenu(GetMenuHandle(mApple), 'DRVR');
    DrawMenuBar();
    Trace("window");
    gWin = GetNewWindow(128, NULL, (WindowPtr)-1);
    r = qd.screenBits.bounds;                    /* a Plus's 512 x 342, the XL's 608 x 431, or bigger */
    w = r.right - r.left - 16; if (w > 600) w = 600;
    h = r.bottom - r.top - 52; if (h > 380) h = 380;
    SizeWindow(gWin, w, h, true);
    MoveWindow(gWin, (r.right - r.left - w) / 2, 44, true);
    ShowWindow(gWin);
    Trace("place");
    SetupPlace();
    Trace("view");
    SetView(vSplash);                            /* opens on the solargraph, as LisaSolstice */
    Trace("setup done");
}

int main(void)
{
    EventRecord e;
    Setup();
    while (!gQuit) {
        Boolean got = gHasWNE ? WaitNextEvent(everyEvent, &e, 60, NULL) : (SystemTask(), GetNextEvent(everyEvent, &e));
        unsigned long nowMin = NowLocalSecs() / 60;
        if (nowMin != gLastMinute) { gLastMinute = nowMin; SetPort(gWin); InvalRect(&gWin->portRect); }
        if (!got) continue;
        switch (e.what) {
        case mouseDown: DoMouse(&e); break;
        case keyDown: case autoKey:
            if (e.modifiers & cmdKey) DoMenu(MenuKey((char)(e.message & charCodeMask)));
            else {
                /* the Lisa's plain letter keys; on the opening picture any other key goes on to the countdown */
                static const char kKeys[] = "DOEWY....FLPG.M";
                char c = (char)(e.message & charCodeMask), *k;
                if (c >= 'a' && c <= 'z') c -= 32;
                if (c != '.' && c && (k = strchr(kKeys, c)) != NULL) SetView((short)(k - kKeys + 1));
                else if (gView == vSplash) SetView(vCount);
            }
            break;
        case updateEvt:
            BeginUpdate((WindowPtr)e.message);
            DrawWindow();
            EndUpdate((WindowPtr)e.message);
            break;
        }
    }
    return 0;
}
