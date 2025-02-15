/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <pdfmm/pdfmm.h>
#include "TestUtils.h"

using namespace std;
using namespace mm;

int main(int argc, char* argv[])
{
    // Add a fonts directory for more consistents run
    auto fontPath = TestUtils::GetTestInputPath() / "Fonts";
    if (!fs::exists(fontPath))
        throw runtime_error("Missing Fonts directory");

    PdfFontManager::AddFontDirectory(fontPath.u8string());
    PdfError::SetMaxLoggingSeverity(PdfLogSeverity::Warning);
    return Catch::Session().run(argc, argv);
}
