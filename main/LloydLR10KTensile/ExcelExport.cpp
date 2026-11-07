/**

    @file      ExcelExport.cpp
    @brief     Excel (.xlsx) export of a test tab.
    @details   MakeExcelReport() fills an ExcelReport::Report (plain data) from the tab; ExcelWriter.cpp writes it.
               Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"
#include "ExcelReport.h"
#include "ExcelWriter.h"

#include <ctime>

#include "LR10KController.h"

ExcelReport::Report AppMainExcel::MakeExcelReport(const TabControl::TabTestDataClass& tabData)
{
    ExcelReport::Report report;
    report.date = tabData.date;
    report.description = tabData.description;

    // Units of the test itself (normally the same as the app's current units)
    const int unitIndex = tabData.unitsConvertionIndex;
    report.units.si = unitIndex == 0;
    report.units.length = LR10KAppGlobals::GetUnitOption<const char*>(0, unitIndex);
    report.units.force = LR10KAppGlobals::GetUnitOption<const char*>(1, unitIndex);
    report.units.speed = LR10KAppGlobals::GetUnitOption<const char*>(2, unitIndex);
    report.units.stressAxis = LR10KAppGlobals::GetUnitOption<const char*>(6, unitIndex);
    report.units.stress = LR10KAppGlobals::GetUnitOption<const char*>(7, unitIndex);
    report.units.modulus = report.units.si ? "GPa" : report.units.stress;

    report.appVersion = LR10KAppVersion;
    {
        const std::time_t now = std::time(nullptr);
        char text[32] = "";
        if (std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", std::localtime(&now)) > 0)
            report.exportedAt = text;
    }

    report.tension = tabData.tension;
    report.kind = tabData.tension ? ExcelReport::TestKind::Tension
        : (tabData.compressionOptionPicked == 1 ? ExcelReport::TestKind::Bending3Point : ExcelReport::TestKind::Compression);
    report.bending = { tabData.calcData_Compression[0], tabData.calcData_Compression[1], tabData.calcData_Compression[2], tabData.calcData_Compression[3] };
    report.specimen.crossSection = tabData.areaOptionPicked;
    report.specimen.thickness = tabData.thickness;
    report.specimen.width = tabData.width;
    report.specimen.length = tabData.length;
    report.specimen.diameter = tabData.diameter;
    report.storedArea = tabData.area;
    report.zeroOffset = tabData.LR10OffsetZeroPointExtensionAxis; // The test's own (was the global value of the active tab)

    report.youngsModulus = tabData.calcYoungsModulus;
    const double* nominal[TestCalc::TensileResults::kPoints] = { tabData.calcPoint_NominalStressStrain_PropLimit, tabData.calcPoint_NominalStressStrain_yield,
        tabData.calcPoint_NominalStressStrainUTS, tabData.calcPoint_NominalStressStrain_fracture };
    for (int i = 0; i < TestCalc::TensileResults::kPoints; i++)
        report.nominal[i] = { nominal[i][0], nominal[i][1] };

    report.crossheadSpeed = tabData.settings_speedAndPlot[0];
    report.jogSpeed = tabData.settings_speedAndPlot[1];
    report.loadErrorPercent = tabData.settings_speedAndPlot[2];
    report.samplingRateHz = tabData.LR10KTensileData.SamplingRate;

    report.data.reserve(tabData.LR10KTensileData.Data.size());
    for (int i = 0; i < tabData.LR10KTensileData.Data.Size; i++)
        report.data.push_back({ (double)tabData.LR10KTensileData.Data[i].x, (double)tabData.LR10KTensileData.Data[i].y });
    for (const auto& point_tuple : tabData.plot_dataPoints)
    {
        const ImPlotPoint& p = std::get<0>(point_tuple);
        report.points.push_back({ std::get<1>(point_tuple), { p.x, p.y } });
    }
    return report;
}

bool AppMainExcel::SavePlotDataAsAnXlsxFile(const TabControl::TabTestDataClass& tabData, const char* filename)
{
    return SaveReportAsXlsxFile(MakeExcelReport(tabData), filename);
}

bool AppMainExcel::SaveReportAsXlsxFile(const ExcelReport::Report& report, const char* filename)
{
    const ExcelWriter::Result result = ExcelWriter::Save(report, filename);
    if (result.ok)
        DockingApps::console.AddLog("Successfully saved Excel report: %s", result.fileName.c_str());
    else
        DockingApps::console.AddLog("[error] Could not save the Excel report %s: %s. Is the file open in Excel?", result.fileName.c_str(), result.error.c_str());
    return result.ok;
}
