/**

    @file      ExcelWriter.cpp
    @brief     Writes an ExcelReport::Report to an .xlsx file (report, setup and graph sheets with charts).
    @details   Depends only on ExcelReport.h and libxlsxwriter (no GUI), so it can be built and checked on its
               own (tests/excel_writer_check.cpp writes sample files). The numbers come from ExcelReport.h.

**/

#include "ExcelWriter.h"

#include <xlsxwriter.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {
    enum GraphSheet { VAR_RAW, VAR_ENG_STRESS, VAR_TRUE_STRESS };
}

static void WriteGraphData(const ExcelReport::Report& report, lxw_workbook* workbook, std::vector<lxw_chart_data_label*>* activeLabels, lxw_worksheet* worksheet, GraphSheet dataVariation)
{
    // Create your custom labels
    static lxw_chart_font labels_font_green     = { .color = LXW_COLOR_GREEN };
    static lxw_chart_font labels_font_purple    = { .color = LXW_COLOR_PURPLE };
    static lxw_chart_font labels_font_blue      = { .color = LXW_COLOR_BLUE };
    static lxw_chart_font labels_font_orange    = { .color = LXW_COLOR_ORANGE };
    static lxw_chart_font labels_font_red       = { .color = LXW_COLOR_RED };
    static lxw_chart_font labels_font_default   = { .color = LXW_COLOR_BLACK };

    static lxw_chart_data_label label_prop_limit    = { .value = (char*)ExcelReport::PointShortName(0), .font = &labels_font_purple };
    static lxw_chart_data_label label_yield_str     = { .value = (char*)ExcelReport::PointShortName(1), .font = &labels_font_orange };
    static lxw_chart_data_label label_uts           = { .value = (char*)ExcelReport::PointShortName(2), .font = &labels_font_blue };
    static lxw_chart_data_label label_frac          = { .value = (char*)ExcelReport::PointShortName(3), .font = &labels_font_red };
    static lxw_chart_data_label label_other         = { .value = (char*)ExcelReport::PointShortName(4), .font = &labels_font_default };
    static lxw_chart_data_label label_e_lower       = { .value = (char*)ExcelReport::PointShortName(5), .font = &labels_font_green };
    static lxw_chart_data_label label_e_higher      = { .value = (char*)ExcelReport::PointShortName(6), .font = &labels_font_green };

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
        worksheet_write_string(worksheet, (lxw_row_t)(row + 1), colStartPoint, ExcelReport::PointName(point.id), NULL);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartPoint + 1, point.at.x, fmt_decimal3);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartPoint + 2, point.at.y, fmt_decimal3);

        // Label for the crucial point on the chart
        activeLabels->push_back(data_labels_options[point.id]);
    }
    activeLabels->push_back(NULL); // Add NULL terminator for the C-style array

    // Keep the header row visible when scrolling
    worksheet_freeze_panes(worksheet, 1, 0);
}

