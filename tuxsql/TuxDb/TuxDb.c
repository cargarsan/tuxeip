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


#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>

#define MAX_LISTE_SIZE 50
#include <sutil/MySQL.h>
#include <sutil/List.h>
#include <sutil/iniparser.h>
#include <tuxeip/TuxEip.h>
#include "TuxDb.h"

#define space printf("\n");
#define UPDATERATE 60
#define TO_RESTART 3
#define WAITTORECONNECT 10

#define MAXERRORTAGPC 50


/****************** Struct definition ***************************/



/********************* Function declaration *********************/
void SigHand(int num);
void Log(int level,char *format,...);

int GetConfig(char * ini_name);

PLC *GetPlc(char *str);
int GetPlcType(char *Type);
int GetNetType(char *NetWork);
int GetSerial(void);
int GetTns(void);
int ParsePath(char *strpath,char Ip[],char Path[]);
int writeplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,void *value);
int readplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,double *value);
int readwritedata(PLC *plc);

// int UpdateTag(char *tag,char *value);
int UpdateBobine(MACHINE *machine);

int mainprog(void);

/********************** Global Vars ******************************/

int DEAMON=0;
int TEST=0;
int debuglevel=LOG_WARNING;

int Terminated=0;
int starttime=0;
int UpdateRate=30;

char Config[MAXSTRSIZE]={"/etc/tuxdb.conf\0"};
char DBHOST[20]={"localhost\0"};
char DB[20]={"histosql\0"};
char USER[20]={"histosql\0"};
char PASS[20]={"histosql\0"};
char LOG_TITLE[20]={"TUXDb\0"};

int AB_VARCOUNT;
int tns=0;
int Serial=0;
char *PidFile=NULL;
char *CLPath=NULL;
PLC *plc=NULL;

LISTE machines;

