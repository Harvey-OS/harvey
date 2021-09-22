/* Translated by Comeau C/C++ (version 4.3.10.1) */
/* Wed Dec 23 21:07:49 2009 */


/* WARNING: This code must ONLY be used with Plan9_BETA12 */
/* and the Plan_9_native_8c C x86 compiler via como only */

/* WARNING: This is copyrighted and proprietary code of Comeau Computing */
/* and its suppliers with specific permissions and restrictions. */

/* THIS IS AS PER YOUR LICENSE, http://www.comeaucomputing.com/license.html */
/* To use this file in any other manner, contact comeau@comeaucomputing.com */

void *memcpy(); 
void *memset();
struct __T17602588 ; 
struct __T17601456 ; 
struct __C1 ; 
struct __C2 ; 
struct __C4 ; 
struct __C6 ; 
struct __C7 ; 
union __C8 ; 
struct __C9 ; 
struct __C10 ;
struct __Q2_3std9exception ;
struct __Q2_3std9bad_alloc ;
struct __Q2_3std12_Locale_impl ;

struct __Q2_3std6locale ;
enum __Q3_3std10ctype_base4mask {
	space__Q2_3std10ctype_base = 8,
	print__Q2_3std10ctype_base = 87,
	cntrl__Q2_3std10ctype_base = 32,
	upper__Q2_3std10ctype_base = 1,
	lower__Q2_3std10ctype_base,
	alpha__Q2_3std10ctype_base,
	digit__Q2_3std10ctype_base,
	punct__Q2_3std10ctype_base = 16,
	xdigit__Q2_3std10ctype_base = 128,
	alnum__Q2_3std10ctype_base = 7,
	graph__Q2_3std10ctype_base = 23};
struct __Q2_3std58pair__tm__46_PFQ3_3std8ios_base5eventRQ2_3std8ios_basei_vi ;
enum __Q3_3std8ios_base5event { 
	erase_event__Q2_3std8ios_base, imbue_event__Q2_3std8ios_base, copyfmt_event__Q2_3std8ios_base};
struct __Q2_3std8ios_base ;
struct __Q3_3std8ios_base4Init ;
enum __Q3_3WTF8Internal9AllocType {
	AllocTypeMalloc__Q2_3WTF8Internal = 928868176,
	AllocTypeClassNew__Q2_3WTF8Internal,
	AllocTypeClassNewArray__Q2_3WTF8Internal,
	AllocTypeFastNew__Q2_3WTF8Internal,
	AllocTypeFastNewArray__Q2_3WTF8Internal,
	AllocTypeNew__Q2_3WTF8Internal,
	AllocTypeNewArray__Q2_3WTF8Internal};
struct __Q2_7WebCore7IntSize ;
struct __Q2_7WebCore8IntPoint ;
struct __Q2_7WebCore7IntRect ;
enum __Q2_7WebCore20ScrollbarOrientation { 
	HorizontalScrollbar__7WebCore, VerticalScrollbar__7WebCore};

enum __Q2_7WebCore13ScrollbarMode { 
	ScrollbarAuto__7WebCore, ScrollbarAlwaysOff__7WebCore, ScrollbarAlwaysOn__7WebCore};

enum __Q2_7WebCore20ScrollbarControlSize { 
	RegularScrollbar__7WebCore, SmallScrollbar__7WebCore};
enum __Q2_7WebCore13ScrollbarPart {
	NoPart__7WebCore,
	BackButtonStartPart__7WebCore,
	ForwardButtonStartPart__7WebCore,
	BackTrackPart__7WebCore = 4U,
	ThumbPart__7WebCore = 8U,
	ForwardTrackPart__7WebCore = 16U,
	BackButtonEndPart__7WebCore = 32U,
	ForwardButtonEndPart__7WebCore = 64U,
	ScrollbarBGPart__7WebCore = 128U,
	TrackBGPart__7WebCore = 256U,
	AllParts__7WebCore = 4294967295U};
