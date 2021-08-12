/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_MAP_H
#define PDF_ENCODING_MAP_H

#include "PdfDefines.h"
#include "PdfObject.h"
#include "PdfName.h"
#include "PdfCharCodeMap.h"

namespace mm {

class PdfIndirectObjectList;
class PdfDynamicEncoding;

struct PDFMM_API PdfEncodingLimits
{
public:
    PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
        const PdfCharCode& firstCharCode, const PdfCharCode& lastCharCode);
    PdfEncodingLimits();

    unsigned char MinCodeSize;
    unsigned char MaxCodeSize;
    PdfCharCode FirstChar;     // The first defined character code
    PdfCharCode LastChar;      // The last defined character code
};

/** Represent a CID (Character ID) with full code unit information
 */
struct PdfCID
{
    unsigned Id;
    PdfCharCode Unit;

    PdfCID();

    /**
     * Create a CID that has an identical code unit of minimum size
     */
    explicit PdfCID(unsigned id);

    PdfCID(unsigned id, const PdfCharCode& unit);

    /**
     * Create a CID that has an identical code as a code unit representation
     */
    PdfCID(const PdfCharCode& unit);
};

/** 
 * A PdfEncodingMap is a low level interface to convert
 * between utf8 and encoded strings in Pdf.
 * \remarks Don't use this class directly, use PdfEncoding
 */
class PDFMM_API PdfEncodingMap
{
protected:
    PdfEncodingMap(const PdfEncodingLimits& limits);

public:
    /** Try decode next char code from utf8 string range
     */
    bool TryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from unicode code point
     */
    bool TryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const;

    /**
     * Get the char code from a span of unicode code points
     * \param codePoints it can be a single code point or a ligature
     * \return true if the code points match a character code
     */
    bool TryGetCharCode(const cspan<char32_t>& codePoints, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from cid
     */
    bool TryGetCharCode(unsigned cid, PdfCharCode& codeUnit) const;

    /** Try decode next cid from from encoded string range
     */
    bool TryGetNextCID(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCID& cid) const;

    /** Try decode next code points from encoded string range
     */
    bool TryGetNextCodePoints(std::string_view::iterator& it,
        const std::string_view::iterator& end, std::vector<char32_t>& codePoints) const;

    /** Try get code points from char code unit
     *
     * \remarks it will iterate available code sizes
     */
    bool TryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

    /** Try get CID identifier code from code unit
     * \param id the identifier of the CID. The identifier is
     * actually the PdfCID::Id part in the full CID representation
     */
    bool TryGetCIDId(const PdfCharCode& codeUnit, unsigned& id) const;

    const PdfEncodingLimits& GetLimits() const { return m_limits; }

    /**
     * True if the encoding has proper CID mapping
     *
     * Most maps represents just an identity CID encoding such
     * as built-in/difference encodings and rely on presence
     * of /FirstChar to map from a character code to a index
     * in the glyph list
     */
    virtual bool HasCIDMapping() const;

    /**
     * True if the encoding has ligatures support
     */
    virtual bool HasLigaturesSupport() const;

    void WriteToUnicodeCMap(PdfObject& cmapObj) const;

    /** Get an export object that will be used during font init
     * \param objects list to use to create document objects
     * \param name name to use
     * \param obj if not null the object will be used instead
     */
    void GetExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

public:
    virtual ~PdfEncodingMap();

protected:
    /**
     * Try get next char code unit from a utf8 string range
     *
     * \remarks Default implementation just throws
     */
    virtual bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from a ligature
     * \param ligature the span has at least 2 unicode code points
     * \remarks Default implementation just throws
     */
    virtual bool tryGetCharCodeSpan(const cspan<char32_t>& ligature, PdfCharCode& codeUnit) const;

    /**
     * Try get char code unit from unicode code point
     */
    virtual bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const = 0;

    /**
     * Get code points from a code unit
     *
     * \param wantCID true requires mapping to CID identifier, false for Unicode code points
     */
    virtual bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const = 0;

    /** Get an export object that will be used during font init
     *
     * \remarks Default implementation just throws
     */
    virtual void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

    /** During a WriteToUnicodeCMap append "beginbfchar" and "beginbfrange" entries
     *  "bf" stands for Base Font, see Adobe tecnichal notes #5014
     */
    virtual void appendBaseFontEntries(PdfStream& stream) const = 0;

    static void AppendUTF16CodeTo(PdfStream& stream, const cspan<char32_t>& codePoints, std::u16string& u16tmp);

private:
    bool tryGetNextCodePoints(std::string_view::iterator& it, const std::string_view::iterator& end,
        PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

private:
    PdfEncodingLimits m_limits;
};

/**
 * Basic PdfEncodingMap implementation using a PdfCharCodeMap
 */
class PDFMM_API PdfEncodingMapBase : public PdfEncodingMap
{
    friend class PdfDynamicEncoding;

protected:
    PdfEncodingMapBase(PdfCharCodeMap&& map, const std::optional<PdfEncodingLimits>& limits = { });

protected:
    bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const override;

    bool tryGetCharCodeSpan(const cspan<char32_t>& codePoints, PdfCharCode& codeUnit) const override;

    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void appendBaseFontEntries(PdfStream& stream) const override;

    inline const PdfCharCodeMap& GetCharMap() const { return *m_charMap; }

private:
    PdfEncodingMapBase(const std::shared_ptr<PdfCharCodeMap>& map);
    static PdfEncodingLimits findLimits(const PdfCharCodeMap& map);

private:
    std::shared_ptr<PdfCharCodeMap> m_charMap;
};

/**
 * PdfEncodingMap used by "simple" encodings like PdfPredefinedEncoding
 * or PdfDifferenceEncoding thats can define all their charset with
 * a single range
 */
class PDFMM_API PdfEncodingMapSimple : public PdfEncodingMap
{
protected:
    using PdfEncodingMap::PdfEncodingMap;

    void appendBaseFontEntries(PdfStream& stream) const override;
};

/** Dummy encoding map that will just throw
 */
class PdfDummyEncodingMap final : public PdfEncodingMap
{
public:
    PdfDummyEncodingMap();

    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void appendBaseFontEntries(PdfStream& stream) const override;
};

/** Convenience typedef for a const PdfEncoding shared ptr
 */
typedef std::shared_ptr<const PdfEncodingMap> PdfEncodingMapConstPtr;

}

#endif // PDF_ENCODING_MAP_H