static void CreateGraphChart(const ExcelReport::Report& report, lxw_workbook* workbook, const char* sheet_name, lxw_worksheet* worksheet, GraphSheet dataVariation)
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

    // Only the labels of the points that are on this sheet, in the same order as the rows
    std::vector<lxw_chart_data_label*> activeLabels;

    /* Write the data for the chart. */
    WriteGraphData(report, workbook, &activeLabels, worksheet, dataVariation);
    const uint32_t curveRows = (uint32_t)report.data.size();
    const uint32_t pointRows = (uint32_t)(activeLabels.size() - 1); // without the NULL terminator

    // Set column width
    worksheet_set_column(worksheet, 0, 0, 22, NULL);
    worksheet_set_column(worksheet, 1, 2, 12, NULL);
    worksheet_set_column(worksheet, 3, 4, 11, NULL);

    // Date above the right side of the chart
    lxw_format* date_fmt = workbook_add_format(workbook);
    format_set_align(date_fmt, LXW_ALIGN_RIGHT);
    format_set_bold(date_fmt);
    format_set_font_color(date_fmt, LXW_COLOR_GRAY);
    worksheet_write_string(worksheet, CELL("U1"), ("Test Date: " + date).c_str(), date_fmt);

    // No data: a chart with an empty (reversed) range makes Excel report the file as damaged
    if (curveRows == 0)
    {
        worksheet_write_string(worksheet, CELL("G2"), "No data recorded for this test.", NULL);
        return;
    }

    // Create smooth scatter plot with lines
    lxw_chart* chart = workbook_add_chart(workbook, LXW_CHART_SCATTER_SMOOTH);

    // ----------------------------------------------------
    // --------------- Plot data
    // ----------------------------------------------------
    const char* seriesName = "Load";
    if (dataVariation == VAR_ENG_STRESS)
        seriesName = "Engineering stress";
    else if (dataVariation == VAR_TRUE_STRESS)
        seriesName = "True stress";

    lxw_chart_series* series = chart_add_series(chart, NULL, NULL);
    chart_series_set_categories(series, sheet_name, 1, 3, curveRows, 3); // Column D (X-values)
    chart_series_set_values(series, sheet_name, 1, 4, curveRows, 4);     // Column E (Y-values)
    chart_series_set_name(series, seriesName); // (was "Tensile stress" on every chart)
    chart_series_set_line(series, &line);

    // ----------------------------------------------------
    // --------------- Crucial Points
    // ----------------------------------------------------
    if (pointRows > 0)
    {
        lxw_chart_series* series2 = chart_add_series(chart, NULL, NULL);
        chart_series_set_categories(series2, sheet_name, 1, 1, pointRows, 1); // Column B (X-values), only the rows with points
        chart_series_set_values(series2, sheet_name, 1, 2, pointRows, 2);     // Column C (Y-values)
        chart_series_set_name(series2, "Crucial point");

        // Markers only, no connecting line
        chart_series_set_marker_type(series2, LXW_CHART_MARKER_CIRCLE);
        chart_series_set_marker_size(series2, marker_size);
        chart_series_set_marker_fill(series2, &marker_fill);
        chart_series_set_marker_line(series2, &marker_line);
        chart_series_set_line(series2, &line_none);

        // A label with the point's name above each point
        chart_series_set_labels(series2);
        lxw_chart_font global_font = { .bold = LXW_TRUE, .rotation = -30 };
        chart_series_set_labels_font(series2, &global_font);
        chart_series_set_labels_line(series2, &label_border);
        chart_series_set_labels_fill(series2, &label_fill);
        chart_series_set_labels_position(series2, LXW_CHART_LABEL_POSITION_ABOVE);
        chart_series_set_labels_custom(series2, activeLabels.data());
    }

    // ----------------------------------------------------
    // --------------- Chart
    // ----------------------------------------------------
    std::string diagramName = "DIRECT INPUT LOAD-EXTENSION (LLOYD LR 10K)\n" + date;
    if (dataVariation == VAR_ENG_STRESS)
        diagramName = "ENGINEERING STRESS - STRAIN (Material Test Results)\n" + date;
    else if (dataVariation == VAR_TRUE_STRESS)
        diagramName = "TRUE STRESS - STRAIN (Material Test Results)\n" + date;
    chart_title_set_name(chart, diagramName.c_str());
    chart_title_set_name_font(chart, &font_12);

    if (dataVariation == VAR_RAW)
    {
        chart_axis_set_name(chart->x_axis, ("Extension (" + report.units.length + ")").c_str());
        chart_axis_set_name(chart->y_axis, ("Load (" + report.units.force + ")").c_str());
    }
    else
    {
        chart_axis_set_name(chart->x_axis, dataVariation == VAR_TRUE_STRESS ? "True strain (%)" : "Strain (%)");
        chart_axis_set_name(chart->y_axis, ("Stress (" + report.units.stressAxis + ")").c_str());
    }

    chart_set_style(chart, 17);

    // Faint minor gridlines. Excel chooses the spacing from the data (it used to be fixed at 250 N / 5 MPa and
    // 0.1 on the x axis: a dense grid for small tests, almost none for large ones).
    lxw_chart_line minor_line_style = {
        .color = LXW_COLOR_SILVER,
        .width = 0.5,
        .dash_type = LXW_CHART_LINE_DASH_DOT
    };
    chart_axis_minor_gridlines_set_line(chart->y_axis, &minor_line_style);
    chart_axis_minor_gridlines_set_line(chart->x_axis, &minor_line_style);

    // One legend entry per series is enough at the bottom
    chart_legend_set_position(chart, LXW_CHART_LEGEND_BOTTOM);

    lxw_chart_options options = {
        .x_offset = 0,
        .y_offset = 0,
        .x_scale = 2,
        .y_scale = 1.6 // Tall enough for the 2-line title
    };
    worksheet_insert_chart_opt(worksheet, CELL("G2"), chart, &options);
}

