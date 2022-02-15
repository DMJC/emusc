/*  
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */

// Control ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#ifndef __CONTROL_ROM_H__
#define __CONTROL_ROM_H__


#include "config.h"

#include <fstream>
#include <string>
#include <vector>


class ControlRom
{
private:
  Config &_config;
  uint32_t _banksVSC[8] = { 0x00034, 0x0BD34, 0x0DEF4, 0x10034,
                            0x1BD34, 0x1DEF4, 0x20034, 0x30000 };
  uint32_t _banksSC55[8] = { 0x10000, 0x1BD00, 0x1DEC0, 0x20000,
                             0x2BD00, 0x2DEC0, 0x30000, 0x38080 };

  enum SynthModel {
    sm_VSC,
    sm_SC55,
    sm_SC55mkII,
    sm_SC88
  };
  enum SynthModel _synthModel;

  std::vector<uint8_t> _romData;

  int _identify_model(void);
  uint32_t *_get_banks(void);

  // To be replaced with std::endian::native from C++20
  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); } 

  uint16_t _native_endian_uint16(uint8_t *ptr);
  uint32_t _native_endian_3bytes_uint32(uint8_t *ptr);
  uint32_t _native_endian_4bytes_uint32(uint8_t *ptr);

  ControlRom();

public:
  ControlRom(Config &config);
  ~ControlRom();

  // Internal data structures extracted from the control ROM file

  struct Sample {         // 16 bytes
    uint8_t  volume;      // Volume attenuation 7F to 0
    uint32_t address;     // Offset on vsc, bank + scrambled address on SC55.
                          // Bits above 20 are wave bank.
    uint16_t attackEnd;   // boundry between attack and decay? Unconfirmed.
    uint16_t sampleLen;   // Sample Size
    uint16_t loopLen;     // Loop point, used as sample_len - loop_len - 1
    uint8_t  loopMode;    // 2 if not a looping sound, 1 forward then back,
                          // 0 forward only.
    uint8_t  rootKey;     // Base pitch of the sample
    uint16_t pitch;       // Fine pitch adjustment, 2048 to 0. Pos. incr. pitch.
    uint16_t fineVolume;  // Always 0x400 on VSC, appears to be 1000ths of a
                          // decibel. Positive is higher volume.
    std::vector<int32_t> sampleSet; // All samples stored in 24 bit 32kHz mono 
  };

  struct Partial {        // 48 bytes in total
    std::string name;
    uint8_t breaks[16];   // Note breakpoints corresponding to sample addresses
    uint16_t samples[16]; // Set of addresses to the sample table. 0 is default,
  };                      // and above corresponds to breakpoints

  struct InstPartial {    // 92 bytes in total
    uint16_t partialIndex;// Part table index, 0xFFFF for unused

    // From heatmap spreadsheet found online. Not used yet.
    int8_t panpot;
    int8_t coarsePitch;
    int8_t finePitch;
    int8_t randPitch;
    int8_t PitchKeyFlw;
    int8_t TvpLfoDepth;
    int8_t pitchMult;
    int8_t pitchLvlP0;
    int8_t pitchLvlP1;
    int8_t pitchLvlP2;
    int8_t pitchLvlP3;
    int8_t pitchLvlP4;
    int8_t pitchDurP1;
    int8_t pitchDurP2;
    int8_t pitchDurP3;
    int8_t pitchDurP4;
    int8_t pitchDurRel;
    int8_t TVFBaseFlt;
    int8_t LowVelClear;
    int8_t TVFResonance;
    int8_t TVFLvlInit;
    int8_t TVFLvlP1;
    int8_t TVFLvlP2;
    int8_t TVFLvlP3;
    int8_t TVFLvlP4;
    int8_t TVFDurP1;
    int8_t TVFDurP2;
    int8_t TVFDurP3;
    int8_t TVFDurP4;
    int8_t TVFDurRel;
    int8_t TVFLFODepth;   // TCA LFO Depth
    int8_t TVALvl1;       // TVA Level 1 (Attack)
    int8_t TVALvl2;       // TVA Level 1 (Hold)
    int8_t TVALvl3;       // TVA Level 1 (Decay)
    int8_t TVALvl4;       // TVA Level 1 (Sustain)
    int8_t TVADur1;       // TVA Level 1 (Attack)
    int8_t TVADur2;       // TVA Level 1 (Hold)
    int8_t TVADur3;       // TVA Level 1 (Decay)
    int8_t TVADur4;       // TVA Level 1 (Sustain)
    int8_t TVADur5;       // TVA Level 1 (Release)
    
  };

  struct Instrument {     // 204 bytes in total
    std::string name;
    struct InstPartial partials[2];
  };                      // Contains 20 unused bytes (header, unknown purpose)

  struct DrumSet {        //1164 bytes
    uint16_t preset[128];
    uint8_t volume[128];
    uint8_t key[128];
    uint8_t assignGroup[128];// AKA exclusive class
    uint8_t panpot[128];
    uint8_t reverb[128];
    uint8_t chorus[128];
    uint8_t flags[128];   // 0x10 -> note on,  0x01 -> note_off
    std::string name;     // 12 chars
  };

  struct Variation {
    uint16_t variation[128];  // All models have 128 x 128 variation table
  };

  int get_instruments(std::vector<Instrument> &instruments);
  int get_partials(std::vector<Partial> &partials);
  int get_samples(std::vector<Sample> &samples);
  int get_variations(std::vector<Variation> &variations);
  int get_drumSet(std::vector<DrumSet> &drumSets);

  int dump_demo_songs(std::string path);
};


#endif  // __CONTROL_ROM_H__
