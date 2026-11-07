/**

    @file      ExcelExport.cpp
    @brief     Excel (.xlsx) export of test data, crucial points and report.
    @details   Writes an ExcelReport::Report (plain data, see ExcelReport.h); MakeExcelReport() fills one from a test tab.
               Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"
#include "ExcelReport.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// ------------------------------- Excel related -------------------------------
// ----------------------------------------------------------------------------- σ
void create_style_gallery() {
    lxw_workbook* workbook = workbook_new("chart_style_gallery.xlsx");
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Styles");
    lxw_worksheet* data_ws = workbook_add_worksheet(workbook, "Data");

    // 1. Write some dummy data for the style charts to use
    for (int i = 0; i < 5; i++) {
        worksheet_write_number(data_ws, i, 0, (i + 1) * 10, NULL);
    }

    // 2. Loop through all 48 styles
    int chart_count = 1;
    for (int row = 0; row < 96; row += 16) {      // Jump rows for grid
        for (int col = 0; col < 64; col += 8) {  // Jump columns for grid
            if (chart_count > 48) break;

            // Create a small chart
            lxw_chart* chart = workbook_add_chart(workbook, LXW_CHART_LINE);

            // Add data from our dummy sheet
            chart_add_series(chart, NULL, "=Data!$A$1:$A$5");

            // Set the style and a title so you know which is which
            char title[20];
            sprintf(title, "Style %d", chart_count);
            chart_title_set_name(chart, title);
            chart_legend_set_position(chart, LXW_CHART_LEGEND_NONE);

            // Apply the style!
            chart_set_style(chart, chart_count);

            // Insert into the gallery sheet
            worksheet_insert_chart(worksheet, row, col, chart);

            chart_count++;
        }
    }

    workbook_close(workbook);
}

void AppMainExcel::write_worksheet_data(const ExcelReport::Report& report, lxw_workbook* workbook, std::vector<lxw_chart_data_label*>* activeLabels, lxw_worksheet* worksheet, AppMainExcel::ExcelDataVariationWorksheetData dataVariation)
{
    // Create your custom labels
    static lxw_chart_font labels_font_green     = { .color = LXW_COLOR_GREEN };
    static lxw_chart_font labels_font_purple    = { .color = LXW_COLOR_PURPLE };
    static lxw_chart_font labels_font_blue      = { .color = LXW_COLOR_BLUE };
    static lxw_chart_font labels_font_orange    = { .color = LXW_COLOR_ORANGE };
    static lxw_chart_font labels_font_red       = { .color = LXW_COLOR_RED };
    static lxw_chart_font labels_font_default   = { .color = LXW_COLOR_BLACK };

    static lxw_chart_data_label label_prop_limit    = { .value = (char*)TabControl::plot_items_short[0], .font = &labels_font_purple };
    static lxw_chart_data_label label_yield_str     = { .value = (char*)TabControl::plot_items_short[1], .font = &labels_font_orange };
    static lxw_chart_data_label label_uts           = { .value = (char*)TabControl::plot_items_short[2], .font = &labels_font_blue };
    static lxw_chart_data_label label_frac          = { .value = (char*)TabControl::plot_items_short[3], .font = &labels_font_red };
    static lxw_chart_data_label label_other         = { .value = (char*)TabControl::plot_items_short[4], .font = &labels_font_default };
    static lxw_chart_data_label label_e_lower       = { .value = (char*)TabControl::plot_items_short[5], .font = &labels_font_green };
    static lxw_chart_data_label label_e_higher      = { .value = (char*)TabControl::plot_items_short[6], .font = &labels_font_green };

    // Create the array of label pointer options (All possible labels for all crucial points, in the order of the IDs)
    static lxw_chart_data_label* data_labels_options[] = {
        &label_prop_limit,  &label_yield_str,
        &label_uts,         &label_frac,
        &label_other,       &label_e_lower,
        &label_e_higher
    };

    /* Add a bold format to use to highlight the header cells. */
    lxw_format* fmt_header = workbook_add_format(workbook);
    format_set_bold(fmt_header);
    format_set_align(fmt_header, LXW_ALIGN_CENTER);

    lxw_format* fmt_decimal3 = workbook_add_format(workbook);
    format_set_num_format(fmt_decimal3, "0.000");
    format_set_align(fmt_decimal3, LXW_ALIGN_CENTER);

    const ExcelReport::Units& units = report.units;
    std::string label = "";
    if (dataVariation == VAR_RAW)
    {
        // Crucial points
        worksheet_write_string(worksheet, CELL("A1"), "P ID", fmt_header);
        label = "P X (" + units.length + ")";
        worksheet_write_string(worksheet, CELL("B1"), label.c_str(), fmt_header);
        label = "P Y (" + units.force + ")";
        worksheet_write_string(worksheet, CELL("C1"), label.c_str(), fmt_header);

        // Load-extension data
        label = "X (" + units.length + ")";
        worksheet_write_string(worksheet, CELL("D1"), label.c_str(), fmt_header);
        label = "Y (" + units.force + ")";
        worksheet_write_string(worksheet, CELL("E1"), label.c_str(), fmt_header);
    }
    else
    {
        // Crucial points (strain in percent: the header used to say just "ε")
        worksheet_write_string(worksheet, CELL("A1"), "P ID", fmt_header);
        label = "P X (ε, %)";
        worksheet_write_string(worksheet, CELL("B1"), label.c_str(), fmt_header);
        label = "P Y (σ, " + units.stress + ")";
        worksheet_write_string(worksheet, CELL("C1"), label.c_str(), fmt_header);

        // Stress-strain data
        label = "X (ε, %)";
        worksheet_write_string(worksheet, CELL("D1"), label.c_str(), fmt_header);
        label = "Y (σ, " + units.stress + ")";
        worksheet_write_string(worksheet, CELL("E1"), label.c_str(), fmt_header);
    }

    // The numbers (zero offset, stress/strain conversion, which crucial points) come from ExcelReport::BuildSheet,
    // tested in tests/excel_report_test.cpp
    const ExcelReport::Sheet sheet = dataVariation == VAR_RAW ? ExcelReport::Sheet::LoadExtension
        : dataVariation == VAR_ENG_STRESS ? ExcelReport::Sheet::EngineeringStress : ExcelReport::Sheet::TrueStress;
    const ExcelReport::SheetRows rows = ExcelReport::BuildSheet(report, sheet);

    const int colStartData = 3;
    for (size_t row = 0; row < rows.curve.size(); row++)
    {
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartData, rows.curve[row].x, fmt_decimal3);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartData + 1, rows.curve[row].y, fmt_decimal3);
    }

    const int colStartPoint = 0;
    for (size_t row = 0; row < rows.points.size(); row++)
    {
        const ExcelReport::PointRow& point = rows.points[row];
        worksheet_write_string(worksheet, (lxw_row_t)(row + 1), colStartPoint, TabControl::plot_items[point.id], NULL);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartPoint + 1, point.at.x, fmt_decimal3);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartPoint + 2, point.at.y, fmt_decimal3);

        // Label for the crucial point on the chart
        activeLabels->push_back(data_labels_options[point.id]);
    }
    activeLabels->push_back(NULL); // Add NULL terminator for the C-style array
}

