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
#include "MLX90640_I2C_Driver.h"
#include "MLX90640_API.h"
#include <math.h>
#include <stdio.h>

//By TFT: the newlib C library uses iprintf
#ifndef _MIOSIX
#define iprintf printf
#endif

namespace annotato {
int16_t kVddGlobal;
int16_t vdd25Global;
float KvPTATGlobal;
float KtPTATGlobal;
uint16_t vPTAT25Global;
float alphaPTATGlobal;
int16_t gainEEGlobal;
float tgcGlobal;
float cpKvGlobal;
float cpKtaGlobal;
uint8_t resolutionEEGlobal;
uint8_t calibrationModeEEGlobal;
float KsTaGlobal;
float ksToGlobal[4];
int16_t ctGlobal[4];
float alphaGlobal[768];
int16_t offsetGlobal[768];
float ktaGlobal[768];
float kvGlobal[768];
float cpAlphaGlobal[2];
int16_t cpOffsetGlobal[2];
float ilChessCGlobal[3];
uint16_t brokenPixelsGlobal[5];
uint16_t outlierPixelsGlobal[5];

void ExtractVDDParameters(const uint16_t *eeData);
void ExtractPTATParameters(const uint16_t *eeData);
void ExtractGainParameters(const uint16_t *eeData);
void ExtractTgcParameters(const uint16_t *eeData);
void ExtractResolutionParameters(const uint16_t *eeData);
void ExtractKsTaParameters(const uint16_t *eeData);
void ExtractKsToParameters(const uint16_t *eeData);
void ExtractAlphaParameters(const uint16_t *eeData);
void ExtractOffsetParameters(const uint16_t *eeData);
void ExtractKtaPixelParameters(const uint16_t *eeData);
void ExtractKvPixelParameters(const uint16_t *eeData);
void ExtractCPParameters(const uint16_t *eeData);
void ExtractCILCParameters(const uint16_t *eeData);
int ExtractDeviatingPixels(const uint16_t *eeData);
int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2);
int CheckEEPROMValid(const uint16_t *eeData);  
  
int MLX90640_DumpEE(uint8_t slaveAddr, uint16_t *eeData)
{
     return MLX90640_I2CRead(slaveAddr, 0x2400, 832, eeData);
}

int MLX90640_GetFrameData(uint8_t slaveAddr, uint16_t *frameData)
{
    uint16_t dataReady = 1;
    uint16_t controlRegister1;
    uint16_t statusRegister;
    int error = 1;
    uint8_t cnt = 0;
    
    dataReady = 0;
    while(dataReady == 0)
    {
        error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
        if(error != 0)
        {
            return error;
        }    
        dataReady = statusRegister & 0x0008;
    }       
        
    while(dataReady != 0 && cnt < 5)
    { 
        //Fix by TFT
        error = MLX90640_I2CWrite(slaveAddr, 0x8000, statusRegister & ~0x0008);
        //error = MLX90640_I2CWrite(slaveAddr, 0x8000, 0x0030);
        if(error == -1)
        {
            return error;
        }
            
        error = MLX90640_I2CRead(slaveAddr, 0x0400, 832, frameData); 
        if(error != 0)
        {
            return error;
        }
                   
        error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
        if(error != 0)
        {
            return error;
        }    
        dataReady = statusRegister & 0x0008;
        cnt = cnt + 1;
    }

    if(cnt > 1) iprintf("MLX90640_GetFrameData tried %d times\n", cnt);

    if(cnt > 4)
    {
        return -8;
    }    
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    frameData[832] = controlRegister1;
    frameData[833] = statusRegister & 0x0001;
    
    if(error != 0)
    {
        return error;
    }
    
    return frameData[833];

    //Code modified by TFT to use the enable/disable overwrite feature
    //Note: it works, but accidental overwrite doesn't seem to be the cause
    //of noise, so it's not an improvement,
    //also when the MCU can't keep up with the data rate the following write
    //error occur and the image data is stuck at -100??C or something
    //reg[0x8000] = 0x10 Error readback is 0x9
    //reg[0x8000] = 0x11 Error readback is 0x8
//     uint16_t statusRegister;
//     int error = 1;
//     
//     uint16_t dataReady = 0;
//     while(dataReady == 0)
//     {
//         error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
//         if(error != 0)
//         {
//             return error;
//         }    
//         dataReady = statusRegister & 0x0008;
//     }       
// 
//     uint8_t cnt = 0;
//     while(dataReady != 0 && cnt < 5)
//     {
//         //Clear Enable overwrite and New data available bits
//         error = MLX90640_I2CWrite(slaveAddr, 0x8000, statusRegister & ~0x0018);
//         if(error == -1)
//         {
//             return error;
//         }
//             
//         error = MLX90640_I2CRead(slaveAddr, 0x0400, 832, frameData); 
//         if(error != 0)
//         {
//             return error;
//         }
//                    
//         error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
//         if(error != 0)
//         {
//             return error;
//         }
//         
//         //Enable back overwrite
//         error = MLX90640_I2CWrite(slaveAddr, 0x8000, statusRegister | 0x0010);
//         if(error == -1)
//         {
//             return error;
//         }
//         
//         dataReady = statusRegister & 0x0008;
//         cnt = cnt + 1;
//     }
// 
//     if(cnt > 1) iprintf("MLX90640_GetFrameData tried %d times\n", cnt);
// 
//     if(cnt > 4)
//     {
//         return -8;
//     }
//     
//     uint16_t controlRegister1;
//     error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
//     frameData[832] = controlRegister1;
//     frameData[833] = statusRegister & 0x0001;
//     
//     if(error != 0)
//     {
//         return error;
//     }
//     
//     return frameData[833];
}