ExcelWriter::Result ExcelWriter::Save(const ExcelReport::Report& report, const char* filename)
{
    Result result;
    std::string name = filename;
    if (name.find(".xlsx") == std::string::npos) name.append(".xlsx");
    result.fileName = name;

    // 1. Create Workbook
    lxw_workbook* workbook = workbook_new(name.c_str());
    if (!workbook) {
        result.error = "could not create the file (check the folder and its permissions)";
        return result;
    }

    // File properties (File > Info in Excel)
    const std::string subject = std::string(ExcelReport::TestKindText(report.kind)) + " test, " + report.date;
    const std::string comments = "Exported with LR 10K software " + report.appVersion;
    lxw_doc_properties properties = {};
    properties.title = "LR 10K material test report";
    properties.subject = subject.c_str();
    properties.comments = comments.c_str();
    workbook_set_properties(workbook, &properties);

    // 2. Formats
    lxw_format* fmt_header = workbook_add_format(workbook);
    format_set_bold(fmt_header);
    format_set_bg_color(fmt_header, LXW_COLOR_BLUE);
    format_set_font_color(fmt_header, LXW_COLOR_WHITE);
    format_set_align(fmt_header, LXW_ALIGN_CENTER);
    format_set_border(fmt_header, LXW_BORDER_THIN);

    lxw_format* fmt_section = workbook_add_format(workbook);
    format_set_bold(fmt_section);
    format_set_bottom(fmt_section, LXW_BORDER_THIN);

    lxw_format* fmt_table_header = workbook_add_format(workbook);
    format_set_bold(fmt_table_header);
    format_set_align(fmt_table_header, LXW_ALIGN_CENTER);
    format_set_text_wrap(fmt_table_header);
    format_set_bg_color(fmt_table_header, 0xDDEBF7);
    format_set_border(fmt_table_header, LXW_BORDER_THIN);

    lxw_format* fmt_unit = workbook_add_format(workbook);
    format_set_bold(fmt_unit);
    format_set_align(fmt_unit, LXW_ALIGN_LEFT);

    lxw_format* fmt_text = workbook_add_format(workbook);
    format_set_align(fmt_text, LXW_ALIGN_RIGHT);

    lxw_format* fmt_missing = workbook_add_format(workbook);
    format_set_align(fmt_missing, LXW_ALIGN_CENTER);
    format_set_font_color(fmt_missing, LXW_COLOR_GRAY);

    lxw_format* fmt_number[4] = {};
    const char* numberFormats[4] = { "0", "0.0", "0.00", "0.000" };
    for (int i = 0; i < 4; i++)
    {
        fmt_number[i] = workbook_add_format(workbook);
        format_set_num_format(fmt_number[i], numberFormats[i]);
    }

    lxw_format* fmt_description = workbook_add_format(workbook);
    format_set_align(fmt_description, LXW_ALIGN_VERTICAL_TOP);
    format_set_text_wrap(fmt_description);
    format_set_border(fmt_description, LXW_BORDER_MEDIUM);
    format_set_italic(fmt_description);

    // A label / value / unit line in columns A, B, C
    auto writeLine = [&](lxw_worksheet* ws, lxw_row_t row, const ExcelReport::Line& line) {
        worksheet_write_string(ws, row, 0, line.label.c_str(), NULL);
        if (line.isNumber)
            worksheet_write_number(ws, row, 1, line.number, fmt_number[(std::clamp)(line.decimals, 0, 3)]);
        else
            worksheet_write_string(ws, row, 1, line.text.c_str(), line.text == "-" ? fmt_missing : fmt_text);
        if (!line.unit.empty())
            worksheet_write_string(ws, row, 2, line.unit.c_str(), fmt_unit);
    };

    // ---------------------------------------------------------
    // A. REPORT SHEET (always: the test and specimen information is useful without dimensions too)
    // ---------------------------------------------------------
    lxw_worksheet* ws_report = workbook_add_worksheet(workbook, "Full Report");
    worksheet_set_column(ws_report, 0, 0, 34, NULL);
    worksheet_set_column(ws_report, 1, 5, 15, NULL);
    worksheet_merge_range(ws_report, 0, 0, 0, 5, "MATERIAL TESTING REPORT", fmt_header);

    lxw_row_t row = 2;
    for (const ExcelReport::Section& section : ExcelReport::ReportSections(report))
    {
        worksheet_merge_range(ws_report, row, 0, row, 2, section.title.c_str(), fmt_section);
        row++;
        for (const ExcelReport::Line& line : section.lines)
            writeLine(ws_report, row++, line);
        row++;
    }

    // Crucial points: engineering and true stress/strain side by side. Points that were not placed show "-"
    // (they used to show 0.000, which looked like a measured value).
    if (ExcelReport::HasStressSheets(report))
    {
        worksheet_merge_range(ws_report, row, 0, row, 2, "Crucial points", fmt_section);
        row++;
        const std::string stressHeader = "Stress (" + report.units.stress + ")";
        const std::string trueStressHeader = "True stress (" + report.units.stress + ")";
        worksheet_write_string(ws_report, row, 0, "Point", fmt_table_header);
        worksheet_write_string(ws_report, row, 1, stressHeader.c_str(), fmt_table_header);
        worksheet_write_string(ws_report, row, 2, "Strain (%)", fmt_table_header);
        worksheet_write_string(ws_report, row, 3, trueStressHeader.c_str(), fmt_table_header);
        worksheet_write_string(ws_report, row, 4, "True strain (%)", fmt_table_header);
        row++;
        for (const ExcelReport::PointResult& point : ExcelReport::PointResults(report))
        {
            worksheet_write_string(ws_report, row, 0, point.name.c_str(), NULL);
            if (point.placed)
            {
                worksheet_write_number(ws_report, row, 1, point.stress, fmt_number[3]);
                worksheet_write_number(ws_report, row, 2, point.strainPercent, fmt_number[3]);
                worksheet_write_number(ws_report, row, 3, point.trueStress, fmt_number[3]);
                worksheet_write_number(ws_report, row, 4, point.trueStrainPercent, fmt_number[3]);
            }
            else
            {
                for (lxw_col_t col = 1; col <= 4; col++)
                    worksheet_write_string(ws_report, row, col, "-", fmt_missing);
            }
            row++;
        }
        row++;
    }

    // Description
    worksheet_merge_range(ws_report, row, 0, row, 2, "Description", fmt_section);
    row++;
    worksheet_merge_range(ws_report, row, 0, row + 5, 5, report.description.c_str(), fmt_description);

    // ---------------------------------------------------------
    // B. SETUP SHEET
    // ---------------------------------------------------------
    lxw_worksheet* ws_setup = workbook_add_worksheet(workbook, "Setup");
    worksheet_set_column(ws_setup, 0, 0, 30, NULL);
    worksheet_set_column(ws_setup, 1, 1, 30, NULL);
    worksheet_set_column(ws_setup, 2, 2, 10, NULL);
    worksheet_merge_range(ws_setup, 0, 0, 0, 2, "LR 10K SETTINGS", fmt_header);
    row = 2;
    for (const ExcelReport::Line& line : ExcelReport::SetupLines(report))
        writeLine(ws_setup, row++, line);

    // ---------------------------------------------------------
    // C. GRAPHS (data next to each chart)
    // ---------------------------------------------------------
    const char* sheetNameGraphNmm   = "Graph N-mm";
    lxw_worksheet* worksheetGraphNmm = workbook_add_worksheet(workbook, sheetNameGraphNmm);
    CreateGraphChart(report, workbook, sheetNameGraphNmm, worksheetGraphNmm, VAR_RAW);

    // Stress-strain sheets only where they mean something (tension/compression with dimensions). These sheets
    // used to be created empty without dimensions, and filled with tension formulas for 3-point bending.
    if (ExcelReport::HasStressSheets(report))
    {
        const char* sheetNameEngStress  = "Graph Eng. Stress";
        const char* sheetNameTrueStress = "Graph True Stress";
        lxw_worksheet* worksheetEngStress  = workbook_add_worksheet(workbook, sheetNameEngStress);
        lxw_worksheet* worksheetTrueStress = workbook_add_worksheet(workbook, sheetNameTrueStress);
        CreateGraphChart(report, workbook, sheetNameEngStress, worksheetEngStress, VAR_ENG_STRESS);
        CreateGraphChart(report, workbook, sheetNameTrueStress, worksheetTrueStress, VAR_TRUE_STRESS);
    }

    // ---------------------------------------------------------
    // D. CLOSE & SAVE
    // ---------------------------------------------------------
    const lxw_error err = workbook_close(workbook);
    result.ok = (err == LXW_NO_ERROR);
    if (!result.ok)
        result.error = lxw_strerror(err);
    return result;
}
