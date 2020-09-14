/* 
 * ATmega8A HC-05 Bluetooth programmer/communication module
 * Author: Nikola Cvetkovic
 * Date: 7.2020.
 *
 * Program uses ATmega8A mcu as the controlling device for
 * manipulating with the HC-05 pins, thus detecting the 
 * intent of the user for HC-05 module to be used as the
 * Bluetooth Arduino Uno R3 Programmer, as the standard
 * Bluetooth Communicating device, or in the AT command mode.
 *  
 * The board should support switches, jumpers or some kind 
 * of control for selecting the AT/non-AT mode as well as 
 * Programming/Communicating mode.
 *  
 * ATmega8A is programmed using the Arduino Bootloader,
 * since its sole purpose is to manage simple data transfer.
 */
 
/*
 * Pin number defines:
 */
#define KEY34 4
#define RESETBT 5
#define EN_AB 7
#define EN_RST 8
#define EN_MB 9
#define SavingLED 14
#define BusyLED 15
#define CP 16
#define SW 17
/*
 * Defines for delays and lengths:
 */
#define STARTDELAY 1000
#define BUSYDELAY 300
#define SAVEDELAY 800
#define DEBBDELAY 30
#define DESIGNATOR 7
#define INFOLENGTH 30
/*
 * Command IDs which define command to be sent
 * to the BT module. They are used in CheckChar():
 */
#define UARTID 1
#define NAMEID 2
#define PSWDID 3
#define ROLEID 4
/*
 * Previous switch state, used for debouncing:
 */
int SWold = 0;
/*
 * Character arrays used to save previous BT module
 * configuration:
 */
char prevUART[INFOLENGTH];
char prevNAME[INFOLENGTH];
char prevPSWD[INFOLENGTH];
char prevROLE[INFOLENGTH];
/*
 * Set the pin directions and save the previous module
 * state since the device has either now been powered on
 * or has entered non-AT mode from AT mode:
 */
void setup() {
    
    // Define pin directions:
    pinMode(KEY34, OUTPUT);
    pinMode(RESETBT, OUTPUT);
    pinMode(SavingLED, OUTPUT);
    pinMode(BusyLED, OUTPUT);
    pinMode(EN_AB, OUTPUT);
    pinMode(EN_RST, OUTPUT);
    pinMode(EN_MB, OUTPUT);
    pinMode(CP, INPUT);
    pinMode(SW, INPUT);
  
    // Disable LEDs:
    digitalWrite(BusyLED, HIGH);
    digitalWrite(SavingLED, HIGH);
  
    // Begin serial communication for AT:
    Serial.begin(38400);
    
    // Enter AT command mode:
    digitalWrite(KEY34, HIGH);
    digitalWrite(RESETBT, LOW);
    delay(DEBBDELAY);
    digitalWrite(RESETBT, HIGH);
    delay(STARTDELAY);  
    
    // Save previous BT module configuration:
    SavePrevious();
    // Check which mode of operation is defined:
    CheckMode();
}
/*
 * When dedicated switch is pressed, reevaluation
 * of operation mode is performed, which allows the
 * user to enter Programming or Communication mode 
 * at the specific moment in time, avoiding accidental
 * mode switches.
 */
void loop() {
    // Perform hardware debounce on switch:
    if (digitalRead(SW) == LOW) {
      if (SWold != 0) {
        delay(DEBBDELAY);
        if (digitalRead(SW) == LOW) CheckMode();
      } 
    }
    if (digitalRead(SW) == LOW) SWold = 0;
    else SWold = 1;
}
/*
 * Function checks for a mode of operation:
 * either Programming or Communication mode
 * can be selected.
 */
void CheckMode() {
  
    // Signal that the module is busy:
    digitalWrite(BusyLED, LOW);
    // EN_ABle ATmega8A Rx and Tx signals:
    digitalWrite(EN_MB, HIGH);
    // Open Arduino Rx and Tx so that these pins can
    // be freely used by Arduino UART:
    digitalWrite(EN_AB, LOW);
    digitalWrite(EN_RST, LOW);
    
    // Enter AT command mode:
    digitalWrite(KEY34, HIGH);
    digitalWrite(RESETBT, LOW);
    delay(DEBBDELAY);
    digitalWrite(RESETBT, HIGH);
    
    // Reset the module to default:
    Serial.write("AT+ORGL\r\n");
    delay(2*BUSYDELAY);
    
    // Determine if the communication
    // or Programming mode is selected:
    if (digitalRead(CP) == LOW) {
      ProgrammingMode();
    }
    else {
      CommunicationMode();
    }
    
    // Before reinitializing, clear KEY34:
    Serial.write("AT+INIT\r\n");
    delay(BUSYDELAY);
    digitalWrite(KEY34, LOW);
    digitalWrite(RESETBT, LOW);
    delay(DEBBDELAY);
    digitalWrite(RESETBT, HIGH);

    // Disable ATmega8A Rx and Tx signals:
    digitalWrite(EN_MB, LOW);        
    // Disable busy LED:
    digitalWrite(BusyLED, HIGH);
}
/* 
 *  Function saves previously configured state of
 *  the BT module and then sets UART baud rate,
 *  module name, password and role so that the 
 *  programming of the Arduino can begin.
 */
