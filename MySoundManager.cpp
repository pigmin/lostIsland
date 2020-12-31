// Implement a 8 note polyphonic midi player  :-)
//
// Music data is read from memory.  The "Miditones" program is used to
// convert from a MIDI file to this compact format.
//
// This example code is in the public domain.
 
#include <Audio.h>
#include <Wire.h>

#include "MySoundManager.h"




unsigned char *sp = NULL;
unsigned char *_currentMusic = NULL;
bool bMusicEnded = true;

// Four mixers are needed to handle 8 channels of music
AudioMixer4     mixerFX;
//AudioMixer4     mixerFX2;
AudioMixer4     mixerMOD;


#define AMPLITUDE (0.2)
#ifdef MIDI_MUSIC

// Create 8 waveforms, one for each MIDI channel
AudioSynthWaveform sine0, sine1, sine2, sine3;
AudioSynthWaveform sine4, sine5, sine6, sine7;
AudioSynthWaveform *waves[8] = {
  &sine0, &sine1, &sine2, &sine3,
  &sine4, &sine5, &sine6, &sine7
};


// allocate a wave type to each channel.
// The types used and their order is purely arbitrary.
short wave_type[8] = {
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
};


// Each waveform will be shaped by an envelope
AudioEffectEnvelope env0, env1, env2, env3;
AudioEffectEnvelope env4, env5, env6, env7;
AudioEffectEnvelope *envs[8] = {
  &env0, &env1, &env2, &env3,
  &env4, &env5, &env6, &env7
};

// Route each waveform through its own envelope effect
AudioConnection patchCord01(sine0, env0);
AudioConnection patchCord02(sine1, env1);
AudioConnection patchCord03(sine2, env2);
AudioConnection patchCord04(sine3, env3);
AudioConnection patchCord05(sine4, env4);
AudioConnection patchCord06(sine5, env5);
AudioConnection patchCord07(sine6, env6);
AudioConnection patchCord08(sine7, env7);

// Mix the 8 channels down to 4 audio streams
AudioConnection patchCord17(env0, 0, mixerMOD, 0);
AudioConnection patchCord18(env1, 0, mixerMOD, 1);
AudioConnection patchCord19(env2, 0, mixerMOD, 2);
AudioConnection patchCord20(env3, 0, mixerMOD, 3);

AudioConnection patchCord21(env4, 0, mixerFX2, 0);
AudioConnection patchCord22(env5, 0, mixerFX2, 1);
AudioConnection patchCord23(env6, 0, mixerFX2, 2);
AudioConnection patchCord24(env7, 0, mixerFX2, 3);

#endif

#if 0
//Config generee par l'outil https://www.pjrc.com/teensy/gui/index.html?info=AudioEffectBitcrusher#
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayMemory          sndPlayerCanal4;       //xy=324,1114
AudioEffectBitcrusher    bitcrusher1;    //xy=610,1222
AudioPlayMemory          playMem4;       //xy=627,1302
AudioPlayMemory          playMem5;       //xy=639,1348
AudioPlayMemory          sndPlayerCanal2;       //xy=697,1040
AudioPlayMemory          sndPlayerCanal3;       //xy=698,1079
AudioPlayMemory          sndPlayerCanal1;       //xy=699,1001
AudioMixer4              mixerFX2;         //xy=955,1159
AudioMixer4              mixerFX;         //xy=956,1009
AudioMixer4              mixerMOD;         //xy=964,1311
AudioMixer4              mixerLeft;         //xy=1211,1064
AudioMixer4              mixerRight;         //xy=1215,1199
AudioOutputAnalogStereo  audioOut;          //xy=1484,1117
AudioConnection          patchCord1(sndPlayerCanal4, bitcrusher1);
AudioConnection          patchCord2(bitcrusher1, 0, mixerFX, 3);
AudioConnection          patchCord3(playMem4, 0, mixerMOD, 0);
AudioConnection          patchCord4(playMem5, 0, mixerMOD, 1);
AudioConnection          patchCord5(sndPlayerCanal2, 0, mixerFX, 1);
AudioConnection          patchCord6(sndPlayerCanal3, 0, mixerFX, 2);
AudioConnection          patchCord7(sndPlayerCanal1, 0, mixerFX, 0);
AudioConnection          patchCord8(mixerFX2, 0, mixerLeft, 2);
AudioConnection          patchCord9(mixerFX2, 0, mixerRight, 2);
AudioConnection          patchCord10(mixerFX, 0, mixerLeft, 1);
AudioConnection          patchCord11(mixerFX, 0, mixerRight, 1);
AudioConnection          patchCord12(mixerMOD, 0, mixerLeft, 0);
AudioConnection          patchCord13(mixerMOD, 0, mixerRight, 0);
AudioConnection          patchCord14(mixerLeft, 0, audioOut, 0);
AudioConnection          patchCord15(mixerRight, 0, audioOut, 1);
// GUItool: end automatically generated code

