unit TuxEip;

interface

type
    CIP_SINT=char;	//128..127	8 bits signe
    CIP_INT=Smallint;	//32768..32767	16 bits signe
    CIP_DINT=Longint;	//2147483648..2147483647	32 bits signe
    CIP_LINT=Int64;	//2^0..2^63 64 bits signe
    CIP_USINT=Byte;	//0..255	8 bits non signe
    CIP_UINT=word;	//0..65535	16 bits non signe
    CIP_UDINT=longword;//0..4294967295	32 bits non signe

    pByte=^byte;

const
   //dll='TuxEipDll.dll';
   dll='Project1.dll';


{******************** Ethernet/IP constantes **********************}
   MAX_MSG_LEN=1024; // a verifier
   EIP_PORT=$AF12; //44818
   ENCAP_PROTOCOL=$0001;

   // Ethernet IP commands
   EIP_NOP=$0000;
   EIP_LISTTARGETS=$0001; // Reserved for legacy RA
   EIP_LISTSERVICES=$0004;
   EIP_LISTIDENTITY=$0063;
   EIP_LISTINTERFACES=$0064;
   EIP_REGISTERSESSION=$0065;
   EIP_UNREGISTERSESSION=$0066;
   EIP_SENDRRDATA=$006F;
   EIP_SENDUNITDATA=$0070;
   EIP_INDICATESTATUS=$0072;
   EIP_CANCEL=$0073;

   // Ethernet IP status code
   EIP_SUCCESS=$0000;
   EIP_INVALID_COMMAND=$0001;
   EIP_MEMORY=$0002;
   EIP_INCORRECT_DATA=$0003;
   EIP_INVALID_SESSION_HANDLE=$0064;
   EIP_INVALID_LENGTH=$0065;
   EIP_UNSUPPORTED_PROTOCOL=$0069;

   // Ethernet IP Services Class
   EIP_SC_COMMUNICATION=$0100;
   EIP_VERSION=$01;

{****************************** CM constantes **********************}

const TuxPlcVendorId=1030;

var _OriginatorVendorID:CIP_UINT=TuxPlcVendorId;
    _OriginatorSerialNumber:CIP_UDINT=$12345678;
    _Priority:byte=$0A;//0x07
    _TimeOut_Ticks:CIP_USINT=$05;//0x3f
    _Parameters:word=$43f8;
    _Transport:byte=$a3;
    _TimeOutMultiplier:byte=$01;

{****************************** AB constantes **********************}
const
   Channel_A=CIP_USINT($01);
   Channel_B=CIP_USINT($02);

   // Plc & Slc Datatypes
   PLC_BIT=CIP_USINT(1);
   PLC_BIT_STRING=CIP_USINT(2);
   PLC_BYTE_STRING=CIP_USINT(3);
   PLC_INTEGER=CIP_USINT(4);
   PLC_TIMER=CIP_USINT(5);
   PLC_COUNTER=CIP_USINT(6);
   PLC_CONTROL=CIP_USINT(7);
   PLC_FLOATING=CIP_USINT(8);
   PLC_ARRAY=CIP_USINT(9);
   PLC_ADRESS=CIP_USINT(15);
   PLC_BCD=CIP_USINT(16);

   // Logix Datatypes
   LGX_BOOL=CIP_USINT($C1);
   LGX_BITARRAY=CIP_USINT($D3);
   LGX_SINT=CIP_USINT($C2);
   LGX_INT=CIP_USINT($C3);
   LGX_DINT=CIP_USINT($C4);
   LGX_REAL=CIP_USINT($CA);

