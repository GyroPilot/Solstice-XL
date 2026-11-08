/* solstice.r: Solstice's menus, window, dialogs and Finder resources.  Fits a Plus's 512 x 342. */
#include "Multiverse.r"
#include "Processes.r"
#include "icons.r"

resource 'MBAR' (128) { { 128; 129; 130; 131 } };

resource 'MENU' (128) { 128, textMenuProc, 0x7FFFFFFD, enabled, apple, {
    "About Solstice\0xC9", noIcon, noKey, noMark, plain;
    "-", noIcon, noKey, noMark, plain
} };

resource 'MENU' (129) { 129, textMenuProc, 0x7FFFFFFF, enabled, "File", {
    "Quit", noIcon, "Q", noMark, plain
} };

resource 'MENU' (130) { 130, textMenuProc, 0x7FFFFFFD, enabled, "Edit", {
    "Undo", noIcon, "Z", noMark, plain;
    "-", noIcon, noKey, noMark, plain;
    "Cut", noIcon, "X", noMark, plain;
    "Copy", noIcon, "C", noMark, plain;
    "Paste", noIcon, "V", noMark, plain;
    "Clear", noIcon, noKey, noMark, plain
} };

/* 0.1d4: the same names, order and keys as LisaSolstice 0.1 (the banana, 2 Nov 2026); World Map is the Mac's extra.
   The item numbers are the view numbers in main.c. */
resource 'MENU' (131) { 131, textMenuProc, 0x7FFFDEDF, enabled, "Seasons", {
    "Days to the Next Season", noIcon, "D", noMark, plain;
    "The Earth\0xD5s Orbit", noIcon, "O", noMark, plain;
    "The Earth\0xD5s Tilt", noIcon, "E", noMark, plain;
    "Where the Sun Is Now", noIcon, "W", noMark, plain;
    "Your Town", noIcon, "Y", noMark, plain;
    "-", noIcon, noKey, noMark, plain;
    "Set Your Town\0xC9", noIcon, noKey, noMark, plain;
    "Set Your Time Zone\0xC9", noIcon, noKey, noMark, plain;
    "-", noIcon, noKey, noMark, plain;
    "A Fun Fact", noIcon, "F", noMark, plain;
    "Solstice Traditions", noIcon, "L", noMark, plain;
    "Pictures from History", noIcon, "P", noMark, plain;
    "The Solargraph", noIcon, "G", noMark, plain;
    "-", noIcon, noKey, noMark, plain;
    "World Map", noIcon, "M", noMark, plain
} };

resource 'WIND' (128) { { 44, 8, 330, 504 }, noGrowDocProc, invisible, noGoAway, 0, "Solstice", noAutoCenter };

/* Your Town: items 4, 6, 8, 10 are the fields */
resource 'DLOG' (128) { { 50, 40, 250, 470 }, dBoxProc, invisible, noGoAway, 0, 128, "", centerMainScreen };
resource 'DITL' (128) { {
    { 168, 340, 188, 410 }, Button { enabled, "OK" };
    { 168, 256, 188, 326 }, Button { enabled, "Cancel" };
    { 12, 14, 30, 120 }, StaticText { disabled, "Town:" };
    { 12, 130, 28, 410 }, EditText { enabled, "" };
    { 42, 14, 60, 120 }, StaticText { disabled, "Latitude:" };
    { 42, 130, 58, 200 }, EditText { enabled, "" };
    { 72, 14, 90, 120 }, StaticText { disabled, "Longitude:" };
    { 72, 130, 88, 200 }, EditText { enabled, "" };
    { 102, 14, 120, 120 }, StaticText { disabled, "Hours from UT:" };
    { 102, 130, 118, 200 }, EditText { enabled, "" };
    { 42, 214, 160, 412 }, StaticText { disabled,
        "Degrees, with a minus for south and west (Cincinnati: 39.10, -84.51). Hours from UT: -5 for "
        "Eastern Standard Time, -4 in summer. Kept in \"Solstice Settings\" beside the application." }
} };

resource 'DLOG' (129) { { 40, 40, 294, 470 }, dBoxProc, invisible, noGoAway, 0, 129, "", centerMainScreen };
resource 'DITL' (129) { {
    { 226, 340, 246, 410 }, Button { enabled, "OK" };
    { 10, 14, 92, 412 }, StaticText { disabled,
        "Solstice 1.0. How long until the next solstice or equinox, and where the Sun and the Earth are "
        "now. Computed on this Mac (Meeus, ch. 27), no network needed. World map: Natural Earth (public domain)." };
    { 96, 14, 160, 412 }, StaticText { disabled,
        "The opening picture: the author's solargraph, a pinhole camera in a pop can on his roof from the June to "
        "the December solstice. Solstice history from Tim and Connie O'Connor's Winter Solstice talk." };
    { 164, 14, 220, 412 }, StaticText { disabled,
        "Pictures from History, photos from Wikimedia Commons: the Newgrange chamber by Dentp, CC BY-SA 4.0; "
        "Stonehenge by Jeffrey Pfau, CC BY-SA 3.0. Their copies here are under the same licences." }
} };

data 'SlSt' (0, "Owner resource") { $"0C536F6C 73746963 6520312E 30202020" };
resource 'FREF' (128) { 'APPL', 0, "" };
resource 'BNDL' (128) { 'SlSt', 0, { 'ICN#', { 0, 128 }; 'FREF', { 0, 128 } } };

resource 'vers' (1) { 0x01, 0x00, release, 0x00, verUS, "1.0", "1.0, Solstice for the classic Mac" };

resource 'SIZE' (-1) {
    reserved, acceptSuspendResumeEvents, reserved, canBackground, doesActivateOnFGSwitch,
    backgroundAndForeground, dontGetFrontClicks, ignoreChildDiedEvents, is32BitCompatible,
    notHighLevelEventAware, onlyLocalHLEvents, notStationeryAware, dontUseTextEditServices,
    reserved, reserved, reserved,
    448 * 1024,                      /* 0.1d7: the six pictures add 59 KB */
    384 * 1024
};
