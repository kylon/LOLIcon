// LOLIcon
// by @dots_tb
// Thanks to:
// @CelesteBlue123 partner in crime, shh he doesn't know I added his namespace
// Our testing team, especially: @Yoyogames28 castelo
// @Cimmerian_Iter is also worth mentioning...

// @frangar for original oclockvita
// HENkaku wiki and its contributors (especially yifan lu for Pervasive RE)
// Team Molecule (because everyone does)
// Scorp for Button Swap
// Please read the readme for more credits

//MADE IN THE USA

#include <taihen.h>
#include <string.h>
#include <sys/syslimits.h>
#include <stdio.h>
#include <stdlib.h>
#include "blit.h"
#include "utils.h"
#include "LOLIcon.h"

static SceUID g_hooks[HOOKS_NUM];
static tai_hook_ref_t ref_hooks[HOOKS_NUM];

typedef struct titleid_config {
	int autoMode;
	int mode;
	int hideErrors;
	int showBat;
	int buttonSwap;
	int showFPS;
	int showCpu;
	int showGpu;
	uint32_t osdColor;
	SceCtrlButtons btn1;
	SceCtrlButtons btn2;
} titleid_config;

static char config_path[PATH_MAX];
static titleid_config current_config;

static char titleid[32];
uint32_t current_pid = 0, shell_pid = 0;

static uint64_t ctrl_timestamp;
static uint64_t menu_timestamp;
int error_code = NO_ERROR;
unsigned aModeTimer = TIMER_AMODE;
int showMenu = 0, pos = 0, isReseting = 0, forceReset = 0, isPspEmu = 0, isShell = 1;
int page = 0, aModePowSaveTry = 2, aModeLastFps = 50, maxFps = 0, fontH = 16, kfbH = 0, fps;
long curTime = 0, lateTime = 0, lateTimeAMode = 0, lateTimeAModeR = 0, lateTimeMsg = 0, fps_count = 0;
static char btn1[10], btn2[10], osdColorLbl[10];
int btn1Idx = 0, btn2Idx = 0, osdColorIdx = 0;
SceCtrlButtons validBtn[] = {
	SCE_CTRL_SELECT, SCE_CTRL_START, SCE_CTRL_UP, SCE_CTRL_DOWN, SCE_CTRL_LEFT, SCE_CTRL_RIGHT,
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_SQUARE, SCE_CTRL_TRIANGLE, SCE_CTRL_VOLUP,
	SCE_CTRL_VOLDOWN
};
uint32_t osdColorList[] = {
	COLOR_CYAN, COLOR_MAGENDA, COLOR_YELLOW, COLOR_WHITE, COLOR_BLUE, COLOR_GREEN,
	COLOR_BLACK
};

unsigned int *clock_r1, *clock_r2;
uint32_t *clock_speed;

static int profile_max_battery[] = {111, 111, 111, 111, 111};
static int profile_default[] = {266, 166, 166, 111, 166};
static int profile_game[] = {444, 222, 222, 166, 222};
static int profile_max_performance[] = {444, 222, 222, 166, 333};
static int profile_holy_shit_performance[] = {500, 222, 222, 166, 333};
static int *profiles[5] = {
	profile_max_battery, profile_default, profile_game,
	profile_max_performance, profile_holy_shit_performance
};

void getCustomButtonsLabel() {
	const char *validBtnL[CUSTOM_BTNS_NUM] = {
		"Select", "Start", "Up", "Down", "Left", "Right", "Cross",
		"Circle", "Square", "Triangle", "Vol Up", "Vol Down"
	};

	for (int i=0, t=0; i<CUSTOM_BTNS_NUM; ++i) {
		if (t == 2)
			break;

		if (current_config.btn1 == validBtn[i]) {
			snprintf(btn1, sizeof(btn1), "%s", validBtnL[i]);
			btn1Idx = i;
			++t;

		} else if (current_config.btn2 == validBtn[i]) {
			snprintf(btn2, sizeof(btn2), "%s", validBtnL[i]);
			btn2Idx = i;
			++t;
		}
	}

	return;
}

