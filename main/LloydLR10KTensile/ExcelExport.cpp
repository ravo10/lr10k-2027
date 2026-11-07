/**

    @file      ExcelExport.cpp
    @brief     Excel (.xlsx) export of test data, crucial points and report.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// ------------------------------- Excel related -------------------------------
// ----------------------------------------------------------------------------- σ
void create_style_gallery(TabControl::TabTestDataClass tabData) {
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

void AppMainExcel::write_worksheet_data(TabControl::TabTestDataClass tabData, lxw_workbook* workbook, std::vector<lxw_chart_data_label*>* activeLabels, lxw_worksheet* worksheet, AppMainExcel::ExcelDataVariationWorksheetData dataVariation)
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

    std::string label = "";
    if (dataVariation == VAR_RAW)
    {
        // Crucial points
        worksheet_write_string(worksheet, CELL("A1"), "P ID", fmt_header);
        label = "P X (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(0)) + ")";
        worksheet_write_string(worksheet, CELL("B1"), label.c_str(), fmt_header);
        label = "P Y (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(1)) + ")";
        worksheet_write_string(worksheet, CELL("C1"), label.c_str(), fmt_header);

        // Load-extension data
        label = "X (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(0)) + ")";
        worksheet_write_string(worksheet, CELL("D1"), label.c_str(), fmt_header);
        label = "Y (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(1)) + ")";
        worksheet_write_string(worksheet, CELL("E1"), label.c_str(), fmt_header);
    }
    else
    {
        // Crucial points
        worksheet_write_string(worksheet, CELL("A1"), "P ID", fmt_header);
        label = "P X (ε)";
        worksheet_write_string(worksheet, CELL("B1"), label.c_str(), fmt_header);
        label = "P Y (σ, " + std::string(LR10KAppGlobals::GetUnitOption<const char*>(7)) + ")";
        worksheet_write_string(worksheet, CELL("C1"), label.c_str(), fmt_header);

        // Load-extension data
        label = "X (ε)";
        worksheet_write_string(worksheet, CELL("D1"), label.c_str(), fmt_header);
        label = "Y (σ, " + std::string(LR10KAppGlobals::GetUnitOption<const char*>(7)) + ")";
        worksheet_write_string(worksheet, CELL("E1"), label.c_str(), fmt_header);
    }

    // Main plot points (graph)
    RollingBuffer tensileData = tabData.LR10KTensileData;

    int colStartData = 3;
    int rowStartPoint = 0;
    int colStartPoint = 0;

    // Safe denominators for math. Recompute the area from the dimensions instead of trusting the stored
    // value: files saved before this fix contain a wrong area for circular specimens (pi * d / 4).
    double A_0 = MaterialCalc::CrossSectionArea(tabData.areaOptionPicked, tabData.thickness, tabData.width, tabData.diameter);
    if (A_0 <= 0.0) A_0 = tabData.area > 0 ? tabData.area : 1.0;
    double L_0 = tabData.length > 0 ? tabData.length : 1.0;

    const double zeroPointOffset = defaultValuesGUI::offsetZeroPointExtensionAxis;

    for (size_t row = 0; row < tensileData.Data.size(); row++) {
        const float originalX = tensileData.Data[row].x; // Before the zero point offset: crucial points are stored like this
        tensileData.Data[row].x -= (float)zeroPointOffset;

        ImVec2 dataPlotPointRaw = tensileData.Data[row];
        ImVec2 dataPlotPointStress = tensileData.Data[row];

        // Convert to STRESS
        if (dataVariation != VAR_RAW)
        {
            // Nominal stress
            dataPlotPointStress.x = LR10KAppGlobals::CalcEpsilon_e(dataPlotPointStress.x, L_0);
            dataPlotPointStress.y = LR10KAppGlobals::CalcSigma_e(dataPlotPointStress.y, A_0);
        }
        if (dataVariation == VAR_TRUE_STRESS)
        {
            // True stress. Uses the ENGINEERING strain, so compute it before x is overwritten
            // (this used the already converted true strain before: true stress came out too low).
            const double engineeringStrain = dataPlotPointStress.x;
            dataPlotPointStress.y = LR10KAppGlobals::CalcSigma_t(engineeringStrain, dataPlotPointStress.y);
            dataPlotPointStress.x = LR10KAppGlobals::CalcEpsilon_t(engineeringStrain);
        }

        // -----------------------------------------------

        // Plot point
        if (dataVariation == VAR_RAW)
        {
            // Plot point (RAW)
            worksheet_write_number(worksheet, row + 1, colStartData, dataPlotPointRaw.x, fmt_decimal3);
            worksheet_write_number(worksheet, row + 1, colStartData + 1, dataPlotPointRaw.y, fmt_decimal3);
        }
        else
        {
            // Plot point (STRESS)
            worksheet_write_number(worksheet, row + 1, colStartData, dataPlotPointStress.x*100, fmt_decimal3);
            worksheet_write_number(worksheet, row + 1, colStartData + 1, dataPlotPointStress.y, fmt_decimal3);
        }

        // -----------------------------------------------

        // Only crucial points for RAW and Engineering stress...
        if (dataVariation == VAR_RAW || dataVariation == VAR_ENG_STRESS)
        {
            // Crucial points (UTS, E-Modulus etc.)
            for (const auto& point_tuple : tabData.plot_dataPoints) {
                // Get point
                ImPlotPoint dataRaw = std::get<0>(point_tuple);
                ImPlotPoint dataStress = std::get<0>(point_tuple);

                int id = std::get<1>(point_tuple);

                // A matching point the crucial point is "snapped" to. Compare before the offset is applied:
                // comparing the stored point with the offset row never matched when a zero point offset was
                // set, so the crucial points were missing from the Excel file.
                const bool matchesRow = (dataRaw.x == (double)originalX);

                // Same zero point as the data rows (and as the calculations in the app)
                dataRaw.x -= zeroPointOffset;
                dataStress.x -= zeroPointOffset;

                if (dataVariation == VAR_ENG_STRESS)
                {
                    // Nominal stress
                    dataStress.x = LR10KAppGlobals::CalcEpsilon_e(dataStress.x, L_0);
                    dataStress.y = LR10KAppGlobals::CalcSigma_e(dataStress.y, A_0);
                }

                if (matchesRow)
                {
                    // Write into worksheet
                    worksheet_write_string(worksheet, rowStartPoint + 1, colStartPoint, TabControl::plot_items[id], NULL);

                    if (dataVariation == VAR_ENG_STRESS)
                    {
                        // ENG. STRESS
                        worksheet_write_number(worksheet, rowStartPoint + 1, colStartPoint + 1, dataStress.x*100, fmt_decimal3);
                        worksheet_write_number(worksheet, rowStartPoint + 1, colStartPoint + 2, dataStress.y, fmt_decimal3);
                    }
                    else
                    {
                        // RAW
                        worksheet_write_number(worksheet, rowStartPoint + 1, colStartPoint + 1, dataRaw.x, fmt_decimal3);
                        worksheet_write_number(worksheet, rowStartPoint + 1, colStartPoint + 2, dataRaw.y, fmt_decimal3);
                    }

                    // Add label to crucial point
                    activeLabels->push_back(data_labels_options[id]);

                    rowStartPoint++;
                }
            }
        }

        // -----------------------------------------------
    }
    activeLabels->push_back(NULL); // Add NULL terminator for the C-style array
}

void AppMainExcel::CreateGraphChart(TabControl::TabTestDataClass tabData, lxw_workbook* workbook, const char* sheet_name, lxw_worksheet* worksheet, const std::string date, AppMainExcel::ExcelDataVariationWorksheetData dataVariation)
{
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
    write_worksheet_data(tabData, workbook, &activeLabels, worksheet, dataVariation);

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
    chart_series_set_categories(series, sheet_name, CELL("D2"), (uint32_t)tabData.LR10KTensileData.Data.size() + 1, 3); // Column D (X-values) CELL("D2") = 1,3
    chart_series_set_values(series, sheet_name, CELL("E2"), (uint32_t)tabData.LR10KTensileData.Data.size() + 1, 4); // Column E (Y-values) CELL("E2") = 1,4

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
        chart_series_set_categories(series2, sheet_name, CELL("B2"), (uint32_t)tabData.LR10KTensileData.Data.size() + 1, 1); // Column B (X-values) CELL("B2") = 1,1
        chart_series_set_values(series2, sheet_name, CELL("C2"), (uint32_t)tabData.LR10KTensileData.Data.size() + 1, 2); // Column C (Y-values) CELL("C2") = 1,2

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
        chart_axis_set_name(chart->x_axis, ("Extension (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(0)) + ")").c_str());
        chart_axis_set_name(chart->y_axis, ("Load (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(1)) + ")").c_str());
    }
    else
    {
        chart_axis_set_name(chart->x_axis, "Strain (Epsilon %)");
        chart_axis_set_name(chart->y_axis, ("Stress (" + std::string(LR10KAppGlobals::GetUnitOption<const char*>(6)) + ")").c_str());
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

bool AppMainExcel::SavePlotDataAsAnXlsxFile(TabControl::TabTestDataClass tabData, const char* filename)
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
    bool dimensionsAdded = (tabData.thickness > 0 && tabData.width > 0 && tabData.length > 0);

    std::string unit_raw = LR10KAppGlobals::GetUnitOption<const char*>(0); //SI: mm
    std::string unit_stress = LR10KAppGlobals::GetUnitOption<const char*>(7); //SI: N/mm^2 eller MPa
    std::string unit_strain = "% (ɛ)"; //SI: N/mm^2 eller MPa
    std::string unit_speed = LR10KAppGlobals::GetUnitOption<const char*>(2); //SI: mm/min
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
        worksheet_write_string(ws_report, 1, 0, std::string("Test date: " + tabData.date).c_str(), fmt_bold_left);

        // Properties
        std::string dim = tabData.unitsConvertionIndex == 0 ? "SI" : "English Engineering";
        dim = "Dimensions (" + dim + ")";
        worksheet_write_string(ws_report, CELL("A4"), dim.c_str(), fmt_bold_left);

        

        if (tabData.areaOptionPicked == 0)
        {
            label = "Gauge Thickness (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A5"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B5"), tabData.thickness, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C5"), unit_raw.c_str(), fmt_bold_left);

            label = "Gauge Width (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A6"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B6"), tabData.width, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C6"), unit_raw.c_str(), fmt_bold_left);
        }
        else if (tabData.areaOptionPicked == 1)
        {
            label = "-";
            worksheet_write_string(ws_report, CELL("A5"), label.c_str(), NULL);
            worksheet_write_string(ws_report, CELL("B5"), "-", NULL);
            worksheet_write_string(ws_report, CELL("C5"), "-", NULL);

            label = "Gauge Diameter (" + unit_raw + ")";
            worksheet_write_string(ws_report, CELL("A6"), label.c_str(), NULL);
            worksheet_write_number(ws_report, CELL("B6"), tabData.diameter, fmt_decimal3);
            worksheet_write_string(ws_report, CELL("C6"), unit_raw.c_str(), fmt_bold_left);
        }

        label = "Gauge Length (L0) (" + unit_raw + ")";
        worksheet_write_string(ws_report, CELL("A7"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B7"), tabData.length, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C7"), unit_raw.c_str(), fmt_bold_left);

        label = "Cross-sectional area (" + unit_raw + "^2)";
        std::string unit = unit_raw + "^2";
        worksheet_write_string(ws_report, CELL("A8"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B8"), tabData.area, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C8"), unit.c_str(), fmt_bold_left);

        // (Add other calculations like E-Modulus here if needed) σ
        int strain = 0;
        int stress = 1;

        // Stress
        worksheet_write_string(ws_report, CELL("A10"), "Engineering stress (nominal stress)", fmt_bold_left);

        // GPa with SI units, psi with imperial units (same as in the app)
        const std::string unit_modulus = LR10KAppGlobals::unitsConvertionIndex == 0 ? "GPa" : LR10KAppGlobals::GetUnitOption<const char*>(7);
        label = "Young's Modulus, E-Modulus (" + unit_modulus + ")";
        worksheet_write_string(ws_report, CELL("A11"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B11"), tabData.calcYoungsModulus, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C11"), unit_modulus.c_str(), fmt_bold_left);

        label = "Proportional Limit (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A12"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B12"), tabData.calcPoint_NominalStressStrain_PropLimit[stress], fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C12"), unit_stress.c_str(), fmt_bold_left);

        label = "Yield Strength (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A13"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B13"), tabData.calcPoint_NominalStressStrain_yield[stress], fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C13"), unit_stress.c_str(), fmt_bold_left);

        label = "Ultimate Tensile Stress (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A14"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B14"), tabData.calcPoint_NominalStressStrainUTS[stress], fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C14"), unit_stress.c_str(), fmt_bold_left);

        label = "Fracture (" + unit_stress + ")";
        worksheet_write_string(ws_report, CELL("A15"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("B15"), tabData.calcPoint_NominalStressStrain_fracture[stress], fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C15"), unit_stress.c_str(), fmt_bold_left);

        // Strain
        worksheet_write_string(ws_report, CELL("D10"), "Engineering strain (nominal strain)", fmt_bold_left);

        label = "Proportional Limit (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D12"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E12"), tabData.calcPoint_NominalStressStrain_PropLimit[strain]*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F12"), unit_strain.c_str(), fmt_bold_left);

        label = "Yield Strength (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D13"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E13"), tabData.calcPoint_NominalStressStrain_yield[strain]*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F13"), unit_strain.c_str(), fmt_bold_left);

        label = "Ultimate Tensile Stress (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D14"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E14"), tabData.calcPoint_NominalStressStrainUTS[strain]*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F14"), unit_strain.c_str(), fmt_bold_left);

        label = "Fracture (" + unit_strain + ")";
        worksheet_write_string(ws_report, CELL("D15"), label.c_str(), NULL);
        worksheet_write_number(ws_report, CELL("E15"), tabData.calcPoint_NominalStressStrain_fracture[strain]*100, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("F15"), unit_strain.c_str(), fmt_bold_left);

        // ------------
        // Description
        // ------------
        worksheet_write_string(ws_report, CELL("A17"), "Description", fmt_bold_left);
        worksheet_merge_range(ws_report, CELL("A17"), CELL("C17"), "Description", fmt_bold_left);

        worksheet_write_string(ws_report, CELL("A18"), tabData.description.c_str(), fmt_upper_right);
        worksheet_merge_range(ws_report, CELL("A18"), CELL("C23"), tabData.description.c_str(), fmt_upper_right);
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
    worksheet_write_number(ws_setup, CELL("B3"), tabData.settings_speedAndPlot[0], fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C3"), unit_speed.c_str(), fmt_bold_left);

    label = "Jog speed (" + unit_speed + ")";
    worksheet_write_string(ws_setup, CELL("A4"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B4"), tabData.settings_speedAndPlot[1], fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C4"), unit_speed.c_str(), fmt_bold_left);

    label = "Load Error (%)";
    worksheet_write_string(ws_setup, CELL("A5"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B5"), tabData.settings_speedAndPlot[2], fmt_decimal2);
    worksheet_write_string(ws_setup, CELL("C5"), "%", fmt_bold_left);

    label = "Test Type (Tension or Compr.)";
    worksheet_write_string(ws_setup, CELL("A6"), label.c_str(), NULL);
    worksheet_write_string(ws_setup, CELL("B6"), tabData.tension ? "Tension Testing" : "Compression Testing", fmt_bold_right);

    // Some old saves won't have this data, and is set to 0 after loading it in (before 2026)
    label = "Sampling Rate (Hz)";
    worksheet_write_string(ws_setup, CELL("A7"), label.c_str(), NULL);
    worksheet_write_number(ws_setup, CELL("B7"), tabData.LR10KTensileData.SamplingRate, fmt_decimal0);
    worksheet_write_string(ws_setup, CELL("C7"), "Hz", fmt_bold_left);

    // ---------------------------------------------------------
    // C. GRAPH 1: LOAD - EXTENSION (The "Raw" Graph)
    // ---------------------------------------------------------
    // For this one, we stick to the existing pattern:
    // Create a sheet -> Write the data -> Add the chart.

    // Prep Data (Zero Offset)
    RollingBuffer tensileData = tabData.LR10KTensileData;
    for (size_t i = 0; i < tensileData.Data.size(); i++) {
        tensileData.Data[i].x -= (float)defaultValuesGUI::offsetZeroPointExtensionAxis;
    }

    // Calculate Limits (Safe Defaults)
    float maxX = 0.0f; float maxY = 0.0f;
    for (const auto& point : tensileData.Data) {
        if (point.x > maxX) maxX = point.x;
        if (point.y > maxY) maxY = point.y;
    }
    if (maxX <= 0) maxX = 1.0f;
    if (maxY <= 0) maxY = 1.0f;

    // Worksheet names
    const char* sheetNameGraphNmm   = "Graph N-mm";
    const char* sheetNameEngStress  = "Graph Eng. Stress";  
    const char* sheetNameTrueStress = "Graph True Stress";

    // Add the worksheets
    lxw_worksheet* worksheetGraphNmm    = workbook_add_worksheet(workbook, sheetNameGraphNmm);
    lxw_worksheet* worksheetEngStress   = workbook_add_worksheet(workbook, sheetNameEngStress);
    lxw_worksheet* worksheetTrueStress  = workbook_add_worksheet(workbook, sheetNameTrueStress);

    // Call the Load-Extension Chart
    CreateGraphChart(tabData, workbook, sheetNameGraphNmm, worksheetGraphNmm, tabData.date, AppMainExcel::VAR_RAW);


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
        CreateGraphChart(tabData, workbook, sheetNameEngStress, worksheetEngStress, tabData.date, AppMainExcel::VAR_ENG_STRESS);

        // 2. True Stress (Optional)
        // Only call this if you want a SECOND sheet for True Stress
        CreateGraphChart(tabData, workbook, sheetNameTrueStress, worksheetTrueStress, tabData.date, AppMainExcel::VAR_TRUE_STRESS);
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