struct __Q2_3WTF13FastAllocBase ;
struct __Q2_14WTFNoncopyable11Noncopyable ;
struct __Q2_3WTF5Mutex ;
struct __Q2_7WebCore9TimerBase ;
struct __Q2_3WTF14RefCountedBase ;
struct __Q2_3WTF37RefCounted__tm__19_Q2_7WebCore6Widget ;
struct __Q2_7WebCore6Widget ;
struct __Q2_7WebCore14ScrollbarTheme ;
struct __Q2_7WebCore35Timer__tm__22_Q2_7WebCore9Scrollbar ;
struct __Q2_7WebCore9Scrollbar ;
struct __Q2_7WebCore15ScrollbarClient ;
struct __Q2_3WTF68IdentityExtractor__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;
struct __CPR412____Q2_3WTF400HashTableConstIterator__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ51JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ51JQ2_3WTF58PtrHash__tm__J160JQ2_3WTF61HashTraits__tm__J160JQ2_3WTFJ279J ;
struct __Q2_3WTF62DefaultHash__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;
struct __Q2_3WTF36RefPtr__tm__22_Q2_7WebCore9Scrollbar ;
struct __Q2_3WTF80GenericHashTraitsBase__tm__51_XCbL_1_0Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;
struct __Q2_3WTF68GenericHashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;




struct __Q2_3WTF61HashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;
struct __CPR399____Q2_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J ;
struct __CPR208____Q2_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J ;
struct __Q2_7WebCore10ScrollView ;
struct __Q2_3WTF58PtrHash__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ;
struct __Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget ; 
struct __T17602588 { 
	struct __T17602588 *next; 
	void (*ctor)(); 
	void (*dtor)();
}; 
struct __T17601456 { 
	struct __T17601456 *next; 
	void * object; 
	void (*dtor)();
}; 
struct __C2 { 
	struct __C10 *regions; 
	void * *obj_table; 
	struct __C1 *array_table;
	unsigned short	saved_region_number;
	char	__dummy[2];
}; 
struct __C4 { 
	short	d; 
	short	i; 
	void (*f)();
}; 
struct __C7 { 
	int	setjmp_buffer[10]; 
	struct __C6 *catch_entries; 
	void * rtinfo; 
	unsigned short	region_number;
	char	__dummy[2];
}; 
union __C8 { 
	struct __C7 try_block; 
	struct __C2 function; 
	struct __C6 *
	throw_spec;
}; 
struct __C9 { 
	struct __C9 *next; 
	unsigned char	kind; 
	union __C8 variant;
}; 
struct __C10 { 
	void (*dtor)(); 
	unsigned short	handle; 
	unsigned short	next; 
	unsigned char	flags;
	char	__dummy[3];
};
typedef unsigned long	size_t;
struct __Q2_3std9exception { 
	const struct __C4 *__vptr;
};
typedef long	ptrdiff_t;
typedef ptrdiff_t __Q2_3std10streamsize;
struct __Q2_3std9bad_alloc { 
	struct __Q2_3std9exception __b_Q2_3std9exception;
};
struct __Q2_3std6locale {
	struct __Q2_3std12_Locale_impl *_M_impl;
};
typedef unsigned	__Q3_3std8ios_base8fmtflags;
typedef unsigned char	__Q3_3std8ios_base7seekdir;
typedef unsigned char	__Q3_3std8ios_base7iostate;
typedef unsigned short	__Q3_3std8ios_base8openmode;
typedef void (*__Q3_3std8ios_base14event_callback)(enum __Q3_3std8ios_base5event , struct __Q2_3std8ios_base *, int);
struct __Q2_3std8ios_base {
	__Q3_3std8ios_base8fmtflags _M_fmtflags;
	__Q3_3std8ios_base7iostate _M_iostate;
	__Q3_3std8ios_base8openmode _M_openmode;
	__Q3_3std8ios_base7seekdir _M_seekdir;
	__Q3_3std8ios_base7iostate _M_exception_mask;

	__Q2_3std10streamsize _M_precision;
	__Q2_3std10streamsize _M_width;

	struct __Q2_3std6locale _M_locale;

	struct __Q2_3std58pair__tm__46_PFQ3_3std8ios_base5eventRQ2_3std8ios_basei_vi *_M_callbacks;
	size_t _M_num_callbacks;
	size_t _M_callback_index;


	long	*_M_iwords;
	size_t _M_num_iwords;