int MLX90640_ExtractParameters(const uint16_t *eeData)
{
    int error = CheckEEPROMValid(eeData);
    
    if(error == 0)
    {
        ExtractVDDParameters(eeData);
        ExtractPTATParameters(eeData);
        ExtractGainParameters(eeData);
        ExtractTgcParameters(eeData);
        ExtractResolutionParameters(eeData);
        ExtractKsTaParameters(eeData);
        ExtractKsToParameters(eeData);
        ExtractAlphaParameters(eeData);
        ExtractOffsetParameters(eeData);
        ExtractKtaPixelParameters(eeData);
        ExtractKvPixelParameters(eeData);
        ExtractCPParameters(eeData);
        ExtractCILCParameters(eeData);
        error = ExtractDeviatingPixels(eeData);
    }
    
    return error;

}

//------------------------------------------------------------------------------

int MLX90640_SetResolution(uint8_t slaveAddr, uint8_t resolution)
{
    uint16_t controlRegister1;
    int value;
    int error;
    
    value = (resolution & 0x03) << 10;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    
    if(error == 0)
    {
        value = (controlRegister1 & 0xF3FF) | value;
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);        
    }    
    
    return error;
}

//------------------------------------------------------------------------------

int MLX90640_GetCurResolution(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int resolutionRAM;
    int error;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if(error != 0)
    {
        return error;
    }    
    resolutionRAM = (controlRegister1 & 0x0C00) >> 10;
    
    return resolutionRAM; 
}

//------------------------------------------------------------------------------

int MLX90640_SetRefreshRate(uint8_t slaveAddr, uint8_t refreshRate)
{
    uint16_t controlRegister1;
    int value;
    int error;
    
    value = (refreshRate & 0x07)<<7;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if(error == 0)
    {
        value = (controlRegister1 & 0xFC7F) | value;
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);
    }    
    
    return error;
}

//------------------------------------------------------------------------------

int MLX90640_GetRefreshRate(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int refreshRate;
    int error;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if(error != 0)
    {
        return error;
    }    
    refreshRate = (controlRegister1 & 0x0380) >> 7;
    
    return refreshRate;
}

//------------------------------------------------------------------------------

int MLX90640_SetInterleavedMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int value;
    int error;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    
    if(error == 0)
    {
        value = (controlRegister1 & 0xEFFF);
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);        
    }    
    
    return error;
}

//------------------------------------------------------------------------------

int MLX90640_SetChessMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int value;
    int error;
        
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    
    if(error == 0)
    {
        value = (controlRegister1 | 0x1000);
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);        
    }    
    
    return error;
}

//------------------------------------------------------------------------------

int MLX90640_GetCurMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int modeRAM;
    int error;
    
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if(error != 0)
    {
        return error;
    }    
    modeRAM = (controlRegister1 & 0x1000) >> 12;
    
    return modeRAM; 
}

//------------------------------------------------------------------------------

