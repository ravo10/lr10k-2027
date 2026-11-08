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

    // Colours (a calm, neutral look: dark text, one blue accent, light grey lines)
    constexpr lxw_color_t kInk        = 0x111827; //!< Titles and values
    constexpr lxw_color_t kText       = 0x374151; //!< Labels
    constexpr lxw_color_t kMuted      = 0x6B7280; //!< Units, notes, axis numbers
    constexpr lxw_color_t kRule       = 0xE5E7EB; //!< Hairlines between rows, gridlines
    constexpr lxw_color_t kCard       = 0xF3F4F6; //!< Key result cards, description box
    constexpr lxw_color_t kAccent     = 0x2563EB; //!< The curve, section rules
    constexpr lxw_color_t kHeaderFill = 0x1F2937; //!< Table headers
    constexpr lxw_color_t kWhite      = 0xFFFFFF;

    //! Chart marker/label colour per crucial point ID (0..6), close to the app's colours.
    constexpr lxw_color_t kPointColour[7] = {
        0x7C3AED, // Proportional limit: violet
        0xD97706, // Yield: amber
        0x0891B2, // UTS: teal
        0xDC2626, // Fracture: red
        0x4B5563, // Other: grey
        0x16A34A, // E-lower: green
        0x16A34A, // E-upper: green
    };

    //! "N/mm^2" -> "N/mm²" for what is shown (the numbers and the checks use the plain units).
    std::string Pretty(std::string text)
    {
        for (std::string::size_type at; (at = text.find("^2")) != std::string::npos;) text.replace(at, 2, "\xC2\xB2");
        for (std::string::size_type at; (at = text.find("^3")) != std::string::npos;) text.replace(at, 2, "\xC2\xB3");
        return text;
    }

    //! The formats used on every sheet (one set per workbook).
    struct Formats
    {
        lxw_format* title = nullptr;      //!< Sheet title, large
        lxw_format* subtitle = nullptr;   //!< Test type and date under the title
        lxw_format* version = nullptr;    //!< "LR 10K software ..." at the top right
        lxw_format* section = nullptr;    //!< Section title with an accent rule under it
        lxw_format* label = nullptr;      //!< A label / value / unit line
        lxw_format* textRight = nullptr;
        lxw_format* textLeft = nullptr;
        lxw_format* unit = nullptr;
        lxw_format* missing = nullptr;    //!< "-" (not placed / not calculated)
        lxw_format* number[4] = {};       //!< 0 .. 3 decimals
        lxw_format* tableHeader = nullptr;
        lxw_format* tableText = nullptr;
        lxw_format* tableNumber = nullptr; //!< 0.000
        lxw_format* tableMissing = nullptr;
        lxw_format* cardLabel = nullptr;
        lxw_format* cardMissing = nullptr;
        lxw_format* description = nullptr;
        lxw_format* footnote = nullptr;
        lxw_format* note = nullptr;
    };

    lxw_format* NewFormat(lxw_workbook* wb, double size = 10.0, lxw_color_t colour = kText)
    {
        lxw_format* f = workbook_add_format(wb);
        format_set_font_name(f, "Calibri");
        format_set_font_size(f, size);
        format_set_font_color(f, colour);
        format_set_align(f, LXW_ALIGN_VERTICAL_CENTER);
        return f;
    }

    void Hairline(lxw_format* f)
    {
        format_set_bottom(f, LXW_BORDER_HAIR);
        format_set_bottom_color(f, kRule);
    }

    Formats MakeFormats(lxw_workbook* wb)
    {
        Formats f;
        f.title = NewFormat(wb, 20.0, kInk);
        format_set_bold(f.title);
        f.subtitle = NewFormat(wb, 11.0, kMuted);
        f.version = NewFormat(wb, 9.0, kMuted);
        format_set_align(f.version, LXW_ALIGN_RIGHT);

        f.section = NewFormat(wb, 12.0, kInk);
        format_set_bold(f.section);
        format_set_bottom(f.section, LXW_BORDER_MEDIUM);
        format_set_bottom_color(f.section, kAccent);

        f.label = NewFormat(wb);
        Hairline(f.label);
        f.textRight = NewFormat(wb, 10.0, kInk);
        format_set_align(f.textRight, LXW_ALIGN_RIGHT);
        Hairline(f.textRight);
        f.textLeft = NewFormat(wb, 10.0, kInk);
        Hairline(f.textLeft);
        f.unit = NewFormat(wb, 10.0, kMuted);
        Hairline(f.unit);
        f.missing = NewFormat(wb, 10.0, kMuted);
        format_set_align(f.missing, LXW_ALIGN_RIGHT);
        Hairline(f.missing);
        const char* numberFormats[4] = { "0", "0.0", "0.00", "0.000" };
        for (int i = 0; i < 4; i++)
        {
            f.number[i] = NewFormat(wb, 10.0, kInk);
            format_set_num_format(f.number[i], numberFormats[i]);
            Hairline(f.number[i]);
        }

        f.tableHeader = NewFormat(wb, 10.0, kWhite);
        format_set_bold(f.tableHeader);
        format_set_bg_color(f.tableHeader, kHeaderFill);
        format_set_text_wrap(f.tableHeader);
        format_set_align(f.tableHeader, LXW_ALIGN_CENTER);
        f.tableText = NewFormat(wb);
        Hairline(f.tableText);
        f.tableNumber = NewFormat(wb, 10.0, kInk);
        format_set_num_format(f.tableNumber, "0.000");
        Hairline(f.tableNumber);
        f.tableMissing = NewFormat(wb, 10.0, kMuted);
        format_set_align(f.tableMissing, LXW_ALIGN_CENTER);
        Hairline(f.tableMissing);

        f.cardLabel = NewFormat(wb, 9.0, kMuted);
        format_set_bg_color(f.cardLabel, kCard);
        format_set_top(f.cardLabel, LXW_BORDER_MEDIUM);
        format_set_top_color(f.cardLabel, kAccent);
        format_set_indent(f.cardLabel, 1);
        f.cardMissing = NewFormat(wb, 18.0, kMuted);
        format_set_bg_color(f.cardMissing, kCard);
        format_set_indent(f.cardMissing, 1);

        f.description = NewFormat(wb, 10.0, kInk);
        format_set_align(f.description, LXW_ALIGN_VERTICAL_TOP);
        format_set_text_wrap(f.description);
        format_set_bg_color(f.description, kCard);
        format_set_border(f.description, LXW_BORDER_THIN);
        format_set_border_color(f.description, kRule);
        f.footnote = NewFormat(wb, 9.0, kMuted);
        f.note = NewFormat(wb, 11.0, kMuted);
        return f;
    }

    //! A4, small margins, a footer with the page number; the report sheets fit the page width.
    void PageSetup(lxw_worksheet* ws, bool landscape)
    {
        worksheet_set_paper(ws, 9); // A4
        if (landscape) worksheet_set_landscape(ws); else worksheet_set_portrait(ws);
        worksheet_set_margins(ws, 0.5, 0.5, 0.6, 0.6);
        worksheet_set_footer(ws, "&L&8LR 10K material test report&R&8Page &P of &N");
        worksheet_fit_to_pages(ws, 1, 0);
    }

    //! Title (row 1), subtitle (row 2) and the software version at the top right, over columns 0..lastCol.
    void SheetTitle(lxw_worksheet* ws, const Formats& f, lxw_col_t lastCol, const char* title, const std::string& subtitle, const std::string& version)
    {
        worksheet_set_row(ws, 0, 30, NULL);
        worksheet_write_string(ws, 0, 0, title, f.title);
        if (!version.empty())
            worksheet_write_string(ws, 0, lastCol, ("LR 10K software " + version).c_str(), f.version);
        worksheet_write_string(ws, 1, 0, subtitle.c_str(), f.subtitle);
    }
}

