#include <SFE_BMP180.h>
#include <Wire.h>

#define DATADIR "/altilog/"

#define DEBUG
//#define ENABLE_BEEP

#include <SdFat.h> //22930, 1286
SdFat SD;
File dataFile;


#define BEEP_PIN 5
#define CHIP_SELECT 10

#define MODE_RECORD 0
#define MODE_SERIAL 1

#define CMD_RESET "reset"
#define CMD_LIST "list"
#define CMD_DOWNLOAD "download"
#define CMD_ANALYZE "analyze"
#define CMD_DELETE "delete all"
#define CMD_AUTODEL "autodel"

#define MAX_CMD_LEN 20


SFE_BMP180 pressure;

double baseline; // baseline pressure

unsigned long lastFlush = 0;
unsigned long time_offset = 0;
unsigned char mode = MODE_RECORD;

char serialCmdString[MAX_CMD_LEN] = "";
unsigned char serialCmdLen = 0;

void(* resetFunc) (void) = 0;


/***************************************************************************************
 * 
 * Pressure Calculation
 * 
 ***************************************************************************************/


double getTemperature()
{
    char status;
    double T;
    
    status = pressure.startTemperature();
    if (status != 0)
    {
        // Wait for the measurement to complete:
        
        delay(status);
        
        // Retrieve the completed temperature measurement:
        // Note that the measurement is stored in the variable T.
        // Use '&T' to provide the address of T to the function.
        // Function returns 1 if successful, 0 if failure.
        
        status = pressure.getTemperature(T);
        if (status != 0) {
            return T;
        } else {
            #if defined(DEBUG) && 0
            Serial.print(F("error retrieving temperature mmnt\n"));
            #endif
            return -100;
        }
    } else {
        #if defined(DEBUG) && 0
        Serial.print(F("error starting temperature mmnt\n"));
        #endif
        return -101;
    }
}


double getPressure(double T)
{
    char status;
    double P, p0, a;
    
    // You must first get a temperature measurement to perform a pressure reading.
    
    // Start a temperature measurement:
    // If request is successful, the number of ms to wait is returned.
    // If request is unsuccessful, 0 is returned.
    
    
    // Start a pressure measurement:
    // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
    // If request is successful, the number of ms to wait is returned.
    // If request is unsuccessful, 0 is returned.
    
    status = pressure.startPressure(3);
    if (status != 0)
    {
        // Wait for the measurement to complete:
        delay(status);
        
        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.
        
        status = pressure.getPressure(P, T);
        if (status != 0)
        {
            return (P);
        } else {
            #if defined(DEBUG) && 0
            Serial.print(F("error retrieving pressure mmnt\n"));
            #endif
            return -100;
        }
    } else {
        #if defined(DEBUG) && 0
        Serial.prinn(F("error starting pressure mmnt\n"));
        #endif
        return -101;
    }
}


/***************************************************************************************
 * 
 * Setup & Main Loop
 * 
 ***************************************************************************************/



void setup() {
    double T;
    Serial.begin(115200);
    
    #if defined(DEBUG) && 0
    Serial.print(F("REBOOT\n"));
    #endif
    
    
    // Initialize the sensor (it is important to get calibration values stored on the device).
    
    if (pressure.begin()) {
        #if defined(DEBUG) && 0
        Serial.print(F("BMP180 init success\n"));
        #endif
    } else {
        // Oops, something went wrong, this is usually a connection problem,
        // see the comments at the top of this sketch for the proper connections.
        #if defined(DEBUG)
        Serial.print(F("BMP180 fail\n\n"));
        #endif
        while (1); // Pause forever.
    }
    
    // Get the baseline pressure:
    
    for (int i=0; i<30; i++) {
        T = getTemperature();
        baseline += getPressure(T);
    }
    baseline = baseline / 30;
    
    
    #if defined(DEBUG) && 0
    Serial.print(F("baseline pressure: "));
    Serial.print(baseline);
    Serial.print(F(" mb\n"));
    
    Serial.print("Initializing SD card...");
    #endif
    
    // see if the card is present and can be initialized:
    if (!SD.begin(CHIP_SELECT, SPI_HALF_SPEED)) {
        #if defined(DEBUG)
        Serial.print(F("SD Card failed\n"));
        #endif //DEBUG
        // don't do anything more:
        return;
    }
    
    #if defined(DEBUG) && 0
    Serial.print(F("card initialized.\n"));
    #endif
    
    if (mode == MODE_RECORD) {
        
        for (unsigned int i = 1; i < 9999; i++) {
            char fname[20] = DATADIR "0000.log";
            int j=4;
            int n=i;
            
            do {       
                fname[8+j] = n % 10 + '0';
                j--;
            } while ((n /= 10) > 0);
            
            if (!SD.exists(fname)) {
                dataFile = SD.open(fname, FILE_WRITE);
                #if defined(DEBUG) && 0
                Serial.print(F("filename: "));
                Serial.print(fname);
                Serial.print("\n");
                #endif
                break;
            }
        }
        
    }
    pinMode(BEEP_PIN, OUTPUT);
    digitalWrite(BEEP_PIN, LOW);
    for (int i=0; i < 3; i++) { beep(100);}
    time_offset = millis();
}

