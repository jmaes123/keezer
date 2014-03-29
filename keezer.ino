#include <EEPROM.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <LiquidCrystal.h>
#include <LCDKeypad.h>

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);

#define AMBIENT_SENSOR_BUS 13
OneWire ambientSensors(AMBIENT_SENSOR_BUS);

DallasTemperature tempSensors(&oneWire);
DallasTemperature externalSensors(&ambientSensors);

#define NUM_INTERNAL_SENSORS 2
DeviceAddress internalThermometers[NUM_INTERNAL_SENSORS];
volatile float internalTemperatures[NUM_INTERNAL_SENSORS];
float setPoint = 2;
volatile unsigned long timeOfLastFreezerStateChange = 0;

#define ON 1
#define OFF 0

#define FREEZER_OFF_ICON 'X'
#define FREEZER_ON_ICON 4
#define FAN_ICON_OFF '|'

#define FAN_ENABLE_PIN 11
#define COMPRESSOR_ENABLE_PIN 12


#define SETPOINT_EEPROM_ADDRESS 0
float tempSetPoint;


volatile uint8_t fanState = OFF, fanIcon = FAN_ICON_OFF;
volatile uint8_t freezerState = OFF, freezerIcon = FREEZER_OFF_ICON;


//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
LCDKeypad lcd;

#define degSym 223
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

byte frownie[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b10001
};

byte backslash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};

byte downArrow[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};

uint8_t fanIcons[] = 
{
  '|',
  '/',
  '-',
  3,
  '|',
  '/',
  '-',
  3
};

int toggle1 = 0;

ISR(TIMER1_COMPA_vect){//timer1 interrupt 2Hz 
  cli();

  if (toggle1){
    digitalWrite(13,HIGH);
    toggle1 = 0;
  }
  else{
    digitalWrite(13,LOW);
    toggle1 = 1;
  }



  getTemperatures();

  stepFanEnable();
  stepFreezerEnable();

  sei();
}


void setup() {

  pinMode(13, OUTPUT);
  pinMode(FAN_ENABLE_PIN, OUTPUT);
  pinMode(COMPRESSOR_ENABLE_PIN, OUTPUT);

  lcd.createChar(0, heart);
  lcd.createChar(1, smiley);
  lcd.createChar(2, frownie);
  lcd.createChar(3, backslash);
  lcd.createChar(4, downArrow);

  lcd.begin(16, 2);

  lcd.print("Initializing...");

  fetchSetpoint();

  lcd.setCursor(0,1);
  tempSensors.begin();
  lcd.print("Found ");
  lcd.print(tempSensors.getDeviceCount(), DEC);
  lcd.println(" sensors.");
  lcd.setCursor(0,1);

  tempSensors.setWaitForConversion(FALSE);
  externalSensors.setWaitForConversion(FALSE);

  for(int i=0; i < NUM_INTERNAL_SENSORS; i++)
  {
    getSensorAddress(i);

    tempSensors.setResolution(internalThermometers[i], 10);

  }
  
  requestTemperatures();

  delay(1000);
  lcd.clear();

  randomSeed(analogRead(0));
  setupInterrupts();
  //Serial.begin(9600);

}



void updateDisplay()
{
  static uint8_t i = 0;
  static unsigned long lastRunTime = 0;
  char foo[10];
  unsigned long curMillis = millis();
  unsigned long deltaTime = curMillis - lastRunTime;
  lastRunTime = curMillis;
  //deltaTime = millis() - lastRunTime;


  //Display Temperature Set Point
  lcd.setCursor(0,0);
  dtostrf(tempSetPoint, 5, 1, foo);
  lcd.print(foo);
  lcd.write(degSym);

  //snprintf(foo, sizeof(foo), "%4d", deltaTime);
  //lcd.print(foo);

  //dtostrf(100.5+i,7,2,foo);
  //lcd.print(foo);
  //printDouble(100.5+i, 100);

  //Display Freezer Compressor State:
  lcd.setCursor(8, 0);
  lcd.write(freezerIcon);


  //Display Time In Freezer State:
  lcd.setCursor(10,0);
  lcd.print(secondsToMMSS(timeInFreezerState()));
  //delay(1500);

  //Display Internal Temperature 1
  lcd.setCursor(0,1);
  dtostrf(internalTemperatures[0], 6, 2, foo);
  lcd.print(foo);
  lcd.write(degSym);

  //Display Stirrer FAN state
  lcd.setCursor(8,1);
  lcd.write(fanIcon);

  //Display Internal Temperature 2
  lcd.setCursor(9,1);
  dtostrf(internalTemperatures[1], 6, 2, foo);
  lcd.print(foo);
  lcd.write(degSym);

  i++;
}