#endif


// Now create 2 mixers for the main output
AudioMixer4     mixerLeft;
AudioMixer4     mixerRight;
AudioOutputAnalogStereo  audioOut;

AudioPlayMemory sndPlayerCanal1;
AudioPlayMemory sndPlayerCanal2;
AudioPlayMemory sndPlayerCanal3;
//AudioPlayMemory sndPlayerCanal4;

AudioConnection FX1(sndPlayerCanal1, 0, mixerFX, 0);
AudioConnection FX2(sndPlayerCanal2, 0, mixerFX, 1);
AudioConnection FX3(sndPlayerCanal3, 0, mixerFX, 2);
/*
AudioEffectBitcrusher    bitcrusher1;
AudioConnection          patchCord1(sndPlayerCanal4, bitcrusher1);
AudioConnection          patchCord2(bitcrusher1, 0, mixerFX, 3);
*/
// Mix all channels to both the outputs
AudioConnection patchCord33(mixerMOD, 0, mixerLeft, 0);
AudioConnection patchCord35(mixerFX, 0, mixerLeft, 1);
//AudioConnection patchCord34(mixerFX2, 0, mixerLeft, 2);

AudioConnection patchCord37(mixerMOD, 0, mixerRight, 0);
AudioConnection patchCord39(mixerFX, 0, mixerRight, 1);
//AudioConnection patchCord38(mixerFX2, 0, mixerRight, 2);

AudioConnection patchCord41(mixerLeft, 0, audioOut, 0);
AudioConnection patchCord42(mixerRight, 0, audioOut, 1);



//AudioConnection c9(mix1, 1, audioOut, 1);


// Initial value of the volume control
int volume = 50;

void setupSoundManager()
{
// http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
  Serial.print("Begin ");
  Serial.println(__FILE__);
  
  delay(500);
  // Proc = 12 (13),  Mem = 2 (8)
  // Audio connections require memory to work.
  // The memory usage code indicates that 10 is the maximum
  // so give it 12 just to be sure.
  AudioMemory(8);
 
  // reduce the gain on some channels, so half of the channels
  // are "positioned" to the left, half to the right, but all
  // are heard at least partially on both ears
  mixerMOD.gain(0, 0.3);
  mixerMOD.gain(1, 0.3);
  mixerMOD.gain(2, 0.3);
  mixerMOD.gain(3, 0.3);

  mixerFX.gain(0, 1);
  mixerFX.gain(1, 1);
  mixerFX.gain(2, 1);
  mixerFX.gain(3, 1);

  mixerLeft.gain(0, 0.3);
  mixerLeft.gain(1, 0.3);
  mixerLeft.gain(2, 0.3);
  mixerLeft.gain(3, 0.3);

  mixerRight.gain(0, 0.3);
  mixerRight.gain(1, 0.3);
  mixerRight.gain(2, 0.3);
  mixerRight.gain(3, 0.3);

  //16 bits par defaut (aucun effet)
//  bitcrusher1.bits(16);
  //44Khz = par defaut, aucun changement
//  bitcrusher1.sampleRate(44100);

#ifdef MIDI_MUSIC
  // set envelope parameters, for pleasing sound :-)
  for (int i=0; i<8; i++) {
    envs[i]->attack(9.2);
    envs[i]->hold(2.1);
    envs[i]->decay(31.4);
    envs[i]->sustain(0.6);
    envs[i]->release(84.5);
    // uncomment these to hear without envelope effects
    //envs[i]->attack(0.0);
    //envs[i]->hold(0.0);
    //envs[i]->decay(0.0);
    //envs[i]->release(0.0);
  }
#endif
  Serial.println("setup done");
  
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();

  Serial.print("Proc = ");
  Serial.print(AudioProcessorUsage());
  Serial.print(" (");    
  Serial.print(AudioProcessorUsageMax());
  Serial.print("),  Mem = ");
  Serial.print(AudioMemoryUsage());
  Serial.print(" (");    
  Serial.print(AudioMemoryUsageMax());
  Serial.println(")");
}

