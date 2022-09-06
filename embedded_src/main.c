#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "stm32f2xx_hal.h"

struct result {
  int min;
  int max;
};

struct result_f {
  float min;
  float max;
};

struct result_combo {
  struct result_f floatRes;
  struct result intRes;
};

extern struct result_combo run_iniziale();
extern struct result_combo run_annotato();

uint32_t tickMs;

void getTickMs()
{
  HAL_TickFreqTypeDef freqType = HAL_GetTickFreq();
  switch (freqType) {
    case HAL_TICK_FREQ_10HZ:
      tickMs = 100U;
      break;
    case HAL_TICK_FREQ_100HZ:
      tickMs = 10U;
      break;
    case HAL_TICK_FREQ_1KHZ:
    default:
      tickMs = 1U;
  }
}

void main(void)
{
  uint32_t dt, dtb;
  struct result_combo handResultCombo;

  printf("hello, welcome to the crazy taffo bench campaign\n");
  getTickMs();

  HAL_Delay(5 * 1000);
  
  printf("Internal timer ticking every %dms\n\n", tickMs);
  
  printf("Running default hand test...\n");
  
  dt = HAL_GetTick();
  handResultCombo = run_iniziale();
  dtb = HAL_GetTick();
  printf("[float] min = %f max = %f\n", handResultCombo.floatRes.min, handResultCombo.floatRes.max);
  printf("[int] min = %d max = %d\n", handResultCombo.intRes.min, handResultCombo.intRes.max);
  printf("Floating point test took %dms\n\n", (dtb - dt) * tickMs);
  
  memset(&handResultCombo, 0, sizeof(handResultCombo));
  printf("Running TAFFO annotated hand test...\n");
  
  dt = HAL_GetTick();
  handResultCombo = run_annotato();
  dtb = HAL_GetTick();
  printf("[float] min = %f max = %f\n", handResultCombo.floatRes.min, handResultCombo.floatRes.max);
  printf("[int] min = %d max = %d\n", handResultCombo.intRes.min, handResultCombo.intRes.max);
  printf("Fixed point test took %dms\n\n", (dtb - dt) * tickMs);
  
  printf("halting\n");
}