void handleButtonPress()
{
  switch(lcd.button())
  {
  case KEYPAD_NONE:
    return;
  case KEYPAD_SELECT:
    setNewSetpoint();
    break;
  case KEYPAD_UP:
    lcd.clear();
    break;
  default:
    break;
  }
}

void setNewSetpoint()
{
  char str[15];
  char done = 0;
  unsigned long startTime = millis();
  unsigned int timeSinceButtonPress;

  lcd.clear();
  lcd.print("Change Setpoint:");

  lcd.setCursor(4,1);
  dtostrf(tempSetPoint, 5, 1, str);
  lcd.print(str);
  lcd.write(degSym);

  waitReleaseButton();
  timeSinceButtonPress = 0;
  while(!done)
  {

    switch(lcd.button())
    {
    case KEYPAD_NONE:
      timeSinceButtonPress = millis() - startTime;
      if(timeSinceButtonPress > 15000)
      {
        done = 1;
      }
      break;

    case KEYPAD_UP:
      waitReleaseButton();
      timeSinceButtonPress = 0;
      tempSetPoint += 0.5;
      break;

    case KEYPAD_DOWN:
      waitReleaseButton();
      timeSinceButtonPress = 0;
      tempSetPoint -= 0.5;
      break;

    case KEYPAD_SELECT:
      waitReleaseButton();
      timeSinceButtonPress = 0;
      saveSetpoint();

      done=1;  

    default:
      timeSinceButtonPress = 0;
      break;
    }

    lcd.setCursor(4,1);
    dtostrf(tempSetPoint, 5, 1, str);
    lcd.print(str);
    lcd.write(degSym);
  }
  fetchSetpoint();
  lcd.clear();
}

int waitButton()
{
  int buttonPressed; 
  waitReleaseButton;
  lcd.blink();
  while((buttonPressed=lcd.button())==KEYPAD_NONE)
  {
  }
  delay(50);  
  lcd.noBlink();
  return buttonPressed;
}

void waitReleaseButton()
{
  delay(50);
  while(lcd.button()!=KEYPAD_NONE)
  {
  }
  delay(50);
}

int timeInFreezerState()
{
  return (millis() - timeOfLastFreezerStateChange) / 1000;
}

char *secondsToMMSS(int seconds)
{
  int s, m;
  static char str[10];

  m = seconds/60;
  s = seconds - (m * 60);

  snprintf(str, sizeof(str), "%03d:%02d", m, s);

  return str; 

}

void fetchSetpoint()
{
  float setPoint;
  unsigned char *tmp = (unsigned char *) &setPoint;

  for(int i = 0; i < sizeof(tempSetPoint); i++)
  {
    tmp[i] = EEPROM.read(SETPOINT_EEPROM_ADDRESS + i); 
  }


  tempSetPoint = setPoint;
}

void saveSetpoint()
{

  unsigned char *tmp = (unsigned char *) &tempSetPoint;
  for(int i=0; i < sizeof(tempSetPoint); i++)
  {
    EEPROM.write(SETPOINT_EEPROM_ADDRESS + i, tmp[i]);
  }


}


void loop()
{
  handleButtonPress();
  updateDisplay();
}