void getOSDColorLabel() {
	const char *colors[OSD_COLOR_NUM] = {
		"Cyano", "Magenta", "Yellow", "White", "Blue",
		"Green", "Black"
	};

	for (int i=0; i<OSD_COLOR_NUM; ++i) {
		if (current_config.osdColor == osdColorList[i]) {
			snprintf(osdColorLbl, sizeof(osdColorLbl), "%s", colors[i]);
			osdColorIdx = i;
			break;
		}
	}

	return;
}

int isValidCustomBtnCombo() {
	return (current_config.btn1 != current_config.btn2);
}

void reset_config() {
	memset(&current_config, 0, sizeof(current_config));
	current_config.mode = 2;
	current_config.osdColor = COLOR_CYAN;
	current_config.btn1 = SCE_CTRL_SELECT;
	current_config.btn2 = SCE_CTRL_UP;
}

int load_config() {
	snprintf(config_path, sizeof(config_path), CONFIG_PATH"%s.bin", titleid);

	if (!FileExists(config_path)) {
		snprintf(config_path, sizeof(config_path), CONFIG_PATH"default.bin");

		if (!FileExists(config_path))
			return NO_ERROR;
	}

	if (ReadFile(config_path, &current_config, sizeof(current_config)) < 0) {
		reset_config();
		return LOAD_ERROR;
	}

	return LOAD_GOOD;
}

int save_config(int saveDef) {
	if (saveDef)
		snprintf(config_path, sizeof(config_path), CONFIG_PATH"default.bin");
	else
		snprintf(config_path, sizeof(config_path), CONFIG_PATH"%s.bin", titleid);

	if (WriteFile(config_path, &current_config, sizeof(current_config)) < 0)
		return SAVE_ERROR;

	return SAVE_GOOD;
}

void refreshClocks() {
	isReseting = 1;
	kscePowerSetArmClockFrequency(profiles[current_config.mode][0]);
	kscePowerSetBusClockFrequency(profiles[current_config.mode][1]);
	kscePowerSetGpuEs4ClockFrequency(profiles[current_config.mode][2], profiles[current_config.mode][2]);
	kscePowerSetGpuXbarClockFrequency(profiles[current_config.mode][3]);
	kscePowerSetGpuClockFrequency(profiles[current_config.mode][4]);
	isReseting = 0;
}

void load_and_refresh() {
	error_code = load_config();

	refreshClocks();
}

// This function is from VitaJelly by DrakonPL and Rinne's framecounter
void doFps() {
	++fps_count;

	if ((curTime - lateTime) > TIMER_SECOND) {
		lateTime = curTime;
		fps = (int)fps_count;
		fps_count = 0;
	}

	return;
}

void adjustClock() {
	maxFps = (fps-1) > maxFps ? fps-1:maxFps;

	if (isShell) {
		if (current_config.mode != 2) {
			current_config.mode = 2;
			refreshClocks();
		}

		return;

	} else if ((curTime - lateTimeAMode) > aModeTimer) {
		int fpsDiff;

		lateTimeAMode = curTime;
		fpsDiff = abs(fps-aModeLastFps);
		aModeTimer = TIMER_AMODE;

		if ((fpsDiff > 6 || fps < maxFps) && current_config.mode < 4) {
			--aModePowSaveTry;
			++current_config.mode;
			refreshClocks();

		} else if (aModePowSaveTry && !fpsDiff && current_config.mode > 1) {
			aModeTimer = TIMER_SECOND*2;
			--current_config.mode;
			refreshClocks();
		}

		aModeLastFps = fps;
	}

	if ((curTime - lateTimeAModeR) > TIMER_AMODE_R) {
		lateTimeAModeR = curTime;
		aModePowSaveTry = 1;
		maxFps = 0;
	}

	return;
}