void MLX90640_CalculateTo(const uint16_t *frameData, float emissivity, float tr, float *result)
{
    float vdd __attribute((annotate("scalar()")));
    float ta __attribute((annotate("scalar()")));
    float ta4 __attribute((annotate("scalar()")));
    float tr4 __attribute((annotate("scalar()")));
    float taTr __attribute((annotate("scalar()")));
    float gain __attribute((annotate("scalar()")));
    float irDataCP[2] __attribute((annotate("scalar()")));
    float irData __attribute((annotate("scalar()")));
    float alphaCompensated __attribute((annotate("scalar()")));
    uint8_t mode;
    int8_t ilPattern;
    int8_t chessPattern;
    int8_t pattern;
    int8_t conversionPattern;
    float Sx __attribute((annotate("scalar()")));
    float To __attribute((annotate("scalar()")));
    float alphaCorrR[4] __attribute((annotate("scalar()")));
    int8_t range;
    uint16_t subPage;
    
    subPage = frameData[833];
    vdd = MLX90640_GetVdd(frameData);
    ta = MLX90640_GetTa(frameData);
    ta4 = pow((ta + 273.15), (double)4);
    tr4 = pow((tr + 273.15), (double)4);
    taTr = tr4 - (tr4-ta4)/emissivity;
    
    alphaCorrR[0] = 1 / (1 + ksToGlobal[0] * 40);
    alphaCorrR[1] = 1 ;
    alphaCorrR[2] = (1 + ksToGlobal[2] * ctGlobal[2]);
    alphaCorrR[3] = alphaCorrR[2] * (1 + ksToGlobal[3] * (ctGlobal[3] - ctGlobal[2]));
    
//------------------------- Gain calculation -----------------------------------    
    gain = frameData[778];
    if(gain > 32767)
    {
        gain = gain - 65536;
    }
    
    gain = gainEEGlobal / gain;
  
//------------------------- To calculation -------------------------------------    
    mode = (frameData[832] & 0x1000) >> 5;
    
    irDataCP[0] = frameData[776];  
    irDataCP[1] = frameData[808];
    for( int i = 0; i < 2; i++)
    {
        if(irDataCP[i] > 32767)
        {
            irDataCP[i] = irDataCP[i] - 65536;
        }
        irDataCP[i] = irDataCP[i] * gain;
    }
    irDataCP[0] = irDataCP[0] - cpOffsetGlobal[0] * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    if( mode ==  calibrationModeEEGlobal)
    {
        irDataCP[1] = irDataCP[1] - cpOffsetGlobal[1] * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    }
    else
    {
      irDataCP[1] = irDataCP[1] - (cpOffsetGlobal[1] + ilChessCGlobal[0]) * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    }

    for( int pixelNumber = 0; pixelNumber < 768; pixelNumber++)
    {
        ilPattern = pixelNumber / 32 - (pixelNumber / 64) * 2; 
        chessPattern = ilPattern ^ (pixelNumber - (pixelNumber/2)*2); 
        conversionPattern = ((pixelNumber + 2) / 4 - (pixelNumber + 3) / 4 + (pixelNumber + 1) / 4 - pixelNumber / 4) * (1 - 2 * ilPattern);
        
        if(mode == 0)
        {
          pattern = ilPattern; 
        }
        else 
        {
          pattern = chessPattern; 
        }               
        
        if(pattern == frameData[833])
        {    
            irData = frameData[pixelNumber];
            if(irData > 32767)
            {
                irData = irData - 65536;
            }
            irData = irData * gain;
            
            irData = irData - offsetGlobal[pixelNumber]*(1 + ktaGlobal[pixelNumber]*(ta - 25))*(1 + kvGlobal[pixelNumber]*(vdd - 3.3));
            if(mode !=  calibrationModeEEGlobal)
            {
              irData = irData + ilChessCGlobal[2] * (2 * ilPattern - 1) - ilChessCGlobal[1] * conversionPattern;
            }
            
            irData = irData / emissivity;
    
            irData = irData - tgcGlobal * irDataCP[subPage];
            
            alphaCompensated = (alphaGlobal[pixelNumber] - tgcGlobal * cpAlphaGlobal[subPage])*(1 + KsTaGlobal * (ta - 25));
            
            Sx = pow((double)alphaCompensated, (double)3) * (irData + alphaCompensated * taTr);
            Sx = sqrt(sqrt(Sx)) * ksToGlobal[1];
            
            To = sqrt(sqrt(irData/(alphaCompensated * (1 - ksToGlobal[1] * 273.15) + Sx) + taTr)) - 273.15;
                    
            if(To < ctGlobal[1])
            {
                range = 0;
            }
            else if(To < ctGlobal[2])
            {
                range = 1;            
            }   
            else if(To < ctGlobal[3])
            {
                range = 2;            
            }
            else
            {
                range = 3;            
            }      
            
            To = sqrt(sqrt(irData / (alphaCompensated * alphaCorrR[range] * (1 + ksToGlobal[range] * (To - ctGlobal[range]))) + taTr)) - 273.15;
            
            result[pixelNumber] = To;
        }
    }
}

//------------------------------------------------------------------------------

