// V02
#include "clsPin.h"

clsPin::clsPin(uint8_t Pin, uint8_t Mode, uint8_t ActiveLevel, uint8_t PinState, String strActive, String strInactive, String strLow, String strHigh, unsigned long ShortPressMilliseconds, unsigned long LongPressMilliseconds)
{
    clsPin::Pin = Pin;
    clsPin::Mode = Mode;
    clsPin::ActiveLevel = ActiveLevel;
    clsPin::strActive = strActive;
    clsPin::strInactive = strInactive;
    clsPin::strLow = strLow;
    clsPin::strHigh = strHigh;
    clsPin::ShortHoldMilliseconds = ShortPressMilliseconds;
    clsPin::LongHoldMilliseconds = LongHoldMilliseconds;
    pinMode(Pin, Mode);
    switch (Mode)
    {
    case OUTPUT:
        SetState(PinState);
        break;
    }
}
void clsPin::SetMode()
{
    pinMode(Pin, Mode);
    switch (Mode)
    {
    case OUTPUT:
    case OUTPUT_OPEN_DRAIN:
        SetLevel(Level);
        break;
    }
}
ledc_timer_bit_t clsPin::GetTimerResolution(uint32_t Frequency)
{
    // if (Frequency > 1024000)
    //   return (ledc_timer_bit_t)5;
    if (Frequency > 512000)
        return (ledc_timer_bit_t)6;
    if (Frequency > 256000)
        return (ledc_timer_bit_t)7;
    if (Frequency > 128000)
        return (ledc_timer_bit_t)8;
    else if (Frequency > 64000)
        return (ledc_timer_bit_t)9;
    else if (Frequency > 32000)
        return LEDC_TIMER_10_BIT;
    else if (Frequency > 16000)
        return LEDC_TIMER_11_BIT;
    else if (Frequency > 8000)
        return LEDC_TIMER_12_BIT;
    else if (Frequency > 4000)
        return LEDC_TIMER_13_BIT;
    else
        return LEDC_TIMER_14_BIT;
}
void clsPin::setupLedCpulseTime(int GpioPin, uint32_t Frequency, uint32_t PulseTimeUs, ledc_timer_t Timer, ledc_channel_t Channel, ledc_timer_bit_t TimerResolution)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = TimerResolution,
        .timer_num = Timer,
        .freq_hz = Frequency,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);
    uint32_t FrequencySet = ledc_get_freq(LEDC_LOW_SPEED_MODE, Timer);
    uint32_t MaxTics = 1 << TimerResolution;
    uint32_t PeriodTimeUs = 1000000 / FrequencySet;
    uint32_t Tics = PulseTimeUs * MaxTics / PeriodTimeUs;
    ledc_channel_config_t ledc_channel = {
        .gpio_num = GpioPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = Channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = Timer,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, Channel, Tics);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, Channel);
}
void clsPin::setupLedCpulseTime(int GpioPin, uint32_t Frequency, uint32_t PulseTimeUs, ledc_timer_t Timer, ledc_channel_t Channel)
{
    if (Frequency < 2)
        Frequency = 2;
    else if (Frequency > 512000)
        Frequency = 512000;
    setupLedCpulseTime(GpioPin, Frequency, PulseTimeUs, Timer, Channel, GetTimerResolution(Frequency));
}
void clsPin::setupLedCpulseTime(int GpioPin, uint32_t Frequency, uint32_t PulseTimeUs)
{
    setupLedCpulseTime(GpioPin, Frequency, PulseTimeUs, LEDC_TIMER_0, LEDC_CHANNEL_0);
}
void clsPin::setupLedCdutyCycle(int GpioPin, uint32_t Frequency, uint32_t DutyCycle, ledc_timer_t Timer, ledc_channel_t Channel)
{
    uint32_t PeriodTimeUs = 1000000 / Frequency;
    uint32_t PulseTimeUs = PeriodTimeUs * DutyCycle / 100;
    setupLedCpulseTime(GpioPin, Frequency, PulseTimeUs, Timer, Channel);
}
void clsPin::setupLedCdutyCycle(int GpioPin, uint32_t Frequency, uint32_t DutyCycle)
{
    setupLedCpulseTime(GpioPin, Frequency, DutyCycle, LEDC_TIMER_0, LEDC_CHANNEL_0);
}
void clsPin::Flash(uint8_t Count, uint16_t TimeActive, uint16_t TimeInactive)
{
    for (uint8_t i = 0; i < Count; i++)
    {
        SetState(true);
        delay(TimeActive);
        SetState(false);
        delay(TimeInactive);
    }
};
void clsPin::Flash()
{
    Flash(1, 200, 200);
};
void clsPin::TogglePin()
{
    digitalWrite(Pin, !digitalRead(Pin));
}
void clsPin::SetActive()
{
    SetLevel(ActiveLevel);
}
void clsPin::SetInactive()
{
    SetLevel(!ActiveLevel);
}
void clsPin::SetState(uint8_t PinState)
{
    if (PinState)
        SetLevel(ActiveLevel);
    else
        SetLevel(!ActiveLevel);
}
void clsPin::SetLevel(uint8_t Level)

