/*
 * author: Carli DeCapito
 * program: create command prompt program
 * date: 2/27/18
 * 
 * mpu library and code adapted from: https://maker.pro/education/imu-interfacing-tutorial-get-started-with-arduino-and-the-mpu-6050-sensor
 * bmp library and code adapted from: https://learn.adafruit.com/adafruit-bmp280-barometric-pressure-plus-temperature-sensor-breakout/arduino-test
 *
**/
// LIBRARIES ///////////////////////////////////////////////////////
#include <Wire.h>
#include <math.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "DHT.h"


// MACROS //////////////////////////////////////////////////////////
#define CLK_SPD 16000000
#define RXE 0x80
#define TBE 0x20

//cursor 
#define CURSOR_MAX 200000
#define CURSOR_MIN 10000

//blink
#define BLINK_MAX 40000
#define BLINK_MIN 10000

//tone
#define TONE_MAX 20000
#define TONE_MIN 10000

#define DHTPIN 7 
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321 
//Global Variables /////////////////////////////////////////////////
// baud rate register
volatile unsigned int* myUBRR0 = (volatile unsigned int*) 0x00C4;

// USART0 control registers A, B, C
volatile unsigned char* myUCSR0A = (volatile unsigned char*) 0xC0;
volatile unsigned char* myUCSR0B = (volatile unsigned char*) 0xC1;
volatile unsigned char* myUCSR0C = (volatile unsigned char*) 0xC2;

// USART0 data register
volatile unsigned char* myUDR0 = (volatile unsigned char*) 0xC6;

//timer control reg
volatile unsigned char * myTCCR1A = ( unsigned char * ) 0x80;
volatile unsigned char * myTCCR1B = ( unsigned char * ) 0x81;
volatile unsigned char * myTCCR1C = ( unsigned char * ) 0x82;
volatile unsigned char * myTIMSK1 = ( unsigned char * ) 0x6F;
volatile unsigned int * myTCNT1 =   ( unsigned int * ) 0x84;
volatile unsigned char * myTIFR1 =  ( unsigned char * ) 0x36;

//port B control regs
volatile unsigned char * myDDRB =   ( unsigned char * ) 0x24;
volatile unsigned char * myPortB =   ( unsigned char * ) 0x25;

//MPU 
MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;

//BMP280 
Adafruit_BMP280 bmp; // I2C

//DHT
// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
#define OUTPUT_READABLE_ACCELGYRO

//function used global variables
//cursor
long long int cursorSpeed = 80000;
long long int cursorCount = 80000;
unsigned int cursorTracker = 0;
bool cursorFlag = 0;

//blink LED
long long int blinkFreq = 15000;
long long int blinkCount = 15000;
bool blinkFlag = 0;

//soundtone
long long int soundFreq = 15000;
long long int soundCount = 15000;
bool soundFlag = 0;

//bmp 
long long int bmpFreq = 100000;
long long int bmpCount = 5000;
bool bmpFlag = 0;

//mpu 
long long int mpuFreq = 100000;
long long int mpuCount = 5000;
bool mpuFlag = 0;

//dht
long long int dhtFreq = 100000;
long long int dhtCount = 5000;
bool dhtFlag = 0;

///////////////////////////////////////////////////////////////////
//setup ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void setup() {
  //set timer to normal mode
  *myTCCR1A = 0;
  *myTCCR1B = 0;
  *myTCCR1C = 0;
  *myTIMSK1 = 0x01;

  //set DDRB to output on pin 6
  *myDDRB |= 0b01000000;
  
  //set LED to output
  pinMode( LED_BUILTIN, OUTPUT );
  
  // join I2C bus (I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif

  crsoff();
  blinkoff();

  //initalize uart
  U0_Init( 9600 );
  
  putLine( "Welcome to Carli's Arduino Mega Command Processor..." );
}

///////////////////////////////////////////////////////////////////
//Loop ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void loop() {
  //main menu output
  putLine( "Please enter the desired function:" );
  putLine( "" );
  putLine( "\t1. Keyboard echo" );
  putLine( "\t2. ASCII character echo" );
  putLine( "\t3. Change the rotating cursor speed" );
  putLine( "\t4. Blink LED" );
  putLine( "\t5. Sound tone" );
  putLine( "\t6. Output MPU-6050 Data" );
  putLine( "\t7. Output BMP280 Data " );
  putLine( "\t8. Output DHT Data" );
  putLine( "" );
  putString( "Select a Command >" );

  //turn cursor on while waiting
  crson();
  
  while( true )
  {
    //check if input is available
    if( kbhit() )
    {
      //if it is then send to cmdproc
      cmdproc( *myUDR0 );
      break;
    }
    else
    {
      //otherwise rotate cursor
      syswrk();
    }
  }
}
///////////////////////////////////////////////////////////////////
//Command Functionality ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////
//once command has been input, call corresponding functionality
bool cmdproc( const char input )
{
  putChar( '\b' );
  putChar( '>' );

  //turn cursor off
  crsoff();
  putLine( "" );
  switch( input )
  {
    case '1':
      keyboardEcho();
      break;
    case '2':
      ASCIIEcho();
      break;
    case '3':
      CrsSpeedChange();
      break;
    case '4':
      BlinkSpeedChange();
      break;
    case '5':
      ToneSpeedChange();
      break;
    case '6':
      mpuOutput();
      break;
    case '7':
      bmpOutput();
      break;
    case '8':
      dhtOutput();
      break;
    default:
      putLine( "Invalid entry, please try again." );
  }
}