void MLX90640_GetImage(const uint16_t *frameData, float *result)
{
    float vdd __attribute((annotate("scalar()")));
    float ta __attribute((annotate("scalar()")));
    float gain __attribute((annotate("scalar()")));
    float irDataCP[2] __attribute((annotate("scalar()")));
    float irData __attribute((annotate("scalar()")));
    float alphaCompensated __attribute((annotate("scalar()")));
    uint8_t mode;
    int8_t ilPattern;
    int8_t chessPattern;
    int8_t pattern;
    int8_t conversionPattern;
    float image __attribute((annotate("scalar()")));
    uint16_t subPage;
    
    subPage = frameData[833];
    vdd = MLX90640_GetVdd(frameData);
    ta = MLX90640_GetTa(frameData);
    
//------------------------- Gain calculation -----------------------------------    
    gain = frameData[778];
    if(gain > 32767)
    {
        gain = gain - 65536;
    }
    
    gain = gainEEGlobal / gain;
  
//------------------------- Image calculation -------------------------------------    
    mode = (frameData[832] & 0x1000) >> 5;
    
    irDataCP[0] = frameData[776];  
    irDataCP[1] = frameData[808];
    for( int i = 0; i < 2; i++)
    {
        if(irDataCP[i] > 32767)
        {
            irDataCP[i] = irDataCP[i] - 65536;
        }
        irDataCP[i] = irDataCP[i] * gain;
    }
    irDataCP[0] = irDataCP[0] - cpOffsetGlobal[0] * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    if( mode ==  calibrationModeEEGlobal)
    {
        irDataCP[1] = irDataCP[1] - cpOffsetGlobal[1] * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    }
    else
    {
      irDataCP[1] = irDataCP[1] - (cpOffsetGlobal[1] + ilChessCGlobal[0]) * (1 + cpKtaGlobal * (ta - 25)) * (1 + cpKvGlobal * (vdd - 3.3));
    }

    for( int pixelNumber = 0; pixelNumber < 768; pixelNumber++)
    {
        ilPattern = pixelNumber / 32 - (pixelNumber / 64) * 2; 
        chessPattern = ilPattern ^ (pixelNumber - (pixelNumber/2)*2); 
        conversionPattern = ((pixelNumber + 2) / 4 - (pixelNumber + 3) / 4 + (pixelNumber + 1) / 4 - pixelNumber / 4) * (1 - 2 * ilPattern);
        
        if(mode == 0)
        {
          pattern = ilPattern; 
        }
        else 
        {
          pattern = chessPattern; 
        }
        
        if(pattern == frameData[833])
        {    
            irData = frameData[pixelNumber];
            if(irData > 32767)
            {
                irData = irData - 65536;
            }
            irData = irData * gain;
            
            irData = irData - offsetGlobal[pixelNumber]*(1 + ktaGlobal[pixelNumber]*(ta - 25))*(1 + kvGlobal[pixelNumber]*(vdd - 3.3));
            if(mode !=  calibrationModeEEGlobal)
            {
              irData = irData + ilChessCGlobal[2] * (2 * ilPattern - 1) - ilChessCGlobal[1] * conversionPattern;
            }
            
            irData = irData - tgcGlobal * irDataCP[subPage];
            
            alphaCompensated = (alphaGlobal[pixelNumber] - tgcGlobal * cpAlphaGlobal[subPage])*(1 + KsTaGlobal * (ta - 25));
            
            image = irData/alphaCompensated;
            
            result[pixelNumber] = image;
        }
    }
}

//------------------------------------------------------------------------------

float MLX90640_GetVdd(const uint16_t *frameData)
{
    float vdd __attribute((annotate("scalar()")));
    float resolutionCorrection __attribute((annotate("scalar()")));

    int resolutionRAM;    
    
    vdd = frameData[810];
    if(vdd > 32767)
    {
        vdd = vdd - 65536;
    }
    resolutionRAM = (frameData[832] & 0x0C00) >> 10;
    resolutionCorrection = pow(2, (double)resolutionEEGlobal) / pow(2, (double)resolutionRAM);
    vdd = (resolutionCorrection * vdd - vdd25Global) / kVddGlobal + 3.3;
    
    return vdd;
}

//------------------------------------------------------------------------------

float MLX90640_GetTa(const uint16_t *frameData)
{
    float ptat __attribute((annotate("scalar()")));
    float ptatArt __attribute((annotate("scalar()")));
    float vdd __attribute((annotate("scalar()")));
    float ta __attribute((annotate("scalar()")));
    
    vdd = MLX90640_GetVdd(frameData);
    
    ptat = frameData[800];
    if(ptat > 32767)
    {
        ptat = ptat - 65536;
    }
    
    ptatArt = frameData[768];
    if(ptatArt > 32767)
    {
        ptatArt = ptatArt - 65536;
    }
    ptatArt = (ptat / (ptat * alphaPTATGlobal + ptatArt)) * pow(2, (double)18);
    
    ta = (ptatArt / (1 + KvPTATGlobal * (vdd - 3.3)) - vPTAT25Global);
    ta = ta / KtPTATGlobal + 25;
    
    return ta;
}

//------------------------------------------------------------------------------

int MLX90640_GetSubPageNumber(const uint16_t *frameData)
{
    return frameData[833];    

}    

//------------------------------------------------------------------------------

