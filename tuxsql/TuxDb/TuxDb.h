/***************************************************************************
*   Copyright (C) 2010 by TuxPLC                                          *
*   Author Stephane JEANNE                           *
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

#ifndef _TUXDB_H
#define _TUXDB_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAXPATHSIZE 100
#define MAXALIASSIZE 50
#define MAXSTRSIZE 128

typedef struct _MACHINE{
								char MachineId[MAXALIASSIZE+1];
								char Ratio_Addr[MAXALIASSIZE+1];
								char P_Bob_Addr[MAXALIASSIZE+1];
								char A_Bob_Addr[MAXALIASSIZE+1];
								char Temp_Addr[MAXALIASSIZE+1];	
								float Ratio;		
								int Prev_Bob;
								int Act_Bob;
								int Bobine;
								float Temp;
							} MACHINE;

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

#endif /* _TUXDB_H */