void drawErrors() {
	if (current_config.hideErrors || !error_code)
		return;

	const char *errors[5]={
		"", "There was a problem saving.", "Configuration saved.",
		"There was a problem loading.", "Configuration loaded."
	};

	blit_stringf(20, 0, "%s",  errors[error_code]);

	if ((curTime - lateTimeMsg) > 3000000) {
		error_code = 0;
		lateTimeMsg = curTime;
	}
}

int kscePowerSetClockFrequency_patched(tai_hook_ref_t ref_hook, int port, int freq) {
	int ret = 0;

	if (!isReseting)
		profile_default[port] = freq;

	if (port == 0 && freq == 500) {
		ret = TAI_CONTINUE(int, ref_hook, 444);
		ksceKernelDelayThread(10000);
		*clock_speed = profiles[current_config.mode][port];
		*clock_r1 = 0xF;
		*clock_r2 = 0x0;

		return ret;

	} else if (port == 2) {
		ret = TAI_CONTINUE(int, ref_hook, profiles[current_config.mode][port], profiles[current_config.mode][port]);

	} else {
		ret = TAI_CONTINUE(int, ref_hook, profiles[current_config.mode][port]);
	}

	return ret;
}

int checkButtons(int port, tai_hook_ref_t ref_hook, SceCtrlData *ctrl, int count) {
	int ret = 0;

	if (ref_hook == 0)
		return 1;

	ret = TAI_CONTINUE(int, ref_hook, port, ctrl, count);

	if (isPspEmu)
		return ret;

	if (!showMenu) {
		if ((ctrl->timeStamp > menu_timestamp + 400*1000) && (ctrl->buttons & current_config.btn1) && (ctrl->buttons & current_config.btn2)) {
			menu_timestamp = ctrl->timeStamp;
			ctrl_timestamp = showMenu = 1;
		}

		if (current_config.buttonSwap && (ctrl->buttons & 0x6000) && (ctrl->buttons & 0x6000) != 0x6000 &&
				((isShell && shell_pid == ksceKernelGetProcessId()) || (!isShell && current_pid == ksceKernelGetProcessId())))
			ctrl->buttons = ctrl->buttons ^ 0x6000;

	} else {
		unsigned buttons = ctrl->buttons;
		ctrl->buttons = 0;

		if ((ctrl->timeStamp > menu_timestamp + 400*1000) && (buttons & current_config.btn1) && (buttons & current_config.btn2)) {
			menu_timestamp = ctrl->timeStamp;
			error_code = showMenu = 0;
		}

		if (ctrl->timeStamp > ctrl_timestamp + 200*1000) {
			if (showMenu && ksceKernelGetProcessId() == shell_pid) {
				int menuOpt = (page*10)+pos; // page-pos

				if (buttons & SCE_CTRL_LEFT) {
					switch (menuOpt) {
						case 10: { // 1-0
							if (!current_config.autoMode && current_config.mode > 0) {
								--current_config.mode;
								refreshClocks();
							}
						}
							break;
						case 25: { // 2-5
							if ((osdColorIdx-1) >= 0) {
								--osdColorIdx;

								current_config.osdColor = osdColorList[osdColorIdx];
								getOSDColorLabel();
							}
						}
							break;
						case 31: { // 3-1
							if ((btn1Idx-1) >= 0) {
								--btn1Idx;

								current_config.btn1 = validBtn[btn1Idx];
								if (!isValidCustomBtnCombo()) {
									btn1Idx = (btn1Idx-1) >= 0 ? btn1Idx-1:btn1Idx+1;
									current_config.btn1 = validBtn[btn1Idx];
								}

								getCustomButtonsLabel();
							}
						}
							break;
						case 32: { // 3-2
							if ((btn2Idx-1) >= 0) {
								--btn2Idx;

								current_config.btn2 = validBtn[btn2Idx];
								if (!isValidCustomBtnCombo()) {
									btn2Idx = (btn2Idx-1) >= 0 ? btn2Idx-1:btn2Idx+1;
									current_config.btn2 = validBtn[btn2Idx];
								}

								getCustomButtonsLabel();
							}
						}
							break;
						default:
							break;
					}

					ctrl_timestamp = ctrl->timeStamp;

				} else if (buttons & SCE_CTRL_RIGHT) {
					switch (menuOpt) {
						case 10: { // 1-0
							if (!current_config.autoMode && current_config.mode < 4) {
								++current_config.mode;
								refreshClocks();
							}
						}
							break;
						case 25: { // 2-5
							if ((osdColorIdx+1) < OSD_COLOR_NUM) {
								++osdColorIdx;

								current_config.osdColor = osdColorList[osdColorIdx];
								getOSDColorLabel();
							}
						}
							break;
						case 31: { // 3-1
							if ((btn1Idx+1) < CUSTOM_BTNS_NUM) {
								++btn1Idx;

								current_config.btn1 = validBtn[btn1Idx];
								if (!isValidCustomBtnCombo()) {
									btn1Idx = (btn1Idx+1) < CUSTOM_BTNS_NUM ? btn1Idx+1:btn1Idx-1;
									current_config.btn1 = validBtn[btn1Idx];
								}

								getCustomButtonsLabel();
							}
						}
							break;
						case 32: { // 3-2
							if ((btn2Idx+1) < CUSTOM_BTNS_NUM) {
								++btn2Idx;

								current_config.btn2 = validBtn[btn2Idx];
								if (!isValidCustomBtnCombo()) {
									btn2Idx = (btn2Idx+1) < CUSTOM_BTNS_NUM ? btn2Idx+1:btn2Idx-1;
									current_config.btn2 = validBtn[btn2Idx];
								}

								getCustomButtonsLabel();
							}
						}
							break;
						default:
							break;
					}

					ctrl_timestamp = ctrl->timeStamp;

				} else if ((buttons & SCE_CTRL_UP) && pos > 0) {
					ctrl_timestamp = ctrl->timeStamp;
					--pos;

				} else if (buttons & SCE_CTRL_DOWN) {
					ctrl_timestamp = ctrl->timeStamp;
					++pos;

				} else if (buttons & SCE_CTRL_CIRCLE) {
					page = pos = 0;

				} else if (buttons & SCE_CTRL_CROSS) {
					switch (menuOpt) {
						case 0: { // 0-0
							error_code = save_config(0);
						}
							break;
						case 1: { // 0-1
							error_code = save_config(1);
						}
							break;
						case 2: { // 0-2
							reset_config();
							refreshClocks();
						}
							break;
						case 3: { // 0-3
							page = 1;
							pos = 0;
						}
							break;
						case 4: { // 0-4
							getOSDColorLabel();

							page = 2;
							pos = 0;
						}
							break;
						case 5: { // 0-5
							getCustomButtonsLabel();

							page = 3;
							pos = 0;
						}
							break;
						case 6: { // 0-6
							kscePowerRequestColdReset();
						}
							break;
						case 7: { // 0-7
							kscePowerRequestStandby();
						}
							break;
						case 11: { // 1-1
							current_config.autoMode = !current_config.autoMode;
							aModePowSaveTry = 2;
						}
							break;
						case 20: { // 2-0
							current_config.showFPS = !current_config.showFPS;
						}
							break;
						case 21: { // 2-1
							current_config.showBat = !current_config.showBat;
						}
							break;
						case 22: { // 2-2
							current_config.showCpu = !current_config.showCpu;
						}
							break;
						case 23: { // 2-3
							current_config.showGpu = !current_config.showGpu;
						}
							break;
						case 24: { // 2-4
							current_config.hideErrors = !current_config.hideErrors;
						}
							break;
						case 30: { // 3-0
							current_config.buttonSwap = !current_config.buttonSwap;
						}
							break;
						default:
							break;
					}

					ctrl_timestamp = ctrl->timeStamp;
				}
			}
		}
	}

	if (KERNEL_PID != ksceKernelGetProcessId() && shell_pid != ksceKernelGetProcessId()) {
		if (forceReset == 1) {
			if (current_pid == ksceKernelGetProcessId()) {
				if (ksceKernelGetProcessTitleId(current_pid, titleid, sizeof(titleid)) == 0 && titleid[0] != 0)
					forceReset = 2;

			} else {
				current_pid=ksceKernelGetProcessId();
			}
		}

	} else if (forceReset == 2) {
		isShell = 0;
		curTime = fps_count = lateTime = lateTimeAMode = 0;
		lateTimeAModeR = maxFps = lateTimeMsg = forceReset = 0;
		aModeLastFps = 50;
		aModePowSaveTry = 2;

		load_and_refresh();
	}

	return ret;
}

