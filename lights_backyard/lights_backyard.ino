#include <SparkFunDS1307RTC.h>
#include <Wire.h>

#include <RFM69.h>
#include <SPI.h>
#include <LowPower.h>

// Addresses for this node. CHANGE THESE FOR EACH NODE!
#define NETWORKID     0   // Must be the same for all nodes
#define MYNODEID      99   // My node ID
#define TONODEID      1   // Destination node ID

// RFM69 frequency, uncomment the frequency of your module:
#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY     RF69_915MHZ

// AES encryption (or not):
#define ENCRYPT       true // Set to "true" to use encryption
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

// Use ACKnowledge when sending messages (or not):
#define USEACK        true // Request ACKs or not

// Comment out the line below if you want month printed before date.
// E.g. October 31, 2016: 10/31/16 vs. 31/10/16
#define PRINT_USA_DATE

#define SQW_INPUT_PIN 2   // Input pin to read SQW
#define SQW_OUTPUT_PIN 13 // LED to indicate SQW's state
#define RELAY_PIN 10

RFM69 radio;
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network


typedef struct {
  int           nodeId;  
  int           trigger;   //0 = off  1 = on 
  
} Payload;
Payload theData;

int relayStatus;

void setup() 
{
  // Use the serial monitor to view time/date output
  Serial.begin(9600);
  pinMode(SQW_INPUT_PIN, INPUT_PULLUP);
  pinMode(SQW_OUTPUT_PIN, OUTPUT);
  digitalWrite(SQW_OUTPUT_PIN, digitalRead(SQW_INPUT_PIN));
  pinMode(RELAY_PIN, OUTPUT);
  rtc.begin(); // Call rtc.begin() to initialize the library
  // (Optional) Sets the SQW output to a 1Hz square wave.
  // (Pull-up resistor is required to use the SQW pin.)
  //rtc.writeSQW(SQW_SQUARE_1);

  // Now set the time...
  // You can use the autoTime() function to set the RTC's clock and
  // date to the compiliers predefined time. (It'll be a few seconds
  // behind, but close!)
    
  //rtc.autoTime();
  
  // Or you can use the rtc.setTime(s, m, h, day, date, month, year)
  // function to explicitly set the time:
  // e.g. 7:32:16 | Monday October 31, 2016:
  //rtc.setTime(16, 25, 15, 7, 24, 12, 16);  // Uncomment to manually set time
  // rtc.set12Hour(); // Use rtc.set12Hour to set to 12-hour mode
  
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.encrypt(ENCRYPTKEY);

  digitalWrite(RELAY_PIN, HIGH); //on
  
}

void loop() 
{
  static int8_t lastSecond = -1;

  //Get the latest time from RTC Clock
  // Call rtc.update() to update all rtc.seconds(), rtc.minutes(),
  rtc.update();
  int hour = rtc.hour();
  int minute = rtc.minute();
  int second = rtc.second();
  
  if (rtc.second() != lastSecond) // If the second has changed
  {
    printTime(); // Print the new time
    lastSecond = rtc.second(); // Update lastSecond value

    //read relay switch
    relayStatus = digitalRead(RELAY_PIN);
    Serial.print("Status ");
    Serial.println(relayStatus);

    //1900 hours turn lights on
    //if(hour == 19 && minute == 00 && second == 10){
    if(hour > 17 && hour < 02 ){ 
      digitalWrite(RELAY_PIN, HIGH); //on
      Serial.println("Light ON");
      //delay(9000); // Wait 9 seconds
      //Serial.println("Light OFF");
      //digitalWrite(RELAY_PIN, LOW); //off
      theData.trigger = relayStatus;
      sendData();
      delay(1500);
    }  
    
    //Turn off at 6am
    //if(hour == 06 && minute == 00 && second == 10){
    if(hour > 02 && hour < 17){
      Serial.println("Light OFF");
      digitalWrite(RELAY_PIN, LOW); //off
      delay(1500);
    }
    
    if (radio.receiveDone()) // Got one!
    {
      radioReceive();
    }

    
  }
 
  
  
    // Read the state of the SQW pin and show it on the
    // pin 13 LED. (It should blink at 1Hz.)
    //digitalWrite(SQW_OUTPUT_PIN, digitalRead(SQW_INPUT_PIN));
}

void radioReceive(){

    int lightMode = 0;

    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    Serial.print(" [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");
    if (promiscuousMode)
    {
      Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");
    }

    if (radio.DATALEN != sizeof(Payload))
      Serial.print("Invalid payload received, not matching Payload struct!");
    else
    {
      theData = *(Payload*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
      Serial.print(" nodeId=");
      Serial.println(theData.nodeId);
      Serial.print(" trigger=");
      Serial.println(theData.trigger);
      lightMode = theData.trigger;
    }
    Serial.println(radio.RSSI);
     
    // Print out the information:
    /*Serial.print("received from node ");
    Serial.print(radio.SENDERID, DEC);
    Serial.print(", message [");

    // The actual message is contained in the DATA array,
    // and is DATALEN bytes in size:
    for (byte i = 0; i < radio.DATALEN; i++){
      lightMode = radio.DATA[0];
      Serial.print((char)radio.DATA[i]);
    }
    
    // RSSI is the "Receive Signal Strength Indicator",
    // smaller numbers mean higher power.
    Serial.print("], RSSI ");*/
    

    if(lightMode == 1){
      Serial.println("Light On by radio");
      digitalWrite(RELAY_PIN, HIGH); //on
    }else{
      Serial.println("Light OFF by radio");
      digitalWrite(RELAY_PIN, LOW); //off
    }

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.println("ACK sent");
    }
  
}

void sendData(){
 
      
      Serial.print("Sending to node ");
      Serial.println(TONODEID, DEC);
      
      if (radio.sendWithRetry(TONODEID, (const void*)(&theData), sizeof(theData)))
          Serial.println("ACK received!");
        else
          Serial.println("no ACK received");
      
      delay(100);
      
  
}

void printTime()
{
  Serial.print(String(rtc.hour()) + ":"); // Print hour
  if (rtc.minute() < 10)
    Serial.print('0'); // Print leading '0' for minute
  Serial.print(String(rtc.minute()) + ":"); // Print minute
  if (rtc.second() < 10)
    Serial.print('0'); // Print leading '0' for second
  Serial.print(String(rtc.second())); // Print second

  if (rtc.is12Hour()) // If we're in 12-hour mode
  {
    // Use rtc.pm() to read the AM/PM state of the hour
    if (rtc.pm()) Serial.print(" PM"); // Returns true if PM
    else Serial.print(" AM");
  }
  
  Serial.print(" | ");

  // Few options for printing the day, pick one:
  Serial.print(rtc.dayStr()); // Print day string
  //Serial.print(rtc.dayC()); // Print day character
  //Serial.print(rtc.day()); // Print day integer (1-7, Sun-Sat)
  Serial.print(" - ");
#ifdef PRINT_USA_DATE
  Serial.print(String(rtc.month()) + "/" +   // Print month
                 String(rtc.date()) + "/");  // Print date
#else
  Serial.print(String(rtc.date()) + "/" +    // (or) print date
                 String(rtc.month()) + "/"); // Print month
#endif
  Serial.println(String(rtc.year()));        // Print year
}
