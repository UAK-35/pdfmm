/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DEFINES_H
#define PDF_DEFINES_H

/** \file PdfDeclarations.h
 *        This file should be included as the FIRST file in every header of
 *        pdfmm lib. It includes all standard files, defines some useful
 *        macros, some datatypes and all important enumeration types. On
 *        supporting platforms it will be precompiled to speed compilation.
 */

 // Include some base macro definitions
#include "basedefs.h"

// Include common system files
#include "baseincludes.h"

#include "PdfCompilerCompat.h"
#include "PdfVersion.h"

// Error Handling Defines
#include "PdfError.h"

#define FORWARD_DECLARE_FCONFIG()\
extern "C" {\
    struct _FcConfig;\
    typedef struct _FcConfig FcConfig;\
}

#define FORWARD_DECLARE_FREETYPE()\
extern "C"\
{\
    struct FT_FaceRec_;\
    typedef struct FT_FaceRec_* FT_Face;\
}

/**
 * \namespace mm
 *
 * All classes, functions, types and enums of pdfmm
 * are members of these namespace.
 *
 * If you use pdfmm, you might want to add the line:
 *       using namespace mm;
 * to your application.
 */
namespace mm {

// NOTE: This may change in the future
using Matrix2D = std::array<double, 6>;

/** A backing storage for a CID to GID map
 * \remarks It must preserve ordering
 */
using CIDToGIDMap = std::map<unsigned, unsigned>;

// Enums

/**
 * Enum to identify different versions of the PDF file format
 */
enum class PdfVersion
{
    Unknown = 0,
    V1_0 = 10,       ///< PDF 1.0
    V1_1 = 11,       ///< PDF 1.1
    V1_2 = 12,       ///< PDF 1.2
    V1_3 = 13,       ///< PDF 1.3
    V1_4 = 14,       ///< PDF 1.4
    V1_5 = 15,       ///< PDF 1.5
    V1_6 = 16,       ///< PDF 1.6
    V1_7 = 17,       ///< PDF 1.7
    V2_0 = 20,       ///< PDF 2.0
};

/** The default PDF Version used by new PDF documents
 *  in pdfmm.
 */
constexpr PdfVersion PdfVersionDefault = PdfVersion::V1_4;

enum class PdfALevel
{
    Unknown = 0,
    L1B,
    L1A,
    L2B,
    L2A,
    L2U,
    L3B,
    L3A,
    L3U,
    L4E,
    L4F,
};

enum class PdfStringState : uint8_t
{
    RawBuffer,          ///< The string is an unvaluated raw buffer
    Ascii,              ///< The string use characters that are in both Ascii and PdfDocEncoding charsets
    PdfDocEncoding,     ///< The string uses characters that are in the whole PdfDocEncoding charset
    Unicode,            ///< The string uses characters that are in the whole Unicode charset
};

enum class PdfEncodingMapType
{
    Indeterminate,              ///< Indeterminate map type, such as identity encodings
    Simple,                     ///< A legacy encoding, such as built-in or difference
    CMap                        ///< A proper CMap encoding or pre-defined CMap names
};

/**
 * Specify additional options for writing the PDF.
 */
enum class PdfWriteFlags
{
    None = 0,
    Clean = 1,             ///< Create a PDF that is readable in a text editor, i.e. insert spaces and linebreaks between tokens
    NoInlineLiteral = 2,   ///< Don't write spaces before literal types (numerical, references, null)

