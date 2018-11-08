#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
// OLED display TWI address
#define OLED_ADDR   0x3C
#define D5 5
#define D6 6

unsigned int frqIn;
unsigned int Acc;
signed char accTopTemp=0;
signed char Mask=0;
float test;
byte amp;
byte arpMode=0;
byte sinVal;
byte noteIndex=0;
byte chordIndex=0;
long checktime=0;
long checkButtonW;
long checkButtonA;

int octRange=3;
int tempo=100;
unsigned note=0;
unsigned temp=0;
byte oct=3;
unsigned outFreq=0;
byte incomingByte;
byte waveforms = 1; //1,2,3,4 sq,saw,tria,sin
//use timer T2 do create a PWM sampled signal that will be filtered out to become an analog signal
const uint16_t scale[] = {857,908,962,1020,1080,1144,1212,1285,1361,1442,1528,1618,1715,1816,1924,2040,2160,2288,2424,2570,2722,2884,3056,3236,3430,3632};
const float oct_multi[] = {0.125,0.25,0.5,1,2,4};
const int majorChords[] = {12,7,12,0,4}; //semitone sequence 0+4+7
static const uint8_t sin_table[] PROGMEM =
{127,130,133,136,140,143,146,149,152,155,158,161,164,167,170,173,
176,179,182,185,188,190,193,196,198,201,204,206,208,211,213,216,
218,220,222,224,226,228,230,232,234,235,237,239,240,242,243,244,
245,247,248,249,250,251,251,252,253,253,254,254,254,255,255,255,
255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,
245,244,242,241,239,238,236,235,233,231,229,227,225,223,221,219,
217,214,212,210,207,205,202,200,197,194,192,189,186,183,180,178,
175,172,169,166,163,160,157,154,151,147,144,141,138,135,132,129,
125,127,127,127,127,127,127,127};
//starting display
Adafruit_SSD1306 display(-1);

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode (A0,INPUT); //frequency analog potentiometer
  pinMode(D5,INPUT_PULLUP);  //waveform change button
  pinMode(D6,INPUT_PULLUP);  //amplitude change button

/*
///////////////////////////////////////////////////////////////////////////////
*/

  DDRD = DDRD | 1<<DDD3; // PD3 (Arduino D3) as output (OC2B timer output)
  DDRB = DDRB | 1<<DDB5; //pin B5 (pin13) output

  //use one timer T1 to select a sampling frequency @20Khz and trigger an interrupt
  TCCR1A = 0<<COM1A0 | 0<<COM1B0 | 0<<WGM10 | 0<<WGM11; // CTC TOP=OCR1A
  OCR1A = 99;
  TCCR1B = 1<<WGM12 | 2<<CS10;  //010 prescaler 1:8 (16Mhz/8)/99+1=20Khz (101 5 prescaler1024)
  TIMSK1 =(1<<OCIE1A);

  TCCR2A = 0<<COM2A0 | 2<<COM2B0 | 3<<WGM20; // fastPWM on OC2B (pin3)
  OCR2A = 200;
  OCR2B = 100;
  TCCR2B = 1<<WGM22 | 1<<CS20;  //prescaler 1:1
  Serial.println("Welcome to FunGen");
  //display write
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

}

ISR(TIMER1_COMPA_vect) {
  temp=frqIn;
  Acc=Acc+temp;
  accTopTemp=Acc>>8;
  Mask=accTopTemp>>7;
  switch (waveforms) {
    case (1):
      OCR2B=(Acc>>8)&0x80;        //soft square top@0x80
    break;
    case (2):
      OCR2B=Acc>>8;               //sawtooth top@FF
    break;
    case (3):
      OCR2B=accTopTemp^Mask;      //soft triangle wave top@0x80
      OCR2B=OCR2B<<1;
    break;
    case (4):                     //sin wave (table) top@0xFF
      sinVal=pgm_read_byte(&sin_table[accTopTemp^Mask]);
      //test=sin(Acc>8); //uncomment this to understand the weight of floating point math @20Khz
      OCR2B=sinVal^Mask;
    break;
    case (5):                     //|sin| (double frequency) top@0xFF
      sinVal=pgm_read_byte(&sin_table[accTopTemp^Mask]);
      OCR2B=sinVal;
    break;
    case (6):                     //square top@FF
      OCR2B=Mask;
    break;
    }

  //Serial.println("X     "+String(Acc>>8));
  //Serial.println("OCR2B "+String(OCR2B)); //test only this line with serial plotter to check waveform
  //Serial.println(accTopTemp,BIN);
  //Serial.println(Mask,BIN);
}

void getHwInput () {
    //get input from freq potentiometer
    frqIn=analogRead(A0)*15;
    if (frqIn>0) outFreq=frqIn*0.3052;
    //Serial.print(outFreq);
    //outFreq=analogRead(A0);
    //get input from amp+ button
    //missing anti-rimbalzo
    if (!digitalRead(D5)){
    (amp<4)?amp++:amp=0;
    }

  //get input from waveform button (or )
    if (!digitalRead(D6)){
      if (waveforms<4)
        waveforms++;
      else
        waveforms=1;
    }
}


void loop() {
  if((millis()-checktime)>tempo)
      {
        getHwInput();
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(5,2);
        display.print("Wave: ");
        switch (waveforms) {
          case(1): display.print("SQUARE");;
          break;
          case(2): display.print("SAWTOOTH");
          break;
          case(3): display.print("TRIANGULAR");
          break;
          case(4): display.print("SIN");
          break;
          }
        display.setCursor(5,30);
        display.setTextSize(2);
        display.print("f:"+String(outFreq));
        display.display();
        checktime=millis();
      }
   }