///////////////////////////////////////////////////////////////////
//Cursor Helper Functions /////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void crson()
{
  //to rewrite over cursor
  putChar( '\b' );
  //restart
  putChar( '|' );
  //set flag to 1
  cursorFlag = 1;
}

//turn off cursor flag
void crsoff()
{
  cursorFlag = 0;
}

//system work in background
void syswrk()
{
  cursorCount--;
  crsrot();

  blinkCount--;
  blinkLED();

  if( bmpFlag == 1 )
  {
    bmpCount--;
    bmpHelper();
  }

  if( mpuFlag == 1 )
  {
    mpuCount--;
    mpuHelper();
  } 

  if( dhtFlag == 1 )
  {
    dhtCount--;
    dhtHelper();
  }
}

//changes symbol of cursor, to appear like its rotating
void crsrot()
{
  if( cursorCount <= 0 )
  {
    putChar( '\b' );
    switch (cursorTracker)
    {
    case 0:
      putChar( '/' );
      break;
    case 1:
      putChar( '-' );
      break;
    case 2:
      putChar( '\\' );
      break;
    case 3:
      putChar( '|' );
      break;
    }

    cursorTracker++;
    cursorTracker %= 4;

    cursorCount = cursorSpeed;
  }
}


///////////////////////////////////////////////////////////////////
//Blink Helper Functions //////////////////////////////////////////
///////////////////////////////////////////////////////////////////
//turns  blink flag on
void blinkon()
{
  blinkFlag = 1;
}

//turns blink flag off
void blinkoff()
{
  blinkFlag = 0;

}

//blinks LED by checking count
void blinkLED()
{
  if( blinkCount <= 0 )
  {
      //invert bit at LED
       digitalWrite( LED_BUILTIN, !( digitalRead( LED_BUILTIN ) ) );
       //change frequency
       blinkCount = blinkFreq;
  }
}

///////////////////////////////////////////////////////////////////
//KEYBOARD ECHO ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void keyboardEcho()
{
  char input = 0;
  putLine( "Keyboard Echo:" );
  putLine( "Enter any character and terminal will echo that character. " );
  putLine( "" );
  putChar( ' ' );
  crson();
  
  do{
    if( kbhit() )
    {
      input = getChar();
      if( input != 27 )
      {
        putChar( '\b' );
        putChar( input );
        putLine( "" );
        putChar( ' ' );
      }
    }
    else
    {
      syswrk();
    }
  }while( input != 27 );
  crsoff();
}

///////////////////////////////////////////////////////////////////
//ASCII ECHO //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void ASCIIEcho()
{
  char buffer[4];
  char input = 0;
  putLine( "Echo ASCII character:" );
  putLine( "Enter any character and the ASCII hexadecimal equivalent will be echoed." );
  putLine( "" );
  putChar( ' ' );
  crson();
  do{
    if( kbhit() )
    {
      input = getChar();
      if( input != 27 )
      {
        putChar( '\b' );
        putChar( input );
        putChar( ' ' );
        putChar( '=' );
        putChar( ' ' );
        putChar( '0' );
        putChar( 'x' );
        putLine( itoa( input, buffer, 16 ) );
        putChar( ' ');
      }
    }
    else
    {
      syswrk();
    }
  } while ( input != 27 );
  crsoff();
}


