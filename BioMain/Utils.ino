#define MAX_MULTI_LOG 10

void printGeneralParameters(Print* output){
#ifdef THR_ETHERNET
  output->print(F("IP:"));
  printIP(output, ip, 4, DEC);
  output->print(F("MAC:"));
  printIP(output, mac, 6, HEX);
#endif
  output->print(F("EPOCH:"));
  output->println(now());
  output->print(F("millis:"));
  output->println(millis());
#ifdef I2C_RELAY_FOOD
  output->print(F("I2C relay food:"));
  output->println(I2C_RELAY_FOOD); 
#endif
#ifdef FLUX
  output->print(F("I2C Flux:"));
  output->println(I2C_FLUX); 
#endif
#ifdef THR_LINEAR_LOGS
  output->print(F("Next log index:"));
  output->println(nextEntryID);
#endif
#ifdef THR_LINEAR_LOGS
  output->print(F("FlashID:"));
  sst.printFlashID(output);
#endif


}

void printIP(Print* output, uint8_t* tab, uint8_t s, byte format){
  for(int i=0; i<s; i++){
    output->print(tab[i], format);
    output->print(' ');
  }
  output->println("");
}


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 
 This method will mainly set/read the parameters:
 Uppercase + number + CR ((-) and 1 to 5 digit) store a parameter (0 to 25 depending the letter)
 example: A100, A-1
 -> Many parameters may be set at once
 example: C10,20,30,40,50
 Uppercase + CR read the parameter
 example: A
 -> Many parameters may be read at once
 example: A,B,C,D
 s : read all the parameters
 h : help
 l : show the log file
 */




void printResult(char* data, Print* output) {
  boolean theEnd=false;
  byte paramCurrent=0; // Which parameter are we defining
  // The maximal length of a parameter value. It is a int so the value must be between -32768 to 32767
#define MAX_PARAM_VALUE_LENGTH 12
  char paramValue[MAX_PARAM_VALUE_LENGTH];
  byte paramValuePosition=0;
  byte i=0;

  while (!theEnd) {
    byte inChar=data[i];
    i++;
    if (inChar=='\0' || i==SERIAL_BUFFER_LENGTH) theEnd=true;
    if (inChar=='p') { // show settings
      printGeneralParameters(output);
    } 
    else if (inChar=='h') {
      printHelp(output);
    }
    else if (inChar=='i') { // show i2c (wire) information
#if defined(GAS_CTRL) || defined(I2C_LCD) || defined(PH_CTRL) || defined(I2C_RELAY_FOOD)
      wireInfo(output); 
#else  //not elsif !!
      noThread(output);
#endif
    } 
    /*
    else if (inChar=='o') { // show oneWire information
     #if defined(TEMP_LIQ) || defined(TEMP_PLATE)
     output->println("To implement");
     //oneWireInfo(output); TODO
     #else
     noThread(output);
     #endif
     }
     */
    else if (inChar=='s') { // show settings
      printParameters(output);
    } 
#ifdef EEPROM_DUMP
    else if (inChar=='z') { // show debug info
      getStatusEEPROM(output);
    } 
#endif
    else if (inChar=='f') { // show settings
      printFreeMemory(output);
    } 
    else if (inChar==',') { // store value and increment
      if (paramCurrent>0) {
        if (paramValuePosition>0) {
          setAndSaveParameter(paramCurrent-1,atoi(paramValue));
        } 
        else {
          output->println(parameters[paramCurrent-1]);
        }
        if (paramCurrent<=MAX_PARAM) {
          paramCurrent++;
          paramValuePosition=0;
          paramValue[0]='\0';
        }
      }
    }
    else if (theEnd) {
      // this is a carriage return;
      if (paramCurrent>0) {
        if (paramValuePosition>0) {
          setAndSaveParameter(paramCurrent-1,atoi(paramValue));
        } 
        else {
          output->println(parameters[paramCurrent-1]);
        }
      }
      else if (data[0]=='c') {
        if (paramValuePosition>0) {
          printCompactParameters(output, atoi(paramValue));
        } 
        else {
          printCompactParameters(output);
        } 
      }  
#ifdef THR_LINEAR_LOGS
      else if (data[0]=='d') {
        if (paramValuePosition>0) {
          if (atol(paramValue)==1234) {
            validateFlash(output);
          }
        } 
        else {
          output->println(F("To format flash enter d1234"));
        }
      }
#endif
      else if (data[0]=='e') {
        if (paramValuePosition>0) {
          setTime(atol(paramValue));
        } 
        else {
          output->println(now());
        }
      }
      else if (data[0]=='l') {
#ifdef THR_LINEAR_LOGS
        if (paramValuePosition>0) {
          printLogN(output,atol(paramValue));
        } 
        else {
          printLastLog(output);
        }
#else
        noThread(output);
#endif
      }
      else if (data[0]=='r') {
        if (paramValuePosition>0) {
          if (atol(paramValue)==1234) {
            resetParameters();
            output->println(F("Reset done"));
          }
        } 
        else {
          output->println(F("To reset enter r1234"));
        }
      }
      else if (data[0]=='q') {
        if (paramValuePosition>0) {
          setQualifier(atoi(paramValue));
        } 
        else {
          uint16_t a=getQualifier();
          output->println(a);
        }
      }  
      else if (data[0]=='m') {
#ifdef THR_LINEAR_LOGS
        if (paramValuePosition>0) {
          long currentValueLong=atol(paramValue);
          if (( currentValueLong - nextEntryID ) < 0) {
            printLogN(output,currentValueLong);
          } 
          else {
            byte endValue=MAX_MULTI_LOG;
            if (currentValueLong > nextEntryID) {
              endValue=0;
            } 
            else if (( nextEntryID - currentValueLong ) < MAX_MULTI_LOG) {
              endValue= nextEntryID - currentValueLong;
            }
            for (byte i=0; i<endValue; i++) {
              currentValueLong=printLogN(output,currentValueLong)+1;
              nilThdSleepMilliseconds(25);
            }
          }
        } 
        else {
          // we will get the first and the last log ID
          output->println(nextEntryID-1);
        }
#else
        noThread(output);
#endif
      }
    }
    else if ((inChar>47 && inChar<58) || inChar=='-') { // a number (could be negative)
      if (paramValuePosition<MAX_PARAM_VALUE_LENGTH) {
        paramValue[paramValuePosition]=inChar;
        paramValuePosition++;
        if (paramValuePosition<MAX_PARAM_VALUE_LENGTH) {
          paramValue[paramValuePosition]='\0';
        }
      }
    } 
    else if (inChar>64 && inChar<92) { // a character so we define the field
      // we extend however the code to allow 2 letters fields !!!
      // we extend however the code to allow 2 letters fields !!
      if (paramCurrent>0) {
        paramCurrent*=26;
      }
      paramCurrent+=inChar-64;
      if (paramCurrent>MAX_PARAM) {
        paramCurrent=0; 
      }
    } 
  }
}

