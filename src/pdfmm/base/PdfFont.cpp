/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFont.h"

#include <regex>
#include <utfcpp/utf8.h>

#include <pdfmm/private/PdfEncodingPrivate.h>
#include <pdfmm/private/PdfStandard14FontData.h>
#include <pdfmm/private/outstringstream.h>

#include "PdfArray.h"
#include "PdfEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfInputStream.h"
#include "PdfObjectStream.h"
#include "PdfWriter.h"
#include "PdfCharCodeMap.h"
#include "PdfEncodingShim.h"
#include "PdfFontMetrics.h"
#include "PdfPage.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontManager.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfDocument.h"
#include "PdfStringStream.h"

using namespace std;
using namespace cmn;
using namespace mm;

static double getCharWidth(double widthGlyph, const PdfTextState& state, bool ignoreCharSpacing);
static string_view toString(PdfFontStretch stretch);

PdfFont::PdfFont(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfDictionaryElement(doc, "Font"), m_Metrics(metrics)
{
    if (metrics == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Metrics must me not null");

    this->initBase(encoding);
}

PdfFont::PdfFont(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfDictionaryElement(obj), m_Metrics(metrics)
{
    if (metrics == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Metrics must me not null");

    this->initBase(encoding);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    PdfStringStream out;
    out << "pdfmmFt" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.GetString());
}

PdfFont::~PdfFont() { }

bool PdfFont::TryGetSubstituteFont(PdfFont*& substFont)
{
    return TryGetSubstituteFont(PdfFontCreateFlags::None, substFont);
}

bool PdfFont::TryGetSubstituteFont(PdfFontCreateFlags initFlags, PdfFont*& substFont)
{
    auto encoding = GetEncoding();
    auto& metrics = GetMetrics();
    PdfFontMetricsConstPtr newMetrics;
    if (metrics.HasFontFileData())
    {
        newMetrics = PdfFontMetricsFreetype::FromMetrics(metrics);
    }
    else
    {
        // Early intercept Standard14 fonts
        PdfStandard14FontType std14Font;
        if (m_Metrics->IsStandard14FontMetrics(std14Font) ||
            PdfFont::IsStandard14Font(metrics.GetFontNameSafe(), true, std14Font))
        {
            newMetrics = PdfFontMetricsStandard14::GetInstance(std14Font);
        }
        else
        {
            PdfFontSearchParams params;
            params.Style = metrics.GetStyle();

            newMetrics = PdfFontManager::GetFontMetrics(metrics.GetFontNameSafe(true), params);
            if (newMetrics == nullptr)
            {
                substFont = nullptr;
                return false;
            }
        }
    }

    if (!encoding.HasValidToUnicodeMap())
    {
        shared_ptr<PdfCMapEncoding> toUnicode = newMetrics->CreateToUnicodeMap(encoding.GetLimits());
        encoding = PdfEncoding(encoding.GetEncodingMapPtr(), toUnicode);
    }

    PdfFontCreateParams params;
    params.Encoding = encoding;
    params.Flags = initFlags;
    auto newFont = PdfFont::Create(GetDocument(), newMetrics, params);
    if (newFont == nullptr)
    {
        substFont = nullptr;
        return false;
    }

    substFont = GetDocument().GetFontManager().AddImported(std::move(newFont));
    return true;
}

void PdfFont::initBase(const PdfEncoding& encoding)
{
    m_IsEmbedded = false;
    m_EmbeddingEnabled = false;
    m_SubsettingEnabled = false;
    m_cidToGidMap = m_Metrics->GetCIDToGIDMap();

    if (encoding.IsNull())
    {
        m_DynamicCIDMap = std::make_shared<PdfCharCodeMap>();
        m_DynamicToUnicodeMap = std::make_shared<PdfCharCodeMap>();
        m_Encoding.reset(new PdfDynamicEncoding(m_DynamicCIDMap, m_DynamicToUnicodeMap, *this));
    }
    else
    {
        m_Encoding.reset(new PdfEncodingShim(encoding, *this));
    }

    PdfStringStream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.GetString());

    // By default ensure the font has the /BaseFont name read
    // from the loaded metrics or inferred from a font file
    m_Name = m_Metrics->GetFontNameSafe();
}

void PdfFont::WriteStringToStream(PdfObjectStream& stream, const string_view& str) const
{
    outstringstream ostream;
    WriteStringToStream(ostream, str);
    stream.Append(ostream.str());
}

void PdfFont::WriteStringToStream(ostream& stream, const string_view& str) const
{
    auto encoded = m_Encoding->ConvertToEncoded(str);
    unique_ptr<PdfFilter> filter = PdfFilterFactory::Create(PdfFilterType::ASCIIHexDecode);
    charbuff buffer;
    filter->EncodeTo(buffer, encoded);

    stream << "<";
    stream.write(buffer.data(), buffer.size());
    stream << ">";
}

void PdfFont::InitImported(bool wantEmbed, bool wantSubset)
{
    PDFMM_ASSERT(!IsObjectLoaded());

    // No embedding implies no subsetting
    m_EmbeddingEnabled = wantEmbed;
    m_SubsettingEnabled = wantEmbed && wantSubset && SupportsSubsetting();
    if (m_SubsettingEnabled)
    {
        unsigned gid;
        char32_t spaceCp = U' ';
        if (TryGetGID(spaceCp, PdfGlyphAccess::Width, gid))
        {
            // If it exist a glyph for space character
            // always add it for subsetting
            unicodeview codepoints(&spaceCp, 1);
            PdfCID cid;
            (void)tryAddSubsetGID(gid, codepoints, cid);
        }
    }

    string fontName;
    if (m_Metrics->IsStandard14FontMetrics())
    {
        fontName = m_Metrics->GetFontName();
    }
    else
    {
        fontName = m_Metrics->GetBaseFontName();
        if ((m_Metrics->GetStyle() & PdfFontStyle::Bold) == PdfFontStyle::Bold)
        {
            if ((m_Metrics->GetStyle() & PdfFontStyle::Italic) == PdfFontStyle::Italic)
                fontName += ",BoldItalic";
            else
                fontName += ",Bold";
        }
        else if ((m_Metrics->GetStyle() & PdfFontStyle::Italic) == PdfFontStyle::Italic)
        {
            fontName += ",Italic";
        }
    }

    if (m_SubsettingEnabled)
    {
        m_SubsetPrefix = GetDocument().GetFontManager().GenerateSubsetPrefix();
        PDFMM_ASSERT(!m_SubsetPrefix.empty());
        fontName = m_SubsetPrefix + fontName;
    }

    m_Name = fontName;
    initImported();

    if (m_EmbeddingEnabled && !m_SubsettingEnabled
        && !m_Encoding->IsDynamicEncoding())
    {
        // Perform embedded immediately if subsetting is
        // not enabled and there's no dynamic encoding
        embedFont();
        m_IsEmbedded = true;
    }
}

void PdfFont::EmbedFont()
{
    if (m_IsEmbedded || !m_EmbeddingEnabled)
        return;

    if (m_SubsettingEnabled)
        embedFontSubset();
    else
        embedFont();

    m_IsEmbedded = true;
}

void PdfFont::embedFont()
{
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Embedding not implemented for this font type");
}

void PdfFont::embedFontSubset()
{
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Subsetting not implemented for this font type");
}

unsigned PdfFont::GetGID(char32_t codePoint, PdfGlyphAccess access) const
{
    unsigned gid;
    if (!TryGetGID(codePoint, access, gid))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Can't find a gid");

    return gid;
}

bool PdfFont::TryGetGID(char32_t codePoint, PdfGlyphAccess access, unsigned& gid) const
{
    if (IsObjectLoaded() || !m_Metrics->HasUnicodeMapping())
    {
        PdfCharCode codeUnit;
        unsigned cid;
        if (!m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoint, codeUnit)
            || !m_Encoding->TryGetCIDId(codeUnit, cid))
        {
            gid = 0;
            return false;
        }

        return TryMapCIDToGID(cid, access, gid);
    }
    else
    {
        return m_Metrics->TryGetGID(codePoint, gid);
    }
}

double PdfFont::GetStringWidth(const string_view& str, const PdfTextState& state) const
{
    // Ignore failures
    double width;
    (void)TryGetStringWidth(str, state, width);
    return width;
}

bool PdfFont::TryGetStringWidth(const string_view& str, const PdfTextState& state, double& width) const
{
    vector<unsigned> gids;
    bool success = tryConvertToGIDs(str, PdfGlyphAccess::Width, gids);
    width = 0;
    for (unsigned i = 0; i < gids.size(); i++)
        width += getCharWidth(m_Metrics->GetGlyphWidth(gids[i]), state, false);

    return success;
}

double PdfFont::GetStringWidth(const PdfString& encodedStr, const PdfTextState& state) const
{
    // Ignore failures
    double width;
    (void)TryGetStringWidth(encodedStr, state, width);
    return width;
}

bool PdfFont::TryGetStringWidth(const PdfString& encodedStr, const PdfTextState& state, double& width) const
{
    vector<PdfCID> cids;
    bool success = true;
    if (!m_Encoding->TryConvertToCIDs(encodedStr, cids))
        success = false;

    width = getStringWidth(cids, state);
    return success;
}

double PdfFont::GetCharWidth(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing) const
{
    // Ignore failures
    double width;
    if (!TryGetCharWidth(codePoint, state, ignoreCharSpacing, width))
        return GetDefaultCharWidth(state, ignoreCharSpacing);

    return width;
}

bool PdfFont::TryGetCharWidth(char32_t codePoint, const PdfTextState& state, double& width) const
{
    return TryGetCharWidth(codePoint, state, false, width);
}

bool PdfFont::TryGetCharWidth(char32_t codePoint, const PdfTextState& state,
    bool ignoreCharSpacing, double& width) const
{
    unsigned gid;
    if (TryGetGID(codePoint, PdfGlyphAccess::Width, gid))
    {
        width = getCharWidth(m_Metrics->GetGlyphWidth(gid), state, ignoreCharSpacing);
        return true;
    }
    else
    {
        width = getCharWidth(m_Metrics->GetDefaultWidth(), state, ignoreCharSpacing);
        return false;
    }
}

double PdfFont::GetDefaultCharWidth(const PdfTextState& state, bool ignoreCharSpacing) const
{
    if (ignoreCharSpacing)
    {
        return m_Metrics->GetDefaultWidth() * state.FontSize
            * state.FontScale;
    }
    else
    {
        return (m_Metrics->GetDefaultWidth() * state.FontSize
            + state.CharSpacing) * state.FontScale;
    }
}

double PdfFont::GetCIDWidthRaw(unsigned cid) const
{
    unsigned gid;
    if (!TryMapCIDToGID(cid, PdfGlyphAccess::Width, gid))
        return m_Metrics->GetDefaultWidth();

    return m_Metrics->GetGlyphWidth(gid);
}

void PdfFont::GetBoundingBox(PdfArray& arr) const
{
    auto& matrix = m_Metrics->GetMatrix();
    arr.Clear();
    vector<double> bbox;
    m_Metrics->GetBoundingBox(bbox);
    arr.Add(PdfObject(static_cast<int64_t>(std::round(bbox[0] / matrix[0]))));
    arr.Add(PdfObject(static_cast<int64_t>(std::round(bbox[1] / matrix[3]))));
    arr.Add(PdfObject(static_cast<int64_t>(std::round(bbox[2] / matrix[0]))));
    arr.Add(PdfObject(static_cast<int64_t>(std::round(bbox[3] / matrix[3]))));
}

void PdfFont::FillDescriptor(PdfDictionary& dict) const
{
    // Optional values
    int weight;
    double xHeight;
    double stemH;
    string familyName;
    double leading;
    double avgWidth;
    double maxWidth;
    double defaultWidth;
    PdfFontStretch stretch;

    dict.AddKey("FontName", PdfName(this->GetName()));
    if ((familyName = m_Metrics->GetFontFamilyName()).length() != 0)
        dict.AddKey("FontFamily", PdfString(familyName));
    if ((stretch = m_Metrics->GetFontStretch()) != PdfFontStretch::Unknown)
        dict.AddKey("FontStretch", PdfName(toString(stretch)));
    dict.AddKey(PdfName::KeyFlags, static_cast<int64_t>(m_Metrics->GetFlags()));
    dict.AddKey("ItalicAngle", static_cast<int64_t>(std::round(m_Metrics->GetItalicAngle())));

    PdfArray bbox;
    GetBoundingBox(bbox);

    auto& matrix = m_Metrics->GetMatrix();
    if (GetType() == PdfFontType::Type3)
    {
        // ISO 32000-1:2008 "should be used for Type 3 fonts in Tagged PDF documents"
        dict.AddKey("FontWeight", static_cast<int64_t>(m_Metrics->GetWeight()));
    }
    else
    {
        if ((weight = m_Metrics->GetWeightRaw()) > 0)
            dict.AddKey("FontWeight", static_cast<int64_t>(weight));

        // The following entries are all optional in /Type3 fonts
        dict.AddKey("FontBBox", bbox);
        dict.AddKey("Ascent", static_cast<int64_t>(std::round(m_Metrics->GetAscent() / matrix[3])));
        dict.AddKey("Descent", static_cast<int64_t>(std::round(m_Metrics->GetDescent() / matrix[3])));
        dict.AddKey("CapHeight", static_cast<int64_t>(std::round(m_Metrics->GetCapHeight() / matrix[3])));
        // NOTE: StemV is measured horizontally
        dict.AddKey("StemV", static_cast<int64_t>(std::round(m_Metrics->GetStemV() / matrix[0])));

        if ((xHeight = m_Metrics->GetXHeightRaw()) > 0)
            dict.AddKey("XHeight", static_cast<int64_t>(std::round(xHeight / matrix[3])));

        if ((stemH = m_Metrics->GetStemHRaw()) > 0)
        {
            // NOTE: StemH is measured vertically
            dict.AddKey("StemH", static_cast<int64_t>(std::round(stemH / matrix[3])));
        }

        if (!IsCIDKeyed())
        {
            // Default for /MissingWidth is 0
            // NOTE: We assume CID keyed fonts to use the /DW entry
            // in the CIDFont dictionary instead. See 9.7.4.3 Glyph
            // Metrics in CIDFonts in ISO 32000-1:2008
            if ((defaultWidth = m_Metrics->GetDefaultWidthRaw()) > 0)
                dict.AddKey("MissingWidth", static_cast<int64_t>(std::round(defaultWidth / matrix[0])));
        }
    }

    if ((leading = m_Metrics->GetLeadingRaw()) > 0)
        dict.AddKey("Leading", static_cast<int64_t>(std::round(leading / matrix[3])));
    if ((avgWidth = m_Metrics->GetAvgWidthRaw()) > 0)
        dict.AddKey("AvgWidth", static_cast<int64_t>(std::round(avgWidth / matrix[0])));
    if ((maxWidth = m_Metrics->GetMaxWidthRaw()) > 0)
        dict.AddKey("MaxWidth", static_cast<int64_t>(std::round(maxWidth / matrix[0])));
}

void PdfFont::EmbedFontFile(PdfObject& descriptor)
{
    auto fontdata = m_Metrics->GetOrLoadFontFileData();
    if (fontdata.empty())
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    switch (m_Metrics->GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::CIDType1:
            EmbedFontFileType1(descriptor, fontdata, m_Metrics->GetFontFileLength1(), m_Metrics->GetFontFileLength2(), m_Metrics->GetFontFileLength3());
            break;
        case PdfFontFileType::Type1CCF:
            EmbedFontFileType1CCF(descriptor, fontdata);
            break;
        case PdfFontFileType::TrueType:
            EmbedFontFileTrueType(descriptor, fontdata);
            break;
        case PdfFontFileType::OpenType:
            EmbedFontFileOpenType(descriptor, fontdata);
            break;
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported font type embedding");
    }
}

void PdfFont::EmbedFontFileType1(PdfObject& descriptor, const bufferview& data, unsigned length1, unsigned length2, unsigned length3)
{
    auto contents = embedFontFileData(descriptor, "FontFile", data);
    contents->GetDictionary().AddKey("Length1", static_cast<int64_t>(length1));
    contents->GetDictionary().AddKey("Length2", static_cast<int64_t>(length2));
    contents->GetDictionary().AddKey("Length3", static_cast<int64_t>(length3));
}

void PdfFont::EmbedFontFileType1CCF(PdfObject& descriptor, const bufferview& data)
{
    auto contents = embedFontFileData(descriptor, "FontFile3", data);
    PdfName subtype;
    if (IsCIDKeyed())
        subtype = PdfName("CIDFontType0C");
    else
        subtype = PdfName("Type1C");

    contents->GetDictionary().AddKey(PdfName::KeySubtype, subtype);
}

void PdfFont::EmbedFontFileTrueType(PdfObject& descriptor, const bufferview& data)
{
    auto contents = embedFontFileData(descriptor, "FontFile2", data);
    contents->GetDictionary().AddKey("Length1", static_cast<int64_t>(data.size()));
}

void PdfFont::EmbedFontFileOpenType(PdfObject& descriptor, const bufferview& data)
{
    auto contents = embedFontFileData(descriptor, "FontFile3", data);
    contents->GetDictionary().AddKey(PdfName::KeySubtype, PdfName("OpenType"));
}

PdfObject* PdfFont::embedFontFileData(PdfObject& descriptor, const PdfName& fontFileName, const bufferview& data)
{
    auto contents = GetDocument().GetObjects().CreateDictionaryObject();
    descriptor.GetDictionary().AddKeyIndirect(fontFileName, contents);
    contents->GetOrCreateStream().Set(data);
    return contents;
}

void PdfFont::initImported()
{
    // By default do nothing
}

double PdfFont::getStringWidth(const vector<PdfCID>& cids, const PdfTextState& state) const
{
    double width = 0;
    for (auto& cid : cids)
        width += getCharWidth(GetCIDWidthRaw(cid.Id), state, false);

    return width;
}

double PdfFont::GetLineSpacing(const PdfTextState& state) const
{
    return m_Metrics->GetLineSpacing() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlineThickness(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlineThickness() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlinePosition(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlinePosition() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeOutPosition(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeOutPosition() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeOutThickness(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeOutThickness() * state.FontSize;
}

double PdfFont::GetAscent(const PdfTextState& state) const
{
    return m_Metrics->GetAscent() * state.FontSize;
}

double PdfFont::GetDescent(const PdfTextState& state) const
{
    return m_Metrics->GetDescent() * state.FontSize;
}

PdfCID PdfFont::AddSubsetGIDSafe(unsigned gid, const unicodeview& codePoints)
{
    PDFMM_ASSERT(m_SubsettingEnabled && !m_IsEmbedded);
    auto found = m_SubsetGIDs.find(gid);
    if (found != m_SubsetGIDs.end())
        return found->second;

    PdfCID ret;
    if (!tryAddSubsetGID(gid, codePoints, ret))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile,
            "The encoding doesn't support these characters or the gid is already present");
    }

    return ret;
}

PdfCharCode PdfFont::AddCharCodeSafe(unsigned gid, const unicodeview& codePoints)
{
    // NOTE: This method is supported only when doing fully embedding
    // of an imported font with valid unicode mapping
    PDFMM_ASSERT(!m_SubsettingEnabled
        && m_Encoding->IsDynamicEncoding()
        && !IsObjectLoaded()
        && m_Metrics->HasUnicodeMapping());

    PdfCharCode code;
    if (m_DynamicToUnicodeMap->TryGetCharCode(codePoints, code))
        return code;

    code = PdfCharCode(m_DynamicToUnicodeMap->GetSize());
    // NOTE: We assume in this context cid == gid identity
    m_DynamicCIDMap->PushMapping(code, gid);
    m_DynamicToUnicodeMap->PushMapping(code, codePoints);
    return code;
}

bool PdfFont::tryConvertToGIDs(const std::string_view& utf8Str, PdfGlyphAccess access, std::vector<unsigned>& gids) const
{
    bool success = true;
    if (IsObjectLoaded() || !m_Metrics->HasUnicodeMapping())
    {
        // NOTE: This is a best effort strategy. It's not intended to
        // be accurate in loaded fonts
        auto it = utf8Str.begin();
        auto end = utf8Str.end();

        auto& toUnicode = m_Encoding->GetToUnicodeMapSafe();
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            PdfCharCode codeUnit;
            unsigned cid;
            unsigned gid;
            if (toUnicode.TryGetCharCode(cp, codeUnit))
            {
                if (m_Encoding->TryGetCIDId(codeUnit, cid))
                {
                    if (!TryMapCIDToGID(cid, access, gid))
                    {
                        // Fallback
                        gid = cid;
                        success = false;
                    }
                }
                else
                {
                    // Fallback
                    gid = codeUnit.Code;
                    success = false;
                }
            }
            else
            {
                // Fallback
                gid = cp;
                success = false;
            }

            gids.push_back(gid);
        }
    }
    else
    {
        auto it = utf8Str.begin();
        auto end = utf8Str.end();
        vector<unsigned> gids;
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            unsigned gid;
            if (!m_Metrics->TryGetGID(cp, gid))
            {
                // Fallback
                gid = cp;
                success = false;
            }

            gids.push_back(gid);
        }

        // Try to subsistute GIDs for fonts that support
        // a glyph substitution mechanism
        vector<unsigned char> backwardMap;
        m_Metrics->SubstituteGIDs(gids, backwardMap);
    }

    return success;
}

bool PdfFont::tryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid)
{
    (void)codePoints;
    PDFMM_ASSERT(m_SubsettingEnabled && !IsObjectLoaded());
    if (m_Encoding->IsDynamicEncoding())
    {
        // We start numberings CIDs from 1 since CID 0
        // is reserved for fallbacks
        auto inserted = m_SubsetGIDs.try_emplace(gid, PdfCID((unsigned)m_SubsetGIDs.size() + 1));
        cid = inserted.first->second;
        if (!inserted.second)
            return false;

        m_DynamicCIDMap->PushMapping(cid.Unit, cid.Id);
        m_DynamicToUnicodeMap->PushMapping(cid.Unit, codePoints);
        return true;
    }
    else
    {
        PdfCharCode codeUnit;
        if (!m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoints, codeUnit))
        {
            cid = { };
            return false;
        }

        // We start numberings CIDs from 1 since CID 0
        // is reserved for fallbacks
        auto inserted = m_SubsetGIDs.try_emplace(gid, PdfCID((unsigned)m_SubsetGIDs.size() + 1, codeUnit));
        cid = inserted.first->second;
        return inserted.second;
    }
}

