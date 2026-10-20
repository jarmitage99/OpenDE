#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>

namespace dx {
    namespace fractaltexture {
        struct Offsets {
            size_t UMask = 0;
            size_t VMask = 0;
            size_t LightOutput = 0;
            size_t SoundOutput = 0;
            size_t GlobalPhase = 0;
            size_t DrawPhase = 0;
            size_t AuxPhase = 0;
        };
    }

    extern fractaltexture::Offsets offsets_fractaltexture;

    namespace fractaltexture {
        inline int32_t& UMask(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_fractaltexture.UMask);
        }

        inline int32_t& VMask(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_fractaltexture.VMask);
        }

        inline int32_t& LightOutput(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_fractaltexture.LightOutput);
        }

        inline int32_t& SoundOutput(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_fractaltexture.SoundOutput);
        }

        inline int32_t& GlobalPhase(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_fractaltexture.GlobalPhase);
        }

        inline uint8_t& DrawPhase(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_fractaltexture.DrawPhase);
        }

        inline uint8_t& AuxPhase(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_fractaltexture.AuxPhase);
        }

    }

    namespace firetexture {
        struct Offsets {
            size_t SparkType = 0;
            size_t RenderHeat = 0;
            size_t bRising = 0;
            size_t FX_Heat = 0;
            size_t FX_Size = 0;
            size_t FX_AuxSize = 0;
            size_t FX_Area = 0;
            size_t FX_Frequency = 0;
            size_t FX_Phase = 0;
            size_t FX_HorizSpeed = 0;
            size_t FX_VertSpeed = 0;
            size_t DrawMode = 0;
            size_t SparksLimit = 0;
            size_t NumSparks = 0;
            size_t Sparks = 0;
            size_t OldRenderHeat = 0;
            size_t RenderTable = 0;
            size_t StarStatus = 0;
            size_t PenDownX = 0;
            size_t PenDownY = 0;
        };
    }

    extern firetexture::Offsets offsets_firetexture;

    namespace firetexture {
        inline uint8_t& SparkType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.SparkType);
        }

        inline uint8_t& RenderHeat(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.RenderHeat);
        }

        inline uint32_t& bRising(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_firetexture.bRising);
        }

        inline uint8_t& FX_Heat(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_Heat);
        }

        inline uint8_t& FX_Size(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_Size);
        }

        inline uint8_t& FX_AuxSize(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_AuxSize);
        }

        inline uint8_t& FX_Area(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_Area);
        }

        inline uint8_t& FX_Frequency(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_Frequency);
        }

        inline uint8_t& FX_Phase(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_Phase);
        }

        inline uint8_t& FX_HorizSpeed(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_HorizSpeed);
        }

        inline uint8_t& FX_VertSpeed(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.FX_VertSpeed);
        }

        inline uint8_t& DrawMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.DrawMode);
        }

        inline int32_t& SparksLimit(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_firetexture.SparksLimit);
        }

        inline int32_t& NumSparks(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_firetexture.NumSparks);
        }

        inline uint8_t* Sparks(UObject* o)
        {
            return o->data + offsets_firetexture.Sparks;
        }

        inline int32_t& OldRenderHeat(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_firetexture.OldRenderHeat);
        }

        inline uint8_t& RenderTable(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.RenderTable)[index];
        }

        inline uint8_t& StarStatus(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.StarStatus);
        }

        inline uint8_t& PenDownX(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.PenDownX);
        }

        inline uint8_t& PenDownY(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_firetexture.PenDownY);
        }

    }

    namespace watertexture {
        struct Offsets {
            size_t DropType = 0;
            size_t WaveAmp = 0;
            size_t FX_Frequency = 0;
            size_t FX_Phase = 0;
            size_t FX_Amplitude = 0;
            size_t FX_Speed = 0;
            size_t FX_Radius = 0;
            size_t FX_Size = 0;
            size_t FX_Depth = 0;
            size_t FX_Time = 0;
            size_t NumDrops = 0;
            size_t Drops = 0;
            size_t SourceFields = 0;
            size_t RenderTable = 0;
            size_t WaterTable = 0;
            size_t WaterParity = 0;
            size_t OldWaveAmp = 0;
        };
    }

    extern watertexture::Offsets offsets_watertexture;

    namespace watertexture {
        inline uint8_t& DropType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.DropType);
        }

        inline uint8_t& WaveAmp(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.WaveAmp);
        }

        inline uint8_t& FX_Frequency(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Frequency);
        }

        inline uint8_t& FX_Phase(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Phase);
        }

        inline uint8_t& FX_Amplitude(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Amplitude);
        }

        inline uint8_t& FX_Speed(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Speed);
        }

        inline uint8_t& FX_Radius(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Radius);
        }

        inline uint8_t& FX_Size(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Size);
        }

        inline uint8_t& FX_Depth(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Depth);
        }

        inline uint8_t& FX_Time(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.FX_Time);
        }

        inline int32_t& NumDrops(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_watertexture.NumDrops);
        }

        inline uint8_t* Drops(UObject* o)
        {
            return o->data + offsets_watertexture.Drops;
        }

        inline int32_t& SourceFields(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_watertexture.SourceFields);
        }

        inline uint8_t& RenderTable(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.RenderTable)[index];
        }

        inline uint8_t& WaterTable(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.WaterTable)[index];
        }

        inline uint8_t& WaterParity(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_watertexture.WaterParity);
        }

        inline int32_t& OldWaveAmp(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_watertexture.OldWaveAmp);
        }

    }

    namespace wavetexture {
        struct Offsets {
            size_t BumpMapLight = 0;
            size_t BumpMapAngle = 0;
            size_t PhongRange = 0;
            size_t PhongSize = 0;
        };
    }

    extern wavetexture::Offsets offsets_wavetexture;

    namespace wavetexture {
        inline uint8_t& BumpMapLight(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_wavetexture.BumpMapLight);
        }

        inline uint8_t& BumpMapAngle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_wavetexture.BumpMapAngle);
        }

        inline uint8_t& PhongRange(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_wavetexture.PhongRange);
        }

        inline uint8_t& PhongSize(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_wavetexture.PhongSize);
        }

    }

    namespace wettexture {
        struct Offsets {
            size_t SourceTexture = 0;
            size_t OldSourceTex = 0;
            size_t LocalSourceBitmap = 0;
        };
    }

    extern wettexture::Offsets offsets_wettexture;

    namespace wettexture {
        inline UObject*& SourceTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_wettexture.SourceTexture);
        }

        inline UObject*& OldSourceTex(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_wettexture.OldSourceTex);
        }

        inline int32_t& LocalSourceBitmap(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_wettexture.LocalSourceBitmap);
        }

    }

    namespace icetexture {
        struct Offsets {
            size_t GlassTexture = 0;
            size_t SourceTexture = 0;
            size_t PanningStyle = 0;
            size_t TimeMethod = 0;
            size_t HorizPanSpeed = 0;
            size_t VertPanSpeed = 0;
            size_t Frequency = 0;
            size_t Amplitude = 0;
            size_t MoveIce = 0;
            size_t MasterCount = 0;
            size_t UDisplace = 0;
            size_t VDisplace = 0;
            size_t UPosition = 0;
            size_t VPosition = 0;
            size_t TickAccu = 0;
            size_t OldUDisplace = 0;
            size_t OldVDisplace = 0;
            size_t OldGlassTex = 0;
            size_t OldSourceTex = 0;
            size_t LocalSource = 0;
            size_t ForceRefresh = 0;
        };
    }

    extern icetexture::Offsets offsets_icetexture;

    namespace icetexture {
        inline UObject*& GlassTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_icetexture.GlassTexture);
        }

        inline UObject*& SourceTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_icetexture.SourceTexture);
        }

        inline uint8_t& PanningStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.PanningStyle);
        }

        inline uint8_t& TimeMethod(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.TimeMethod);
        }

        inline uint8_t& HorizPanSpeed(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.HorizPanSpeed);
        }

        inline uint8_t& VertPanSpeed(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.VertPanSpeed);
        }

        inline uint8_t& Frequency(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.Frequency);
        }

        inline uint8_t& Amplitude(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_icetexture.Amplitude);
        }

        inline uint32_t& MoveIce(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_icetexture.MoveIce);
        }

        inline float& MasterCount(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.MasterCount);
        }

        inline float& UDisplace(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.UDisplace);
        }

        inline float& VDisplace(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.VDisplace);
        }

        inline float& UPosition(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.UPosition);
        }

        inline float& VPosition(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.VPosition);
        }

        inline float& TickAccu(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_icetexture.TickAccu);
        }

        inline int32_t& OldUDisplace(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_icetexture.OldUDisplace);
        }

        inline int32_t& OldVDisplace(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_icetexture.OldVDisplace);
        }

        inline UObject*& OldGlassTex(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_icetexture.OldGlassTex);
        }

        inline UObject*& OldSourceTex(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_icetexture.OldSourceTex);
        }

        inline int32_t& LocalSource(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_icetexture.LocalSource);
        }

        inline int32_t& ForceRefresh(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_icetexture.ForceRefresh);
        }

    }

    void bindProps_fire();
}