void ExtractVDDParameters(const uint16_t *eeData)
{
    int16_t kVdd;
    int16_t vdd25;
    
    kVdd = eeData[51];
    
    kVdd = (eeData[51] & 0xFF00) >> 8;
    if(kVdd > 127)
    {
        kVdd = kVdd - 256;
    }
    kVdd = 32 * kVdd;
    vdd25 = eeData[51] & 0x00FF;
    vdd25 = ((vdd25 - 256) << 5) - 8192;
    
    kVddGlobal = kVdd;
    vdd25Global = vdd25;
}

//------------------------------------------------------------------------------

void ExtractPTATParameters(const uint16_t *eeData)
{
    float KvPTAT __attribute((annotate("scalar()")));
    float KtPTAT __attribute((annotate("scalar()")));
    int16_t vPTAT25;
    float alphaPTAT __attribute((annotate("scalar()")));
    
    KvPTAT = (eeData[50] & 0xFC00) >> 10;
    if(KvPTAT > 31)
    {
        KvPTAT = KvPTAT - 64;
    }
    KvPTAT = KvPTAT/4096;
    
    KtPTAT = eeData[50] & 0x03FF;
    if(KtPTAT > 511)
    {
        KtPTAT = KtPTAT - 1024;
    }
    KtPTAT = KtPTAT/8;
    
    vPTAT25 = eeData[49];
    
    alphaPTAT = (eeData[16] & 0xF000) / pow(2, (double)14) + 8.0f;
    
    KvPTATGlobal = KvPTAT;
    KtPTATGlobal = KtPTAT;
    vPTAT25Global = vPTAT25;
    alphaPTATGlobal = alphaPTAT;
}

//------------------------------------------------------------------------------

void ExtractGainParameters(const uint16_t *eeData)
{
    int16_t gainEE;
    
    gainEE = eeData[48];
    if(gainEE > 32767)
    {
        gainEE = gainEE -65536;
    }
    
    gainEEGlobal = gainEE;
}

//------------------------------------------------------------------------------

void ExtractTgcParameters(const uint16_t *eeData)
{
    float tgc __attribute((annotate("scalar()")));
    tgc = eeData[60] & 0x00FF;
    if(tgc > 127)
    {
        tgc = tgc - 256;
    }
    tgc = tgc / 32.0f;

    tgcGlobal = tgc;
}

//------------------------------------------------------------------------------

void ExtractResolutionParameters(const uint16_t *eeData)
{
    uint8_t resolutionEE;
    resolutionEE = (eeData[56] & 0x3000) >> 12;    
    
    resolutionEEGlobal = resolutionEE;
}

//------------------------------------------------------------------------------

void ExtractKsTaParameters(const uint16_t *eeData)
{
    float KsTa;
    KsTa = (eeData[60] & 0xFF00) >> 8;
    if(KsTa > 127)
    {
        KsTa = KsTa -256;
    }
    KsTa = KsTa / 8192.0f;
    
    KsTaGlobal = KsTa;
}

//------------------------------------------------------------------------------

void ExtractKsToParameters(const uint16_t *eeData)
{
    int KsToScale;
    int8_t step;
    
    step = ((eeData[63] & 0x3000) >> 12) * 10;
    
    ctGlobal[0] = -40;
    ctGlobal[1] = 0;
    ctGlobal[2] = (eeData[63] & 0x00F0) >> 4;
    ctGlobal[3] = (eeData[63] & 0x0F00) >> 8;
    
    ctGlobal[2] = ctGlobal[2]*step;
    ctGlobal[3] = ctGlobal[2] + ctGlobal[3]*step;
    
    KsToScale = (eeData[63] & 0x000F) + 8;
    KsToScale = 1 << KsToScale;
    
    ksToGlobal[0] = eeData[61] & 0x00FF;
    ksToGlobal[1] = (eeData[61] & 0xFF00) >> 8;
    ksToGlobal[2] = eeData[62] & 0x00FF;
    ksToGlobal[3] = (eeData[62] & 0xFF00) >> 8;
    
    
    for(int i = 0; i < 4; i++)
    {
        if(ksToGlobal[i] > 127)
        {
            ksToGlobal[i] = ksToGlobal[i] -256;
        }
        ksToGlobal[i] = ksToGlobal[i] / KsToScale;
    } 
}

//------------------------------------------------------------------------------