static void WriteGraphData(const ExcelReport::Report& report, const Formats& f, lxw_worksheet* worksheet, GraphSheet dataVariation, std::vector<ExcelReport::PointRow>* points)
{
    const ExcelReport::Units& units = report.units;
    const std::string stress = Pretty(units.stress);
    const bool raw = dataVariation == VAR_RAW;
    const char* strain = dataVariation == VAR_TRUE_STRESS ? "True strain (%)" : "Strain (%)";
    const std::string x = raw ? "Extension (" + units.length + ")" : strain;
    const std::string y = raw ? "Load (" + units.force + ")" : (dataVariation == VAR_TRUE_STRESS ? "True stress (" : "Stress (") + stress + ")";

    // Crucial points (A..C) and the curve (D..E), each with a header
    worksheet_set_row(worksheet, 0, 30, NULL);
    worksheet_write_string(worksheet, 0, 0, "Crucial point", f.tableHeader);
    worksheet_write_string(worksheet, 0, 1, x.c_str(), f.tableHeader);
    worksheet_write_string(worksheet, 0, 2, y.c_str(), f.tableHeader);
    worksheet_write_string(worksheet, 0, 3, x.c_str(), f.tableHeader);
    worksheet_write_string(worksheet, 0, 4, y.c_str(), f.tableHeader);

    // The numbers (zero offset, stress/strain conversion, which crucial points) come from ExcelReport::BuildSheet,
    // tested in tests/excel_report_test.cpp
    const ExcelReport::Sheet sheet = raw ? ExcelReport::Sheet::LoadExtension
        : dataVariation == VAR_ENG_STRESS ? ExcelReport::Sheet::EngineeringStress : ExcelReport::Sheet::TrueStress;
    const ExcelReport::SheetRows rows = ExcelReport::BuildSheet(report, sheet);

    const int colStartData = 3;
    for (size_t row = 0; row < rows.curve.size(); row++)
    {
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartData, rows.curve[row].x, f.tableNumber);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), colStartData + 1, rows.curve[row].y, f.tableNumber);
    }

    for (size_t row = 0; row < rows.points.size(); row++)
    {
        const ExcelReport::PointRow& point = rows.points[row];
        worksheet_write_string(worksheet, (lxw_row_t)(row + 1), 0, ExcelReport::PointName(point.id), f.tableText);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), 1, point.at.x, f.tableNumber);
        worksheet_write_number(worksheet, (lxw_row_t)(row + 1), 2, point.at.y, f.tableNumber);
    }
    *points = rows.points;

    // Keep the header row visible when scrolling
    worksheet_freeze_panes(worksheet, 1, 0);
}