void playMusic(const unsigned char *pMusic) {
#ifdef MIDI_MUSIC
  bMusicEnded = true;
  if (pMusic[0] == 'P' && pMusic[1] == 't')
  {
    uint8_t headerSize = (uint8_t)pMusic[2];
    pMusic += headerSize;
  }
  sp = (unsigned char*)pMusic;
  _currentMusic = (unsigned char*)pMusic;
  bMusicEnded = false;
#endif
}

void stopMusic() {
  #ifdef MIDI_MUSIC
  bMusicEnded = true;
  for (unsigned char chan=0; chan<8; chan++) {
      envs[chan]->noteOff();
      waves[chan]->amplitude(0);
    }
  #endif
}

void updateSoundManager(unsigned long now)
{
  #ifdef MIDI_MUSIC
  unsigned char c,opcode,chan;
  unsigned long d_time;

  static unsigned long last_time = 0;
  static unsigned long pauseUntill = 0;

  if (bMusicEnded)
    return;

// Change this to if(1) for measurement output every 5 seconds
#ifdef DEBUG
 {
  if(millis() - last_time >= 5000) {
    Serial.print("Proc = ");
    Serial.print(AudioProcessorUsage());
    Serial.print(" (");    
    Serial.print(AudioProcessorUsageMax());
    Serial.print("),  Mem = ");
    Serial.print(AudioMemoryUsage());
    Serial.print(" (");    
    Serial.print(AudioMemoryUsageMax());
    Serial.println(")");
    last_time = millis();
  }
}
#endif

  if (pauseUntill > 0 && now < pauseUntill)
    return;

  pauseUntill = 0;

  if (sp == NULL)
    return;

  // Volume control
  //  uncomment if you have a volume pot soldered to your audio shield
  /*
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    codec.volume((float)n / 1023);
  }
  */
  
  // read the next note from the table
  c = *sp++;
  opcode = c & 0xF0;
  chan = c & 0x0F;

  if (chan >= 8)
    return;
  
  if(c < 0x80) {
    // Delay
    d_time = (c << 8) | *sp++;
    pauseUntill = now + d_time;
    return;
  }

  if(*sp == CMD_STOP) {
    for (chan=0; chan<8; chan++) {
      envs[chan]->noteOff();
      waves[chan]->amplitude(0);
    }
    Serial.println("Music DONE");
    bMusicEnded = true;
    return;
  }

  // It is a command
  
  // Stop the note on 'chan'
  if(opcode == CMD_STOPNOTE) {
    envs[chan]->noteOff();
    return;
  }
  
  // Play the note on 'chan'
  if(opcode == CMD_PLAYNOTE) {
    unsigned char note = *sp++;
    unsigned char velocity = *sp++;
    if (note < 128 && velocity < 127)
    {
      AudioNoInterrupts();
        waves[chan]->begin(AMPLITUDE * velocity2amplitude[velocity-1],
                          tune_frequencies2_PGM[note],
                          wave_type[chan]);
        envs[chan]->noteOn();
      
      AudioInterrupts();
    }
    return;
  }

  // replay the tune
  if(opcode == CMD_RESTART) {
    sp = _currentMusic;
    return;
  }
  #endif
}