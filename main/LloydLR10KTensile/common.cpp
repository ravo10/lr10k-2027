/**

    @file      common.cpp
    @brief     Contains logic for different systems including:
               Calculator, Excel (related), File Browsing Logic,
               File Configuration Settings, Tab System, Local Functions,
               LR 10K COM System, LR 10K Control System and Console System.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"

#include "LR10KController.h"

inline LR10KController _LR10KController; //!< A global scope to gain access to the LR 10K Controller logic.

// -----------------------------------------------------------------------------
// --------------------------------- Calculator --------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Sigma_e or Sigma_n | F/A_0.
    @param  f   - 
    @param  a_0 - 
    @retval     - 
**/
double LR10KAppGlobals::CalcSigma_e(double f, double a_0)
{
    return f / a_0;
}
/**
    @brief  Epsilon_e or Epsilon_n | Delta_L / L_0.
    @param  delta_L - 
    @param  l_0     - 
    @retval         - 
**/
double LR10KAppGlobals::CalcEpsilon_e(double delta_L, double l_0)
{
    return delta_L / l_0;
}
/**
    @brief  Sigma_t | Sigma_e * ( 1 + Epsilon_e ).
    @param  sigma_e   - 
    @param  epsilon_e - 
    @retval           - 
**/
double LR10KAppGlobals::CalcSigma_t(double epsilon_e, double sigma_e)
{
    return sigma_e * (1 + epsilon_e);
}
/**
    @brief  Epsilon_t | ln * ( 1 + Epsilon_e ).
    @param  epsilon_e - 
    @retval           - 
**/
double LR10KAppGlobals::CalcEpsilon_t(double epsilon_e)
{
    return std::log(1 + epsilon_e);
}
/**
    @brief  Youngs Modulus. Change the signature to accept limits.
    @param  strainData -
    @param  stressData -
    @param  lower_limit -
    @param  upper_limit -
    @retval            -
**/
double CalculateYoungsModulus(const std::vector<double>& strainData, const std::vector<double>& stressData, double lower_limit, double upper_limit)
{
    // Ensure the limits are in the correct order in case the user crossed them!
    double actual_lower = std::min(lower_limit, upper_limit);
    double actual_upper = std::max(lower_limit, upper_limit);

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    int n = 0;

    for (size_t i = 0; i < strainData.size(); i++)
    {
        double x = strainData[i]; // Strain
        double y = stressData[i]; // Stress

        // Use dynamic limits
        if (x >= actual_lower && x <= actual_upper)
        {
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
            n++;
        }
    }

    double e_module = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);

    // SI Units
    if (LR10KAppGlobals::unitsConvertionIndex == 0)
        // Convert to GPa
        e_module /= 1000;

    if (n < 2) return 0.0;
    return e_module;
}

// -----------------------------------------------------------------------------
// --------------------------------- Convertion --------------------------------
// -----------------------------------------------------------------------------

int FindClosestRawDataIndex(const ImVector<ImVec2>& rawData, double target_x)
{
	if (rawData.empty()) return -1;

	int best_idx = 0;
	double min_distance = DBL_MAX;

	// Loop through raw data ONCE (O(N) efficiency)
	for (int i = 0; i < rawData.Size; i++)
	{
		double distance = std::abs(rawData[i].x - target_x);
		if (distance < min_distance)
		{
			min_distance = distance;
			best_idx = i;
		}
	}
	return best_idx;
}

ImVector<ImVec2> SmoothTensileData(const ImVector<ImVec2>& raw_data, int window_size)
{
	// If we don't have enough data to fill the window, just return the raw data
	if (raw_data.size() < window_size || window_size < 2) return raw_data;

	ImVector<ImVec2> smoothed_data = raw_data;
	int half_window = window_size / 2;

	// Slide the window across the data array
	for (size_t i = half_window; i < raw_data.size() - half_window; ++i)
	{
		double sum_y = 0.0;

		// Add up all the Y (Load) values inside the current window
		for (int j = -half_window; j <= half_window; ++j) {
			sum_y += raw_data[i + j].y;
		}

		// Calculate the average and overwrite the Y value in our copy
		smoothed_data[i].y = sum_y / window_size;
	}

	return smoothed_data;
}

void SnapCrucialPointsToSmoothedCurve(std::vector<std::tuple<ImPlotPoint, int>>& plot_dataPoints, ImVector<ImVec2>& smoothed_data)
{
    // If there is no graph data to snap to, exit safely
    if (smoothed_data.empty()) return;

    // Loop through every crucial marker
    for (auto& ptTuple : plot_dataPoints)
    {
        // Get a reference to the ImPlotPoint so we can modify it permanently
        ImPlotPoint& pt = std::get<0>(ptTuple);

        // Find the index in the smoothed graph that matches the marker's X value (Strain)
        // (Reusing the FindClosestRawDataIndex function you already have!)
        int snap_idx = FindClosestRawDataIndex(smoothed_data, pt.x);

        if (snap_idx != -1)
        {
            // Update the marker to sit exactly on the new smoothed curve
            // We update both X and Y just to prevent any floating-point drift
            pt.x = smoothed_data[snap_idx].x;
            pt.y = smoothed_data[snap_idx].y;
        }
    }
}

ImPlotPoint LR10KAppGlobals::NominalStressStrainGetter(int idx, void* user_data) {
    // Cast the void* back to our struct
    StressStrainContext* ctx = (StressStrainContext*)user_data;

    // Grab the raw load/extension point at this specific index
    double extension = ctx->RawData[idx].x;
    double load = ctx->RawData[idx].y;

    // Calculate real-time stress and strain
    double strain = LR10KAppGlobals::CalcEpsilon_e(extension, ctx->Length0);
    double stress = LR10KAppGlobals::CalcSigma_e(load, ctx->Area0);

    // Return the calculated point to ImPlot
    return ImPlotPoint(strain, stress);
}

ImPlotPoint LR10KAppGlobals::TrueStressStrainGetter(int idx, void* user_data) {
    // Cast the void* back to our struct
    StressStrainContext* ctx = (StressStrainContext*)user_data;

    // Grab the raw load/extension point at this specific index
    double extension = ctx->RawData[idx].x;
    double load = ctx->RawData[idx].y;

    // Calculate real-time stress and strain
    double strain = LR10KAppGlobals::CalcEpsilon_e(extension, ctx->Length0);
    double stress = LR10KAppGlobals::CalcSigma_e(load, ctx->Area0);

    // Calculate the true stress
    ImVec2 stressStrainTrue;
    stressStrainTrue.x = LR10KAppGlobals::CalcEpsilon_t(strain);
    stressStrainTrue.y = LR10KAppGlobals::CalcSigma_t(strain, stress);

    // Return the calculated point to ImPlot
    return ImPlotPoint(stressStrainTrue.x, stressStrainTrue.y);
}

/**
    @brief  Gets the correct value from the units conversion.
    @details When "index" is >= 8, then use double value. Else it is const char*.
    @tparam ReturnType -
    @param  index      -
    @param  unitIndex  -
    @retval            -
**/
template <typename ReturnType>
ReturnType LR10KAppGlobals::GetUnitOption(int index, int unitIndex)
{
    if (unitIndex < LR10KAppGlobals::unitsConvertion.size()) {
        if (std::holds_alternative<ReturnType>(LR10KAppGlobals::unitsConvertion[unitIndex][index]))
        {
            return std::get<ReturnType>(LR10KAppGlobals::unitsConvertion[unitIndex][index]);
        }
    }
    // Return a default value or handle the out-of-bounds case as needed.
    return ReturnType{};
}

void AppCalculator::AdaptTabDataForLoadError(TabControl::TabTestDataClass* tabData)
{
    // -------------------------------------------------------------------------
    // ----------- Convert load to new load that includes load error -----------
    // -------------------------------------------------------------------------

    const double currentLoadErrorGUI    = (double)LR10KAppGlobals::testing_speedAndPlotActual[2];
    const double oldLoadErrorValue      = (double)tabData->oldLoadErrorValue;

    const double loadErrorDifference    = (double)(currentLoadErrorGUI - oldLoadErrorValue);
    const double loadScalar             = loadErrorDifference >= 0 ? (1 + std::abs(loadErrorDifference) / 100) : (1 - std::abs(loadErrorDifference) / 100);

    // "Normal" plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
    {
        tabData->LR10KTensileData.Data[i].y *= loadScalar;
    }

    // Plot points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        points.y *= loadScalar;

        const int index = std::get<int>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index);
    }

    // Save new load error
    tabData->oldLoadErrorValue = currentLoadErrorGUI;
}

void AppCalculator::CalculateStressPoints(TabControl::TabTestDataClass* tabData, bool playSound)
{
    // -------------------------------------------------------------------------
    // 1. Calculate the initial Area (A_0) safely based on user shape settings
    // -------------------------------------------------------------------------
    const double T_0 = LR10KAppGlobals::calc_thickness;
    const double W_0 = LR10KAppGlobals::calc_width;
    const double L_0 = LR10KAppGlobals::calc_length > 0 ? LR10KAppGlobals::calc_length : 1.0;
    const double D_0 = LR10KAppGlobals::calc_diameter;

    double A_0 = 1.0; // Default safe denominator
    if (LR10KAppGlobals::areaOptionPicked == 0) // Example: 0 = Rectangle
        A_0 = (T_0 * W_0) > 0 ? (T_0 * W_0) : 1.0;
    else if (LR10KAppGlobals::areaOptionPicked == 1) // Example: 1 = Circle (pi * r^2)
        A_0 = (D_0 > 0) ? (IM_PI * (D_0 / 2.0) * (D_0 / 2.0)) : 1.0;

    // -------------------------------------------------------------------------
    // 2. Map the UI arrays to a manageable list so we can loop through them
    //    Indices match standard IDs: 0=Prop, 1=Yield, 2=UTS, 3=Frac
    // -------------------------------------------------------------------------
    double* nom_arrays[4] = { tabData->calcPoint_NominalStressStrain_PropLimit, tabData->calcPoint_NominalStressStrain_yield, tabData->calcPoint_NominalStressStrainUTS, tabData->calcPoint_NominalStressStrain_fracture };
    double* true_arrays[4] = { tabData->calcPoint_TrueStressStrain_PropLimit, tabData->calcPoint_TrueStressStrain_yield, tabData->calcPoint_TrueStressStrainUTS, tabData->calcPoint_TrueStressStrain_fracture };
    const char* point_names[4] = { "Proportional limit", "Yielding", "Ultimate Tensile Strength", "Fracture" };

    // Reset all arrays to 0.0 quickly
    for (int i = 0; i < 4; i++) {
        nom_arrays[i][0] = 0.0; nom_arrays[i][1] = 0.0;
        true_arrays[i][0] = 0.0; true_arrays[i][1] = 0.0;
    }

    // -------------------------------------------------------------------------
    // 3. Process the crucial points (if they exist)
    // -------------------------------------------------------------------------
    for (const auto& point_tuple : tabData->plot_dataPoints)
    {
        ImPlotPoint dataPoint = std::get<0>(point_tuple);
        int propID = std::get<1>(point_tuple);

        // Only process valid standard points (0 to 3)
        if (propID >= 0 && propID <= 3)
        {
            DockingApps::console.AddLog("Found a %s point! Calculating...", point_names[propID]);

            // Nominal / Engineering Math
            nom_arrays[propID][0] = LR10KAppGlobals::CalcEpsilon_e(dataPoint.x - tabData->LR10OffsetZeroPointExtensionAxis, L_0);
            nom_arrays[propID][1] = LR10KAppGlobals::CalcSigma_e(dataPoint.y, A_0);

            // True Math
            true_arrays[propID][0] = LR10KAppGlobals::CalcEpsilon_t(nom_arrays[propID][0]);
            true_arrays[propID][1] = LR10KAppGlobals::CalcSigma_t(nom_arrays[propID][0], nom_arrays[propID][1]);
        }
    }

    // -------------------------------------------------------------------------
    // 4. Calculate Young's Modulus using linear regression on ALL raw data
    // -------------------------------------------------------------------------
    if (tabData->LR10KTensileData.Data.Size > 10) // Only if we have enough test data
    {
        // Default limits (0.05% to 0.25%)
        double e_lower = 0.0005;
        double e_upper = 0.0025;

        // See if the user has added custom draggable bounds (ID 5 and 6)
        for (const auto& point_tuple : tabData->plot_dataPoints) {
            if (std::get<1>(point_tuple) == 5) e_lower = LR10KAppGlobals::CalcEpsilon_e(std::get<0>(point_tuple).x - tabData->LR10OffsetZeroPointExtensionAxis, L_0);
            if (std::get<1>(point_tuple) == 6) e_upper = LR10KAppGlobals::CalcEpsilon_e(std::get<0>(point_tuple).x - tabData->LR10OffsetZeroPointExtensionAxis, L_0);
        }

        // We need temporary vectors to feed your regression function
        std::vector<double> strainData, stressData;
        strainData.reserve(tabData->LR10KTensileData.Data.Size);
        stressData.reserve(tabData->LR10KTensileData.Data.Size);

        for (int i = 0; i < tabData->LR10KTensileData.Data.Size; i++) {
            double strainDataWithOffset = (double)(tabData->LR10KTensileData.Data[i].x - tabData->LR10OffsetZeroPointExtensionAxis);
            strainDataWithOffset = (strainDataWithOffset > 0 ? strainDataWithOffset : 0.0);
            double sigmaValue = strainDataWithOffset == 0 ? 0.0 : tabData->LR10KTensileData.Data[i].y;

            strainData.push_back(LR10KAppGlobals::CalcEpsilon_e(strainDataWithOffset, L_0));
            stressData.push_back(LR10KAppGlobals::CalcSigma_e(sigmaValue, A_0));
        }

        tabData->calcYoungsModulus = CalculateYoungsModulus(strainData, stressData, e_lower, e_upper);
        DockingApps::console.AddLog("Calculated the E-Modulus! Lower limit: %f %% and upper limit: %f %%.\nThis resulted in the E-Modulus to be: %f GPa", e_lower * 100, e_upper * 100, tabData->calcYoungsModulus);
    }

    // -------------------------------------------------------------------------
    // 5. Finalize state
    // -------------------------------------------------------------------------
    if (playSound)       PlaySoundEffect("buttonClick1");
}

void AppCalculator::CalculateCompressionBendMomentAndUTS(TabControl::TabTestDataClass* tabData, bool playSound)
{
    // 1. Setup dimensions safely (Prevent Divide by Zero later)
    const double T_0 = LR10KAppGlobals::calc_thickness > 0 ? LR10KAppGlobals::calc_thickness : 1.0;
    const double W_0 = LR10KAppGlobals::calc_width > 0 ? LR10KAppGlobals::calc_width : 1.0;
    const double L_0 = LR10KAppGlobals::calc_length > 0 ? LR10KAppGlobals::calc_length : 1.0;
    double A_0 = T_0 * W_0;

    tabData->thickness = T_0;
    tabData->width = W_0;
    tabData->length = L_0;

    ImPlotPoint UTS = tabData->GetUTS();

    tabData->calcData_Compression[0] = UTS.y;                                                // Load             (max y)
    tabData->calcData_Compression[1] = UTS.x - tabData->LR10OffsetZeroPointExtensionAxis;    // Travel distance  (max x)

    // -------------------------------------------------------------------------
    // STANDARD 3-POINT BENDING FORMULAS
    // -------------------------------------------------------------------------
    // Max bending moment: M = (F * L) / 4 (Uses Span Length, NOT Travel!)
    tabData->calcData_Compression[2] = (tabData->calcData_Compression[0] * L_0) / 4.0;

    // Flexural Strength (UTS): Sigma = (3 * F * L) / (2 * w * t^2)
    tabData->calcData_Compression[3] = (3.0 * tabData->calcData_Compression[0] * L_0) / (2.0 * W_0 * (T_0 * T_0));

    // -------------------------------------------------------------------------
    // 3-Point Bending Flexural Modulus (E-Modulus) using Linear Regression
    // -------------------------------------------------------------------------
    if (tabData->LR10KTensileData.Data.Size > 10)
    {
        // 1. Default limits for the linear region (in raw Extension/Travel)
        double ext_lower = 0.5;
        double ext_upper = 2.0;

        // 2. See if the user has added custom draggable bounds (ID 5 and 6)
        for (const auto& point_tuple : tabData->plot_dataPoints) {
            // Because we are in bending mode, the graph is just Load/Extension. 
            // So we grab the raw X (Extension) directly!
            if (std::get<1>(point_tuple) == 5) ext_lower = std::get<0>(point_tuple).x;
            if (std::get<1>(point_tuple) == 6) ext_upper = std::get<0>(point_tuple).x;
        }

        // 3. Extract raw Load and Extension arrays for regression
        std::vector<double> extensionData, loadData;
        extensionData.reserve(tabData->LR10KTensileData.Data.Size);
        loadData.reserve(tabData->LR10KTensileData.Data.Size);

        for (int i = 0; i < tabData->LR10KTensileData.Data.Size; i++) {
            extensionData.push_back(tabData->LR10KTensileData.Data[i].x); // Travel
            loadData.push_back(tabData->LR10KTensileData.Data[i].y);      // Load
        }

        // 4. Reuse your existing regression function to get the exact slope (m = F / dL)
        double slope = CalculateYoungsModulus(extensionData, loadData, ext_lower, ext_upper);

        // 5. Safely apply the 3-point bending formula: E = (slope * L^3) / (4 * w * t^3)
        // (Re-using the safe L_0, W_0, and T_0 variables from the top of the function)
        double denominator = 4.0 * W_0 * pow(T_0, 3);

        tabData->calcYoungsModulus = (slope * pow(L_0, 3)) / denominator;
    }

    // -------------------------------------------------------------------------
    // ------------------ Save any calculations to current tab -----------------
    // -------------------------------------------------------------------------
    if (playSound)
        PlaySoundEffect("buttonClick1");
}

// Sets the different global material properties
void AppCalculator::SetLocalMaterialProperties(TabControl::TabTestDataClass* tabData, int areaOption, double thickness, double width, double length, double diameter)
{
    // Save values locally
    tabData->areaOptionPicked   = areaOption;
    tabData->thickness          = thickness;
    tabData->width              = width;
    tabData->length             = length;
    tabData->diameter           = diameter;

    // Calculate area
    if (areaOption == 0)
        tabData->area = thickness * width;
    else if (areaOption == 1)
        tabData->area = (M_PI * diameter) / 4;
}

// Sets the different global material properties
void AppCalculator::SetGlobalMaterialProperties(TabControl::TabTestDataClass* tabData, int areaOption, double thickness, double width, double length, double diameter)
{
    // Save values globally
    LR10KAppGlobals::areaOptionPicked = areaOption;
    LR10KAppGlobals::calc_thickness = thickness;
    LR10KAppGlobals::calc_width = width;
    LR10KAppGlobals::calc_length = length;
    LR10KAppGlobals::calc_diameter = diameter;

    DockingApps::mainFileSystem.SaveApplicationSettings();
}

void AppCalculator::AddEmoduleBounds(TabControl::TabTestDataClass* tabData)
{
    // Make sure we have enough data to drop the points, and that they don't already exist
    bool bounds_exist = false;
    for (const auto& pt : tabData->plot_dataPoints) {
        if (std::get<1>(pt) == 5 || std::get<1>(pt) == 6) bounds_exist = true;
    }

    if (!bounds_exist && tabData->LR10KTensileData.Data.Size > 50)
    {
        // Drop them at early index points (e.g., 10 and 50) as a starting guess
        ImVec2 p1 = tabData->LR10KTensileData.Data[10];
        ImVec2 p2 = tabData->LR10KTensileData.Data[50];

        tabData->plot_dataPoints.push_back({ ImPlotPoint(p1.x, p1.y), 5 }); // ID 5
        tabData->plot_dataPoints.push_back({ ImPlotPoint(p2.x, p2.y), 6 }); // ID 6

        tabData->NeedsRecalculation.markAsUnsaved = true;
        tabData->NeedsRecalculation.recalculate = true;
    }
}

