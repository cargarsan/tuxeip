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

// SJ:9/10/06 Modif des champs groupname->plcname

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>

#define MAX_LISTE_SIZE 500
#include <sutil/MySQL.h>
#include <sutil/List.h>
#include <sutil/iniparser.h>
#include <tuxeip/TuxEip.h>
#include <odx/odx.h>

#include "TuxMX.h"

#define space printf("\n");
//#define MxPort "6500"
#define UPDATERATE 60
#define TO_RESTART 3
#define WAITTORECONNECT 10

#define MAXERRORTAGPC 50



//#define WAITING 60 // Gardian
//#define WAIT_FOR_RECONNECT 60

/****************** Struct definition ***************************/



/********************* Function declaration *********************/
void SigHand(int num);
void Log(int level,char *format,...);
void MXLog(char *format,...);
int GetConfig(char * ini_name);

PLC *GetPlc(char *str);
int GetPlcType(char *Type);
int GetNetType(char *NetWork);
int GetSerial(void);
int GetTns(void);
int ParsePath(char *strpath,char Ip[],char Path[]);
int writeplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,void *value);
int readplc(PLC *plc,Eip_Session *session,Eip_Connection *connection,char *address,Data_Type type,double *value);
int readwritedata(PLC *plc,LISTE *var);

int ConnectMx(char *host,char *port,char *user,char *pass,byte options);
int GetGroups(MYSQL *db,LISTE *groups,char *MxName);
int DelGroupe(LISTE *groups,GROUP *groupe);
int StartGroups(void);
int StopGroups(void);

int GetTags(MYSQL *db,GROUP *groupe);
int BuildTags(LISTE *groups);

GROUP *FindGroupByMsgId(LISTE *groups,int MsgId);

int UpdateTag(char *tag,char *value);
int UpdateGroup(GROUP *groupe);
int InsBobine(GROUP *groupe);

int mainprog(void);

/********************** Global Vars ******************************/
//jmp_buf jb;

int DEAMON=0;
int TEST=0;
int debuglevel=LOG_WARNING;

int Terminated=0;
int MXsock=-1;
int starttime=0;
int numofmsg=0, //number of messages
		timeout=0, // number of timeout
		numofto=0,
		connlost=0, // number of connections lost
		connretries=0; // number of connections retries

char Config[MAXSTRSIZE]={"/etc/tuxmx2.conf\0"};
char DBHOST[20]={"localhost\0"};
char DB[20]={"histosql\0"};
char USER[20]={"histosql\0"};
char PASS[20]={"histosql\0"};
char MXNAME[3]={"M2\0"};
char MXADDRESS[20]={"192.168.1.2\0"};
char MXPORT[5]={"6500\0"};
char LOG_TITLE[20]={"TUXMxopen\0"};

int AB_VARCOUNT;
int tns=0;
int Serial=0;
char *PidFile=NULL;
char *CLPath=NULL;
PLC *plc=NULL;

LISTE groups;
LISTE variables;

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
void MXLog(char *format,...)
{	va_list list;
	va_start(list,format);//NULL

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
		vsyslog(LOG_DEBUG,format,list);
	}

	va_end(list);
