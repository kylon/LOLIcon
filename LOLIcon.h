#define LEFT_LABEL_X CENTER(24)
#define RIGHT_LABEL_X CENTER(0)
#define printf ksceDebugPrintf

#define CONFIG_PATH		"ur0:LOLIcon/"
#define TIMER_SECOND	1000000 // 1 second
#define TIMER_AMODE 	15000000 // 15 seconds
#define TIMER_AMODE_R	30000000 // 30 seconds
#define CUSTOM_BTNS_NUM	12
#define HOOKS_NUM		14

#define NO_ERROR	0
#define SAVE_ERROR	1
#define SAVE_GOOD	2
#define LOAD_ERROR	3
#define LOAD_GOOD	4

#define MENU_OPTION_F(TEXT,...)\
	blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
	blit_stringf(LEFT_LABEL_X, 120+16*entries++, (TEXT), __VA_ARGS__);

#define MENU_OPTION_FC(POS,TEXT,...)\
	blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
	blit_stringf(LEFT_LABEL_X, POS+16*entries++, (TEXT), __VA_ARGS__);

#define MENU_OPTION(TEXT,...)\
	blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
	blit_stringf(LEFT_LABEL_X, 120+16*entries++, (TEXT));

#define MENU_OPTION_C(POS,TEXT,...)\
	blit_set_color(0x00FFFFFF, (pos != entries) ? 0x00FF0000 : 0x0000FF00);\
	blit_stringf(LEFT_LABEL_X, POS+16*entries++, (TEXT));

#define ksceKernelExitProcess               _ksceKernelExitProcess
#define ksceKernelGetModuleInfo             _ksceKernelGetModuleInfo
#define ksceKernelGetModuleList             _ksceKernelGetModuleList
#define kscePowerGetGpuEs4ClockFrequency	_kscePowerGetGpuEs4ClockFrequency
#define kscePowerSetGpuEs4ClockFrequency	_kscePowerSetGpuEs4ClockFrequency
#define kscePowerGetGpuClockFrequency		_kscePowerGetGpuClockFrequency
#define kscePowerSetGpuClockFrequency		_kscePowerSetGpuClockFrequency

int (*_kscePowerGetGpuEs4ClockFrequency)(int*, int*);
int (*_kscePowerSetGpuEs4ClockFrequency)(int, int);
int (*_kscePowerGetGpuClockFrequency)(void);
int (*_kscePowerSetGpuClockFrequency)(int);
int (*_ksceKernelGetModuleInfo)(SceUID, SceUID, SceKernelModuleInfo *);
int (*_ksceKernelGetModuleList)(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);
int (*_ksceKernelExitProcess)(int);