void CreateInputDoubleElement(TabControl::TabTestDataClass* tabData, const char *label, double *v, double min_val, double max_val)
{
    if (ImGui::InputDouble(label, v, (double)LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(*v), (double)(0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(*v), ImGuiInputTextFlags_CharsScientific))
    {
        // Clamp the value instantly if the user types something out of bounds
        if (*v < min_val) *v = min_val;
        if (*v > max_val) *v = max_val;

        tabData->NeedsRecalculation.markAsUnsaved = true;
        tabData->NeedsRecalculation.recalculate = true;
    }
    ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
}

void CreateInputFloatElement(TabControl::TabTestDataClass* tabData, const char* label, float* v, float min_val, float max_val)
{
    if (ImGui::InputFloat(label, v, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(*v), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(*v), ImGuiInputTextFlags_CharsScientific))
    {
        // Clamp the value instantly if the user types something out of bounds
        if (*v < min_val) *v = min_val;
        if (*v > max_val) *v = max_val;

        tabData->NeedsRecalculation.markAsUnsaved = true;
        tabData->NeedsRecalculation.recalculate = true;
    }
    ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
}

void DrawLegendChip(ImVec4 color, float indent = 8.0f) {
    //ImVec4 col = ImPlot::GetColormapColor(index);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
    ImVec2 p = ImGui::GetCursorScreenPos();
    // Draw a small 12x12 rounded square
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 12, p.y + 12), ImGui::ColorConvertFloat4ToU32(color), 3.0f);
    ImGui::Dummy(ImVec2(16, 12)); // Move cursor past the chip
    ImGui::SameLine();
}

// The Core Helper (Does the actual drawing)
void TextWithBackground_Core(ImVec4 textColor, ImColor bcolor, const char* fmt, va_list args) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(buf);
    ImVec2 padding = ImVec2(5.0f, 3.0f);

    // Calculate Box Size
    ImVec2 rect_max = ImVec2(pos.x + textSize.x + padding.x * 2.0f, pos.y + textSize.y + padding.y * 2.0f);

    // Draw White Box
    draw_list->AddRectFilled(pos, rect_max, bcolor, 4.0f);

    // Draw Text (Shifted by padding)
    ImGui::SetCursorScreenPos(ImVec2(pos.x + padding.x, pos.y + padding.y));
    ImGui::TextColored(textColor, "%s", buf);

    // Reset Cursor for the next UI element
    ImGui::SetCursorScreenPos(ImVec2(pos.x, rect_max.y + ImGui::GetStyle().ItemSpacing.y));
}

// Explicit Background Color
void TextWithBackground(ImVec4 textColor, ImColor bcolor, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextWithBackground_Core(textColor, bcolor, fmt, args);
    va_end(args);
}
// Default White Background (Omits the bcolor parameter)
void TextWithWhiteBackground(ImVec4 textColor, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextWithBackground_Core(textColor, IM_COL32(255, 255, 255, 220), fmt, args);
    va_end(args);
}

void AppCalculator::Draw(TabControl::TabTestDataClass* tabData, bool* showCalculator, ImGuiWindowFlags extern_windowFlags)
{
    if (tabData == nullptr) return;

    ImGuiWindowFlags window_flags = 0;
    window_flags |= extern_windowFlags;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(680, 870), ImGuiCond_Once);

    ImGui::Begin("Calculator", showCalculator, window_flags);

    ImGui::Spacing();

    // ------------------------------------------------------------------
    // TABS
    // ------------------------------------------------------------------
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
    tab_bar_flags |= ImGuiTabBarFlags_TabListPopupButton;
    
    if (ImGui::BeginTabBar("CalculatorTabs", tab_bar_flags))
    {
        double fracturePointExtension = tabData->GetFracturePoint(tabData).x;

        // --------------------
        // MATERIAL PROP.
        // --------------------
        /*float static check_Thickness = -1.0f;
        float static check_Width = -1.0f;
        float static check_Length = -1.0f;
        float static check_Diameter = -1.0f;
        int static   check_areaOption = LR10KAppGlobals::areaOptionPicked;
        float static check_offsetZeroPoint = defaultValuesGUI::offsetZeroPointExtensionAxis;*/

        // Warning if any values have changed but no calculated again
        static ImVec4 colorNormal = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        static ImVec4 colorNormal2 = ImVec4(0.695f, 1.000f, 0.442f, 0.784f);
        static ImVec4 colorNormal3 = ImVec4(0.647f, 1.000f, 0.780f, 0.784f);
        static ImVec4 colorWarning = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

        // Used for local (green) or global colors (orange)
        ImVec4 greenColor = ImColor::HSV(1.38f, 0.8f, 1.0f);
        ImVec4 greenColor2 = ImColor::HSV(1.38f, 0.8f, 0.5f);
        ImVec4 orangeColor = ImColor::HSV(2.1f, 0.8f, 1.0f);
        ImVec4 orangeColor2 = ImColor::HSV(2.1f, 0.8f, 0.5f);

        // First Curve (Red-ish)
        const ImVec4 IMPLOT_RED = ImVec4(0.894f, 0.102f, 0.110f, 1.0f);
        // Second Curve (Blue-ish)
        const ImVec4 IMPLOT_BLUE = ImVec4(0.216f, 0.494f, 0.722f, 1.0f);

        ImVec4 colorText = colorNormal;
        ImVec4 colorText_nom = IMPLOT_RED;//ImPlot::GetColormapColor(1);//orangeColor;//colorNormal3;
        ImVec4 colorText_true = IMPLOT_BLUE;//ImPlot::GetColormapColor(0);
        ImVec4 colorText2 = colorNormal2;

        // -------------------------------------------------------------------------
        // -------------------------- Material properties --------------------------
        // -------------------------------------------------------------------------
        if (ImGui::BeginTabItem("Material Properties"))
        {
            // --------------------------------
            // --- Buttons
            // --------------------------------
            ImGui::SeparatorText("Configuration");

            // If the user wants to insert parameters from loaded file
            // Load materials
            if (ImGui::BeginChild("##Load material properties Button", ImVec2(210, 110)))
            {
                ImGui::Text("Load global material\nproperties into local\nvalues.");

                ImGui::PushStyleColor(ImGuiCol_Button, greenColor2);
                if (ImGui::Button("Set local values"))
                {
                    // Update the local material properties from the global values
                    DockingApps::calculator.SetLocalMaterialProperties(tabData, LR10KAppGlobals::areaOptionPicked, LR10KAppGlobals::calc_thickness, LR10KAppGlobals::calc_width, LR10KAppGlobals::calc_length, LR10KAppGlobals::calc_diameter);
                    tabData->NeedsRecalculation.markAsUnsaved = true;
                    tabData->NeedsRecalculation.recalculate = true;
                }
                ImGui::PopStyleColor();
                ImGui::SetItemTooltip("Load global material properties into\nthe local values (this test).");
            }
            ImGui::EndChild();

            // Set material properties
            ImGui::SameLine();
            if (ImGui::BeginChild("##Set material properties Button", ImVec2(190, 110)))
            {
                ImGui::Text("Set global material\nproperties from local\nvalues.");

                ImGui::PushStyleColor(ImGuiCol_Button, orangeColor2);
                if (ImGui::Button("Set global values"))
                {
                    // Update the global material properties from the local values
                    DockingApps::calculator.SetGlobalMaterialProperties(tabData, LR10KAppGlobals::areaOptionPicked, tabData->thickness, tabData->width, tabData->length, tabData->diameter);
                }
                ImGui::PopStyleColor();
                ImGui::SetItemTooltip("Insert and save global material\nproperties from the local values (this test).");
            }
            ImGui::EndChild();

            // Checkbox
            ImGui::SameLine();
            if (ImGui::BeginChild("##Load from global values Checkbox", ImVec2(160, 110)))
            {
                ImGui::Text("Always load from\nglobal values");

                if (ImGui::Checkbox("##Load from global values", &LR10KAppGlobals::useGlobalValuesOnNewTests))
                {
                    DockingApps::mainFileSystem.SaveApplicationSettings();
                }
                ImGui::SetItemTooltip("If you want new tests to automatically\nload from global values.");
            }
            ImGui::EndChild();

            // --------------------------------
            // --- Input
            // --------------------------------

            // Cross sectional area
            ImGui::SeparatorText("Cross-sectional area selection");
            ImGui::PushFont(Extra::fonts.jetbrainsFont);
            if (ImGui::RadioButton("Rectangular", &LR10KAppGlobals::areaOptionPicked, 0))
            {
                // Must be the same for local and global...
                if (tabData->areaOptionPicked != LR10KAppGlobals::areaOptionPicked)
                {
                    tabData->areaOptionPicked = LR10KAppGlobals::areaOptionPicked;
                    tabData->MarkTabAsUnsaved();
                }
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Circular", &LR10KAppGlobals::areaOptionPicked, 1))
            {
                // Must be the same for local and global...
                if (tabData->areaOptionPicked != LR10KAppGlobals::areaOptionPicked)
                {
                    tabData->areaOptionPicked = LR10KAppGlobals::areaOptionPicked;
                    tabData->MarkTabAsUnsaved();
                }
                DockingApps::mainFileSystem.SaveApplicationSettings();
            }

            if (!std::isnan(tabData->area) && tabData->area > -1)
            {
                std::string str = "(local area calculated: %.2f ";
                str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
                str.append("^2)");
                ImGui::Text(str.c_str(), tabData->area);
            }

            // Local values (start values)
            ImGui::Spacing();

            ImGui::BeginDisabled(tabData->test_unfinished_test_started);

            ImGui::BeginDisabled(LR10KAppGlobals::useGlobalValuesOnNewTests);
            ImGui::PushStyleColor(ImGuiCol_Separator, greenColor);
            ImGui::SeparatorText("Local Values | Material properties");
            ImGui::PopStyleColor();

            if (tabData->areaOptionPicked == 0)
            {
                ImGui::Text("Gauge Thickness (σ_n, σ_t):");
                CreateInputDoubleElement(tabData, "##Gauge thickness", &tabData->thickness, 0.01, 300);

                ImGui::Text("Gauge Width (σ_n, σ_t):");
                CreateInputDoubleElement(tabData, "##Gauge width", &tabData->width, 0.01, 500);
            }
            else if (tabData->areaOptionPicked == 1)
            {
                ImGui::Text("Gauge Diameter (D0) (σ_n, σ_t):");
                CreateInputDoubleElement(tabData, "##Diameter", &tabData->diameter, 0.01, 800);
            }

            ImGui::Text("Gauge Length (L0) (ε_n, σ_t, ε_t):");
            CreateInputDoubleElement(tabData, "##Gauge length", &tabData->length, 0.01, 1000);
            ImGui::EndDisabled();

            // Global values
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, orangeColor);
            ImGui::SeparatorText("Global Values | Material properties");
            ImGui::PopStyleColor();

            if (LR10KAppGlobals::areaOptionPicked == 0)
            {
                ImGui::Text("Global Gauge Thickness (σ_n, σ_t):");
                CreateInputFloatElement(tabData, "##Global thickness", &LR10KAppGlobals::calc_thickness, 0.01f, 300.0f);

                ImGui::Text("Global Gauge Width (σ_n, σ_t):");
                CreateInputFloatElement(tabData, "##Global width", &LR10KAppGlobals::calc_width, 0.01f, 800.0f);
            }
            else if (LR10KAppGlobals::areaOptionPicked == 1)
            {
                ImGui::Text("Global Gauge Diameter (σ_n, σ_t):");
                CreateInputFloatElement(tabData, "##Global Diameter", &LR10KAppGlobals::calc_diameter, 0.01f, 500.0f);
            }

            ImGui::Text("Global Gauge Length (L0) (ε_n, σ_t, ε_t):");
            CreateInputFloatElement(tabData, "##Global Gauge length", &LR10KAppGlobals::calc_length, 0.01f, 1000.0f);
            ImGui::PopFont();

            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }
        // --------------------
        // Stress (N/MM^2 | MPa | Sigma)
        // --------------------
        if (ImGui::BeginTabItem("Stress Calculation"))
        {
            ImGuiStyle& style = ImGui::GetStyle();
            static float scaling_header = 1.5f;
            static float scaling = 1.3f;

            // Output the calculations
            std::string str = "Stress (";
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
            str.append(" | ");
            str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
            str.append(" | σ)");
            ImGui::SeparatorText(str.c_str());

            ImGui::Spacing();
            ImGui::PushFont(NULL, style.FontSizeBase* scaling_header);
            ImGui::Text("Different calculated stress underneath");
            ImGui::PopFont();

            DrawLegendChip(IMPLOT_RED);
            ImGui::Text("Engineering Stress");
            DrawLegendChip(IMPLOT_BLUE);
            ImGui::Text("True Stress");

            ImGui::Spacing();

            ImGui::BulletText("Calculations will be saved in the Excel Books,\nalongside with the graph.");

            ImGui::Spacing();

            // If it is tensile or compression
            bool condition1 = tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked); // Normal tension or compression
            bool condition2 = !tabData->tension && 1 == tabData->compressionOptionPicked; // 3-point compression

            static std::string str1 = "";
            static std::string str2 = "";
            static std::string str3 = "";

            if (ImGui::BeginTabBar("StressCalulationTabs", tab_bar_flags))
            {
                // ---------------------------- Tension
                if (condition1 && ImGui::BeginTabItem("Proportional Limit"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase* scaling);
                    ImGui::Text("Proportional Limit");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Engineering stress (nominal stress)");
                    str1 = "Stress, σ_nom/σ_e: %0.3f ";
                    str1.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
                    TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_PropLimit[1]);
                    TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_PropLimit[0] * 100);
                    ImGui::SeparatorText("True stress");
                    str3 = "Stress, σ_t: %0.3f ";
                    str3.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
                    TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_PropLimit[1]);
                    TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_PropLimit[0] * 100);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                if (condition1 && ImGui::BeginTabItem("Yielding"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Yielding Strength (lower)");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Engineering stress (nominal stress)");
                    TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_yield[1]);
                    TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_yield[0] * 100);
                    ImGui::SeparatorText("True stress");
                    TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_yield[1]);
                    TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_yield[0] * 100);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                if (condition1 && ImGui::BeginTabItem("UTS"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Ultimate Tensile Strength (UTS)");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Engineering stress (nominal stress)");
                    TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrainUTS[1]);
                    TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrainUTS[0] * 100);
                    ImGui::SeparatorText("True stress");
                    TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrainUTS[1]);
                    TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrainUTS[0] * 100);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                if (condition1 && ImGui::BeginTabItem("Fracture"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Fracture");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Engineering stress (nominal stress)");
                    TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_fracture[1]);
                    TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_fracture[0] * 100);
                    ImGui::SeparatorText("True stress");
                    TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_fracture[1]);
                    TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_fracture[0] * 100);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }

                // ------------------------------ Compression

                if (condition2 && ImGui::BeginTabItem("UTS"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase* scaling);
                    ImGui::Text("Ultimate Tensile Strength (UTS)");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("UTS Bending");
                    str = "Ultimate Tensile Strength: %0.2f ";
                    str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
                    TextWithWhiteBackground(colorText_nom, str.c_str(), tabData->calcData_Compression[3]);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                if (condition2 && ImGui::BeginTabItem("Max. Bending Moment"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Maximum Bending Moment");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Bending Moment");
                    str = "Max. Moment (Force*Arm): %0.2f ";
                    str.append(LR10KAppGlobals::GetUnitOption<const char*>(1)).append(LR10KAppGlobals::GetUnitOption<const char*>(0)); // Nmm or lbf * in
                    ImGui::TextColored(colorText, str.c_str(), tabData->calcData_Compression[2]);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                if (condition2 && ImGui::BeginTabItem("Max. Load & Travel"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Maximum Load & Travel");
                    ImGui::Separator();
                    ImGui::PopFont();

                    ImGui::SeparatorText("Bending Load");
                    std::string str = "Max. Load: %0.2f ";
                    str.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
                    ImGui::TextColored(colorText, str.c_str(), tabData->calcData_Compression[0]);

                    ImGui::SeparatorText("Bending Travel");
                    str = "Max. Travel: %0.2f ";
                    str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
                    ImGui::TextColored(colorText, str.c_str(), tabData->calcData_Compression[1]);

                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }

                // ----------------- Tension or Compression

                if (ImGui::BeginTabItem("E-Modulus"))
                {
                    ImGui::PushFont(Extra::fonts.jetbrainsFont);
                    ImGui::PushFont(NULL, style.FontSizeBase * scaling);
                    ImGui::Text("Young's Modulus (E-Modulus)");
                    ImGui::PopFont();

                    if (ImGui::Button("Add custom E-Modulus Bounds"))
                        DockingApps::calculator.AddEmoduleBounds(tabData);
                    ImGui::Separator();

                    ImGui::SeparatorText("Based on engineering stress (nominal stress)");

                    // 1. Default percentages (already represented as % for the UI)
                    double lowerEPerc = 0.05;
                    double upperEPerc = 0.25;
                    bool foundLowerE = false;
                    bool foundUpperE = false;

                    // 1. Set the limits so the user can't accidentally cross the values
                    double min_strain = 0.0;
                    double max_strain = 100.0; // Or whatever your max reasonable strain is
                    float drag_speed = 0.005f; // Makes it adjust slowly for fine-tuning

                    // ---------

                    // 2. Use a const reference to avoid copying the entire vector!
                    const auto& pointData = tabData->plot_dataPoints;

                    // Safely get L_0 for calculating actual strain from the raw extension
                    double L_0 = tabData->length > 0 ? tabData->length : 1.0;

                    for (size_t i = 0; i < pointData.size(); i++)
                    {
                        // Use std::get<0> and <1> for safe tuple extraction
                        ImPlotPoint point = std::get<0>(pointData[i]);
                        int id = std::get<1>(pointData[i]);

                        // Lower E
                        if (id == 5)
                        {
                            foundLowerE = true;
                            // Strain Percentage = (Extension / Original Length) * 100
                            lowerEPerc = (point.x / L_0) * 100.0;
                        }
                        // Upper E
                        else if (id == 6)
                        {
                            foundUpperE = true;
                            upperEPerc = (point.x / L_0) * 100.0;
                        }
                    }

                    static size_t firstTabIDCheck = tabData->tabId;
                    size_t secondTabIDCheck = tabData->tabId;
                    static double dragSliderLowerEPerc = foundLowerE ? lowerEPerc : 0.05; // In %
                    static double dragSliderUpperEPerc = foundUpperE ? upperEPerc : 0.25; // In %

                    // If user switches tab, adjust the slider
                    if (firstTabIDCheck != secondTabIDCheck)
                    {
                        dragSliderLowerEPerc = foundLowerE ? lowerEPerc : 0.05;
                        dragSliderUpperEPerc = foundUpperE ? upperEPerc : 0.25;

                        firstTabIDCheck = tabData->tabId;
                    }
                    // If no custom point is set, reset the slider to default
                    if (!foundLowerE)
                    {
                        dragSliderLowerEPerc = 0.05;

                        // Make sure upper E must be a higher value than lower E
                        if (dragSliderUpperEPerc < dragSliderLowerEPerc) dragSliderUpperEPerc = dragSliderLowerEPerc;
                    }
                    if (!foundUpperE)
                    {
                        dragSliderUpperEPerc = 0.25;

                        // Make sure lower E must be a lower value than lower E
                        if (dragSliderLowerEPerc > dragSliderUpperEPerc) dragSliderLowerEPerc = dragSliderUpperEPerc;
                    }

                    // ---------------------------------------------------------------

                    ImGui::SeparatorText("E-Modulus Calculation Range");

                    // Give the user a nice visual hint if they are using custom bounds vs standard
                    if (!foundLowerE || !foundUpperE)
                    {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "according to ASTM D638 / ISO 527 defaults.");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "using your custom dragged boundaries.");

                        // Add the reset button exactly here!
                        ImGui::SameLine();
                        if (ImGui::Button("Reset to Default boundaries"))
                        {
                            // 1. Calculate the exact physical extension for 0.05% and 0.25% strain
                            double target_lower_ext = 0.0005 * L_0;
                            double target_upper_ext = 0.0025 * L_0;

                            // 2. Loop through the points and modify them directly (notice the '&' so we change the real data)
                            for (auto& pt_tuple : tabData->plot_dataPoints)
                            {
                                ImPlotPoint& pt = std::get<0>(pt_tuple);
                                int id = std::get<1>(pt_tuple);

                                if (id == 5 || id == 6)
                                {
                                    // Pick the target extension based on which point this is
                                    double target_x = (id == 5) ? target_lower_ext : target_upper_ext;

                                    // Snap it perfectly to the closest real data point on the graph line!
                                    int snap_idx = FindClosestRawDataIndex(tabData->LR10KTensileData.Data, target_x);
                                    if (snap_idx != -1)
                                    {
                                        pt.x = tabData->LR10KTensileData.Data[snap_idx].x;
                                        pt.y = tabData->LR10KTensileData.Data[snap_idx].y;

                                        // Reset slider
                                        if (id == 5) dragSliderLowerEPerc = (pt.x / L_0) * 100.0;
                                        if (id == 6) dragSliderUpperEPerc = (pt.x / L_0) * 100.0;
                                    }
                                }
                            }

                            // 3. Trigger the app to recalculate the Young's Modulus instantly
                            tabData->NeedsRecalculation.markAsUnsaved = true;
                            tabData->NeedsRecalculation.recalculate = true;

                            // (Optional) Play your standard UI click sound!
                            PlaySoundEffect("buttonClick0");
                        }
                        if (std::round(lowerEPerc * 100.0) / 100 == 0.05 && std::round(upperEPerc * 100.0) / 100 == 0.25)
                        {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "According to ASTM D638 / ISO 527 defaults.");
                        }
                    }

                    // ---------------------------------------------------------------

                    // 2. Interactive Drag inputs
                    // Notice how the 'max' for the Lower input is the Upper input, 
                    // and the 'min' for the Upper input is the Lower input. This prevents them from crossing!
                    ImGui::BeginDisabled(!foundLowerE);
                    if (ImGui::DragScalar("Start Strain (%)", ImGuiDataType_Double, &dragSliderLowerEPerc, drag_speed, &min_strain, &dragSliderUpperEPerc, "%.3f %%"))
                    {
                        // 1. Convert the % Strain back to raw Extension (X value)
                        double target_x = (dragSliderLowerEPerc / 100.0) * L_0;

                        // 2. Snap to the closest real data point
                        int snap_idx = FindClosestRawDataIndex(tabData->LR10KTensileData.Data, target_x);
                        if (snap_idx != -1)
                        {
                            // 3. Find the plot point with ID == 5 and update it
                            for (auto& pointTuple : tabData->plot_dataPoints)
                            {
                                if (std::get<1>(pointTuple) == 5) // ID 5 = Lower E
                                {
                                    std::get<0>(pointTuple).x = tabData->LR10KTensileData.Data[snap_idx].x;
                                    std::get<0>(pointTuple).y = tabData->LR10KTensileData.Data[snap_idx].y;
                                    break; // Stop searching once found
                                }
                            }
                        }
                    }
                    ImGui::EndDisabled();

                    ImGui::BeginDisabled(!foundUpperE);
                    if (ImGui::DragScalar("End Strain (%)", ImGuiDataType_Double, &dragSliderUpperEPerc, drag_speed, &dragSliderLowerEPerc, &max_strain, "%.3f %%"))
                    {
                        // 1. Convert the % Strain back to raw Extension (X value)
                        double target_x = (dragSliderUpperEPerc / 100.0) * L_0;

                        // 2. Snap to the closest real data point
                        int snap_idx = FindClosestRawDataIndex(tabData->LR10KTensileData.Data, target_x);
                        if (snap_idx != -1)
                        {
                            // 3. Find the plot point with ID == 6 and update it
                            for (auto& pointTuple : tabData->plot_dataPoints)
                            {
                                if (std::get<1>(pointTuple) == 6) // ID 6 = Upper E
                                {
                                    std::get<0>(pointTuple).x = tabData->LR10KTensileData.Data[snap_idx].x;
                                    std::get<0>(pointTuple).y = tabData->LR10KTensileData.Data[snap_idx].y;
                                    break; // Stop searching once found
                                }
                            }
                        }
                    }
                    ImGui::EndDisabled();

                    // 3. Draw the dynamic text using ImGui's native formatting!
                    ImGui::Text("Strain range set to %.2f %% - %.2f %%,", dragSliderLowerEPerc, dragSliderUpperEPerc);

                    // ---------------------------------------------------------------

                    ImGui::Spacing();
                    std::string str2 = "Young's modulus, E-Modulus: %0.3f ";

                    if (LR10KAppGlobals::unitsConvertionIndex == 0)
                        str2.append("GPa");
                    else
                        str2.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
                    TextWithBackground(colorText2, IM_COL32(52, 52, 52, 250), str2.c_str(), (double)tabData->calcYoungsModulus);
                    ImGui::Spacing();

                    ImGui::PopFont();
                    ImGui::EndTabItem();

                }
            }

            ImGui::EndTabBar();
            ImGui::EndTabItem();
        }
        // --------------------
        // FILTER
        // --------------------
        if (ImGui::BeginTabItem("Filters"))
        {
            ImGui::SeparatorText("Filter out extension from start (without extensiometer)");

            ImGui::PushFont(Extra::fonts.jetbrainsFont);
            // Have the option to move the starting strain point (x-axis) an offset distance from zero point
            ImGui::Text("Remove some slack from\nthe extension/strain axis:");
            ImGui::Text("0 mm = \"Default setup\".");
            ImGui::Text("Will only affect the\nresult for calculation(s).");
            ImGui::Text("* Fracture point is needed.");

            ImGui::BeginDisabled(fracturePointExtension <= 0 && !mainGUIGlobals::useDemoMode);
            if (ImGui::InputFloat("##Offset zero point ext.", &defaultValuesGUI::offsetZeroPointExtensionAxis, (0.01f), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(defaultValuesGUI::offsetZeroPointExtensionAxis), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsScientific))
            {
                // Will only be active if a fracture point is added
                if (!mainGUIGlobals::useDemoMode)
                {
                    if (defaultValuesGUI::offsetZeroPointExtensionAxis < 0)
                        defaultValuesGUI::offsetZeroPointExtensionAxis = 0;
                    else if (defaultValuesGUI::offsetZeroPointExtensionAxis > fracturePointExtension)
                        defaultValuesGUI::offsetZeroPointExtensionAxis = fracturePointExtension;
                }
                else
                {
                    if (defaultValuesGUI::offsetZeroPointExtensionAxis < 0)
                        defaultValuesGUI::offsetZeroPointExtensionAxis = 0;
                    else if (defaultValuesGUI::offsetZeroPointExtensionAxis > defaultValuesGUI::extensionAxis)
                        defaultValuesGUI::offsetZeroPointExtensionAxis = defaultValuesGUI::extensionAxis;
                }
                // Save the current GUI value to tab
                tabData->LR10OffsetZeroPointExtensionAxis = defaultValuesGUI::offsetZeroPointExtensionAxis;

                // Recalculate
                tabData->NeedsRecalculation.markAsUnsaved = true;
                tabData->NeedsRecalculation.recalculate = true;
            }

            ImGui::SameLine();
            ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));

            if (LR10KAppGlobals::showStressStrain && tabData != nullptr)
            {
                double L_0 = tabData->length > 1.0 ? tabData->length : 1.0;
                ImGui::Text("(Offset in percentage: %.2f %%)", (defaultValuesGUI::offsetZeroPointExtensionAxis / L_0)*100);
            }

            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::PopFont();

            ImGui::EndTabItem();
        }
        // --------------------
        // EXTRA TOOLS
        // --------------------
        if (ImGui::BeginTabItem("Extra Tools"))
        {
            ImGui::SeparatorText("Calculate a percentage of the materials start length");

            ImGui::PushFont(Extra::fonts.jetbrainsFont);
            // Calculate percentage of length of material to millimeter
            ImGui::Text("Percentage of the Gauge Length:");
            static float percentageOfLength = 0.00f;
            if (ImGui::InputFloat("##Percentage of length1", &percentageOfLength, (0.1f), (0.5f), "%.2f", ImGuiInputTextFlags_CharsScientific))
            {
                if (percentageOfLength < 0.0f)
                    percentageOfLength = 0.0f;
                else if (percentageOfLength > 100.0f)
                    percentageOfLength = 100.0f;
            }
            ImGui::SameLine(); ImGui::Text("%%");
            float percentageOfLengthOutput = LR10KAppGlobals::calc_length * (percentageOfLength / 100);
            ImGui::InputFloat("##Percentage of length2", &percentageOfLengthOutput, NULL, NULL, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(percentageOfLengthOutput), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
            ImGui::SetItemTooltip("(Read only)");

            ImGui::Spacing();
            ImGui::PopFont();

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();

    ImGui::Spacing();
    ImGui::End();
}

// -----------------------------------------------------------------------------
// ----------------- For calculating a propotional limit point -----------------
// -----------------------------------------------------------------------------

/**
    @brief  Function to calculate the mean of a vector of values
    @param  data   - 
    @param  member - 
    @retval        - 
**/
double calculateMean(const std::vector<DataPoint>& data, double(DataPoint::* member)) {
    double sum = 0.0;
    for (const DataPoint& point : data) {
        sum += point.*member;
    }
    return sum / data.size();
}

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

    // Safe denominators for math
    double A_0 = tabData.area > 0 ? tabData.area : 1.0;
    double L_0 = tabData.length > 0 ? tabData.length : 1.0;

    for (size_t row = 0; row < tensileData.Data.size(); row++) {
        tensileData.Data[row].x -= (float)defaultValuesGUI::offsetZeroPointExtensionAxis;

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
            // True stress
            dataPlotPointStress.x = LR10KAppGlobals::CalcEpsilon_t(dataPlotPointStress.x);
            dataPlotPointStress.y = LR10KAppGlobals::CalcSigma_t(dataPlotPointStress.x, dataPlotPointStress.y);
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

                if (dataVariation == VAR_ENG_STRESS)
                {
                    // Nominal stress
                    dataStress.x = LR10KAppGlobals::CalcEpsilon_e(dataStress.x, L_0);
                    dataStress.y = LR10KAppGlobals::CalcSigma_e(dataStress.y, A_0);
                }

                // A matching point the crucial point is "snapped" to
                if (dataRaw.x == dataPlotPointRaw.x)
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

        worksheet_write_string(ws_report, 0, 0, "MATERIAL TESTING REPORT", fmt_header);
        worksheet_merge_range(ws_report, 0, 0, 0, 5, "MATERIAL TESTING REPORT", fmt_header);
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

        worksheet_write_string(ws_report, CELL("A11"), "Young's Moduls, E-Modulus (GPa)", NULL);
        worksheet_write_number(ws_report, CELL("B11"), tabData.calcYoungsModulus, fmt_decimal3);
        worksheet_write_string(ws_report, CELL("C11"), "GPa", fmt_bold_left);

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
    }

    // ---------------------------------------------------------
    // B. SETUP SHEET
    // ---------------------------------------------------------
    lxw_worksheet* ws_setup = workbook_add_worksheet(workbook, "Setup");
    worksheet_set_column(ws_setup, 0, 3, 27, NULL); //27px
    worksheet_write_string(ws_setup, 0, 0, "LR 10K SETTINGS", fmt_header);
    worksheet_merge_range(ws_setup, 0, 0, 0, 2, "LR 10K SETTINGS", fmt_header);

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

// -----------------------------------------------------------------------------
// ---------------------------- File Browsing Logic ----------------------------
// -----------------------------------------------------------------------------

void AppMainFileSystem::SaveAsLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveAsLR10KTest.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string filename = fileBrowserSaveAsLR10KTest.GetSelected().string();
        fileBrowserSaveAsLR10KTest.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used

        // Make a independent copy for saving to a file
        TabControl::TabTestDataClass tabDataForSave = *tabData;
        tabData->ConvertUnitsToNewUnit(&tabDataForSave, 0, false, false, false);
        
        // Create JSON data and save it
        json jsonData = DockingApps::mainFileSystem.CreateJSONDataFromCurrentTab(&tabDataForSave);
        bool save = DockingApps::mainFileSystem.SaveJSONDataToFile(tabData, jsonData, filename.c_str());

        if (save)
            DockingApps::console.AddLog("Successfully saved plot data from current tab!");
        else
            DockingApps::console.AddLog("[error] Could not save plot data from current tab...");
    }
}

