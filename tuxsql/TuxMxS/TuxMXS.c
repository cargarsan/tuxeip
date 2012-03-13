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

#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
		
		
//#define INCLUDE_SRC
#define MAX_LISTE_SIZE 500
#include <sutil/MySQL.h>
#include <sutil/List.h>
#include <sutil/iniparser.h>
#include <tuxeip/TuxEip.h>
#include <simplex/simplex.h>
#include "TuxMXS.h"

#define space printf("\n");
//#define MxPort "6500"
#define UPDATERATE 60
#define TO_RESTART 3
#define WAITTORECONNECT 10

#define MAXERRORTAGPC 50
#define WAITING 60 // Gardian
#define WAIT_FOR_RECONNECT 60

/********************* Functions Decl **************************/
void SigHand(int num);
void Log(int level,char *format,...);
void FlushBuffer(void *buffer,int size);
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

int OpenSock(char *serveur,char *port);
int BuildTags(LISTE *tags);
int GetTagName(LISTE *tags,int Address,char *Name);
void Affiche(TMxMsg *msg);

void Update(LISTE *tags,TMxMsg *msg);
int UpdateTag(char *tag,char *value);
int InsBobine(LISTE *tags,TMxMsg *msg5,TMxMsg *msg6);
int mainprog(void);

/********************** Global Vars ******************************/

int DEAMON=0;
int TEST=0;
int debuglevel=LOG_WARNING;

char Config[MAXSTRSIZE]={"/etc/tuxmx4.conf\0"};
char DBHOST[20]={"localhost\0"};
char DB[20]={"histosql\0"};
char USER[20]={"histosql\0"};
char PASS[20]={"histosql\0"};
char MXNAME[3]={"M4\0"};
char MXADDRESS[20]={"lex\0"};
char MXPORT[5]={"6500\0"};
char LOG_TITLE[20]={"TUXMxSimplex\0"};

int AB_VARCOUNT;
int tns=0;
int Serial=0;
char *PidFile=NULL;
char *CLPath=NULL;
PLC *plc=NULL;

LISTE variables;

int Terminated=0;
int MXsock=-1;
byte inbuf[BufferMaxSize];
Buffer mybuffer;
LISTE Tags;

int msg_ACK=0,msg_NACK=0;
int starttime=0;
int numofmsg=0, //number of messages
		timeout=0, // number of timeout
		numofto=0,
		connlost=0, // number of connections lost
		connretries=0; // number of connections retries

/********************** Functions Def ****************************/

