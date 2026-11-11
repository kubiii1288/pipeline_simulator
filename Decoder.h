//
// Created by kubiii on 11/10/26.
//

#ifndef PIPELINE_SIMULATOR_DECODER_H
#define PIPELINE_SIMULATOR_DECODER_H
#include "Instruction.h"

namespace pipeline_sim
{
       DecodedInstruction decode(std::uint32_t raw);
} // pipeline_sim

#endif //PIPELINE_SIMULATOR_DECODER_H