void loop() {
    if (mode == MODE_RECORD) {
        // put your main code here, to run repeatedly:
        double a, P, T;
        char line[100] = "";
        char tmp_str[15];
        
        unsigned long now;
        
        // Get a new pressure reading:
        T = getTemperature();
        P = getPressure(T);
        
        // Show the relative altitude difference between
        // the new reading and the baseline reading:
        
        a = pressure.altitude(P, baseline);
        now = millis() - time_offset;
        
        // format; Milliseconds, pressure, altitude, temperature
        ltoa(now, tmp_str, 10);
        strcat(line, tmp_str);
        strcat(line, ",");
        
        dtostrf(P, 4, 2, tmp_str);
        strcat(line, tmp_str);
        strcat(line, ",");
        
        dtostrf(a, 4, 2, tmp_str);
        strcat(line, tmp_str);
        strcat(line, ",");
        
        dtostrf(T, 4, 2, tmp_str);
        strcat(line, tmp_str);
        strcat(line, "\n");
        
        #if defined(DEBUG) && 0
        Serial.print(line);
        #endif
        
        if (dataFile) {
            dataFile.print(line);
            if (now - lastFlush > 5000) {
                dataFile.flush();
                lastFlush = now;
            }
        }
    }
    if (mode == MODE_SERIAL) {
        delay(50);
    }
}



/***************************************************************************************
 * 
 * Serial Commands
 * 
 ***************************************************************************************/


void serialEvent() {
    while (Serial.available()) {
        // get the new byte:
        char inChar = (char)Serial.read();
        // add it to the inputString:
        
        if (inChar == '\n') {
            //switch mode if required
            if (mode == MODE_RECORD) {
                mode = MODE_SERIAL;
                if (dataFile) {
                    dataFile.flush();
                    dataFile.close();
                }
            }
            
            if (strcmp(serialCmdString,CMD_LIST) == 0) {
                parseDataDirectory(0);
            } else if (strcmp(serialCmdString,CMD_ANALYZE) == 0) {
                parseDataDirectory(1);
            } else if (strcmp(serialCmdString,CMD_AUTODEL) == 0) {
                parseDataDirectory(2);
            } else if (strcmp(serialCmdString,CMD_DOWNLOAD) == 0) {
                parseDataDirectory(3);
            } else if (strcmp(serialCmdString,CMD_DELETE) == 0) {
                parseDataDirectory(4);
            } else if (strcmp(serialCmdString,CMD_RESET) == 0) {
                resetFunc();
            } else {
                Serial.print(F("Unknown command.\n"));
            }
            serialCmdString[0] = '\0';
            serialCmdLen = 0;
        } else {
            if (serialCmdLen < MAX_CMD_LEN) {
                serialCmdString[serialCmdLen] = inChar;
                serialCmdLen++;
                serialCmdString[serialCmdLen] = '\0';
            }
        }
    }
}


