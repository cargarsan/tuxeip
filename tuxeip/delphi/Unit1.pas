unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,TuxEip,
  StdCtrls, ComCtrls, Grids, ExtCtrls, Buttons;

type
  TForm1 = class(TForm)
    StatusBar1: TStatusBar;
    HeaderControl1: THeaderControl;
    Panel1: TPanel;
    Panel2: TPanel;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    ConnID: TLabel;
    Label3: TLabel;
    Edit2: TEdit;
    network: TComboBox;
    plc: TComboBox;
    Button4: TButton;
    Button3: TButton;
    Edit3: TEdit;
    Edit4: TEdit;
    Session: TGroupBox;
    ip: TEdit;
    Button1: TButton;
    Button2: TButton;
    GroupBox2: TGroupBox;
    address: TEdit;
    Label2: TLabel;
    Label4: TLabel;
    value: TEdit;
    Button5: TButton;
    Button6: TButton;
    Memo1: TMemo;
    Edit7: TEdit;
    number: TUpDown;
    Label5: TLabel;
    datatype: TComboBox;
    Label6: TLabel;
    Timer1: TTimer;
    CheckBox1: TCheckBox;
    CheckBox2: TCheckBox;
    Dest_adress: TEdit;
    Label7: TLabel;
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure CheckBox1Click(Sender: TObject);
    procedure networkChange(Sender: TObject);
  private
    { Déclarations privées }
  public
    procedure Log(txt:pchar);
    procedure Logf(const Format: string; const Args: array of const);
    procedure SetDataType(datatype:integer);
    function GetDataType:integer;
    function GetPath(var path:array of byte):integer;
  end;

var
  Form1: TForm1;
  session1:PEip_Session;
  conn1:pEip_Connection;
  path:packed array[0..50] of byte;
  pathsize:integer;

implementation

{$R *.DFM}

procedure TForm1.SetDataType(datatype:integer);
begin
 case datatype of
  PLC_BIT:form1.datatype.ItemIndex:=0;
  PLC_BIT_STRING:form1.datatype.ItemIndex:=1;
  PLC_BYTE_STRING:form1.datatype.ItemIndex:=2;
  PLC_INTEGER:form1.datatype.ItemIndex:=3;
  PLC_TIMER:form1.datatype.ItemIndex:=4;
  PLC_COUNTER:form1.datatype.ItemIndex:=5;
  PLC_CONTROL:form1.datatype.ItemIndex:=6;
  PLC_FLOATING:form1.datatype.ItemIndex:=7;
  PLC_ARRAY:form1.datatype.ItemIndex:=8;
  PLC_ADRESS:form1.datatype.ItemIndex:=9;
  PLC_BCD:form1.datatype.ItemIndex:=10;
  LGX_BOOL:form1.datatype.ItemIndex:=11;
  LGX_BITARRAY:form1.datatype.ItemIndex:=12;
  LGX_SINT:form1.datatype.ItemIndex:=13;
  LGX_INT:form1.datatype.ItemIndex:=14;
  LGX_DINT:form1.datatype.ItemIndex:=15;
  LGX_REAL:form1.datatype.ItemIndex:=16;
 else
   form1.datatype.ItemIndex:=-1;
 end;
end;

function TForm1.GetDataType:integer;
begin
 case datatype.ItemIndex of
  0: result:=PLC_BIT;
  1: result:=PLC_BIT_STRING;
  2: result:=PLC_BYTE_STRING;
  3: result:=PLC_INTEGER;
  4: result:=PLC_TIMER;
  5: result:=PLC_COUNTER;
  6: result:=PLC_CONTROL;
  7: result:=PLC_FLOATING;
  8: result:=PLC_ARRAY;
  9: result:=PLC_ADRESS;
  10: result:=PLC_BCD;
  11: result:=LGX_BOOL;
  12: result:=LGX_BITARRAY;
  13: result:=LGX_SINT;
  14: result:=LGX_INT;
  15: result:=LGX_DINT;
  16: result:=LGX_REAL;
 else result:=-1;
 end;
end;

procedure TForm1.Logf(const Format: string; const Args: array of const);
begin
 StatusBar1.SimpleText:=sysutils.Format(Format,Args);
 Memo1.Lines.Add(StatusBar1.SimpleText);
end;

procedure TForm1.Log(txt:pchar);
begin
 StatusBar1.SimpleText:=txt;
 Memo1.Lines.Add(txt);
end;

procedure TForm1.Button1Click(Sender: TObject);
begin
 session1:=OpenSession(pchar(ip.text),44818,500);
 Log(cip_err_msg);
 if session1<>nil then
 begin
  if RegisterSession(session1)<>EIP_SUCCESS then
   Log(cip_err_msg)
  else begin
   Log('Session Ok');
  end;
 end;
end;

procedure TForm1.Button2Click(Sender: TObject);
begin
 UnRegisterSession(session1);
 CloseSession(session1);
 session1:=nil;
 Log('Session closed');
end;

function TForm1.GetPath(var path:array of byte):integer;
var index,value,i:integer;
    s:string;
begin
 i:=0;
 index:=0;
 s:=edit2.Text;
 repeat
  Val(s,value,index);
  delete(s,1,index);
  //Logf('txt : %s',[s]);
  path[i]:=value;
  //Logf('i :%d , value : %d ,index : %d',[i,value,index]);
  i:=i+1;
 until (index=0);
 result:=i;