void drawMenu() {
	int entries = 0, labelEntries = 0, posY;

	blit_set_color(COLOR_WHITE, COLOR_BLUE);

	switch (page) {
		case 0:
			blit_stringf(LEFT_LABEL_X, 72, "LOLIcon by @dots_tb");
			blit_stringf(LEFT_LABEL_X, 72+fontH, "Version Y.y by kylon");
			MENU_OPTION_F(120, fontH, "Save for %s", titleid);
			MENU_OPTION(120, fontH, "Save as Default");
			MENU_OPTION(120, fontH, "Clear settings");
			MENU_OPTION(120, fontH, "Oclock Options");
			MENU_OPTION(120, fontH, "OSD Options");
			MENU_OPTION(120, fontH, "Ctrl Options");
			MENU_OPTION(120, fontH, "Restart vita");
			MENU_OPTION(120, fontH, "Shutdown vita");
			break;
		case 1:
			blit_stringf(LEFT_LABEL_X, 88, "CLOCK SETTINGS");

			blit_stringf(LEFT_LABEL_X, (posY = 120+fontH*labelEntries++), "CPU     ");
			blit_stringf(RIGHT_LABEL_X, posY, "%-4d  MHz - %d:%d", kscePowerGetArmClockFrequency(), *clock_r1, *clock_r2);
			blit_stringf(LEFT_LABEL_X, (posY = 120+fontH*labelEntries++), "BUS     ");
			blit_stringf(RIGHT_LABEL_X, posY, "%-4d  MHz", kscePowerGetBusClockFrequency());

			int r1, r2;
			kscePowerGetGpuEs4ClockFrequency(&r1, &r2);
			blit_stringf(LEFT_LABEL_X, (posY = 120+fontH*labelEntries++), "GPUes4  ");
			blit_stringf(RIGHT_LABEL_X, posY, "%-d   MHz", r1);

			blit_stringf(LEFT_LABEL_X, (posY = 120+fontH*labelEntries++), "XBAR    ");
			blit_stringf(RIGHT_LABEL_X, posY, "%-4d  MHz", kscePowerGetGpuXbarClockFrequency());
			blit_stringf(LEFT_LABEL_X, (posY = 120+fontH*labelEntries++), "GPU     ");
			blit_stringf(RIGHT_LABEL_X, posY, "%-4d  MHz", kscePowerGetGpuClockFrequency());

			posY += fontH*2;
			if (!current_config.autoMode) {
				switch (current_config.mode) {
					case 0:
						MENU_OPTION(posY, fontH, "Profile: Max Batt.");
						break;
					case 2:
						MENU_OPTION(posY, fontH, "Profile: Game Def.");
						break;
					case 3:
						MENU_OPTION(posY, fontH, "Profile: Max Perf.");
						break;
					case 4:
						MENU_OPTION(posY, fontH, "Profile: Holy Shit.");
						break;
					default: // 1
						MENU_OPTION(posY, fontH, "Profile: Default");
						break;
				}
			} else {
				MENU_OPTION(posY, fontH, "Profile: Auto");
			}

			MENU_OPTION_F(posY, fontH, "Auto Mode %d", current_config.autoMode);
			break;
		case 2:
			blit_stringf(LEFT_LABEL_X, 88, "OSD");
			MENU_OPTION_F(120, fontH, "Show FPS %d",current_config.showFPS);
			MENU_OPTION_F(120, fontH, "Show Battery %d",current_config.showBat);
			MENU_OPTION_F(120, fontH, "Show CPU Freq %d",current_config.showCpu);
			MENU_OPTION_F(120, fontH, "Show GPU Freq %d",current_config.showGpu);
			MENU_OPTION_F(120, fontH, "Hide Errors %d",current_config.hideErrors);
			MENU_OPTION_F(120, fontH, "Text Color %s",osdColorLbl);
			break;
		case 3:
			blit_stringf(LEFT_LABEL_X, 88, "CONTROL");
			MENU_OPTION_F(120, fontH, "Swap X/O Buttons %d", current_config.buttonSwap);
			MENU_OPTION_F(120, fontH, "Menu Button 1 %s", btn1);
			MENU_OPTION_F(120, fontH, "Menu Button 2 %s", btn2);
			break;
		default:
			break;
	}

	if (pos >= entries)
		pos = entries -1;
}