type
    EPath_Format = (FPadded,FPacked);
    EPath_LogicalAdress=(_8Bits,_16Bits,_32Bits);
    Path_Segment=byte;
    Error_type=(Internal_Error,Sys_Error,EIP_Error,MR_Error,CM_Error,AB_Error,PCCC_Error);

    Eip_Session=packed record
		sock:integer;
		// Encapsulation parameters
		Session_Handle:CIP_UDINT;
		Sender_ContextL,Sender_ContextH:CIP_DINT;
		// Receive time-out
		timeout:integer;
		References:integer;
		Data:pointer;
                end;
    pEip_Session=^Eip_Session;

    Eip_Connection=packed record
	  // Connected send parameters
		Session:pEip_Session;
		References:integer;
		Data:pointer;
		ConnectionSerialNumber:CIP_UINT;
		OriginatorVendorID:CIP_UINT;
		OriginatorSerialNumber:CIP_UDINT;
		OT_ConnID:CIP_UDINT; //originator's CIP Produced session ID
		TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
		packet:CIP_INT;
		Path_size:BYTE;
		end;
    pEip_Connection=^Eip_Connection;

    DHP_Header=packed record
                Dest_link:CIP_UINT;
		Dest_adress:CIP_UINT;
		Src_link:CIP_UINT;
		Src_adress:CIP_UINT;
                end;
    pDHP_Header=^DHP_Header;

    Plc_Type=(Unknow,PLC5,SLC500,LGX);
    AB_Data_Type=(AB_UNKNOW,AB_BIT,AB_SINT,AB_INT,AB_DINT,AB_REAL,AB_TIMER,AB_COUNTER);

    LGX_Read=packed record
                    datatype:integer;//CIP_USINT;
                    Varcount:integer;
                    totalsize:integer;
                    elementsize:integer;
                    mask:cardinal;
                    end;
    pLGX_Read=^LGX_Read;

    PLC_Read=packed record
                    datatype:integer;//CIP_USINT;
                    Varcount:integer;
                    totalsize:integer;
                    elementsize:integer;
                    mask:cardinal;
                    end;
    pPLC_Read=^PLC_Read;


  function cip_errno:cardinal;cdecl;external dll;
  function cip_ext_errno:cardinal;cdecl;external dll;
  function cip_err_type:Error_type;cdecl;external dll;
  function cip_err_msg:pchar;cdecl;external dll;


  // Sessions & Connections functions
  function OpenSession(serveur:pchar;port,timeout:integer):PEip_Session; cdecl;external dll name '_OpenSession';
  procedure CloseSession(session:pEip_Session); cdecl;external dll name 'CloseSession';
  function RegisterSession(session:pEip_Session):integer; cdecl;external dll name '_RegisterSession';
  function UnRegisterSession(session:pEip_Session):integer; cdecl;external dll name '_UnRegisterSession';

  function _ConnectPLCOverCNET(
		session:pEip_Session;
		Plc:Plc_Type;
		Priority:byte;
		TimeOut_Ticks:CIP_USINT;//used to calculate request timeout information
		TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
		ConnSerialNumber:CIP_UINT;// session serial number
		OriginatorVendorID:CIP_UINT;
		OriginatorSerialNumber:CIP_UDINT;
		TimeOutMultiplier:CIP_USINT;
		RPI:CIP_UDINT;// originator to target packet rate in msec
		Transport:CIP_USINT;
		path:pByte;pathsize:CIP_USINT):pEip_Connection;cdecl;external dll name '_ConnectPLCOverCNET';

  function ConnectPLCOverCNET(
                session:pEip_Session;
                Plc:Plc_Type;
                TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
                ConnSerialNumber:CIP_UINT;// session serial number
                RPI:CIP_UDINT;// originator to target packet rate in msec
                var path:Byte;pathsize:CIP_USINT):pEip_Connection;forward;

  function _ConnectPLCOverDHP(
		session:pEip_Session;
		Plc:Plc_Type;
		Priority:byte;
		TimeOut_Ticks:CIP_USINT;//used to calculate request timeout information
		TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
		ConnSerialNumber:CIP_UINT;// session serial number
		OriginatorVendorID:CIP_UINT;
		OriginatorSerialNumber:CIP_UDINT;
		TimeOutMultiplier:CIP_USINT;
		RPI:CIP_UDINT;// originator to target packet rate in msec
		Transport:CIP_USINT;
                channel:CIP_USINT; //DHP_Channel
		path:pByte;pathsize:CIP_USINT):pEip_Connection;cdecl;external dll name '_ConnectPLCOverDHP';

  function ConnectPLCOverDHP(
                session:pEip_Session;
                Plc:Plc_Type;
                TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
                ConnSerialNumber:CIP_UINT;// session serial number
                RPI:CIP_UDINT;// originator to target packet rate in msec
                channel:CIP_USINT;
                var path:byte;pathsize:CIP_USINT):pEip_Connection;forward;

  function Forward_Close(connection:pEip_Connection):integer;cdecl;external dll name '_Forward_Close';

  // Logix functions
  function ReadLgxData(
                        session:pEip_Session;
                        connection:pEip_Connection;
                        adress:pchar;
                        number:CIP_UINT):pLGX_Read ;cdecl;external dll name '_ReadLgxData';

  function _WriteLgxData(
                         session:pEip_Session;
                         connection:pEip_Connection;
                         adress:pchar;
                         datatype:CIP_USINT;
                         data:pointer;
                         number:CIP_UINT):integer ;cdecl;external dll name '_WriteLgxData';

  function WriteLgxData(
                         session:pEip_Session;
                         connection:pEip_Connection;
                         adress:pchar;
                         datatype:CIP_USINT;
                         data:single):integer;

  function GetLGXValueAsFloat(reply:pLGX_Read;index:integer):single;cdecl;external dll name '_GetLGXValueAsFloat';
  function GetLGXValueAsInteger(reply:pLGX_Read;index:integer):integer;cdecl;external dll name '_GetLGXValueAsInteger';
  procedure FreeLGXRead(Data:pLGX_Read);cdecl;external dll name '_FreeLGXRead';


  // Plc & Slc functions
  function GetTns:CIP_UINT;

  function _ReadPLCData(
                        session:pEip_Session;
                        connection:pEip_Connection;
                        dhp:pDHP_Header;
                        routepath:pByte;
                        routepathsize:CIP_USINT;
                        plctype:Plc_Type;
                        tns:CIP_UINT;
                        adress:pchar;
                        number:CIP_UINT):pPLC_Read ;cdecl;external dll name '_ReadPLCData';

  function _WritePLCData(
                        session:pEip_Session;
                        connection:pEip_Connection;
                        dhp:pDHP_Header;
                        routepath:pByte;
                        routepathsize:CIP_USINT;
                        plctype:Plc_Type;
                        tns:CIP_UINT;
                        adress:pchar;
                        datatype:CIP_USINT;
                        data:pointer;
                        number:CIP_UINT):integer ;cdecl;external dll name '_WritePLCData';

  function ReadPLCData(
                        session:pEip_Session;
                        connection:pEip_Connection;
                        dhp:pDHP_Header;
                        routepath:pByte;
                        routepathsize:CIP_USINT;
                        plctype:Plc_Type;
                        adress:pchar;
                        number:CIP_UINT):pPLC_Read;

  function WritePLCData(
                         session:pEip_Session;
                         connection:pEip_Connection;
                         dhp:pDHP_Header;
                         routepath:pByte;
                         routepathsize:CIP_USINT;
                         plctype:Plc_Type;
                         adress:pchar;
                         datatype:CIP_USINT;
                         data:single):integer;

  function PCCC_GetValueAsBoolean(reply:pPLC_Read;index:integer):integer;cdecl;external dll name '_PCCC_GetValueAsBoolean';
  function PCCC_GetValueAsInteger(reply:pPLC_Read;index:integer):integer;cdecl;external dll name '_PCCC_GetValueAsInteger';
  function PCCC_GetValueAsFloat(reply:pPLC_Read;index:integer):single;cdecl;external dll name '_PCCC_GetValueAsFloat';
  procedure FreePLCRead(Data:pLGX_Read);cdecl;external dll name '_FreePLCRead';

