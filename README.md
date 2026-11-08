# Solstice for the classic Mac (1.0, 5 Nov 2026)

How long until the next solstice or equinox, and where the Sun and the Earth are now. The classic Mac companion
to the banana's LisaSolstice on the Lisa; since 0.1d7 the two line up (Tim: "I would like the two apps to largly
align"). 68000 code, System 6 and 7, Plus and up, built with Retro68. No network needed.

It opens on **Tim's solargraph** (a pinhole camera in a pop can on his roof in Cincinnati, June to December
solstice); any key or a click goes on. **Seasons menu**, the same names, order and keys as LisaSolstice 0.1:

| Key | Page | Notes |
|---|---|---|
| D | Days to the Next Season | Now; Next; the time left ("in 48 days, 18 hours and 4 minutes"); how far through the season; **Mac extra:** the next five events with days to go (Tim, 0.1d2) |
| O | The Earth's Orbit | from above the North Pole, March left, June bottom, September right, December top, "Earth now" |
| E | The Earth's Tilt | Sun left, night half shaded, axis, equator, dotted tropics and polar circles, "The north leans ... degrees ..." |
| W | Where the Sun Is Now | overhead point, declination, the equation of time ("A sundial is ... AHEAD of the clock"), the orbit since the March equinox |
| Y | Your Town | place and zone; today; sunrise, sunset, daylight and noon height at both solstices and both equinoxes |
| | Set Your Town..., Set Your Time Zone... | one dialog; kept in "Solstice Settings" beside the app (else the Map control panel, else Cincinnati UTC-5) |
| F | A Fun Fact | the 25 facts LisaSolstice carries (Tim and Connie O'Connor's Winter Solstice talk, the evergreen fact, 8 astronomy facts), in turn; art/facts.txt -> src/facts.c (tools/mkfacts.py) |
| L | Solstice Traditions | Tim and Connie's timeline (checked by the banana): how many winter solstices each custom has seen, counted to this year |
| P | Pictures from History | the six Tim chose for both apps (3 Nov), oldest site first: Newgrange (Wakeman 1903), the Newgrange chamber at midwinter (photo Dentp, CC BY-SA 4.0), Stonehenge at sunset (photo Jeffrey Pfau, CC BY-SA 3.0), Great Serpent Mound (old postcard), the Yule log (1800s engraving), the heavens (1600s engraving); captions word for word; sources in art/history/SOURCES.txt; art/history -> src/pictures.c (tools/mkpictures.py) |
| G | The Solargraph | the opening picture again |
| M | World Map | **Mac extra:** day and night now, the overhead point, your town |

Letter keys work alone or with Command (the Apple key on a Macintosh XL).

| Path | What |
|---|---|
| `src/astro.c` | the Sun: Meeus ch. 27 seasons, Almanac Sun position, sunrise/sunset, terminator (portable C) |
| `src/main.c`, `src/solstice.r` | the application (facts are STR# 200 in solstice.r) |
| `src/splash.c` | the solargraph, 512 x 325 1-bit, by `tools/mksplash.py` from `art/solargraph_mac_512x325.png` (the banana's `art/mac_versions.py`: blue channel, 2-98 % stretch, Floyd-Steinberg) |
| `src/worldmap.c` | 360 x 180 1-bit map from Natural Earth 1:110m land (public domain), by `tools/mkworld.py` |
| `src/icons.r` | the four-seasons icon, shared with LisaSolstice (the banana's `tools/seasons_icon.py`) |
| `tests/astro_test.c` | host check against USNO season times 2024-2025 (all within a minute) and a Cincinnati sunrise |
| `test/run_qemu.sh` | System 7.6.1 in QEMU: every page by its letter key, a screenshot each |
| `tools/package.sh VERSION` | dist/ (.dc42, .bin, page PNGs) and the unpacked app + READ ME for the shares |

Tested: host test PASS; a real Macintosh XL (MacWorks Plus II, System 7.5.5); emulators from System 6.0.2 to 7.5.5
(Snow, Mac Plus ROM; QEMU, System 7.6.1: every page draws).  QEMU's Mac clock runs on UT, so in the emulator "Now"
is 5 hours off; a real Mac set to local time is right.

Release: 1.0 (5 Nov 2026), an 800K Disk Copy 4.2 disk with the application and its READ ME, on the GitHub release
page; also on the "Mac XL Apps 1.1" disk.  By GyroPilot (Tim O'Connor).  World map: Natural Earth (public domain).
Pictures from History: see art/history/SOURCES.txt (two photos are CC BY-SA; their copies here keep those licences).

Lessons: InvalRect works on the current port (SetPort first, or a bus error at launch). Retro68's MacBinary has
Finder flags 0: set hasBundle (package.sh does it in the .finf) or the Finder shows a generic icon.
