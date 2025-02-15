/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DEFINES_PRIVATE_H
#define PDF_DEFINES_PRIVATE_H

#ifndef BUILDING_PDFMM
#error PdfDeclarationsPrivate.h is only available for use in the core pdfmm src/ build .cpp files
#endif

#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cinttypes>
#include <climits>

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <iostream>

#include "Format.h"
#include "numbers_compat.h"

 // Macro to define friendship with test classes.
 // Must be defined before base declarations
#define PDFMM_UNIT_TEST(classname) friend class classname

#include <pdfmm/base/PdfDeclarations.h>

#ifdef _WIN32
// Microsft itself assumes little endian
// https://github.com/microsoft/STL/blob/b11945b73fc1139d3cf1115907717813930cedbf/stl/inc/bit#L336
#define PDFMM_IS_LITTLE_ENDIAN
#else // Unix

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) || !defined(__ORDER_BIG_ENDIAN__)
#error "Byte order macros are not defined"
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PDFMM_IS_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PDFMM_IS_BIG_ENDIAN
#else
#error "__BYTE_ORDER__ macro has not"
#endif

#endif // _WIN32

#ifdef PDFMM_IS_LITTLE_ENDIAN
#define AS_BIG_ENDIAN(n) utls::ByteSwap(n)
#define FROM_BIG_ENDIAN(n) utls::ByteSwap(n)
#else // PDFMM_IS_BIG_ENDIAN
#define AS_BIG_ENDIAN(n) n
#define FROM_BIG_ENDIAN(n) n
#endif

/** \def VERBOSE_DEBUG_DISABLED
 *  Debug define. Enable it, if you need
 *  more debug output to the command line from pdfmm
 *
 *  Setting VERBOSE_DEBUG_DISABLED will make pdfmm
 *  EXTREMELY slow and verbose, so it's not practical
 *  even for regular debugging.
 */
#define VERBOSE_DEBUG_DISABLED

// Should we do lots of extra (expensive) sanity checking?  You should not
// define this on production builds because of the runtime cost and because it
// might cause the library to abort() if it notices something nasty.
// It may also change the size of some objects, and is thus not binary
// compatible.
//
// If you don't know you need this, avoid it.
//
#define EXTRA_CHECKS_DISABLED

#ifdef DEBUG
#include <cassert>
#define PDFMM_ASSERT(x) assert(x);
#else
#define PDFMM_ASSERT(x)
#endif // DEBUG

// This is a do nothing macro that can be used to define
// an invariant property without actually checking for it,
// not even in DEBUG build. It's user responsability to
// ensure it's actually satisfied
#define PDFMM_INVARIANT(x)

#define CMAP_REGISTRY_NAME "pdfmm"

/** \def PDFMM_RAISE_ERROR(code)
 *
 *  Throw an exception of type PdfError with the error code x, which should be
 *  one of the values of the enum PdfErrorCode. File and line info are included.
 */
#define PDFMM_RAISE_ERROR(code) throw ::mm::PdfError(code, __FILE__, __LINE__)

 /** \def PDFMM_RAISE_ERROR_INFO(code, msg)
  *
  *  Throw an exception of type PdfError with the error code, which should be
  *  one of the values of the enum PdfErrorCode. File and line info are included.
  *  Additionally extra information on the error, msg is set, which will also be
  *  output by PdfError::PrintErrorMsg().
  *  msg can be a C string, but can also be a C++ std::string.
  */
#define PDFMM_RAISE_ERROR_INFO(code, msg, ...) throw ::mm::PdfError(code, __FILE__, __LINE__, COMMON_FORMAT(msg, ##__VA_ARGS__))

  /** \def PDFMM_PUSH_FRAME(err, msg)
   *
   * Add frame to error callastack
   */
#define PDFMM_PUSH_FRAME(err) err.AddToCallstack(__FILE__, __LINE__)

   /** \def PDFMM_PUSH_FRAME_INFO(err, msg)
    *
    * Add frame to error callastack with msg information
    */
#define PDFMM_PUSH_FRAME_INFO(err, msg, ...) err.AddToCallstack(__FILE__, __LINE__, COMMON_FORMAT(msg, ##__VA_ARGS__))

    /** \def PDFMM_PUSH_FRAME(err, msg)
     *
     *  Evaluate `cond' as a binary predicate and if it is true, raise a logic error with the
     *  info string `msg' .
     */