implementation
var tns:CIP_UINT=0;


function ConnectPLCOverCNET(
         session:pEip_Session;
         Plc:Plc_Type;
         TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
         ConnSerialNumber:CIP_UINT;// session serial number
         RPI:CIP_UDINT;// originator to target packet rate in msec
         var path:byte;pathsize:CIP_USINT):pEip_Connection;
begin
result:=_ConnectPLCOverCNET(session,Plc,_Priority,_TimeOut_Ticks,TO_ConnID,ConnSerialNumber,
		_OriginatorVendorID,_OriginatorSerialNumber,_TimeOutMultiplier,RPI,_Transport,@path,pathsize);
end;
function ConnectPLCOverDHP(
         session:pEip_Session;
         Plc:Plc_Type;
         TO_ConnID:CIP_UDINT; //originator's CIP consumed session ID
         ConnSerialNumber:CIP_UINT;// session serial number
         RPI:CIP_UDINT;// originator to target packet rate in msec
         channel:CIP_USINT;
         var path:byte;pathsize:CIP_USINT):pEip_Connection;
begin
result:=_ConnectPLCOverDHP(session,Plc,_Priority,_TimeOut_Ticks,TO_ConnID,ConnSerialNumber,
		_OriginatorVendorID,_OriginatorSerialNumber,_TimeOutMultiplier,RPI,
                _Transport,channel,@path,pathsize);