void ExtractAlphaParameters(const uint16_t *eeData)
{
    int accRow[24];
    int accColumn[32];
    int p = 0;
    int alphaRef;
    uint8_t alphaScale;
    uint8_t accRowScale;
    uint8_t accColumnScale;
    uint8_t accRemScale;
    

    accRemScale = eeData[32] & 0x000F;
    accColumnScale = (eeData[32] & 0x00F0) >> 4;
    accRowScale = (eeData[32] & 0x0F00) >> 8;
    alphaScale = ((eeData[32] & 0xF000) >> 12) + 30;
    alphaRef = eeData[33];
    
    for(int i = 0; i < 6; i++)
    {
        p = i * 4;
        accRow[p + 0] = (eeData[34 + i] & 0x000F);
        accRow[p + 1] = (eeData[34 + i] & 0x00F0) >> 4;
        accRow[p + 2] = (eeData[34 + i] & 0x0F00) >> 8;
        accRow[p + 3] = (eeData[34 + i] & 0xF000) >> 12;
    }
    
    for(int i = 0; i < 24; i++)
    {
        if (accRow[i] > 7)
        {
            accRow[i] = accRow[i] - 16;
        }
    }
    
    for(int i = 0; i < 8; i++)
    {
        p = i * 4;
        accColumn[p + 0] = (eeData[40 + i] & 0x000F);
        accColumn[p + 1] = (eeData[40 + i] & 0x00F0) >> 4;
        accColumn[p + 2] = (eeData[40 + i] & 0x0F00) >> 8;
        accColumn[p + 3] = (eeData[40 + i] & 0xF000) >> 12;
    }
    
    for(int i = 0; i < 32; i ++)
    {
        if (accColumn[i] > 7)
        {
            accColumn[i] = accColumn[i] - 16;
        }
    }

    for(int i = 0; i < 24; i++)
    {
        for(int j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            alphaGlobal[p] = (eeData[64 + p] & 0x03F0) >> 4;
            if (alphaGlobal[p] > 31)
            {
                alphaGlobal[p] = alphaGlobal[p] - 64;
            }
            alphaGlobal[p] = alphaGlobal[p]*(1 << accRemScale);
            alphaGlobal[p] = (alphaRef + (accRow[i] << accRowScale) + (accColumn[j] << accColumnScale) + alphaGlobal[p]);
            alphaGlobal[p] = alphaGlobal[p] / pow(2,(double)alphaScale);
        }
    }
}

//------------------------------------------------------------------------------

void ExtractOffsetParameters(const uint16_t *eeData)
{
    int occRow[24];
    int occColumn[32];
    int p = 0;
    int16_t offsetRef;
    uint8_t occRowScale;
    uint8_t occColumnScale;
    uint8_t occRemScale;
    

    occRemScale = (eeData[16] & 0x000F);
    occColumnScale = (eeData[16] & 0x00F0) >> 4;
    occRowScale = (eeData[16] & 0x0F00) >> 8;
    offsetRef = eeData[17];
    if (offsetRef > 32767)
    {
        offsetRef = offsetRef - 65536;
    }
    
    for(int i = 0; i < 6; i++)
    {
        p = i * 4;
        occRow[p + 0] = (eeData[18 + i] & 0x000F);
        occRow[p + 1] = (eeData[18 + i] & 0x00F0) >> 4;
        occRow[p + 2] = (eeData[18 + i] & 0x0F00) >> 8;
        occRow[p + 3] = (eeData[18 + i] & 0xF000) >> 12;
    }
    
    for(int i = 0; i < 24; i++)
    {
        if (occRow[i] > 7)
        {
            occRow[i] = occRow[i] - 16;
        }
    }
    
    for(int i = 0; i < 8; i++)
    {
        p = i * 4;
        occColumn[p + 0] = (eeData[24 + i] & 0x000F);
        occColumn[p + 1] = (eeData[24 + i] & 0x00F0) >> 4;
        occColumn[p + 2] = (eeData[24 + i] & 0x0F00) >> 8;
        occColumn[p + 3] = (eeData[24 + i] & 0xF000) >> 12;
    }
    
    for(int i = 0; i < 32; i ++)
    {
        if (occColumn[i] > 7)
        {
            occColumn[i] = occColumn[i] - 16;
        }
    }

    for(int i = 0; i < 24; i++)
    {
        for(int j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            offsetGlobal[p] = (eeData[64 + p] & 0xFC00) >> 10;
            if (offsetGlobal[p] > 31)
            {
                offsetGlobal[p] = offsetGlobal[p] - 64;
            }
            offsetGlobal[p] = offsetGlobal[p]*(1 << occRemScale);
            offsetGlobal[p] = (offsetRef + (occRow[i] << occRowScale) + (occColumn[j] << occColumnScale) + offsetGlobal[p]);
        }
    }
}

//------------------------------------------------------------------------------

