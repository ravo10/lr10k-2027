/**

    @file      ExcelWriter.h
    @brief     Writes an ExcelReport::Report to an .xlsx file.
    @details   No GUI code: AppMainExcel (ExcelExport.cpp) fills the report from a test tab and logs the result.

**/
#pragma once

#include "ExcelReport.h"

#include <string>

namespace ExcelWriter
{
    struct Result
    {
        bool ok = false;
        std::string fileName;  //!< With ".xlsx" added if it was missing
        std::string error;     //!< Why it failed (empty on success)
    };

    //! Write the report (sheets: Full Report, Setup, Graph N-mm and, where they apply, the stress graphs).
    Result Save(const ExcelReport::Report& report, const char* filename);
}