void ProgrammingMode() {
    
    // Set UART speed for programming:
    Serial.write("AT+UART=115200,0,0\r\n");
    delay(BUSYDELAY);
    // Set the name of the programmer so it is
    // easily recognized by the PC or Android:
    Serial.write("AT+NAME=BT_Programmer\r\n");
    delay(BUSYDELAY);
    // Set the role of the module to slave:
    Serial.write("AT+ROLE=0\r\n");
    delay(BUSYDELAY);
    // Set the password:
    Serial.write("AT+PSWD=1234\r\n");
    delay(BUSYDELAY);
    
    // Short Arduino Rx and Tx with Board Rx and Tx
    // and also allow BT module to reset Arduino when
    // needed
    digitalWrite(EN_AB, HIGH);
    digitalWrite(EN_RST, HIGH);
}

/*
 * Function configures the module according to the 
 * configuration obtained upon powering the system on,
 * reseting, or returning from AT command mode.
 */
void CommunicationMode () {

    // Set UART speed as it was before starting the module:
    Serial.write("AT+UART=");
    Serial.write(prevUART);
    Serial.write("\r\n");
    delay(BUSYDELAY);
    
    // Set the name of the programmer:
    Serial.write("AT+NAME=");
    Serial.write(prevNAME);
    Serial.write("\r\n");
    delay(BUSYDELAY);

    // Set the password:
    Serial.write("AT+PSWD=");
    Serial.write(prevPSWD);
    Serial.write("\r\n");
    delay(BUSYDELAY);
    
    // Set the role of the module:
    Serial.write("AT+ROLE=");
    Serial.write(prevROLE);
    Serial.write("\r\n");
    delay(BUSYDELAY);
    
    // Open Arduino Rx and Tx so that these pins can
    // be freely used by Arduino UART:
    digitalWrite(EN_AB, LOW);
    digitalWrite(EN_RST, LOW);
}

/*
 * Function saves previous BT module configuration
 * before allowing the user to change it.
 */
void SavePrevious () {

    // Turn on SavingLED:
    digitalWrite(SavingLED, LOW);
    // EN_ABle ATmega8A Rx and Tx signals:
    digitalWrite(EN_MB, HIGH);

    delay(SAVEDELAY);
    // Request UART speed for communicating:
    Serial.write("AT+UART?\r\n");
    // Read BT module answer and save the baud rate:
    ReadAnswer(UARTID);
    
    // Request module name:
    Serial.write("AT+NAME?\r\n");
    delay(4*BUSYDELAY);
    // Read BT module answer and save the name:
    ReadAnswer(NAMEID);
  
    // Request password for pairing:
    Serial.write("AT+PSWD?\r\n");
    // Read BT module answer and save the password:
    ReadAnswer(PSWDID);
    
    // Request module role (master/slave):
    Serial.write("AT+ROLE?\r\n");
    // Read BT module answer and save the role:
    ReadAnswer(ROLEID);

    // Exit AT mode:
    digitalWrite(KEY34, LOW);
    digitalWrite(RESETBT, LOW);
    delay(DEBBDELAY);
    digitalWrite(RESETBT, HIGH);
    // Disable ATmega8A Rx and Tx signals:
    digitalWrite(EN_MB, LOW);
    // Turn off SavingLED:
    digitalWrite(SavingLED, HIGH);
}

/*
 * Function reads BT module answer and calls 
 * CheckChar function so that the information
 * can be extracted.
 */
void ReadAnswer(int CommandID) {
  
    // String which will keep received value:
    char ReceivedString[INFOLENGTH];
    delay(SAVEDELAY);
    // Read until Rx buffer is empty: 
    int i = 0;
    boolean Read = false;
    while(Read == false) {
      if (Serial.available() > 0) {
        while (Serial.available() > 0){
          ReceivedString[i] = Serial.read();
          i++;
        }
        ReceivedString[i] = '\0';
        Read = true;
      }
    }
    
    // If something is read, extract information from it:
    if (ReceivedString[0] != 0) 
      CheckChar(ReceivedString, CommandID);
}

/*
 * Function extracts information about UART baud rate,
 * module name, password or role, depending on which
 * parameters are sent to it. The first argument should be
 * module answer to the information inquiry, and second which
 * information should be extracted.
 */
void CheckChar(char Answer[], int CommandID) {
  
    byte i = 0;
    byte m = 0;
    char UsefulInfo[INFOLENGTH];
    
    // Useful information is separated with a colon:
    for (i = 0; i < DESIGNATOR; i++) {
      if (Answer[i] == ':') break;
    }
  
    // After the colon, information is sent:
    for (m = 0; m < INFOLENGTH; m++){
      if (Answer[i+m+1] != '\r'){
        UsefulInfo[m] = Answer[i+m+1];
      }
      else break;
    }
    // Signal the end of a character:
    UsefulInfo[m] = '\0';
  
    // Depending on the command ID, save extracted
    // information appropriately:
    switch(CommandID) {
      case UARTID:
        i = 0;
        while (UsefulInfo[i] != '\0') {
          prevUART[i] = UsefulInfo[i];
          i++;
        }
        prevUART[i] = '\0';
        break;
      case NAMEID:
        i = 0;
        while (UsefulInfo[i] != '\0') {
          prevNAME[i] = UsefulInfo[i];
          i++;
        }
        prevNAME[i] = '\0';
        break;
      case PSWDID:
        i = 0;
        while (UsefulInfo[i] != '\0') {
          prevPSWD[i] = UsefulInfo[i];
          i++;
        }
        prevPSWD[i] = '\0';
        break;
      case ROLEID:
        i = 0;
        while (UsefulInfo[i] != '\0') {
          prevROLE[i] = UsefulInfo[i];
          i++;
        }
        prevROLE[i] = '\0';
        break;
    }
}