void PdfFont::AddSubsetGIDs(const PdfString& encodedStr)
{
    if (IsObjectLoaded())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't add used GIDs to a loaded font");

    if (m_Encoding->IsDynamicEncoding())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't add used GIDs from an encoded string to a font with a dynamic encoding");

    if (!m_SubsettingEnabled)
        return;

    if (m_IsEmbedded)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "Can't add more subsetting glyphs on an already embedded font");
    }

    vector<PdfCID> cids;
    unsigned gid;
    (void)GetEncoding().TryConvertToCIDs(encodedStr, cids);
    for (auto& cid : cids)
    {
        if (TryMapCIDToGID(cid.Id, PdfGlyphAccess::FontProgram, gid))
        {
            (void)m_SubsetGIDs.try_emplace(gid,
                PdfCID((unsigned)m_SubsetGIDs.size() + 1, cid.Unit));
        }
    }
}

bool PdfFont::SupportsSubsetting() const
{
    return false;
}

bool PdfFont::IsStandard14Font() const
{
    return m_Metrics->IsStandard14FontMetrics();
}

bool PdfFont::IsStandard14Font(PdfStandard14FontType& std14Font) const
{
    return m_Metrics->IsStandard14FontMetrics(std14Font);
}

PdfObject& PdfFont::GetDescendantFontObject()
{
    auto obj = getDescendantFontObject();
    if (obj == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Descendant font object must not be null");

    return *obj;
}

bool PdfFont::TryMapCIDToGID(unsigned cid, PdfGlyphAccess access, unsigned& gid) const
{
    if (m_cidToGidMap != nullptr && m_cidToGidMap->HasGlyphAccess(access))
        return m_cidToGidMap->TryMapCIDToGID(cid, gid);

    return tryMapCIDToGID(cid, gid);
}

bool PdfFont::tryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    PDFMM_ASSERT(!IsObjectLoaded());
    if (m_Encoding->IsSimpleEncoding() && m_Metrics->HasUnicodeMapping())
    {
        // Simple encodings must retrieve the gid from the
        // metrics using the mapped unicode code point
        char32_t mappedCodePoint = m_Encoding->GetCodePoint(cid);
        if (mappedCodePoint == U'\0'
            || !m_Metrics->TryGetGID(mappedCodePoint, gid))
        {
            gid = 0;
            return false;
        }

        return true;
    }
    else
    {
        // The font is not loaded, hence it's imported:
        // we assume cid == gid identity. CHECK-ME: Does it work
        // if we font to create a substitute font of a loaded font
        // with a /CIDToGIDMap ???
        gid = cid;
        return true;
    }
}