{
    clsPin::Level = Level;
    digitalWrite(Pin, Level);
}
void clsPin::SetupPWM(uint32_t Frequency, uint32_t ResolutionBits)
{
    uint32_t f = ledcSetup(0, Frequency, ResolutionBits);
    if (f > 0)
    {
        clsPin::Frequency = Frequency;
        clsPin::ResolutionBits = ResolutionBits;
        clsPin::PeriodTics = 1 << ResolutionBits;
        clsPin::PeriodUs = (uint32_t)1000000 / Frequency;
        ledcAttachPin(Pin, 0);
    }
    else
        SetupPWM(1000, 4);
}
void clsPin::SetDutyCycle(uint32_t DutyCycle)
{
    ledcWrite(Pin, DutyCycle);
    clsPin::DutyCycle = DutyCycle;
}
void clsPin::SetPulseTimeUs(uint32_t PulseTimeUs)
{
    uint32_t DutyCycle = (PeriodTics * PulseTimeUs) / PeriodUs;
    SetDutyCycle(DutyCycle);
}
bool IRAM_ATTR clsPin::IsActive()
{
    return (GetLevel() == ActiveLevel);
}
uint8_t IRAM_ATTR clsPin::GetLevel()
{
    return digitalRead(Pin);
}
String clsPin::GetStateStr()
{
    return GetStateStr(strActive, strInactive);
}
String clsPin::GetStateStr(String strActive, String strInactive)
{
    if (IsActive())
        return strActive;
    else
        return strInactive;
}
String clsPin::GetLevelStr()
{
    if (GetLevel())
        return strActive;
    else
        return strInactive;
}
String clsPin::GetPinStateStr()
{
    switch (PinState)
    {
    case PinStateU:
        return "U";
    case PinStateUA:
        return "UA";
    case PinStateUI:
        return "UI";

    case PinStateI:
        return "I";
    case PinStateIP:
        return "IP";
    case PinStateIU:
        return "IU";
    case PinStateIA:
        return "IA";
    case PinStateIPA:
        return "IPA";
    case PinStateIUA:
        return "IUA";
    case PinStateA:
        return "A";
    case PinStateAP:
        return "AP";
    case PinStateAU:
        return "AU";
    case PinStateAI:
        return "AI";
    case PinStateAPI:
        return "API";
    case PinStateAUI:
        return "AUI";

    case PinStateS:
        return "S";
    case PinStateSP:
        return "SP";
    case PinStateSI:
        return "SI";
    case PinStateSPI:
        return "SPI";
    case PinStateIS:
        return "IS";
    case PinStateISA:
        return "ISA";

    default:
        return "??";
    }
}
void clsPin::AttachInterrupt(void (*ISR_callback)(void), uint8_t InterruptMode)
{
    attachInterrupt(digitalPinToInterrupt(Pin), ISR_callback, InterruptMode);
}
void clsPin::AttachInterrupt(void (*ISR_callback)(void))
{
    attachInterrupt(digitalPinToInterrupt(Pin), ISR_callback, CHANGE);
}
// This method can not be attached to an interrupt handler.
// It must be called from outside the class.
// The calling method outside the class must be attached to the interrupt handler
// example:
// void IRAM_ATTR ProcessISR()
// {
//   PinXX.ProcessPinActiveISR();
// }
// void SetupPinXXInterrupt()
// {
//   PinXX.AttachInterrupt(ProcessISR, CHANGE);
// }
void IRAM_ATTR clsPin::ProcessPinActiveISR() // processes the pin active changes to detect a long or short active time
{
    if (IsActive())
    {
        lastTimeActive = millis(); // When active, register the last time pin was active
    }
    else
    {
        long MillesPassed = millis() - lastTimeActive; // Calculate how long the pin was active
        if (MillesPassed > LongHoldMilliseconds)        // if active for a long time
        {
            ProcessLongHoldActive = true; // signal that a long active  has to be processed
        }
        else if (MillesPassed > ShortHoldMilliseconds) // if active for a short time (debounce)
        {
            ProcessShortHoldActive = true; // signal that a short active has to be processed
        }
    }
}
bool IRAM_ATTR clsPin::PinStateStable(long StableTime)
{
    if (StableTime == 0)
        return false; //
    return (millis() >= (lastTimeChange + StableTime));
}
bool IRAM_ATTR clsPin::PinStateStable()
{
    return PinStateStable(DebounceTime);
}
PinStates IRAM_ATTR clsPin::ProcessStateStableShort()
{
    switch (PinState)
    {
    case PinStateA:
    case PinStateAU:
    case PinStateAP:
        return PinStateS;
    case PinStateSI:
        return PinStateIS;
    case PinStateSPI:
        return PinStateI;
    }
    return PinState; // nothing changed
}
PinStates IRAM_ATTR clsPin::ProcessStateStable()
{
    switch (PinState)
    {
    case PinStateUI:
    case PinStateAPI:
        return PinStateI;
    case PinStateAI:
        return PinStateIU;
    case PinStateIA:
    case PinStateIUA:
        return PinStateAU;
    case PinStateSI:
        return PinStateIS;
    case PinStateSPI:
        return PinStateI;
    case PinStateUA:
    case PinStateIPA:
    case PinStateISA:
        return PinStateA;
    case PinStateAUI:
        return PinStateIU;
    }
    return PinState; // nothing changed, should not get here
}
PinStates IRAM_ATTR clsPin::ProcessStateChange()
{
    lastTimeChange = millis(); // register the time of change
    if (State)                 // if the new state is active
    {
        switch (PinState)
        {
        case PinStateU:
        case PinStateUI:
            return PinStateUA;
        case PinStateI:
            return PinStateIA;
        case PinStateIP:
            return PinStateIPA;
        case PinStateIU:
            return PinStateIUA;
        case PinStateAU:
            return PinStateAUI;
        case PinStateAI:
            return PinStateA;
        case PinStateAPI:
            return PinStateAP;

        case PinStateIS:
            return PinStateISA;
        }
    }
    else // the new state is inactive
    {
        switch (PinState)
        {
        case PinStateU:
        case PinStateUA:
            return PinStateUI;
        case PinStateA:
            return PinStateAI;
        case PinStateAP:
            return PinStateAPI;
        case PinStateS:
            return PinStateSI;
        case PinStateSP:
            return PinStateSPI;
        case PinStateIA:
            return PinStateI;
        case PinStateIPA:
            return PinStateIP;
        case PinStateAU:
            return PinStateAUI;

        case PinStateISA:
            return PinStateIS;
        }
    }
    return PinState; // should not get here
}
PinStates IRAM_ATTR clsPin::GetPinState()
{
    bool LastState = State;                           // save the last state
    State = IsActive();                                  // update the state
    if ((State != LastState) || (PinState == PinStateU)) // there is a state change or the first callcall
        PinState = ProcessStateChange();                 // return a pin state change
    if (PinStateStable())                                // If the state is stable
        PinState = ProcessStateStable();                 // return a new stable pin state
    if (PinStateStable(ShortHoldMilliseconds))           // If the state is stable for the short press time
        PinState = ProcessStateStableShort();            // return a new stable pin state
    return PinState;                                     // return the current pin state
}
void IRAM_ATTR clsPin::SetProcessed()
{
    switch (GetPinState())
    {
    case PinStateA:
    case PinStateAU:
        PinState = PinStateAP;
        break;
    case PinStateAI:
    case PinStateAUI:
        PinState = PinStateAPI;
        break;
    case PinStateI:
    case PinStateIU:
        PinState = PinStateIP;
        break;
    case PinStateIA:
    case PinStateIUA:
        PinState = PinStateIPA;
        break;

    case PinStateS:
        PinState = PinStateSP;
        break;
    case PinStateSI:
        PinState = PinStateSPI;
        break;
    case PinStateIS:
        PinState = PinStateIP;
        break;
    }
}