void ExtractKtaPixelParameters(const uint16_t *eeData)
{
    int p = 0;
    int8_t KtaRC[4];
    int8_t KtaRoCo;
    int8_t KtaRoCe;
    int8_t KtaReCo;
    int8_t KtaReCe;
    uint8_t ktaScale1;
    uint8_t ktaScale2;
    uint8_t split;

    KtaRoCo = (eeData[54] & 0xFF00) >> 8;
    if (KtaRoCo > 127)
    {
        KtaRoCo = KtaRoCo - 256;
    }
    KtaRC[0] = KtaRoCo;
    
    KtaReCo = (eeData[54] & 0x00FF);
    if (KtaReCo > 127)
    {
        KtaReCo = KtaReCo - 256;
    }
    KtaRC[2] = KtaReCo;
      
    KtaRoCe = (eeData[55] & 0xFF00) >> 8;
    if (KtaRoCe > 127)
    {
        KtaRoCe = KtaRoCe - 256;
    }
    KtaRC[1] = KtaRoCe;
      
    KtaReCe = (eeData[55] & 0x00FF);
    if (KtaReCe > 127)
    {
        KtaReCe = KtaReCe - 256;
    }
    KtaRC[3] = KtaReCe;
  
    ktaScale1 = ((eeData[56] & 0x00F0) >> 4) + 8;
    ktaScale2 = (eeData[56] & 0x000F);

    for(int i = 0; i < 24; i++)
    {
        for(int j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            split = 2*(p/32 - (p/64)*2) + p%2;
            ktaGlobal[p] = (eeData[64 + p] & 0x000E) >> 1;
            if (ktaGlobal[p] > 3)
            {
                ktaGlobal[p] = ktaGlobal[p] - 8;
            }
            ktaGlobal[p] = ktaGlobal[p] * (1 << ktaScale2);
            ktaGlobal[p] = KtaRC[split] + ktaGlobal[p];
            ktaGlobal[p] = ktaGlobal[p] / pow(2,(double)ktaScale1);
        }
    }
}

//------------------------------------------------------------------------------

void ExtractKvPixelParameters(const uint16_t *eeData)
{
    int p = 0;
    int8_t KvT[4];
    int8_t KvRoCo;
    int8_t KvRoCe;
    int8_t KvReCo;
    int8_t KvReCe;
    uint8_t kvScale;
    uint8_t split;

    KvRoCo = (eeData[52] & 0xF000) >> 12;
    if (KvRoCo > 7)
    {
        KvRoCo = KvRoCo - 16;
    }
    KvT[0] = KvRoCo;
    
    KvReCo = (eeData[52] & 0x0F00) >> 8;
    if (KvReCo > 7)
    {
        KvReCo = KvReCo - 16;
    }
    KvT[2] = KvReCo;
      
    KvRoCe = (eeData[52] & 0x00F0) >> 4;
    if (KvRoCe > 7)
    {
        KvRoCe = KvRoCe - 16;
    }
    KvT[1] = KvRoCe;
      
    KvReCe = (eeData[52] & 0x000F);
    if (KvReCe > 7)
    {
        KvReCe = KvReCe - 16;
    }
    KvT[3] = KvReCe;
  
    kvScale = (eeData[56] & 0x0F00) >> 8;


    for(int i = 0; i < 24; i++)
    {
        for(int j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            split = 2*(p/32 - (p/64)*2) + p%2;
            kvGlobal[p] = KvT[split];
            kvGlobal[p] = kvGlobal[p] / pow(2,(double)kvScale);
        }
    }
}

//------------------------------------------------------------------------------

void ExtractCPParameters(const uint16_t *eeData)
{
    float alphaSP[2] __attribute((annotate("scalar()")));
    int16_t offsetSP[2];
    float cpKv __attribute((annotate("scalar()")));
    float cpKta __attribute((annotate("scalar()")));
    uint8_t alphaScale;
    uint8_t ktaScale1;
    uint8_t kvScale;

    alphaScale = ((eeData[32] & 0xF000) >> 12) + 27;
    
    offsetSP[0] = (eeData[58] & 0x03FF);
    if (offsetSP[0] > 511)
    {
        offsetSP[0] = offsetSP[0] - 1024;
    }
    
    offsetSP[1] = (eeData[58] & 0xFC00) >> 10;
    if (offsetSP[1] > 31)
    {
        offsetSP[1] = offsetSP[1] - 64;
    }
    offsetSP[1] = offsetSP[1] + offsetSP[0]; 
    
    alphaSP[0] = (eeData[57] & 0x03FF);
    if (alphaSP[0] > 511)
    {
        alphaSP[0] = alphaSP[0] - 1024;
    }
    alphaSP[0] = alphaSP[0] /  pow(2,(double)alphaScale);
    
    alphaSP[1] = (eeData[57] & 0xFC00) >> 10;
    if (alphaSP[1] > 31)
    {
        alphaSP[1] = alphaSP[1] - 64;
    }
    alphaSP[1] = (1 + alphaSP[1]/128) * alphaSP[0];
    
    cpKta = (eeData[59] & 0x00FF);
    if (cpKta > 127)
    {
        cpKta = cpKta - 256;
    }
    ktaScale1 = ((eeData[56] & 0x00F0) >> 4) + 8;    
    cpKtaGlobal = cpKta / pow(2,(double)ktaScale1);
    
    cpKv = (eeData[59] & 0xFF00) >> 8;
    if (cpKv > 127)
    {
        cpKv = cpKv - 256;
    }
    kvScale = (eeData[56] & 0x0F00) >> 8;
    cpKvGlobal = cpKv / pow(2,(double)kvScale);
       
    cpAlphaGlobal[0] = alphaSP[0];
    cpAlphaGlobal[1] = alphaSP[1];
    cpOffsetGlobal[0] = offsetSP[0];
    cpOffsetGlobal[1] = offsetSP[1];
}

