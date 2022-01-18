//PHTest.cpp
//this program reads pH data from an Atlas Scientific pH probe operating in I2C mode

#include <stdio.h>
#include <wiringPi.h>
#include "../RemoteControlTest/PHSensor.h"
#include "../RemoteControlTest/Util.h"

enum PH_CALTYPE { LOW_CAL, MID_CAL, HIGH_CAL };

#define NUM_PH_SAMPLES 10

int CheckForFlag(int argc, char** argv, char* szFlag) {//check to see if szFlag can be found in any of the argv elements
	for (int i = 0; i < argc; i++) {
		if (strstr(argv[i], szFlag)) {
			return i;
		}
	}
	return -1;
}

void PHShowUsage() {
	int nNumPHSamples = NUM_PH_SAMPLES;
	printf("PHTest\n");
	printf("Usage:\n\n");
	printf("PHTest [-t <temperature value>] --> the program collects and displays %d samples of pH data. The optional -t flag can be used to specify that temperature compensation be used at the specified temperature value.\n\n",nNumPHSamples);
	printf("PHTest -cmid <pH value> [-t <temperature value>] --> perform a mid-point calibration using a calibration solution with the specified pH. E.g., \"PHTest -cmid 7.00\" does a midpoint calibration assuming a calibration solution with pH 7.00 is used. This midpoint calibration erases any previous low point or high point calibrations performed previously, so should be performed before those calibrations. The -t flag can optionally be used to specify the temperature of the calibration solution. If not used, a temperature of 25 deg C is assumed.\n\n");
	printf("PHTest -clow <pH value> [-t <temperature value>] --> perform a low-point calibration using a calibration solution with the specified pH. E.g., \"PHTest - clow 4.00\" does a lowpoint calibration assuming a calibration solution with pH 4.00 is used. The -t flag can optionally be used to specify the temperature of the calibration solution. If not used, a temperature of 25 deg C is assumed.\n\n");
	printf("PHTest -chigh <pH value> [-t <temperature value>] --> perform a high-point calibration using a calibration solution with the specified pH. E.g., \"PHTest -chigh 10.00\" does a highpoint calibration assuming a calibration solution with pH 10.00 is used. The -t flag can optionally be used to specify the temperature of the calibration solution. If not used, a temperature of 25 deg C is assumed.\n\n");
	printf("PHTest -factory --> restores the factory calibration\n\n");
	printf("PHTest -plock <0 or 1> --> turn the I2C protocol lock feature on (1) or off (0).\n\n");
	printf("PHTest -h --> displays this help message.\n");
}

int DoCalibration(PHSensor *pHSensor, double dCalPHVal, PH_CALTYPE calType, bool bUseTemperature, double dTempDegC) {
	printf("Make sure that the pH sensor is connected to AMOS and immersed in a calibration solution with pH %.2f, then press any key to get started. Once the readings have stabilized, press the 'C' key to do the actual calibration.\n",dCalPHVal);
	int nKeyboardChar = -1;
	while (nKeyboardChar < 0) {
		nKeyboardChar = Util::getch_noblock();
		usleep(10000);
	}
	nKeyboardChar = -1;
	double dPH = 0.0;
	while (nKeyboardChar != 'c' && nKeyboardChar != 'C') {
		nKeyboardChar = Util::getch_noblock();
		if (bUseTemperature) {//get temperature compensated measurement
			if (pHSensor->GetPHSensorPH(dPH, dTempDegC)) {
				printf("pH = %.2f\n", dPH);
			}
		}
		else {
			if (pHSensor->GetPHSensorPH(dPH)) {
				printf("pH = %.2f\n", dPH);
			}
		}
	}
	//do calibration
	bool bCalibratedOK = false;
	if (calType == LOW_CAL) {
		bCalibratedOK = pHSensor->CalibrateLowpoint(dCalPHVal, dTempDegC);
	}
	else if (calType == MID_CAL) {
		bCalibratedOK = pHSensor->CalibrateMidpoint(dCalPHVal, dTempDegC);
	}
	else if (calType == HIGH_CAL) {
		bCalibratedOK = pHSensor->CalibrateHighpoint(dCalPHVal, dTempDegC);
	}
	if (bCalibratedOK) {
		printf("Calibrated successfully.\n");
	}
	else {
		printf("Error, unable to calibrate.\n");
	}
	return 0;
}

void ProtocolLock(int nLock, PHSensor *pHSensor) {
	if (!pHSensor->ProtocolLock(nLock)) {
		printf("Error, unable to change protocol lock.\n");
	}
	else {
		if (nLock > 0) {
			printf("I2C protocol lock successfully enabled.\n");
		}
		else {
			printf("I2C protocol lock successfully removed.\n");
		}
	}
}


