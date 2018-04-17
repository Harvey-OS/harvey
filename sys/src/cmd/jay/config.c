#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <jay.h>
#include "dat.h"
#include "fns.h"

void
initdefaultconfig(){
	jayconfig = malloc(sizeof(Jayconfig));
	jayconfig->taskPanelColor = 0x8C7160FF;

	jayconfig->mainMenuColor = 0x363533FF;
	jayconfig->mainMenuHooverColor = 0x36A4BFFF;

	jayconfig->windowTitleColor = 0x4D4D4DFF;
	jayconfig->windowTitleFontColor = DWhite;
	jayconfig->windowInTopBorder = 0xC4CAC8FF;
	jayconfig->windowInBottomBorder = DPalegreygreen;
	jayconfig->windowSelectedColor = 0xCCCCCCFF;
	jayconfig->windowScrollBarFrontColor = DWhite;//0x999999FF;
	jayconfig->windowTextCursorColor = DWhite;
	jayconfig->windowBackgroundColor = DBlack;
	jayconfig->windowBackTextColor = 0x666666FF;
	jayconfig->windowFrontTextColor = DWhite;

	jayconfig->backgroundColor = DBlack;
	jayconfig->backgroundimgpath = "/usr/harvey/lib/background.img";

	jayconfig->menuBackColor = DPalegreyblue;
	jayconfig->menuHighColor = DGreyblue;
	jayconfig->menuBorderColor = jayconfig->menuHighColor;
	jayconfig->menuSelTextColor = DWhite;
	jayconfig->menuTextColor = DWhite;

	jayconfig->mainBackColor = DWhite;
	jayconfig->mainTextColor = DBlack;
}

char *
getjayconfig(){
  char s[1024];
  sprint(s,"taskPanelColor=0x%08X\n", jayconfig->taskPanelColor);
  sprint(s, "%smainMenuColor=0x%08X\n", s, jayconfig->mainMenuColor);
  sprint(s, "%smainMenuHooverColor=0x%08X\n", s, jayconfig->mainMenuHooverColor);
  sprint(s, "%swindowTitleColor=0x%08X\n", s, jayconfig->windowTitleColor);
  sprint(s, "%swindowTitleFontColor=0x%08X\n", s, jayconfig->windowTitleFontColor);
  sprint(s, "%swindowInTopBorder=0x%08X\n", s, jayconfig->windowInTopBorder);
  sprint(s, "%swindowInBottomBorder=0x%08X\n", s, jayconfig->windowInBottomBorder);
  sprint(s, "%swindowSelectedColor=0x%08X\n", s, jayconfig->windowSelectedColor);
  sprint(s, "%swindowScrollBarFrontColor=0x%08X\n", s, jayconfig->windowScrollBarFrontColor);
  sprint(s, "%swindowTextCursorColor=0x%08X\n", s, jayconfig->windowTextCursorColor);
  sprint(s, "%swindowBackgroundColor=0x%08X\n", s, jayconfig->windowBackgroundColor);
  sprint(s, "%swindowBackTextColor=0x%08X\n", s, jayconfig->windowBackTextColor);
  sprint(s, "%swindowFrontTextColor=0x%08X\n", s, jayconfig->windowFrontTextColor);
  sprint(s, "%sbackgroundColor=0x%08X\n", s, jayconfig->backgroundColor);
	sprint(s, "%sbackgroundimgpath=%s\n)", s, jayconfig->backgroundimgpath);
	sprint(s, "%smenuBackColor=0x%08X\n", s, jayconfig->menuBackColor);
	sprint(s, "%smenuHighColor=0x%08X\n", s, jayconfig->menuHighColor);
	sprint(s, "%smenuBorderColor=0x%08X\n", s, jayconfig->menuBorderColor);
	sprint(s, "%smenuSelTextColor=0x%08X\n", s, jayconfig->menuSelTextColor);
	sprint(s, "%smenuTextColor=0x%08X\n", s, jayconfig->menuTextColor);

  return estrdup(s);
}