void AppMainFileSystem::PreSaveLR10KTestBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserPreSaveLR10KTest.HasSelected())
    {
        // Get current selection (path)
        const std::string foldername = fileBrowserPreSaveLR10KTest.GetSelected().string();
        fileBrowserPreSaveLR10KTest.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used
        
        // Store pre-save location
        tabData->current_temp_preSaveLocation = foldername;

        DockingApps::console.AddLog("Successfully set pre-save location for current tab!");
        PlaySoundEffect("buttonClick1");
    }
}

void AppMainFileSystem::SaveXlsxBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveXlsx.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string filename = fileBrowserSaveXlsx.GetSelected().string();
        fileBrowserSaveXlsx.ClearSelected();

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used
        DockingApps::mainExcel.SavePlotDataAsAnXlsxFile(*tabData, filename.c_str()); // Returns its own console log on success/non success
    }
}

void AppMainFileSystem::OpenLR10KTestFileBrowserLogic()
{
    // Check if either the browser or the drag-and-drop triggered
    if (!LR10KAppGlobals::pendingFileToLoad.empty() || fileBrowserOpenLR10KTest.HasSelected())
    {
        std::string filename;

        // 1. Determine the source of the file path
        if (!LR10KAppGlobals::pendingFileToLoad.empty()) {
            filename = LR10KAppGlobals::pendingFileToLoad;
            LR10KAppGlobals::pendingFileToLoad = ""; // Clear immediately

            // Sync the browser folder location so it's ready for the next "Open" click
            fileBrowserOpenLR10KTest.SetPwd(std::filesystem::path(filename).parent_path());
        }
        else {
            filename = fileBrowserOpenLR10KTest.GetSelected().string();
            fileBrowserOpenLR10KTest.ClearSelected();
        }

        // 2. Save the last folder location
        DockingApps::mainFileSystem.SaveApplicationFolderSettingsOnly();

        // 3. Load from JSON
        TabControl::TabTestDataClass* tabDataLoaded = DockingApps::mainFileSystem.LoadJSONDataFromFileForANewTab(filename.c_str());
        tabDataLoaded->loadedFromFileName = filename;

        // 4. Sleek Path Logic: Use std::filesystem to get the folder path
        // This replaces the stringstream/seglist loop entirely!
        std::filesystem::path p(filename);
        tabDataLoaded->current_temp_preSaveLocation = p.parent_path().string() + "\\";

        // 5. Validation and Tab Creation (Rest of your existing code)
        if (tabDataLoaded->isValid())
        {
            DockingApps::mainTabSystem.CreateANewTab(tabDataLoaded);

            if ((tabDataLoaded->thickness > 0 && tabDataLoaded->width > 0) || tabDataLoaded->length > 0)
                tabDataLoaded->ConvertUnitsToNewUnitAfterLoad(tabDataLoaded);

            PlaySoundEffect("saveFileSuccess");
            tabDataLoaded->request_graph_autofit = true;
            DockingApps::console.AddLog("Successfully loaded plot data to a new tab!");
        }
        else {
            DockingApps::console.AddLog("[error] Could not load plot data to new tab...");
        }
    }
}

void AppMainFileSystem::PreSaveVideoRecordingExtBrowserLogic(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (fileBrowserSaveVideoRecordingExt.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string folderlocation = fileBrowserSaveVideoRecordingExt.GetSelected().string();
        fileBrowserSaveVideoRecordingExt.ClearSelected();

        std::filesystem::path filepath = DockingApps::mainFileSystem.CreateFilepathForTabWNoFileExtension(tabData, folderlocation);

        std::string rawVideoPath = filepath.string() + "_RAW.mp4"; // This is the full path incl. generated filename for the raw video
        std::string videoPath = filepath.string() + "_OVERLAY.mp4"; // This is the full path incl. generated filename for the raw video

        g_Extensiometer.video_save_path_raw = rawVideoPath.c_str(); // This is the full path incl. generated filename .avi
        g_Extensiometer.video_save_path = videoPath.c_str(); // This is the full path incl. generated filename .avi

        DockingApps::mainFileSystem.SaveApplicationSettings(); // Want to save the last folder location used

        DockingApps::console.AddLog("Successfully saved last saved folder position for video recording!");
        PlaySoundEffect("buttonClick1");
    }
}

// -----------------------------------------------------------------------------
// ------------------------ File Configuration Settings ------------------------
// -----------------------------------------------------------------------------

bool AppMainFileSystem::CreateImguiConfigurationFile(std::string targetFolderPath, bool overwrite)
{
    const char* configFileFrom  = "imgui_original.ini";
    const char* configFileTo    = "imgui.ini";

    std::filesystem::path imguiConfigFileFrom = std::filesystem::current_path();
    imguiConfigFileFrom.append(configFileFrom);

    std::filesystem::path imguiConfigFileTo = targetFolderPath;
    imguiConfigFileTo.append(configFileTo);

    if (!overwrite)
    {
        // Only copy if it doesn't exist
        if (!std::filesystem::exists(imguiConfigFileTo))
            return std::filesystem::copy_file(imguiConfigFileFrom, imguiConfigFileTo);
    }
    else
    {
        // Set new changes
        ImGui::LoadIniSettingsFromDisk(imguiConfigFileFrom.string().c_str());
    }

    return true; // If file exsits, it will not be created again
}

// -----------------------------------------------------------------------------