static void CreateGraphChart(const ExcelReport::Report& report, lxw_workbook* workbook, const Formats& f, const char* sheet_name, lxw_worksheet* worksheet, GraphSheet dataVariation)
{
    const std::string date = ExcelReport::TidyDate(report.date);

    std::vector<ExcelReport::PointRow> points;
    WriteGraphData(report, f, worksheet, dataVariation, &points);
    const uint32_t curveRows = (uint32_t)report.data.size();

    worksheet_set_column(worksheet, 0, 0, 24, NULL);
    worksheet_set_column(worksheet, 1, 4, 13, NULL);
    worksheet_set_column(worksheet, 5, 5, 3, NULL);  // Space before the chart
    worksheet_gridlines(worksheet, LXW_HIDE_ALL_GRIDLINES);
    worksheet_set_tab_color(worksheet, kAccent);
    PageSetup(worksheet, true);

    // No data: a chart with an empty (reversed) range makes Excel report the file as damaged
    if (curveRows == 0)
    {
        worksheet_write_string(worksheet, CELL("G2"), "No data recorded for this test.", f.note);
        return;
    }

    // Straight lines between the measured samples (the smoothed curve could overshoot between them)
    lxw_chart* chart = workbook_add_chart(workbook, LXW_CHART_SCATTER_STRAIGHT);

    // ----------------------------------------------------
    // --------------- The curve
    // ----------------------------------------------------
    const char* seriesName = "Load";
    if (dataVariation == VAR_ENG_STRESS)
        seriesName = "Engineering stress";
    else if (dataVariation == VAR_TRUE_STRESS)
        seriesName = "True stress";

    lxw_chart_series* series = chart_add_series(chart, NULL, NULL);
    chart_series_set_categories(series, sheet_name, 1, 3, curveRows, 3); // Column D (X-values)
    chart_series_set_values(series, sheet_name, 1, 4, curveRows, 4);     // Column E (Y-values)
    chart_series_set_name(series, seriesName);
    lxw_chart_line curveLine = {};
    curveLine.color = kAccent;
    curveLine.width = 2.0;
    chart_series_set_line(series, &curveLine);
    chart_series_set_marker_type(series, LXW_CHART_MARKER_NONE);

    // ----------------------------------------------------
    // --------------- Crucial points: one series each (own colour, labelled with its name)
    // ----------------------------------------------------
    lxw_chart_line noLine = {};
    noLine.none = LXW_TRUE;
    lxw_chart_line labelBorder = {};
    labelBorder.color = kRule;
    labelBorder.width = 0.75;
    lxw_chart_fill labelFill = {};
    labelFill.color = kWhite;
    for (size_t i = 0; i < points.size(); i++)
    {
        const int id = points[i].id;
        const lxw_row_t row = (lxw_row_t)(i + 1);
        lxw_chart_series* point = chart_add_series(chart, NULL, NULL);
        chart_series_set_categories(point, sheet_name, row, 1, row, 1); // Column B
        chart_series_set_values(point, sheet_name, row, 2, row, 2);     // Column C
        chart_series_set_name(point, ExcelReport::PointShortName(id));
        chart_series_set_line(point, &noLine);

        lxw_chart_fill markerFill = {};
        markerFill.color = kPointColour[id];
        lxw_chart_line markerLine = {};
        markerLine.color = kWhite;
        markerLine.width = 1.25;
        chart_series_set_marker_type(point, LXW_CHART_MARKER_CIRCLE);
        chart_series_set_marker_size(point, 8);
        chart_series_set_marker_fill(point, &markerFill);
        chart_series_set_marker_line(point, &markerLine);

        // The point's name as its label (custom text: "show series name" also shows the value in some versions)
        lxw_chart_data_label nameLabel = {};
        nameLabel.value = (char*)ExcelReport::PointShortName(id);
        lxw_chart_data_label* labels[] = { &nameLabel, NULL };
        chart_series_set_labels(point);
        chart_series_set_labels_custom(point, labels);
        lxw_chart_font labelFont = {};
        labelFont.name = (char*)"Calibri";
        labelFont.size = 9;
        labelFont.bold = LXW_TRUE;
        labelFont.color = kPointColour[id];
        chart_series_set_labels_font(point, &labelFont);
        chart_series_set_labels_line(point, &labelBorder);
        chart_series_set_labels_fill(point, &labelFill);
        // The E-limits are low on the curve: their labels go right of the point, the others above it
        // Where there is usually room: the E-limits (low on the steep part) and the fracture (the end of the curve)
        // to the right, the proportional limit below (under the curve), the yield point and the UTS above
        lxw_chart_label_position position = LXW_CHART_LABEL_POSITION_ABOVE;
        if (id == TestCalc::ModulusLower || id == TestCalc::ModulusUpper || id == TestCalc::Fracture)
            position = LXW_CHART_LABEL_POSITION_RIGHT;
        else if (id == TestCalc::ProportionalLimit)
            position = LXW_CHART_LABEL_POSITION_BELOW;
        chart_series_set_labels_position(point, position);
    }

    // ----------------------------------------------------
    // --------------- Title, axes, gridlines
    // ----------------------------------------------------
    std::string title = "Load - extension";
    if (dataVariation == VAR_ENG_STRESS)
        title = "Engineering stress - strain";
    else if (dataVariation == VAR_TRUE_STRESS)
        title = "True stress - strain";
    if (!date.empty())
        title += "   |   " + date;
    chart_title_set_name(chart, title.c_str());
    lxw_chart_font titleFont = {};
    titleFont.name = (char*)"Calibri";
    titleFont.size = 13;
    titleFont.bold = LXW_TRUE;
    titleFont.color = kInk;
    chart_title_set_name_font(chart, &titleFont);

    if (dataVariation == VAR_RAW)
    {
        chart_axis_set_name(chart->x_axis, ("Extension (" + report.units.length + ")").c_str());
        chart_axis_set_name(chart->y_axis, ("Load (" + report.units.force + ")").c_str());
    }
    else
    {
        chart_axis_set_name(chart->x_axis, dataVariation == VAR_TRUE_STRESS ? "True strain (%)" : "Strain (%)");
        chart_axis_set_name(chart->y_axis, ((dataVariation == VAR_TRUE_STRESS ? "True stress (" : "Stress (") + report.units.stressAxis + ")").c_str());
    }

    lxw_chart_font axisNameFont = {};
    axisNameFont.name = (char*)"Calibri";
    axisNameFont.size = 10;
    axisNameFont.color = kText;
    lxw_chart_font axisNumberFont = {};
    axisNumberFont.name = (char*)"Calibri";
    axisNumberFont.size = 9;
    axisNumberFont.color = kMuted;
    lxw_chart_line axisLine = {};
    axisLine.color = 0x9CA3AF;
    axisLine.width = 0.75;
    lxw_chart_line gridLine = {};
    gridLine.color = kRule;
    gridLine.width = 0.75;
    for (lxw_chart_axis* axis : { chart->x_axis, chart->y_axis })
    {
        chart_axis_set_name_font(axis, &axisNameFont);
        chart_axis_set_num_font(axis, &axisNumberFont);
        chart_axis_set_line(axis, &axisLine);
        chart_axis_set_major_tick_mark(axis, LXW_CHART_AXIS_TICK_MARK_OUTSIDE);
        // Light major gridlines only; Excel chooses the spacing from the data
        chart_axis_major_gridlines_set_visible(axis, LXW_TRUE);
        chart_axis_major_gridlines_set_line(axis, &gridLine);
    }

    // Flat: no chart border, white plot area, no legend (the points are labelled; the curve is in the title)
    lxw_chart_line chartBorder = {};
    chartBorder.none = LXW_TRUE;
    chart_chartarea_set_line(chart, &chartBorder);
    lxw_chart_fill white = {};
    white.color = kWhite;
    chart_chartarea_set_fill(chart, &white);
    chart_plotarea_set_fill(chart, &white);
    chart_plotarea_set_line(chart, &chartBorder);
    chart_legend_set_position(chart, LXW_CHART_LEGEND_NONE);

    lxw_chart_options options = {};
    options.x_offset = 0;
    options.y_offset = 4;
    options.x_scale = 2.2;
    options.y_scale = 2.2;
    worksheet_insert_chart_opt(worksheet, CELL("G2"), chart, &options);

    // Printed: the chart only, on one landscape page
    worksheet_print_area(worksheet, 0, 6, 33, 22); // G1:W34
    worksheet_fit_to_pages(worksheet, 1, 1);
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
    const std::string date = ExcelReport::TidyDate(report.date);
    const std::string subject = std::string(ExcelReport::TestKindText(report.kind)) + " test, " + date;
    const std::string comments = "Exported with LR 10K software " + report.appVersion;
    lxw_doc_properties properties = {};
    properties.title = (char*)"LR 10K material test report";
    properties.subject = (char*)subject.c_str();
    properties.comments = (char*)comments.c_str();
    workbook_set_properties(workbook, &properties);

    // 2. Formats
    const Formats f = MakeFormats(workbook);
    const std::string subtitle = std::string(ExcelReport::TestKindText(report.kind)) + " test" + (date.empty() ? "" : "   |   " + date);

    // A label / value / unit line in columns A, B, C
    auto writeLine = [&](lxw_worksheet* ws, lxw_row_t row, const ExcelReport::Line& line) {
        worksheet_write_string(ws, row, 0, line.label.c_str(), f.label);
        if (line.isNumber)
            worksheet_write_number(ws, row, 1, line.number, f.number[(std::clamp)(line.decimals, 0, 3)]);
        else if (line.text == "-")
            worksheet_write_string(ws, row, 1, line.text.c_str(), f.missing);
        else // Long notes are left-aligned so they can run on into the empty columns
            worksheet_write_string(ws, row, 1, line.text.c_str(), line.text.size() > 32 ? f.textLeft : f.textRight);
        worksheet_write_string(ws, row, 2, Pretty(line.unit).c_str(), f.unit);
    };

    // ---------------------------------------------------------
    // A. REPORT SHEET (always: the test and specimen information is useful without dimensions too)
    // ---------------------------------------------------------
    lxw_worksheet* ws_report = workbook_add_worksheet(workbook, "Full Report");
    worksheet_gridlines(ws_report, LXW_HIDE_ALL_GRIDLINES);
    worksheet_set_tab_color(ws_report, kHeaderFill);
    PageSetup(ws_report, false);
    worksheet_set_column(ws_report, 0, 0, 30, NULL);
    worksheet_set_column(ws_report, 1, 6, 13, NULL);
    SheetTitle(ws_report, f, 6, "Material test report", subtitle, report.appVersion);

    // Key results: up to 4 cards (A, B:C, D:E, F:G), a small label over a large value with its unit
    lxw_row_t row = 3;
    {
        const std::vector<ExcelReport::KeyResult> cards = ExcelReport::KeyResults(report);
        worksheet_set_row(ws_report, row, 20, NULL);
        worksheet_set_row(ws_report, row + 1, 34, NULL);
        for (size_t i = 0; i < cards.size() && i < 4; i++)
        {
            const ExcelReport::KeyResult& card = cards[i];
            const lxw_col_t first = i == 0 ? 0 : (lxw_col_t)(2 * i - 1);
            const lxw_col_t last = i == 0 ? 0 : (lxw_col_t)(2 * i);

            // The unit is part of the number format, so the cell stays a number
            lxw_format* value = NewFormat(workbook, 18.0, kInk);
            format_set_bold(value);
            format_set_bg_color(value, kCard);
            format_set_indent(value, 1);
            format_set_align(value, LXW_ALIGN_LEFT);
            std::string numberFormat = card.decimals <= 0 ? "0" : "0." + std::string((size_t)(std::min)(card.decimals, 3), '0');
            const std::string unit = Pretty(card.unit);
            if (!unit.empty()) numberFormat += unit == "%" ? "\" %\"" : "\" " + unit + "\"";
            format_set_num_format(value, numberFormat.c_str());

            if (first == last)
            {
                worksheet_write_string(ws_report, row, first, card.label.c_str(), f.cardLabel);
                if (card.known) worksheet_write_number(ws_report, row + 1, first, card.value, value);
                else worksheet_write_string(ws_report, row + 1, first, "-", f.cardMissing);
            }
            else
            {
                worksheet_merge_range(ws_report, row, first, row, last, card.label.c_str(), f.cardLabel);
                worksheet_merge_range(ws_report, row + 1, first, row + 1, last, "", card.known ? value : f.cardMissing);
                if (card.known) worksheet_write_number(ws_report, row + 1, first, card.value, value);
                else worksheet_write_string(ws_report, row + 1, first, "-", f.cardMissing);
            }
        }
        row += 3;
    }

    for (const ExcelReport::Section& section : ExcelReport::ReportSections(report))
    {
        worksheet_set_row(ws_report, row, 22, NULL);
        worksheet_merge_range(ws_report, row, 0, row, 2, section.title.c_str(), f.section);
        row++;
        for (const ExcelReport::Line& line : section.lines)
            writeLine(ws_report, row++, line);
        row++;
    }

    // Crucial points: engineering and true stress/strain side by side. Points that were not placed show "-"
    // (they used to show 0.000, which looked like a measured value).
    if (ExcelReport::HasStressSheets(report))
    {
        worksheet_set_row(ws_report, row, 22, NULL);
        worksheet_merge_range(ws_report, row, 0, row, 4, "Crucial points", f.section);
        row++;
        const std::string stressHeader = "Stress (" + Pretty(report.units.stress) + ")";
        const std::string trueStressHeader = "True stress (" + Pretty(report.units.stress) + ")";
        worksheet_set_row(ws_report, row, 30, NULL);
        worksheet_write_string(ws_report, row, 0, "Point", f.tableHeader);
        worksheet_write_string(ws_report, row, 1, stressHeader.c_str(), f.tableHeader);
        worksheet_write_string(ws_report, row, 2, "Strain (%)", f.tableHeader);
        worksheet_write_string(ws_report, row, 3, trueStressHeader.c_str(), f.tableHeader);
        worksheet_write_string(ws_report, row, 4, "True strain (%)", f.tableHeader);
        row++;
        for (const ExcelReport::PointResult& point : ExcelReport::PointResults(report))
        {
            worksheet_write_string(ws_report, row, 0, point.name.c_str(), f.tableText);
            if (point.placed)
            {
                worksheet_write_number(ws_report, row, 1, point.stress, f.tableNumber);
                worksheet_write_number(ws_report, row, 2, point.strainPercent, f.tableNumber);
                worksheet_write_number(ws_report, row, 3, point.trueStress, f.tableNumber);
                worksheet_write_number(ws_report, row, 4, point.trueStrainPercent, f.tableNumber);
            }
            else
            {
                for (lxw_col_t col = 1; col <= 4; col++)
                    worksheet_write_string(ws_report, row, col, "-", f.tableMissing);
            }
            row++;
        }
        row++;
    }

    // Description
    worksheet_set_row(ws_report, row, 22, NULL);
    worksheet_merge_range(ws_report, row, 0, row, 2, "Description", f.section);
    row++;
    worksheet_merge_range(ws_report, row, 0, row + 4, 6, report.description.c_str(), f.description);
    row += 6;
    if (!report.exportedAt.empty())
        worksheet_write_string(ws_report, row, 0, ("Exported " + report.exportedAt + (report.appVersion.empty() ? "" : " with LR 10K software " + report.appVersion)).c_str(), f.footnote);

    // ---------------------------------------------------------
    // B. SETUP SHEET
    // ---------------------------------------------------------
    lxw_worksheet* ws_setup = workbook_add_worksheet(workbook, "Setup");
    worksheet_gridlines(ws_setup, LXW_HIDE_ALL_GRIDLINES);
    worksheet_set_tab_color(ws_setup, 0x9CA3AF);
    PageSetup(ws_setup, false);
    worksheet_set_column(ws_setup, 0, 0, 30, NULL);
    worksheet_set_column(ws_setup, 1, 1, 26, NULL);
    worksheet_set_column(ws_setup, 2, 2, 12, NULL);
    SheetTitle(ws_setup, f, 2, "LR 10K settings", subtitle, "");
    row = 3;
    worksheet_set_row(ws_setup, row, 22, NULL);
    worksheet_merge_range(ws_setup, row, 0, row, 2, "Machine and export", f.section);
    row++;
    for (const ExcelReport::Line& line : ExcelReport::SetupLines(report))
        writeLine(ws_setup, row++, line);

    // ---------------------------------------------------------
    // C. GRAPHS (data next to each chart)
    // ---------------------------------------------------------
    const char* sheetNameGraphNmm   = "Graph N-mm";
    lxw_worksheet* worksheetGraphNmm = workbook_add_worksheet(workbook, sheetNameGraphNmm);
    CreateGraphChart(report, workbook, f, sheetNameGraphNmm, worksheetGraphNmm, VAR_RAW);

    // Stress-strain sheets only where they mean something (tension/compression with dimensions). These sheets
    // used to be created empty without dimensions, and filled with tension formulas for 3-point bending.
    if (ExcelReport::HasStressSheets(report))
    {
        const char* sheetNameEngStress  = "Graph Eng. Stress";
        const char* sheetNameTrueStress = "Graph True Stress";
        lxw_worksheet* worksheetEngStress  = workbook_add_worksheet(workbook, sheetNameEngStress);
        lxw_worksheet* worksheetTrueStress = workbook_add_worksheet(workbook, sheetNameTrueStress);
        CreateGraphChart(report, workbook, f, sheetNameEngStress, worksheetEngStress, VAR_ENG_STRESS);
        CreateGraphChart(report, workbook, f, sheetNameTrueStress, worksheetTrueStress, VAR_TRUE_STRESS);
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