void AppMainExcel::CreateGraphChart(const ExcelReport::Report& report, lxw_workbook* workbook, const char* sheet_name, lxw_worksheet* worksheet, AppMainExcel::ExcelDataVariationWorksheetData dataVariation)
{
    const std::string& date = report.date;
    // ----------------------------------------------------
    // --------------- Chart styles and setup
    // ----------------------------------------------------
    lxw_chart_font font_12 = { .size = 12, .bold = LXW_TRUE };
    lxw_chart_line line_none = { .none = 1 };
    lxw_chart_line line = { .width = 2 };

    // Fill and Line for the Points
    uint8_t marker_size = 9;
    lxw_chart_fill marker_fill = { .color = LXW_COLOR_ORANGE };
    lxw_chart_line marker_line = { .color = LXW_COLOR_GRAY };

    // Label
    lxw_chart_line label_border = { .color = LXW_COLOR_SILVER, .width = 1.0 };
    lxw_chart_fill label_fill = { .color = LXW_COLOR_WHITE };

    // For current chart, we only want to show the labels that are active (the ones that have a point on the graph). So we create an array of pointers to only those labels.
    std::vector<lxw_chart_data_label*> activeLabels;

    /* Write some data for the chart. */
    write_worksheet_data(report, workbook, &activeLabels, worksheet, dataVariation);

    // Set column width
    worksheet_set_column(worksheet, 0, 0, 7, NULL);
    worksheet_set_column(worksheet, 1, 1, 10, NULL);
    worksheet_set_column(worksheet, 2, 2, 12, NULL);
    worksheet_set_column(worksheet, 3, 3, 7, NULL);
    worksheet_set_column(worksheet, 4, 4, 10, NULL);

    // Create smooth scatter plot with lines
    lxw_chart* chart = workbook_add_chart(workbook, LXW_CHART_SCATTER_SMOOTH);

    // ----------------------------------------------------
    // --------------- Plot data
    // ----------------------------------------------------

    /* Add the first series to the chart (plot data). */
    lxw_chart_series* series = chart_add_series(chart, NULL, NULL);
    chart_series_set_categories(series, sheet_name, CELL("D2"), (uint32_t)report.data.size() + 1, 3); // Column D (X-values) CELL("D2") = 1,3
    chart_series_set_values(series, sheet_name, CELL("E2"), (uint32_t)report.data.size() + 1, 4); // Column E (Y-values) CELL("E2") = 1,4

    /* Set the name for the series instead of the default "Series 1". */
    chart_series_set_name(series, "Tensile stress");

    chart_series_set_line(series, &line);

    // ----------------------------------------------------
    // --------------- Crucial Points
    // ----------------------------------------------------

    if (dataVariation == VAR_RAW || dataVariation == VAR_ENG_STRESS)
    {
        /* Add the second series to the chart (crucial points). */
        lxw_chart_series* series2 = chart_add_series(chart, NULL, NULL);
        chart_series_set_categories(series2, sheet_name, CELL("B2"), (uint32_t)report.data.size() + 1, 1); // Column B (X-values) CELL("B2") = 1,1
        chart_series_set_values(series2, sheet_name, CELL("C2"), (uint32_t)report.data.size() + 1, 2); // Column C (Y-values) CELL("C2") = 1,2

        /* Set the name for the series instead of the default "Series 2". */
        chart_series_set_name(series2, "Crucial point");

        // Force series2 to show ONLY markers and NO line
        chart_series_set_marker_type(series2, LXW_CHART_MARKER_CIRCLE);
        chart_series_set_marker_size(series2, marker_size);
        chart_series_set_marker_fill(series2, &marker_fill);
        chart_series_set_marker_line(series2, &marker_line);
        chart_series_set_line(series2, &line_none); // This removes the connecting line

        // 1. Enable labels for the Crucial Points series (Turns them "ON")
        chart_series_set_labels(series2);
        // Rotation 270 is often better for vertical text (reads bottom-to-top)
        lxw_chart_font global_font = { .bold = LXW_TRUE, .rotation = -30 };
        chart_series_set_labels_font(series2, &global_font);

        // 2. Set the formatting for the label "bubble"
        chart_series_set_labels_line(series2, &label_border);
        chart_series_set_labels_fill(series2, &label_fill);

        // 3. Position the labels
        chart_series_set_labels_position(series2, LXW_CHART_LABEL_POSITION_ABOVE);

        // 4. Apply the custom labels! (This overwrites the numbers with your text)
        // For current chart, we only want to show the labels that are active (the ones that have a point on the graph). So we create an array of pointers to only those labels.
        chart_series_set_labels_custom(series2, activeLabels.data());
    }

    // ----------------------------------------------------
    // --------------- Chart
    // ----------------------------------------------------

    // Create a format for the header date
    lxw_format* date_fmt = workbook_add_format(workbook);
    format_set_align(date_fmt, LXW_ALIGN_RIGHT);
    format_set_bold(date_fmt);
    format_set_font_color(date_fmt, LXW_COLOR_GRAY);

    // Write it in cell Q1 or R1 (visually above the right side of the chart)
    char alt_date[100];
    sprintf(alt_date, "Test Date: %s", date.c_str());
    worksheet_write_string(worksheet, CELL("U1"), alt_date, date_fmt); // U1

    std::string diagramName = "DIRECT INPUT LOAD-EXTENSION (LLOYD LR 10K)\n" + date;
    if (dataVariation == AppMainExcel::VAR_ENG_STRESS)
        diagramName = "ENGINEERING STRESS - STRAIN (Material Test Results)\n" + date;
    else if (dataVariation == AppMainExcel::VAR_TRUE_STRESS)
        diagramName = "TRUE STRESS - STRAIN (Material Test Results)\n" + date;
    chart_title_set_name(chart, diagramName.c_str());
    chart_title_set_name_font(chart, &font_12);

    // ----------------------------------------------------

    /* Add a chart title and some axis labels. */
    chart_title_set_name(chart, diagramName.c_str());
    if (dataVariation == AppMainExcel::VAR_RAW)
    {
        chart_axis_set_name(chart->x_axis, ("Extension (" + report.units.length + ")").c_str());
        chart_axis_set_name(chart->y_axis, ("Load (" + report.units.force + ")").c_str());
    }
    else
    {
        chart_axis_set_name(chart->x_axis, "Strain (Epsilon %)");
        chart_axis_set_name(chart->y_axis, ("Stress (" + report.units.stressAxis + ")").c_str());
    }

    /* Set an Excel chart style. */
    chart_set_style(chart, 17);

    // 1. Define the spacing for the minor grid
    if (dataVariation == AppMainExcel::VAR_RAW)
    {
        chart_axis_set_minor_unit(chart->y_axis, 250);
        chart_axis_set_minor_unit(chart->x_axis, 0.1);
    }
    else
    {
        chart_axis_set_minor_unit(chart->y_axis, 5);
        chart_axis_set_minor_unit(chart->x_axis, 0.1);
    }
    // 3. Make the minor lines very faint so they don't distract
    lxw_chart_line minor_line_style = {
        .color = LXW_COLOR_SILVER,  // Light gray
        .width = 0.5,               // Thinner than major lines
        .dash_type = LXW_CHART_LINE_DASH_DOT // Optional: make them dashed
    };
    // This function "turns on" the minor gridlines using the style above
    chart_axis_minor_gridlines_set_line(chart->y_axis, &minor_line_style);
    chart_axis_minor_gridlines_set_line(chart->x_axis, &minor_line_style);

    // Configure the Insert Options
    lxw_chart_options options = {
        .x_offset = 0,
        .y_offset = 0,
        .x_scale = 2, // Trial and error: 2.15 usually hits Column U perfectly
        .y_scale = 1.6   // Make it tall enough for your 3-line title
    };

    worksheet_insert_chart_opt(worksheet, CELL("G2"), chart, &options);
}

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

    report.tension = tabData.tension;
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
    std::string name = filename;
    if (name.find(".xlsx") == std::string::npos) name.append(".xlsx");

    // 1. Create Workbook
    lxw_workbook* workbook = workbook_new(name.c_str());
    if (!workbook) {
        DockingApps::console.AddLog("[error] Failed to create workbook. Check permissions.");
        return false;
    }

    // 2. Create Global Formats
    lxw_format* fmt_header = workbook_add_format(workbook);
    format_set_bold(fmt_header);
    format_set_bg_color(fmt_header, LXW_COLOR_BLUE);
    format_set_font_color(fmt_header, LXW_COLOR_WHITE); // Makes the text readable
    format_set_align(fmt_header, LXW_ALIGN_CENTER);
    format_set_border(fmt_header, LXW_BORDER_THIN);

    // ---------------------------------------------------------
    // A. FULL REPORT SHEET
    // ---------------------------------------------------------
    // (Used to require thickness and width, so circular specimens got no report or stress sheets)
    const bool dimensionsAdded = ExcelReport::HasStressSheets(report);

    const std::string unit_raw = report.units.length;     //SI: mm
    const std::string unit_stress = report.units.stress;  //SI: N/mm^2
    const std::string unit_strain = "% (ɛ)";
    const std::string unit_speed = report.units.speed;    //SI: mm/min
    std::string label = ""; // Container

    // Formats
    lxw_format* fmt_bold_left = workbook_add_format(workbook);
    format_set_bold(fmt_bold_left);
    format_set_align(fmt_bold_left, LXW_ALIGN_LEFT);

    lxw_format* fmt_bold_right = workbook_add_format(workbook);
    format_set_bold(fmt_bold_right);
    format_set_align(fmt_bold_right, LXW_ALIGN_RIGHT);

    lxw_format* fmt_upper_right = workbook_add_format(workbook);
    format_set_align(fmt_upper_right, LXW_ALIGN_VERTICAL_TOP);
    format_set_border(fmt_upper_right, LXW_BORDER_MEDIUM);
    format_set_italic(fmt_upper_right);

    lxw_format* fmt_decimal3 = workbook_add_format(workbook);
    format_set_num_format(fmt_decimal3, "0.000");

    lxw_format* fmt_decimal2 = workbook_add_format(workbook);
    format_set_num_format(fmt_decimal2, "0.00");

    lxw_format* fmt_decimal0 = workbook_add_format(workbook);
    format_set_num_format(fmt_decimal0, "0");

    if (dimensionsAdded) {
        lxw_worksheet* ws_report = workbook_add_worksheet(workbook, "Full Report");
        worksheet_set_column(ws_report, 0, 0, 32, NULL);
        worksheet_set_column(ws_report, 1, 2, 13, NULL);
        worksheet_set_column(ws_report, 3, 3, 32, NULL);
        worksheet_set_column(ws_report, 4, 5, 13, NULL);

        worksheet_write_string(ws_report, CELL("A1"), "MATERIAL TESTING REPORT", fmt_header);
        worksheet_merge_range(ws_report, CELL("A1"), CELL("F1"), "MATERIAL TESTING REPORT", fmt_header);
        worksheet_write_string(ws_report, 1, 0, std::string("Test date: " + report.date).c_str(), fmt_bold_left);

        // Properties
        std::string dim = report.units.si ? "SI" : "English Engineering";
        dim = "Dimensions (" + dim + ")";
        worksheet_write_string(ws_report, CELL("A4"), dim.c_str(), fmt_bold_left);

        

        if (report.specimen.crossSection == MaterialCalc::Rectangular)
        {
            label = "Gauge Thickness (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A5"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B5"), report.specimen.thickness, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C5"), unit_raw.c_str(), fmt_bold_left);

            label = "Gauge Width (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A6"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B6"), report.specimen.width, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C6"), unit_raw.c_str(), fmt_bold_left);
        }
        else if (report.specimen.crossSection == MaterialCalc::Circular)
        {
            label = "-";
            worksheet_write_string(ws_report, CELL("A5"), label.c_str(), NULL);
            worksheet_write_string(ws_report, CELL("B5"), "-", NULL);
            worksheet_write_string(ws_report, CELL("C5"), "-", NULL);

            label = "Gauge Diameter (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A6"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B6"), report.specimen.diameter, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C6"), unit_raw.c_str(), fmt_bold_left);
        }

        label = "Gauge Length (L0) (" + unit_raw + ")";
        worksheet_write_string(ws_report, CELL("A7"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B7"), report.specimen.length, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C7"), unit_raw.c_str(), fmt_bold_left);

        label = "Cross-sectional area (" + unit_raw + "^2)";
        std::string unit = unit_raw + "^2";
        worksheet_write_string(ws_report, CELL("A8"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B8"), ExcelReport::AreaForExport(report), fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C8"), unit.c_str(), fmt_bold_left);

        // (Add other calculations like E-Modulus here if needed) σ
        int strain = 0;
        int stress = 1;

        // Stress
        worksheet_write_string(ws_report, CELL("A10"), "Engineering stress (nominal stress)", fmt_bold_left);

        // GPa with SI units, psi with imperial units (same as in the app)
        const std::string unit_modulus = report.units.modulus;
        label = "Young's Modulus, E-Modulus (" + unit_modulus + ")";
        worksheet_write_string(ws_report, CELL("A11"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B11"), report.youngsModulus, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C11"), unit_modulus.c_str(), fmt_bold_left);

        label = "Proportional Limit (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A12"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B12"), report.nominal[0].stress, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C12"), unit_stress.c_str(), fmt_bold_left);

        label = "Yield Strength (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A13"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B13"), report.nominal[1].stress, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C13"), unit_stress.c_str(), fmt_bold_left);

        label = "Ultimate Tensile Stress (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A14"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B14"), report.nominal[2].stress, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C14"), unit_stress.c_str(), fmt_bold_left);

        label = "Fracture (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A15"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B15"), report.nominal[3].stress, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C15"), unit_stress.c_str(), fmt_bold_left);

        // Strain
        worksheet_write_string(ws_report, CELL("D10"), "Engineering strain (nominal strain)", fmt_bold_left);

        label = "Proportional Limit (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D12"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E12"), report.nominal[0].strain*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F12"), unit_strain.c_str(), fmt_bold_left);

        label = "Yield Strength (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D13"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E13"), report.nominal[1].strain*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F13"), unit_strain.c_str(), fmt_bold_left);

        label = "Ultimate Tensile Stress (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D14"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E14"), report.nominal[2].strain*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F14"), unit_strain.c_str(), fmt_bold_left);

        label = "Fracture (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D15"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E15"), report.nominal[3].strain*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F15"), unit_strain.c_str(), fmt_bold_left);

        // ------------
        // Description
        // ------------
        worksheet_write_string(ws_report, CELL("A17"), "Description", fmt_bold_left);
        worksheet_merge_range(ws_report, CELL("A17"), CELL("C17"), "Description", fmt_bold_left);

        worksheet_write_string(ws_report, CELL("A18"), report.description.c_str(), fmt_upper_right);
        worksheet_merge_range(ws_report, CELL("A18"), CELL("C23"), report.description.c_str(), fmt_upper_right);
    }

    // ---------------------------------------------------------
    // B. SETUP SHEET
    // ---------------------------------------------------------
    lxw_worksheet* ws_setup = workbook_add_worksheet(workbook, "Setup");
    worksheet_set_column(ws_setup, 0, 3, 27, NULL); //27px
    worksheet_write_string(ws_setup, CELL("A1"), "LR 10K SETTINGS", fmt_header);
    worksheet_merge_range(ws_setup, CELL("A1"), CELL("C1"), "LR 10K SETTINGS", fmt_header);

    // (Write your setup values here as before)
    label = "Crosshead speed (" + unit_speed + ")";
    worksheet_write_string(ws_setup, CELL("A3"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B3"), report.crossheadSpeed, fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C3"), unit_speed.c_str(), fmt_bold_left);

    label = "Jog speed (" + unit_speed + ")";
    worksheet_write_string(ws_setup, CELL("A4"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B4"), report.jogSpeed, fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C4"), unit_speed.c_str(), fmt_bold_left);

    label = "Load Error (%)";
    worksheet_write_string(ws_setup, CELL("A5"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B5"), report.loadErrorPercent, fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C5"), "%", fmt_bold_left);

    label = "Test Type (Tension or Compr.)";
    worksheet_write_string(ws_setup, CELL("A6"), label.c_str(), NULL);
    worksheet_write_string(ws_setup, CELL("B6"), report.tension ? "Tension Testing" : "Compression Testing", fmt_bold_right);

    // Some old saves won't have this data, and is set to 0 after loading it in (before 2026)
    label = "Sampling Rate (Hz)";
    worksheet_write_string(ws_setup, CELL("A7"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B7"), report.samplingRateHz, fmt_decimal0);
    worksheet_write_string(ws_setup, CELL("C7"), "Hz", fmt_bold_left);

    // ---------------------------------------------------------
    // C. GRAPH 1: LOAD - EXTENSION (The "Raw" Graph)
    // ---------------------------------------------------------
    // For this one, we stick to the existing pattern:
    // Create a sheet -> Write the data -> Add the chart.

    // Worksheet names
    const char* sheetNameGraphNmm   = "Graph N-mm";
    const char* sheetNameEngStress  = "Graph Eng. Stress";  
    const char* sheetNameTrueStress = "Graph True Stress";

    // Add the worksheets
    lxw_worksheet* worksheetGraphNmm    = workbook_add_worksheet(workbook, sheetNameGraphNmm);
    lxw_worksheet* worksheetEngStress   = workbook_add_worksheet(workbook, sheetNameEngStress);
    lxw_worksheet* worksheetTrueStress  = workbook_add_worksheet(workbook, sheetNameTrueStress);

    // Call the Load-Extension Chart
    CreateGraphChart(report, workbook, sheetNameGraphNmm, worksheetGraphNmm, AppMainExcel::VAR_RAW);


    // ---------------------------------------------------------
    // D. GRAPH 2: STRESS - STRAIN (The "New" Graphs)
    // ---------------------------------------------------------
    if (dimensionsAdded)
    {
        // We do NOT create worksheets here. 
        // The 'AddStressLineChart' function now creates its own sheet ("Stress-Strain Plot") 
        // and calculates its own data internally.

        // 1. Engineering Stress (Nominal)
        // Pass NULL for the worksheet because the function creates a new one.
        CreateGraphChart(report, workbook, sheetNameEngStress, worksheetEngStress, AppMainExcel::VAR_ENG_STRESS);

        // 2. True Stress (Optional)
        // Only call this if you want a SECOND sheet for True Stress
        CreateGraphChart(report, workbook, sheetNameTrueStress, worksheetTrueStress, AppMainExcel::VAR_TRUE_STRESS);
    }

    // ---------------------------------------------------------
    // E. CLOSE & SAVE
    // ---------------------------------------------------------
    lxw_error err = workbook_close(workbook);

    if (err == LXW_NO_ERROR) {
        DockingApps::console.AddLog("Successfully saved Excel report: %s", name.c_str());
        return true;
    }
    else {
        DockingApps::console.AddLog("[error] Save failed (Error %d). Is the file open in Excel?", err);
        return false;
    }
}