///////////////////////////////////////////////////////////////////
//CURSOR SPEED CHANGE /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void CrsSpeedChange()
{
  putLine( "Change cursor speed:" );
  putLine( "Enter a '+' to increase cursor speed, and a '-' to decrease cursor speed" );
  putLine( "" );
  putChar( ' ' );

  crson();
  
  char input = 0;
  do {
    if( kbhit() )
    {
      input = getChar();
      switch( input )
      {
        case '+':
          cursorSpeed -= 10000;
          if( cursorSpeed <= CURSOR_MIN )
          {
            cursorSpeed = CURSOR_MIN;
            putChar( '\b' );
            putLine( "Max cursor speed reached" );
            putChar( ' ' );
          }
          else
          {
            putChar( '\b' );
            putLine( "Increasing cursor speed" );
            putChar( ' ' ); 
          }
          break;
        case '-':
          cursorSpeed += 10000;
          if( cursorSpeed >= CURSOR_MAX )
          {
            cursorSpeed = CURSOR_MAX;
            putChar( '\b' );
            putLine( "Min cursor speed reached" );
            putChar( ' ' );
          }
          else
          {
            putChar( '\b' );
            putLine( "Decreasing cursor speed" );
            putChar( ' ' ); 
          }
          break;
        case 27:
          break;
        default:
          putLine( "Invalid input... Please try again." );
          putChar( '\b' );
          putChar( ' ' );
      }
    }
    else
    {
      syswrk();
    }
  }while( input != 27 );
  crsoff();
}



///////////////////////////////////////////////////////////////////
//BLINK LED ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void BlinkSpeedChange()
{
  putLine( "Blink LED:" );
  putLine( "Enter a '+' to increase blinking frequency, and a '-' to decrease blinking frequency" );
  putLine( "" );
  putChar( ' ' );

  crson();            
  char input = 0;
  do {
    if( kbhit() )
    {
      input = getChar();
      switch( input )
      {
        case '+':
          blinkFreq -= 2000;
          if( blinkFreq <= BLINK_MIN )
          {
            blinkFreq = BLINK_MIN;
            putChar( '\b' );
            putLine( "Max blinking frequency reached" );
            putChar( ' ' );
          }
          else
          {
            putChar( '\b' );
            putLine( "Increasing blinking frequency" );
            putChar( ' ' );
          }
          break;
        case '-':
          blinkFreq += 2000;
          if( blinkFreq >= BLINK_MAX )
          {
            blinkFreq = BLINK_MAX;
            putLine( "Min blinking frequency reached" );
            putChar( '\b' );
            putChar( ' ' );
          }
           else
          {
            putChar( '\b' );
            putLine( "Decreasing blinking frequency" );
            putChar( ' ' );
          }
          break;
        case 27:
          break;
        default:
          putLine( "Invalid input... Please try again." );
          putChar( '\b' );
          putChar( ' ' );
      }
    }
    else
    {
     syswrk();
    }
    
  }while( input != 27 );
  crsoff();
}


///////////////////////////////////////////////////////////////////
//SOUND TONE //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void ToneSpeedChange()
{
  putLine( "Change sound tone:" );
  putLine( "Enter a '+' to increase frequency, and a '-' to decrease frequency" );
  putLine( "" );
  putChar( ' ' );

  crson();

  //turn on timer with prescalar of 1
  *myTCCR1B |= 0x01;

  char input = 0;
  do {
    if( kbhit() )
    {
      input = getChar();
      switch( input )
      {
        case '+':
          soundFreq += 1000;
          if( soundFreq >= TONE_MAX )
          {
            soundFreq = TONE_MAX;
            putChar( '\b' );
            putLine( "Min frequency reached" );
            putChar( ' ' );
          }
          else
          {
            putChar( '\b' );
            putLine( "Decreasing Frequency" );
            putChar( ' ' );
          }
          break;
        case '-':
            soundFreq -= 1000;
            if( soundFreq <= TONE_MIN )
            {
                soundFreq = TONE_MIN;
                putLine( "Max frequency reached" );
                putChar( '\b' );
                putChar( ' ' );
            }
          else
          {
            putChar( '\b' );
            putLine( "Increasing Frequency" );
            putChar( ' ' );
          }
          break;
        case 27:
          break;
        default:
          putLine( "Invalid input... Please try again." );
          putChar( '\b' );
          putChar( ' ' );
      }
    }
    else
    {
      syswrk();
    }      
  }while( input != 27 );
  *myTCCR1B &= 0xF8;
  crsoff();
}

//interrupt driven timer
ISR( TIMER1_OVF_vect )
{
    //turn off timer
    *myTCCR1B &= 0xF8;

    //toggle
    *myPortB ^= 0x40;
    
    //load counter
    *myTCNT1 = ( unsigned int ) ( 65536 - ( long ) soundFreq );

    //turn on timer by setting prescalar to 1
    *myTCCR1B |= 0x01;
}


///////////////////////////////////////////////////////////////////
//UART Functionality  /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
//initalizes baud rate
void U0_Init( unsigned int baudRate )
{
  // initalize baud rate
  unsigned int BaudRate = (unsigned int) (CLK_SPD/16/baudRate - 1);
  *myUBRR0 = BaudRate;

  // Enable receiver and transmitter
  // Disable receiver and transmitter complete interrupt
  // Disable data register empty interrupt
  *myUCSR0B = 0x18;

  // Set to asynchronous mode, Disable parity mode, 1 stop bit, 8-bit data size
  *myUCSR0C = 0x06;
}