	void * *_M_pwords;
	size_t _M_num_pwords; 
	const struct __C4 * __vptr;
};
struct __Q3_3std8ios_base4Init {
	char	__dummy[4];
};
typedef unsigned	uint32_t;
struct __Q2_7WebCore7IntSize {
	int	m_width; 
	int	m_height;
};
struct __Q2_7WebCore8IntPoint {
	int	m_x; 
	int	m_y;
};
struct __Q2_7WebCore7IntRect {
	struct __Q2_7WebCore8IntPoint m_location;
	struct __Q2_7WebCore7IntSize m_size;
};
struct __Q2_3WTF13FastAllocBase {
	char	__dummy[4];
};
struct __Q2_14WTFNoncopyable11Noncopyable { 
	struct __Q2_3WTF13FastAllocBase __b_Q2_3WTF13FastAllocBase;
};
typedef uint32_t __Q2_3WTF16ThreadIdentifier;
typedef void *__Q2_3WTF13PlatformMutex;




struct __Q2_3WTF5Mutex {
	__Q2_3WTF13PlatformMutex m_mutex;
};
struct __Q2_7WebCore9TimerBase {
	double	m_nextFireTime;
	double	m_repeatInterval;
	int	m_heapIndex;
	unsigned	m_heapInsertionOrder;


	__Q2_3WTF16ThreadIdentifier m_thread; 
	const struct __C4 * __vptr;
};
typedef void *PlatformWidget;
struct __Q2_3WTF14RefCountedBase {
	int	m_refCount;

	char	m_deletionHasBegun;
	char	__dummy[3];
};




struct __Q2_3WTF37RefCounted__tm__19_Q2_7WebCore6Widget { 
	struct __Q2_3WTF14RefCountedBase __b_Q2_3WTF14RefCountedBase;
};
struct __Q2_7WebCore6Widget { 
	struct __Q2_3WTF37RefCounted__tm__19_Q2_7WebCore6Widget __b_Q2_3WTF37RefCounted__tm__19_Q2_7WebCore6Widget;
	struct __Q2_7WebCore10ScrollView *m_parent;
	PlatformWidget m_widget;
	char	m_selfVisible;
	char	m_parentVisible;

	struct __Q2_7WebCore7IntRect m_frame; 
	const struct __C4 * __vptr;
};
typedef struct __C4 __Q3_7WebCore35Timer__tm__22_Q2_7WebCore9Scrollbar18TimerFiredFunction;
struct __Q2_7WebCore35Timer__tm__22_Q2_7WebCore9Scrollbar { 
	struct __Q2_7WebCore9TimerBase __b_Q2_7WebCore9TimerBase;
	struct __Q2_7WebCore9Scrollbar *m_object;
	__Q3_7WebCore35Timer__tm__22_Q2_7WebCore9Scrollbar18TimerFiredFunction m_function;
};
struct __Q2_7WebCore9Scrollbar { 
	struct __Q2_7WebCore6Widget __b_Q2_7WebCore6Widget;
	struct __Q2_7WebCore15ScrollbarClient *m_client;
	enum __Q2_7WebCore20ScrollbarOrientation m_orientation;
	enum __Q2_7WebCore20ScrollbarControlSize m_controlSize;
	struct __Q2_7WebCore14ScrollbarTheme *m_theme;

	int	m_visibleSize;
	int	m_totalSize;
	float	m_currentPos;
	float	m_dragOrigin;
	int	m_lineStep;
	int	m_pageStep;
	float	m_pixelStep;

	enum __Q2_7WebCore13ScrollbarPart m_hoveredPart;
	enum __Q2_7WebCore13ScrollbarPart m_pressedPart;
	int	m_pressedPos;

	char	m_enabled;

	struct __Q2_7WebCore35Timer__tm__22_Q2_7WebCore9Scrollbar m_scrollTimer;
	char	m_overlapsResizer;

