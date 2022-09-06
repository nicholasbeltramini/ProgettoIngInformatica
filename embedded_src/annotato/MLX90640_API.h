/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef _MLX640_API_H_
#define _MLX640_API_H_

#include <stdint.h>

namespace annotato {
extern int16_t kVddGlobal;
extern int16_t vdd25Global;
extern float KvPTATGlobal __attribute((annotate("scalar(range(-10000,10000) final)"))); //OK
extern float KtPTATGlobal __attribute((annotate("scalar(range(-100,100) final)"))); //OK
extern uint16_t vPTAT25Global;
extern float alphaPTATGlobal __attribute((annotate("scalar(range(-250000,250000) final)"))); //OK
extern int16_t gainEEGlobal;
extern float tgcGlobal __attribute((annotate("scalar(disabled)"))); //OK PERO
extern float cpKvGlobal __attribute((annotate("scalar()"))); //OK
extern float cpKtaGlobal __attribute((annotate("scalar()"))); //OK
extern uint8_t resolutionEEGlobal;
extern uint8_t calibrationModeEEGlobal;
extern float KsTaGlobal __attribute((annotate("scalar(disabled)"))); //OK PERO
extern float ksToGlobal[4] __attribute((annotate("scalar(disabled)"))); //OK PERO
extern int16_t ctGlobal[4];
extern float alphaGlobal[768] __attribute((annotate("scalar(disabled)"))); //OK PERO
extern int16_t offsetGlobal[768];
extern float ktaGlobal[768] __attribute((annotate("scalar(disabled)"))); //OK PERO
extern float kvGlobal[768] __attribute((annotate("scalar(disabled)"))); //OK PERO
extern float cpAlphaGlobal[2] __attribute((annotate("scalar(disabled)"))); //OK PERO
extern int16_t cpOffsetGlobal[2];
extern float ilChessCGlobal[3] __attribute((annotate("scalar()"))); //OK
extern uint16_t brokenPixelsGlobal[5];
extern uint16_t outlierPixelsGlobal[5];

int MLX90640_DumpEE(uint8_t slaveAddr, uint16_t *eeData);

int MLX90640_GetFrameData(uint8_t slaveAddr, uint16_t *frameData);

int MLX90640_ExtractParameters(const uint16_t *eeData);

float MLX90640_GetVdd(const uint16_t *frameData);

float MLX90640_GetTa(const uint16_t *frameData);

void MLX90640_GetImage(const uint16_t *frameData, float *result);

void MLX90640_CalculateTo(const uint16_t *frameData, float emissivity, float tr, float *result);

int MLX90640_SetResolution(uint8_t slaveAddr, uint8_t resolution);

int MLX90640_GetCurResolution(uint8_t slaveAddr);

int MLX90640_SetRefreshRate(uint8_t slaveAddr, uint8_t refreshRate);

int MLX90640_GetRefreshRate(uint8_t slaveAddr);

int MLX90640_GetSubPageNumber(const uint16_t *frameData);

int MLX90640_GetCurMode(uint8_t slaveAddr);

int MLX90640_SetInterleavedMode(uint8_t slaveAddr);

int MLX90640_SetChessMode(uint8_t slaveAddr);
}
#endif