PdfObject* PdfFont::getDescendantFontObject()
{
    // By default return null
    return nullptr;
}

string PdfFont::ExtractBaseName(const string_view& fontName, bool& isItalic, bool& isBold)
{
    // TABLE H.3 Names of standard fonts
    string name = (string)fontName;
    regex regex;
    smatch matches;
    isItalic = false;
    isBold = false;
    
    // NOTE: For some reasons, "^[A-Z]{6}\+" doesn't work
    regex = std::regex("^[A-Z][A-Z][A-Z][A-Z][A-Z][A-Z]\\+", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        // 5.5.3 Font Subsets: Remove EOODIA+ like prefixes
        name.erase(matches[0].first - name.begin(), 7);
    }

    regex = std::regex("[,-]BoldItalic", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("BoldItalic") + 1);
        isBold = true;
        isItalic = true;
    }

    regex = std::regex("[,-]BoldOblique", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("BoldOblique") + 1);
        isBold = true;
        isItalic = true;
    }

    regex = std::regex("[,-]Bold", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Bold") + 1);
        isBold = true;
    }

    regex = std::regex("[,-]Italic", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Italic") + 1);
        isItalic = true;
    }

    regex = std::regex("[,-]Oblique", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Oblique") + 1);
        isItalic = true;
    }

    regex = std::regex("[,-]Regular", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Oblique") + 1);
        // Nothing to set
    }

    // 5.5.2 TrueType Fonts: If the name contains any spaces, the spaces are removed
    name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
    return name;
}

