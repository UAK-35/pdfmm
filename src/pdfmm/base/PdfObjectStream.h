/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OBJECT_STREAM_H
#define PDF_OBJECT_STREAM_H

#include "PdfDeclarations.h"

#include "PdfFilter.h"
#include "PdfEncrypt.h"

namespace mm {

class InputStream;
class PdfName;
class PdfObject;
class OutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  You have to use a concrete implementation of a stream,
 *  which can be retrieved from a StreamFactory.
 *  \see PdfIndirectObjectList
 *  \see PdfMemoryObjectStream
 *  \see PdfFileObjectStream
 */
class PDFMM_API PdfObjectStream
{
    friend class PdfParserObject;
    friend class PdfObjectInputStream;

public:
    /** The default filter to use when changing the stream content.
     *  It's a static member and applies to all newly created/changed streams.
     *  The default value is PdfFilterType::FlateDecode.
     */
    static enum PdfFilterType DefaultFilter;

protected:
    /** Create a new PdfObjectStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param parent parent object
     */
    PdfObjectStream(PdfObject& parent);

public:
    virtual ~PdfObjectStream();

    /** Write the stream to an output device
     *  \param device write to this outputdevice.
     *  \param encrypt encrypt stream data using this object
     */
    virtual void Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt) = 0;

    /** Set a binary buffer as stream data.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param buffer buffer containing the stream data
     *  \param filters a list of filters to use when appending data
     */
    void Set(const bufferview& buffer, const PdfFilterList& filters);

    /** Set a binary buffer as stream data.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param buffer buffer containing the stream data
     *  \param len length of the buffer
     *  \param filters a list of filters to use when appending data
     */
    void Set(const char* buffer, size_t len, const PdfFilterList& filters);

    /** Set a binary buffer as stream data.
     *  All data will be Flate-encoded.
     *
     *  \param buffer buffer containing the stream data
     */
    void Set(const bufferview& buffer);

    /** Set a binary buffer as stream data.
     *  All data will be Flate-encoded.
     *
     *  \param buffer buffer containing the stream data
     *  \param len length of the buffer
     */
    void Set(const char* buffer, size_t len);

    /** Set a binary buffer whose contents are read from an InputStream
     *  All data will be Flate-encoded.
     *
     *  \param stream read stream contents from this InputStream
     */
    void Set(InputStream& stream);

    /** Set a binary buffer whose contents are read from an InputStream
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param stream read stream contents from this InputStream
     *  \param filters a list of filters to use when appending data
     */
    void Set(InputStream& stream, const PdfFilterList& filters);

    /** Sets raw data for this stream which is read from an input stream.
     *  This method does neither encode nor decode the read data.
     *  The filters of the object are not modified and the data is expected
     *  encoded as stated by the /Filters key in the stream's object.
     *
     *  \param stream read data from this input stream
     *  \param len    read exactly len bytes from the input stream,
     *                 if len = -1 read until the end of the input stream
     *                 was reached.
     */
    void SetRawData(InputStream& stream, ssize_t len = -1);

    /** Start appending data to this stream.
     *
     *  This method has to be called before any of the append methods.
     *  All appended data will be Flate-encoded.
     *
     *  \param clearExisting if true any existing stream contents will
     *         be cleared.
     *
     *  \see Append
     *  \see EndAppend
     *  \see eDefaultFilter
     */
    void BeginAppend(bool clearExisting = true);

    /** Start appending data to this stream.
     *  This method has to be called before any of the append methods.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param filters a list of filters to use when appending data
     *  \param clearExisting if true any existing stream contents will
               be cleared.
     *  \param deleteFilters if true existing filter keys are deleted if an
     *         empty list of filters is passed (required for SetRawData())
     *
     *  \see Append
     *  \see EndAppend
     */
    void BeginAppend(const PdfFilterList& filters, bool clearExisting = true, bool deleteFilters = true);

    /** Append a binary buffer to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param view a buffer
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    PdfObjectStream& Append(const std::string_view& view);

    /** Append a binary buffer to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param buffer a buffer
     *  \param len size of the buffer
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    PdfObjectStream& AppendBuffer(const char* buffer, size_t size);
    PdfObjectStream& AppendBuffer(const bufferview& buffer);

    /** Finish appending data to this stream.
     *  BeginAppend() has to be called before this method.
     *
     *  \see BeginAppend
     *  \see Append
     */
    void EndAppend();

    /** Get the stream's length with all filters applied (e.g. if the stream is
     * Flate-compressed, the length of the compressed data stream).
     *
     *  \returns the length of the internal buffer
     */
    virtual size_t GetLength() const = 0;

    /** Get a copy of a the stream and write it to an OutputStream
     *
     *  \param stream data is written to this stream.
     */
    virtual void CopyTo(OutputStream& stream) const = 0;

    /** Get a buffer of the current stream which has been
     *  filtered by all filters as specified in the dictionary's
     *  /Filter key. For example, if the stream is Flate-compressed,
     *  the buffer returned from this method will have been decompressed.
     *
     *  The caller has to the buffer.
     *
     *  \param buffer pointer to the buffer
     */
    void ExtractTo(charbuff& buffer) const;

    /** Get a filtered copy of a the stream and write it to an OutputStream
     *
     *  \param stream filtered data is written to this stream.
     */
    void ExtractTo(OutputStream& stream) const;

    charbuff GetFilteredCopy() const;

    void MoveTo(PdfObject& obj);

    /** Create a copy of a PdfObjectStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    PdfObjectStream& operator=(const PdfObjectStream& rhs);

protected:
    /** Begin appending data to this stream.
     *  Clears the current stream contents.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param filters use these filters to encode any data written
     *         to the stream.
     */
    virtual void BeginAppendImpl(const PdfFilterList& filters) = 0;

    /** Append a binary buffer to the current stream contents.
     *
     *  \param data a buffer
     *  \param len length of the buffer
     *
     *  \see BeginAppend
     *  \see Append
     *  \see EndAppend
     */
    virtual void AppendImpl(const char* data, size_t len) = 0;

    /** Finish appending data to the stream
     */
    virtual void EndAppendImpl() = 0;

    virtual void CopyFrom(const PdfObjectStream& rhs);

    void EnsureAppendClosed();

    PdfObject& GetParent() { return *m_Parent; }

    virtual std::unique_ptr<InputStream> GetInputStream() const = 0;

private:
    PdfObjectStream(const PdfObjectStream& rhs) = delete;

    void endAppend();

    void SetRawData(InputStream& stream, ssize_t len, bool markObjectDirty);

    void BeginAppend(const PdfFilterList& filters, bool clearExisting, bool deleteFilters, bool markObjectDirty);

private:
    PdfObject* m_Parent;
    bool m_Append;
};

};

#endif // PDF_OBJECT_STREAM_H