    // NOTE: The following flags are actually never set but
    // they are kept for documenting some PDF peculiarities
    // when writing compact code
    NoPDFAPreserve = 4,    ///< When writing compact (PdfWriteFlags::Clean is unset) code, preserving PDF/A compliance is not required
};

/**
 * Every PDF datatype that can occur in a PDF file
 * is referenced by an own enum (e.g. Bool or String).
 *
 * \see PdfVariant
 *
 * Remember to update PdfVariant::GetDataTypeString() when adding members here.
 */
enum class PdfDataType : uint8_t
{
    Unknown = 0,           ///< The Datatype is unknown. The value is chosen to enable value storage in 8-bit unsigned integer
    Bool,                  ///< Boolean datatype: Accepts the values "true" and "false"
    Number,                ///< Number datatype for integer values
    Real,                  ///< Real datatype for floating point numbers
    String,                ///< String datatype in PDF file. Strings have the form (Hallo World!) in PDF files. \see PdfString
    Name,                  ///< Name datatype. Names are used as keys in dictionary to reference values. \see PdfName
    Array,                 ///< An array of other PDF data types
    Dictionary,            ///< A dictionary associates keys with values. A key can have another dictionary as value
    Null,                  ///< The null datatype is always null
    Reference,             ///< The reference datatype contains references to PDF objects in the PDF file of the form 4 0 R. \see PdfObject
    RawData,               ///< Raw PDF data
};

enum class PdfTextExtractFlags
{
    None = 0,
    IgnoreCase = 1,
    KeepWhiteTokens = 2,
    TokenizeWords = 4,
};

enum class PdfXObjectType
{
    Unknown = 0,
    Form,
    Image,
    PostScript,
};

/**
 * Every filter that can be used to encode a stream
 * in a PDF file is referenced by an own enum value.
 * Common filters are PdfFilterType::FlateDecode (i.e. Zip) or
 * PdfFilterType::ASCIIHexDecode
 */
enum class PdfFilterType
{
    None = 0,                  ///< Do not use any filtering
    ASCIIHexDecode,            ///< Converts data from and to hexadecimal. Increases size of the data by a factor of 2! \see PdfHexFilter
    ASCII85Decode,             ///< Converts to and from Ascii85 encoding. \see PdfAscii85Filter
    LZWDecode,
    FlateDecode,               ///< Compress data using the Flate algorithm of ZLib. This filter is recommended to be used always. \see PdfFlateFilter
    RunLengthDecode,           ///< Run length decode data. \see PdfRLEFilter
    CCITTFaxDecode,
    JBIG2Decode,
    DCTDecode,
    JPXDecode,
    Crypt
};

/**
 * Enum for the font descriptor flags
 *
 * See ISO 32000-1:2008 Table 121 — Font flags
 */
enum class PdfFontDescriptorFlags
{
    None        = 0,
    FixedPitch  = 1 << 0,
    Serif       = 1 << 1,
    Symbolic    = 1 << 2, ///< Font contains glyphs outside the Standard Latin character set. It does **not** mean the font is a symbol like font 
    Script      = 1 << 3,
    NonSymbolic = 1 << 5, ///< Font uses the Standard Latin character set or a subset of it. It does **not** mean the font uses only textual/non symbolic characters
    Italic      = 1 << 6, ///< Glyphs have dominant vertical strokes that are slanted
    AllCap      = 1 << 16,
    SmallCap    = 1 << 17,
    ForceBold   = 1 << 18, ///< Determine whether bold glyphs shall be painted with extra pixels even
};

enum class PdfFontStretch
{
    Unknown = 0,
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    Normal,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded,
};

/** Enum specifying the type of the font
 *
 * It doesn't necessarily specify the underline font file type,
 * as per the value Standard14. To know that, refer to
 * PdfFontMetrics::GetFontFileType()
 */
enum class PdfFontType
{
    Unknown = 0,
    Type1,
    Type3,
    TrueType,
    CIDType1,    ///< This is a "CIDFontType0"
    CIDTrueType, ///< This is a "CIDFontType2"
};

enum class PdfFontFileType
{
    // Table 126 – Embedded font organization for various font types
    Unknown = 0,
    Type1,
    Type1CCF,    ///< Compact Font Representation for /Type1 fonts
    CIDType1,    ///< This is a Type1 font that can be used only in CID Fonts
    Type3,
    TrueType,
    OpenType     ///< OpenType font. This is /Subtype "OpenType" for /FontFile3
};

/** Font style flags used during searches
 */
enum class PdfFontStyle : uint8_t
{
    Regular = 0,
    Italic = 1,
    Bold = 2,
};

/** When accessing a glyph, there may be a difference in
 * the glyph ID to retrieve the width or to index it
 * within the font program
 */
enum class PdfGlyphAccess : uint8_t
{
    Width = 1,         ///< The glyph is accessed in the widths arrays (/Widths, /W1 keys)
    FontProgram = 2    ///< The glyph is accessed in the font program
};

/** Flags to control font creation.
 */
enum class PdfFontAutoSelectBehavior
{
    None = 0,                   ///< No auto selection
    Standard14 = 1,             ///< Automatically select a Standard14 font if the fontname matches one of them
    Standard14Alt = 2,          ///< Automatically select a Standard14 font if the fontname matches one of them (standard and alternative names)
};

/** Font init flags
 */
enum class PdfFontCreateFlags
{
    None = 0,                 ///< No special settings
    DontEmbed = 1,            ///< Do not embed font data. Not embedding Standard14 fonts implies non CID
    DontSubset = 2,           ///< Don't subset font data (includes all the font glyphs)
    PreferNonCID = 4,         ///< Prefer non CID, simple fonts (/Type1, /TrueType)
};

enum class PdfFontMatchBehaviorFlags
{
    None,
    MatchExactName,           ///< Match exact font name
};

/**
 * Enum for the colorspaces supported
 * by PDF.
 */
enum class PdfColorSpace
{
    Unknown = 0,
    DeviceGray,
    DeviceRGB,
    DeviceCMYK,
    CalGray,
    CalRGB,
    Lab,            ///< CIE-Lab
    ICCBased,
    Indexed,
    Pattern,
    Separation,
    DeviceN
};

enum class PdfPixelFormat
{
    Unknown = 0,
    ARGB,
    BGRA,
    Gray,
    CMYK,
};

/**
 * Enum for text rendering mode (Tr)
 */
enum class PdfTextRenderingMode
{
    Unknown = 0,
    Fill,                      ///< Default mode, fill text
    Stroke,                    ///< Stroke text
    FillStroke,                ///< Fill, then stroke text
    Invisible,                 ///< Neither fill nor stroke text (invisible)
    FillAddToClipPath,         ///< Fill text and add to path for clipping
    StrokeAddToClipPath,       ///< Stroke text and add to path for clipping
    FillStrokeAddToClipPath,   ///< Fill, then stroke text and add to path for clipping
    AddToClipPath,             ///< Add text to path for clipping
};

/**
 * Enum for the different stroke styles that can be set
 * when drawing to a PDF file (mostly for line drawing).
 */
enum class PdfStrokeStyle
{
    Solid,
    Dash,
    Dot,
    DashDot,
    DashDotDot,
    Custom
};

/**
 * Enum to specifiy the initial information of the
 * info dictionary.
 */
enum class PdfInfoInitial
{
    None = 0,
    WriteCreationTime = 1,      ///< Write the creation time (current time). Default for new documents
    WriteModificationTime = 2,  ///< Write the modification time (current time). Default for loaded documents
    WriteProducer = 4,          ///< Write producer key. Default for new documents
};

/**
 * Enum for line cap styles when drawing.
 */
enum class PdfLineCapStyle
{
    Butt = 0,
    Round = 1,
    Square = 2
};

/**
 * Enum for line join styles when drawing.
 */
enum class PdfLineJoinStyle
{
    Miter = 0,
    Round = 1,
    Bevel = 2
};

/**
 * Enum for vertical text alignment
 */
enum class PdfVerticalAlignment
{
    Top = 0,
    Center = 1,
    Bottom = 2
};

/**
 * Enum for text alignment
 */
enum class PdfHorizontalAlignment
{
    Left = 0,
    Center = 1,
    Right = 2
};

enum class PdfSaveOptions
{
    None,
    // NOTE: Make room for some more options to come later
    NoModifyDateUpdate = 8,
    Clean = 16,
};

/**
 * Enum holding the supported page sizes by pdfmm.
 * Can be used to construct a PdfRect structure with
 * measurements of a page object.
 *
 * \see PdfPage
 */
enum class PdfPageSize
{
    Unknown = 0,
    A0,              ///< DIN A0
    A1,              ///< DIN A1
    A2,              ///< DIN A2
    A3,              ///< DIN A3
    A4,              ///< DIN A4
    A5,              ///< DIN A5
    A6,              ///< DIN A6
    Letter,          ///< Letter
    Legal,           ///< Legal
    Tabloid,         ///< Tabloid
};

/**
 * Enum holding the supported of types of "PageModes"
 * that define which (if any) of the "panels" are opened
 * in Acrobat when the document is opened.
 *
 * \see PdfDocument
 */
enum class PdfPageMode
{
    DontCare,
    UseNone,
    UseThumbs,
    UseBookmarks,
    FullScreen,
    UseOC,
    UseAttachments
};

/**
 * Enum holding the supported of types of "PageLayouts"
 * that define how Acrobat will display the pages in
 * relation to each other
 *
 * \see PdfDocument
 */
enum class PdfPageLayout
{
    Ignore,
    Default,
    SinglePage,
    OneColumn,
    TwoColumnLeft,
    TwoColumnRight,
    TwoPageLeft,
    TwoPageRight
};

enum class PdfStandard14FontType
{
    Unknown = 0,
    TimesRoman,
    TimesItalic,
    TimesBold,
    TimesBoldItalic,
    Helvetica,
    HelveticaOblique,
    HelveticaBold,
    HelveticaBoldOblique,
    Courier,
    CourierOblique,
    CourierBold,
    CourierBoldOblique,
    Symbol,
    ZapfDingbats,
};

/**
 * List of PDF stream content operators
 */
enum class PdfOperator
{
    Unknown = 0,
    // ISO 32008-1:2008 Table 51 – Operator Categories
    // General graphics state
    w,
    J,
    j,
    M,
    d,
    ri,
    i,
    gs,
    // Special graphics state
    q,
    Q,
    cm,
    // Path construction
    m,
    l,
    c,
    v,
    y,
    h,
    re,
    // Path painting
    S,
    s,
    f,
    F,
    f_Star,
    B,
    B_Star,
    b,
    b_Star,
    n,
    // Clipping paths
    W,
    W_Star,
    // Text objects
    BT,
    ET,
    // Text state
    Tc,
    Tw,
    Tz,
    TL,
    Tf,
    Tr,
    Ts,
    // Text positioning
    Td,
    TD,
    Tm,
    T_Star,
    // Text showing
    Tj,
    TJ,
    Quote,
    DoubleQuote,
    // Type 3 fonts
    d0,
    d1,
    // Color
    CS,
    cs,
    SC,
    SCN,
    sc,
    scn,
    G,
    g,
    RG,
    rg,
    K,
    k,
    // Shading patterns
    sh,
    // Inline images
    BI,
    ID,
    EI,
    // XObjects
    Do,
    // Marked content
    MP,
    DP,
    BMC,
    BDC,
    EMC,
    // Compatibility
    BX,
    EX,
};

/**
 * List of defined Rendering intents
 */
enum class PdfRenderingIntent
{
    AbsoluteColorimetric,
    RelativeColorimetric,
    Perceptual,
    Saturation,
};

/**
 * List of defined transparency blending modes
 */
enum class PdfBlendMode
{
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity,
};

};

