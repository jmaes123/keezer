
void setupInterrupts()
{
  cli();
  

  //set timer1 interrupt at 2Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 7811;// = (16*10^6) / (2*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();
}


//FREEZER CONTROL

void setFreezerState(uint8_t state)
{
    switch(state)
    {
      case ON:
        freezerIcon = FREEZER_ON_ICON;
        freezerState = ON;
        digitalWrite(COMPRESSOR_ENABLE_PIN, HIGH);
        break;
        
      case OFF:
      default:
        freezerIcon = FREEZER_OFF_ICON;
        freezerState = OFF;
        digitalWrite(COMPRESSOR_ENABLE_PIN, LOW);
    }
    
}



//Thermometer Functions
void getSensorAddress(uint8_t id)
{
  
  if(!tempSensors.getAddress(internalThermometers[id], id))
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    FAILURE!        FAILURE!");
    lcd.setCursor(0,1);
    lcd.print("Error reading device ");
    lcd.print(id);
    lcd.print(" address.");
    
    while(1) {
      delay(1500);
      for( int i = 0; i < 16; i++)
      {
        lcd.scrollDisplayLeft();
        delay(300);
      }
      delay(1500);
      for( int i = 0; i < 16; i++)
      {
        lcd.scrollDisplayRight();
        delay(300);
      }
    }
  }
}

void requestTemperatures()
{
  tempSensors.requestTemperatures();
}

void getTemperatures()
{
  for(int i=0; i < NUM_INTERNAL_SENSORS; i++)
  {
    internalTemperatures[i] = tempSensors.getTempC(internalThermometers[i]);
  }
  
  requestTemperatures();
}


