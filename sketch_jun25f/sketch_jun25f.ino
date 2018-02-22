#include <NMEAGPS.h>


#include <NeoSWSerial.h>
NeoSWSerial A7board( 10, 11 ); // 2nd best on whatever two pins you want.  Almost as efficient.
// DO NOT USE SoftwareSerial!

NMEAGPS gps;
gps_fix fix;

enum state_t { WAITING, READING_GPS, GPS_ON_WAIT, GPS_READING, GPS_READING_WAIT, GPS_OFF_WAIT, SENDING_SMS }; // other states?
state_t state = WAITING;
uint32_t stateTime;
const uint32_t CHECK_LOCATION_TIME = 5000; // ms

void echoA7chars()
{
  while (A7board.available())
    Serial.write( A7board.read() ); // just echo what we are receiving
}

void ignoreA7chars()
{
  while (A7board.available())
    A7board.read(); // ignore what we are receiving
}

void setup()
{
  Serial.begin( 115200 );
  A7board.begin( 9600 );
}

void loop()
{
  switch (state) {

    case WAITING:
      echoA7chars();

      if (millis() - stateTime >= CHECK_LOCATION_TIME) {

        // Turn GPS data on
        A7board.println( F("AT+GPS=1") );
        stateTime = millis();
        state     = GPS_ON_WAIT;
      }
      break;

    case GPS_ON_WAIT:
      echoA7chars();

      // Wait for the GPS to turn on and acquire a fix
      if (millis() - stateTime >= 5000) { // 5 seconds

        //  Request GPS data
        A7board.println( F("AT+GPSRD=1") );
        Serial.print( F("Waiting for GPS fix...") );

        stateTime = millis();
        state     = GPS_READING;
      }
      break;

    case GPS_READING:
      while (gps.available( A7board )) { // parse the NMEA data
        fix = gps.read(); // this structure now contains all the GPS fields

        if (fix.valid.location) {
          Serial.println();

          // Now that we have a fix, turn GPS mode off
          A7board.println(  F("AT+GPSRD=0") );

          stateTime = millis();
          state     = GPS_READING_WAIT;
        }
      }

      if (millis() - stateTime > 1000) {
        Serial.print( '.' ); // if still waiting for fix, print a dot.
        stateTime = millis();
      }
      break;

    case GPS_READING_WAIT:
      ignoreA7chars();

      // Wait for the GPS data to stop
      if (millis() - stateTime >= 1000) {

        // Turn GPS power off
        A7board.println(  F("AT+GPS=0") );

        stateTime = millis();
        state     = GPS_OFF_WAIT;
      }
      break;


    case GPS_OFF_WAIT:
      echoA7chars();

      // Wait for the GPS to power off
      if (millis() - stateTime >= 1000) {

        // Show the location we will send via SMS
        Serial.print( F("loc: ") );
        Serial.print( fix.latitude(), 6 ); // use the latitude field of the fix structure
        Serial.print( F(", ") );
        Serial.println( fix.longitude(), 6 ); // use the longitude field of the fix structure

        // Send SMS with location values ?
        //A7board.print( SMS commands? );
        //A7board.print( F("lat=") );
        //A7board.print( fix.latitude(), 6 ); // use the latitude field of the fix structure
        //  ...

        stateTime = millis();
        state     = SENDING_SMS;
      }
      break;
      
    case SENDING_SMS:
      echoA7chars();

      if (millis() - stateTime >= 2000) {
        stateTime = millis();
        state     = WAITING; // start over
      }
      break;

    //  ... other states ...
  }
}
