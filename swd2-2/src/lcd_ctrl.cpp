#include "lcd_ctrl.hpp"


// initialize the library with the numbers of the interface pins
// LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
ST7032 lcd;
SemaphoreHandle_t lcd_mutex;
char baseMacChr[18] = {0};
char stateChr[6] = {0};
char netChr[4] = {0};
const int lcd_en_pin = 17;
const int i2c_sda_pin = 23;
const int i2c_scl_pin = 22;

void lcdUpdate(void)
{
  xSemaphoreTake(lcd_mutex, portMAX_DELAY);
  lcd.setCursor(0, 0);
  lcd.print("STA:");
  lcd.print(stateChr);
  lcd.setCursor(9, 0);
  lcd.print("NET:");
  lcd.print(netChr);
  lcd.setCursor(0, 1);
  lcd.print("MAC:");
  lcd.print(baseMacChr);
  xSemaphoreGive(lcd_mutex);
}

void lcdSetState(stateNum state)
{
  switch (state)
  {
  case NP:
    sprintf(stateChr, "    ");
    break;
  case DO:
    sprintf(stateChr, "DO  ");
    break;
  case REST:
    sprintf(stateChr, "REST");
    break;

  default:
    sprintf(stateChr, "    ");
    break;
  }
  lcdUpdate();
}

void lcdSetNetStatus(int net)
{
  switch (net)
  {
  case 0:
    sprintf(netChr, "  ");
    break;
  case 1:
    sprintf(netChr, "NG");
    break;
  case 2:
    sprintf(netChr, "OK");
    break;
  default:
    sprintf(netChr, "  ");
    break;
  }
  lcdUpdate();
}

void lcdShutdown(void){
  pinMode(i2c_scl_pin,OUTPUT);
  pinMode(i2c_sda_pin,OUTPUT);
  digitalWrite(i2c_scl_pin,LOW);
  digitalWrite(i2c_sda_pin,LOW);
  pinMode(lcd_en_pin, INPUT);
}

void lcdInit(void)
{
  lcd_mutex = xSemaphoreCreateMutex();
  xSemaphoreTake(lcd_mutex, portMAX_DELAY);
  pinMode(lcd_en_pin, OUTPUT);
  digitalWrite(lcd_en_pin,LOW);
  lcd.begin(16, 2);
  lcd.setContrast(40);
  lcd.clear();
  xSemaphoreGive(lcd_mutex);

  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

  lcdUpdate();
}