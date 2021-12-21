//
// Copyright(C) 2019-2020 Nightdive Studios, LLC
//
// DESCRIPTION:
//       ymfmidi player C/C++ header code.
//

#ifdef __cplusplus
extern "C" {
#endif
    int I_ymfmLoad(const unsigned char* fileData, const unsigned int fileSize, const unsigned char* patchData, const unsigned int patchSize, int sampleRate);
    void I_ymfmSetLooping(int looping);
    void I_ymfmSetGain(float gain);
    void I_ymfmGenerate(void *udata, unsigned char *stream, int len);
#ifdef __cplusplus
}
#endif