ENABLE_BITMASK_OPERATORS(mm::PdfSaveOptions);
ENABLE_BITMASK_OPERATORS(mm::PdfWriteFlags);
ENABLE_BITMASK_OPERATORS(mm::PdfInfoInitial);
ENABLE_BITMASK_OPERATORS(mm::PdfFontStyle);
ENABLE_BITMASK_OPERATORS(mm::PdfFontCreateFlags);
ENABLE_BITMASK_OPERATORS(mm::PdfFontAutoSelectBehavior);
ENABLE_BITMASK_OPERATORS(mm::PdfFontDescriptorFlags);
ENABLE_BITMASK_OPERATORS(mm::PdfGlyphAccess);
ENABLE_BITMASK_OPERATORS(mm::PdfTextExtractFlags);

/**
 * \mainpage
 *
 * <b>pdfmm</b> is a library to work with the PDF file format and includes also a few
 * tools. The name comes from the first letter of PDF (Portable Document
 * Format).
 *
 * The <b>pdfmm</b> library is a free portable C++ library which includes
 * classes to parse a PDF file and modify its contents into memory. The changes
 * can be written back to disk easily. pdfmm does not currently provide any
 * rendering facility but the parser could be used to write a PDF viewer.
 * Besides parsing pdfmm includes also very simple classes to create your
 * own PDF files. All classes are documented so it is easy to start writing
 * your own application using pdfmm.
 *
 *
 * As of now <b>pdfmm</b> is available for Unix, Mac OS X and Windows platforms.
 *
 * More information can be found at: https://github.com/pdfmm/pdfmm
 *
 * <b>pdfmm</b> is maintained by Francesco Pretto <ceztko@gmail.com>,
 * and it's based on the work done by Dominik Seichter, Leonard Rosenthol,
 * Craig Ringer and others in the PoDoFo (http://podofo.sourceforge.net/)
 * library.
 *
 * \page Codingstyle (Codingstyle)
 * \verbinclude CODINGSTYLE.txt
 *
 */

#endif // PDF_DEFINES_H