int getFindModNameFromPID(int pid, char *mod_name, int size) {
	SceKernelModuleInfo sceinfo;
	sceinfo.size = sizeof(sceinfo);
	int ret;
	size_t count;
	SceUID modids[128];

	if ((ret = ksceKernelGetModuleList(pid, 0xff, 1, modids, &count)) == 0) {
		for (int i=0; i<count; ++i) {
			ret = ksceKernelGetModuleInfo(pid, modids[count - 1], &sceinfo);

			if (ret == 0 && strncmp(mod_name, sceinfo.module_name, size) == 0)
				return 1;
		}

		return 0;
	}

	return ret;
}

int _sceDisplaySetFrameBufInternalForDriver(int fb_id1, int fb_id2, const SceDisplayFrameBuf *pParam, int sync) {
	int textX = 10;

	if (isPspEmu || !fb_id1 || !pParam)
		return TAI_CONTINUE(int, ref_hooks[0], fb_id1, fb_id2, pParam, sync);

	if (!shell_pid && fb_id2) { // 3.68 fix
		if (ksceKernelGetProcessTitleId(ksceKernelGetProcessId(), titleid, sizeof(titleid)) == 0 && titleid[0] != 0) {
			if (strncmp("main", titleid, sizeof(titleid)) == 0) {
				shell_pid = ksceKernelGetProcessId();
				load_and_refresh();
			}
		}
	}

	SceDisplayFrameBuf kfb;
	memset(&kfb, 0, sizeof(kfb));
	memcpy(&kfb, pParam, sizeof(SceDisplayFrameBuf));
	blit_set_frame_buf(&kfb);

	if (kfb.height != kfbH) {
		kfbH = kfb.height;
		fontH = kfbH < 544 ? 21:16;
	}

	if (showMenu)
		drawMenu();

	blit_set_color(current_config.osdColor, COLOR_TRANSPARENT);
	if ((isShell && shell_pid == ksceKernelGetProcessId()) || (!isShell && current_pid == ksceKernelGetProcessId())) {
		curTime = ksceKernelGetProcessTimeWideCore();

		drawErrors();

		if (current_config.showFPS || current_config.autoMode)
			doFps();

		if (current_config.showFPS) {
			blit_stringf(10, 520, "FPS:%d", fps);
			textX += 105;
		}

		if (current_config.autoMode)
			adjustClock();

		if (current_config.showBat) {
			blit_stringf(textX, 520, "Batt:%02d\%", kscePowerGetBatteryLifePercent());
			textX += 140;
		}

		if (current_config.showCpu) {
			blit_stringf(textX, 520, "CPU:%-4d", kscePowerGetArmClockFrequency());
			textX += 120;
		}

		if (current_config.showGpu)
			blit_stringf(textX, 520, "GPU:%-4d", kscePowerGetGpuClockFrequency());
	}

	return TAI_CONTINUE(int, ref_hooks[0], fb_id1, fb_id2, pParam, sync);
}

