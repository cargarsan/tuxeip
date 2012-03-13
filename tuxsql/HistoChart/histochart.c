//*****************************************
//*	HistoChart								*
//*	Copyright  2004  Leicht			*
//*	mail : leicht@tuxplc.net 			*
//*	web : http://tuxplc.net				*
//*													*
//*	Version 0.99 : 25 may 2004	*
//*****************************************
#include "HistoChart.h"

TPen pen[8];
TPen onePen;
int timerange;
int lag;
int height,width;
char server[21];
time_t curtime;

gdImagePtr im;
int hChart, wChart;
time_t timeMin, timeMax;
int TagNumber;
int Legend_Right; //Pixels Number for right legend

int 	black,
	red,
	green,
	blue,
	white,
	magenta,
	cyan,
	yellow,
	orange,
	lightgray,
	midgray,
	darkgray,
	midgreen,
	darkmagenta,
	darkcyan,
	lightlightgray;
int 	BackgroundColor;
int styleDotted[4], styleDashed[6];
	
int color()
{
	red = gdImageColorAllocate(im, 255, 0, 0);
	green = gdImageColorAllocate(im, 0, 255, 0);
	blue = gdImageColorAllocate(im, 0, 0, 255);
	black = gdImageColorAllocate(im, 0, 0, 0);	
	white = gdImageColorAllocate(im, 255, 255, 255);
	magenta = gdImageColorAllocate(im, 255, 0, 255);
	cyan = gdImageColorAllocate(im, 0, 255, 255);
	yellow = 	gdImageColorAllocate(im, 255, 255, 0);
	orange = gdImageColorAllocate(im, 0xFF, 0x80, 0x00);	
	lightgray = gdImageColorAllocate(im, 0xC3, 0xC3, 0xC3);
	midgray = gdImageColorAllocate(im, 0xA0, 0xA0, 0xA0);
	darkgray = gdImageColorAllocate(im, 0x80, 0x80, 0x80);
	midgreen = gdImageColorAllocate(im, 0, 0xC0, 0);
	darkmagenta = gdImageColorAllocate(im, 0x80, 0, 0x80);	
	darkcyan = gdImageColorAllocate(im, 0, 0x80, 0x80);
	lightlightgray = gdImageColorAllocate(im, 0xFA, 0xFA, 0xFA);
	
	BackgroundColor = lightlightgray;
	pen[0].penColor = red;
	pen[1].penColor = blue;
	pen[2].penColor = green;
	pen[3].penColor = magenta;
	pen[4].penColor = cyan;
	pen[5].penColor = orange;
	pen[6].penColor = darkcyan;
	pen[7].penColor = darkmagenta;

	/* Set up dotted style. Leave every other pixel alone. */
	styleDotted[0] = darkgray;
	styleDotted[1] = gdTransparent;
	styleDotted[2] = gdTransparent;
	styleDotted[3] = gdTransparent;	
	/* Set up dashed style. Three on, three off. */
	styleDashed[0] = darkgray;
	styleDashed[1] = darkgray;
	styleDashed[2] = darkgray;
	styleDashed[3] = gdTransparent;
	styleDashed[4] = gdTransparent;
	styleDashed[5] = gdTransparent;

	return SUCCES;
}

int Open_Db(char *host,char *user,char *password,char *dbname)
{
	mysql_init(&Default_Db);
	mysql_options(&Default_Db,MYSQL_READ_DEFAULT_GROUP,"TUXHISTO");
	mysql_real_connect(&Default_Db,host,user,password,dbname,0,NULL,0);
	return(_GetErrorCode(&Default_Db,MysqlErrorMsg));
}
double calcScale(double inputValue, double I_MIN, double I_MAX, double O_MIN, double O_MAX) {
	double rangeInput = I_MAX-I_MIN;
	double rangeOutput = O_MAX-O_MIN;
	return inputValue*(rangeOutput/rangeInput)+O_MAX-(I_MAX*(rangeOutput/rangeInput));
}

