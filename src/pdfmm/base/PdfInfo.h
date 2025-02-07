/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INFO_H
#define PDF_INFO_H

#include "PdfName.h"
#include "PdfDate.h"
#include "PdfElement.h"

namespace mm
{

class PdfString;

/** This class provides access to the documents
 *  info dictionary, which provides information
 *  about the PDF document.
 */
class PDFMM_API PdfInfo final : public PdfDictionaryElement
{
public:
    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param obj must be an info dictionary
     */
    PdfInfo(PdfObject& obj);

    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param obj must be an info dictionary.
     *  \param initial which information should be
     *         writting initially to the information
     */
    PdfInfo(PdfObject& obj, PdfInfoInitial initial);

    /** Set the title of the document.
     *  \param title title
     */
    void SetTitle(nullable<const PdfString&> title);

    /** Get the title of the document
     *  \returns the title
     */
    nullable<PdfString> GetTitle() const;

    /** Set the author of the document.
     *  \param author author
     */
    void SetAuthor(nullable<const PdfString&> author);

    /** Get the author of the document
     *  \returns the author
     */
    nullable<PdfString> GetAuthor() const;

    /** Set the subject of the document.
     *  \param subject subject
     */
    void SetSubject(nullable<const PdfString&> subject);

    /** Get the subject of the document
     *  \returns the subject
     */
    nullable<PdfString> GetSubject() const;

    /** Set keywords for this document
     *  \param keywords a list of keywords
     */
    void SetKeywords(nullable<const PdfString&> keywords);

    /** Get the keywords of the document
     *  \returns the keywords
     */
    nullable<PdfString> GetKeywords() const;

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param creator creator
     */
    void SetCreator(nullable<const PdfString&> creator);

    /** Get the creator of the document
     *  \returns the creator
     */
    nullable<PdfString> GetCreator() const;

    /** Set the producer of the document.
     *  \param producer producer
     */
    void SetProducer(nullable<const PdfString&> producer);

    /** Get the producer of the document
     *  \returns the producer
     */
    nullable<PdfString> GetProducer() const;

    void SetCreationDate(nullable<PdfDate> date);

    /** Get creation date of document
     *  \return creation date
     */
    nullable<PdfDate> GetCreationDate() const;

    void SetModDate(nullable<PdfDate> date);

    /** Get modification date of document
     *  \return modification date
     */
    nullable<PdfDate> GetModDate() const;

    /** Set the trapping state of the document.
     *  \param trapped trapped
     */
    void SetTrapped(const PdfName& trapped);

    /** Get the trapping state of the document
     *  \returns the title
     */
    const PdfName& GetTrapped() const;

private:
    /** Add the initial document information to the dictionary.
     *  \param initial which information should be
     *         writting initially to the information
     */
    void init(PdfInfoInitial initial);

    /** Get a value from the info dictionary as name
     *  \para name the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    nullable<PdfString> getStringFromInfoDict(const PdfName& name) const;

    /** Get a value from the info dictionary as name
    *  \para name the key to fetch from the info dictionary
    *  \return a value from the info dictionary
    */
    const PdfName& getNameFromInfoDict(const PdfName& name) const;
};

};

#endif // PDF_INFO_H
