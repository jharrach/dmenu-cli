#ifndef ANSI_H
# define ANSI_H

/* Control Sequence Introducer */
#define CSI "\e[" 

/*
 * Moves the cursor n (default 1) cells in the given direction.
 * If the cursor is already at the edge of the screen, this has no effect. 
*/
#define ANSI_CUU(n) CSI #n "A"
#define ANSI_CUD(n) CSI #n "B"
#define ANSI_CUF(n) CSI #n "C"
#define ANSI_CUB(n) CSI #n "D"

/* Moves cursor to beginning of the line n (default 1) lines down. */
#define ANSI_CNL(n) CSI #n "E"

/* Moves cursor to beginning of the line n (default 1) lines up. */
#define ANSI_CPL(n) CSI #n "F"

/* Moves cursor to column n (default 1). */
#define ANSI_CHA(n) CSI #n "G"

/*
 * Moves the cursor to row n, column m.
 * The values are 1-based, and default to 1 (top left corner) if omitted.
 * A sequence such as CSI ;5H is a synonym for CSI 1;5H
 * as well as CSI 17;H is the same as CSI 17H and CSI 17;1H
 */
#define ANSI_CUP(n, m) CSI #n ";" #m "H"

/*
 * Clears part of the screen.
 * If n is 0 (or missing), clear from cursor to end of screen.
 * If n is 1, clear from cursor to beginning of the screen.
 * If n is 2, clear entire screen (and moves cursor to upper left on DOS ANSI.SYS).
 * If n is 3, clear entire screen and delete all lines saved in the scrollback buffer.
 */
#define ANSI_ED(n) CSI #n "J"

/*
 * Erases part of the line.
 * If n is 0 (or missing), clear from cursor to the end of the line.
 * If n is 1, clear from cursor to beginning of the line.
 * If n is 2, clear entire line.
 * Cursor position does not change. 
 */
#define ANSI_EL(n) CSI #n "K"

/*
 * Scroll whole page up by n (default 1) lines.
 * New lines are added at the bottom.
 */
#define ANSI_SU(n) CSI #n "S"

/*
 * Scroll whole page down by n (default 1) lines.
 * New lines are added at the top. 
 */
#define ANSI_SD(n) CSI #n "T"

/*
 * Reports the cursor position (CPR) by transmitting ESC[n;mR,
 * where n is the row and m is the column. 
 */
#define ANSI_DSR CSI "6n"

#define ANSI_CUSHOW                     CSI "?25h"
#define ANSI_CUHIDE                     CSI "?25l"
#define ANSI_ENABLE_FOCUS_REPORT        CSI "?1004h"
#define ANSI_DISABLE_FOCUS_REPORT       CSI "?1004l"
#define ANSI_ENABLE_ALTERNATIVE_BUFFER  CSI "?1049h"
#define ANSI_DISABLE_ALTERNATIVE_BUFFER CSI "?1049l"

/* colors */

#define ANSI_FG_BLACK                   CSI "30m"
#define ANSI_FG_RED                     CSI "31m"
#define ANSI_FG_GREEN                   CSI "32m"
#define ANSI_FG_YELLOW                  CSI "33m"
#define ANSI_FG_BLUE                    CSI "34m"
#define ANSI_FG_MAGENTA                 CSI "35m"
#define ANSI_FG_CYAN                    CSI "36m"
#define ANSI_FG_WHITE                   CSI "37m"
#define ANSI_FG_DEFAULT                 CSI "39m"

#define ANSI_FG_BRIGHT_BLACK            CSI "90m"
#define ANSI_FG_BRIGHT_RED              CSI "91m"
#define ANSI_FG_BRIGHT_GREEN            CSI "92m"
#define ANSI_FG_BRIGHT_YELLOW           CSI "93m"
#define ANSI_FG_BRIGHT_BLUE             CSI "94m"
#define ANSI_FG_BRIGHT_MAGENTA          CSI "95m"
#define ANSI_FG_BRIGHT_CYAN             CSI "96m"
#define ANSI_FG_BRIGHT_WHITE            CSI "97m"

#define ANSI_BG_BLACK                   CSI "40m"
#define ANSI_BG_RED                     CSI "41m"
#define ANSI_BG_GREEN                   CSI "42m"
#define ANSI_BG_YELLOW                  CSI "43m"
#define ANSI_BG_BLUE                    CSI "44m"
#define ANSI_BG_MAGENTA                 CSI "45m"
#define ANSI_BG_CYAN                    CSI "46m"
#define ANSI_BG_WHITE                   CSI "47m"
#define ANSI_BG_DEFAULT                 CSI "49m"

#define ANSI_BG_BRIGHT_BLACK            CSI "100m"
#define ANSI_BG_BRIGHT_RED              CSI "101m"
#define ANSI_BG_BRIGHT_GREEN            CSI "102m"
#define ANSI_BG_BRIGHT_YELLOW           CSI "103m"
#define ANSI_BG_BRIGHT_BLUE             CSI "104m"
#define ANSI_BG_BRIGHT_MAGENTA          CSI "105m"
#define ANSI_BG_BRIGHT_CYAN             CSI "106m"
#define ANSI_BG_BRIGHT_WHITE            CSI "107m"

#endif /* ANSI_H */
