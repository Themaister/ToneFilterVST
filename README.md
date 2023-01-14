# ToneFilter VST

## Add VST headers

Unfortunately, these headers cannot be shared publically, make sure to populate the SDK headers.

```
VST_SDK
VST_SDK/public.sdk
VST_SDK/public.sdk/source
VST_SDK/public.sdk/source/vst2.x
VST_SDK/public.sdk/source/vst2.x/audioeffect.h
VST_SDK/public.sdk/source/vst2.x/audioeffectx.h
VST_SDK/public.sdk/source/vst2.x/vstplugmain.cpp
VST_SDK/public.sdk/source/vst2.x/audioeffectx.cpp
VST_SDK/public.sdk/source/vst2.x/audioeffect.cpp
VST_SDK/public.sdk/source/vst2.x/aeffeditor.h
VST_SDK/pluginterfaces
VST_SDK/pluginterfaces/vst2.x
VST_SDK/pluginterfaces/vst2.x/aeffect.h
VST_SDK/pluginterfaces/vst2.x/vstfxstore.h
VST_SDK/pluginterfaces/vst2.x/aeffectx.h
```

## Build

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

This should make a VST dll.