end;

procedure TForm1.Button3Click(Sender: TObject);
begin
 pathsize:=GetPath(path);
 if not ((network.ItemIndex>=0)and(plc.ItemIndex>=0)) then exit;
 //network.ItemIndex:=0;
 //plc.ItemIndex:=2;

 case network.ItemIndex of
  0:conn1:=ConnectPLCOverCNET(session1,Plc_Type(plc.ItemIndex+1),strtoint(Edit3.Text),
                              strtoint(Edit4.Text),5000,path[0],pathsize);
  1:conn1:=ConnectPLCOverDHP(session1,Plc_Type(plc.ItemIndex+1),strtoint(Edit3.Text),
                              strtoint(Edit4.Text),5000,Channel_A,path[0],pathsize);
  2:conn1:=ConnectPLCOverDHP(session1,Plc_Type(plc.ItemIndex+1),strtoint(Edit3.Text),
                              strtoint(Edit4.Text),5000,Channel_B,path[0],pathsize);
  else begin
   conn1:=nil;
   Log('Connection failed');
  end;
 end;
 if conn1<>nil then Log(cip_err_msg) else Log('Erreur');
end;

procedure TForm1.Button4Click(Sender: TObject);
begin
 if conn1<>nil then
  begin
   Forward_Close(conn1);
   Log(cip_err_msg);
   conn1:=nil;
  end else Log('No Connection to close');
end;

procedure TForm1.Button5Click(Sender: TObject);
var read:pLGX_Read;
    //read_plc:pPLC_Read;
    dhp:pDHP_Header;
    i:integer;
begin
 if (network.ItemIndex>0) then
 begin
  GetMem(dhp,sizeof(DHP_Header));
  dhp.Dest_link:=0;
  dhp.Dest_adress:=StrToInt(Dest_adress.Text);
  dhp.Src_link:=0;
  dhp.Src_adress:=0;
 end else dhp:=NIL;

 case plc.ItemIndex of
  0:begin // SLC500
     pPLC_Read(read):=ReadPLCData(session1,conn1,dhp,NIL,0,SLC500,pchar(address.text),number.Position);
    end;
  1:begin // PLC5
     pPLC_Read(read):=ReadPLCData(session1,conn1,dhp,NIL,0,PLC5,pchar(address.text),number.Position);
    end;
  2:begin
     read:=ReadLgxData(session1,conn1,pchar(address.text),number.Position);
    end;
 else
  read:=NIL;
 end;

 if read<>nil then
  begin
   SetDataType(read.datatype);
   if CheckBox2.Checked then
   begin
    logf('*   datatype : %d (%x)',[read.datatype,read.datatype]);
    logf('*   Varcount : %d (%x)',[read.Varcount,read.Varcount]);
    logf('*   totalsize : %d (%x)',[read.totalsize,read.totalsize]);
    logf('*   elementsize : %d (%x)',[read.elementsize,read.elementsize]);
    logf('*   mask : %d (%x)',[read.mask,read.mask]);
   end; 
   for i:=0 to read.Varcount-1 do
   begin
    if (plc.ItemIndex=2) then value.Text:=floattostr(GetLGXValueAsFloat(read,i))
     else value.Text:=floattostr(PCCC_GetValueAsFloat(pPLC_Read(read),i));
    if (read.Varcount-1)>1 then logf('Address %s[%d] = %s',[address.text,i,value.Text])
    else logf('Address %s = %s',[address.text,value.Text]);
   end;
   FreeLGXRead(read);
  end else
  begin
   Log('Read error');
   Log(cip_err_msg);
  end;
end;

procedure TForm1.Button6Click(Sender: TObject);
var res:integer;
    datatype:integer;
    dhp:pDHP_Header;
begin
 if (network.ItemIndex>0) then
 begin
  GetMem(dhp,sizeof(DHP_Header));
  dhp.Dest_link:=0;
  dhp.Dest_adress:=StrToInt(Dest_adress.Text);
  dhp.Src_link:=0;
  dhp.Src_adress:=0;
 end else dhp:=NIL;
 
 datatype:=GetDataType;
 case plc.ItemIndex of
  0:begin // SLC500
     res:= WritePLCData(session1,conn1,dhp,NIL,0,SLC500,pchar(address.text),datatype,StrToFloat(value.text));
    end;
  1:begin // PLC5
     res:= WritePLCData(session1,conn1,dhp,NIL,0,PLC5,pchar(address.text),datatype,StrToFloat(value.text));
    end;
  2:begin
     res:=WriteLgxData(session1,conn1,pchar(address.text),datatype,StrToFloat(value.text));
    end;
  else res:=0;
 end;
 Logf('result=%d (%s)',[res,cip_err_msg]);
 Log(cip_err_msg);
end;

procedure TForm1.CheckBox1Click(Sender: TObject);
begin
 timer1.Enabled:=CheckBox1.Checked;
end;

procedure TForm1.networkChange(Sender: TObject);
begin
 Dest_adress.Enabled:=(network.ItemIndex>0);
 Label7.Enabled:=Dest_adress.Enabled;
end;

end.
