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

#ifndef _TUXMXS_H
#define _TUXMXS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAXPATHSIZE 100
#define MAXALIASSIZE 50
#define MAXSTRSIZE 128

typedef struct _TAGSimplex{
								char TagName[30];
								int Address;
								int Bobine;
							} TAGSimplex;

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

#endif /* _TUXMXS_H */