json AppMainFileSystem::ProduceJSONSettingsFromGUISettings(bool defaultSettings)
{
    // Create an empty container
    json settings;

    if (!defaultSettings)
    {
        // ---------------------------------------------------------------------
        // ------------------------ Current Tab settings -----------------------
        // ---------------------------------------------------------------------
        settings["LR10KConfigVersion"] = LR10KAppVersion;

        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        settings["gui_showSettings"]    = g_WindowStates.show_settings;
        settings["gui_showCalculator"]  = g_WindowStates.show_calculator;
        settings["gui_showGraph"]       = g_WindowStates.show_graph;
        settings["gui_showDescription"] = g_WindowStates.show_description;
        settings["gui_showConsole"]     = g_WindowStates.show_console;
        settings["gui_showDocs"]        = g_WindowStates.show_docs;
        settings["gui_showExtensiometer"]  = g_WindowStates.show_extensiometer;
        settings["enableSound"]         = mainGUIGlobals::enableSound;

		settings["ext_regionOfInterest"]	= g_Extensiometer.restrictive_area;
		settings["ext_targetColor"]			= LR10KAppGlobals::opticalSourceExtensiometerTargetColor;
		settings["ext_cameraActive"]		= g_Extensiometer.is_active;
		settings["ext_useOpticalExt"]		= LR10KAppGlobals::opticalSourceAsExtensiometer;

        settings["fullScreen"]          = mainGUIGlobals::fullScreen;
        settings["enableDPIAwareness"]  = mainGUIGlobals::enableDPIAwareness;

        settings["ignoreSavingPrompt"]  = mainGUIGlobals::ignoreSavingPrompt;
        settings["ignoreSavingPrompt2"] = mainGUIGlobals::ignoreSavingPrompt2;
        settings["guiTheme"]            = mainGUIGlobals::guiTheme;

        // ---------------------------------------------------------------------

        settings["tensionTesting"] = LR10KAppGlobals::tensionTesting;
        settings["unitsConvertionIndex"] = LR10KAppGlobals::unitsConvertionIndex;

        settings["testing_speedAndPlotGUI"]     = LR10KAppGlobals::testing_speedAndPlotGUI;
        settings["testing_speedAndPlotActual"]  = LR10KAppGlobals::testing_speedAndPlotActual;


        settings["thickness"]   = LR10KAppGlobals::calc_thickness;
        settings["width"]       = LR10KAppGlobals::calc_width;
        settings["length"]      = LR10KAppGlobals::calc_length;
        settings["useGlobalValuesOnNewTests"]      = LR10KAppGlobals::useGlobalValuesOnNewTests;

        settings["areaOptionPicked"]        = LR10KAppGlobals::areaOptionPicked;
        settings["compressionOptionPicked"] = LR10KAppGlobals::compressionOptionPicked;

        settings["diameter"]    = LR10KAppGlobals::calc_diameter;

        settings["LR10KTravelDistanceControl"]      = TabControl::LR10KTravelDistanceControl;
        settings["LR10KTravelDistanceActionAfter"]  = TabControl::LR10KTravelDistanceActionAfter;

        settings["maximumLoadAllowedInsideLR10K"] = LR10KAppGlobals::maximumLoadAllowedInsideLR10K;
        settings["maximumExtensionAllowedInsideLR10K"] = LR10KAppGlobals::maximumExtensionAllowedInsideLR10K;

        settings["extensiometerRange"]  = LR10KAppGlobals::extensiometerRange;
        settings["enable_extensiomter"] = LR10KAppGlobals::enable_extensiomter;

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        settings["fileBrowsing_LastLR10KSaveAsLocation"]    = fileBrowserSaveAsLR10KTest.GetSelected();
        settings["fileBrowsing_LastLR10KSaveLocation"]      = fileBrowserPreSaveLR10KTest.GetSelected();
        settings["fileBrowsing_LastXlxsSaveLocation"]       = fileBrowserSaveXlsx.GetSelected();
        settings["fileBrowsing_LastLR10KOpenLoadLocation"]  = fileBrowserOpenLR10KTest.GetSelected();
        settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = fileBrowserSaveVideoRecordingExt.GetSelected();

        settings["showHotkeyTips"]                          = LR10KAppGlobals::showHotkeyTips;
    }
    else
    {
        // ---------------------------------------------------------------------
        // -------------------------- Default settings -------------------------
        // ---------------------------------------------------------------------

        settings["LR10KConfigVersion"] = LR10KAppVersion;

        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        settings["gui_showSettings"]		= false;
        settings["gui_showCalculator"]		= false;
        settings["gui_showGraph"]			= true;
        settings["gui_showDescription"]		= false;
        settings["gui_showConsole"]			= false;
        settings["gui_showDocs"]			= false;
        settings["gui_showExtensiometer"]	= true;
        settings["enableSound"]				= true;

		settings["ext_regionOfInterest"]	= 0.2f;
        settings["ext_targetColor"]			= { 0.286442f, 0.598039f, 0.228662f }; // Green color
		settings["ext_cameraActive"]		= false;
		settings["ext_useOpticalExt"]		= false;

        settings["fullScreen"]          = false;
        settings["enableDPIAwareness"]  = false;

        settings["ignoreSavingPrompt"]  = false;
        settings["ignoreSavingPrompt2"] = false;
        settings["guiTheme"]            = mainGUIThemes::USNDark;

        // ---------------------------------------------------------------------

        settings["tensionTesting"] = true;
        settings["unitsConvertionIndex"] = 0;

        settings["testing_speedAndPlotGUI"] = {
                defaultValuesGUI::speedAndPlotGUI[0]
            ,   defaultValuesGUI::speedAndPlotGUI[1]
            ,   defaultValuesGUI::speedAndPlotGUI[2]
        };
        settings["testing_speedAndPlotActual"] = {
                defaultValuesGUI::speedAndPlot[0]
            ,   defaultValuesGUI::speedAndPlot[1]
            ,   defaultValuesGUI::speedAndPlot[2]
        };

        settings["thickness"]                   = defaultValuesGUI::calc_thickness;
        settings["width"]                       = defaultValuesGUI::calc_width;
        settings["length"]                      = defaultValuesGUI::calc_length;
        settings["diameter"]                    = defaultValuesGUI::calc_diameter;
        settings["useGlobalValuesOnNewTests"]   = defaultValuesGUI::useGlobalValuesOnNewTests;

        settings["areaOptionPicked"]        = 0;
        settings["compressionOptionPicked"] = 0;

        settings["LR10KTravelDistanceControl"]          = 0.000f;
        settings["LR10KTravelDistanceActionAfter"]      = 0;

        settings["maximumLoadAllowedInsideLR10K"]       = 10000.0f;
        settings["maximumExtensionAllowedInsideLR10K"]  = 500.0f;

        /*settings["breakThreshold"] = 0.1f;
        settings["enable_breakThreshold"] = 0;*/

        settings["extensiometerRange"]  = 1000.0f;
        settings["enable_extensiomter"] = 0;

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        std::pair<bool, std::string> localDocumentsFolder = GetLocalDocumentsFolder();
        std::string path;
        if (localDocumentsFolder.first)
            path = localDocumentsFolder.second;
        else
            path = std::filesystem::current_path().string();

        settings["fileBrowsing_LastLR10KSaveAsLocation"]    = path;
        settings["fileBrowsing_LastLR10KSaveLocation"]      = path;
        settings["fileBrowsing_LastXlxsSaveLocation"]       = path;
        settings["fileBrowsing_LastLR10KOpenLoadLocation"]  = path;
        settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = path;

        settings["showHotkeyTips"]                          = false;
    }

    return settings;
}

void AppMainFileSystem::InsertJSONSettingsToGUISettings(json settings)
{
    // All valid values that must exsist, or else try and create a new settings file
    std::string_view gui_showSettings                           = "gui_showSettings";
    std::string_view gui_showCalculator                         = "gui_showCalculator";
    std::string_view gui_showGraph                              = "gui_showGraph";
    std::string_view gui_showDescription                        = "gui_showDescription";
    std::string_view gui_showConsole                            = "gui_showConsole";
    std::string_view gui_showDocs                               = "gui_showDocs";
    std::string_view gui_showExtensiometer                      = "gui_showExtensiometer";
    std::string_view enableSound                                = "enableSound";
    std::string_view ext_regionOfInterest						= "ext_regionOfInterest";
	std::string_view ext_targetColor							= "ext_targetColor";
	std::string_view ext_cameraActive							= "ext_cameraActive";
	std::string_view ext_useOpticalExt							= "ext_useOpticalExt";
    std::string_view fullScreen                                 = "fullScreen";
    std::string_view enableDPIAwareness                         = "enableDPIAwareness";
    std::string_view ignoreSavingPrompt                         = "ignoreSavingPrompt";
    std::string_view ignoreSavingPrompt2                        = "ignoreSavingPrompt2";
    std::string_view guiTheme                                   = "guiTheme";
    std::string_view tensionTesting                             = "tensionTesting";
    std::string_view unitsConvertionIndex                       = "unitsConvertionIndex";
    std::string_view testing_speedAndPlotGUI                    = "testing_speedAndPlotGUI";
    std::string_view testing_speedAndPlotActual                 = "testing_speedAndPlotActual";
    std::string_view thickness                                  = "thickness";
    std::string_view width                                      = "width";
    std::string_view length                                     = "length";
    std::string_view diameter                                   = "diameter";
    std::string_view useGlobalValuesOnNewTests                  = "useGlobalValuesOnNewTests";
    std::string_view areaOptionPicked                           = "areaOptionPicked";
    std::string_view compressionOptionPicked                    = "compressionOptionPicked";
    std::string_view LR10KTravelDistanceControl                 = "LR10KTravelDistanceControl";
    std::string_view LR10KTravelDistanceActionAfter             = "LR10KTravelDistanceActionAfter";
    std::string_view maximumLoadAllowedInsideLR10K              = "maximumLoadAllowedInsideLR10K";
    std::string_view maximumExtensionAllowedInsideLR10K         = "maximumExtensionAllowedInsideLR10K";
    /*std::string_view breakThreshold                             = "breakThreshold";
    std::string_view enable_breakThreshold                      = "enable_breakThreshold";*/
    std::string_view extensiometerRange                         = "extensiometerRange";
    std::string_view enable_extensiomter                        = "enable_extensiomter";
    std::string_view fileBrowsing_LastLR10KSaveAsLocation       = "fileBrowsing_LastLR10KSaveAsLocation";
    std::string_view fileBrowsing_LastLR10KSaveLocation         = "fileBrowsing_LastLR10KSaveLocation";
    std::string_view fileBrowsing_LastXlxsSaveLocation          = "fileBrowsing_LastXlxsSaveLocation";
    std::string_view fileBrowsing_LastLR10KOpenLoadLocation     = "fileBrowsing_LastLR10KOpenLoadLocation";
    std::string_view fileBrowsing_LastSaveVideoRecordingExtLocation = "fileBrowsing_LastSaveVideoRecordingExtLocation";
    std::string_view showHotkeyTips                             = "showHotkeyTips";

    // Check every key
    bool thereIsAnUnvalidKey = false;

    if (!settings.contains(std::string{ gui_showSettings                        })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showCalculator                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showGraph                           })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showDescription                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showConsole                         })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showDocs                            })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showExtensiometer                   })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enableSound                             })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ext_regionOfInterest					})) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_targetColor							})) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_cameraActive						})) thereIsAnUnvalidKey = true;
	if (!settings.contains(std::string{ ext_useOpticalExt						})) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fullScreen                              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enableDPIAwareness                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ignoreSavingPrompt                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ ignoreSavingPrompt2                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ guiTheme                                })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ tensionTesting                          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ unitsConvertionIndex                    })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ testing_speedAndPlotGUI                 })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ testing_speedAndPlotActual              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ thickness                               })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ width                                   })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ length                                  })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ diameter                                })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ useGlobalValuesOnNewTests               })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ areaOptionPicked                        })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ compressionOptionPicked                 })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceControl              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceActionAfter          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumLoadAllowedInsideLR10K           })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumExtensionAllowedInsideLR10K      })) thereIsAnUnvalidKey = true;
    /*if (!settings.contains(std::string{ breakThreshold                          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_breakThreshold                   })) thereIsAnUnvalidKey = true;*/
    if (!settings.contains(std::string{ extensiometerRange                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_extensiomter                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveAsLocation    })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveLocation      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastXlxsSaveLocation       })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KOpenLoadLocation  })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastSaveVideoRecordingExtLocation })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ showHotkeyTips                          })) thereIsAnUnvalidKey = true;

    // Delete old settings file and initialize a new one
    if (thereIsAnUnvalidKey)
    {
        std::pair<bool, std::string> appData = GetLocalAppDataFolderLR10K();

        if (appData.first)
        {
            LR10KAppGlobals::LocalFolderPath = (std::filesystem::path)appData.second;

            // Set the global configuration settings file name
            LR10KAppGlobals::configSettingPathFile = LR10KAppGlobals::LocalFolderPath;
            std::string fileName = "v"; fileName.append(LR10KAppVersion).append("_lr10k.ini");
            LR10KAppGlobals::configSettingPathFile.append(fileName);

            // Delete
            if (std::filesystem::remove(LR10KAppGlobals::configSettingPathFile))
            {
                // Initialize
                LR10KGUISettingsInitialization(appData.second);
            }
        }
    }
    else
    {
        // ---------------------------------------------------------------------
        // ------------- Insert settings into respective containers ------------
        // ---------------------------------------------------------------------
        
        // ---------------------------------------------------------------------
        // -------------------------------- GUI --------------------------------
        // ---------------------------------------------------------------------
        g_WindowStates.show_settings        = settings["gui_showSettings"];
        g_WindowStates.show_calculator      = settings["gui_showCalculator"];
        g_WindowStates.show_graph           = settings["gui_showGraph"];
        g_WindowStates.show_description     = settings["gui_showDescription"];
        g_WindowStates.show_console         = settings["gui_showConsole"];
        g_WindowStates.show_docs            = settings["gui_showDocs"];
        g_WindowStates.show_extensiometer   = settings["gui_showExtensiometer"];

        mainGUIGlobals::enableSound         = settings["enableSound"];

		g_Extensiometer.restrictive_area							= settings["ext_regionOfInterest"];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[0]	= settings["ext_targetColor"][0];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[1]	= settings["ext_targetColor"][1];
		LR10KAppGlobals::opticalSourceExtensiometerTargetColor[2]	= settings["ext_targetColor"][2];
		g_Extensiometer.is_active									= settings["ext_cameraActive"];
		LR10KAppGlobals::opticalSourceAsExtensiometer				= settings["ext_useOpticalExt"];

        mainGUIGlobals::fullScreen          = settings["fullScreen"];
        mainGUIGlobals::enableDPIAwareness  = settings["enableDPIAwareness"];

        mainGUIGlobals::ignoreSavingPrompt  = settings["ignoreSavingPrompt"];
        mainGUIGlobals::ignoreSavingPrompt2 = settings["ignoreSavingPrompt2"];
        mainGUIGlobals::guiTheme            = settings["guiTheme"];

        // ---------------------------------------------------------------------

        LR10KAppGlobals::tensionTesting = settings["tensionTesting"];
        LR10KAppGlobals::unitsConvertionIndex = settings["unitsConvertionIndex"];

        LR10KAppGlobals::testing_speedAndPlotGUI[0] = settings["testing_speedAndPlotGUI"][0];
        LR10KAppGlobals::testing_speedAndPlotGUI[1] = settings["testing_speedAndPlotGUI"][1];
        LR10KAppGlobals::testing_speedAndPlotGUI[2] = settings["testing_speedAndPlotGUI"][2];

        LR10KAppGlobals::testing_speedAndPlotActual[0] = settings["testing_speedAndPlotActual"][0];
        LR10KAppGlobals::testing_speedAndPlotActual[1] = settings["testing_speedAndPlotActual"][1];
        LR10KAppGlobals::testing_speedAndPlotActual[2] = settings["testing_speedAndPlotActual"][2];

        LR10KAppGlobals::calc_thickness             = settings["thickness"];
        LR10KAppGlobals::calc_width                 = settings["width"];
        LR10KAppGlobals::calc_length                = settings["length"];
        LR10KAppGlobals::calc_diameter              = settings["diameter"];
        LR10KAppGlobals::useGlobalValuesOnNewTests  = settings["useGlobalValuesOnNewTests"];

        LR10KAppGlobals::areaOptionPicked           = settings["areaOptionPicked"];
        LR10KAppGlobals::compressionOptionPicked    = settings["compressionOptionPicked"];

        TabControl::LR10KTravelDistanceControl      = settings["LR10KTravelDistanceControl"];
        TabControl::LR10KTravelDistanceActionAfter  = settings["LR10KTravelDistanceActionAfter"];

        LR10KAppGlobals::maximumLoadAllowedInsideLR10K      = settings["maximumLoadAllowedInsideLR10K"];
        LR10KAppGlobals::maximumExtensionAllowedInsideLR10K = settings["maximumExtensionAllowedInsideLR10K"];

        LR10KAppGlobals::extensiometerRange                 = settings["extensiometerRange"];
        LR10KAppGlobals::enable_extensiomter                = settings["enable_extensiomter"];

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        fileBrowserSaveAsLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveAsLocation"]);
        fileBrowserPreSaveLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveLocation"]);
        fileBrowserSaveXlsx.SetPwd(settings["fileBrowsing_LastXlxsSaveLocation"]);
        fileBrowserOpenLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KOpenLoadLocation"]);
        fileBrowserSaveVideoRecordingExt.SetPwd(settings["fileBrowsing_LastSaveVideoRecordingExtLocation"]);

        LR10KAppGlobals::showHotkeyTips = settings["showHotkeyTips"];

        // ---------------------------------------------------------------------
        // ------------- Update the GUI information for current tab ------------
        // ---------------------------------------------------------------------
        if (TabControl::allTabTestData.size() > 0)
        {
            int currentTabId = TabControl::currActiveTabIndexId;

            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[0] = LR10KAppGlobals::testing_speedAndPlotActual[0];
            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[1] = LR10KAppGlobals::testing_speedAndPlotActual[1];
            TabControl::allTabTestData[currentTabId]->settings_speedAndPlot[2] = LR10KAppGlobals::testing_speedAndPlotActual[2];
        }
    }
}

bool AppMainFileSystem::LoadApplicationSettings()
{
    // Check if settings file exsist, and load it - If not - Abort

    // Read from JSON file
    std::ifstream inFile(LR10KAppGlobals::configSettingPathFile, std::ios::in);

    if (inFile && inFile.is_open())
    {
        // Buffer container
        json tabDataJson;

        // Insert into container
        inFile >> tabDataJson;

        // Clean up
        inFile.close();

        // Load settings into application
        InsertJSONSettingsToGUISettings(tabDataJson);

        return true;
    }

    // Clean up
    inFile.close();

    return false;
}

bool AppMainFileSystem::SaveApplicationSettingsToFile(json settings)
{
    // Overwrite any other existing settings file

    // Open stream
    std::ofstream outFile(LR10KAppGlobals::configSettingPathFile, std::ios::out);
    
    if (outFile && outFile.is_open())
    {
        // Write prettified JSON to a file
        outFile << std::setw(4) << settings << std::endl;

        // Clean up
        outFile.close();

        return true;
    }

    // Clean up
    outFile.close();

    return false;
}

bool AppMainFileSystem::SaveApplicationSettings()
{
    // Produce standard settings
    json settings = ProduceJSONSettingsFromGUISettings();

    // Save application settings
    return SaveApplicationSettingsToFile(settings);
}

bool AppMainFileSystem::SaveApplicationFolderSettingsOnly()
{
    // Check if settings file exsist, and load it - If not - Abort

    // Read from JSON file
    std::ifstream inFile(LR10KAppGlobals::configSettingPathFile, std::ios::in);

    if (inFile && inFile.is_open())
    {
        // ---------------------------------------------------------------------
        // -------------------------------- Load -------------------------------
        // ---------------------------------------------------------------------

        // Buffer container
        json tabDataJson;

        // Insert into container
        inFile >> tabDataJson;

        // Clean up
        inFile.close();

        // ---------------------------------------------------------------------
        // -------------- Insert current folder settings and save --------------
        // ---------------------------------------------------------------------

        // Insert new folder settings
        tabDataJson["fileBrowsing_LastLR10KSaveAsLocation"]    = fileBrowserSaveAsLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastLR10KSaveLocation"]      = fileBrowserPreSaveLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastXlxsSaveLocation"]       = fileBrowserSaveXlsx.GetSelected();
        tabDataJson["fileBrowsing_LastLR10KOpenLoadLocation"]  = fileBrowserOpenLR10KTest.GetSelected();
        tabDataJson["fileBrowsing_LastSaveVideoRecordingExtLocation"]  = fileBrowserSaveVideoRecordingExt.GetSelected();

        // Save
        return SaveApplicationSettingsToFile(tabDataJson);
    }

    // Clean up
    inFile.close();

    return false;
}

