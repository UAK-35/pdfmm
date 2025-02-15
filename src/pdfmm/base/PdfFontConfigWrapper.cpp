/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontConfigWrapper.h"

#include <fontconfig/fontconfig.h>

using namespace std;
using namespace mm;

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
// Windows, Android and Apple architectures don't primarly
// use fontconfig. We can supply a fallback configuration,
// if a system configuration is not found
#define HAS_FALLBACK_CONFIGURATION
#endif

PdfFontConfigWrapper::PdfFontConfigWrapper(FcConfig* fcConfig)
    : m_FcConfig(fcConfig)
{
    if (fcConfig == nullptr)
        createDefaultConfig();
}

PdfFontConfigWrapper::~PdfFontConfigWrapper()
{
    FcConfigDestroy(m_FcConfig);
}

string PdfFontConfigWrapper::GetFontConfigFontPath(const string_view fontName,
    PdfFontStyle style, unsigned& faceIndex)
{
    FcPattern* pattern;
    FcPattern* matched;
    FcResult result = FcResultMatch;
    FcValue value;

    bool isItalic = (style & PdfFontStyle::Italic) == PdfFontStyle::Italic;
    bool isBold = (style & PdfFontStyle::Bold) == PdfFontStyle::Bold;

    // Build a pattern to search using postscript name, bold and italic
    pattern = FcPatternBuild(0, FC_POSTSCRIPT_NAME, FcTypeString, fontName.data(),
        FC_WEIGHT, FcTypeInteger, (isBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
        FC_SLANT, FcTypeInteger, (isItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),
        static_cast<char*>(0));

    FcDefaultSubstitute(pattern);

    if (!FcConfigSubstitute(m_FcConfig, pattern, FcMatchFont))
    {
        FcPatternDestroy(pattern);
        faceIndex = 0;
        return { };
    }

    string path;
    matched = FcFontMatch(m_FcConfig, pattern, &result);
    if (result != FcResultNoMatch)
    {
        (void)FcPatternGet(matched, FC_FILE, 0, &value);
        path = reinterpret_cast<const char*>(value.u.s);
        (void)FcPatternGet(matched, FC_INDEX, 0, &value);
        faceIndex = (unsigned)value.u.i;
#ifdef PDFMM_VERBOSE_DEBUG
        mm::LogMessage(PdfLogSeverity::Debug,
            "Got Font {}, face index {} for {}", path, faceIndex, fontName);
#endif // PDFMM_VERBOSE_DEBUG
    }

    FcPatternDestroy(pattern);
    FcPatternDestroy(matched);
    return path;
}

void PdfFontConfigWrapper::AddFontDirectory(const string_view& path)
{
    if (!FcConfigAppFontAddDir(m_FcConfig, (const FcChar8*)path.data()))
        throw runtime_error("Unable to add font directory");
}

FcConfig* PdfFontConfigWrapper::GetFcConfig()
{
    return m_FcConfig;
}

void PdfFontConfigWrapper::createDefaultConfig()
{
#ifdef _WIN32
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>WINDOWSFONTDIR</dir>
    <dir prefix="xdg">fonts</dir>
    <cachedir>LOCAL_APPDATA_FONTCONFIG_CACHE</cachedir>
    <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
)";
#elif __ANDROID__
    // On android fonts are located in /system/fonts
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>/system/fonts</dir>
</fontconfig>
)";
#elif __APPLE__
    // Fonts location https://stackoverflow.com/a/2557291/213871
    const char* fontconf =
        R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
    <dir>/System/Library/Fonts</dir>
</fontconfig>
)";
#endif

#ifdef HAS_FALLBACK_CONFIGURATION
    // Implement the fallback as discussed in fontconfig mailing list
    // https://lists.freedesktop.org/archives/fontconfig/2022-February/006883.html

    auto config = FcConfigCreate();
    if (config == nullptr)
        throw runtime_error("Could not allocate font config");

    // Manually try to load the config to determine
    // if a system configuration exists. Tell FontConfig
    // to not complain if it doesn't
    (void)FcConfigParseAndLoad(config, nullptr, FcFalse);

    auto configFiles = FcConfigGetConfigFiles(config);
    if (FcStrListNext(configFiles) == nullptr)
    {
        // No system config found, supply a fallback configuration
        if (!FcConfigParseAndLoadFromMemory(config, (const FcChar8*)fontconf, true))
        {
            FcConfigDestroy(config);
            throw runtime_error("Could not parse font config");
        }

        // Load fonts for the config
        if (!FcConfigBuildFonts(config))
        {
            FcConfigDestroy(config);
            throw runtime_error("Could not load fonts in fontconfig");
        }

        m_FcConfig = config;
    }
    else
    {
        // Destroy the temporary config
        FcStrListDone(configFiles);
        FcConfigDestroy(config);
#endif
        // Default initialize a local FontConfig configuration
        // http://mces.blogspot.com/2015/05/how-to-use-custom-application-fonts.html
        m_FcConfig = FcInitLoadConfigAndFonts();
        PDFMM_ASSERT(m_FcConfig != nullptr);
#ifdef HAS_FALLBACK_CONFIGURATION
    }
#endif
}