	char	m_suppressInvalidation;
	char	__dummy[2];
};
struct __Q2_7WebCore15ScrollbarClient { 
	const struct __C4 *__vptr;
};
typedef struct __Q2_3WTF58PtrHash__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget __Q3_3WTF73DefaultHash__ps__27_Q2_3WTF17RefPtr__tm__4_Z1Z__tm__19_Q2_7WebCore6Widget4Hash;
struct __Q2_3WTF62DefaultHash__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget {
	char	__dummy[4];
};
struct __Q2_3WTF36RefPtr__tm__22_Q2_7WebCore9Scrollbar {
	struct __Q2_7WebCore9Scrollbar *m_ptr;
};
typedef __Q3_3WTF73DefaultHash__ps__27_Q2_3WTF17RefPtr__tm__4_Z1Z__tm__19_Q2_7WebCore6Widget4Hash __CPR223____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J13HashFunctions;
typedef struct __Q2_3WTF61HashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget __CPR221____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J11ValueTraits;
struct __Q2_3WTF80GenericHashTraitsBase__tm__51_XCbL_1_0Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget {
	char	__dummy[4];
};
typedef struct __Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget __Q3_3WTF68GenericHashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget9TraitType;
struct __Q2_3WTF68GenericHashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget { 
	struct __Q2_3WTF80GenericHashTraitsBase__tm__51_XCbL_1_0Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget __b_Q2_3WTF80GenericHashTraitsBase__tm__51_XCbL_1_0Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget;
};
typedef __Q3_3WTF68GenericHashTraits__tm__43_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6Widget9TraitType __CPR218____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J9ValueType;



typedef struct __CPR399____Q2_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J __CPR223____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J13HashTableType;
typedef struct __CPR412____Q2_3WTF400HashTableConstIterator__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ51JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ51JQ2_3WTF58PtrHash__tm__J160JQ2_3WTF61HashTraits__tm__J160JQ2_3WTFJ279J __CPR415____Q3_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J14const_iterator;


typedef __CPR218____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J9ValueType 
__CPR409____Q3_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J9ValueType;
struct __CPR399____Q2_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J {
	__CPR409____Q3_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J9ValueType *m_table;
	int	m_tableSize;
	int	m_tableSizeMask;
	int	m_keyCount;
	int	m_deletedCount;




	__CPR415____Q3_3WTF387HashTable__tm__369_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTFJ38JQ2_3WTF68IdentityExtractor__tm__43_Q2_3WTFJ38JQ2_3WTF58PtrHash__tm__J147JQ2_3WTF61HashTraits__tm__J147JQ2_3WTFJ266J14const_iterator * m_iterators;
	struct __Q2_3WTF5Mutex m_mutex;
};
struct __CPR208____Q2_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J {
	__CPR223____Q3_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J13HashTableType m_impl;
};
struct __Q2_7WebCore10ScrollView { 
	struct __Q2_7WebCore6Widget __b_Q2_7WebCore6Widget; 
	struct __Q2_7WebCore15ScrollbarClient __b_Q2_7WebCore15ScrollbarClient;
	struct __Q2_3WTF36RefPtr__tm__22_Q2_7WebCore9Scrollbar m_horizontalScrollbar;
	struct __Q2_3WTF36RefPtr__tm__22_Q2_7WebCore9Scrollbar m_verticalScrollbar;
	enum __Q2_7WebCore13ScrollbarMode m_horizontalScrollbarMode;
	enum __Q2_7WebCore13ScrollbarMode m_verticalScrollbarMode;
	char	m_prohibitsScrolling;

	struct __CPR208____Q2_3WTF196HashSet__tm__180_Q2_3WTF33RefPtr__tm__19_Q2_7WebCore6WidgetQ2_3WTF58PtrHash__tm__43_Q2_3WTFJ36JQ2_3WTF61HashTraits__tm__J93J m_children;



	char	m_canBlitOnScroll;

	struct __Q2_7WebCore7IntSize m_scrollOffset;
	struct __Q2_7WebCore7IntSize m_fixedLayoutSize;
	struct __Q2_7WebCore7IntSize m_contentsSize;

	int	m_scrollbarsAvoidingResizer;
	char	m_scrollbarsSuppressed;

	char	m_inUpdateScrollbars;
	unsigned	m_updateScrollbarsPass;

	struct __Q2_7WebCore8IntPoint m_panScrollIconPoint;
	char	m_drawPanScrollIcon;
	char	m_useFixedLayout;

	char	m_paintsEntireContents;
	char	__dummy;
};
extern void *__nw__FUl(size_t);