static int power_patched1(int freq) { return kscePowerSetClockFrequency_patched(ref_hooks[1], 0, freq); }
static int power_patched2(int freq) { return kscePowerSetClockFrequency_patched(ref_hooks[2], 1, freq); }
static int power_patched3(int freq) { return kscePowerSetClockFrequency_patched(ref_hooks[3], 2, freq); }
static int power_patched4(int freq) { return kscePowerSetClockFrequency_patched(ref_hooks[4], 3, freq); }

static int keys_patched1(int port, SceCtrlData *ctrl, int count) {
	int ret, state;

	if (isPspEmu) {
		ENTER_SYSCALL(state);
		ret = TAI_CONTINUE(int, ref_hooks[5], port, ctrl, count);
		EXIT_SYSCALL(state);

	} else {
		ret = checkButtons(port, ref_hooks[5], ctrl, count);
	}

	return ret;
}

static int keys_patched2(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[6], ctrl, count);
}

static int keys_patched3(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[7], ctrl, count);
}

static int keys_patched4(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[8], ctrl, count);
}

static int keys_patched5(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[9], ctrl, count);
}

static int keys_patched6(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[10], ctrl, count);
}

static int keys_patched7(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[11], ctrl, count);
}

static int keys_patched8(int port, SceCtrlData *ctrl, int count) {
	return checkButtons(port, ref_hooks[12], ctrl, count);
}