void AppMainFileSystem::LR10KGUISettingsInitialization(const std::string folderPath)
{
    // Save the path globally
    LR10KAppGlobals::LocalFolderPath = folderPath;

    // Set the global configuration settings file name
    LR10KAppGlobals::configSettingPathFile = LR10KAppGlobals::LocalFolderPath;
    std::string fileName = "v"; fileName.append(LR10KAppVersion).append("_lr10k.ini");
    LR10KAppGlobals::configSettingPathFile.append(fileName); // If big changes occur, it will always access succeed...

    if (!std::filesystem::exists(LR10KAppGlobals::configSettingPathFile))
    {
        // Produce standard settings
        json settings = ProduceJSONSettingsFromGUISettings(true);

        // Save application settings
        if (SaveApplicationSettingsToFile(settings))
        {
            DockingApps::console.AddLog("*First time GUI settings initialization successful!");
        }
        else
            DockingApps::console.AddLog("[error] First time initialization failed... Could not create \"%s\"", fileName);
    }

    // Load default application settings
    if (LoadApplicationSettings())
    {
        DockingApps::console.AddLog("*Initialization of GUI settings successful!");
    }
    else
        DockingApps::console.AddLog("[error] Initialization failed... Could not load \"%s\"", fileName);
}

void AppMainFileSystem::PromptUserToCloseTabBeforeSaving(const char* popUpID, bool openPopup, bool* closeTabCheck)
{
    // Check if any tab has unsaved things
    bool unsavedThings = false;
    for (int i = 0; i < TabControl::allTabTestData.size(); i++)
    {
        if (TabControl::allTabTestData[i]->unsavedThingsInTab)
            unsavedThings = true;
    }

    // Maybe close
    if (openPopup)
    {
        if ((mainGUIGlobals::forcePopupCloseApp && !mainGUIGlobals::ignoreSavingPrompt) || (unsavedThings && !mainGUIGlobals::ignoreSavingPrompt))
            ImGui::OpenPopup(popUpID); // ImGuiPopupFlags_AnyPopupLevel
        else
            *closeTabCheck = true;
    }

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(popUpID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("All unsaved data will be lost.\nDo you want to continue?");
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox(" Don't ask me again", &mainGUIGlobals::ignoreSavingPrompt);
        ImGui::PopStyleVar();

        ImGui::NewLine();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();

            *closeTabCheck = true;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();

            // It will only get saved and applied if the "OK" button is clicked.
            if (mainGUIGlobals::ignoreSavingPrompt) mainGUIGlobals::ignoreSavingPrompt = false;

            *closeTabCheck = false;
        }
        ImGui::EndPopup();
    }
}

// -----------------------------------------------------------------------------
// --------------------------------- Tab System --------------------------------
// -----------------------------------------------------------------------------
double alpha = 0.1; // 10% new data, 90% old smoothed data
static double smoothed_load = 0.0;

/**
    @brief Adds a new point to the global "LR10KTensileData" container, that will be displayed in the plot area.
    @param tabData        - 
    @param LR10KExtension - 
    @param LR10KLoad      - 
**/
void TabControl::AddNewPointFromLoadAndExtension(TabControl::TabTestDataClass* tabData, float LR10KExtension, float LR10KLoad)
{
    // Add data points if test is running
    if (!tabData->test_finished && tabData->test_running_test)
    {
        // Get previous point
        ImVec2 lastPointAdded =
            tabData->LR10KTensileData.Data.Size > 0
            ? tabData->LR10KTensileData.Data[tabData->LR10KTensileData.Data.Size - 1]
            : ImVec2(0.0f, 0.0f);

        // New point
        ImVec2 newPoint((float)RoundTo((double)LR10KExtension, 3), (float)RoundTo((double)LR10KLoad, 3));

        // The extension must be higher (saftey if the rig is shaking or something..)
        // !!Important to prevent negative values!!
        if (newPoint.x >= 0.0f && newPoint.y >= 0.0f && (lastPointAdded.x == 0.0f || newPoint.x > lastPointAdded.x))
            // Add point
            tabData->LR10KTensileData.AddPoint(newPoint.x, newPoint.y);
            tabData->LR10KTensileData.AddTime();

        // Increment data (used always for both real and fake load)
        if (mainGUIGlobals::useDemoMode
            && !mainGUIGlobals::useDemoModeLiveExtension
            && tabData->test_running_test)
        {
            TabControl::LR10KOffsetExtension += 0.003f;
        }
    }
}
/**
    @brief  Find the test data based on the uniq id
    @param  tabId - 
    @retval       - 
**/
TabControl::TabTestDataClass* TabControl::FindTabTestData(int tabId)
{
    // Find the tab that contains the tabId
    for (int i = 0; i < allTabTestData.size(); i++)
    {
        // Check if it matches any tabId
        if (allTabTestData[i]->tabId == tabId)
            return allTabTestData[i]; // Success
    }

    return {};
}
/**
    @brief Used for undoing crucial point.
**/
void TabControl::CrucialPointsUndoPoint(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    std::vector<std::tuple<ImPlotPoint, int>>* dataPoints = &tabData->plot_dataPoints;

    // Check if the last point has an property ID == 4, or contiune to check
    for (int i = dataPoints->size() - 1; i > 0; i--)
    {
        // Check if valid...
        if (dataPoints->size() > 0 && std::get<int>(dataPoints->at(i)) == IM_ARRAYSIZE(TabControl::plot_items) - 1)
        {
            dataPoints->erase(dataPoints->begin() + i);
            break;
        }
    }
}

/**
    @brief Tries to prevent hitting extreme limits for LR 10K. Load and extension.
**/
void TabControl::PreventExtremeLimitsLR10K(TabControl::TabTestDataClass* tabData)
{
    if (!LR10KAppGlobals::configurationDone) return;

    float LR10KConvertedLoad        = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
    // float LR10KConvertedDistance    = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();

    if (
        // Load (N)
        LR10KConvertedLoad >= LR10KAppGlobals::maximumLoadAllowedInsideLR10K) // 10000 N
    {
        if (tabData->test_unfinished_test_started)
            LR10KControlSystem::EndTest(tabData);
        else
            LR10KControlSystem::Translation_STOP(tabData);
    }

    // Per. now, it is not easy to get the distance, since when zero point is set, the internal distance for LR 10K is set to zero.
    //if (
    //    // Load (N)
    //    LR10KConvertedLoad >= 9999.0f // 10000 N
    //    // Extension (mm)
    //    || LR10KConvertedDistance <= 9.0f // 10 mm
    //    || LR10KConvertedDistance >= 999.0f) // 1000 mm
    //{
    //    if (tabData->test_unfinished_test_started)
    //        LR10KControlSystem::EndTest(tabData);
    //    else
    //        LR10KControlSystem::Translation_STOP(tabData);
    //}
}
/**
    @brief Tries to prevent hitting user set limits for LR 10K. Load and extension.
    @param tabData -
**/
void TabControl::PreventUserLimitsLR10K(TabControl::TabTestDataClass* tabData)
{
    if (!LR10KAppGlobals::configurationDone) return;

    float LR10KConvertedLoad        = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad();
    // float LR10KConvertedDistance    = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension();

    if (
        // Load (N)
        LR10KConvertedLoad >= LR10KAppGlobals::maximumLoadAllowedInsideLR10K) // 10000 N
    {
        if (tabData->test_unfinished_test_started)
            LR10KControlSystem::EndTest(tabData);
        else
            LR10KControlSystem::Translation_STOP(tabData);
    }

    // Per. now, it is not easy to get the distance, since when zero point is set, the internal distance for LR 10K is set to zero.
    //if (
    //    // Load (N)
    //    LR10KConvertedLoad >= LR10KAppGlobals::maximumLoadAllowedInsideLR10K // 10000 N
    //        // Extension (mm)
    //        || LR10KConvertedDistance >= LR10KAppGlobals::maximumExtensionAllowedInsideLR10K) // 1000 mm
    //{
    //    if (tabData->test_unfinished_test_started)
    //        LR10KControlSystem::EndTest(tabData);
    //    else
    //        LR10KControlSystem::Translation_STOP(tabData);
    //}
}

/**
    @brief  For running an auto updater of the plot area, for smooth axis update. (a hook)
    @details Will also check if the LR 10K goes over the min/max limits extension/load and end the test if it does.
                Will also check for break detection and if there is a pyshical crash when translating downwards (when test not started)
    @param  tabData - 
    @retval         - 
**/
UINT __cdecl TabControl::AutoAdjustNormalExtensionLoadVariabels_PlotUpdater(TabControl::TabTestDataClass* tabData)
{
    LR10KController _LR10KController;

    while (!mainGUIGlobals::done)
    {
        // The controller for "PauseUpdate2" is in the main_dx11.cpp, and will destroy this thread when a test is finished
        if (tabData != nullptr)
        {
            // Check if the LR 10K hits any limits to End Test

            // Extreme limits
            if (!mainGUIGlobals::useDemoMode)
                PreventExtremeLimitsLR10K(tabData);
            // User set limits
            if (!mainGUIGlobals::useDemoMode)
                PreventUserLimitsLR10K(tabData);

            // Update the graph (this will create a smooth animation also inside this hook)
            //AutoAdjustNormalExtensionLoadVariabels(tabData, true);

            if (!tabData->test_finished)
            {
                // Check if there is a physical crash or not to stop the machine
                DockingApps::mutexControl.StatusMessage();
                if (
                    !tabData->test_unfinished_test_started
                    && LR10KAppGlobals::shutLR10KDownIfLoadToHigh
                    && LR10KAppGlobals::currentStatusMessage.load() != "Ready"
                    && LR10KAppGlobals::stopTranslationOfCrossheadSaftey)
                {
                    // See if the load is over 50 N, and stop the translation
                    if (abs(LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad()) > 50.0f)
                    {
                        _LR10KController.w_Stop();
                        defaultValuesGUI::resetNavigationGUIButtons = true;
                    }
                }
            }
        }
    }

    return 0;
}

/**
    @brief  Checks if distance travelled control is activated.
    @retval  - 
**/
bool TabControl::CheckIfDistanceTravelControlActivated()
{
    return TabControl::LR10KTravelDistanceControl > 0;
}
/**
    @brief Logic for checking if the machine should be stopped/pause after a certain distance travelled
    @param tabData -
    @param LR10KExtension - 
**/
void TabControl::CheckIfDistanceTraveledToLimit(TabControl::TabTestDataClass* tabData, float LR10KExtension)
{
    if (tabData == nullptr) return;

    if (TabControl::CheckIfDistanceTravelControlActivated() && LR10KExtension >= TabControl::LR10KTravelDistanceControl)
    {
        switch (TabControl::LR10KTravelDistanceActionAfter)
        {
        case 0:
            LR10KControlSystem::EndTest(tabData);
            break;
        case 1:
            LR10KControlSystem::PauseTest(tabData);
            break;
        default:
            break;
        }
    }
}

/**
    @brief Main function for calculating e.g. stresses and strain. It is run before main.Draw.
    @param tabData - 
**/
void TabControl::CalculateDataFromTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    if (LR10KAppGlobals::useGlobalValuesOnNewTests)
    {
        // Update the local material properties in the calculator for the current tab
        DockingApps::calculator.SetLocalMaterialProperties(tabData, LR10KAppGlobals::areaOptionPicked, LR10KAppGlobals::calc_thickness, LR10KAppGlobals::calc_width, LR10KAppGlobals::calc_length, LR10KAppGlobals::calc_diameter);
    }

    // If it is tensile or compression
    bool condition1 = tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked); // Normal tension or compression
    bool condition2 = !tabData->tension && 1 == tabData->compressionOptionPicked; // 3-point compression

    if (tabData->NeedsRecalculation.recalculate) {
        if (condition1)
        {
            // Calculate stresses
            DockingApps::calculator.CalculateStressPoints(tabData);
            if (tabData->NeedsRecalculation.markAsUnsaved) tabData->MarkTabAsUnsaved();
        }
        else if (condition2)
        {
            // Calculate bending moment and UTS
            DockingApps::calculator.CalculateCompressionBendMomentAndUTS(tabData);
            if (tabData->NeedsRecalculation.markAsUnsaved) tabData->MarkTabAsUnsaved();
        }

        tabData->NeedsRecalculation.markAsUnsaved = false;
        tabData->NeedsRecalculation.recalculate = false;
    }
}

/**
    @brief Creates a new tab and fills it in with information, if any.
    @param newTabData - 
**/
void AppMainTabSystem::CreateANewTab(TabControl::TabTestDataClass* newTabData)
{
    if (newTabData == nullptr) return;

    // Tab index is always increasing, and the tab test data is not removed. This is for keeping everything uniq
    size_t tabIndexId = TabControl::allTabTestData.size();

    // Add new tab
    TabControl::activeTestTabsId.push_back(tabIndexId);

    // Insert tab data
    newTabData->tabId = tabIndexId;

    // Initialize necesarry values from GUI (could come from buttons that are previously set)
    // that will affect the type of test that will be ran.
    // These values are also retrived from tabData before a test i ran and complete
    newTabData->tension                 = LR10KAppGlobals::tensionTesting;
    newTabData->compressionOptionPicked = LR10KAppGlobals::compressionOptionPicked;

    TabControl::allTabTestData.push_back(newTabData);

    // Set the current tab
    TabControl::currActiveTabIndexId = tabIndexId;
}
/**
    @brief Creates the navigation system for all of the tabs + the information area underneath a tab.
**/
void AppMainTabSystem::CreateTabMenuSystem(TabControl::TabTestDataClass* tabData)
{
    // Insert a tab menu, so new tests can be added
    if (tabData == nullptr) // Initialize with some default tabs
    {
        TabControl::TabTestDataClass* newTabTestData = new TabControl::TabTestDataClass();
        CreateANewTab(newTabTestData);

        tabData = newTabTestData;
    }

    // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
    static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;

    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        // Leading TabItemButton(): click the "?" button to open a menu
        if (ImGui::TabItemButton(" ? ", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip))
            ImGui::OpenPopup("MyHelpMenu");

        if (ImGui::BeginPopup("MyHelpMenu"))
        {
            ImGui::Selectable("These are all of your current tests.\n\nTo start a new test, click the \"+ New Test\" button.\nYou can save a test as an Excel book also.");
            ImGui::EndPopup();
        }

        bool notALockedTab = !tabData->test_unfinished_test_started;

        // Trailing Tabs: "+ New Test"
        if (ImGui::TabItemButton("+ New Test", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)
            && TabControl::activeTestTabsId.Size < TabControl::MAXTABS && notALockedTab)
        {
            TabControl::TabTestDataClass* newTabTestData = new TabControl::TabTestDataClass;
            CreateANewTab(newTabTestData);
            PlaySoundEffect("buttonClick1");
        }

        // Submit our regular tabs
        for (int i = 0; i < TabControl::activeTestTabsId.Size; i++)
        {
            int tabTestId = TabControl::activeTestTabsId[i];
            TabControl::TabTestDataClass* loopedTabData = TabControl::allTabTestData[tabTestId];

            // 1. Setup Tab Flags
            int flags = loopedTabData->unsavedThingsInTab ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;
            if (TabControl::forcedTabIndexId == tabTestId && tabData->test_unfinished_test_started)
                flags |= ImGuiTabItemFlags_SetSelected;

            // 2. Setup Close Button Pointer
            // If there's only 1 tab, or it's locked, pass nullptr so the "X" button hides.
            // Otherwise, pass a pointer to `open` so ImGui can draw the "X" and modify it.
            bool open = true;
            bool* p_open = (TabControl::activeTestTabsId.Size > 1 && notALockedTab) ? &open : nullptr;

            char name[32];
            snprintf(name, sizeof(name), "%04d      ", tabTestId);

            // 3. Draw the Tab
            if (ImGui::BeginTabItem(name, p_open, flags))
            {
                // ==========================================
                // TAB SWITCH DETECTOR
                // ==========================================
                if (TabControl::currActiveTabIndexId != tabTestId)
                {
                    TabControl::currActiveTabIndexId = tabTestId; // Update tracking

                    // Flag this tab to auto-fit its axes on the next frame!
                    // (You should add `bool request_graph_autofit = false;` to your TabTestDataClass)
                    loopedTabData->request_graph_autofit = true;

                    // ==========================================
                    // TAB CONTENT / INFORMATION UPDATE
                    // ==========================================
                    // Update the settings for vertical sliders if changed
                    if (LR10KAppGlobals::testing_speedAndPlotActual[0] != loopedTabData->settings_speedAndPlot[0] || LR10KAppGlobals::testing_speedAndPlotActual[2] != loopedTabData->settings_speedAndPlot[2])
                    {
                        LR10KAppGlobals::testing_speedAndPlotGUI[0] = ((loopedTabData->settings_speedAndPlot[0] / LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0]) * 100);
                        LR10KAppGlobals::testing_speedAndPlotGUI[2] = loopedTabData->settings_speedAndPlot[2];
                        LR10KAppGlobals::testing_speedAndPlotActual[0] = loopedTabData->settings_speedAndPlot[0];
                        LR10KAppGlobals::testing_speedAndPlotActual[2] = loopedTabData->settings_speedAndPlot[2];
                    }
                }

                // Update units if changed
                if (loopedTabData->fileFinishedLoading && loopedTabData->unitsConvertionIndex != LR10KAppGlobals::unitsConvertionIndex)
                {
                    LR10KAppGlobals::unitsConvertionIndex = loopedTabData->unitsConvertionIndex;
                    loopedTabData->ConvertUnitsToNewUnit(loopedTabData, LR10KAppGlobals::unitsConvertionIndex, false);
                }

                // Draw textual info
                ImGui::Text("This is tensile test: %04d. You can save the data within the \"Graph\" panel as a .XLSX file, and as a loadable file in the menu.", tabTestId);
                ImGui::Separator();

                if (loopedTabData->test_finished)
                {
                    ImGui::Text("This test is finished. It was completed: %s", loopedTabData->date.c_str());
                    if (!loopedTabData->loadedFromFileName.empty())
                        ImGui::Text("(Represents same data as in file: %s).", loopedTabData->loadedFromFileName.c_str());
                    ImGui::Separator();
                }

                // Apply global variable offset
                defaultValuesGUI::offsetZeroPointExtensionAxis = loopedTabData->LR10OffsetZeroPointExtensionAxis;

                ImGui::EndTabItem();
            }

            // 4. Tab Close Logic
            bool user_clicked_close = (p_open && !open);
            bool closeTabCheck = false;

            if (user_clicked_close)
            {
                // If we don't need a prompt, flag it to close instantly
                if (mainGUIGlobals::ignoreSavingPrompt || !loopedTabData->unsavedThingsInTab)
                {
                    closeTabCheck = true;
                }
            }

            // The Prompt function MUST run every frame so ImGui can draw the modal!
            // It will trigger OpenPopup internally if 'needs_prompt' is true this frame.
            bool needs_prompt = (user_clicked_close && !mainGUIGlobals::ignoreSavingPrompt && loopedTabData->unsavedThingsInTab);

            std::string popUpID = "Close tab #" + std::to_string(tabTestId) + " without saving?";
            DockingApps::mainFileSystem.PromptUserToCloseTabBeforeSaving(popUpID.c_str(), needs_prompt, &closeTabCheck);

            // Execute the actual close and array cleanup
            if (closeTabCheck)
            {
                ImGui::SetTabItemClosed(name);
                TabControl::activeTestTabsId.erase(TabControl::activeTestTabsId.Data + i);
                PlaySoundEffect("buttonClick0");
                i--; // Decrement iterator so we don't skip the next tab
            }
        } // End of the for-loop

        ImGui::EndTabBar();
    }
}
/**
    @brief  Used for checking if the tab. is running a test or not.
    @retval  - 
**/
bool AppMainTabSystem::ThisTabIsNotRunningATest()
{
    if (TabControl::aTestIsStartedOnTabIndexId > -1)
    {
        return TabControl::currActiveTabIndexId != TabControl::aTestIsStartedOnTabIndexId;
    }

    return false;
}

