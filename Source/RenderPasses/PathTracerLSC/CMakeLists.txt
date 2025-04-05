add_plugin(PathTracerLSC)

target_sources(PathTracerLSC PRIVATE
    ColorType.slang
    GeneratePaths.cs.slang
    GuideData.slang
    LoadShadingData.slang
    NRDHelpers.slang
    Params.slang
    PathState.slang
    PathTracerLSC.slang
    PathTracerLSC.cpp
    PathTracerLSC.h
    PathTracerNRD.slang
    ReflectTypes.cs.slang
    ResolvePass.cs.slang
    StaticParams.slang
    TracePass.rt.slang
)

target_copy_shaders(PathTracerLSC RenderPasses/PathTracerLSC)

target_source_group(PathTracerLSC "RenderPasses")
