#define MQL_VERSION "1.1"
#include <Arduino.h>
#include "clsPin.h"
#include <U8g2lib.h>
#include "u8g2TextBox.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <Preferences.h>
#include "esp_err.h"
#include "AiEsp32RotaryEncoder.h"

#ifdef NoRGB // if the board doesn't match the selected board, the RGB support could prevent correctly setting an output pin
#undef RGB_BUILTIN
#endif

#ifndef ROTARY_DEBOUNCE_TIME
#define ROTARY_DEBOUNCE_TIME 100
#endif

#ifndef MIST_DEBOUNCE_TIME
#define MIST_DEBOUNCE_TIME 1000
#endif

#ifndef COOLANT_DEBOUNCE_TIME
#define COOLANT_DEBOUNCE_TIME 1000
#endif

enum ManualModes // Used to switch between coolant and mist mode
{
  ManualModeCoolant,
  ManualModeMist,
  ManualModeInjection
};

ManualModes ManualMode = ManualModeMist;

// Addition constructors for other types of displays can be found at
// https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#buffer-size

// Tested OK ESP32C3-mini pins 6,7;5,7;3,4 || hardware i2c using Wire.begin(SDA,SCL) before u8g2.begin()
U8G2_SH1106_128X64_NONAME_F_HW_I2C OledDisplay(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C OledDisplay(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

u8g2TextBox Version(&OledDisplay, 0, 0, 128, 8, u8g2_font_5x8_mr, "MQL injection: ", MQL_VERSION, false, 3, 1);
u8g2TextBox PinsState(&OledDisplay, 0, 0, 96, 8, u8g2_font_5x8_mr, "PinState: ", "", false, 3, 1);
u8g2TextBox OnState(&PinsState, 96, 0, 24, 8, "", "");
u8g2TextBox AliveState(&OnState, 24, 0, 8, 8, "", "");
u8g2TextBox Mode(&PinsState, 0, 8, 128, 8, "Mode:      ", "");
u8g2TextBox Frequency(&Mode, 0, 8, "Freq:         ", "   Hz");
u8g2TextBox Setpoint(&Frequency, 0, 8, "Setpoint:  ", " ms");
u8g2TextBox ModeLarge(&OledDisplay, 0, 40, 32, 32, u8g2_font_spleen16x32_mr, "", "", false, 3, 1);
u8g2TextBox InjectionTimeLarge(&ModeLarge, 32, 0, 96, 32, "", "");

u8g2TextBox Counter(&Setpoint, 0, 8, 48, 8, "Cnt: ", "");
u8g2TextBox Debug(&Counter, 48, 0, 80, 8, "", "");

AiEsp32RotaryEncoder RotaryEncoder = AiEsp32RotaryEncoder(ROTARY_A_GPIO, ROTARY_B_GPIO, ROTARY_SW_GPIO, ROTARY_VCC_P, ROTARY_STEPS);

volatile int32_t Count = 0;
String DebugString = "--";
// volatile long SetpointRequested = 10;
volatile long InjectionSetpoint = MIN_INJECTION_TIME;
volatile bool MistUpdateRequest = false;
volatile bool SettingChanged = false;
volatile bool SettingUpdateRequest = false;
volatile unsigned long SettingChangedAt = 0;

volatile long InjectionPeriodTics = 10000 / INJECTIONFREQUENCY;
volatile long InjectionActiveTics = 300;
volatile long InjectionInActiveTics = InjectionPeriodTics - InjectionActiveTics;

Preferences FlashData;

clsPin PinLed(LED_GPIO, OUTPUT, LED_ACTIVE, false, "L", "l");
clsPin PinRotaryA(ROTARY_A_GPIO, INPUT_PULLUP, ROTARY_ACTIVE, false, "A", "a");
clsPin PinRotaryB(ROTARY_B_GPIO, INPUT_PULLUP, ROTARY_ACTIVE, false, "B", "b");
clsPin PinRotarySW(ROTARY_SW_GPIO, INPUT_PULLUP, ROTARY_ACTIVE, false, "S", "s", 2000);
clsPin PinMist(MIST_GPIO, INPUT_PULLUP, MIST_ACTIVE, false, "M", "m");
clsPin PinCoolant(COOLANT_GPIO, INPUT_PULLUP, COOLANT_ACTIVE, false, "C", "c");
clsPin PinInjection(INJECTION_GPIO, OUTPUT, INJECTION_ACTIVE, false, "I", "i");
clsPin PinRelay(RELAY_GPIO, OUTPUT, RELAY_ACTIVE, false, "R", "r");

bool InjectionActive()
{
  if (InjectionActiveTics > 0)
    return true;
  else
    return false;
}
bool SystemActive()
{
  if (InjectionActive() || PinRelay.IsActive())
    return true;
  else
    return false;
}
String GetOnStateStr()
{
  if (SystemActive())
    return "ON";
  else
    return "off";
}
void ShowOnState()
{
  OnState.Show(GetOnStateStr());
}
void ShowPinState()
{
  String S = "";
  S = S + PinRotaryA.GetStateStr();
  S = S + PinRotaryB.GetStateStr();
  S = S + PinRotarySW.GetStateStr();
  S = S + PinCoolant.GetStateStr();
  S = S + PinMist.GetStateStr();
  S = S + PinRelay.GetStateStr();
  S = S + PinInjection.GetStateStr();
  // S = S + PinLed.GetStateStr();
  PinsState.Show(S);
}
void ShowAlive()
{
  AliveState.ShowAlive();
}
void ShowFrequency()
{
  Frequency.Show(INJECTIONFREQUENCY);
}
void ShowMode()
{
  switch (ManualMode)
  {
  case ManualModeCoolant:
    Mode.Show("Coolant");
    break;
  case ManualModeMist:
    Mode.Show("Mist");
    break;
  case ManualModeInjection:
    Mode.Show("Injection");
    break;
  }
}
void ShowModeLarge()
{
  switch (ManualMode)
  {
  case ManualModeCoolant:
    ModeLarge.Show("C");
    break;
  case ManualModeMist:
    ModeLarge.Show("M");
    break;
  case ManualModeInjection:
    ModeLarge.Show("I");
  }
}
void ShowSetpoint()
{
  switch (ManualMode)
  {
  case ManualModeCoolant:
    Setpoint.Show("---");
    break;
  case ManualModeMist:
  case ManualModeInjection:
    Setpoint.Show(0.1 * (double_t)RotaryEncoder.readEncoder(), 6, 1);
    break;
  }
}
void ShowCounter()
{
  Counter.Show(Count);
}
void ShowInjectionActiveTime()
{
  switch (ManualMode)
  {
  case ManualModeCoolant:
    InjectionTimeLarge.Show(GetOnStateStr());
    break;
  case ManualModeMist:
  case ManualModeInjection:
    InjectionTimeLarge.Show(0.1 * (double_t)InjectionActiveTics, 5, 1);
    break;
  }
}
void ShowDebug()
{
  Debug.Show(DebugString);
}
void ShowStatus()
{
  ShowPinState();
  ShowOnState();
  ShowAlive();
  ShowMode();
  ShowFrequency();
  ShowSetpoint();
  ShowModeLarge();
  ShowInjectionActiveTime();
  // ShowCounter();
  // ShowDebug();
}
void CalculateInjectionPeriod()
{
  InjectionActiveTics = InjectionSetpoint;
  if (InjectionActiveTics > 0)
    if (InjectionActiveTics < MIN_INJECTION_TIME)
      InjectionActiveTics = MIN_INJECTION_TIME;
  if (InjectionActiveTics > MAX_INJECTION_TIME)
    InjectionActiveTics = MAX_INJECTION_TIME;
  InjectionInActiveTics = InjectionPeriodTics - InjectionActiveTics;
  if (InjectionInActiveTics < 0)
    InjectionInActiveTics = 1;
}
void IRAM_ATTR SignalSettingChanged()
{
  SettingChanged = true;
  SettingUpdateRequest = true;
  SettingChangedAt = millis();
}
void IRAM_ATTR readEncoderISR()
{
  switch (ManualMode)
  {
  case ManualModeCoolant: // in coolant mode, no rotary knob processing
    break;
  case ManualModeMist:
  case ManualModeInjection:
    RotaryEncoder.readEncoder_ISR();
    if (RotaryEncoder.encoderChanged())
      SignalSettingChanged();
    break;
  }
}
void SetInjectionActive(bool Active)
{
  if (Active)
  {
    InjectionSetpoint = RotaryEncoder.readEncoder();
  }
  else
  {
    InjectionSetpoint = 0;
  }
  CalculateInjectionPeriod();
}
void SetupRotaryEncoder()
{
  RotaryEncoder.begin();
  RotaryEncoder.setup(readEncoderISR);
  RotaryEncoder.setBoundaries(MIN_INJECTION_TIME, MAX_INJECTION_TIME, false); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  RotaryEncoder.setAcceleration(ROTARY_ACCELERATION);                         // or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
  RotaryEncoder.setEncoderValue(MIN_INJECTION_TIME);
}
void SetupInjection()
{
  RotaryEncoder.setEncoderValue(InjectionSetpoint);
  SetInjectionActive(false);
}

const char *GetSetpointName()
{
  switch (ManualMode) // seems names can't be to long
  {
  case ManualModeCoolant:
    return "SpC";
  case ManualModeMist:
    return "SpMi";
  case ManualModeInjection:
    return "SpI";
  default:
    return "SpX"; // Must not get here
  }
}
void ReadSetpoint()
{
  // InjectionSetpoint = FlashData.getLong(GetSetpointName(), 0);
  // RotaryEncoder.setEncoderValue(InjectionSetpoint); // set the encoder to the new value  switch (ManualMode) // seems names can't be to long
  switch (ManualMode) // seems names can't be to long
  {
  case ManualModeCoolant:
    InjectionSetpoint = FlashData.getLong("SpC", 0);
    break;
  case ManualModeMist:
    InjectionSetpoint = FlashData.getLong("SpM", 0);
    break;
  case ManualModeInjection:
    InjectionSetpoint = FlashData.getLong("SpI", 0);
    break;
  }
  DebugString = InjectionSetpoint;
  RotaryEncoder.setEncoderValue(InjectionSetpoint);
  // SetupInjection();
}
void WriteSetpoint()
{
  // FlashData.putLong(GetSetpointName(), RotaryEncoder.readEncoder());
  switch (ManualMode) // seems names can't be to long
  {
  case ManualModeCoolant:
    FlashData.putLong("SpC", RotaryEncoder.readEncoder());
    break;
  case ManualModeMist:
    FlashData.putLong("SpM", RotaryEncoder.readEncoder());
    break;
  case ManualModeInjection:
    FlashData.putLong("SpI", RotaryEncoder.readEncoder());
    break;
  }
}
bool SupportedMode(ManualModes Mode)
{
#ifdef MODE_COOLANT
  if (Mode == ManualModeCoolant)
    return true;
#endif
#ifdef MODE_MIST
  if (Mode == ManualModeMist)
    return true;
#endif
#ifdef MODE_INJECTION
  if (Mode == ManualModeInjection)
    return true;
#endif
  return false;
}
ManualModes GetFirstSupportedMode(ManualModes ModeCurrent, ManualModes Mode1, ManualModes Mode2, ManualModes ModeDefault)
{
  if (SupportedMode(ModeCurrent))
    return ModeCurrent;
  if (SupportedMode(Mode1))
    return Mode1;
  if (SupportedMode(Mode2))
    return Mode2;
  return ModeDefault;
}
ManualModes GetNextSupportedMode(ManualModes ModeCurrent, ManualModes Mode1, ManualModes Mode2, ManualModes ModeDefault)
{
  return GetFirstSupportedMode(Mode1, Mode2, ModeCurrent, ModeDefault);
}
ManualModes GetFirstSupportedMode(ManualModes ModeCurrent)
{
  switch (ModeCurrent)
  {
  case ManualModeCoolant:
    return GetFirstSupportedMode(ManualModeCoolant, ManualModeMist, ManualModeInjection, ManualModeCoolant);
  case ManualModeMist:
    return GetFirstSupportedMode(ManualModeMist, ManualModeInjection, ManualModeCoolant, ManualModeCoolant);
  case ManualModeInjection:
    return GetFirstSupportedMode(ManualModeInjection, ManualModeCoolant, ManualModeMist, ManualModeCoolant);
  }
  return ManualModeCoolant; // Should not get here
}
ManualModes GetNextSupportedMode(ManualModes ModeCurrent)
{
  switch (ModeCurrent)
  {
  case ManualModeCoolant:
    return GetNextSupportedMode(ManualModeCoolant, ManualModeMist, ManualModeInjection, ManualModeCoolant);
  case ManualModeMist:
    return GetNextSupportedMode(ManualModeMist, ManualModeInjection, ManualModeCoolant, ManualModeCoolant);
  case ManualModeInjection:
    return GetNextSupportedMode(ManualModeInjection, ManualModeCoolant, ManualModeMist, ManualModeCoolant);
  }
  return ManualModeCoolant; // Should not get here
}
void SetInactive()
{
  PinRelay.SetInactive();
  SetInjectionActive(false);
}
void SetActive()
{
  SetInactive();
  switch (ManualMode)
  {
  case ManualModeCoolant:
    PinRelay.SetActive();
    break;
  case ManualModeMist:
    PinRelay.SetActive();
    SetInjectionActive(true);
    break;
  case ManualModeInjection:
    SetInjectionActive(true);
    break;
  }
}
void ReadWriteFlashData(bool read)
{
  long LongValue = 0;
  String DBversion = "1.0";
  if (read)
  {
    DBversion = FlashData.getString("DBversion", DBversion);
    ManualMode = (ManualModes)FlashData.getLong("Mode", ManualModeCoolant);
    ManualMode = GetFirstSupportedMode(ManualMode); // make sure it is a supported mode
    ReadSetpoint();                                 // read the setpoint for this mode
  }
  else
  {
    FlashData.putString("DBversion", "1.0");
    FlashData.putLong("Mode", ManualMode);
    WriteSetpoint();
    Count++;
  }
}
void SetManualMode(ManualModes NewMode)
{
  NewMode = GetFirstSupportedMode(NewMode); // correct the manual mode
  if (ManualMode == NewMode)
    return;               // nothing to change
  ManualMode = NewMode;   // Set the new mode
  SignalSettingChanged(); // save the changed mode (after 5 seconds)
  SetInactive();          // Inactivate all relays
  ReadSetpoint();         // read the last saved setpoint
  SetupInjection();       // setup the injection
}
void SaveSetpointWhenChanged()
{
  if (!SettingChanged)
    return;
  if ((millis() - SettingChangedAt) < 5000)
    return;
  SettingChanged = false;
  ReadWriteFlashData(false);
}
void ProcessSetpointUpdateRequest()
{
  FlashData.putLong("Mode", ManualMode);
  if (SettingUpdateRequest)
  {
    if (InjectionActive())
    {
      InjectionSetpoint = RotaryEncoder.readEncoder();
      CalculateInjectionPeriod();
    }
    SettingUpdateRequest = false;
  }
}
void ActivateMode(ManualModes Mode)
{
  SetInactive();       // start by deactivating all relais
  SetManualMode(Mode); // Set and correct the mode
  switch (ManualMode)
  {
  case ManualModeCoolant:
    PinRelay.SetActive();
    break;
  case ManualModeMist:
    PinRelay.SetActive();
    InjectionSetpoint = RotaryEncoder.readEncoder();
    CalculateInjectionPeriod();
    break;
  case ManualModeInjection:
    InjectionSetpoint = RotaryEncoder.readEncoder();
    CalculateInjectionPeriod();
    break;
  }
}
void SetManualMode(bool CoolantActive, bool MistActive)
{
  if ((CoolantActive) && (!MistActive))
  {
#ifdef MODE_COOLANT
    ActivateMode(ManualModeCoolant);
#endif
  }
  else if ((!CoolantActive) && (MistActive))
  {
#ifdef MODE_MIST
    ActivateMode(ManualModeMist);
#endif
  }
  else if ((CoolantActive) && (MistActive))
  {
#ifdef MODE_INJECTION
    ActivateMode(ManualModeInjection);
#endif
  }
  else // set inactive and disable relays
  {
    InjectionSetpoint = 0;
    CalculateInjectionPeriod();
    SetInactive(); // Inactivate relays
    return;
  }
}
void ProcessMistPinUpdateRequest()
{
  switch (PinMist.GetPinState()) // update the SW button status
  {
  case PinStateI:  // InActive
  case PinStateIU: // InActive, comming from unprocessed active
  case PinStateA:  // Active
  case PinStateAU: // Active, comming from unprocessed active
  {
    PinMist.SetProcessed();
    SetManualMode(PinCoolant.IsActive(), PinMist.IsActive());
    break;
  }
  }
}
void ProcessCoolantPinUpdateRequest()
{
  switch (PinCoolant.GetPinState()) // update the SW button status
  {
  case PinStateI:  // InActive
  case PinStateIU: // InActive, comming from unprocessed active
  case PinStateA:  // Active
  case PinStateAU: // Active, comming from unprocessed active
  {
    PinCoolant.SetProcessed();
    SetManualMode(PinCoolant.IsActive(), PinMist.IsActive());
    break;
  }
  }
}
void IRAM_ATTR SetInjectionState(bool State)
{
#if DEBUG
  PinRelay.SetState(State);
#else
  PinLed.SetState(State);
#endif
  PinInjection.SetState(State);
}
void IRAM_ATTR Delay_Tics(int32_t Tics)
{
  if (Tics < 50)
    delayMicroseconds(Tics * 100); // Delay in micro seconds, this lock the task core
  else
    delay(Tics / 10); // Delay in milli seconds,  task core is freedup during delay
}
void IRAM_ATTR BackGroundTask(void *pvParameters)
{
  for (;;)
  {
    if (InjectionActiveTics > 0)
    {
      SetInjectionState(true);
    }
    Delay_Tics(InjectionActiveTics);
    SetInjectionState(false);
    Delay_Tics(InjectionInActiveTics);
  }
}
void SetupBackGroundTask(void)
{
  static uint8_t ucParameterToPass;
  TaskHandle_t xHandle = NULL;
  // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
  // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
  // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
  // the new task attempts to access it.
  xTaskCreate(BackGroundTask, "INJECT", 10240, &ucParameterToPass, 2, &xHandle); // Priority 2 assures the task is running without interruption
  configASSERT(xHandle);
}
void SetupDebouncing()
{
  PinRotarySW.DebounceTime = ROTARY_DEBOUNCE_TIME;
  PinMist.DebounceTime = MIST_DEBOUNCE_TIME;
  PinCoolant.DebounceTime = COOLANT_DEBOUNCE_TIME;
}
void ProcessRotaryButtonUpdateRequest()
{
  switch (PinRotarySW.GetPinState()) // update the SW button status
  {
  // process SW pressed on released
  case PinStateIU: // Inactive, comming from unprocessed active
    PinRotarySW.SetProcessed();
    switch (ManualMode)
    {
    case ManualModeCoolant:
      SetInjectionActive(false); // Disable injection
      PinRelay.TogglePin();      // Toggle the relay pin
      break;
    case ManualModeMist:
      SetInjectionActive(!InjectionActive());
      PinRelay.SetState(InjectionActive()); // Set the relay state equal to the injection state
      break;
    case ManualModeInjection:
      SetInjectionActive(!InjectionActive());
      break;
    }
    break;
  case PinStateS:                                  // Pin is short time active
    ManualMode = GetNextSupportedMode(ManualMode); // get the next supported mode
    SetManualMode(ManualMode);
    PinRotarySW.SetProcessed();
  }
}
void TestPWM()
{
  PinLed.SetupPWM(500, 8);
  PinLed.SetDutyCycle(200);
  for (;;)
    ShowStatus;
}
void setup(void)
{
  // Serial.begin(115200);  // Can't use serial because rx/tx pins are used
  PinLed.Flash(); // #1 Show startingup
  FlashData.begin("MQL", false);
  ReadWriteFlashData(true);
  PinLed.Flash(); // #2 Show progress
  SetupRotaryEncoder();
  CalculateInjectionPeriod();
  SetupInjection();
  PinLed.Flash(); // #3 Show progress and give powersupply time to get stable
                  // Seems that pinmodes are changed after i2c is activated.
  Wire.begin(SDA_PIN, SCL_PIN);
  PinMist.SetMode();
  PinCoolant.SetMode();
  OledDisplay.begin(); // InjectionSetpoint = 0;// disable injection
  Version.Show();      // Show the version
  PinLed.Flash();      // # 4 Show progress
  // Seems that pinmodes are changed after i2c is activated.
  PinMist.SetMode();
  PinCoolant.SetMode();
  PinInjection.SetMode();
  SetupDebouncing();
  PinLed.Flash(); // # 5 Show progress
  SetupBackGroundTask();
  PinLed.Flash(); // #6 Show progress
  // TestPWM();
}
void loop(void)
{
  ShowStatus();
  // PinRelay.TogglePin();
  // delay(500);
  ProcessMistPinUpdateRequest();
  ProcessCoolantPinUpdateRequest();
  ProcessSetpointUpdateRequest();
  SaveSetpointWhenChanged();
  ProcessRotaryButtonUpdateRequest();
  // PinLed.TogglePin();
  delay(10); // do a small delay so back ground tasks have more time to run
}