int GetSpan(double Vmin,double Vmax,double *Interval,double *Origine,int MinGrad,int MaxGrad)
{
	double Tab[]={1,2,4,5,10};
	int TabSize = sizeof(Tab)/sizeof(double);
	double span,result=0,acc1=0,acc2=0;
	int facteur,i,j;
	*Interval=0; 
  
  if ((Vmax-Vmin)<=0) //return(0);
	{ 
		double Vmin_temp=Vmin;
		Vmin=Vmax;
		Vmax=Vmin_temp;
	}	
  span=Vmax-Vmin;
	*Origine=Vmin;
  
  if (span<MinGrad) facteur=-ceil(log10(MinGrad/span));
		else facteur=ceil(log10(span/MinGrad))-1;
  for(i=MinGrad;i<=MaxGrad;i++)
   for(j=0;j<TabSize;j++)
    {
      acc1=(i*Tab[j])/(span*pow(10,-facteur));
      if ((acc1>=1)&&((acc1<acc2)||(acc2==0)))
       {
         acc2=acc1;
         result=i;
         *Interval=Tab[j]*pow(10,facteur);
       }
    }
 *Origine=floor(Vmin/(*Interval))*(*Interval);
 if ((*Origine+(*Interval)*result)<Vmax) result+=1;
 return(result);
}

void getHour() {
	//char vmsg[60];
	int diff;
	time(&curtime);
	diff = curtime % 3600;
	//sprintf(vmsg,"diff : %i *** cur : %i *** %s",diff,curtime,ctime(&curtime));
	//MyLog(vmsg);
	curtime = curtime - diff + 3600;
}

void print_error(char *errormsg) 
{
	gdImageString(im, gdFontSmall,	im->sx / 2 - (strlen(errormsg) * gdFontSmall->w / 2),	im->sy / 2 - gdFontSmall->h / 2,	errormsg, black);
}