int SceProcEventForDriver_414CC813(int pid, int id, int r3, int r4, int r5, int r6) {
	SceKernelProcessInfo info;
	info.size = 0xE8;

	if (strncmp("main", titleid, sizeof(titleid)) == 0) {
		switch (id) {
			case 0x1: // startup
				if (!shell_pid && ksceKernelGetProcessInfo(pid, &info) == 0 && info.ppid == KERNEL_PID) {
					shell_pid = pid;
					strncpy(titleid, "main", sizeof("main"));
					load_and_refresh();
					break;
				}
			case 0x5:
				isPspEmu = getFindModNameFromPID(pid, "adrenaline", sizeof("adrenaline")) ||
							getFindModNameFromPID(pid, "ScePspemu", sizeof("ScePspemu"));
				current_pid = pid;

				if (!isPspEmu) {
					forceReset = 1;

				} else {
					ksceKernelGetProcessTitleId(pid, titleid, sizeof(titleid));
					showMenu = isShell = 0;
					current_config.autoMode = 0;
					current_config.mode = 1;
					refreshClocks();
				}
				break;
			default:
				break;
		}

	} else if ((id == 0x4 || id == 0x3) && (current_pid == pid || isPspEmu)) {
		curTime = fps_count = lateTime = lateTimeAMode = 0;
		lateTimeAModeR = maxFps = lateTimeMsg = isPspEmu = 0;
		aModeLastFps = 50;
		aModePowSaveTry = 2;
		isShell = 1;

		strncpy(titleid, "main", sizeof("main"));
		load_and_refresh();
	}

	return TAI_CONTINUE(int, ref_hooks[13], pid, id, r3, r4, r5, r6);
}