std::vector<std::pair<ImPlotPoint, int>> TabControl::TabTestDataClass::GetCrucialDataPoints()
{
    std::vector<std::pair<ImPlotPoint, int>> crucialDataPoints;

    for (auto pt_tuple : plot_dataPoints)
    {
        ImPlotPoint pt = std::get<0>(pt_tuple);
        int id = std::get<1>(pt_tuple);

        bool hasTheIdAlready = std::any_of(
            crucialDataPoints.begin(),
            crucialDataPoints.end(),
            [id](const auto& dataTuple) {
                return std::get<1>(dataTuple) == id; // std::get<1> gets the 'int' from the tuple
            }
        );

        // Take the first ID found
        if (!hasTheIdAlready) crucialDataPoints.push_back({ pt , id });
    }

    return crucialDataPoints;
}

bool TabControl::TabTestDataClass::CheckIfCrucialPointExsists(int pointID)
{
    bool hasID = std::any_of(
        plot_dataPoints.begin(),
        plot_dataPoints.end(),
        [pointID](const auto& dataTuple) {
            return std::get<1>(dataTuple) == pointID; // std::get<1> gets the 'int' from the tuple
        }
    );

    return hasID;
}

ImPlotPoint TabControl::TabTestDataClass::GetUTS()
{
    ImPlotPoint biggestLoadPoint;

    // Find maximum
    for (int i = 0; i < LR10KTensileData.Data.Size; i++)
    {
        ImVec2 dataPoint = LR10KTensileData.Data[i];

        if (dataPoint.y > biggestLoadPoint.y)
        {
            biggestLoadPoint.x = dataPoint.x;
            biggestLoadPoint.y = dataPoint.y;
        }
    }

    return biggestLoadPoint;
}
ImPlotPoint TabControl::TabTestDataClass::GetFracturePoint(TabTestDataClass* tabData)
{
    ImPlotPoint fracturePoint(0, 0);

    auto it = std::find_if(
        tabData->plot_dataPoints.begin(),
        tabData->plot_dataPoints.end(),
        [](const auto& dataTuple) {
            return std::get<1>(dataTuple) == 3;
        }
    );

    if (it != tabData->plot_dataPoints.end()) {
        // ID 3 exists! Access the data:
        ImPlotPoint pointData = std::get<0>(*it);
        int id = std::get<1>(*it);

        fracturePoint = pointData;
    }

    tabData->MarkTabAsUnsaved();

    return fracturePoint;
}

//void TabControl::TabTestDataClass::AssignPoint_PropotionalLimit(TabControl::TabTestDataClass* tabData, ImPlotPoint propotionalLimitPoint)
//{
//    if (!tabData->tension && 1 == tabData->compressionOptionPicked) return;
//
//    // Only assign if point x and y is not NaN, which can happen...
//    if (!std::isnan(propotionalLimitPoint.x) && !std::isnan(propotionalLimitPoint.y) && propotionalLimitPoint.y > 0 && propotionalLimitPoint.x > 0)
//    {
//        plot_dataPoints.push_back({ propotionalLimitPoint, 0 });
//
//        // Assign correct data point index ID Globally
//        plot_importantDataPoints[0] = { propotionalLimitPoint, 0 }; // Propotional limit point has index ID 0
//        DockingApps::console.AddLog("Automatically added a Propotional Limit point. (just an assumtion)");
//    }
//    else
//        DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");
//
//    //TabControl::RefreshUpdateImportantDataPoints(tabData);
//}
//void TabControl::TabTestDataClass::AssignPoint_YieldPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint yieldingPoint)
//{
//    // THIS LOGIC IS NOT FROM THEORY, ONLY ASSUMTION AND IS REMOVED. Easier to do it manually...
//    //// Order checker
//    //if (plot_importantDataPoints[0].first.x == 0) return;
//
//    //// Only assign if point x and y is not NaN, which can happen...
//    //if (!std::isnan(yieldingPoint.x) && !std::isnan(yieldingPoint.y) && yieldingPoint.y > 0 && yieldingPoint.x > 0)
//    //{
//    //    plot_dataPoints.push_back({ yieldingPoint, 1, false });
//
//    //    // Assign correct data point index ID Globally
//    //    plot_importantDataPoints[1] = { yieldingPoint, 1 }; // Yield point has index ID 1
//    //    DockingApps::console.AddLog("Automatically added a Yielding point. (just an assumtion)");
//    //}
//    //else
//    //    DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");
//
//    //TabControl::RefreshUpdateImportantDataPoints(tabData);
//}
void TabControl::TabTestDataClass::AssignPoint_UltimateTensileStrength(TabControl::TabTestDataClass* tabData)
{
    ImPlotPoint UTSPoint = this->GetUTS(); // UTS

    // Only assign if point x and y is not NaN, which can happen when using Demo mode...
    if ((!std::isnan(UTSPoint.x) && !std::isnan(UTSPoint.y) && UTSPoint.y > 0 && UTSPoint.x > 0))
    {
        plot_dataPoints.push_back({ UTSPoint, 2 });
        DockingApps::console.AddLog("Automatically added a Ultimate Tensile Strength point. (just an assumtion)");
        tabData->MarkTabAsUnsaved();
    }
    else
        DockingApps::console.AddLog("Could not add Ultimate Tensile Strength since one or more points were NaN...");
}
void TabControl::TabTestDataClass::AssignPoint_BreakPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint breakPoint)
{
    // Only assign if point x and y is not NaN, which can happen...
    if (!std::isnan(breakPoint.x) && !std::isnan(breakPoint.y) && breakPoint.y > 0 && breakPoint.x > 0)
    {
        plot_dataPoints.push_back({ breakPoint, 3 });

        DockingApps::console.AddLog("Automatically added a Break/Fracture point. (just an assumption)");
        tabData->MarkTabAsUnsaved();
    }
    else
        DockingApps::console.AddLog("Could not add Break Point since one or more points were NaN...");

    //TabControl::RefreshUpdateImportantDataPoints(tabData);
}
void TabControl::TabTestDataClass::ConvertUnitsBackToBase(TabControl::TabTestDataClass* tabData)
{
    // -------------------------------------------------------------------------
    // ----------------- Convert all datapoints back to Newton -----------------
    // -------------------------------------------------------------------------

    // Get the old unit index before changing it to the new one
    const int oldUnitIndex = tabData->unitsConvertionIndex;

    if (oldUnitIndex > -1)
    {
        // E.g. mm and in
        const double unitConvOld_dist = LR10KAppGlobals::GetUnitOption<double>(8, oldUnitIndex);
        // E.g. N an lbf
        const double unitConvOld_force = LR10KAppGlobals::GetUnitOption<double>(9, oldUnitIndex);
        // E.g. n/mm^2 and lbs/in^2
        const double unitConvOld_stress = LR10KAppGlobals::GetUnitOption<double>(10, oldUnitIndex);

        tabData->thickness  /= unitConvOld_dist;
        tabData->width      /= unitConvOld_dist;
        tabData->length     /= unitConvOld_dist;
        tabData->diameter   /= unitConvOld_dist;
        // Area will be calculated automatically

        // "Normal" plot data
        for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
        {
            tabData->LR10KTensileData.Data[i].x /= unitConvOld_dist;
            tabData->LR10KTensileData.Data[i].y /= unitConvOld_force;
        }

        // Plot points
        for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
        {
            ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
            points.x /= unitConvOld_dist;
            points.y /= unitConvOld_force;

            const int index = std::get<int>(tabData->plot_dataPoints[i]);

            // Save
            tabData->plot_dataPoints[i] = std::make_tuple(points, index);
        }

        //// Crucial points
        //for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
        //{
        //    ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
        //    points.x /= unitConvOld_dist;
        //    points.y /= unitConvOld_force;

        //    const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

        //    // Save
        //    tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
        //}
    }
}
void TabControl::TabTestDataClass::ConvertUnitsToNewUnit(TabControl::TabTestDataClass* tabData, int unitsIndex, bool saveUnitIndex, bool updateGlobals, bool markAsSaved)
{
    if (tabData == nullptr) return;

    ConvertUnitsBackToBase(tabData);

    // -------------------------------------------------------------------------
    // ------------------ Convert all data points to new unit ------------------
    // -------------------------------------------------------------------------

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8, unitsIndex);
    // E.g. N an lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9, unitsIndex);
    // E.g. n/mm^2 and lbs/in^2
    const double unitConv_stress = LR10KAppGlobals::GetUnitOption<double>(10, unitsIndex);

    tabData->thickness  *= unitConv_dist;
    tabData->width      *= unitConv_dist;
    tabData->length     *= unitConv_dist;
    tabData->diameter   *= unitConv_dist;
    // Area will be calculated automatically

    // "Normal" plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
    {
        tabData->LR10KTensileData.Data[i].x *= unitConv_dist;
        tabData->LR10KTensileData.Data[i].y *= unitConv_force;
    }

    // Plot points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        ImPlotPoint points    = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index             = std::get<int>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index);
    }

    // -------------------------------------------------------------------------
    // -------------------------------- Finished -------------------------------
    // -------------------------------------------------------------------------

    // Save the new unit index to tabdata
    if (saveUnitIndex)
        tabData->unitsConvertionIndex = LR10KAppGlobals::unitsConvertionIndex;

    if (updateGlobals)
    {
        // Update global values
        LR10KAppGlobals::calc_thickness = tabData->thickness;
        LR10KAppGlobals::calc_width     = tabData->width;
        LR10KAppGlobals::calc_length    = tabData->length;
        LR10KAppGlobals::calc_diameter  = tabData->diameter;
    }

    tabData->NeedsRecalculation.markAsUnsaved = true;
    tabData->NeedsRecalculation.recalculate = true;
}
void TabControl::TabTestDataClass::ConvertUnitsToNewUnitAfterLoad(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr || (tabData != nullptr && !tabData->fileFinishedLoading)) return;

    // -------------------------------------------------------------------------
    // ------------------ Convert all data points to new unit ------------------
    // -------------------------------------------------------------------------

    // E.g. mm and in
    const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8, tabData->unitsConvertionIndex);
    // E.g. N an lbf
    const double unitConv_force = LR10KAppGlobals::GetUnitOption<double>(9, tabData->unitsConvertionIndex);
    // E.g. n/mm^2 and lbs/in^2
    const double unitConv_stress = LR10KAppGlobals::GetUnitOption<double>(10, tabData->unitsConvertionIndex);

    tabData->thickness *= unitConv_dist;
    tabData->width *= unitConv_dist;
    tabData->length *= unitConv_dist;
    tabData->diameter *= unitConv_dist;
    // Area will be calculated automatically

    // "Normal" plot data
    for (int i = 0; i < tabData->LR10KTensileData.Data.size(); i++)
    {
        tabData->LR10KTensileData.Data[i].x *= unitConv_dist;
        tabData->LR10KTensileData.Data[i].y *= unitConv_force;
    }

    // Plot points
    for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index = std::get<int>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index);
    }

    // -------------------------------------------------------------------------
    // -------------------------------- Finished -------------------------------
    // -------------------------------------------------------------------------

    // Update global values
    LR10KAppGlobals::calc_thickness = tabData->thickness;
    LR10KAppGlobals::calc_width = tabData->width;
    LR10KAppGlobals::calc_length = tabData->length;
    LR10KAppGlobals::calc_diameter = tabData->diameter;

    // Update to new unit
    if (LR10KAppGlobals::unitsConvertionIndex == tabData->unitsConvertionIndex)
        tabData->NeedsRecalculation.markAsUnsaved = false;
        
    LR10KAppGlobals::unitsConvertionIndex = tabData->unitsConvertionIndex;
    tabData->NeedsRecalculation.recalculate = true;
}

void TabControl::TabTestDataClass::MarkTabAsSaved()
{
    unsavedThingsInTab = false;
}
void TabControl::TabTestDataClass::MarkTabAsUnsaved()
{
    unsavedThingsInTab = true;
}

// -----------------------------------------------------------------------------
// ------------------------------ Local Functions ------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Get the current time in string form. (readable)
    @retval  - 
**/
const char* getCurrentTimeString()
{
    // Current date/time based on current system
    time_t now = time(0);
    // Convert now to string form
    char* dt = ctime(&now);

    return dt;
}

// -----------------------------------------------------------------------------
// ----------------------------- LR 10K COM System -----------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Find all COM ports available on the current computer.
    @retval  -
**/
std::pair<PULONG, ULONG> LR10KCOMSystem::FindAllCOMPorts()
{
    // Find all COM Serial Ports, and return their name in correct order
    ULONG portNumbers[100]; // Ports IDs
    ULONG portsFound;       // Amount of ports found

    GetCommPorts(portNumbers, 100, &portsFound);
    std::pair<PULONG, ULONG> allComPorts = std::make_pair(portNumbers, portsFound);

    return allComPorts;
}
/**
    @brief  Connect to COM Port. When iPortNumber = -1, the first port available will be connected to.
    @param  iPortNumber -
    @retval             -
**/
bool LR10KCOMSystem::TryConnectingToCOMPort(int iPortNumber)
{
    // Maybe disconnect from previous COM port first
    LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();

    #if UNICODE
        std::wstring str = L"\\\\.\\COM";
        str.append(std::to_wstring(iPortNumber));
        LR10KDataDeEncryptionSystem::ConnectedCOMPortName = str;

        // Used for trying to creating/connecting to a COM port.
        auto tryToConnect = [](LPCWSTR str) { LR10KDataDeEncryptionSystem::m_hSerial = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); };
    #else
        std::string str = "\\\\.\\COM";
        str.append(std::to_string(iPortNumber));
        LR10KDataDeEncryptionSystem::ConnectedCOMPortName = str;

        // Used for trying to creating/connecting to a COM port.
        auto tryToConnect = [](LPCSTR str) { LR10KDataDeEncryptionSystem::m_hSerial = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); };
    #endif

    // Connect to a spesific COM Port
    if (iPortNumber > -1)
    {
        tryToConnect(str.c_str());

        // Could not connect
        if (LR10KDataDeEncryptionSystem::m_hSerial == INVALID_HANDLE_VALUE)
        {
            DockingApps::console.AddLog("[error] COM%d Port does not exist/could not be connected to...", iPortNumber);
            return false;
        }
    }
    else
    {
        // Check for maximum 100 ports - Try connecting to the first valid one
        for (int i = { 0 }; i < 100; i++)
        {
#if UNICODE
            str = L"\\\\.\\COM";
            str.append(std::to_wstring(i));
#else
            str = "\\\\.\\COM";
            str.append(std::to_string(i));
#endif // !UNICODE

            tryToConnect(str.c_str());

            // Connection successful
            if (LR10KDataDeEncryptionSystem::m_hSerial != INVALID_HANDLE_VALUE)
                break;
        }

        // No valid serial port found on computer...
        if (LR10KDataDeEncryptionSystem::m_hSerial == INVALID_HANDLE_VALUE)
        {
            DockingApps::console.AddLog("[error] No valid COM Port found on computer out of the first 100...");
            return false;
        }
    }

    // Set some basic settings for the COM Port that is
    // compatible with the LR 10K Tensile Macine
    DCB serialParams = { 0 };
    serialParams.DCBlength = sizeof(serialParams);

    GetCommState(LR10KDataDeEncryptionSystem::m_hSerial, &serialParams);
    serialParams.BaudRate = 9600;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    serialParams.fInX = false;
    serialParams.fOutX = false;
    serialParams.fOutxCtsFlow = true;
    serialParams.fOutxDsrFlow = true;
    serialParams.fDsrSensitivity = true;

    serialParams.fDtrControl = DTR_CONTROL_HANDSHAKE; // DTR_CONTROL_ENABLE; // DTR_CONTROL_HANDSHAKE;
    serialParams.fRtsControl = DTR_CONTROL_HANDSHAKE; // RTS_CONTROL_ENABLE; // RTS_CONTROL_HANDSHAKE;
    SetCommState(LR10KDataDeEncryptionSystem::m_hSerial, &serialParams);

    // Set timeouts (ms)
    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutConstant = 50;
    timeout.ReadTotalTimeoutMultiplier = 50;
    timeout.WriteTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 10;

    SetCommTimeouts(LR10KDataDeEncryptionSystem::m_hSerial, &timeout);

    // ---------------------------------------------------
    // Start reading from COM
    // ---------------------------------------------------

    // Create a new thread to handle information, so it doesn't stall the main thread (old thread being destroyed when disconnecting from old COM port)
    LR10KDataDeEncryptionSystem::m_pThreadReadCom = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::ReadFromLR10KAndWriteToRingBuffer, LR10KDataDeEncryptionSystem::m_hSerial, &LR10KDataDeEncryptionSystem::m_SerialBuffer);
    // Let the thread run independently detached from main
    LR10KDataDeEncryptionSystem::m_pThreadReadCom->detach();
    // Start the thread
    LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag.store(false);

    // Everything OK
    LR10KAppGlobals::connectedComPortNumber = iPortNumber;

    // Update the GUI if everything is successful
    LR10KAppGlobals::readyToConnectToLR10K = true;

    PlaySoundEffect("buttonClick1");

    DockingApps::console.AddLog("COM%d port connected!", LR10KAppGlobals::connectedComPortNumber);

    return true;
}
/**
    @brief  Tries to disconnect from current COM port.
    @retval  -
**/
bool LR10KCOMSystem::TryDisconnectingFromCurrCOMPort()
{
    LR10KControlSystem::TellLR10KToStopSendingData();
    Sleep(30);

    if (LR10KDataDeEncryptionSystem::m_hSerial != INVALID_HANDLE_VALUE && LR10KCOMSystem::CheckIfCOMIsConnected())
    {
        // Terminate thread (since we are disconnecting from COM port)
        LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadCom, &LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag);

        // Get file name
        #if UNICODE
                std::wstring str = L"\\\\.\\COM";
                str.append(std::to_wstring(LR10KAppGlobals::connectedComPortNumber));
        #else
                std::string str = "\\\\.\\COM";
                str.append(std::to_string(LR10KAppGlobals::connectedComPortNumber));
        #endif

        // Close handle
        if (CloseHandle(LR10KDataDeEncryptionSystem::m_hSerial))
        {
            DeleteFile(LR10KDataDeEncryptionSystem::ConnectedCOMPortName.c_str());
            DockingApps::console.AddLog("Disconnected from COM%d...", LR10KAppGlobals::connectedComPortNumber);

            LR10KDataDeEncryptionSystem::m_hSerial = INVALID_HANDLE_VALUE;
            LR10KAppGlobals::connectedComPortNumber = -1;

            return true;
        }
        else
            DockingApps::console.AddLog("[error] Could not disconnected from COM%d...", LR10KAppGlobals::connectedComPortNumber);
    }

    return false;
}
/**
    @brief  Checks if COM is connected to software or not.
    @retval  - 
**/
bool LR10KCOMSystem::CheckIfCOMIsConnected()
{
    return LR10KAppGlobals::connectedComPortNumber > -1;
}
/**
    @brief  Check if there are any data being sent from LR 10K Tensile Machine.
    @param  hSerial -
    @param  uInfo   -
    @retval         -
**/
BOOL LR10KCOMSystem::PeekDataFromCom(HANDLE hSerial, byte& uInfo)
{
    DWORD readBytes{ 0 };
    return ReadFile(hSerial, &uInfo, 1, &readBytes, NULL);
}
/**
    @brief  Get full data from LR 10K Tensile Machine to GUI.
    @param  hSerial -
    @retval         -
**/
byte LR10KCOMSystem::GetDataFromCom(HANDLE hSerial)
{
    byte uInfo = 0x00;
    DWORD readBytes{ 0 };
    BOOL success = 0;

    // Wait until data from COM is read
    const auto start = std::chrono::high_resolution_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(6, start) && readBytes == 0)
    {
        success = ReadFile(hSerial, &uInfo, 1, &readBytes, NULL);
        // printf("ReadFile %d %d %01X\n", success, readBytes, uInfo);
    }

    // Doesn't make sense, but removes warnings in VS
    if (success)
        return uInfo;

    return uInfo;
}
/**
    @brief  Send data from GUI to LR 10K Tensile Machine.
    @param  bufferMessage -
    @param  hSerial       -
    @retval               -
**/
bool LR10KCOMSystem::SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE hSerial)
{
    DWORD WriteBytes;

    if (bufferMessage.isBuffSingle())
        WriteFile(hSerial, &bufferMessage.message_single, (DWORD)(size_t)1, &WriteBytes, NULL);
    else
        WriteFile(hSerial, &bufferMessage.message, (DWORD)bufferMessage.size_of_message, &WriteBytes, NULL);

    // If it fails to send data from COM
    if (WriteBytes == 0)
    {
        // This can appear under setup 1, but it is not a problem
        DockingApps::console.AddLog("[error] Failed to send data to COM... Check connection!");
        return false;
    }

    return true;
}
/**
    @brief  Reads data sent back from the LR 10K Tensile Machine through the COM port. It waits for a certain HEX feedback.
    @param  hexdecimalData -
    @retval                -
**/
bool LR10KCOMSystem::WaitAndReadFromCOM(byte hexdecimalData)
{
    byte rec{ 0 }; // Used to record packages of HEX data read from the serial buffer.

    // Try and read data
    int numberOfTries = 0;
    const auto start = std::chrono::high_resolution_clock::now();
    do
    {
        // Will be around 1000 tries in 1 seconds for 1ms
        if (numberOfTries >= 2000)
            break;

        rec = LR10KDataDeEncryptionSystem::m_SerialBuffer.Read(LR10KAppGlobals::configurationDone);
        printf("Leser av COM: %02x\n", rec);

        numberOfTries++;
    } while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(1, start) && rec != hexdecimalData);

    return true;
}

