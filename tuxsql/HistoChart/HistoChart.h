//*****************************************
//*	HistoChart								*
//*	Copyright  2004  Leicht			*
//*	mail : leicht@tuxplc.net 			*
//*	web : http://tuxplc.net				*
//*													*
//*	Version 0.99 : 25 may 2004	*
//*****************************************

#ifndef _HISTOCHART_H
#define _HISTOCHART_H

#include "cgic.h"
#include "gd.h"
#include "gdfonts.h"
#include <syslog.h>
#include <sutil/MySQL.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

//const
#define ERROR_MSG_LEN 512
#define PENNUMBER 8
#define PROG_NAME "TUXHISTO"
#define YOFFSET 20
#define XOFFSET 40
#define TITLE_BOTTOM 40

//Error
#define SUCCES 0
#define ERROR -1
#define EMPTYRESULT 1

typedef struct {
	char TagName[30];
	int ID;
	double Limit;
	double I_MIN;
	double I_MAX;
	double O_MIN;
	double O_MAX;
	int penColor;
	char UNIT[20];
	int isDigital;
} TPen;

/* Define Debug for a console appli, else Daemon*/
//#define DEBUG

#ifdef DEBUG
	#define MyLog printf
#else
	#define MyLog(m) syslog(LOG_NOTICE,m) 
#endif

//Variables
extern gdImagePtr im;
extern int BackgroundColor;
extern int black, red, green, blue, white,	magenta, cyan,	yellow, orange, lightgray, midgray,	darkgray, midgreen,	darkmagenta, darkcyan;
extern int styleDotted[4], styleDashed[6];
extern TPen pen[8];
extern TPen onePen;
extern int TagNumber;
extern int timerange;
extern int lag;
extern int height,width;
extern int Legend_Right;
extern time_t curtime, timeMin, timeMax;
extern int hChart, wChart;

extern MYSQL Default_Db;
extern unsigned int MysqlError;
extern char MysqlErrorMsg[ERROR_MSG_LEN];
extern MYSQL_RES *SqlResult;

#endif /* _HISTOCHART_H */
