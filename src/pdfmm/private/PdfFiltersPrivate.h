/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FILTERS_PRIVATE_H
#define PDF_FILTERS_PRIVATE_H

/**
 * \file PdfFiltersPrivate.h
 *
 * Provides implementations of various PDF stream filters.
 *
 * This is an internal header. It should not be included in pdfmm.h, and
 * should not be included directly by client applications. These filters should
 * only be accessed through the factory interface in PdfFilters.h .
 */

#include <pdfmm/base/PdfFilter.h>

#include <zlib.h>

#ifdef PDFMM_HAVE_JPEG_LIB
extern "C" {
#ifdef _WIN32		// Collision between Win32 and libjpeg headers
#define XMD_H
#undef FAR

#ifndef HAVE_BOOLEAN
#define HAVE_BOOLEAN
#define PDFMM_JPEG_HAVE_BOOLEAN // not to be defined in the build system
#endif

#endif
#include <jpeglib.h>

#ifdef PDFMM_JPEG_HAVE_BOOLEAN
#undef HAVE_BOOLEAN
#endif
}
#endif // PDFMM_HAVE_JPEG_LIB

#ifdef PDFMM_HAVE_TIFF_LIB
extern "C" {
#include <tiffio.h>
#ifdef _WIN32		// Collision between tiff and jpeg-headers
#define XMD_H
#undef FAR
#endif
}
#endif // PDFMM_HAVE_TIFF_LIB


namespace mm {

#define PDFMM_FILTER_INTERNAL_BUFFER_SIZE 4096

class PdfPredictorDecoder;
class OutputStreamDevice;

/** The ascii hex filter.
 */
class PdfHexFilter final : public PdfFilter
{
public:
    PdfHexFilter();

    inline bool CanEncode() const override { return true; }

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCIIHexDecode; }

private:
    char m_DecodedByte;
    bool m_Low;
};

/** The Ascii85 filter.
 */
class PdfAscii85Filter final : public PdfFilter
{
public:
    PdfAscii85Filter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCII85Decode; }

private:
    void EncodeTuple(unsigned tuple, int bytes);
    void WidePut(unsigned tuple, int bytes) const;

private:
    int m_count;
    unsigned m_tuple;
};

/** The Flate filter.
 */
class PdfFlateFilter final : public PdfFilter
{
public:
    PdfFlateFilter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary* decodeParms) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::FlateDecode; }

private:
    void EncodeBlockInternal(const char* buffer, size_t len, int nMode);

private:
    unsigned char m_buffer[PDFMM_FILTER_INTERNAL_BUFFER_SIZE];

    z_stream m_stream;
    std::shared_ptr<PdfPredictorDecoder> m_Predictor;
};

/** The RLE filter.
 */
class PdfRLEFilter final : public PdfFilter
{
public:
    PdfRLEFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    inline PdfFilterType GetType() const override { return PdfFilterType::RunLengthDecode; }

private:
    int m_CodeLen;
};

/** The LZW filter.
 */
class PdfLZWFilter final : public PdfFilter
{
    struct TLzwItem
    {
        std::vector<unsigned char> value;
    };

    using TLzwTable = std::vector<TLzwItem>;
    using TILzwTable = TLzwTable::iterator;
    using TCILzwTable = TLzwTable::const_iterator;

public:
    PdfLZWFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::LZWDecode; }

private:
    void InitTable();

private:
    static const unsigned short s_masks[4];
    static const unsigned short s_clear;
    static const unsigned short s_eod;

    TLzwTable m_table;

    unsigned m_mask;
    unsigned m_code_len;
    unsigned char m_character;

    bool m_First;

    std::shared_ptr<PdfPredictorDecoder> m_Predictor;
};

#ifdef PDFMM_HAVE_JPEG_LIB

void PDFMM_API jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize);

extern "C"
{
    void JPegErrorExit(j_common_ptr cinfo);

    void JPegErrorOutput(j_common_ptr, int);
};

/** The DCT filter can decoded JPEG compressed data.
 *
 *  This filter requires JPEG lib to be available
 */
class PdfDCTFilter final : public PdfFilter
{
public:
    PdfDCTFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::DCTDecode; }

private:
    struct jpeg_decompress_struct m_cinfo;
    struct jpeg_error_mgr m_jerr;

    charbuff m_buffer;
    OutputStreamDevice* m_Device;
};

#endif // PDFMM_HAVE_JPEG_LIB

#ifdef PDFMM_HAVE_TIFF_LIB

/** The CCITT filter can decoded CCITTFaxDecode compressed data.
 *
 *  This filter requires TIFFlib to be available
 */
class PdfCCITTFilter final : public PdfFilter
{
public:
    PdfCCITTFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* buffer, size_t len) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* buffer, size_t len) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::CCITTFaxDecode; }

private:
    TIFF* m_tiff;
};

#endif // PDFMM_HAVE_TIFF_LIB

};


#endif // PDF_FILTERS_PRIVATE_H