int GetTag(int index)
{	
	int res;
	char strQuery[255];
	MYSQL_ROW row;
	sprintf(strQuery,"select TAGNAME,I_MIN,I_MAX,O_MIN,O_MAX,TAG_UNIT,ID,DIGITAL from DEFINITION where TAGNAME='%s'",pen[index].TagName);	
	res=mysql_real_query(&Default_Db,strQuery,strlen(strQuery));
	if (res) 
	{
		GetErrorCode;//(&Default_Db,MysqlErrorMsg);
		return ERROR;
	}
	SqlResult=mysql_store_result(&Default_Db);
	if (SqlResult==NULL)
	{
		GetErrorCode;//(&Default_Db,MysqlErrorMsg);
		return ERROR;
	}
	// Empty result
	if (res=mysql_num_rows(SqlResult)==0) {
		bzero(&pen[index],sizeof(TPen));
		sprintf(MysqlErrorMsg,"Empty Result");
		return	EMPTYRESULT;
	}
	while ((row = mysql_fetch_row(SqlResult)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(SqlResult);
		if (row[1]!=NULL)	pen[index].I_MIN=atof(row[1]);	
		if (row[2]!=NULL)	pen[index].I_MAX=atof(row[2]);	
		if (row[3]!=NULL)	pen[index].O_MIN=atof(row[3]);	
		if (row[4]!=NULL)	pen[index].O_MAX=atof(row[4]);
		snprintf(pen[index].UNIT,lengths[5]+1,"%s",row[5]);		
		if (row[6]!=NULL)	pen[index].ID=atoi(row[6]);
		if (row[7]!=NULL)	pen[index].isDigital=atoi(row[7]);			
	}
	mysql_free_result(SqlResult);
	res=GetErrorCode;//(&Default_Db,MysqlErrorMsg);
	return res;
}


int drawChart()
{
	int res;
	int index;
	im = gdImageCreate(width, height);
	// Calc time chart
	timeMin = curtime-((timerange * (lag+1))*3600);
	timeMax = curtime-((timerange * lag)*3600);	
	if (color()) {
		MyLog("Draw color chart failed\n");
		return (-1);
	}
	//draw Background
	gdImageFill(im, 32, 32, BackgroundColor);	
	//Open DB
	res = Open_Db("histohost","histosql","histosql","histosql");
	if (res) {
		print_error(MysqlErrorMsg);
		MyLog(MysqlErrorMsg);
		return ERROR;
	}
	//get tag info
	TagNumber = 0;
	for (index = 0 ; index < PENNUMBER; index++) {
		if (GetTag(index)==0)
			TagNumber++;
	}
	if (TagNumber==1) {
		for (index = 0 ; index < PENNUMBER; index++) {
			if (pen[index].TagName[0] != 0)
				onePen = pen[index];
		}			
	}
	//draw grid
	drawGrid();
	//draw chart zone
	gdImageRectangle(im, XOFFSET, YOFFSET, width - Legend_Right, height - TITLE_BOTTOM, black);
	//draw pen
	for (index = 0 ; index < PENNUMBER; index++) {
		if (pen[index].TagName[0] != 0)
			drawPen(index);
	}	
	
	//Close DB
	CloseDb;
	/* Now output the image. Note the content type! */
	cgiHeaderContentType("image/png");
	/* Send the image to cgiOut */
	gdImagePng(im, cgiOut);
	/* Free the gd image */
    gdImageDestroy(im);	
	return 0;
}

int drawGrid() 
{
	int i, j, x, nTick, nMiniTick, index , vLegend, ypos;
	int maxChar;
	time_t vtime;
	struct tm *tTime;
	double voutput;
	double spanMin=0, interval=0 ,vValue, y;
	int TicksNumber;
	char sdate[30];
	char yLabel[20];
	//char vmsg[30];
	switch (timerange) {
		case 24 :
			nTick = 6;
			nMiniTick = 10;
			break;
		case 12 :
			nTick = 4;
			nMiniTick = 10;
			break;
		case 8 :
			nTick = 2;
			nMiniTick = 10;
			break;
		case 4 :
			nTick = 2;
			nMiniTick = 10;
			break;
		case 1 :
			nTick = 6;
			nMiniTick = 10;
			break;
		default : nTick = 6;
	}
	//set Y min and Y max value when multi pen
	if (TagNumber>1) { 	
		onePen.O_MIN = 0.0;
		onePen.O_MAX = 100.0;		
	}
	//calc initial size chart zone for pen
	if (TagNumber==1) { 
		sprintf(yLabel,"%g",onePen.O_MAX);
		Legend_Right = 26 + strlen(yLabel) *gdFontSmall->w;
	} else {
		maxChar = 0;
		for (index = 0 ; index < PENNUMBER; index++) {
			if (strlen(pen[index].TagName)>maxChar)
				maxChar = strlen(pen[index].TagName);
		}		
		Legend_Right = 40 + maxChar *gdFontSmall->w;		
	}		
	hChart = height - YOFFSET - TITLE_BOTTOM;
	wChart = width - XOFFSET - Legend_Right;
	// calc spanMin & interval & TicksNumber
	TicksNumber = GetSpan(onePen.O_MIN, onePen.O_MAX, &interval, &spanMin, 10, 15);	
	//draw tag unit
	if (TagNumber==1) { 
		gdImageStringUp(im, gdFontSmall,	width - 2 - gdFontSmall->h,	height / 2 - (strlen(onePen.UNIT) *gdFontSmall->w / 2),	onePen.UNIT, black);
	}
	// draw legend for multi pen
	vLegend = 0;
	if (TagNumber>1) { 	
		for (index = 0 ; index < PENNUMBER; index++) {
			if (pen[index].TagName[0] != 0) {
				ypos = (vLegend * (1.5* gdFontSmall->h) ) +YOFFSET + 20 ;
				gdImageLine(im,width - Legend_Right + 20 ,ypos,width - Legend_Right+30,ypos, pen[index].penColor); 
				gdImageString(im, gdFontSmall,	width - Legend_Right+35 ,	ypos - gdFontSmall->h / 2,	pen[index].TagName, black);	
				vLegend++;
			}
		}		
	}
	//X Label and tick for begin chart	
	tTime = localtime(&timeMin);
	strftime(sdate, sizeof(sdate), "%H:%M", tTime);
	x = XOFFSET;
	gdImageLine(im,x,height - TITLE_BOTTOM,x,height - TITLE_BOTTOM+5, darkgray); //for label X
	gdImageString(im, gdFontSmall,	x - (strlen(sdate) * gdFontSmall->w / 2),	height - TITLE_BOTTOM+6,	sdate, black);	
	strftime(sdate, sizeof(sdate), "%d/%m", tTime);		
	gdImageString(im, gdFontSmall,	x - (strlen(sdate) * gdFontSmall->w / 2),	height - TITLE_BOTTOM+8+gdFontSmall->h,	sdate, black);
	//X Label and tick for end chart	
	tTime = localtime(&timeMax);
	strftime(sdate, sizeof(sdate), "%H:%M", tTime);
	x = XOFFSET+wChart;
	gdImageLine(im,x,height - TITLE_BOTTOM,x,height - TITLE_BOTTOM+5, darkgray); //for label X
	gdImageString(im, gdFontSmall,	x - (strlen(sdate) * gdFontSmall->w / 2),	height - TITLE_BOTTOM+6,	sdate, black);	
	strftime(sdate, sizeof(sdate), "%d/%m", tTime);		
	gdImageString(im, gdFontSmall,	x - (strlen(sdate) * gdFontSmall->w / 2),	height - TITLE_BOTTOM+8+gdFontSmall->h,	sdate, black);
	// X label between	begin and end 	
	for (i=0; i < timerange; i=i+nTick) {
			if (i>0) {
			vtime = timeMin+(i*3600);
			tTime = localtime(&vtime);
			strftime(sdate, sizeof(sdate), "%H:%M", tTime);
			voutput=calcScale(vtime,timeMin,timeMax, 0, wChart-2);
			x = XOFFSET + abs(voutput) + 1;
			gdImageLine(im,x,height - TITLE_BOTTOM,x,height - TITLE_BOTTOM+5, darkgray); //for label X
			gdImageString(im, gdFontSmall,	x - (strlen(sdate) * gdFontSmall->w / 2),	height - TITLE_BOTTOM+6,	sdate, black);	 			
		}
	}	 
	//vertical line
	for (i=0; i < timerange; i++) {
		vtime = timeMin+(i*3600);
		voutput=calcScale(vtime,timeMin,timeMax, 0, wChart-2);
		x = XOFFSET + abs(voutput) + 1;
		if (i>0) { 
			gdImageSetStyle(im, styleDashed, 6);
			gdImageLine(im,x,YOFFSET,x,height - TITLE_BOTTOM, gdStyled);
		}
		if (timerange<=8) {
			for (j=1; j < 6; j++) {
				vtime = timeMin+(i*3600)+(j*600);
				voutput=calcScale(vtime,timeMin,timeMax, 0, wChart-2);
				x = XOFFSET + abs(voutput) + 1;	
				gdImageSetStyle(im, styleDotted, 4);			
				gdImageLine(im,x,YOFFSET,x,height - TITLE_BOTTOM, gdStyled);			
			}
		}
	}
	//Y Label and tick for begin chart	
	sprintf(yLabel,"%g",onePen.O_MIN);
	//left Y label
	gdImageLine(im,XOFFSET-4,height - TITLE_BOTTOM,XOFFSET,height - TITLE_BOTTOM, black); 
	gdImageString(im, gdFontSmall,	XOFFSET-6 - (strlen(yLabel) * gdFontSmall->w),	height - TITLE_BOTTOM - gdFontSmall->h / 2,	yLabel, black);	
	//right Y label
	gdImageLine(im,width - Legend_Right,height - TITLE_BOTTOM,width - Legend_Right+4,height - TITLE_BOTTOM, black); 
	gdImageString(im, gdFontSmall,	width - Legend_Right+8 ,	height - TITLE_BOTTOM - gdFontSmall->h / 2,	yLabel, black);			
	//Y Label and tick for end chart	
	sprintf(yLabel,"%g",onePen.O_MAX);
	//left Y label
	gdImageLine(im,XOFFSET-4,YOFFSET,XOFFSET,YOFFSET, black); 
	gdImageString(im, gdFontSmall,	XOFFSET-6 - (strlen(yLabel) * gdFontSmall->w),	YOFFSET- gdFontSmall->h / 2,	yLabel, black);	
	//right Y label
	gdImageLine(im,width - Legend_Right,YOFFSET,width - Legend_Right+4,YOFFSET, black); 
	gdImageString(im, gdFontSmall,	width - Legend_Right+8 ,	YOFFSET - gdFontSmall->h / 2,	yLabel, black);			
	// Y label between	begin and end 	and Horizontal line
	for (i=0; i < TicksNumber ; i++) {
		vValue = spanMin+(i*interval);
		if ((vValue>onePen.O_MIN) && (vValue<onePen.O_MAX)) {
			voutput=calcScale(vValue, onePen.O_MIN, onePen.O_MAX, 0, hChart-2);
			y = hChart - abs(voutput) + YOFFSET -1;
			sprintf(yLabel,"%g",vValue);
			gdImageLine(im,XOFFSET-4,y,XOFFSET,y, darkgray); //left
			gdImageString(im, gdFontSmall,	XOFFSET-6 - (strlen(yLabel) * gdFontSmall->w),	y- gdFontSmall->h / 2,	yLabel, black);	
			gdImageLine(im,width - Legend_Right,y,width - Legend_Right+4,y, darkgray); //right
			gdImageString(im, gdFontSmall,	width - Legend_Right+8 ,	y - gdFontSmall->h / 2,	yLabel, black);	
			gdImageSetStyle(im, styleDashed, 6);
			gdImageLine(im,XOFFSET,y,width - Legend_Right,y, gdStyled);
		}
	}
	
	return 0;	
}

int drawPen(int index) 
{
	int res=0;
	int i = 0;
	int x0 = -1;
	int y0 = -1;
	int x, y, digitalValue ;
	double voutput;
	char vmsg[30];
	char strQuery[255];
	MYSQL_ROW row;
	sprintf(strQuery,"Select UNIX_TIMESTAMP(TIMEVALUE),DATAVALUE from HISTO where ID=%i and TIMEVALUE BETWEEN FROM_UNIXTIME(%i)  and FROM_UNIXTIME(%i) ORDER BY TIMEVALUE",pen[index].ID,timeMin,timeMax);
	//sprintf(strQuery,"Select UNIX_TIMESTAMP(TIMEVALUE),DATAVALUE from HISTO where TAGNAME='%s' and TIMEVALUE>FROM_UNIXTIME(%i)  and TIMEVALUE<FROM_UNIXTIME(%i) ",pen[index].TagName,timeMin,timeMax);
	res=mysql_real_query(&Default_Db,strQuery,strlen(strQuery));
	if (res) 
	{
		GetErrorCode;//(&Default_Db,MysqlErrorMsg);
		return ERROR;
	}
	SqlResult=mysql_use_result(&Default_Db);
	if (SqlResult==NULL)
	{
		GetErrorCode;//(&Default_Db,MysqlErrorMsg);
		return ERROR;
	}
	//sprintf(vmsg,"Nombre : %i=%i",timeMin,timeMax);
	//MyLog(vmsg);	
	while ((row = mysql_fetch_row(SqlResult)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(SqlResult);
		if (pen[index].isDigital != 1) 
		{
			voutput=calcScale(atof(row[1]),pen[index].I_MIN,pen[index].I_MAX, 0, hChart-2);
			y = hChart - voutput + YOFFSET -1 ;
			voutput=calcScale(atof(row[0]),timeMin,timeMax, 0, wChart-2);
			x = XOFFSET + abs(voutput) + 1;
			if (x0!=-1)   {
				if ((y0>YOFFSET) && (y>YOFFSET) && (y0<(YOFFSET+hChart)) && (y<(YOFFSET+hChart))) {
					gdImageLine(im,x0,y0,x,y,pen[index].penColor);
				} 
				if ((y0>YOFFSET) && (y0<(YOFFSET+hChart)) && (y<=YOFFSET)) {
					gdImageLine(im,x0,y0,x,YOFFSET+1,pen[index].penColor);
				} 
				if ((y0<=YOFFSET) && (y>YOFFSET) && (y<(YOFFSET+hChart)) ) {
					gdImageLine(im,x0,YOFFSET+1,x,y,pen[index].penColor);
				} 	
				if ((y0>YOFFSET) && (y>=(YOFFSET+hChart)) && (y0<(YOFFSET+hChart)) ) {
					gdImageLine(im,x0,y0,x,YOFFSET+hChart,pen[index].penColor);
				} 	
				if ((y0>=(YOFFSET+hChart)) && (y>YOFFSET) && (y<(YOFFSET+hChart)) ) {
					gdImageLine(im,x0,YOFFSET+hChart,x,y,pen[index].penColor);
				} 
					/*if ((y0>YOFFSET) && (y>YOFFSET) && (y0<(YOFFSET+hChart)) && (y<(YOFFSET+hChart))) {
						gdImageLine(im,x0,y0,x,y0,pen[index].penColor);
						gdImageLine(im,x,y0,x,y,pen[index].penColor);
					} 
	*/
				}				
			x0=x;
			y0=y;
		} else 
		{
			if (atof(row[1]) != 0) {
				voutput=calcScale(pen[index].O_MAX,0,100, 0, hChart-2);
			}	else {
				voutput=calcScale(pen[index].O_MIN,0,100, 0, hChart-2);
			}				
			y = hChart - voutput + YOFFSET -1 ;
			voutput=calcScale(atof(row[0]),timeMin,timeMax, 0, wChart-2);
			x = XOFFSET + abs(voutput) + 1;
			if (x0!=-1)   {
				if ((y0>YOFFSET) && (y>YOFFSET) && (y0<(YOFFSET+hChart)) && (y<(YOFFSET+hChart))) {
						gdImageLine(im,x0,y0,x,y0,pen[index].penColor);
						gdImageLine(im,x,y0,x,y,pen[index].penColor);
				} 
			}				
			x0=x;
			y0=y;			
		}
	}
	if (pen[index].Limit!=0)
	{
		voutput=calcScale(pen[index].Limit,pen[index].O_MIN,pen[index].O_MAX, 0, hChart-2);
		y = hChart - voutput + YOFFSET;//gdStyledBrushed
		if ((y>YOFFSET) && (y<(YOFFSET+hChart)) ) 
			{	//gdImageLine(im,XOFFSET,y,XOFFSET+wChart,y,pen[index].penColor);
				gdImageDashedLine(im,XOFFSET,y,XOFFSET+wChart,y,pen[index].penColor);
			}
	}
	//sprintf(vmsg,"pen[%i].Limit=%d",index,pen[index].Limit);
	//MyLog(vmsg);
	mysql_free_result(SqlResult);
	res=GetErrorCode;//(&Default_Db,MysqlErrorMsg);	
	return res;
}

int cgiMain() {
	int index;
	char vtag[5];
	char ltag[5];
	//char vmsg[200];
	double test;
	cgiFormIntegerBounded("width", &width, 400, 1200, 800);
	cgiFormIntegerBounded("height", &height, 150, 900, 300);
	cgiFormInteger("range", &timerange, 24);
	cgiFormInteger("lag", &lag, 0);
	getHour();
// 	sprintf(vmsg,"w=%d h=%d r=%d l=%d",width,height,timerange,lag);
// 	MyLog(vmsg);
	// get tag0 to tag7 param
	for (index = 0 ; index < PENNUMBER; index++) {
		sprintf(vtag,"tag%i",index);
		sprintf(ltag,"lim%i",index);
		cgiFormString(vtag, pen[index].TagName, 30);
		cgiFormDouble(ltag, &(pen[index].Limit), 0);
		//sprintf(vmsg,"pen[%i].Limit=%d",index,pen[index].Limit);
		//MyLog(vmsg);
	}
	if (drawChart() != 0)
	{
		MyLog("Draw chart failed\n");
		return (-1);
	}
	return 0;
}