/********************** Functions Def ****************************/
void SigHand(int num)
{
	switch (num)
	{
		case SIGTERM:	Log(LOG_NOTICE,"receive SIGTERM\n");
									Terminated=1;
									break;
		case SIGINT:	Log(LOG_NOTICE,"receive SIGINT\n");
									Terminated=1;
									break;
		case SIGIO:	Log(LOG_INFO,"receive SIGIO (%d / %d)\n");
									//Terminated=1;
									break;
		case SIGSEGV:	Log(LOG_CRIT,"!!!!!!!!!!!! receive SIGSEGV, program ERROR!!!!!!!!!!!!!!!!!!!\n");
									//longjmp(jb,-1);
									Terminated=1;
									exit(1);
									break;
		case SIGUSR1:	Log(LOG_INFO,"receive SIGUSR1 (%d / %d)\n");
									//Restart=1;
									break;
		default:	Log(LOG_CRIT,"receive signal: %d (%d / %d)\n");
									Terminated=1;
									break;
	}
}
void Log(int level,char *format,...)
{	va_list list;
	va_start(list,format);//NULL
	if (level<=debuglevel)
	{
		if (!DEAMON)
		{	char str[500];
			if (format!="\n")
			{
				vsnprintf(str,sizeof(str),format,list);
				printf("%ld : %s",time(NULL)-starttime,str);
			} else vprintf(format,list);
		}
		else
		{
			vsyslog(level,format,list);
		}
	}
	va_end(list);
}
int GetConfig(char * ini_name)
{
	int i,sections_n;
	char keyname[MAXSTRSIZE+1];
	int debug;
	dictionary	*	ini;
	ini = iniparser_new(ini_name);
	if (ini==NULL)
	{
		Log(LOG_ERR, "cannot parse file [%s]\n", ini_name);
		return(1) ;
	}
	// Main Section
	DEAMON=iniparser_getboolean(ini, "Main:Daemon", DEAMON);
	debug=iniparser_getint(ini, "Main:Log", debuglevel);
	switch (debug)
	{
		case 3:debuglevel=LOG_DEBUG;break;
		case 2:debuglevel=LOG_INFO;break;
		case 1:debuglevel=LOG_WARNING;break;
		case 0:debuglevel=LOG_ERR;break;
		default:debuglevel=LOG_WARNING;break;
	}
	UpdateRate=iniparser_getint(ini, "Main:UpdateRate", UpdateRate);
	if (UpdateRate<10) {UpdateRate=10;}

	// Db section 
	strncpy(DBHOST,iniparser_getstring(ini,"Db:DbHost" ,""),sizeof(DBHOST));
	strncpy(DB,iniparser_getstring(ini,"Db:DbName" ,""),sizeof(DB));
	strncpy(USER,iniparser_getstring(ini,"Db:DbUserName" ,""),sizeof(USER));
	strncpy(PASS,iniparser_getstring(ini,"Db:DbPassword" ,""),sizeof(PASS));

	// PLC Section 
	CLPath=strdup(iniparser_getstring(ini,"PLC:CLPath" ,NULL));
	plc=GetPlc(CLPath);

	// Variables Section
	AB_VARCOUNT=0;
	sections_n=iniparser_getnsec(ini);
	for(i=0;i<sections_n;i++)
	{
		char *section=iniparser_getsecname(ini,i);
		if (strncasecmp("Machine",section,7)==0)
		{
			MACHINE *var=malloc(sizeof(MACHINE));
			AddListe(&machines,var);
			memset(var,0,sizeof(MACHINE));
			AB_VARCOUNT++;

			memset(keyname,0,MAXSTRSIZE);snprintf(keyname,MAXSTRSIZE,"%s:%s",section,"Machine_Id");
			strncpy((char *)(var->MachineId),iniparser_getstring(ini,keyname,""),MAXALIASSIZE);	

			memset(keyname,0,MAXSTRSIZE);snprintf(keyname,MAXSTRSIZE,"%s:%s",section,"NoBobine_P");
			strncpy((char *)(var->P_Bob_Addr),iniparser_getstring(ini,keyname,""),MAXALIASSIZE);			

			memset(keyname,0,MAXSTRSIZE);snprintf(keyname,MAXSTRSIZE,"%s:%s",section,"NoBobine_A");
			strncpy((char *)(var->A_Bob_Addr),iniparser_getstring(ini,keyname,""),MAXALIASSIZE);

			memset(keyname,0,MAXSTRSIZE);snprintf(keyname,MAXSTRSIZE,"%s:%s",section,"Ratio_Vap");
			strncpy((char *)(var->Ratio_Addr),iniparser_getstring(ini,keyname,""),MAXALIASSIZE);

			memset(keyname,0,MAXSTRSIZE);snprintf(keyname,MAXSTRSIZE,"%s:%s",section,"Temp_Ext");
			strncpy((char *)(var->Temp_Addr),iniparser_getstring(ini,keyname,""),MAXALIASSIZE);
		}
//		free(section);
	}
	iniparser_free(ini);
	return(0);
}
PLC *GetPlc(char *str)
{	char Path[MAXPATHSIZE];
	char Type[MAXALIASSIZE];
	char NetWork[MAXALIASSIZE];
	int Node=0;

	if (sscanf(str,"%50s %10s %10s %d",Path,Type,NetWork,&Node)==4)
	{
		PLC  *plc=malloc(sizeof(PLC));
		if (plc!=NULL)
		{
			Log(LOG_DEBUG,"Writing to %s (%s,%s,%d)\n",Path,Type,NetWork,Node);
			bzero(plc,sizeof(PLC));
			strncpy(plc->PlcPath,Path,sizeof(plc->PlcPath));
			plc->PlcType=GetPlcType(Type);
			plc->NetWork=GetNetType(NetWork);
			plc->Node=Node;
		}
		return(plc);
	} else return(NULL);
}
int GetPlcType(char *Type)
{
	if (!strncasecmp(Type,"PLC",strlen(Type))) return(PLC5);
		else
		{
			if (!strncasecmp(Type,"SLC",strlen(Type))) return(SLC500);
			else
			{
				if (!strncasecmp(Type,"LGX",strlen(Type))) return(LGX);
				else return(Unknow);
			}
		}
}
int GetNetType(char *NetWork)
{
	if (!strncasecmp(NetWork,"DHP",strlen(NetWork))) return(1);
		else
		{
			if (!strncasecmp(NetWork,"DHP_A",strlen(NetWork))) return(1);
			else
			{
				if (!strncasecmp(NetWork,"DHP_B",strlen(NetWork))) return(2);
				else return(0);
			}
		}
}
int GetSerial(void)
{
	return(getpid()+Serial++);
}
int GetTns(void)
{
	return(tns++);
}
int ParsePath(char *strpath,char Ip[],char Path[])
{ int index=0,len=0;
	len=strlen(strpath);
	char *str=malloc(len);
	strcpy(str,strpath);
	char *pos=strtok(str,",");
	if (pos!=NULL)
	{
		strcpy(Ip,pos);
		while ((pos=strtok(NULL,","))!=NULL)
		{
			Path[index]=strtol(pos,(char**)NULL,10);
			if (errno!=ERANGE) index++;
		};
		free(str);
		return(index);
	} else return(0);
}
int writeplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,void *value)
{
	DHP_Header temp={0,0,0,0};
	DHP_Header *dhp=&temp;
	int res=1;
	switch (plc->PlcType)
	{
		case PLC5:
		case SLC500:
			{
				dhp->Dest_adress=plc->Node;
				if (plc->NetWork) // DHP
					res=WritePLCData(session,connection,dhp,NULL,0,plc->PlcType,GetTns(),address,PLCDataType(type),value,1);
				else res=WritePLCData(session,connection,NULL,NULL,0,plc->PlcType,GetTns(),address,PLCDataType(type),value,1);
				if (res>0)	Log(LOG_DEBUG,"WritePlc : %s : %s\n",address,cip_err_msg);
				else Log(LOG_WARNING,"Error while writing value on tag %s: (%d) %s\n",address,cip_errno,cip_err_msg);
			}; break;
		case LGX:
		{
			res=WriteLgxData(session,connection,address,LGXDataType(type),value,1);
			if (res>0)	Log(LOG_DEBUG,"WritePlc : %s : %s\n",address,cip_err_msg);
				else Log(LOG_WARNING,"Error while writing value on tag %s: (%d) %s\n",address,cip_errno,cip_err_msg);
		}; break;
		default:Log(LOG_WARNING,"Plc type unknow \n");
			break;
	}
	return(cip_errno);
}
int readplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,double *value)
{	DHP_Header temp={0,0,0,0};
	DHP_Header *dhp=&temp;
	//PCCC_Header *head=NULL;
	int tns=getpid();

	switch (plc->PlcType)
	{
		case PLC5:
		case SLC500:
			{
				PLC_Read *data=NULL;
				dhp->Dest_adress=plc->Node;
				if (plc->NetWork) // DHP
					data=ReadPLCData(session,connection,dhp,NULL,0,plc->PlcType,tns++,address,1);
				else data=ReadPLCData(session,connection,NULL,NULL,0,plc->PlcType,tns++,address,1);
				if (data!=NULL)
				{
					if (!cip_errno)
					{
						*value=PCCC_GetValueAsFloat(data,0);
						Log(LOG_DEBUG,"ReadPlc : %s = %f (%s)\n",address,*value,cip_err_msg);
					}	else
					{
						Log(LOG_WARNING,"Get PCCC value on tag %s : (%d / ext %d) %s\n",address,cip_errno,cip_err_msg,cip_ext_errno);
					}
					free(data);
				} else
				{
					Log(LOG_WARNING,"Error while decoding PCCC value on tag %s : (%d) %s\n",address,cip_errno,cip_err_msg);
				}
			}; break;
		case LGX:
		{
			LGX_Read *data=ReadLgxData(session,connection,address,1);
			if (data!=NULL)
			{
				if (!cip_errno)
				{
					*value=GetLGXValueAsFloat(data,0);
					//Log(LOG_DEBUG,"ReadPlc : %s = %f (%s)\n",address,*value,cip_err_msg);
				}	else
				{
					Log(LOG_WARNING,"Get value : (%d) %s\n",cip_errno,cip_err_msg);
				}
				free(data);
			} else
			{
				Log(LOG_WARNING,"ReadLgxData error on tag : %s (%d : %s)\n",address,cip_errno,cip_err_msg);
			}
		}; break;
		default:Log(LOG_WARNING,"Plc type unknow \n");
			break;
	}
	return(cip_errno);
}
int readwritedata(PLC *plc)
{	int result=0;
	int res,i,path_size;
	char ip[16],path[40];
	Eip_Session *session=NULL;
	Eip_Connection *connection=NULL;

	path_size=ParsePath(plc->PlcPath,ip,path);
	if (path_size<=0) return(1);

	session=OpenSession(ip);

	if (session!=NULL)
	{
		session->Sender_ContextL=getpid();
		if (RegisterSession(session)>=0)
		{
			if (plc->NetWork)
					connection=_ConnectPLCOverDHP(session,
					plc->PlcType,
					_Priority,_TimeOut_Ticks,
					(int)session, //TO_ConnID,
					GetSerial(), //ConnSerialNumber
					_OriginatorVendorID,_OriginatorSerialNumber,_TimeOutMultiplier,
					UPDATERATE*500,
					_Transport,
					plc->NetWork,
					path,
					path_size);
				else
					connection=_ConnectPLCOverCNET(session,
					plc->PlcType,
					_Priority,_TimeOut_Ticks,
					(int)session, //TO_ConnID,
					GetSerial(), //ConnSerialNumber
					_OriginatorVendorID,_OriginatorSerialNumber,_TimeOutMultiplier,
					UPDATERATE*500,
					_Transport,
					path,
					path_size);
	// --------------- Connection is Ok ----------------
			if (connection!=NULL)
			{
				Log(LOG_DEBUG,"Connection (%p) created for PLC : %s\n",connection,cip_err_msg);
	//--------------- Reading / Writing Value from Lgx ------------------         
				for(i=0;i<AB_VARCOUNT;i++)
				{ double val_float;
					int val_int;
					MACHINE *mach=machines.Data[i];			
					Log(LOG_INFO,"entering ReadData %s\n",mach->MachineId);

					res=readplc(plc,session,connection,mach->Ratio_Addr,AB_REAL,&val_float);
					if (res) Log(LOG_WARNING,"ReadData %s = %f (%d): %s\n",mach->Ratio_Addr,val_float,res,cip_err_msg);
					Log(LOG_INFO,"ReadData %s = %f (%d): %s\n",mach->Ratio_Addr,val_float,res,cip_err_msg);
					mach->Ratio=val_float;result+=res;

					res=readplc(plc,session,connection,mach->P_Bob_Addr,AB_REAL,&val_float);
					if (res) Log(LOG_WARNING,"ReadData %s = %f (%d): %s\n",mach->P_Bob_Addr,val_float,res,cip_err_msg);
					Log(LOG_INFO,"ReadData %s = %f (%d): %s\n",mach->P_Bob_Addr,val_float,res,cip_err_msg);
					mach->Prev_Bob=val_float;result+=res;

					res=readplc(plc,session,connection,mach->A_Bob_Addr,AB_REAL,&val_float);
					if (res) Log(LOG_WARNING,"ReadData %s = %f (%d): %s\n",mach->A_Bob_Addr,val_float,res,cip_err_msg);
					Log(LOG_INFO,"ReadData %s = %f (%d): %s\n",mach->A_Bob_Addr,val_float,res,cip_err_msg);
					mach->Act_Bob=val_float;result+=res;

					res=readplc(plc,session,connection,mach->Temp_Addr,AB_REAL,&val_float);
					if (res) Log(LOG_WARNING,"ReadData %s = %f (%d): %s\n",mach->Temp_Addr,val_float,res,cip_err_msg);
					Log(LOG_INFO,"ReadData %s = %f (%d): %s\n",mach->Temp_Addr,val_float,res,cip_err_msg);
					mach->Temp=val_float;result+=res;
				}

	//-------------------------------------------------------/
				if ((res=Forward_Close(connection))>=0)
					Log(LOG_DEBUG,"Connection (%p) Killed\n",connection);
					else Log(LOG_WARNING,"Unable to kill Connection (%p)\n",connection);
			} else 	//connection=NULL
			{
				Log(LOG_CRIT,"Unable to create connection : %s\n",cip_err_msg);
				result=1;
			}
			if ((res=UnRegisterSession(session))>=0)
				Log(LOG_DEBUG,"Session (%p) Killed\n",session);
				else Log(LOG_WARNING,"Unable to kill session (%p)\n",session);
			//Log(LOG_WARNING,"closing session (%p)\n",session);
			CloseSession(session);
			return(result);
		}else // Prob RegisterSession
		{
			Log(LOG_CRIT,"Unable to register session : %s \n",cip_err_msg);
			CloseSession(session);
			return(1);
		}
	} else
	{
		Log(LOG_CRIT,"Unable to open session for : %s\n",cip_err_msg);
		return(1);
	}
}
int UpdateBobine(MACHINE *machine)
{	int i;
	char query[500];

	Log(LOG_DEBUG,"Entering UpdateBobine : %s (bobine n° : %d)\n",machine->MachineId,machine->Prev_Bob);

	memset(query,0,sizeof(query));
	sprintf(query,"update BOBINE set RATIO_VAP=%f,TEMP_EXT=%f where MACHINE='%s' and to_days(NOW())=to_days(TEMPS) and NOBOBINE=%d",machine->Ratio,machine->Temp,machine->MachineId,machine->Prev_Bob);

	Log(LOG_DEBUG,"UpdateBobine : %s \n",query);
	i=_Execute(&Default_Db,query);
	Log(LOG_NOTICE,"UpdateBobine : %s (%d)\n",MysqlErrorMsg,i);
	return(i);
}
int mainprog(void)
{	int res=0;
	int i,rwd;

	res=OpenDb(DBHOST,USER,PASS,DB);
	if (res!=0)
	{
		Log(LOG_CRIT,"+OpenDb (%d) : %s\n",res,MysqlErrorMsg);
		return(res);
	}	else
	{
		while (!Terminated)
		{
			rwd=readwritedata(plc);
			if (rwd!=0) Log(LOG_CRIT,"readwritedata : %d\n",rwd); else
			{
				for(i=0;i<machines.Count;i++)
				{
					MACHINE *mach=machines.Data[i];
					if ((mach->Bobine==0) || (mach->Prev_Bob!=mach->Bobine))
					{	
						if (UpdateBobine(mach)==1) mach->Bobine=mach->Prev_Bob;
					}
				}
			}
			sleep(UpdateRate);
		}
		Log(LOG_INFO,"Logoff : (%d) \n",res);
		return(0);
		CloseDb;
	}
}
int main (int argc,char *argv[])
{
	// Gestion des signaux
	signal(SIGTERM,SigHand);
	signal(SIGINT,SigHand);
	signal(SIGPOLL,SigHand);
	signal(SIGIO,SigHand);
	signal(SIGUSR1,SigHand);
	signal(SIGSEGV,SigHand);

	int c;

	while ((c=getopt(argc,argv,"c:?h"))!=-1)
		switch(c)
		{
			case 'c': // Config File
				{
					Log(LOG_DEBUG,"config : %s\n",optarg);
					memset(Config,0,sizeof(Config));
					strncpy(Config,optarg,sizeof(Config)-1);
					break;
				}
			case '?':
			case 'h':
				{
					printf("%s (Build on %s %s)\n",LOG_TITLE,__DATE__,__TIME__);
					printf("usage: %s:[-c] [-?,h]\n",argv[0]);
					printf("-c\tConfig file {-c...} default is %s\n",Config);
					return(0);
				}
				break;
			default:break;
		}
	starttime=time(NULL);
	if (GetConfig(Config)) exit(1);

	if (DEAMON)
	{
		openlog(LOG_TITLE,LOG_NDELAY,LOG_USER);
	}
	Log(LOG_ALERT,"starting %s, Database is %s on %s (%d)\n",LOG_TITLE,DB,DBHOST,getpid());
	if (DEAMON)
	{
	switch (fork())
		{
		case -1:
						Log(LOG_CRIT,"Daemon creation Error\n");
						closelog();
						exit(1);
		case 0:	setsid();
						chdir("/");
						umask(0);
						close(0);
						close(1);
						close(2);
						Log(LOG_NOTICE,"Daemon OK (Pid = %d)\n",getpid());
						if (0)
							{
								Log (LOG_CRIT,"Connecting Failed\n");
								Log(LOG_ALERT,"stopped\n");
								closelog();
								exit(2);
							}
							else
							{
								mainprog();
								Log(LOG_ALERT,"stopped\n");
								closelog();
								exit(0);
							}
		default : exit(0);
		}
	}else
	{
		mainprog();
		Log(LOG_ALERT,"stopped\n");
		exit(0);
	}
}