/*
	Hooks

	[1] scePowerSetArmClockFrequency
	[2] scePowerSetBusClockFrequency
	[3] scePowerSetGpuClockFrequency
	[4] scePowerSetGpuXbarClockFrequency
	[5] sceCtrlPeekBufferPositive
	[6] sceCtrlPeekBufferPositive2
	[7] sceCtrlReadBufferPositive
	[8] sceCtrlReadBufferPositiveExt2
	[9] sceCtrlPeekBufferPositiveExt2
	[10] sceCtrlPeekBufferPositiveExt
	[11] sceCtrlReadBufferPositive2
	[12] sceCtrlReadBufferPositiveExt
*/
void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	ksceIoMkdir(CONFIG_PATH,6);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x475BCC82, &_kscePowerGetGpuEs4ClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x264C24FC, &_kscePowerSetGpuEs4ClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x64641E6A, &_kscePowerGetGpuClockFrequency);
	module_get_export_func(KERNEL_PID, "ScePower", 0x1590166F, 0x621BD8FD , &_kscePowerSetGpuClockFrequency);

	tai_module_info_t tai_info;

	tai_info.size = sizeof(tai_module_info_t);

	clock_r1 = (unsigned int *)pa2va(0xE3103000);
	clock_r2 = (unsigned int *)pa2va(0xE3103004);

	taiGetModuleInfoForKernel(KERNEL_PID, "ScePower", &tai_info);
	module_get_offset(KERNEL_PID, tai_info.modid, 1, 0x4124 + 0xA4, (uintptr_t)&clock_speed);

	memset(&titleid, 0, sizeof(titleid));
	strncpy(titleid, "main", sizeof(titleid));

	memset(&btn1, 0, sizeof(btn1));
	memset(&btn2, 0, sizeof(btn2));

	reset_config();
	refreshClocks();

	if (module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0xD269F915, &_ksceKernelGetModuleInfo))
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xDAA90093, &_ksceKernelGetModuleInfo);

	if (module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x97CF7B4E, &_ksceKernelGetModuleList))
		module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xB72C75A4, &_ksceKernelGetModuleList);

	if (module_get_export_func(KERNEL_PID, "SceProcessmgr", 0x7A69DE86, 0x4CA7DC42, &_ksceKernelExitProcess))
		module_get_export_func(KERNEL_PID, "SceProcessmgr", 0xEB1F8EF7, 0x905621F9, &_ksceKernelExitProcess);

	g_hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceDisplay", 0x9FED47AC, 0x16466675, _sceDisplaySetFrameBufInternalForDriver);
	g_hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[1], "ScePower", 0x1590166F, 0x74DB5AE5, power_patched1);
	g_hooks[2] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[2], "ScePower", 0x1590166F, 0xB8D7B3FB, power_patched2);
	g_hooks[3] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[3], "ScePower", 0x1590166F, 0x264C24FC, power_patched3);
	g_hooks[4] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[4], "ScePower", 0x1590166F, 0xA7739DBE, power_patched4);

	taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &tai_info);

	g_hooks[5] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[5], "SceCtrl", TAI_ANY_LIBRARY, 0xEA1D3A34, keys_patched1);
	g_hooks[6] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[6], tai_info.modid, 0, 0x3EF8, 1, keys_patched2);
	g_hooks[7] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[7], "SceCtrl", TAI_ANY_LIBRARY, 0x9B96A1AA, keys_patched3);
	g_hooks[8] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[8], tai_info.modid, 0, 0x4E14, 1, keys_patched4);
	g_hooks[9] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[9], tai_info.modid, 0, 0x4B48, 1, keys_patched5);
	g_hooks[10] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[10], tai_info.modid, 0, 0x3928, 1, keys_patched6);
    g_hooks[11] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[11], tai_info.modid, 0, 0x449C, 1, keys_patched7);
    g_hooks[12] = taiHookFunctionOffsetForKernel(KERNEL_PID, &ref_hooks[12], tai_info.modid, 0, 0x3BCC, 1, keys_patched8);
	g_hooks[13] = taiHookFunctionImportForKernel(KERNEL_PID, &ref_hooks[13], "SceProcessmgr", TAI_ANY_LIBRARY, 0x414CC813, SceProcEventForDriver_414CC813);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	for (int i=0; i<HOOKS_NUM; ++i) {
		if (g_hooks[i] >= 0)
			taiHookReleaseForKernel(g_hooks[i], ref_hooks[i]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}
