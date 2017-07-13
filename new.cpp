#include <stdlib.h>
#include <fstream> 
#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>
using namespace std; 


const char* predictQuery = "predict -f ETCUBE";
const char* getRotctlQuery = "sudo rotctl -m 601 -r /dev/ttyACM0 -s 9600 get_pos";
int defaultAzimuth;
int mode;
//1 if 222 and 305 belong to [begin,end] then use ulta elevation and ulta azi and default azi to be 270(act) 114(cont)
//2 if only 222 belongs then default = contAzi = 141; act = 180
//3 if only 305 belongs then default = contAzi = 77; act = 0 

void dataLog(string res);	//store data when sat overhead
int getSatEle(string res);	//get elevation angle of satellite using predict data
int getSatAzi(string res);	//get azimuth angle of satellite using predict data
int getRotAzi();
int getRotEle();
void setElevationAzimuth(int elevation, int azimuth);
double actualAzimuth(double cAzi);
double actualElevation(double cEle);
double controlAzimuth(double aAzi);
double controlElevation(double aEle);
int main() {
	int elevation, azimuth;
	int k=1;
	char result[128];
	int def = 0;
	cin>>mode;
	//set default position
	cin>>def;
	if(def != 0) {
		if(mode == 1) set(30, 270);
		if (mode == 2) set(15, 180);
		if(mode == 3) set(15, 0);
	}
	while(k<25) {
		//k++;
		try{
			FILE* pipe = popen(predictQuery, "r");
        	fgets(result, 128, pipe);
			pclose(pipe);
			} catch(std::exception e){
				cout<<"Error in connecting with predict!";
		} 
		cout<<"Data :\n"<<result<<"\n";
		elevation = getSatEle(result);
		//elevation = 45;

		cout<<"Elevation is:"<<elevation<<"\n";
		azimuth = getSatAzi(result);
		//azimuth = 130;
		cout<<"Azimuth is:"<<azimuth<<"\n";
		dataLog(result);
		set(elevation, azimuth);
	}
	return 1;
}

int getSatEle(string info) {	
	int count = 0;
	for(int i=0; info[i]!='*'; i++) {
		if(info[i]==' ') {
			while(info[i]==' ') i++;
			count++;
			if(count==4) {
				char ele[3];
				int j=0;
				while(info[i]!=' ') {
					ele[j] = info[i];
					j++;
					i++;
				}
				return atoi((const char*)ele);
			}
		}
	}
	return 6;	
}

int getSatAzi(string info) {	
	int count = 0;
	for(int i=0; info[i]!='*'; i++) {
		if(info[i]==' ') {
			while(info[i]==' ') i++;
			count++;
			if(count==5) {
				char azi[3];
				int j=0;
				while(info[i]!=' ') {
					azi[j] = info[i];
					j++;
					i++;
				}
				return atoi((const char*)azi);
			}
		}
	}
	return 6;	
}

int getRotAzi() {
	char azi[40];
	char ele[40];
		try{
			FILE* pipe = popen(getRotctlQuery, "r");
        	fgets(azi, 40, pipe);
    		fgets(ele, 40, pipe);
			pclose(pipe);
			} catch(std::exception e){
			cout<<"Error in connecting to rotctl!";
		} 
	contAzimuth = atoi((const char*)azi);   // stores positions as read by device
	return contAzimuth;
}

int getRotEle() {
	char azi[40];
	char ele[40];
		try{
			FILE* pipe = popen(getRotctlQuery, "r");
        	fgets(azi, 40, pipe);
    		fgets(ele, 40, pipe);
			pclose(pipe);
			} catch(std::exception e){
			cout<<"Error in connecting to rotctl!";
		} 
	contElevation = atoi((const char*)ele);	// stores positions as read by device
	return contElevation;
}

void dataLog(string info) {
	try {
		FILE *file;
		file=fopen("pratham2.txt", "a");
		fputs(info.c_str(),file);
		fclose(file);
		} catch(std::exception e){
		cout<<"Error in logging data!";
	} 
}

double actualAzimuth(double cAzi) {
	double aAzi;
	if(mode != 1) aAzi = 2.8555*cAzi - 48.669 + 180;
	else aAzi = 2.8555*cAzi - 48.669 ;
	if (aAzi > 360) aAzi -= 360;
	if(aAzi < 0) aAzi += 360;
	return aAzi;
}
double controlAzimuth(double aAzi) {
	double cAzi;
	/*if(mode != 1) {
		if(aAzi<280) aAzi +=360;
		cAzi = 0.3502*aAzi - 46;
	}*/
	if (mode == 1 && aAzi < 60) aAzi += 360; 
	if (mode == 3 && aAzi < 222) aAzi += 360;
	if (mode == 2 && aAzi < 305) aAzi += 360;
	if(mode != 1) cAzi = 0.3502*aAzi - 46;
	else cAzi = 0.3502*aAzi + 17.044;
	return cAzi;
}

double actualElevation(double cEle) {
	double aEle;
	if(mode != 1) aEle = cEle - 26;
	else aEle = -1.0885*cEle + 218.93;
	return aEle;
}
double controlElevation(double aEle) {
	double cEle;
	if(mode != 1) cEle = aEle + 26;
	else cEle = -0.8978*aEle + 199.8;
	return cEle;
}

void set(int elevation, int azimuth) {
	delta = 3;
	if(elevation < 0) {
		dataLog("sleep mode");
		cout<<"sleep mode";
		sleep(1);
		return;
	}
	else if((elevation > 15 && mode!=1) || (elevation>25 && mode == 1)) {
		cout<<"tracking mode\n";
		dataLog("tracking mode\n");
	}
	else {
		cout<<"preparation to track mode\n";
		dataLog("preparation to track mode\n");	
		if (mode == 1) elevation=25;
		else elevation = 15;
	}
	contAzimuth = controlAzimuth(azimuth);   // stores positions as read by device
	contElevation = controlElevation(elevation);
	do {
		sprintf(setRotctl, "sudo rotctl -m 601 -r /dev/ttyACM0 -s 9600 set_pos %d %d",contAzimuth, contElevation);
		system(setRotctl);
		sleep(0.3);
	}while( abs(getRotAzi() - contAzimuth) < delta && abs(getRotEle() - contElevation) < delta);
	system(setRotctl);
	
	rotorAzimuth = actualAzimuth(contAzimuth);   
	rotorElevation = actualElevation(contElevation);
		
	dataLog("\nAzimuth(control) at:"+contAzimuth);
	dataLog("Azimuth(actual) at:"+rotorAzimuth);
	dataLog("Elevation(actual) at:"+rotorElevation);
	dataLog("\nElevation(control) at:"+contElevation);
	
	cout<<"\nAzimuth(control) set to:"<<contAzimuth;
	cout<<"\nElevation(control) set to:"<<contElevation;
	cout<<"\nAzimuth(actual) set to:"<<actualAzimuth(contAzimuth);
	cout<<"\nElevation(actual) set to:"<<actualElevation(contElevation);
}