// -----------------------------------------------------------------------------
// LR 10K Data Decryption/Encryption System (some code in "LR10KDecryption.cpp" also)
// -----------------------------------------------------------------------------

/**
    @brief  (INPUT 1) Write data from the LR 10K Tensile Machine to the round buffer. (runs on seperate thread)
    @param  m_hSerial      - 
    @param  m_SerialBuffer - 
    @retval                - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::ReadFromLR10KAndWriteToRingBuffer(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer)
{
    byte Data;
    const auto start = std::chrono::high_resolution_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(2, start) && !mainGUIGlobals::done)
    {
        Data = LR10KCOMSystem::GetDataFromCom(m_hSerial);

        m_SerialBuffer->Write(Data);
    }

    // Clean up
    // Terminate thread for (INPUT 2) - Not needed because it is dependent on this thread (INPUT 1)
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadLR10K, &LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag); //!< 

    // Clear/reset serial buffer
    m_SerialBuffer->ClearAllData();

    return 0;
}

/**
    @brief  Used for setup 1 process, so not everything freezes and the user can see the status of connection.
    @retval  - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::ConnectingToLR10K()
{
    m_pThreadLR10KIsLoading_isLoading.store(true);

        LR10KControlSystem::TryConnectingToLR10K();

    Sleep(150); // !!Important!!
    m_pThreadLR10KIsLoading_isLoading.store(false);

    return 0;
}

/**
    @brief  Used for setup 2 process, so not everything freezes and the user can see the status of connection.
    @retval  - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::SendSetupToLR10K()
{
    m_pThreadLR10KIsLoading_isLoading.store(true);

        LR10KControlSystem::SendTestSetupToLR10K();
        LR10KControlSystem::TellLR10KToSendData(); // Make it "official"

    Sleep(150); // !!Important!!
    m_pThreadLR10KIsLoading_isLoading.store(false);

    return 0;
}

/**
    @brief  (INPUT 2) (Step 1) Interpet data from the LR10K Tensile Machine that has been stored in the ring buffer. (runs on seperate thread)
    @param  m_hSerial      - 
    @param  m_SerialBuffer - 
    @retval                - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer)
{
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_InterpetWrittenRingBufferData() && !mainGUIGlobals::done)
    {
        byte uData1 = m_SerialBuffer->Read();
        // printf("Thread got some data from LR 10K: %02x ...\n", uData1);

        if (uData1 >= 0xE0)
            // We have a data package comming. (used for load, distance etc.)
            LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_dataPackage(uData1, m_SerialBuffer);
        else
        {
            switch (uData1)
            {
            case 0xB1:
                // We have a overflow message. (for getting the mutiplier)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_overflowMessage(uData1, m_SerialBuffer);
                break;
            case 0xB6:
                // We have a status message. (for getting the current status of LR 10K)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_statusMessage(uData1, m_SerialBuffer);
                break;
            case 0xBF:
                // We have a warning/error message. (used for e.g. magnetic stop)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_warnErrorStatusMessage(uData1, m_SerialBuffer);
                break;

            default:
                // Unknown. (as of now unkown data)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_unkownData(uData1);
                break;
            }
        }
    }

    // Clear/reset serial buffer
    m_SerialBuffer->ClearAllData();

    return 0;
}

// -----------------------------------------------------------------------------
// ------------- Handle data coming from the LR 10K Tensile Machine ------------
// -----------------------------------------------------------------------------

double RoundTo(double val, int prec) {
    double p = pow(10, (double)prec);

    return floor(val * p + 0.5) / p;
}

/**
    @brief  Calculates readable load from raw load.
    @retval  - 
**/
float LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad()
{
    int value = ((int)TabControl::LR10KRawLoad - (int)TabControl::LR10KOffsetLoad) * LR10KAppGlobals::Convert_to_Newton;
    // Reverse positive direction when compression is enabled
    value = LR10KAppGlobals::tensionTesting ? value : value * -1;

    return (float)RoundTo((double)(value * LR10KAppGlobals::GetUnitOption<double>(9)), 3);
}
/**
    @brief  Calculates readable distance from raw values.
    @retval  -
**/
float LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension()
{
    float value;

    if (LR10KAppGlobals::opticalSourceAsExtensiometer)
        // Real extension from the optical camera source (comes from the Computer Vision thread)
        value = g_Extensiometer.results.travel_mm;
    else
        // From machine
        value = TabControl::LR10KRawExtension * LR10KAppGlobals::Convert_to_mm;

    // Reverse positive direction when compression is enabled
    value = LR10KAppGlobals::tensionTesting ? value : value * -1;

    return (float)RoundTo((double)(value * LR10KAppGlobals::GetUnitOption<double>(8)), 3);
}

/**
    @brief  Displays a nice name for the load. (rounded of with two decimals)
    @retval  - 
**/
std::string LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_load()
{
    char roundedValueChar[20];
    sprintf_s(roundedValueChar, "%.1f", InterpetWrittenRingBufferData_getLoad());

    std::string str = roundedValueChar;
    return str;
}
/**
    @brief  Displays a nice name for the distance. (rounded of with two decimals)
    @retval  - 
**/
std::string LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_extension()
{
    char roundedValueChar[20];
    sprintf_s(roundedValueChar, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::maximumExtensionAllowedInsideLR10K), InterpetWrittenRingBufferData_getExtension());

    std::string str = roundedValueChar;
    return str;
}

// -----------------------------------------------------------------------------
// (Step 4) Final convertion of the interpeted values (runs on the seperate thread "InterpetWrittenRingBufferData")
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
    @brief Current load that is being applied. (Newton)
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_load(LPARAM lParam)
{
    TabControl::LR10KRawLoad = (short)lParam;
    if (TabControl::LR10KOffsetLoad == 0)
        TabControl::LR10KOffsetLoad = TabControl::LR10KRawLoad;

    TabControl::LR10KLoadNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_load();
}
/**
    @brief Total distance travelled from zero point. (mm)
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_extension(LPARAM lParam)
{
    // Reverse positive direction when compression is enabled
    TabControl::LR10KRawExtension += (__int16)lParam;

    TabControl::LR10KExtensionNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_extension();
}
/**
    @brief Total distance travelled. (mm) ?????? ***Not finished
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_extensometerRange(LPARAM lParam)
{
    TabControl::LR10KRawExtensometerRange = (short)lParam;

    char roundedValueChar[20];
    sprintf_s(roundedValueChar, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char((float)TabControl::LR10KRawExtensometerRange), (float)TabControl::LR10KRawExtensometerRange);
    //	m_sTmp.Format(_T("LR10K: Extensometer = %02x in %d"), m_sCurrentExtension, m_sCurrentExtension - m_sOffsetExtension);
    //	m_CtrlLoglist.InsertString(0, m_sTmp.GetString());

    TabControl::LR10KExtensometerRangeNice = roundedValueChar;
}
/**
    @brief Current byte multiplier in use.
    @details Used for load - look at docs for "bMultiplier_load".
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_overflow(LPARAM lParam)
{
    printf("Input, LR 10K: Multiplier update (for load); new multiplier = %d\n", (short)lParam);
}
/**
    @brief Current status the LR 10K Tensile Machine has.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_status(LPARAM lParam)
{
    // Special cases
    if ((byte)lParam == 0x00)
    {
        // If this is sent by LR 10K and it is because it is ready after return to zero point,
        // accept this position as valid if it is 0 for load and extension
        if (LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad() <= 0.005f && LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension() <= 0.005f)
            LR10KAppGlobals::zeroPointIsSet = true;
    }

    switch ((byte)lParam)
    {
    case 0x00:
        DockingApps::console.AddLog("Input, LR 10K: Status - Ready");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Ready"));
        break;
    case 0x40:
        DockingApps::console.AddLog("Input, LR 10K: Status - Stopped fast movement");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Stopped fast movement"));
        break;
    case 0x41:
        DockingApps::console.AddLog("Input, LR 10K: Status - Going up");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Going up"));
        break;
    case 0x61:
        DockingApps::console.AddLog("Input, LR 10K: Status - Going down");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Going down"));
        break;

    default:
        printf("Input, LR 10K: Status - Unknown = %02x\n", (byte)lParam);
        DockingApps::console.AddLog("Input, LR 10K: Status - Unknown = %02x", (byte)lParam);
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Unknown - Look in console"));
        break;
    }

    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
}
/**
    @brief Current warning/error status the LR 10K Tensile Machine has.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_warnErrorStatus(LPARAM lParam)
{
    switch ((byte)lParam)
    {
    case 0x00:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - OK");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "OK";
        break;
    case 0x0C:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status -  Extension range exceeded (error 12)");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Extension range exceeded (error 12)";
        // This warning and others will be reset when the user ends a test or sends a new setup sequence
        break;
    case 0x02:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Tension overload stop. Go DOWN only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Tension overload stop. Go DOWN only";
        break;
    case 0x05:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Lower crosshead limit stop. Go UP only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Lower crosshead limit stop. Go UP only";
        break;
    case 0x06:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Compression overload stop. Go UP only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Compression overload stop. Go UP only";
        break;
    case 0x12:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Keyboard buffer overflow");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Keyboard buffer overflow";
        break;

    default:
        printf("Input, LR 10K: Warn/Error Status - Unknown = %02x\n", (byte)lParam);
        DockingApps::console.AddLog("Input, LR 10K: Status - Unknown = %02x", (byte)lParam);
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Unknown - Look in console";
        break;
    }
}
/**
    @brief Resets the warning received from LR 10K in the software. Sometimes it doesn't send a new update for errors.
**/
void LR10KDataDeEncryptionSystem::warnErrorStatus_Reset()
{
    InterpetWrittenRingBufferData_handling_warnErrorStatus((LPARAM)0x00);
    DockingApps::console.AddLog("... From software.");
}
/**
    @brief Unknow message coming from the LR 10K Tensile Machine.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_unknownData(LPARAM lParam)
{
    printf("Input, LR 10K: Unknown HEX :: 0x%02X\n", (byte)lParam);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
/**
    @brief  Converts a value the user wants to have as an extension limit for the LR 10K, and makes a new value.
    @details Decoded algorithm from HEX values. This float value is at the end converted to a new float for LR 10K.
                It was to my notice that the value for extension got changed, but not to the value entered. I managed to
                map the algorithm. It is basically every value under 5 will become 10. After that it goes in steps of 2, where
                every value between a certain amount will be changed to another value. For instance:
                0 -> 5 will become 10. 5.5 -> 10 will become 20. 10.7 -> 20 will become 30. Here there is an increase of +10
                from the 1 to 2 step. Now there are two steps (i = 2) i % 2 = 0 and the last value will become increased by +20.
                So the new limit will now be 50 instead of 20. 21 -> 30 will become 50 and 31 -> 40 will become 60. (+10).
                The last limit will now have added +20 again, then the next limit becomes 80. 41 -> 50 will become 80. And so on.
                Why is this used? Just so that the LR 10K machine can stop if the user accidentally goes over limit.
    @param  value -
    @retval       -
**/
float LR10KDataDeEncryptionSystem::convertAlgorithmForExtension(float value)
{
    // Initialize
    int limit = 20;
    int switcher = 1;
    int multiplier = 1;

    float newValue = roundf(value);

    while (true)
    {
        if (newValue > 5)
        {
            // Found correct number
            if ((newValue + 10 * multiplier) <= limit)
            {
                newValue = limit;
                break;
            }

            // Increase every 2 step
            if (switcher % 2 == 0)
            {
                switcher = 1;
                limit += 20;
                multiplier++;
            }
            else
            {
                limit += 10;
                switcher++;
            }
        }
        else
        {
            newValue = 10;
            break;
        }
    }

    // Never exceed 1000 mm (which is new machines limit)
    if (newValue > 1000.0f)
        newValue = 1000.0f;

    return newValue;
}

/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value - Intenger which gives away how many decimal places that should be shown.
    @retval       - 
**/
int LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy(float value)
{
    if (value <= 5.0f)
        return 3;
    else if (value < 60.0f)
        return 2;
    else if (value < 600.0f)
        return 1;

    return 0;
}
/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value - 
    @retval       - Characthers that can be used as needed. E.g. %0.1f.
**/
const char* LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(float value)
{
    if (value <= 5.0f)
        return "%0.3f";
    else if (value < 60.0f)
        return "%0.2f";
    else if (value < 600.0f)
        return "%0.1f";

    return "%0.0f";
}
/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value -
    @retval       - A float value that will represent the accuracy. E.g. 0.01f
**/
float LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(float value)
{
    if (value <= 5.0f)
        return 0.001f;
    else if (value < 60.0f)
        return 0.01f;
    else if (value < 600.0f)
        return 0.1f;

    return 0.0f;
}

// -----------------------------------------------------------------------------
// --------------------------- LR 10K Control System ---------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Try to connect from the LR 10K Tensile Machine.
    @retval             - 
**/
bool LR10KControlSystem::TryConnectingToLR10K()
{
    bool setupOneSuccess = _LR10KController.w_Setup1();
    if (!setupOneSuccess)
        return false;

    // -------------------------------------------------------------------------
    // ------------ Start to intrepret stuff from the LR 10K machine -----------
    // -------------------------------------------------------------------------

    // Create a new thread seperate from main thread
    LR10KDataDeEncryptionSystem::m_pThreadReadLR10K = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData, LR10KDataDeEncryptionSystem::m_hSerial, &LR10KDataDeEncryptionSystem::m_SerialBuffer);
    // Let the thread run independently detached from main
    LR10KDataDeEncryptionSystem::m_pThreadReadLR10K->detach();
    // Start the thread
    LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag.store(false);

    // Update the GUI if everything is successful
    LR10KAppGlobals::configurationDone              = true;
    LR10KAppGlobals::currentLiveStatusMessage       = "No";
    LR10KAppGlobals::currentWarnErrorStatusMessage  = "OK";

    PlaySoundEffect("buttonClick1");

    DockingApps::console.AddLog("Connected PC to LR 10K successfully!");
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Applying settings ..."));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // -------------------------------------------------------------------------
    // -------------------------- Send setup to LR 10K -------------------------
    // -------------------------------------------------------------------------

    // Create a new thread seperate from main thread
    LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::SendSetupToLR10K);
    // Let the thread run independently detached from main
    LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading->detach();

    return true;
}
/**
    @brief  Try to disconnect from the LR 10K Tensile Machine.
    @retval  - 
**/
bool LR10KControlSystem::TryDisconnectFromLR10K()
{
    if (LR10KAppGlobals::configurationDone && LR10KCOMSystem::CheckIfCOMIsConnected())
    {
        LR10KControlSystem::TellLR10KToStopSendingData(); // !! Important !!
        Sleep(300); // !! Important !!
        _LR10KController.w_Disconnect();
        Sleep(50);
    }

    // Reset GUI variabels
    LR10KAppGlobals::configurationDone              = false;

    LR10KAppGlobals::sendingLiveDataFromLR10K       = false;
    LR10KAppGlobals::currentLiveStatusMessage       = "N/A";
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("N/A"));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
    LR10KAppGlobals::currentWarnErrorStatusMessage  = "N/A";

    TabControl::LR10KRawLoad                = 0.0f;
    TabControl::LR10KRawExtension           = 0.0f;
    TabControl::LR10KRawExtensometerRange   = 0.0f;

    TabControl::LR10KOffsetLoad                 = 0.0f;
    TabControl::LR10KOffsetExtension            = 0.0f;
    TabControl::LR10KOffsetExtensometerRange    = 0.0f;

    TabControl::LR10KLoadNice       = "0.0";
    TabControl::LR10KExtensionNice   = "0.0";
    TabControl::LR10KExtensometerRangeNice  = "0.0";

    TabControl::LR10KTravelDistanceControl      = 0.0f;
    TabControl::LR10KTravelDistanceActionAfter  = 0;

    // Terminate thread
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadLR10K, &LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag);
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadConnectingToLR10K);
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading);

    // Clear all data
    LR10KDataDeEncryptionSystem::m_SerialBuffer.ClearAllData();
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("N/A"));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();

    DockingApps::console.AddLog("Disconnected PC from LR 10K successfully!");

    return true;
}
/**
    @brief Send setup for future test to LR 10K Tensile Machine.
**/
void LR10KControlSystem::SendTestSetupToLR10K()
{
    LR10KController _LR10KController;

    const float fSpeed      = LR10KAppGlobals::testing_speedAndPlotActual[0];
    const float fJogSpeed   = LR10KAppGlobals::testing_speedAndPlotActual[1];

    DockingApps::console.AddLog("Setting up the LR 10K with current settings ...");
    std::string str = "PC | Sending setup : Test Speed = % .3f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    str.append(" ...");
    DockingApps::console.AddLog(str.c_str(), fSpeed);
    str = "PC | Jog Speed = %.3f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    str.append(" ...");
    DockingApps::console.AddLog(str.c_str(), fJogSpeed);
    DockingApps::console.AddLog("PC | %s Test.", LR10KAppGlobals::tensionTesting ? "Tension" : "Compression");

    LR10KDataDeEncryptionSystem::warnErrorStatus_Reset();

    // Send setup 2
    if (_LR10KController.w_Setup2(fSpeed, fJogSpeed, LR10KAppGlobals::tensionTesting))
    {
        Sleep(150);
        if (LR10KAppGlobals::setZeroPointOnStarTest)
            LR10KControlSystem::SetZeroPoint();

        PlaySoundEffect("buttonClick1");

        DockingApps::console.AddLog("PC | Finished sending setup to LR 10K!");
    }
}