void SigHand(int num)
{ usleep(100000);
	//
	switch (num)
	{
		case SIGTERM:	Log(LOG_NOTICE,"receive SIGTERM\n");
									Terminated=1;
									break;
		case SIGINT:	Log(LOG_NOTICE,"receive SIGINT\n");
									Terminated=1;
									break;
		case SIGIO:	Log(LOG_INFO,"receive SIGIO\n");
									//Terminated=1;
									break;
		case SIGSEGV:	Log(LOG_CRIT,"!!!!!!!!!!!! receive SIGSEGV, program ERROR !!!!!!!!!!!!!!!!!!!\n");
									//siglongjmp(jb,-1);
									Terminated=1;
									exit(1);
									break;
		case SIGUSR1:	Log(LOG_INFO,"receive SIGUSR1\n");
									//Restart=1;
									break;
		default:	Log(LOG_CRIT,"receive signal: %d \n",num);
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
void FlushBuffer(void *buffer,int size)
{	int i;
	if (buffer==NULL) return;
	char *tmpstr=malloc(size+1);
	if (tmpstr==NULL) return;
	memset(tmpstr,0,size+1);
	for(i=0;i<size;i++) sprintf(tmpstr+strlen(tmpstr),"%02X/",*((unsigned char*)(buffer+i)));
	sprintf(tmpstr+strlen(tmpstr),"\n");
	Log(LOG_DEBUG,"FlushBuffer : \n%s",tmpstr);
	free(tmpstr);
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

int OpenSock(char *serveur,char *port)
{
	int s, r, i, portnum;
	struct sockaddr_in sonadr ;

	portnum = atoi(port);
	/*printf("- Connection au serveur %s sur le port %d -\n",argv[1], port);*/

	bzero( (void *)&sonadr, sizeof sonadr );
	sonadr.sin_family = AF_INET ;
	sonadr.sin_port = htons(portnum) ;
	sonadr.sin_addr.s_addr = inet_addr(serveur);

	s = socket( PF_INET, SOCK_STREAM, 0 ) ;
	/* vrification du format de l'adresse donne */
	if( sonadr.sin_addr.s_addr != -1 )
		{
			r=connect(s,(struct sockaddr *)&sonadr, sizeof(sonadr));
			if (!r)	return(s); else {close(s);return(-1);};
		}
	else
	{	/* Le serveur est dsign par nom, il faut
		 * alors lancer un requete dns */
		struct hostent *hp;
		hp = gethostbyname(serveur);
		if (hp==NULL) return(-1);
		for( i=0, r=-1; (r==-1) && (hp->h_addr_list[i] != NULL); i++)
		{	bcopy((char *) hp->h_addr_list[i],(char *)&(sonadr.sin_addr), sizeof(sonadr.sin_addr));
			r=(connect(s, (struct sockaddr *)&sonadr, sizeof(sonadr) ));
		}
		if (!r)	return(s); else {close(s);return(-1);};
	}
}
void Affiche(TMxMsg *msg)
{int i;
	Log (LOG_DEBUG,"idDest : %d\tIdSrc : %d\n",msg->IdDest,msg->IdSrc);
	Log (LOG_DEBUG,"Funct : %d\tSubFunct : %d\n",msg->Funct,msg->SubFunct);
	Log (LOG_DEBUG,"Temps : %s\n",ctime(&(msg->Tps)));
	for(i=0;i<10;i++)
	{
		Log(LOG_DEBUG,"variable [%d] %d = %s\n",i,msg->Variables[i].Repere,msg->Variables[i].Value);
	};
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
void Update(LISTE *tags,TMxMsg *msg)
{	int i;
	if (TEST) return;
	char tagname[30];
	for(i=0;i<10;i++)
	{
		if (GetTagName(tags,msg->Variables[i].Repere,tagname))
		{
			UpdateTag(tagname,msg->Variables[i].Value);
			Log(LOG_DEBUG,"Update [%s] %d = %s\n",tagname,msg->Variables[i].Repere,msg->Variables[i].Value);
		}
	};
}
int GetTagName(LISTE *tags,int Address,char *Name)
{ int i;
	TAGSimplex *Tag;
	//Name="";
	for(i=0;i<tags->Count;i++)
	{
		Tag=tags->Data[i];
		if (Tag->Address==Address)
		{
			strncpy(Name,Tag->TagName,30);
			return(1);
		}
	}
	return(0);
}
int BuildTags(LISTE *tags)
{
	int index=0;
	TAGSimplex *Tag;
	char *sel_str="SELECT d.TAGNAME,d.ADDRESS,d.PLCNAME FROM DEFINITION as d RIGHT JOIN \
	GROUPEMX as g ON g.GROUPNAME=d.PLCNAME where d.ADDRESS is not null and g.MXNAME='M4'";
	Log(LOG_DEBUG,"GetTags : %s\n",sel_str);
	MYSQL_ROW row;
	//int res=mysql_real_query(&Default_Db,sel_str,strlen(sel_str));
	int res=Select(sel_str);
	if (res)
	{
		GetErrorCode;
		Log (LOG_CRIT,"BuildTags : %s\n",MysqlErrorMsg);
		return(-1);
	}
	Store_result;
	if (SqlResult==NULL)
	{
		GetErrorCode;
		Log (LOG_CRIT,"BuildTags : %s\n",MysqlErrorMsg);
		return(-1);
	}
	res=mysql_num_rows(SqlResult);

	while ((row = mysql_fetch_row(SqlResult)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(SqlResult);
		Tag=malloc(sizeof(TAGSimplex));
		memset(Tag,0,sizeof(TAGSimplex));
		if (Tag!=NULL)
		{
			AddListe(tags,Tag);
			snprintf(Tag->TagName,lengths[0]+1,"%s",row[0]);
			if(row[1]!=NULL) Tag->Address=atoi(row[1]);
			if (strncmp(row[2],"@",1)==0) Tag->Bobine=1;
		} else Log(LOG_CRIT,"Malloc error in BuildTags\n");
		index++;
	}
	mysql_free_result(SqlResult);
	res=GetErrorCode;
	if (res) return(res); else return(index);
}
int InsBobine(LISTE *tags,TMxMsg *msg5,TMxMsg *msg6)
{	int i,nbre_vars=0;
	char tagn[500];
	char values[500];
	char *str,*name;
	char tagname[30];
	//char *name;
	if (TEST) return(0);

	Log(LOG_DEBUG,"Entering InsBobine\n");
	if ((msg5==NULL)&&(msg6==NULL)) return(-1);

	memset(tagn,0,sizeof(tagn));
	memset(values,0,sizeof(values));

	sprintf(tagn,"TEMPS,MACHINE,");
	sprintf(values,"now(),'%s',",MXNAME);

	for (i=0;i<10;i++)
	{
		if ((msg5!=NULL)&&(GetTagName(tags,msg5->Variables[i].Repere,tagname)))
		{ name=strchr(tagname,'_');
			if (name!=NULL)
			{
				name++;
				if (nbre_vars++) {sprintf(tagn+strlen(tagn),",");sprintf(values+strlen(values),",");}
				sprintf(tagn+strlen(tagn),"%s",name);
				str=msg5->Variables[i].Value;
				sprintf(values+strlen(values),"'%s'",str);
			}
		}
		if ((msg6!=NULL)&&(GetTagName(tags,msg6->Variables[i].Repere,tagname)))
		{ name=strchr(tagname,'_');
			if (name!=NULL)
			{
				name++;
				if (nbre_vars++) {sprintf(tagn+strlen(tagn),",");sprintf(values+strlen(values),",");}
				sprintf(tagn+strlen(tagn),"%s",name);
				str=msg6->Variables[i].Value;
				sprintf(values+strlen(values),"'%s'",str);
			}
		}
	}
	readwritedata(plc,&variables);
	for (i=0;i<AB_VARCOUNT;i++)
	{
		TAG *atag=variables.Data[i];
		sprintf(tagn+strlen(tagn),",");sprintf(values+strlen(values),",");
		sprintf(tagn+strlen(tagn),"%s",atag->TagName);
		sprintf(values+strlen(values),"'%f'",atag->Value);
	}
	Log(LOG_DEBUG,"InsBobine : %s \n %s \n length : (%d , %d)\n",tagn,values,strlen(tagn),strlen(values));
	i=_Execute(&Default_Db,"insert into BOBINE (%s) values (%s)",tagn,values);

	Log(LOG_NOTICE,"insert into BOBINE (%s) values (%s)\n",tagn,values);
	Log(LOG_NOTICE,"InsBobine : %s (%d)\n",MysqlErrorMsg,i);
	return(i);
}
int mainprog(void)
{	int res=0,return_len;
	TMxMsg *msg=NULL,*msg5=NULL,*msg6=NULL;
	int nfds;
	fd_set rfds, afds;
	struct timeval to;

	nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(MXsock, &afds);

	res=OpenDb(DBHOST,USER,PASS,DB);
	if (res<0) {Log(LOG_CRIT,"OpenDb (%d) : %s\n",res,MysqlErrorMsg);return(res);}
	else
	{
		res=BuildTags(&Tags);
		if (res<=0)
		{
			Log(LOG_WARNING,"Nothing to do\n");
			Terminated=1;
		} else
		{
			int i;
			TAGSimplex *Tag;
			for(i=0;i<Tags.Count;i++)
			{
				Tag=Tags.Data[i];
				Log(LOG_DEBUG,"%d : %s adresse : %d (%d)\n",i,Tag->TagName,Tag->Address,Tag->Bobine);
			}
		}
		//Terminated=1;
		bzero(&mybuffer,sizeof(mybuffer));
		while (!Terminated)
		{
			if (MXsock<0)
			{
				Log(LOG_INFO,"Attempting reconnection (%d connections lost)\n",connlost);
				MXsock=OpenSock(MXADDRESS,MXPORT);
				if (MXsock>=0)
				{
					Log(LOG_NOTICE,"Reconnection OK\n");
				}else
				{
					sleep(WAITTORECONNECT);
					Log(LOG_NOTICE,"Waiting before attempting reconnection (connections retries %d)\n",++connretries);
					continue;
				};
			}
			memcpy(&rfds, &afds, sizeof(rfds));
			to.tv_sec=UPDATERATE;
			to.tv_usec=0;
			switch (select(nfds, &rfds, 0, 0,&to))
			{
				case -1:if(errno == EINTR) continue;
					else Log(LOG_WARNING,"surveillance des descripteurs\n");
				case 0:	continue; // timeout
				default:
					if( FD_ISSET(MXsock, &rfds) )
					{
						return_len=read(MXsock,&inbuf,sizeof(inbuf));
						Log(LOG_DEBUG,"Recu  : %d\n",return_len);
						if (return_len>0)
						{	Addbuffer(&mybuffer,&inbuf,return_len);
							Log(LOG_DEBUG,"Com recu : %d buffersize =%d\n",return_len,mybuffer.Size);
							if (MsgInBuffer(&mybuffer)==0) // get a message
							{	if (CheckMsgOk(&mybuffer)==0) // Message well formatted
								{
									Log(LOG_DEBUG,"Entering DecodeBubber\n");
									msg=DecodeBuffer(&mybuffer);
									/*   */
									Affiche(msg);
									switch (msg->Funct)
									{
										case 3:
											Update(&Tags,msg);
											free(msg);
											break;
										case 4:
											switch (msg->SubFunct)
											{
												case 5:
												case 6:
													if ((msg->SubFunct==5)&&(msg5!=NULL))
													{
														Log(LOG_WARNING,"Reliquat de bobine\n");
														InsBobine(&Tags,msg5,msg6);
														if (msg5!=NULL) {free(msg5);msg5=NULL;};
														if (msg6!=NULL) {free(msg6);msg6=NULL;};
													}
													if (msg->SubFunct==5) msg5=msg;
													if (msg->SubFunct==6) msg6=msg;
													if ((msg5!=NULL)&&(msg6!=NULL))
													{
														Log(LOG_INFO,"Recu bobine\n");
														InsBobine(&Tags,msg5,msg6);
														if (msg5!=NULL) {free(msg5);msg5=NULL;};
														if (msg6!=NULL) {free(msg6);msg6=NULL;};
													}
													break;
												default:
													free(msg);
													break;
											}
											break;
										default:
											free(msg);
											break;
									}
									bzero(&mybuffer,sizeof(mybuffer));
									msg_ACK++;
									Log (LOG_INFO,"Msg recu OK : %d\n",msg_ACK);
								}else
								{
									bzero(&mybuffer,sizeof(mybuffer));
									Reply(MXsock,NAK);
									Log (LOG_NOTICE,"Msg recu mauvais: %d\n",msg_NACK++);
								}
							}else
							{ /* Erreur ?? */
								if (mybuffer.Size>=BufferMaxSize)
								{
									Log(LOG_WARNING,"******** Buffer sature ************\n");
									bzero(&mybuffer,sizeof(mybuffer));
								}
							}
						} else
						{
							close(MXsock);
							FD_CLR(MXsock,&afds);
							MXsock=-1;
							Log(LOG_WARNING,"Server connection lost !\n");
							continue;
						}
					}
			}
		}
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
	MXsock=OpenSock(MXADDRESS,MXPORT);
	if (MXsock == -1)
		{
			Log (LOG_CRIT,"Connecting to MX Failed\n");
			Log(LOG_NOTICE,"stopped\n");
			closelog();
			exit(2);
		}
	Log(LOG_ALERT,"starting %s, Database is %s on %s\n",LOG_TITLE,DB,DBHOST);
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