int main(int argc, char** argv)
{
    const int NUM_SAMPLES = NUM_PH_SAMPLES;
	pthread_mutex_t i2cMutex = PTHREAD_MUTEX_INITIALIZER;
	PHSensor pHSensor(&i2cMutex);

	double dDissolvedO2 = 0.0;//concentration of dissolved oxygen in mg/L
	
	// GPIO Initialization
	if (wiringPiSetupGpio() == -1)
	{
		printf("[x_x] GPIO Initialization FAILED.\n");
		return -1;
	}
	bool bUseTemp = false;
	double dWaterTemp = DEFAULT_PH_TEMP;
	int nFactory = CheckForFlag(argc, argv, (char*)"-factory");//check to see if factory calibration should be restored
	if (nFactory >= 1) {
		pHSensor.RestoreFactoryCal();
		printf("Factory calibration restored.\n");
		return 0;
	}
	int nTempSpecified = CheckForFlag(argc, argv, (char*)"-t");//check to see if temperature is specified
	if (nTempSpecified > 0) {
		bUseTemp = true;
		int nTempValIndex = nTempSpecified + 1;
		if (argc <= nTempValIndex || sscanf(argv[nTempValIndex], "%lf", &dWaterTemp) < 1) {
			printf("Invalid temperature value.\n");
			return -3;
		}
	}
	int nDoCal = CheckForFlag(argc, argv, (char*)"-cmid");//check to see if midpoint pH calibration should be performed
	if (nDoCal >= 1) {
		//parse pH value
		double dPH = 0.0;
		int nPHValIndex = nDoCal + 1;
		if (argc <= nPHValIndex || sscanf(argv[nPHValIndex], "%lf", &dPH) < 1) {
			printf("Invalid pH value for calibration.\n");
			return -2;
		}
		return DoCalibration(&pHSensor, dPH, MID_CAL, bUseTemp, dWaterTemp);
	}
	nDoCal = CheckForFlag(argc, argv, (char*)"-clow");//check to see if a lowpoint pH calibration should be performed
	if (nDoCal >= 1) {
		//parse pH value
		double dPH = 0.0;
		int nPHValIndex = nDoCal + 1;
		if (argc <= nPHValIndex || sscanf(argv[nPHValIndex], "%lf", &dPH) < 1) {
			printf("Invalid pH value for calibration.\n");
			return -4;
		}
		return DoCalibration(&pHSensor, dPH, LOW_CAL, bUseTemp, dWaterTemp);
	}
	nDoCal = CheckForFlag(argc, argv, (char*)"-chigh");//check to see if a highpoint pH calibration should be performed
	if (nDoCal >= 1) {
		//parse pH value
		double dPH = 0.0;
		int nPHValIndex = nDoCal + 1;
		if (argc <= nPHValIndex || sscanf(argv[nPHValIndex], "%lf", &dPH) < 1) {
			printf("Invalid pH value for calibration.\n");
			return -6;
		}
		return DoCalibration(&pHSensor, dPH, HIGH_CAL, bUseTemp, dWaterTemp);
	}

	int nDoProtcolLock = CheckForFlag(argc, argv, (char*)"-plock");//check to see if the I2C protocol lock should be changed
	if (nDoProtcolLock >= 1) {
		if (argc < 3) {
			printf("Need to specify a third parameter (0 or 1) for the protocol lock flag.\n");
			return -8;
		}
		int nProtocolLock = 0;
		sscanf(argv[2], "%d", &nProtocolLock);
		ProtocolLock(nProtocolLock, &pHSensor);
		return 0;
	}
	int nHelp1 = CheckForFlag(argc, argv, (char*)"-h");//check to see if help is requested 
	int nHelp2 = CheckForFlag(argc, argv, (char*)"-H");//check to see if help is requested 
	if (nHelp1 >= 1||nHelp2>=1) {
		PHShowUsage();
		return 0;
	}
	double dPH = 0.0;
	if (bUseTemp) {
		printf("Temperature compensated (at T = %.2f deg C) pH readings:\n", dWaterTemp);
	}
	else {
		printf("pH readings (no temperature compensation):\n");
	}
	for (int i = 0; i < NUM_SAMPLES; i++) {
		if (bUseTemp) {//get temperature compensated pH reading
			if (pHSensor.GetPHSensorPH(dPH, dWaterTemp)) {
				printf("pH = %.2f\n", dPH);
			}
		}
		else {
			//get pH reading without any temperature compensation
			if (pHSensor.GetPHSensorPH(dPH)) {
				printf("pH = %.2f\n", dPH);
			}
		}
	}
	return 0;
}