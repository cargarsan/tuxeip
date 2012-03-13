/***************************************************************************
*   Copyright (C) 2004 by TuxPLC                                          *
*   Author Stephane JEANNE s.jeanne@tuxplc.net                            *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef _TUXMX_H
#define _TUXMX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <odx/idsmsg.h>

#define MAXPATHSIZE 100
#define MAXALIASSIZE 50
#define MAXSTRSIZE 128

typedef struct _GROUP{
								char GroupName[30];
								short int TypeEchange;
								short int TempsCycle;
								short int TransitionMx;
								char Ordinal[255];
								time_t LastUpdate;
								int Varcount;
								int MsgId;
								t_f_symname *TagNames;
								t_f_symname *Symbols;
								t_read_status_info *ReadInfo;
								void *Data;
							} GROUP;

typedef struct _TAG{
								char TagName[MAXALIASSIZE+1];
								char Value_Address[MAXALIASSIZE+1];
								char Reset_Address[MAXALIASSIZE+1];
								float Value;			
							} TAG;

typedef struct _PLC
	{	char PlcName[MAXALIASSIZE];
		char PlcPath[MAXPATHSIZE];
		Plc_Type PlcType;
		int NetWork;
		int Node;
	} PLC;

#ifdef __cplusplus
}
#endif

#endif /* _TUXMX_H */
