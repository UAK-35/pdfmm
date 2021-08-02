/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DESTINATION_H
#define PDF_DESTINATION_H

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfArray.h"
#include "podofo/base/PdfRect.h"
#include "podofo/base/PdfReference.h"

namespace PoDoFo {

class PdfAction;
class PdfPage;
class PdfRect;

enum class PdfDestinationFit
{
    Unknown = 0,
    Fit,
    FitH,
    FitV,
    FitB,
    FitBH,
    FitBV,
};

/** Destination type, as per 12.3.2.2 of the Pdf spec.
 *
 *  (see table 151 in the pdf spec)
 */
enum class PdfDestinationType
{
    Unknown = 0,
    XYZ,
    Fit,
    FitH,
    FitV,
    FitR,
    FitB,
    FitBH,
    FitBV,
};

/** A destination in a PDF file.
 *  A destination can either be a page or an action.
 *
 *  \see PdfOutlineItem \see PdfAnnotation \see PdfDocument
 */
// TODO: Make this a PdfElement
class PODOFO_DOC_API PdfDestination final
{
public:

    /** Create an empty destination - points to nowhere
     */
    PdfDestination(PdfDocument& doc);

    /** Create a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  \param pObject the object to construct from
     */
    PdfDestination(PdfObject& obj);

    /** Create a new PdfDestination with a page as destination
     *  \param pPage a page which is the destination
     *  \param eFit fit mode for the page. Must be EPdfDestinationFit::Fit or EPdfDestinationFit::FitB
     */
    PdfDestination(const PdfPage& page, PdfDestinationFit eFit = PdfDestinationFit::Fit);

    /** Create a destination to a page with its contents magnified to fit into the given rectangle
     *  \param pPage a page which is the destination
     *  \param rRect magnify the page so that the contents of the rectangle are visible
     */
    PdfDestination(const PdfPage& page, const PdfRect& rRect);

    /** Create a new destination to a page with specified left
     *  and top coordinates and a zoom factor.
     *  \param pPage a page which is the destination
     *  \param dLeft left coordinate
     *  \param dTop  top coordinate
     *  \param dZoom zoom factor in the viewer
     */
    PdfDestination(const PdfPage* pPage, double dLeft, double dTop, double dZoom);

    /** Create a new destination to a page.
     *  \param pPage a page which is the destination
     *  \param eFit fit mode for the Page. Allowed values are EPdfDestinationFit::FitH,
     *              EPdfDestinationFit::FitV, EPdfDestinationFit::FitBH, EPdfDestinationFit::FitBV
     *  \param dValue value which is a required argument for the selected fit mode
     */
    PdfDestination(const PdfPage* pPage, PdfDestinationFit eFit, double dValue);

    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     */
    PdfDestination(const PdfDestination& rhs);

    /** Copy an existing PdfDestination
     *  \param rhs copy this PdfDestination
     *  \returns this object
     */
    const PdfDestination& operator=(const PdfDestination& rhs);

    /** Get the page that this destination points to
     *  Requires that this PdfDestination was somehow
     *  created by or from a PdfDocument. Won't work otherwise.
     *  \param pDoc a PDF document owning this destination, needed to resolve pages
     *
     *  \returns the referenced PdfPage
     */
     // TODO: This is bullshit. Make PdfDestination a PdfElement and remove the parameter
    PdfPage* GetPage(PdfDocument* pDoc);

    /** Get the page that this destination points to
     *  Requires that this PdfDestination was somehow
     *  created by or from a PdfDocument. Won't work otherwise.
     *  \param pVecObjects a PdfVecObjects owning this destination, needed to resolve pages
     *
     *  \returns the referenced PdfPage
     */
     // TODO: This is bullshit. Make PdfDestination a PdfElement and remove the parameter
    PdfPage* GetPage(PdfVecObjects* pVecObjects);

    /** Get the destination fit type
     *
     *  \returns the fit type
     */
    PdfDestinationType GetType() const;

    /** Get the destination zoom
     *  Destination must be of type XYZ
     *  otherwise exception is thrown.
     *
     *  \returns the zoom
     */
    double GetZoom() const;

    /** Get the destination rect
     *  Destination must be of type FirR
     *  otherwise exception is thrown
     *
     *  \returns the destination rect
     */
    PdfRect GetRect() const;

    /** Get the destination Top position
     *  Destination must be of type XYZ, FitH, FitR, FitBH
     *  otherwise exception is thrown.
     *
     * \returns the Top position
     */
    double GetTop() const;

    /** Get the destination Left position
     *  Destination must be of type XYZ, FitV or FitR
     *  otherwise exception is thrown.
     *
     * \returns the Left position
     */
    double GetLeft() const;

    /** Get the destination Value
     *  Destination must be of type FitH, FitV
     *  or FitBH, otherwise exception is thrown
     *
     *  \returns the destination Value
     */
    double GetDValue() const;

    /** Get access to the internal object
     *
     *  \returns the internal PdfObject
     */
    inline PdfObject* GetObject() { return m_pObject; }

    /** Get access to the internal object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject* GetObject() const { return m_pObject; }

    /** Get access to the internal array
     *  \returns the internal PdfArray
     */
    inline PdfArray& GetArray() { return m_array; }

    /** Get access to the internal array
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfArray
     */
    inline const PdfArray& GetArray() const { return m_array; }


    /** Adds this destination to an dictionary.
     *  This method handles the all the complexities of making sure it's added correctly
     *
     *  If this destination is empty. Nothing will be added.
     *
     *  \param dictionary the destination will be added to this dictionary
     */
    void AddToDictionary(PdfDictionary& dictionary) const;

private:
    /** Initialize a new PdfDestination from an existing PdfObject (such as loaded from a doc)
     *  and a document.
     *
     *  \param pObject the object to construct from
     *  \param pDoc a PDF document owning this destination, needed to resolve pages
     */
    void Init(PdfObject& obj, PdfDocument& document);

private:
    PdfArray m_array;
    PdfObject* m_pObject;

    /** Create an empty destination - NOT ALLOWED
     */
    PdfDestination();

};

};

#endif // PDF_DESTINATION_H

