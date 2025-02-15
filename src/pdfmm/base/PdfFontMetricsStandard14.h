/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_STANDARD14_H
#define PDF_FONT_METRICS_STANDARD14_H

#include "PdfDeclarations.h"

#include "PdfFontMetrics.h"
#include "PdfRect.h"
#include "PdfVariant.h"

namespace mm {

struct Standard14FontData
{
    const unsigned short* Widths;
    unsigned WidthsSize;
    PdfFontDescriptorFlags Flags;
    uint16_t DefaultWidth;
    PdfFontStretch Stretch;
    int16_t Ascent;
    int16_t Descent;
    uint16_t XHeight;
    uint16_t CapHeight;
    int16_t ItalicAngle;
    int16_t Weight;
    int16_t StemV;
    int16_t StemH;
    int16_t StrikeoutPos;
    int16_t UnderlinePos;
    PdfRect BBox;
};

/**
 * This is the main class to handle the Standard14 metric data.
 */
class PDFMM_API PdfFontMetricsStandard14 final : public PdfFontMetricsBase
{
private:
    PdfFontMetricsStandard14(PdfStandard14FontType fontType,
        const Standard14FontData& data,
        std::unique_ptr<std::vector<double>> parsedWidths = { });

public:
    /** Create a Standard14 font metrics
     * \param fontObj optionally try to read a /Widths entry from the supplied
     */
    static std::unique_ptr<PdfFontMetricsStandard14> Create(
        PdfStandard14FontType fontType);
    static std::unique_ptr<PdfFontMetricsStandard14> Create(
        PdfStandard14FontType fontType, const PdfObject& fontObj);

public:
    unsigned GetGlyphCount() const override;

    bool TryGetGlyphWidth(unsigned gid, double& width) const override;

    bool HasUnicodeMapping() const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultWidthRaw() const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeOutPosition() const override;

    double GetStrikeOutThickness() const override;

    std::string GetFontName() const override;

    std::string GetBaseFontName() const override;

    std::string GetFontFamilyName() const override;

    PdfFontStretch GetFontStretch() const override;

    int GetWeightRaw() const override;

    PdfFontDescriptorFlags GetFlags() const override;

    void GetBoundingBox(std::vector<double>& bbox) const override;

    double GetItalicAngle() const override;

    double GetAscent() const override;

    double GetDescent() const override;

    double GetLeadingRaw() const override;

    double GetCapHeight() const override;

    double GetXHeightRaw() const override;

    double GetStemV() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    bool IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const override;

    unsigned GetFontFileLength1() const override;

    unsigned GetFontFileLength2() const override;

    unsigned GetFontFileLength3() const override;

    inline const Standard14FontData& GetRawData() const { return m_data; }

protected:
    bool getIsItalicHint() const override;

    bool getIsBoldHint() const override;

    datahandle getFontFileDataHandle() const override;

private:
    static std::unique_ptr<PdfFontMetricsStandard14> create(
        PdfStandard14FontType fontType, const PdfObject* fontObj = nullptr);

public:
    static std::shared_ptr<const PdfFontMetricsStandard14> GetInstance(PdfStandard14FontType std14Font);

private:
    PdfStandard14FontType m_Std14FontType;
    Standard14FontData m_data;
    // /Widths parsed from a font object, if available
    std::unique_ptr<std::vector<double>> m_parsedWidths;

    double m_Ascent;
    double m_Descent;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;
};

}

#endif // PDF_FONT_METRICS_STANDARD14_H