//------------------------------------------------------------------------------

void ExtractCILCParameters(const uint16_t *eeData)
{
    float ilChessC[3] __attribute((annotate("scalar()")));
    uint8_t calibrationModeEE;
    
    calibrationModeEE = (eeData[10] & 0x0800) >> 4;
    calibrationModeEE = calibrationModeEE ^ 0x80;

    ilChessC[0] = (eeData[53] & 0x003F);
    if (ilChessC[0] > 31)
    {
        ilChessC[0] = ilChessC[0] - 64;
    }
    ilChessC[0] = ilChessC[0] / 16.0f;
    
    ilChessC[1] = (eeData[53] & 0x07C0) >> 6;
    if (ilChessC[1] > 15)
    {
        ilChessC[1] = ilChessC[1] - 32;
    }
    ilChessC[1] = ilChessC[1] / 2.0f;
    
    ilChessC[2] = (eeData[53] & 0xF800) >> 11;
    if (ilChessC[2] > 15)
    {
        ilChessC[2] = ilChessC[2] - 32;
    }
    ilChessC[2] = ilChessC[2] / 8.0f;
    
    calibrationModeEEGlobal = calibrationModeEE;
    ilChessCGlobal[0] = ilChessC[0];
    ilChessCGlobal[1] = ilChessC[1];
    ilChessCGlobal[2] = ilChessC[2];
}

//------------------------------------------------------------------------------

int ExtractDeviatingPixels(const uint16_t *eeData)
{
    uint16_t pixCnt = 0;
    uint16_t brokenPixCnt = 0;
    uint16_t outlierPixCnt = 0;
    int warn = 0;
    int i;
    
    for(pixCnt = 0; pixCnt<5; pixCnt++)
    {
        brokenPixelsGlobal[pixCnt] = 0xFFFF;
        outlierPixelsGlobal[pixCnt] = 0xFFFF;
    }
        
    pixCnt = 0;    
    while (pixCnt < 768 && brokenPixCnt < 5 && outlierPixCnt < 5)
    {
        if(eeData[pixCnt+64] == 0)
        {
            brokenPixelsGlobal[brokenPixCnt] = pixCnt;
            brokenPixCnt = brokenPixCnt + 1;
        }    
        else if((eeData[pixCnt+64] & 0x0001) != 0)
        {
            outlierPixelsGlobal[outlierPixCnt] = pixCnt;
            outlierPixCnt = outlierPixCnt + 1;
        }    
        
        pixCnt = pixCnt + 1;
        
    } 
    
    if(brokenPixCnt > 4)  
    {
        warn = -3;
    }         
    else if(outlierPixCnt > 4)  
    {
        warn = -4;
    }
    else if((brokenPixCnt + outlierPixCnt) > 4)  
    {
        warn = -5;
    } 
    else
    {
        for(pixCnt=0; pixCnt<brokenPixCnt; pixCnt++)
        {
            for(i=pixCnt+1; i<brokenPixCnt; i++)
            {
                warn = CheckAdjacentPixels(brokenPixelsGlobal[pixCnt],brokenPixelsGlobal[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        }
        
        for(pixCnt=0; pixCnt<outlierPixCnt; pixCnt++)
        {
            for(i=pixCnt+1; i<outlierPixCnt; i++)
            {
                warn = CheckAdjacentPixels(outlierPixelsGlobal[pixCnt],outlierPixelsGlobal[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        } 
        
        for(pixCnt=0; pixCnt<brokenPixCnt; pixCnt++)
        {
            for(i=0; i<outlierPixCnt; i++)
            {
                warn = CheckAdjacentPixels(brokenPixelsGlobal[pixCnt],outlierPixelsGlobal[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        }    
        
    }    
    
    
    return warn;
       
}

//------------------------------------------------------------------------------

 int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2)
 {
     int pixPosDif;
     
     pixPosDif = pix1 - pix2;
     if(pixPosDif > -34 && pixPosDif < -30)
     {
         return -6;
     } 
     if(pixPosDif > -2 && pixPosDif < 2)
     {
         return -6;
     } 
     if(pixPosDif > 30 && pixPosDif < 34)
     {
         return -6;
     }
     
     return 0;    
 }
 
 //------------------------------------------------------------------------------
 
 int CheckEEPROMValid(const uint16_t *eeData)  
 {
     int deviceSelect;
     deviceSelect = eeData[10] & 0x0040;
     if(deviceSelect == 0)
     {
         return 0;
     }
     
     return -7;    
 }     
 }   
