/***************************************************************************
 # Copyright (c) 2015-23, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "LSCPass.h"
#include "RenderGraph/RenderPassHelpers.h"
#include "RenderGraph/RenderPassStandardFlags.h"

using namespace Falcor;

namespace
{
const std::string kShaderFile = "RenderPasses/LSCPass/LSCPass.cs.slang";

const std::string kDepth = "depth";
const std::string kNormW = "normW";
const std::string kPosW = "posW";
const std::string kDirect = "direct";
const std::string kIndirect = "indirect";
//const std::string kShadow = "shadow";

const Falcor::ChannelList kInputChannels = {
    // clang-format off
    { kDepth,         "gDepth",      "Depth buffer (NDC)",                              false /* required */, ResourceFormat::R32Float     },
    { kNormW,         "gNormW",      "Shading normal in world space",                   false /* required */, ResourceFormat::RGBA32Float  },
    { kPosW,          "gPosW",       "Shading position in world space",                 false /* required */, ResourceFormat::RGBA32Float  },
    { kDirect,        "gDirect",     "Direct lighting buffer",                          false /* required */, ResourceFormat::RGBA32Float  },
    { kIndirect,      "gIndirect",   "Indirect lighting buffer",                        false /* required */, ResourceFormat::RGBA32Float  },
    //{ kShadow,        "gShadow",     "Light source visibility buffer (True in shadow)", false /* required */, ResourceFormat::R8Int        },
    // clang-format on
};

const Falcor::ChannelList kOutputChannels = {
    // clang-format off
    {"color", "gColor", "Final color", true /* optional */, ResourceFormat::RGBA32Float}
    // clang-format on
};
} // namespace

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<RenderPass, LSCPass>();
}

LSCPass::LSCPass(ref<Device> pDevice, const Properties& props) : RenderPass(pDevice) {}

Properties LSCPass::getProperties() const
{
    return {};
}

RenderPassReflection LSCPass::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector;

    addRenderPassOutputs(reflector, kOutputChannels);
    addRenderPassInputs(reflector, kInputChannels);

    return reflector;
}

void LSCPass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    auto pDepth = renderData.getTexture(kDepth);
    auto pNormW = renderData.getTexture(kNormW);
    auto pPosW = renderData.getTexture(kPosW);
    auto pDirect = renderData.getTexture(kDirect);
    auto pIndirect = renderData.getTexture(kIndirect);
    //auto pShadow = renderData.getTexture(kShadow);

    // Set shader parameters
    auto var = mpVars->getRootVar();
    var["gDepth"] = pDepth;
    var["gNormW"] = pNormW;
    var["gPosW"] = pPosW;
    var["gDirect"] = pDirect;
    var["gIndirect"] = pIndirect;
    //var["gShadow"] = pShadow;

    mFrameDim = uint2(pDirect->getWidth(), pDirect->getHeight());
    var["PerFrameCB"]["gResolution"] = mFrameDim;

    // Run the LSC algorithm
    DefineList defines;
    mpProgram = Program::createCompute(mpDevice, kShaderFile, "lsc", defines, SlangCompilerFlags::TreatWarningsAsErrors);
    auto pProgram = mpProgram;
    FALCOR_ASSERT(pProgram);
    uint3 numGroups = div_round_up(uint3(mFrameDim.x, mFrameDim.y, 1u), pProgram->getReflector()->getThreadGroupSize());
    mpState->setProgram(pProgram);
    pRenderContext->dispatch(mpState.get(), mpVars.get(), numGroups);
}

void LSCPass::renderUI(Gui::Widgets& widget) {}
