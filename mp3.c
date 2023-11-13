#include "Adafruit_MP3.h"
#include <SPI.h>
#include <SD.h>

#include <Adafruit_ZeroDMA.h>
#include "utility/dma.h"

#define VOLUME_MAX 2047
const char *filename = "test3.mp3";
const int chipSelect = 10;

Adafruit_ZeroDMA leftDMA;
Adafruit_ZeroDMA rightDMA;
ZeroDMAstatus    stat; 
File dataFile;
Adafruit_MP3_DMA player;
DmacDescriptor *desc;

int16_t *ping, *pong;

int getMoreData(uint8_t *writeHere, int thisManyBytes){
  int bytesRead = 0;
  while(dataFile.available() && bytesRead < thisManyBytes){
    *writeHere = dataFile.read();
    writeHere++;
    bytesRead++;
  }
  return bytesRead;
}

void decodeCallback(int16_t *data, int len){
  for(int i=0; i<len; i++){
    int val = map(*data, -32768, 32767, 0, VOLUME_MAX);
    *data++ = val;
  }
}

void dma_callback(Adafruit_ZeroDMA *dma) {
  
  digitalWrite(13, HIGH);
  if(player.fill()){
    leftDMA.abort();
    rightDMA.abort();
  }
  digitalWrite(13, LOW);
}

void doNothing(Adafruit_ZeroDMA *dma) {
  
}

void initMonoDMA(){
  leftDMA.setTrigger(MP3_DMA_TRIGGER);
  leftDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = leftDMA.allocate();

  player.getBuffers(&ping, &pong);

  desc = leftDMA.addDescriptor(
    ping,                   
    (void *)(&DAC->DATA[0]), 
    MP3_OUTBUF_SIZE,
    DMA_BEAT_SIZE_HWORD,             
    true,                            
    false);
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = leftDMA.addDescriptor(
    pong,                   
    (void *)(&DAC->DATA[0]), 
    MP3_OUTBUF_SIZE,
    DMA_BEAT_SIZE_HWORD,               
    true,                            
    false);                   
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  leftDMA.loop(true);
  leftDMA.setCallback(dma_callback);

  rightDMA.setTrigger(MP3_DMA_TRIGGER);
  rightDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = rightDMA.allocate();

  desc = rightDMA.addDescriptor(
    ping + 1,                   
    (void *)(&DAC->DATA[1]), 
    MP3_OUTBUF_SIZE,                     
    DMA_BEAT_SIZE_HWORD,               
    true,                             
    false);                         
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = rightDMA.addDescriptor(
    pong + 1,                    
    (void *)(&DAC->DATA[1]),
    MP3_OUTBUF_SIZE,                      
    DMA_BEAT_SIZE_HWORD,              
    true,                            
    false);                           
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  rightDMA.loop(true);
  rightDMA.setCallback(doNothing);
}

void initStereoDMA(){
  leftDMA.setTrigger(MP3_DMA_TRIGGER);
  leftDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = leftDMA.allocate();

  player.getBuffers(&ping, &pong);

  desc = leftDMA.addDescriptor(
    ping,                    
    (void *)(&DAC->DATA[0]), 
    MP3_OUTBUF_SIZE >> 1,                  
    DMA_BEAT_SIZE_HWORD,               
    true,                            
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           

  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = leftDMA.addDescriptor(
    pong,                   
    (void *)(&DAC->DATA[0]),
    MP3_OUTBUF_SIZE >> 1,                      
    DMA_BEAT_SIZE_HWORD,              
    true,                           
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                         
    
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  leftDMA.loop(true);
  leftDMA.setCallback(dma_callback);

  rightDMA.setTrigger(MP3_DMA_TRIGGER);
  rightDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = rightDMA.allocate();

  desc = rightDMA.addDescriptor(
    ping + 1,                   
    (void *)(&DAC->DATA[1]),
    MP3_OUTBUF_SIZE >> 1,                 
    DMA_BEAT_SIZE_HWORD,             
    true,                            
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                       
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = rightDMA.addDescriptor(
    pong + 1,                    
    (void *)(&DAC->DATA[1]), 
    MP3_OUTBUF_SIZE >> 1,                    
    DMA_BEAT_SIZE_HWORD,           
    true,                            
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  rightDMA.loop(true);
  rightDMA.setCallback(doNothing);
}

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  while(!Serial);

  
  while (!SD.begin(12000000, chipSelect)) {
    Serial.println("Card failed, or not present");
    delay(2000);
  }
  Serial.println("card initialized.");

  dataFile = SD.open(filename);
  if(!dataFile){
    Serial.println("could not open file!");
    while(1);
  }

  analogWrite(A0, 2048);
  analogWrite(A1, 2048);

  player.begin();

  player.setBufferCallback(getMoreData);

  player.setDecodeCallback(decodeCallback);

  player.play();

  if(player.numChannels == 1)
    initMonoDMA(); 
    
  else if(player.numChannels == 2)
    initStereoDMA(); 
  
  else{
    Serial.println("only mono and stereo files are supported");
    while(1);
  }

  rightDMA.startJob();
  leftDMA.startJob();
}

void loop() {
}