/**
    @brief Make LR 10K Tensile Machine start sending data.
**/
void LR10KControlSystem::TellLR10KToSendData()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xC0);

        DockingApps::console.AddLog("PC | Telling LR 10K to start sending data");
        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");

        LR10KAppGlobals::sendingLiveDataFromLR10K = true;
        LR10KAppGlobals::currentLiveStatusMessage = "Yes";

        PlaySoundEffect("buttonClick1");
    }
}
/**
    @brief Make LR 10K Tensile Machine stop sending data.
**/
void LR10KControlSystem::TellLR10KToStopSendingData()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xC1);

        DockingApps::console.AddLog("PC | Telling LR 10K to stop sending data");
        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");

        LR10KAppGlobals::sendingLiveDataFromLR10K = false;
        LR10KAppGlobals::currentLiveStatusMessage = "No";
    }
}
/**
    @brief Get the status report from LR 10K Tensile Machine.
**/
void LR10KControlSystem::GetCurrReportFromLR10K()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xB6);

        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");
    }
}


/**
    @brief  Sets a zero point.
    @retval  - 
**/
bool LR10KControlSystem::SetZeroPoint(TabControl::TabTestDataClass* tabData)
{
    if ((LR10KAppGlobals::configurationDone || mainGUIGlobals::useDemoMode) && (tabData == nullptr || !tabData->test_unfinished_test_started && !tabData->test_finished))
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_SetZero();

        DockingApps::console.AddLog("... From GUI.");

        if (!mainGUIGlobals::useDemoModeLiveLoad)
            TabControl::LR10KRawLoad = 0.0f;

        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            TabControl::LR10KRawExtension = 0.0f;
            TabControl::LR10KRawExtensometerRange = 0.0f;
        }

        if (!mainGUIGlobals::useDemoModeLiveLoad)
            TabControl::LR10KOffsetLoad = 0.0f;

        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            TabControl::LR10KOffsetExtension = 0.0f;
            TabControl::LR10KOffsetExtensometerRange = 0.0f;
        }

        if (!mainGUIGlobals::useDemoModeLiveLoad)
            TabControl::LR10KLoadNice = "0.0";

        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            TabControl::LR10KExtensionNice = "0.0";
            TabControl::LR10KExtensometerRangeNice = "0.0";
        }

        // ---------------------------------------------------------------------

        TabControl::LR10KOffsetLoad = TabControl::LR10KRawLoad;
        TabControl::LR10KOffsetExtensometerRange = TabControl::LR10KRawExtensometerRange;

        TabControl::LR10KLoadNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_load();
        TabControl::LR10KExtensionNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_extension();

        // ---------------------------------------------------------------------

        LR10KAppGlobals::zeroPointIsSet = true;

        DockingApps::console.AddLog("PC: Zero point is set");

        return true;
    }

    return false;
}
/**
    @brief  Start a tensile test.
    @retval  -
**/
bool LR10KControlSystem::StartTest(TabControl::TabTestDataClass* tabData)
{
    auto executeTest = [tabData]() {
        if ((LR10KAppGlobals::configurationDone || mainGUIGlobals::useDemoMode) && !tabData->test_finished && !tabData->test_unfinished_test_started && !tabData->test_running_test)
        {
            tabData->current_temp_preSaveLocation_StarTestNow = false;

            if (g_Extensiometer.record_test)
            {
                // Start recording
                g_Extensiometer.is_recording = true;
            }

            // A real test will never start when demo mode is activated
            if (mainGUIGlobals::useDemoMode && LR10KAppGlobals::setZeroPointOnStarTest)
                SetZeroPoint(tabData);
            else if (LR10KAppGlobals::configurationDone)
            {
                LR10KControlSystem::SendTestSetupToLR10K();
                _LR10KController.w_Go();
            }

            tabData->LR10KTensileData.StartTimeTest = std::chrono::steady_clock::now();
            tabData->LR10KTensileData.TimeData.clear(); // Make sure to clear old data from previous tests too!

            tabData->tension = LR10KAppGlobals::tensionTesting;

            tabData->test_running_test = true;
            tabData->test_unfinished_test_started = true;

            TabControl::aTestIsStartedOnTabIndexId = TabControl::currActiveTabIndexId;

            tabData->oldLoadErrorValue = tabData->settings_speedAndPlot[2];

            // Get the current time at the start of the program
            TabControl::timestampForLastAddedRegressionPoint = std::chrono::high_resolution_clock::now();

            // Force set the current tab
            TabControl::forcedTabIndexId = TabControl::currActiveTabIndexId;

            PlaySoundEffect("StartTest");

            return true;
        }

        };

    // Reset status to default if it got stuck in e.g. an error
    if (!mainGUIGlobals::useDemoMode)
        LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_status((LPARAM)0x00);

    if (tabData == nullptr) return false;

    // Check if user has added gauge length, gauge thickness etc. for test
    bool userHasNotAddedDimensions = LR10KAppGlobals::calc_length == 0
        || (LR10KAppGlobals::areaOptionPicked == 0 && (LR10KAppGlobals::calc_width == 0 || LR10KAppGlobals::calc_thickness == 0))
        || (LR10KAppGlobals::areaOptionPicked == 1 && LR10KAppGlobals::calc_diameter == 0);

    // Show a message
    if (userHasNotAddedDimensions)
    {
        ImGui::OpenPopup(mainGUIGlobals::popUpID_DimensionsNotAdded);
        return false;
    }

    if (tabData->current_temp_preSaveLocation == "")
    {
        // Specify pre-save location GUI to a folder
        if (tabData->current_temp_preSaveLocation == "")
        {
            fileBrowserPreSaveLR10KTest.Open();
            tabData->current_temp_preSaveLocation_StarTestNow = true;
        }
    }
    else
    {
        // If you expect a 1-minute test at 50Hz, that's 3,000 points. Reserve 10,000 to be safe!
        tabData->LR10KTensileData.Data.reserve(10000);
        tabData->LR10KTensileData.TimeData.reserve(10000);

        return executeTest();
    }

    return false;
}
/**
    @brief  End and complete a tensile test before fracture.
    @retval  -
**/
bool LR10KControlSystem::EndTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (tabData->test_unfinished_test_started && !tabData->test_finished)
    {
        if (LR10KAppGlobals::configurationDone)
        {
            _LR10KController.w_EndTest();
        }

        // Set the smoothed data
        ImVector<ImVec2> clean_point_data = SmoothTensileData(tabData->LR10KTensileData.Data, tabData->LR10KTensileData.SmoothedDataWindowIndex);
        tabData->LR10KTensileData.SmoothedData = clean_point_data;

        // Snap the dots so they perfectly hug the new curve!
        SnapCrucialPointsToSmoothedCurve(tabData->plot_dataPoints, tabData->LR10KTensileData.SmoothedData);

        tabData->LR10KTensileData.TimeData.clear(); // Make sure to clear old data from previous tests too!
        tabData->LR10KTensileData.StartTimeTest = TimePoint();

        g_Extensiometer.is_recording = false;
        //LR10KAppGlobals::zeroPointIsSet = false;

        tabData->test_finished = true;
        tabData->test_unfinished_test_started = false;
        tabData->test_running_test = false;

        // Disable force on the the current tab
        TabControl::forcedTabIndexId = -1;

        PlaySoundEffect("EndTest");

        const char* dt = getCurrentTimeString();

        tabData->date = dt;
        TabControl::aTestIsStartedOnTabIndexId = -1; // Reset

        // Automated assignment of crucial points
        if (!tabData->CheckIfCrucialPointExsists(2)) // Will also be assigned at automatic break point detection to get this point first... (just for visuals)
            tabData->AssignPoint_UltimateTensileStrength(tabData);

        LR10KDataDeEncryptionSystem::warnErrorStatus_Reset();

        return true;
    }

    return false;
}
/**
    @brief  Pause a started tensile test.
    @retval  -
**/
bool LR10KControlSystem::PauseTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (tabData->test_running_test && tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_PauseTest();

        tabData->test_running_test = false;

        PlaySoundEffect("buttonClick0");

        return true;
    }

    return false;
}
/**
    @brief  Continue a paused tensile test.
    @retval  -
**/
bool LR10KControlSystem::ContinueTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (!tabData->test_running_test && tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_ContinueTest();

        tabData->test_running_test = true;

        PlaySoundEffect("buttonClick0");

        return true;
    }

    return false;
}
/**
    @brief  Return to zero point after a test is finished.
    @retval  -
**/
bool LR10KControlSystem::ReturnToZeroPoint(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (!tabData->test_unfinished_test_started || tabData->test_finished)
    {
        if ((LR10KAppGlobals::configurationDone && !mainGUIGlobals::useDemoMode) || (LR10KAppGlobals::configurationDone && mainGUIGlobals::useDemoModeLiveExtension))
            _LR10KController.w_ReturnZero();
        DockingApps::console.AddLog("... From GUI.");

        PlaySoundEffect("buttonClick1");

        return true;
    }

    return false;
}

/**
    @brief  Stop the LR 10K Tensile Machine
    @retval  -
**/
bool LR10KControlSystem::Translation_STOP(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Stop();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move LR 10K Tensile Machine UP. Velocity is in mm/s.
    @param  tabData -
    @param  velocity - 
    @retval          - 
**/
bool LR10KControlSystem::Translation_UP(TabControl::TabTestDataClass* tabData, float velocity)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Up();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move LR 10K Tensile Machine DOWN. Velocity is in mm/s.
    @param  tabData -
    @param  velocity - 
    @retval          - 
**/
bool LR10KControlSystem::Translation_DOWN(TabControl::TabTestDataClass* tabData, float velocity)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Down();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move the LR 10K Tensile Machine fast upwards.
    @param  tabData -
    @retval  - 
**/
bool LR10KControlSystem::Translation_FASTUP_press(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_FastUpPress();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Stop the fast upwards movement of the LR 10K Tensile Machine (the same for up/down)
    @retval  -
**/
bool LR10KControlSystem::Translation_FASTUP_release()
{
    if (LR10KAppGlobals::configurationDone)
        _LR10KController.w_FastUpRelease();
    DockingApps::console.AddLog("... From GUI.");

    return true;
}
/**
    @brief  Move the LR 10K Tensile Machine fast downwards.
    @param  tabData -
    @retval  -
**/
bool LR10KControlSystem::Translation_FASTDOWN_press(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_FastDownPress();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Stop the fast upwards movement of the LR 10K Tensile Machine (the same for up/down)
    @retval  - 
**/
bool LR10KControlSystem::Translation_FASTDOWN_release()
{
    if (LR10KAppGlobals::configurationDone)
        _LR10KController.w_FastDownRelease();
    DockingApps::console.AddLog("... From GUI.");

    return true;
}


// -----------------------------------------------------------------------------
// ------------------------------- Console System ------------------------------
// -----------------------------------------------------------------------------

void AppConsole::ClearLog()
{
    for (int i = 0; i < Items.Size; i++)
        free(Items[i]);
    Items.clear();
}

void AppConsole::AddLog(const char* fmt, ...) IM_FMTARGS(2)
{
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    //Items.push_back("-----------");

    Items.push_back(Strdup(buf));
    //Items.push_back("\n");
}

void AppConsole::Draw(TabControl::TabTestDataClass* tabData, const char* title, bool* p_open, ImGuiWindowFlags extern_windowFlags)
{
    if (tabData == nullptr) return;

    ImGuiWindowFlags window_flags = 0;
    window_flags |= extern_windowFlags;


    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open, window_flags))
    {
        ImGui::End();
        return;
    }

    // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
    // So e.g. IsItemHovered() will return true when hovering the title bar.
    // Here we create a context menu only available from the title bar.
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Close Console"))
        {
            *p_open = false;
        }
        ImGui::EndPopup();
    }

    ImGui::TextWrapped("Basic console for the LR 10K Tensile Machine.");
    ImGui::Spacing();
    ImGui::TextWrapped("Enter 'HELP' for help. (like commands etc.)");

    // TODO: display items starting from the bottom

    /*if (ImGui::SmallButton("Add Debug Text")) { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
    ImGui::SameLine();*/
    if (ImGui::SmallButton("Clear")) { ClearLog(); }
    ImGui::SameLine();
    bool copy_to_clipboard = ImGui::SmallButton("Copy");
    //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

    ImGui::Separator();

    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Options, Filter
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(Extra::fonts.jetbrainsFont);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        // Display every line as a separate entry so we can change their color or add custom widgets.
        // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
        // to only process visible items. The clipper will automatically measure the height of your first item and then
        // "seek" to display only items in the visible area.
        // To use the clipper we can replace your standard loop:
        //      for (int i = 0; i < Items.Size; i++)
        //   With:
        //      ImGuiListClipper clipper;
        //      clipper.Begin(Items.Size);
        //      while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // - That your items are evenly spaced (same height)
        // - That you have cheap random access to your elements (you can access them given their index,
        //   without processing all the ones before)
        // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
        // We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
        // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
        // to improve this example code!
        // If your items are of variable height:
        // - Split them into same height items would be simpler and facilitate random-seeking into your list.
        // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!Filter.PassFilter(item))
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            else if (strstr(item, "[translation]")) { color = ImVec4(0.798f, 0.926f, 0.05f, 1.0f); has_color = true; }
            else if (strstr(item, "[before testing]")) { color = ImVec4(0.34f, 0.920f, 0.677f, 1.0f); has_color = true; }
            else if (strstr(item, "[testing]")) { color = ImVec4(0.044f, 0.920f, 0.677f, 1.0f); has_color = true; }
            else if (strstr(item, "[after testing]")) { color = ImVec4(0.34f, 0.920f, 0.677f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        char* s = InputBuf;
        Strtrim(s);
        if (s[0])
            ExecCommand(tabData, s);
        strcpy(s, "");
        reclaim_focus = true;
    }

    mainGUIGlobals::popUp_DimensionsNotAdded();

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    ImGui::End();
}

void AppConsole::ExecCommand(TabControl::TabTestDataClass* tabData, const char* command_line)
{
    if (tabData == nullptr) return;

    AddLog("# %s\n", command_line);

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
        if (Stricmp(History[i], command_line) == 0)
        {
            free(History[i]);
            History.erase(History.begin() + i);
            break;
        }
    History.push_back(Strdup(command_line));

    // Process command
    if (Stricmp(command_line, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command_line, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < Commands.Size; i++)
            AddLog("- %s", Commands[i]);
    }
    else if (Stricmp(command_line, "HISTORY") == 0)
    {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
            AddLog("%3d: %s\n", i, History[i]);
    }
    else if (!DockingApps::mainTabSystem.ThisTabIsNotRunningATest()) // Only allow control from one tab at the time
    {
        // BEFORE TESTING
        if (Stricmp(command_line, "ZERO") == 0)
        {
            if (LR10KControlSystem::SetZeroPoint(tabData))
                AddLog("[before testing] Setting zero point!");
            else
                AddLog("[error] Could not set zero point...");
        }

        // TESTING
        else if (Stricmp(command_line, "START") == 0)
        {
            if (LR10KControlSystem::StartTest(tabData))
                AddLog("[testing] Starting test!");
            else
                AddLog("[error] Could not start test...");
        }
        else if (Stricmp(command_line, "END") == 0)
        {
            if (LR10KControlSystem::EndTest(tabData))
                AddLog("[testing] Ended test!");
            else
                AddLog("[error] Could not end test...");
        }
        else if (Stricmp(command_line, "PAUSE") == 0)
        {
            if (LR10KControlSystem::PauseTest(tabData))
                AddLog("[testing] Pausing test!");
            else
                AddLog("[error] Could not pause test...");
        }
        else if (Stricmp(command_line, "CONTINUE") == 0)
        {
            if (LR10KControlSystem::ContinueTest(tabData))
                AddLog("[testing] Continue test!");
            else
                AddLog("[error] Could not continue test...");
        }

        // AFTER TESTING
        else if (Stricmp(command_line, "REZERO") == 0)
        {
            if (tabData->test_unfinished_test_started)
            {
                if (LR10KControlSystem::EndTest(tabData))
                    AddLog("[testing] Ended test!");
                else
                    AddLog("[error] Could not end test...");
            }

            if (LR10KControlSystem::ReturnToZeroPoint(tabData))
                AddLog("[after testing] Returning to zero point!");
            else
                AddLog("[error] Could not return to zero point...");
        }

        // TRANSLATION
        else if (Stricmp(command_line, "UP") == 0)
        {
            if (LR10KControlSystem::Translation_UP(tabData))
                AddLog("[translation] Moving UP");
            else
                AddLog("[error] Could not move up...");
        }
        else if (Stricmp(command_line, "FUP") == 0)
        {
            if (LR10KControlSystem::Translation_FASTUP_press(tabData))
                AddLog("[translation] Moving FAST UP");
            else
                AddLog("[error] Could not move fast up...");
        }
        else if (Stricmp(command_line, "DOWN") == 0)
        {
            if (LR10KControlSystem::Translation_DOWN(tabData))
                AddLog("[translation] Moving DOWN");
            else
                AddLog("[error] Could not move down...");
        }
        else if (Stricmp(command_line, "FDOWN") == 0)
        {
            if (LR10KControlSystem::Translation_FASTDOWN_press(tabData))
                AddLog("[translation] Moving FAST DOWN");
            else
                AddLog("[error] Could not move fast down...");
        }
        else if (Stricmp(command_line, "STOP") == 0)
        {
            if (LR10KControlSystem::Translation_STOP(tabData))
                AddLog("[translation] Movement STOPPED");
            else
                AddLog("[error] Could not stop...");
        }
    }
    else
    {
        AddLog("Unknown command: '%s'\n", command_line);
    }

    // On command input, we scroll to bottom even if AutoScroll==false
    ScrollToBottom = true;
}

int AppConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
    AppConsole* console = (AppConsole*)data->UserData;
    return console->TextEditCallback(data);
}

int AppConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Example of TEXT COMPLETION

        // Locate beginning of current word
        const char* word_end = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf)
        {
            const char c = word_start[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
                break;
            word_start--;
        }

        // Build a list of candidates
        ImVector<const char*> candidates;
        for (int i = 0; i < Commands.Size; i++)
            if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                candidates.push_back(Commands[i]);

        if (candidates.Size == 0)
        {
            // No match
            AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
        }
        else if (candidates.Size == 1)
        {
            // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0]);
            data->InsertChars(data->CursorPos, " ");
        }
        else
        {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
            int match_len = (int)(word_end - word_start);
            for (;;)
            {
                int c = 0;
                bool all_candidates_matches = true;
                for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                    if (i == 0)
                        c = toupper(candidates[i][match_len]);
                    else if (c == 0 || c != toupper(candidates[i][match_len]))
                        all_candidates_matches = false;
                if (!all_candidates_matches)
                    break;
                match_len++;
            }

            if (match_len > 0)
            {
                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
            }

            // List matches
            AddLog("Possible matches:\n");
            for (int i = 0; i < candidates.Size; i++)
                AddLog("- %s\n", candidates[i]);
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        // Example of HISTORY
        const int prev_history_pos = HistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (HistoryPos == -1)
                HistoryPos = History.Size - 1;
            else if (HistoryPos > 0)
                HistoryPos--;
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (HistoryPos != -1)
                if (++HistoryPos >= History.Size)
                    HistoryPos = -1;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if (prev_history_pos != HistoryPos)
        {
            const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}
