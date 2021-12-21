//
// Copyright(C) 2019-2020 Nightdive Studios, LLC
//
// DESCRIPTION:
//       ymfmidi player code
//

#include "i_ymfm.h"
#include "../ymfmidi/src/player.h"

OPLPlayer* pOPLPlayer = nullptr;

int I_ymfmLoad(const unsigned char* fileData, const unsigned int fileSize, const unsigned char* patchData, const unsigned int patchSize, int sampleRate)
{
    if (pOPLPlayer != nullptr)
    {
        delete pOPLPlayer;
        pOPLPlayer = nullptr;
    }
    pOPLPlayer = new OPLPlayer(1);

    if (pOPLPlayer->loadPatches(patchData, patchSize) && pOPLPlayer->loadSequence(fileData, fileSize))
    {
        pOPLPlayer->setSampleRate(sampleRate);
        pOPLPlayer->reset();

        return 1;
    }

    return 0;
}

void I_ymfmSetLooping(int looping)
{
    if (pOPLPlayer != nullptr)
    {
        pOPLPlayer->setLoop(!!looping);
    }
}

void I_ymfmSetGain(float gain)
{
    if (pOPLPlayer != nullptr)
    {
        pOPLPlayer->setGain((double)gain);
    }
}

void I_ymfmGenerate(void * udata, unsigned char * stream, int len)
{
    if (pOPLPlayer == nullptr)
    {
        return;
    }

    pOPLPlayer->generate(reinterpret_cast<int16_t*>(stream), len / (2 * sizeof(int16_t)));
}