static uint32_t
getuint32property(char *p){
  char *s, aux[11];
  s = p;
  while(*s == ' ' && *s != '\0'){
    s++;
  }
  if (*s == '\0')
    return 0;
  strncpy(aux, s, 10);
  aux[10] = '\0';
  return strtoul(aux, nil, 0);
}

static char *
getstringproperty(char *p){
	char *s, aux[1024];
	s = p;
	while(*s == ' ' && *s != '\0'){
    s++;
  }
	if (*s == '\0')
    return nil;
	strncpy(aux, s, s - p - 1);
	aux[s-p]='\0';
	return strdup(aux);
}

void
setconfigproperty(char *p) {
  char *s, *e, aux[30];
  int size;
  s = p;
  while(*s == ' '&& *s != '\0'){
    s++;
  }
  e = s + 1;
  while(*e != '=' && *e != ' ' && *e != '\0'){
    e++;
  }
  size = e - s;
  strncpy(aux, s, size);
  aux[size] = '\0';
  e++;

  if (strcmp("taskPanelColor", aux)==0){
    jayconfig->taskPanelColor = getuint32property(e);
  } else if (strcmp("mainMenuColor", aux)==0) {
    jayconfig->mainMenuColor = getuint32property(e);
  } else if (strcmp("mainMenuHooverColor", aux)==0) {
    jayconfig->mainMenuHooverColor = getuint32property(e);
  } else if (strcmp("windowTitleColor", aux)==0) {
    jayconfig->windowTitleColor = getuint32property(e);
  } else if (strcmp("windowTitleFontColor", aux)==0) {
    jayconfig->windowTitleFontColor = getuint32property(e);
  } else if (strcmp("windowInTopBorder", aux)==0) {
    jayconfig->windowInTopBorder = getuint32property(e);
  } else if (strcmp("windowInBottomBorder", aux)==0) {
    jayconfig->windowInBottomBorder = getuint32property(e);
  } else if (strcmp("windowSelectedColor", aux)==0) {
    jayconfig->windowSelectedColor = getuint32property(e);
  } else if (strcmp("windowScrollBarFrontColor", aux)==0) {
    jayconfig->windowScrollBarFrontColor = getuint32property(e);
  } else if (strcmp("windowTextCursorColor", aux)==0) {
    jayconfig->windowTextCursorColor = getuint32property(e);
  } else if (strcmp("windowBackgroundColor", aux)==0) {
    jayconfig->windowBackgroundColor = getuint32property(e);
  } else if (strcmp("windowBackTextColor", aux)==0) {
    jayconfig->windowBackTextColor = getuint32property(e);
  } else if (strcmp("windowFrontTextColor", aux)==0) {
    jayconfig->windowFrontTextColor = getuint32property(e);
  } else if (strcmp("backgroundColor", aux)==0) {
    jayconfig->backgroundColor = getuint32property(e);
  } else if (strcmp("backgroundimgpath", aux)==0) {
		jayconfig->backgroundimgpath = getstringproperty(e);
	} else if (strcmp("menuBackColor", aux)==0) {
    jayconfig->menuBackColor = getuint32property(e);
  } else if (strcmp("menuHighColor", aux)==0) {
    jayconfig->menuHighColor = getuint32property(e);
  } else if (strcmp("menuBorderColor", aux)==0) {
    jayconfig->menuBorderColor = getuint32property(e);
  } else if (strcmp("menuSelTextColor", aux)==0) {
    jayconfig->menuSelTextColor = getuint32property(e);
  } else if (strcmp("menuTextColor", aux)==0) {
    jayconfig->menuTextColor = getuint32property(e);
  }
}

void
setjayconfig(char *conf) {
  char *s, *e, *aux;
  int size;
  for(s = conf; *s != '\0'; s++){
    for(e = s; *e != '\n' && *e != '\0'; e++){}
    size = e - s;
    aux = emalloc(size);
    strncpy(aux, s, size);
    *(aux + size - 1) = '\0';
    setconfigproperty(aux);
    free(aux);
    s = e;
  }
}