string PdfFont::ExtractBaseName(const string_view& fontName)
{
    bool isItalic;
    bool isBold;
    return ExtractBaseName(fontName, isItalic, isBold);
}

string_view PdfFont::GetStandard14FontName(PdfStandard14FontType stdFont)
{
    return ::GetStandard14FontName(stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, true, stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, useAltNames, stdFont);
}

bool PdfFont::IsCIDKeyed() const
{
    switch (GetType())
    {
        case PdfFontType::CIDTrueType:
        case PdfFontType::CIDType1:
            return true;
        default:
            return false;
    }
}

bool PdfFont::IsObjectLoaded() const
{
    return false;
}

// TODO:
// Handle word spacing Tw
// 5.2.2 Word Spacing
// Note: Word spacing is applied to every occurrence of the single-byte character code
// 32 in a string when using a simple font or a composite font that defines code 32 as a
// single - byte code.It does not apply to occurrences of the byte value 32 in multiplebyte
// codes.
double getCharWidth(double glyphWidth, const PdfTextState& state, bool ignoreCharSpacing)
{
    if (ignoreCharSpacing)
        return glyphWidth * state.FontSize * state.FontScale;
    else
        return (glyphWidth * state.FontSize + state.CharSpacing) * state.FontScale;
}

string_view toString(PdfFontStretch stretch)
{
    switch (stretch)
    {
        case PdfFontStretch::UltraCondensed:
            return "UltraCondensed";
        case PdfFontStretch::ExtraCondensed:
            return "ExtraCondensed";
        case PdfFontStretch::Condensed:
            return "Condensed";
        case PdfFontStretch::SemiCondensed:
            return "SemiCondensed";
        case PdfFontStretch::Normal:
            return "Normal";
        case PdfFontStretch::SemiExpanded:
            return "SemiExpanded";
        case PdfFontStretch::Expanded:
            return "Expanded";
        case PdfFontStretch::ExtraExpanded:
            return "ExtraExpanded";
        case PdfFontStretch::UltraExpanded:
            return "UltraExpanded";
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}