end;

function WriteLgxData(
                      session:pEip_Session;
                      connection:pEip_Connection;
                      adress:pchar;
                      datatype:CIP_USINT;
                      data:single):integer;
var res:integer;
    Val_BOOL:byte;
    Val_SINT:byte;
    Val_INT:Smallint;
    Val_DINT:integer;
begin
 case datatype of
  LGX_BOOL:begin
            Val_BOOL:=round(data);
            res:=_WriteLgxData(session,connection,adress,datatype,@Val_BOOL,1);
           end;
  //LGX_BITARRAY
  LGX_SINT:begin
            Val_SINT:=round(data);
            res:=_WriteLgxData(session,connection,adress,datatype,@Val_SINT,1);
           end;
  LGX_INT:begin
            Val_INT:=round(data);
            res:=_WriteLgxData(session,connection,adress,datatype,@Val_INT,1);
           end;
  LGX_DINT:begin
            Val_DINT:=round(data);
            res:=_WriteLgxData(session,connection,adress,datatype,@Val_DINT,1);
           end;
  LGX_REAL:begin
            //Val_REAL:=StrToFloat(value.text);
            res:=_WriteLgxData(session,connection,adress,datatype,@data,1);
           end;
  else res:=-1;
 end;
 result:=res;
end;

function GetTns:CIP_UINT;
begin
 tns:=tns+1;
 result:=tns;
end;

function ReadPLCData(
                     session:pEip_Session;
                     connection:pEip_Connection;
                     dhp:pDHP_Header;
                     routepath:pByte;
                     routepathsize:CIP_USINT;
                     plctype:Plc_Type;
                     adress:pchar;
                     number:CIP_UINT):pPLC_Read;
begin
 result:=_ReadPLCData(session,connection,dhp,routepath,routepathsize,plctype,GetTns,adress,number);
end;

function WritePLCData(
                      session:pEip_Session;
                      connection:pEip_Connection;
                      dhp:pDHP_Header;
                      routepath:pByte;
                      routepathsize:CIP_USINT;
                      plctype:Plc_Type;
                      adress:pchar;
                      datatype:CIP_USINT;
                      data:single):integer;
var res:integer;
    Val_INT:Smallint;
begin

 case datatype of
  PLC_INTEGER:begin
            Val_INT:=round(data);
            res:=_WritePLCData(session,connection,dhp,routepath,routepathsize,plctype,GetTns,adress,datatype,@Val_INT,1);
           end;
  PLC_FLOATING:begin
            res:=_WritePLCData(session,connection,dhp,routepath,routepathsize,plctype,GetTns,adress,datatype,@data,1);
           end;
  else res:=-1;
 end;
 result:=res;
end;

end.
