#ifdef TERM_CONTROL

#define LOOP_CYCLES	0.500000	/* Loop time in sec */

/* 
 * Defines the keybindings in term.c - change to your 
 * own preferences.
 */

#define BACK_KEY	'b'
#define NEXT_KEY	'f'
#define PAUSE_KEY	'p'
#define QUIT_KEY	'q'
#define STOP_KEY	's'
#define REWIND_KEY	','
#define FORWARD_KEY	'.'
/* This is convenient on QWERTZ-keyboards. */
#define FAST_REWIND_KEY ';'
#define FAST_FORWARD_KEY ':'
#define FINE_REWIND_KEY '<'
#define FINE_FORWARD_KEY '>'
/* You probably want to use the following bindings instead
 * on a standard QWERTY-keyboard:
 */
 
/* #define FAST_REWIND_KEY '<' */
/* #define FAST_FORWARD_KEY '>' */
/* #define FINE_REWIND_KEY ';' */
/* #define FINE_FORWARD_KEY ':' */

#define PAUSED_STRING	"Paused. \b\b\b\b\b\b\b\b"
#define STOPPED_STRING	"Stopped.\b\b\b\b\b\b\b\b"
#define EMPTY_STRING	"        \b\b\b\b\b\b\b\b"

void term_init(void);
long term_control(struct frame *fr);
void term_restore(void);

#endif
