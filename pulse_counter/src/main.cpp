#include <Arduino.h>
//https://qiita.com/wanko_in_lunch/items/a508d8da78961c855d7f
#include "driver/pcnt.h"

#define PULSE_INPUT_PIN A1    //パルスの入力ピン
#define PCNT_H_LIM_VAL 32767  //カウンタの上限 今回は使ってない
#define PCNT_L_LIM_VAL -32768 //カウンタの下限 今回は使ってない

void setup()
{
  pcnt_config_t pcnt_config; //設定用の構造体の宣言
  pcnt_config.pulse_gpio_num = PULSE_INPUT_PIN;
  pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
  pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.channel = PCNT_CHANNEL_0;
  pcnt_config.unit = PCNT_UNIT_0;
  pcnt_config.pos_mode = PCNT_COUNT_DIS;
  pcnt_config.neg_mode = PCNT_COUNT_INC;
  pcnt_config.counter_h_lim = PCNT_H_LIM_VAL;
  pcnt_config.counter_l_lim = PCNT_L_LIM_VAL;
  pcnt_unit_config(&pcnt_config);   //ユニット初期化
  pcnt_counter_pause(PCNT_UNIT_0);  //カウンタ一時停止
  pcnt_counter_clear(PCNT_UNIT_0);  //カウンタ初期化
  pcnt_counter_resume(PCNT_UNIT_0); //カウント開始
  Serial.begin(115200);
}

void loop()
{
  int16_t count = 0; //カウント数
  pcnt_get_counter_value(PCNT_UNIT_0, &count);
  if (count != 0)
  {
    Serial.print("Counter value: ");
    Serial.println(count);
    delay(1000);
    pcnt_counter_clear(PCNT_UNIT_0);
  }
  delay(10);
}