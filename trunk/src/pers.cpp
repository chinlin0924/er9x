/*
 * Author - Erez Raviv <erezraviv@gmail.com>
 *
 * Based on th9x -> http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "er9x.h"


EFile theFile;  //used for any file operation
EFile theFile2; //sometimes we need two files

#define FILE_TYP_GENERAL 1
#define FILE_TYP_MODEL   2
#define partCopy(sizeDst,sizeSrc)                         \
      pSrc -= (sizeSrc);                                  \
      pDst -= (sizeDst);                                  \
      memmove(pDst, pSrc, (sizeSrc));                     \
      memset (pDst+(sizeSrc), 0,  (sizeDst)-(sizeSrc));
#define fullCopy(size) partCopy(size,size)
void generalDefault()
{
  memset(&g_eeGeneral,0,sizeof(g_eeGeneral));
  g_eeGeneral.myVers   =  2;
  g_eeGeneral.currModel=  0;
  g_eeGeneral.contrast = 30;
  g_eeGeneral.vBatWarn = 90;
  g_eeGeneral.stickMode=  1;
  int16_t sum=0;
  for (int i = 0; i < 4; ++i) {
    sum += g_eeGeneral.calibMid[i]     = 0x200;
    sum += g_eeGeneral.calibSpanNeg[i] = 0x180;
    sum += g_eeGeneral.calibSpanPos[i] = 0x180;
  }
  g_eeGeneral.chkSum = sum;
}
bool eeLoadGeneral()
{
  theFile.openRd(FILE_GENERAL);
  uint8_t sz = theFile.readRlc((uint8_t*)&g_eeGeneral, sizeof(EEGeneral));
  uint16_t sum=0;
  if( sz == sizeof(EEGeneral) && g_eeGeneral.myVers == 2){
    for(int i=0; i<12;i++) sum+=g_eeGeneral.calibMid[i];
    return g_eeGeneral.chkSum == sum;
  }
  return false;
}

void modelDefault(uint8_t id)
{
  memset(&g_model, 0, sizeof(g_model));
  strcpy_P(g_model.name,PSTR("MODEL     "));
  g_model.name[5]='0'+(id+1)/10;
  g_model.name[6]='0'+(id+1)%10;
  g_model.mdVers = MDVERS;
  
  for(uint8_t i= 0; i<4; i++){
    g_model.mixData[i].destCh = i+1;
    g_model.mixData[i].srcRaw = i+1;
    g_model.mixData[i].weight = 100;
  }
}
void eeLoadModelName(uint8_t id,char*buf,uint8_t len)
{
  if(id<MAX_MODELS)
  {
    //eeprom_read_block(buf,(void*)modelEeOfs(id),sizeof(g_model.name));
    theFile.openRd(FILE_MODEL(id));
    memset(buf,' ',len);
    if(theFile.readRlc((uint8_t*)buf,sizeof(g_model.name)) == sizeof(g_model.name) )
    {
      uint16_t sz=theFile.size();
      buf+=len;
      while(sz){ --buf; *buf='0'+sz%10; sz/=10;}
    }
  }
}
int8_t trimRevert(int16_t val)
{
  uint8_t idx = 0;
  bool    neg = val<0; val=abs(val);
  while(val>0){
    idx++;
    val-=idx;
  }
  return neg ? -idx : idx;
}

void load_ver9(uint8_t id)
{
  ModelData_r9 g_model_r9;
  theFile.openRd(FILE_MODEL(id));
  uint16_t sz = theFile.readRlc((uint8_t*)&g_model_r9, sizeof(g_model_r9));
  
  modelDefault(id);
  if(sz != sizeof(ModelData_r9)) return;
  
  g_model.thrTrim = g_model_r9.thrTrim;
  g_model.trimInc = g_model_r9.trimInc;
  g_model.tcutSW  = g_model_r9.tcutSW;
  
  memcpy(&g_model,           &g_model_r9,           16);
  memcpy(&g_model.mixData,   &g_model_r9.mixData,   sizeof(MixData)*MAX_MIXERS);
  memcpy(&g_model.limitData, &g_model_r9.limitData, sizeof(LimitData)*NUM_CHNOUT);
  memcpy(&g_model.curves5,   &g_model_r9.curves5,   (MAX_CURVE5*5)+(MAX_CURVE9*9));

  for (uint8_t i=0; i<4; i++){
    g_model.trim[i] = g_model_r9.trimData[i].trim;
    g_model.expoData[i].drSw1     = g_model_r9.expoData[i].drSw;
    g_model.expoData[i].expo[DR_HIGH][DR_EXPO][DR_RIGHT]   = g_model_r9.expoData[i].expNorm;
    g_model.expoData[i].expo[DR_HIGH][DR_EXPO][DR_LEFT]    = g_model_r9.expoData[i].expNorm;
    g_model.expoData[i].expo[DR_MID][DR_EXPO][DR_RIGHT]   = g_model_r9.expoData[i].expDr;
    g_model.expoData[i].expo[DR_MID][DR_EXPO][DR_LEFT]    = g_model_r9.expoData[i].expDr;
    g_model.expoData[i].expo[DR_HIGH][DR_WEIGHT][DR_RIGHT] = g_model_r9.expoData[i].expNormWeight;
    g_model.expoData[i].expo[DR_HIGH][DR_WEIGHT][DR_LEFT]  = g_model_r9.expoData[i].expNormWeight;
    g_model.expoData[i].expo[DR_MID][DR_WEIGHT][DR_RIGHT] = g_model_r9.expoData[i].expSwWeight;
    g_model.expoData[i].expo[DR_MID][DR_WEIGHT][DR_LEFT]  = g_model_r9.expoData[i].expSwWeight;
  }
  g_model.mdVers = MDVERS;
}

void load_ver14(uint8_t id)
{
  ModelData_r14 g_model_r14;
  theFile.openRd(FILE_MODEL(id));
  uint16_t sz = theFile.readRlc((uint8_t*)&g_model_r14, sizeof(g_model_r14));
  
  modelDefault(id);
  if(sz != sizeof(ModelData_r14)) return;
  
  memcpy(&g_model, &g_model_r14, 25);
  memcpy(&g_model.trim, &g_model_r14.trim, (4+MAX_CURVE5*5+MAX_CURVE9*9));
  memset(&g_model.expoData, 0, sizeof(ExpoData));
  
  for (uint8_t i=0; i<4; i++){
    g_model.expoData[i].drSw1     = g_model_r14.expoData[i].drSw;
    g_model.expoData[i].expo[DR_HIGH][DR_EXPO][DR_RIGHT]   = g_model_r14.expoData[i].expNormR;
    g_model.expoData[i].expo[DR_HIGH][DR_EXPO][DR_LEFT]    = g_model_r14.expoData[i].expNormL;
    g_model.expoData[i].expo[DR_MID][DR_EXPO][DR_RIGHT]   = g_model_r14.expoData[i].expDrR;
    g_model.expoData[i].expo[DR_MID][DR_EXPO][DR_LEFT]    = g_model_r14.expoData[i].expDrL;
    g_model.expoData[i].expo[DR_HIGH][DR_WEIGHT][DR_RIGHT] = g_model_r14.expoData[i].expNormWeightR;
    g_model.expoData[i].expo[DR_HIGH][DR_WEIGHT][DR_LEFT]  = g_model_r14.expoData[i].expNormWeightL;
    g_model.expoData[i].expo[DR_MID][DR_WEIGHT][DR_RIGHT] = g_model_r14.expoData[i].expSwWeightR;
    g_model.expoData[i].expo[DR_MID][DR_WEIGHT][DR_LEFT]  = g_model_r14.expoData[i].expSwWeightL;
  }
  
  g_model.mdVers = MDVERS;
}

void load_ver22(uint8_t id)
{
  ModelData_r22 g_model_r22;
  theFile.openRd(FILE_MODEL(id));
  uint16_t sz = theFile.readRlc((uint8_t*)&g_model_r22, sizeof(g_model_r22));
  
  modelDefault(id);
  if(sz != sizeof(ModelData_r22)) return;
  
  memcpy(&g_model, &g_model_r22, 25);
  memcpy(&g_model.limitData, &g_model_r22.limitData, sizeof(g_model_r22.limitData)+  \
                                                     sizeof(g_model_r22.expoData)+   \
                                                     sizeof(g_model_r22.trim)+       \
                                                     sizeof(g_model_r22.curves5)+    \
                                                     sizeof(g_model_r22.curves9));
                                                     
  for(uint8_t i=0;i<25;i++){
    g_model.mixData[i].destCh    = g_model_r22.mixData[i].destCh;
    g_model.mixData[i].srcRaw    = g_model_r22.mixData[i].srcRaw;
    g_model.mixData[i].carryTrim = g_model_r22.mixData[i].carryTrim;
    g_model.mixData[i].weight    = g_model_r22.mixData[i].weight;
    g_model.mixData[i].swtch     = g_model_r22.mixData[i].swtch;
    g_model.mixData[i].curve     = g_model_r22.mixData[i].curve;
    g_model.mixData[i].speedUp   = g_model_r22.mixData[i].speedUp;
    g_model.mixData[i].speedDown = g_model_r22.mixData[i].speedDown;
  }
  
  g_model.mdVers = MDVERS;
}


void eeLoadModel(uint8_t id)
{
  if(id<MAX_MODELS)
  {
    theFile.openRd(FILE_MODEL(id));
    uint16_t sz = theFile.readRlc((uint8_t*)&g_model, sizeof(g_model));
    
    switch (g_model.mdVers){
      case MDVERS_r9:
        load_ver9(id);
        break;
      case MDVERS_r14:
        load_ver14(id);
        break;
      case MDVERS_r22:
        load_ver22(id);
        break;
      default:
        if(sz != sizeof(ModelData)) modelDefault(id);
        break;
    }
  }
}

bool eeDuplicateModel(uint8_t id)
{
  uint8_t i;
  for( i=id+1; i<MAX_MODELS; i++)
  {
    if(! EFile::exists(FILE_MODEL(i))) break;
  }
  if(i==MAX_MODELS) return false; //no free space in directory left

  theFile.openRd(FILE_MODEL(id));
  theFile2.create(FILE_MODEL(i),FILE_TYP_MODEL,200);
  uint8_t buf[15];
  uint8_t l;
  while((l=theFile.read(buf,15)))
  {
    theFile2.write(buf,l);
    wdt_reset();
  }
  theFile2.closeTrunc();
  //todo error handling
  return true;
}
void eeReadAll()
{
  if(!EeFsOpen()  ||
     EeFsck() < 0 ||
     !eeLoadGeneral()
  )
  {
    alert(PSTR("Bad EEprom Data"));
    EeFsFormat();
    //alert(PSTR("format ok"));
    generalDefault();
    // alert(PSTR("default ok"));

    uint16_t sz = theFile.writeRlc(FILE_GENERAL,FILE_TYP_GENERAL,(uint8_t*)&g_eeGeneral,sizeof(EEGeneral),200);
    if(sz!=sizeof(EEGeneral)) alert(PSTR("genwrite error"));

    modelDefault(0);
    //alert(PSTR("modef ok"));
    theFile.writeRlc(FILE_MODEL(0),FILE_TYP_MODEL,(uint8_t*)&g_model,sizeof(g_model),200);
    //alert(PSTR("modwrite ok"));

  }
  eeLoadModel(g_eeGeneral.currModel);
}


static uint8_t  s_eeDirtyMsk;
static uint16_t s_eeDirtyTime10ms;
void eeDirty(uint8_t msk)
{
  if(!msk) return;
  s_eeDirtyMsk      |= msk;
  s_eeDirtyTime10ms  = g_tmr10ms;
}
#define WRITE_DELAY_10MS 100
void eeCheck(bool immediately)
{
  uint8_t msk  = s_eeDirtyMsk;
  if(!msk) return;
  if( !immediately && ((g_tmr10ms - s_eeDirtyTime10ms) < WRITE_DELAY_10MS)) return;
  s_eeDirtyMsk = 0;
  if(msk & EE_GENERAL){
    if(theFile.writeRlc(FILE_TMP, FILE_TYP_GENERAL, (uint8_t*)&g_eeGeneral,
                        sizeof(EEGeneral),20) == sizeof(EEGeneral))
    {
      EFile::swap(FILE_GENERAL,FILE_TMP);
    }else{
      if(theFile.errno()==ERR_TMO){
        s_eeDirtyMsk |= EE_GENERAL; //try again
        s_eeDirtyTime10ms = g_tmr10ms - WRITE_DELAY_10MS;
#ifdef SIM
        printf("writing aborted GENERAL\n");
#endif
      }else{
        alert(PSTR("EEPROM overflow"));
      }
    }
    //first finish GENERAL, then MODEL !!avoid Toggle effect
  }
  else if(msk & EE_MODEL){
    if(theFile.writeRlc(FILE_TMP, FILE_TYP_MODEL, (uint8_t*)&g_model,
                        sizeof(g_model),20) == sizeof(g_model))
    {
      EFile::swap(FILE_MODEL(g_eeGeneral.currModel),FILE_TMP);
    }else{
      if(theFile.errno()==ERR_TMO){
        s_eeDirtyMsk |= EE_MODEL; //try again
        s_eeDirtyTime10ms = g_tmr10ms - WRITE_DELAY_10MS;
#ifdef SIM
        printf("writing aborted MODEL\n");
#endif
      }else{
        alert(PSTR("EEPROM overflow"));
      }
    }
  }
  beepWarn1();
}