void parseDataDirectory(unsigned char mode) {
    //mode:   0 list
    //        1 analyze
    //        2 autodel
    //        3 download all
    //        4 delete all
    File dir = SD.open("/altilog");
    if (mode == 0) {
        Serial.print(F("FN\t\tSIZE\n"));
    } else if (mode <= 2) {
        Serial.print(F("FN\t\tSIZE\t\tDUR\t\tALT\n"));
    }
    
    if (dir) {
        while (true) {
            File entry =  dir.openNextFile();
            if (! entry) {
                // no more files
                break;
            }
            
            long duration = 0;
            double max_alt = 0;
            
            if ((mode == 1) || (mode == 2)) {
                analyzeLog(&entry, &duration, &max_alt);
            }
            
            if (mode <=2) {
                char fname[20];
                entry.getName(fname, 20);
                Serial.print(fname);
                if (!entry.isDirectory()) {
                    char val[10];
                    ltoa(entry.size(), val, 10);
                    Serial.print(F("\t"));
                    Serial.print(val);
                }
            }
            if ((mode == 1) || (mode == 2)) {
                char val[10];
                
                ltoa(duration, val, 10);
                Serial.print(F("\t\t"));
                Serial.print(val);
                
                dtostrf(max_alt, 4, 2, val);
                Serial.print(F("\t\t"));
                Serial.print(val);
            }
            
            if (mode == 3) {
                sendFile(&entry);
            }
            
            if (mode == 4) {
                char fn[20] = DATADIR;
                char fn2[20];
                entry.getName(fn2, 20);
                strcat(fn, fn2);
                Serial.print(fn2);
                if (SD.remove(fn)) {
                    Serial.print(F(" DEL\n"));
                } else {
                    Serial.print(F(" ERR\n"));
                }
            }
            
            entry.close();
            
            if (mode == 2) {
                //delete mode!!
                if (max_alt < 1) { // increase here??
                    char fn[20] = DATADIR;
                    char fn2[20];
                    entry.getName(fn2, 20);
                    strcat(fn, fn2);
                    if (SD.remove(fn)) {
                        Serial.print(F(" DEL"));
                    } else {
                        Serial.print(F(" ERR"));
                    }
                }
            }
            if (mode <=2) {
                Serial.print("\n");
            }
        }
        dir.close();
    }
}

void analyzeLog(File* logfile, long* dur, double* alt) {
    char buffer[64] = "";
    long row = 0;
    long column = 0;
    char new_row = 1;
    long tmp_l;
    double tmp_d;
    size_t n;
    
    
    while (true) {
        n = readField(logfile, buffer, sizeof(buffer), ",\n");
        // done if Error or at EOF.
        if (n == 0) break;
        
        if (new_row == 1) {
            row++;
            column = 1;
            new_row = 0;
        } else {
            column++;
        }
        
        // Print the type of delimiter.
        if (buffer[n-1] == ',') {
            // Remove the delimiter.
            buffer[n-1] = 0;
        }
        if (buffer[n-1] == '\n') {
            // Remove the delimiter.
            buffer[n-1] = 0;
            //remember to increase row counter
            new_row = 1;
        } else {
            // At eof, too long, or read error.  Too long is error.
        }
        
        // format; Milliseconds, pressure, altitude
        
        if (column == 1) {
            tmp_l = atol(buffer);
            if (tmp_l > *dur) {
                *dur = tmp_l;
            }
        } else if (column == 3) {
            tmp_d = atof(buffer);
            if (tmp_d > *alt) {
                *alt = tmp_d;
            } 
        }
    }
}

void sendFile(File* logfile) {
    char fname[20];
    Serial.print(F("## "));
    logfile->getName(fname, 20);
    
    Serial.print(fname);
    Serial.print("\n");
    
    while (logfile->available()) {
        Serial.write(logfile->read());
    }
}



/***************************************************************************************
 * 
 * Misc Utility functions
 * 
 ***************************************************************************************/


/*
 * Read a file one field at a time.
 *
 * file - File to read.
 * str - Character array for the field.
 * size - Size of str array.
 * delim - String containing field delimiters.
 * return - length of field including terminating delimiter.
 *
 * Note, the last character of str will not be a delimiter if
 * a read error occurs, the field is too long, or the file
 * does not end with a delimiter.  Consider this an error
 * if not at end-of-file.
 *
 */
size_t readField(File* file, char* str, size_t size, char* delim) {
    char ch;
    size_t n = 0;
    while ((n + 1) < size && file->read(&ch, 1) == 1) {
        // Delete CR.
        if (ch == '\r') {
            continue;
        }
        str[n++] = ch;
        if (strchr(delim, ch)) {
            break;
        }
    }
    str[n] = '\0';
    return n;
}

void beep(int duration) {
#if defined(ENABLE_BEEP)
    digitalWrite(BEEP_PIN, HIGH);
#endif
    delay(duration);
#if defined(ENABLE_BEEP)
    digitalWrite(BEEP_PIN, LOW);
#endif
    delay(duration);
    
}

