/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PUSH_BUTTON_H
#define PDF_PUSH_BUTTON_H

#include "PdfButton.h"

namespace PoDoFo
{
    /** A push button is a button which has no state and value
     *  but can toggle actions.
     */
    class PODOFO_DOC_API PdfPushButton : public PdfButton
    {
        friend class PdfField;
    private:
        PdfPushButton(PdfObject& obj, PdfAnnotation* widget);

    public:
        PdfPushButton(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdfPushButton(PdfPage& page, const PdfRect& rect);

        /** Set the rollover caption of this button
         *  which is displayed when the cursor enters the field
         *  without the mouse button being pressed
         *
         *  \param rsText the caption
         */
        void SetRolloverCaption(const PdfString& rsText);

        /**
         *  \returns the rollover caption of this button
         */
        std::optional<PdfString> GetRolloverCaption() const;

        /** Set the alternate caption of this button
         *  which is displayed when the button is pressed.
         *
         *  \param rsText the caption
         */
        void SetAlternateCaption(const PdfString& rsText);

        /**
         *  \returns the rollover caption of this button
         */
        std::optional<PdfString> GetAlternateCaption() const;

    private:
        void Init();
    };
}

#endif // PDF_PUSH_BUTTON_H
