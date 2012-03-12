object Form1: TForm1
  Left = 379
  Top = 232
  Width = 652
  Height = 496
  Caption = 'Form1'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object StatusBar1: TStatusBar
    Left = 0
    Top = 450
    Width = 644
    Height = 19
    Panels = <>
    SimplePanel = False
  end
  object HeaderControl1: THeaderControl
    Left = 0
    Top = 0
    Width = 644
    Height = 17
    DragReorder = False
    Sections = <>
  end
  object Panel1: TPanel
    Left = 0
    Top = 17
    Width = 644
    Height = 240
    Align = alTop
    TabOrder = 2
    object Panel2: TPanel
      Left = 1
      Top = 1
      Width = 288
      Height = 238
      Align = alLeft
      TabOrder = 0
      object Session: TGroupBox
        Left = 1
        Top = 1
        Width = 286
        Height = 96
        Align = alTop
        Caption = 'Session'
        TabOrder = 0
        object ip: TEdit
          Left = 8
          Top = 40
          Width = 97
          Height = 21
          TabOrder = 0
          Text = '10.140.200.46'
        end
        object Button1: TButton
          Left = 118
          Top = 24
          Width = 75
          Height = 25
          Caption = 'open session'
          TabOrder = 1
          OnClick = Button1Click
        end
        object Button2: TButton
          Left = 118
          Top = 56
          Width = 75
          Height = 25
          Caption = 'close session'
          TabOrder = 2
          OnClick = Button2Click
        end
      end
      object GroupBox2: TGroupBox
        Left = 1
        Top = 97
        Width = 286
        Height = 140
        Align = alClient
        Caption = 'Var'
        TabOrder = 1
        object Label2: TLabel
          Left = 16
          Top = 24
          Width = 38
          Height = 13
          Caption = 'Address'
        end
        object Label4: TLabel
          Left = 16
          Top = 80
          Width = 27
          Height = 13
          Caption = 'Value'
        end
        object Label5: TLabel
          Left = 16
          Top = 48
          Width = 37
          Height = 13
          Caption = 'Number'
        end
        object Label6: TLabel
          Left = 8
          Top = 112
          Width = 47
          Height = 13
          Caption = 'DataType'
        end
        object address: TEdit
          Left = 64
          Top = 20
          Width = 89
          Height = 21
          TabOrder = 0
          Text = 'int1'
        end
        object value: TEdit
          Left = 64
          Top = 76
          Width = 89
          Height = 21
          TabOrder = 1
          Text = '0'
        end
        object Button5: TButton
          Left = 176
          Top = 24
          Width = 75
          Height = 25
          Caption = 'Read'
          TabOrder = 2
          OnClick = Button5Click
        end
        object Button6: TButton
          Left = 176
          Top = 64
          Width = 75
          Height = 25
          Caption = 'Write'
          TabOrder = 3
          OnClick = Button6Click
        end
        object Edit7: TEdit
          Left = 64
          Top = 48
          Width = 73
          Height = 21
          TabOrder = 4
          Text = '1'
        end
        object number: TUpDown
          Left = 137
          Top = 48
          Width = 16
          Height = 21
          Associate = Edit7
          Min = 0
          Position = 1
          TabOrder = 5
          Wrap = False
        end
        object datatype: TComboBox
          Left = 64
          Top = 108
          Width = 89
          Height = 21
          ItemHeight = 13
          TabOrder = 6
          Text = 'datatype'
          Items.Strings = (
            'PLC_BIT'
            'PLC_BIT_STRING'
            'PLC_BYTE_STRING'
            'PLC_INTEGER'
            'PLC_TIMER'
            'PLC_COUNTER'
            'PLC_CONTROL'
            'PLC_FLOATING'
            'PLC_ARRAY'
            'PLC_ADRESS'
            'PLC_BCD'
            'LGX_BOOL'
            'LGX_BITARRAY'
            'LGX_SINT'
            'LGX_INT'
            'LGX_DINT'
            'LGX_REAL'
            ' ')
        end
        object CheckBox1: TCheckBox
          Left = 176
          Top = 112
          Width = 97
          Height = 17
          Caption = 'Auto Read'
          TabOrder = 7
          OnClick = CheckBox1Click
        end
        object CheckBox2: TCheckBox
          Left = 176
          Top = 96
          Width = 97
          Height = 17
          Caption = 'Show info'
          TabOrder = 8
        end
      end
    end
    object GroupBox1: TGroupBox
      Left = 289
      Top = 1
      Width = 354
      Height = 238
      Align = alClient
      Caption = 'Connection'
      TabOrder = 1
      object Label1: TLabel
        Left = 16
        Top = 24
        Width = 22
        Height = 13
        Caption = 'Path'
      end
      object ConnID: TLabel
        Left = 24
        Top = 72
        Width = 36
        Height = 13
        Caption = 'ConnID'
      end
      object Label3: TLabel
        Left = 8
        Top = 104
        Width = 54
        Height = 13
        Caption = 'Conn Serial'
      end
      object Label7: TLabel
        Left = 152
        Top = 48
        Width = 62
        Height = 13
        Caption = 'Dest address'
        Enabled = False
      end
      object Edit2: TEdit
        Left = 48
        Top = 20
        Width = 153
        Height = 21
        TabOrder = 0
        Text = '1,0'
      end
      object network: TComboBox
        Left = 48
        Top = 48
        Width = 73
        Height = 21
        ItemHeight = 13
        TabOrder = 1
        Text = 'Network'
        OnChange = networkChange
        Items.Strings = (
          'CNet'
          'DHP Ch A'
          'DHP Ch B')
      end
      object plc: TComboBox
        Left = 216
        Top = 20
        Width = 73
        Height = 21
        ItemHeight = 13
        TabOrder = 2
        Text = 'Controller'
        Items.Strings = (
          'SLC500'
          'PLC5'
          'LOGIX')
      end
      object Button4: TButton
        Left = 126
        Top = 144
        Width = 75
        Height = 25
        Caption = 'disconnect'
        TabOrder = 3
        OnClick = Button4Click
      end
      object Button3: TButton
        Left = 22
        Top = 144
        Width = 75
        Height = 25
        Caption = 'connect'
        TabOrder = 4
        OnClick = Button3Click
      end
      object Edit3: TEdit
        Left = 72
        Top = 72
        Width = 73
        Height = 21
        CharCase = ecUpperCase
        TabOrder = 5
        Text = '$12345678'
      end
      object Edit4: TEdit
        Left = 72
        Top = 104
        Width = 73
        Height = 21
        CharCase = ecUpperCase
        TabOrder = 6
        Text = '$6789'
      end
      object Dest_adress: TEdit
        Left = 224
        Top = 48
        Width = 65
        Height = 21
        Enabled = False
        TabOrder = 7
        Text = '0'
      end
    end
  end
  object Memo1: TMemo
    Left = 0
    Top = 257
    Width = 644
    Height = 193
    Align = alClient
    ScrollBars = ssVertical
    TabOrder = 3
  end
  object Timer1: TTimer
    Enabled = False
    Interval = 500
    OnTimer = Button5Click
    Left = 522
    Top = 275
  end
end
