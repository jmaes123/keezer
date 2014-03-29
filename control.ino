

void stepFanEnable()
{
  static uint8_t fanIconState = 0;

  int deltaT = abs(internalTemperatures[0] - internalTemperatures[1]);

  if(deltaT > 1)
  {
    fanState = ON;
    digitalWrite(FAN_ENABLE_PIN, HIGH);
  }
  else if(deltaT < .5)
  {
    fanState = OFF;

    digitalWrite(FAN_ENABLE_PIN, LOW);
  }

  if(fanState == ON)
  {

    if(fanIconState > 7)
      fanIconState = 0;

    fanIcon = fanIcons[fanIconState++];
  }
  else
  {
    fanIcon = FAN_ICON_OFF;
  }
}



void stepFreezerEnable()
{
  static unsigned long lastStepTime = 0;
  static unsigned long lastCompressorOnTime = 0;
  unsigned long curTime = millis();
  static unsigned long onTime = 0, offTime = 0;

  
  int deltaTime = curTime - lastStepTime;
  
  switch(freezerState)
  {
  case ON:
    if(min(internalTemperatures[0], internalTemperatures[1]) < (tempSetPoint - 1))
    {
      setFreezerState(OFF);
      timeOfLastFreezerStateChange = curTime;
      lastCompressorOnTime = curTime;
    }
    break;

  case OFF:
  default:

    if((min(internalTemperatures[0], internalTemperatures[1]) > (tempSetPoint + 1.5)) && (curTime - lastCompressorOnTime > 120000))
    {
      setFreezerState(ON);
      timeOfLastFreezerStateChange = curTime;
    }
  }

  lastStepTime = curTime;

}




