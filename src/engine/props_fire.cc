#include "engine/props_fire.h"

#include "core/objects.h"

#include <cstdio>

namespace dx {
    fractaltexture::Offsets offsets_fractaltexture;
    firetexture::Offsets offsets_firetexture;
    watertexture::Offsets offsets_watertexture;
    wavetexture::Offsets offsets_wavetexture;
    wettexture::Offsets offsets_wettexture;
    icetexture::Offsets offsets_icetexture;

    void bindProps_fire()
    {
        {
            UClass* cls = objects::findClass("Fire.FractalTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.FractalTexture\n");
            } else {
                offsets_fractaltexture.UMask = cls->findProperty(Name("UMask"))->offset;
                offsets_fractaltexture.VMask = cls->findProperty(Name("VMask"))->offset;
                offsets_fractaltexture.LightOutput = cls->findProperty(Name("LightOutput"))->offset;
                offsets_fractaltexture.SoundOutput = cls->findProperty(Name("SoundOutput"))->offset;
                offsets_fractaltexture.GlobalPhase = cls->findProperty(Name("GlobalPhase"))->offset;
                offsets_fractaltexture.DrawPhase = cls->findProperty(Name("DrawPhase"))->offset;
                offsets_fractaltexture.AuxPhase = cls->findProperty(Name("AuxPhase"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Fire.FireTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.FireTexture\n");
            } else {
                offsets_firetexture.SparkType = cls->findProperty(Name("SparkType"))->offset;
                offsets_firetexture.RenderHeat = cls->findProperty(Name("RenderHeat"))->offset;
                offsets_firetexture.bRising = cls->findProperty(Name("bRising"))->offset;
                offsets_firetexture.FX_Heat = cls->findProperty(Name("FX_Heat"))->offset;
                offsets_firetexture.FX_Size = cls->findProperty(Name("FX_Size"))->offset;
                offsets_firetexture.FX_AuxSize = cls->findProperty(Name("FX_AuxSize"))->offset;
                offsets_firetexture.FX_Area = cls->findProperty(Name("FX_Area"))->offset;
                offsets_firetexture.FX_Frequency = cls->findProperty(Name("FX_Frequency"))->offset;
                offsets_firetexture.FX_Phase = cls->findProperty(Name("FX_Phase"))->offset;
                offsets_firetexture.FX_HorizSpeed = cls->findProperty(Name("FX_HorizSpeed"))->offset;
                offsets_firetexture.FX_VertSpeed = cls->findProperty(Name("FX_VertSpeed"))->offset;
                offsets_firetexture.DrawMode = cls->findProperty(Name("DrawMode"))->offset;
                offsets_firetexture.SparksLimit = cls->findProperty(Name("SparksLimit"))->offset;
                offsets_firetexture.NumSparks = cls->findProperty(Name("NumSparks"))->offset;
                offsets_firetexture.Sparks = cls->findProperty(Name("Sparks"))->offset;
                offsets_firetexture.OldRenderHeat = cls->findProperty(Name("OldRenderHeat"))->offset;
                offsets_firetexture.RenderTable = cls->findProperty(Name("RenderTable"))->offset;
                offsets_firetexture.StarStatus = cls->findProperty(Name("StarStatus"))->offset;
                offsets_firetexture.PenDownX = cls->findProperty(Name("PenDownX"))->offset;
                offsets_firetexture.PenDownY = cls->findProperty(Name("PenDownY"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Fire.WaterTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.WaterTexture\n");
            } else {
                offsets_watertexture.DropType = cls->findProperty(Name("DropType"))->offset;
                offsets_watertexture.WaveAmp = cls->findProperty(Name("WaveAmp"))->offset;
                offsets_watertexture.FX_Frequency = cls->findProperty(Name("FX_Frequency"))->offset;
                offsets_watertexture.FX_Phase = cls->findProperty(Name("FX_Phase"))->offset;
                offsets_watertexture.FX_Amplitude = cls->findProperty(Name("FX_Amplitude"))->offset;
                offsets_watertexture.FX_Speed = cls->findProperty(Name("FX_Speed"))->offset;
                offsets_watertexture.FX_Radius = cls->findProperty(Name("FX_Radius"))->offset;
                offsets_watertexture.FX_Size = cls->findProperty(Name("FX_Size"))->offset;
                offsets_watertexture.FX_Depth = cls->findProperty(Name("FX_Depth"))->offset;
                offsets_watertexture.FX_Time = cls->findProperty(Name("FX_Time"))->offset;
                offsets_watertexture.NumDrops = cls->findProperty(Name("NumDrops"))->offset;
                offsets_watertexture.Drops = cls->findProperty(Name("Drops"))->offset;
                offsets_watertexture.SourceFields = cls->findProperty(Name("SourceFields"))->offset;
                offsets_watertexture.RenderTable = cls->findProperty(Name("RenderTable"))->offset;
                offsets_watertexture.WaterTable = cls->findProperty(Name("WaterTable"))->offset;
                offsets_watertexture.WaterParity = cls->findProperty(Name("WaterParity"))->offset;
                offsets_watertexture.OldWaveAmp = cls->findProperty(Name("OldWaveAmp"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Fire.WaveTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.WaveTexture\n");
            } else {
                offsets_wavetexture.BumpMapLight = cls->findProperty(Name("BumpMapLight"))->offset;
                offsets_wavetexture.BumpMapAngle = cls->findProperty(Name("BumpMapAngle"))->offset;
                offsets_wavetexture.PhongRange = cls->findProperty(Name("PhongRange"))->offset;
                offsets_wavetexture.PhongSize = cls->findProperty(Name("PhongSize"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Fire.WetTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.WetTexture\n");
            } else {
                offsets_wettexture.SourceTexture = cls->findProperty(Name("SourceTexture"))->offset;
                offsets_wettexture.OldSourceTex = cls->findProperty(Name("OldSourceTex"))->offset;
                offsets_wettexture.LocalSourceBitmap = cls->findProperty(Name("LocalSourceBitmap"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Fire.IceTexture");
            if (!cls) {
                std::fprintf(stderr, "missing class Fire.IceTexture\n");
            } else {
                offsets_icetexture.GlassTexture = cls->findProperty(Name("GlassTexture"))->offset;
                offsets_icetexture.SourceTexture = cls->findProperty(Name("SourceTexture"))->offset;
                offsets_icetexture.PanningStyle = cls->findProperty(Name("PanningStyle"))->offset;
                offsets_icetexture.TimeMethod = cls->findProperty(Name("TimeMethod"))->offset;
                offsets_icetexture.HorizPanSpeed = cls->findProperty(Name("HorizPanSpeed"))->offset;
                offsets_icetexture.VertPanSpeed = cls->findProperty(Name("VertPanSpeed"))->offset;
                offsets_icetexture.Frequency = cls->findProperty(Name("Frequency"))->offset;
                offsets_icetexture.Amplitude = cls->findProperty(Name("Amplitude"))->offset;
                offsets_icetexture.MoveIce = cls->findProperty(Name("MoveIce"))->offset;
                offsets_icetexture.MasterCount = cls->findProperty(Name("MasterCount"))->offset;
                offsets_icetexture.UDisplace = cls->findProperty(Name("UDisplace"))->offset;
                offsets_icetexture.VDisplace = cls->findProperty(Name("VDisplace"))->offset;
                offsets_icetexture.UPosition = cls->findProperty(Name("UPosition"))->offset;
                offsets_icetexture.VPosition = cls->findProperty(Name("VPosition"))->offset;
                offsets_icetexture.TickAccu = cls->findProperty(Name("TickAccu"))->offset;
                offsets_icetexture.OldUDisplace = cls->findProperty(Name("OldUDisplace"))->offset;
                offsets_icetexture.OldVDisplace = cls->findProperty(Name("OldVDisplace"))->offset;
                offsets_icetexture.OldGlassTex = cls->findProperty(Name("OldGlassTex"))->offset;
                offsets_icetexture.OldSourceTex = cls->findProperty(Name("OldSourceTex"))->offset;
                offsets_icetexture.LocalSource = cls->findProperty(Name("LocalSource"))->offset;
                offsets_icetexture.ForceRefresh = cls->findProperty(Name("ForceRefresh"))->offset;
            }
        }
    }
}