#define PDFMM_RAISE_LOGIC_IF(cond, msg, ...) {\
    if (cond)\
        throw ::mm::PdfError(PdfErrorCode::InternalLogic, __FILE__, __LINE__, COMMON_FORMAT(msg, ##__VA_ARGS__));\
};

namespace mm
{
    class OutputStream;
    class InputStream;

    PdfVersion GetPdfVersion(const std::string_view& str);

    std::string_view GetPdfVersionName(PdfVersion version);

    constexpr double DEG2RAD = std::numbers::pi / 180;
    constexpr double RAD2DEG = 180 / std::numbers::pi;

    std::vector<std::string> ToPdfKeywordsList(const std::string_view& str);
    std::string ToPdfKeywordsString(const cspan<std::string>&keywords);

    /** Log a message to the logging system defined for pdfmm.
     *  \param logSeverity the severity of the log message
     *  \param msg       the message to be logged
     */
    void LogMessage(PdfLogSeverity logSeverity, const std::string_view& msg);

    template <typename... Args>
    void LogMessage(PdfLogSeverity logSeverity, const std::string_view& msg, const Args&... args)
    {
        LogMessage(logSeverity, COMMON_FORMAT(msg, args...));
    }
}

/**
 * \namespace utls
 *
 * Namespace for private utilities and common functions
 */
namespace utls
{
    const std::locale& GetInvariantLocale();

    bool IsStringEmptyOrWhiteSpace(const std::string_view& str);

    std::string TrimSpacesEnd(const std::string_view& str);

    /** Convert an enum or index to its string representation
     *  which can be written to the PDF file.
     *
     *  This is a helper function for various classes
     *  that need strings and enums for their SubTypes keys.
     *
     *  \param index the index or enum value
     *  \param types an array of strings containing
     *         the string mapping of the index
     *  \param len the length of the string array
     *
     *  \returns the string representation or nullptr for
     *           values out of range
     */
    const char* TypeNameForIndex(unsigned index, const char** types, unsigned len);

    /** Convert a string type to an array index or enum.
     *
     *  This is a helper function for various classes
     *  that need strings and enums for their SubTypes keys.
     *
     *  \param type the type as string
     *  \param types an array of strings containing
     *         the string mapping of the index
     *  \param len the length of the string array
     *  \param unknownValue the value that is returned when the type is unknown
     *
     *  \returns the index of the string in the array
     */
    int TypeNameToIndex(const char* type, const char** types, unsigned len, int unknownValue);

    bool TryGetHexValue(char ch, unsigned char& value);

    // Write the char to the supplied buffer as hexadecimal code
    void WriteCharHexTo(char buf[2], char ch);

    // Append the char to the supplied string as hexadecimal code
    void WriteCharHexTo(std::string& str, char ch);

    // Append the unicode code point to a big endian encoded utf16 string
    void WriteUtf16BETo(std::u16string& str, char32_t codePoint);

    void ReadUtf16BEString(const mm::bufferview& buffer, std::string& utf8str);

    void ReadUtf16LEString(const mm::bufferview& buffer, std::string& utf8str);

    void FormatTo(std::string& str, float value, unsigned short precision);

    void FormatTo(std::string& str, double value, unsigned short precision);

    std::string ToLower(const std::string_view& str);

    std::string Trim(const std::string_view& str, char ch);

    // https://stackoverflow.com/a/38140932/213871
    inline void hash_combine(std::size_t& seed)
    {
        (void)seed;
    }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9E3779B9 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
    }

    // Returns log(ch) / log(256) + 1
    unsigned char GetCharCodeSize(unsigned code);

    // Returns pow(2, size * 8) - 1
    unsigned GetCharCodeMaxValue(unsigned char codeSize);

    template<typename T>
    void move(T& in, T& out)
    {
        out = in;
        in = { };
    }

#ifdef _WIN32
    std::string GetWin32ErrorMessage(unsigned rc);
#endif // _WIN32

#pragma region IO

    // Helper classes to access some std::istream,
    // std::ostream members polymorfically
    template <typename TStream>
    struct stream_helper;

    template <>
    struct stream_helper<std::istream>
    {
        static std::streampos tell(std::istream& stream)
        {
            return stream.tellg();
        }

        static std::istream& seek(std::istream& stream, std::streampos pos)
        {
            return stream.seekg(pos);
        }

        static std::istream& seek(std::istream& stream, std::streamoff off, std::ios_base::seekdir dir)
        {
            return stream.seekg(off, dir);
        }
    };

    template <>
    struct stream_helper<std::ostream>
    {
        static std::streampos tell(std::ostream& stream)
        {
            return stream.tellp();
        }

        static std::ostream& seek(std::ostream& stream, std::streampos pos)
        {
            return stream.seekp(pos);
        }

        static std::ostream& seek(std::ostream& stream, std::streamoff off, std::ios_base::seekdir dir)
        {
            return stream.seekp(off, dir);
        }
    };

    size_t FileSize(const std::string_view& filename);
    /**
     * \returns number or read bytes
     * \param eof true if the stream reached EOF during read
     */
    size_t ReadBuffer(std::istream& stream, char* buffer, size_t size, bool& eof);
    /**
     * \returns true if success, false if eof
     */
    bool ReadChar(std::istream& stream, char& ch);

    std::ifstream open_ifstream(const std::string_view& filename, std::ios_base::openmode mode);
    std::ofstream open_ofstream(const std::string_view& filename, std::ios_base::openmode mode);
    std::fstream open_fstream(const std::string_view& filename, std::ios_base::openmode mode);

    // NOTE: Never use this function unless you really need a C FILE descriptor,
    // as in PdfImage.cpp . For all the other I/O, use an STL stream
    FILE* fopen(const std::string_view& view, const std::string_view& mode);

    void WriteUInt32BE(mm::OutputStream& output, uint32_t value);
    void WriteInt32BE(mm::OutputStream& output, int32_t value);
    void WriteUInt16BE(mm::OutputStream& output, uint16_t value);
    void WriteInt16BE(mm::OutputStream& output, int16_t value);
    void WriteUInt32BE(char* buf, uint32_t value);
    void WriteInt32BE(char* buf, int32_t value);
    void WriteUInt16BE(char* buf, uint16_t value);
    void WriteInt16BE(char* buf, int16_t value);
    void ReadUInt32BE(mm::InputStream& input, uint32_t& value);
    void ReadInt32BE(mm::InputStream& input, int32_t& value);
    void ReadUInt16BE(mm::InputStream& input, uint16_t& value);
    void ReadInt16BE(mm::InputStream& input, int16_t& value);
    void ReadUInt32BE(const char* buf, uint32_t& value);
    void ReadInt32BE(const char* buf, int32_t& value);
    void ReadUInt16BE(const char* buf, uint16_t& value);
    void ReadInt16BE(const char* buf, int16_t& value);

#pragma endregion // IO

#pragma region Byte Swap

    void ByteSwap(std::u16string& str);

#ifdef _MSC_VER
    inline uint16_t ByteSwap(uint16_t n)
    {
        return _byteswap_ushort(n);
    }

    inline uint32_t ByteSwap(uint32_t n)
    {
        return _byteswap_ulong(n);
    }

    inline uint64_t ByteSwap(uint64_t n)
    {
        return _byteswap_uint64(n);
    }

    inline int16_t ByteSwap(int16_t n)
    {
        return (int16_t)_byteswap_ushort((uint16_t)n);
    }

    inline int32_t ByteSwap(int32_t n)
    {
        return (int32_t)_byteswap_ulong((uint32_t)n);
    }

    inline int64_t ByteSwap(int64_t n)
    {
        return (int64_t)_byteswap_uint64((uint64_t)n);
    }
#else
    inline uint16_t ByteSwap(uint16_t n)
    {
        return __builtin_bswap16(n);
    }

    inline uint32_t ByteSwap(uint32_t n)
    {
        return __builtin_bswap32(n);
    }

    inline uint64_t ByteSwap(uint64_t n)
    {
        return __builtin_bswap64(n);
    }

    inline int16_t ByteSwap(int16_t n)
    {
        return (int16_t)__builtin_bswap16((uint16_t)n);
    }

    inline int32_t ByteSwap(int32_t n)
    {
        return (int32_t)__builtin_bswap32((uint32_t)n);
    }

    inline int64_t ByteSwap(int64_t n)
    {
        return (int64_t)__builtin_bswap64((uint64_t)n);
    }
#endif
#pragma endregion // Byte Swap

}

/**
 * \page <pdfmm PdfDefinesPrivate Header>
 *
 * <b>PdfDeclarationsPrivate.h</b> contains preprocessor definitions, inline functions, templates,
 * compile-time const variables, and other things that must be visible across the entirety of
 * the pdfmm library code base but should not be visible to users of the library's headers.
 *
 * This header is private to the library build. It is not installed with pdfmm and must not be
 * referenced in any way from any public, installed header.
 */

#endif // PDF_DEFINES_PRIVATE_H