//check if data is available
unsigned int kbhit()
{
  // returns 0 if receive buffer is empty, int otherwise 
  return *myUCSR0A & RXE;
}

//get data from buffer
unsigned char getChar()
{
  // Wait recieve buffer to be done receiving
  while( !( *myUCSR0A & RXE ) );
  //return char
  return *myUDR0;
}

//output ccharacter
void putChar( const char output )
{
  // wait for USART data register empty
  while( ( *myUCSR0A & TBE ) == 0x00 );
  //save output to data reg
  *myUDR0 = output;
}

//output line
void putLine( const char output[] )
{  
  putString( output );
  putChar( '\n' );
  putChar( '\r' );
}

//output string
void putString( const char output[] )
{
  for( int i = 0; output[i] != '\0'; i++ )
  {
    putChar( output[i] );
  }
}


///////////////////////////////////////////////////////////////////
//I2C Functionality  //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

//mpu /////////////////////////////////////////////////////////////
void mpuOutput()
{
  putLine(("MPU6050 Output"));
  
  // initialize device
  putLine("Initializing MPU6050");
  accelgyro.initialize();

  // verify connection
  putLine("Testing MPU6050 connection...");
  putLine(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  char input = 0;

  mpuOn();
  crson();
  do{
    if( kbhit() )
    {
      input = getChar();
    }
    else
    {
      syswrk(); 
    }
  } while ( input != 27 );
  crsoff();
  mpuOff();
}

void mpuHelper()
{
  char buffer[10];
  if( mpuCount <= 0 )
  {
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    #ifdef OUTPUT_READABLE_ACCELGYRO
    // display tab-separated accel/gyro x/y/z values
    Serial.print("ax: ");
    Serial.print(ax); Serial.print("\t");
    Serial.print("ay: ");
    Serial.print(ay); Serial.print("\t");
    Serial.print("az: ");
    Serial.print(az); Serial.print("\t");
    Serial.print("gx: ");
    Serial.print(gx); Serial.print("\t");
    Serial.print("gy: ");
    Serial.print(gy); Serial.print("\t");
    Serial.print("gz: ");
    Serial.println(gz);
    Serial.println(" ");
    #endif    
    mpuCount = mpuFreq;
  }
}

void mpuOn()
{
  mpuFlag = 1;  
}

void mpuOff()
{
  mpuFlag = 0;  
}

//bmp //////////////////////////////////////////////////////////
void bmpOutput()
{
  putLine(("BMP280 Output"));
  char input = 0;
  delay( 200 ); 
  if (!bmp.begin(0x76)) {  
    putLine(("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }  
  bmpOn();
  crson();
  do{
    if( kbhit() )
    {
      input = getChar();
    }
    else
    {
      syswrk(); 
    }

  } while ( input != 27 );
  crsoff();
  bmpOff(); 
}

void bmpHelper()
{
  char stringFloat[20];
  
  if( bmpCount <= 0 )
  {
    putString(("Temperature = "));
    ftoa( bmp.readTemperature(), stringFloat, 2  ); 
    putString(stringFloat);
    putString(" *C");
    putLine(" ");

    putString(("Pressure = "));
    ftoa(bmp.readPressure(), stringFloat, 2  );
    putString( stringFloat );
    putString(" Pa");
    putLine(" ");

    
    putString(("Approx altitude = "));
    ftoa( bmp.readAltitude(1013.25), stringFloat, 2 );
    putString( stringFloat ); //set to avg sea level pa
    putString(" m");

    putLine(" ");
    putLine(" ");

    bmpCount = bmpFreq;
  }
}

void bmpOn()
{
  bmpFlag = 1;
}

void bmpOff()
{
  bmpFlag = 0;
}

//dht ///////////////////////////////////////////////////////////
void dhtOutput()
{
  putLine(("DHT22 Output"));
  
  // initialize device
  putLine("Initializing DHT22");
  dht.begin();
  
  char input = 0;
  dhtOn();
  crson();
  do{
    if( kbhit() )
    {
      input = getChar();
    }
    else
    {
      syswrk(); 
    }
  } while ( input != 27 );
  crsoff();
  dhtOff();  
}

void dhtHelper()
{
  if( dhtCount <= 0 )
  {
      // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");

  Serial.println(" ");
    dhtCount = dhtFreq; 
  }
}

void dhtOn()
{
  dhtFlag = 1;
}

void dhtOff()
{
  dhtFlag = 0;
}

///////////////////////////////////////////////////////////////////
//Helper Functions  ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
 
    reverse(str, i);
    str[i] = '\0';
    return i;
}
 
void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
 
    // Extract floating part
    float fpart = n - (float)ipart;
 
    // convert integer part to string
    int i = intToStr(ipart, res, 0);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}
 





