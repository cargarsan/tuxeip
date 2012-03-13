-- MySQL dump 9.09
--
-- Host: localhost    Database: histosql
-- ------------------------------------------------------
-- Server version	4.0.16-log

--
-- Table structure for table `DEFINITION`
--

CREATE TABLE DEFINITION (
  TAGNAME varchar(30) NOT NULL default '',
  TAG_DEFINITION varchar(100) default NULL,
  ADDRESS varchar(40) default NULL,
  DATA_TYPE varchar(10) default NULL,
  PLCNAME varchar(30) default NULL,
  GROUPNAME varchar(30) default NULL,
  TIME_SAMPLE smallint(6) default NULL,
  TIME_REFRESH int(6) default NULL,
  TIME_CLEANING smallint(6) default '3',
  HYSTERESIS smallint(6) default NULL,
  I_MIN double default NULL,
  I_MAX double default NULL,
  O_MIN double default NULL,
  O_MAX double default NULL,
  TAG_UNIT varchar(20) default NULL,
  RECORDING tinyint(4) default '0',
  READING tinyint(4) default '0',
  SNAPSHOT_VALUE double default NULL,
  SNAPSHOT_TIME datetime default NULL,
  TAG_SYSTEM char(2) default NULL,
  PRIMARY KEY  (TAGNAME)
) TYPE=MyISAM;

--
-- Dumping data for table `DEFINITION`
--

INSERT INTO DEFINITION VALUES ('LT01','Level transmitter','_LT01','0','PLC01',NULL,30,60,3,1,0,100,0,100,'%',1,1,47.421703,'2004-05-28 10:02:28','AB');


-- MySQL dump 9.09
--
-- Host: localhost    Database: histosql
-- ------------------------------------------------------
-- Server version	4.0.16-log

--
-- Table structure for table `PLC`
--

CREATE TABLE PLC (
  PLCNAME varchar(30) NOT NULL default '',
  PLC_PATH varchar(50) default NULL,
  PLC_TYPE varchar(15) default NULL,
  PLC_NETWORK varchar(10) default NULL,
  PLC_NODE smallint(6) default NULL,
  PLC_ENABLE tinyint(4) default '0',
  PRIMARY KEY  (PLCNAME)
) TYPE=MyISAM;

--
-- Dumping data for table `PLC`
--


INSERT INTO PLC VALUES ('PLC01','192.168.1.5,1,0','LGX','CNET',0,1);
INSERT INTO PLC VALUES ('PLC02','192.168.1.5,1,1','SLC','DHP_A',11,1);

-- MySQL dump 9.09
--
-- Host: localhost    Database: histosql
-- ------------------------------------------------------
-- Server version	4.0.16-log

--
-- Table structure for table `HISTO`
--

CREATE TABLE HISTO (
  TAGNAME varchar(30) NOT NULL default '',
  TIMEVALUE datetime NOT NULL default '0000-00-00 00:00:00',
  DATAVALUE double default NULL,
  PRIMARY KEY  (TAGNAME,TIMEVALUE),
  KEY mytag (TAGNAME)
) TYPE=MyISAM;