/*
	va_list list;
	va_start(list,format);
	Log(LOG_DEBUG,format,list);
	va_end(list);*/
}
int ConnectMx(char *host,char *port,char *user,char *pass,byte options)
{ int sock=0,res=0;
	/* Create socket */
	sock=OdxOpenSock(host,port);
	if (sock<0){Log(LOG_CRIT,"OpenSock : %s\n",strerror(errno));return(sock);}
	Log(LOG_INFO,"connected to %s:%s\n",host,port);
	res=OdxLogon("USER0001","PASS01",options);
	Log(LOG_INFO,"Logon : (%d) %s\n",res,odx_err_msg);
	if (res>=0) return(sock); else return(res);
}
int GetConfig(char * ini_name)
{
	//char *_MxAdress,*_MxPort,*_GradFile,*_CLPath;
	int i;
	char eventname[MAXSTRSIZE];
	int debug;
	dictionary	*	ini ;
	ini = iniparser_new(ini_name);
	if (ini==NULL)
	{
		Log(LOG_ERR, "cannot parse file [%s]\n", ini_name);
		return(1) ;
	}
	/* Main Section*/
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

	/* Db section */
	strncpy(DBHOST,iniparser_getstring(ini,"Db:DbHost" ,""),sizeof(DBHOST));
	strncpy(DB,iniparser_getstring(ini,"Db:DbName" ,""),sizeof(DB));
	strncpy(USER,iniparser_getstring(ini,"Db:DbUserName" ,""),sizeof(USER));
	strncpy(PASS,iniparser_getstring(ini,"Db:DbPassword" ,""),sizeof(PASS));


	/* Measurex section */
	strncpy(MXNAME,iniparser_getstring(ini,"Measurex:MxId" ,""),sizeof(MXNAME));
	strncpy(MXADDRESS,iniparser_getstring(ini,"Measurex:MxAdress" ,""),sizeof(MXADDRESS));
	strncpy(MXPORT,iniparser_getstring(ini,"Measurex:MxPort" ,""),sizeof(MXPORT));

	/* PLC Section */
	CLPath=strdup(iniparser_getstring(ini,"PLC:CLPath" ,NULL));
	plc=GetPlc(CLPath);

	/* Variables Section*/
	AB_VARCOUNT=0;
	for(i=0;i<ini->n;i++)
	{
		char *section=iniparser_getsectionname(ini,i);
		if (strcasecmp("Variables",section)==0)
		{
			char *key=iniparser_getkeyname(ini,i);
			if (key!=NULL)
			{
				AB_VARCOUNT++;
				TAG *var=malloc(sizeof(TAG));
				AddListe(&variables,var);
				memset(var,0,sizeof(TAG));
				strncpy((char *)(var->TagName),key,MAXALIASSIZE);
				strncpy((char *)(var->Value_Address),ini->val[i],MAXALIASSIZE);
				snprintf(eventname,MAXSTRSIZE-1,"Events:%s",var->TagName);
				char *temp=iniparser_getstring(ini,eventname,NULL);
				if (temp!=NULL)	strncpy((char *)(var->Reset_Address),temp,MAXALIASSIZE);
				free(key);
			}
		}
		free(section);
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
						Log(LOG_DEBUG,"ReadPlc : %s on %s = %f (%s)\n",address,plc->PlcName,*value,cip_err_msg);
					}	else
					{
						Log(LOG_WARNING,"Get PCCC value on tag %s (%s) : (%d / ext %d) %s\n",address,plc->PlcName,cip_errno,cip_err_msg,cip_ext_errno);
					}
					free(data);
				} else
				{
					Log(LOG_WARNING,"Error while decoding PCCC value on tag %s (%s) : (%d) %s\n",address,plc->PlcName,cip_errno,cip_err_msg);
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
					Log(LOG_DEBUG,"ReadPlc : %s on %s = %f (%s)\n",address,plc->PlcName,*value,cip_err_msg);
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
		default:Log(LOG_WARNING,"Plc type unknow for : %s\n",plc->PlcName);
			break;
	}
	return(cip_errno);
}
int readwritedata(PLC *plc,LISTE *var)
{	int res,i,path_size;
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
	/*******                  Connection is Ok               **********/
			if (connection!=NULL)
			{
				Log(LOG_DEBUG,"Connection (%p) created for PLC : %s\n",connection,cip_err_msg);
	/***********           Reading / Writing Value from Lgx            ******************/
				for(i=0;i<AB_VARCOUNT;i++)
				{ double val_float;
					int val_int;
					TAG *atag=var->Data[i];

					Log(LOG_INFO,"entering ReadData %s\n",atag->Value_Address);
					res=readplc(plc,session,connection,atag->Value_Address,AB_REAL,&val_float);
					if (res) Log(LOG_WARNING,"ReadData (%d): %s\n",res,cip_err_msg);
					Log(LOG_INFO,"ReadData (%d): %s\n",res,cip_err_msg);
					atag->Value=val_float;

					val_int=1;
					Log(LOG_INFO,"entering WriteData %s = %f\n",atag->Reset_Address,val_int);
					res=writeplc(plc,session,connection,atag->Reset_Address,AB_BIT,&val_int);
					if (res) Log(LOG_WARNING,"WriteData (%d): %s\n",res,cip_err_msg);
					Log(LOG_INFO,"WriteData (%d): %s\n",res,cip_err_msg);
				}

	/***************************************************************/
				if ((res=Forward_Close(connection))>=0)
					Log(LOG_DEBUG,"Connection (%p) Killed\n",connection);
					else Log(LOG_WARNING,"Unable to kill Connection (%p)\n",connection);
			} else 	//connection=NULL
			{
				Log(LOG_CRIT,"Unable to create connection : %s\n",cip_err_msg);
			}
			if ((res=UnRegisterSession(session))>=0)
				Log(LOG_DEBUG,"Session (%p) Killed\n",session);
				else Log(LOG_WARNING,"Unable to kill session (%p)\n",session);
			//Log(LOG_WARNING,"closing session (%p)\n",session);
			CloseSession(session);
			return(0);
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
GROUP *FindGroupByMsgId(LISTE *groups,int MsgId)
{ int i;
	for(i=0;i<groups->Count;i++)
	{
		GROUP *groupe=groups->Data[i];
		if (groupe->MsgId==MsgId) return groupe;
	}
	return(NULL);
}
int GetGroups(MYSQL *db,LISTE *groups,char *MxName)
{
	int index=0;
	char sel_str[255];
	char *tmp_str="SELECT distinct g.* FROM GROUPEMX as g LEFT JOIN DEFINITION as d ON g.GROUPNAME=\
		d.PLCNAME where d.ADDRESS is not null and d.READING=1";
	if (MxName!=NULL) sprintf(sel_str,"%s and MXNAME='%s'",tmp_str,MxName);
		else sprintf(sel_str,"%s",tmp_str);
	Log(LOG_DEBUG,"GetGroups : %s\n",sel_str);
	MYSQL_ROW row;
	int res=mysql_real_query(db,sel_str,strlen(sel_str));
	if (res)
	{
		_GetErrorCode(db,MysqlErrorMsg);
		return(-1);
	}
	SqlResult=mysql_store_result(db);
	if (SqlResult==NULL)
	{
		_GetErrorCode(db,MysqlErrorMsg);
		return(-1);
	}
	if (groups==NULL)
	{
		res=mysql_num_rows(SqlResult);
		mysql_free_result(SqlResult);
		return(res);
	}
	while ((row = mysql_fetch_row(SqlResult)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(SqlResult);
		GROUP *groupe=malloc(sizeof(GROUP));
		AddListe(groups,groupe);
		memset(groupe,0,sizeof(GROUP));
		snprintf(groupe->GroupName,lengths[0]+1,"%s",row[0]);
		if(row[2]!=NULL) groupe->TypeEchange=atoi(row[2]);
		if(row[3]!=NULL) groupe->TempsCycle=atoi(row[3]);
		if(row[5]!=NULL) groupe->TransitionMx=atoi(row[5]);
		snprintf(groupe->Ordinal,lengths[6]+1,"%s",row[6]);
		index++;
	}
	mysql_free_result(SqlResult);
	res=_GetErrorCode(db,MysqlErrorMsg);
	if (res) return(res); else return(index);
}
int DelGroupe(LISTE *groups,GROUP *groupe)
{
	free(groupe->TagNames);
	free(groupe->Symbols);
	free(groupe->ReadInfo);
	return(RemoveListe(groups,groupe));
}
int GetTags(MYSQL *db,GROUP *groupe)
{
	int index=0;
	char sel_str[255];
	char *tmp_str="select TAGNAME,ADDRESS\
	from DEFINITION where ADDRESS is not null and READING=1";
	if (groupe!=NULL) sprintf(sel_str,"%s and PLCNAME='%s'",tmp_str,groupe->GroupName);
		else sprintf(sel_str,"%s",tmp_str);
	Log(LOG_DEBUG,"GetTags : %s\n",sel_str);
	MYSQL_ROW row;
	int res=mysql_real_query(db,sel_str,strlen(sel_str));
	if (res)
	{
		_GetErrorCode(db,MysqlErrorMsg);
		return(-1);
	}
	SqlResult=mysql_store_result(db);
	if (SqlResult==NULL)
	{
		_GetErrorCode(db,MysqlErrorMsg);
		return(-1);
	}
	res=mysql_num_rows(SqlResult);
	if (groupe==NULL)
	{
		mysql_free_result(SqlResult);
		return(res);
	}
	groupe->Varcount=res;

	groupe->TagNames=malloc(res*sizeof(t_f_symname));
	memset(groupe->TagNames,0,res*sizeof(t_f_symname));

	groupe->Symbols=malloc(res*sizeof(t_f_symname));
	memset(groupe->Symbols,0,res*sizeof(t_f_symname));

	groupe->ReadInfo=malloc(res*sizeof(t_read_status_info));
	memset(groupe->ReadInfo,0,res*sizeof(t_read_status_info));

	while ((row = mysql_fetch_row(SqlResult)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(SqlResult);
		snprintf(groupe->TagNames[index],lengths[0]+1,"%s",row[0]);
		snprintf(groupe->Symbols[index],lengths[1]+1,"%s",row[1]);
		index++;
	}
	mysql_free_result(SqlResult);
	res=_GetErrorCode(db,MysqlErrorMsg);
	if (res) return(res); else return(index);
}
int BuildTags(LISTE *groups)
{ int i,count=0;
	int res=GetGroups(&Default_Db,groups,MXNAME);
	if (res)
	{
		for (i=0;i<groups->Count;i++)
		{
			count+=GetTags(&Default_Db,groups->Data[i]);
		}
		return(count);
	} else return(res);
}
int UpdateTag(char *tag,char *value)
{
	char exec_str[255];
	int res1=0;
	if (!TEST)
	{
		sprintf(exec_str,"update DEFINITION set SNAPSHOT_VALUE='%s',SNAPSHOT_TIME=now() where TAGNAME='%s'",value,tag);
		res1=mysql_real_query(&Default_Db,exec_str,strlen(exec_str));
	}
	if (!TEST) return(_GetErrorCode(&Default_Db,MysqlErrorMsg)); else return(0);
}
int InsBobine(GROUP *groupe)
{	int i;
	char tags[500];
	char values[500];
	char str[50];
	//char tagname[50];
	char *name;

	Log(LOG_DEBUG,"Entering InsBobine : %s\n",groupe->GroupName);

	memset(tags,0,sizeof(tags));
	memset(values,0,sizeof(values));

	sprintf(tags,"TEMPS,MACHINE,");
	sprintf(values,"now(),'%s',",MXNAME);

	for (i=0;i<groupe->Varcount;i++)
	{
		OdxGetValueAsString(i,groupe->ReadInfo,str,sizeof(str));
		name=strchr(groupe->TagNames[i],'_');
		if (name!=NULL)
		{
			name++;
			//strcpy(tagname,groupe->TagNames[i]+4);
			if (i) {sprintf(tags+strlen(tags),",");sprintf(values+strlen(values),",");}
			sprintf(tags+strlen(tags),"%s",name);
			sprintf(values+strlen(values),"'%s'",str);
		}
	}
	readwritedata(plc,&variables);
	for (i=0;i<AB_VARCOUNT;i++)
	{
		TAG *atag=variables.Data[i];
		sprintf(tags+strlen(tags),",");sprintf(values+strlen(values),",");
		sprintf(tags+strlen(tags),"%s",atag->TagName);
		sprintf(values+strlen(values),"'%f'",atag->Value);
	}
	Log(LOG_DEBUG,"InsBobine : %s \n %s \n length : (%d , %d)\n",tags,values,strlen(tags),strlen(values));
	i=_Execute(&Default_Db,"insert into BOBINE (%s) values (%s)",tags,values);

	Log(LOG_NOTICE,"insert into BOBINE (%s) values (%s)\n",tags,values);
	Log(LOG_NOTICE,"InsBobine : %s (%d)\n",MysqlErrorMsg,i);
	return(i);
}
int UpdateGroup(GROUP *groupe)
{	int i,res=0;
	char str[50];
	Log(LOG_DEBUG,"Entering UpdateGroup : %s\n",groupe->GroupName);
	for (i=0;i<groupe->Varcount;i++)
	{
		OdxGetValueAsString(i,groupe->ReadInfo,str,sizeof(str));
		res=UpdateTag(groupe->TagNames[i],str);
		//Log(LOG_DEBUG,"UpdateGroup, tag : %s=%s\n",groupe->TagNames[i],str);
	}
	if (strncmp(groupe->GroupName,"@",1)==0) return(InsBobine(groupe));
	return(res);
}
int StartGroups(void)
{	int i,j,res=0;
	for(i=0;i<groups.Count;i++)
	{
		GROUP *group=groups.Data[i];
		Log(LOG_NOTICE,"+%s : %d,%d,%d ord=%s\n",group->GroupName,group->TypeEchange,group->TempsCycle,group->TransitionMx,group->Ordinal);
		for (j=0;j<group->Varcount;j++)
		{
			Log(LOG_DEBUG,"\tvar %d : %s\n",j,group->Symbols[j]);
		}
		group->MsgId=OdxStartDataRead(group->Ordinal,group->TransitionMx+group->TypeEchange,SYMBOL_CACHE,group->TempsCycle,group->Varcount,group->Symbols,group->ReadInfo);
		if (group->MsgId>=0)
		{
			Log(LOG_NOTICE,"+StartDataRead %s ,id %d (%s)\n",group->GroupName,group->MsgId,odx_err_msg);
		}
		else
		{
			res=1;
			Log(LOG_CRIT,"-StartDataRead %s error !!! (%s)\n",group->GroupName,odx_err_msg);
		}
	}
	return(res);
}
int StopGroups(void)
{	int i,res,result=0;
	for(i=0;i<groups.Count;i++)
	{
		GROUP *group=groups.Data[i];
		if (group->MsgId>=0)
		{
			res=OdxCancel(group->MsgId);
			if (res!=0) result=1;
			Log(LOG_INFO,"Cancel : (%d) %s\n",res,odx_err_msg);
		}
	}
	return(result);
}
int mainprog(void)
{	int res=0;
	int i;

	MXsock=ConnectMx(MXADDRESS,MXPORT,"USER0001","PASS01",0);//ID_SESSION_CTL);
	if (MXsock == -1)
		{
			Log (LOG_CRIT,"-Connecting to MX Failed\n");
			Log(LOG_NOTICE,"-stopped\n");
			exit(2);
		}

	res=OpenDb(DBHOST,USER,PASS,DB);
	if (res!=0)
	{
		Log(LOG_CRIT,"+OpenDb (%d) : %s\n",res,MysqlErrorMsg);
		return(res);
	}	else
	{
		res=BuildTags(&groups);
		Log(LOG_WARNING,"+BuildTags : %d (%d groups)\n",res,groups.Count);
		StartGroups();

		while (!Terminated)
		{
			int msglen=0;
			if (MXsock<0)
			{
				Log(LOG_INFO,"-Attempting reconnection (%d connections lost)\n",connlost);
				MXsock=ConnectMx(MXADDRESS,MXPORT,"USER0001","PASS01",0);//ID_SESSION_CTL);
				if (MXsock>=0)
				{
					Log(LOG_NOTICE,"Reconnection OK\n");
					StartGroups();
				}else
				{
					sleep(WAITTORECONNECT);
					Log(LOG_NOTICE,"-Waiting before attempting reconnection (connections retries %d)\n",++connretries);
					continue;
				};
			}
			if ((msglen=OdxGetMsg(0,1000*(1.1*UPDATERATE)))>0)
			{
				numofto=0;
				if (OdxGetmsgtype==MSG_DATA_READ)
				{
					Log(LOG_INFO,"MSG_DATA_READ : msgid = %d (number : %d)\n",OdxGetmsgid,++numofmsg);
					GROUP *groupe=FindGroupByMsgId(&groups,OdxGetmsgid);
					if (groupe!=NULL)
					{
						if (!TEST)
						{
							int up=UpdateGroup(groupe);
							Log(LOG_NOTICE,"\tUpdateGroup %s : %s (%d)\n",groupe->GroupName,MysqlErrorMsg,up);
						}
						Log(LOG_DEBUG,"\t******************************************\n");
						for(i=0;i<groupe->Varcount;i++)
						{ char str[50];
							Log(LOG_DEBUG,"\t%s (%s): %s\n",groupe->TagNames[i],groupe->ReadInfo[i].sym_name,OdxGetValueAsString(i,groupe->ReadInfo,str,sizeof(str)));
						}
					} else Log(LOG_WARNING,"-There is no group with MsgId=%d",OdxGetmsgid);
				} else Log(LOG_INFO,"!receive msg type : %d\n",OdxGetmsgtype);
			} else
			{
				Log(LOG_INFO,"-Wait odx Msg: %s (%d:%d) (type : %d / errno : %d)\n",odx_err_msg,++timeout,numofto,odx_err_type,odx_errno);
				if ((odx_err_type==Odx_Internal_Error)&&(odx_errno==E_ConnectionFailed))
				{
					Log(LOG_WARNING,"-Closing connection : %s\n",odx_err_msg);
					close(MXsock);
					connlost++;
					MXsock=-1;
					continue;
				}
				if (numofto++>=TO_RESTART)
				{
					Log(LOG_INFO,"Attempting to retrieve status\n");
					mxstatus status;
					numofto=0;
					res=OdxStatus(&status);
					if ((odx_errno!=Success)||(status.logon_status!=1))
					{
						Log(LOG_WARNING,"-Status : %s (logon status : %d)\n",odx_err_msg,status.logon_status);
						close (MXsock);
						connlost++;
						MXsock=-1;
					} else Log(LOG_INFO,"Status OK\n");
				}
			}
		}
		StopGroups();
		res=OdxLogoff;
		Log(LOG_INFO,"Logoff : (%d) %s\n",res,odx_err_msg);
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