extern void __dl__FPv(void *);
extern void _S_initialize__Q2_3std8ios_baseSFv(void);
extern void _S_uninitialize__Q2_3std8ios_baseSFv(void);


static void __dt__Q3_3std8ios_base4InitFv(struct __Q3_3std8ios_base4Init *const, int);
extern void __sti___10_scroll_cpp_6362c42a(void); 
extern void __record_needed_destruction(); 
extern unsigned short	__eh_curr_region; 
extern struct __C9 *__curr_eh_stack_entry; 
int	
__LSG____x__L0__infinity__Q2_3std26numeric_limits__tm__2_f__SSFv; 
int	__LSG____x__L0__quiet_NaN__Q2_3std26numeric_limits__tm__2_f__SSFv; 
int	__LSG____x__L0__signaling_NaN__Q2_3std26numeric_limits__tm__2_f__SSFv; 
int	__LSG____x__L0__infinity__Q2_3std26numeric_limits__tm__2_d__SSFv; 
int	
__LSG____x__L0__quiet_NaN__Q2_3std26numeric_limits__tm__2_d__SSFv; 
int	__LSG____x__L0__signaling_NaN__Q2_3std26numeric_limits__tm__2_d__SSFv; 
int	__LSG____x__L0__infinity__Q2_3std26numeric_limits__tm__2_r__SSFv; 
int	__LSG____x__L0__quiet_NaN__Q2_3std26numeric_limits__tm__2_r__SSFv; 
int	
__LSG____x__L0__signaling_NaN__Q2_3std26numeric_limits__tm__2_r__SSFv;
extern long	_S_count__Q3_3std8ios_base4Init;




static struct __Q3_3std8ios_base4Init __initializer__3std = {
	0}; 


static struct __T17602588 __link; 
static struct __T17602588 __link = {
	((struct __T17602588 *)0), ((void (*)())__sti___10_scroll_cpp_6362c42a), ((void (*)())0)};





int	fgb1__Q2_7WebCore10ScrollViewFv( struct __Q2_7WebCore10ScrollView *const this)
{ 
	auto struct __Q2_7WebCore7IntRect __T16453888; 
	auto const struct __C4 * __T16453972; 
	auto const struct __Q2_7WebCore7IntRect * __T16454056; 
	auto const struct __Q2_7WebCore6Widget * __T16455900; 
	auto struct __Q2_7WebCore7IntRect __T16455984; 
	auto const struct __C4 * __T16456068; 
	auto const struct 
		__Q2_7WebCore7IntRect * __T16456152;
	auto int __34859_6_h; 
	__34859_6_h = (((__T16454056 = ((const struct __Q2_7WebCore7IntRect * )((__T16453888 = ((__T16453972 = ((((*((const struct __Q2_7WebCore6Widget * )(&this->__b_Q2_7WebCore6Widget))).__vptr)) + 3)) , (((struct __Q2_7WebCore7IntRect (*)(const struct __Q2_7WebCore6Widget * const))((
		__T16453972->f)))(((const struct __Q2_7WebCore6Widget * )(((char *)((const struct __Q2_7WebCore6Widget * )(&this->__b_Q2_7WebCore6Widget))) + ((__T16453972->d)))))))) , (&__T16453888)))) , (((__T16454056->m_size).m_width))) - ((__T16455900 = ((const struct __Q2_7WebCore6Widget * )(&(*(((*((const struct 
		__Q2_3WTF36RefPtr__tm__22_Q2_7WebCore9Scrollbar * )(&this->m_verticalScrollbar))).m_ptr))).__b_Q2_7WebCore6Widget))) , ((__T16456152 = ((const struct __Q2_7WebCore7IntRect * )((__T16455984 = ((__T16456068 = (((__T16455900->__vptr)) + 3)) , (((struct __Q2_7WebCore7IntRect (*)(const struct 
		__Q2_7WebCore6Widget * const))((__T16456068->f)))(((const struct __Q2_7WebCore6Widget * )(((char *)__T16455900) + ((__T16456068->d)))))))) , (&__T16455984)))) , (((__T16456152->m_size).m_width)))));
	return __34859_6_h;
} 