void printHelp(Print* output) {
  //return the menu
  
#ifndef TEMP_PID_COLD  
  output->println(F("(c)ompact settings"));
#ifdef THR_LINEAR_LOGS
  output->println(F("(d)elete flash"));
#endif
  output->println(F("(e)poch"));
  output->println(F("(f)ree"));
  output->println(F("(h)elp"));
  output->println(F("(i)2c"));
  output->println(F("(l)og"));
  output->println(F("(m)ultiple log"));
  // output->println(F("(o)ne-wire"));
  output->println(F("(p)aram"));
  output->println(F("(q)ualifier"));
  output->println(F("(r)eset"));
  output->println(F("(s)ettings"));
#ifdef EEPROM_DUMP
  output->println(F("(z) eeprom"));
#endif

#endif

#ifdef TEMP_PID_COLD
  output->println(F("(A) Cold temp"));
  output->println(F("(B) Hot temp"));
  output->println(F("(C) Sample temp"));
  output->println(F("(AA) Target temp (>=10C <80C)"));
  output->println(F("(AB) Max Temp"));
  output->println(F("(AC) Windows time 2000ms COOL 5000ms hot"));
  output->println(F("(AD) Min temp"));
  output->println(F("(AE) Max absolute temp"));
  output->println(F("(AF) Status 0=heating 1=cooling 2=ambient reg"));
  output->println(F("(AG) Approx ambient temp (user set)"));
#endif
}


static void printFreeMemory(Print* output)
{
  nilPrintUnusedStack(output);
}


void noThread(Print* output){
  output->println(F("No Thread"));
}


/* Fucntions to convert a number to hexadeciaml */

const char hex[] = "0123456789ABCDEF";

uint8_t toHex(Print* output, byte value) {
  output->print(hex[value>>4&15]);
  output->print(hex[value>>0&15]);
  return value;
}

uint8_t toHex(Print* output, int value) {
  byte checkDigit=toHex(output, (byte)(value>>8&255));
  checkDigit^=toHex(output, (byte)(value>>0&255));
  return checkDigit;
}

uint8_t toHex(Print* output, long value) {
  byte checkDigit=toHex(output, (int)(value>>16&65535));
  checkDigit^=toHex(output, (int)(value>>0&65535));
  return checkDigit;
}







