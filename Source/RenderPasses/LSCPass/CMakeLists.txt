add_plugin(LSCPass)

target_sources(LSCPass PRIVATE
    LSCPass.cs.slang
    LSCPass.cpp
    LSCPass.h
)

target_source_group(LSCPass "RenderPasses")
