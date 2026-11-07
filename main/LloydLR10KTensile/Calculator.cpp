/**

    @file      Calculator.cpp
    @brief     Calculator window and material property calculations (stress, strain, E-modulus, bending).
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"

#include "LR10KController.h"

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
    // Never below zero: the tension/compression setting already gives the test direction a positive sign
    return MaterialCalc::NonNegative(MaterialCalc::EngineeringStress(f, a_0));
}
/**
    @brief  Epsilon_e or Epsilon_n | Delta_L / L_0.
    @param  delta_L - 
    @param  l_0     - 
    @retval         - 
**/
double LR10KAppGlobals::CalcEpsilon_e(double delta_L, double l_0)
{
    // Never below zero (see CalcSigma_e). True strain/stress follow: ln(1 + 0) = 0.
    return MaterialCalc::NonNegative(MaterialCalc::EngineeringStrain(delta_L, l_0));
}
/**
    @brief  Sigma_t | Sigma_e * ( 1 + Epsilon_e ).
    @param  sigma_e   - 
    @param  epsilon_e - 
    @retval           - 
**/
double LR10KAppGlobals::CalcSigma_t(double epsilon_e, double sigma_e)
{
    return MaterialCalc::TrueStress(sigma_e, epsilon_e);
}
/**
    @brief  Epsilon_t | ln * ( 1 + Epsilon_e ).
    @param  epsilon_e - 
    @retval           - 
**/
double LR10KAppGlobals::CalcEpsilon_t(double epsilon_e)
{
    return MaterialCalc::TrueStrain(epsilon_e);
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
    // Least-squares slope of the points inside the window (limits may be given in either order).
    // Tested in tests/material_calc_test.cpp.
    const MaterialCalc::LinearFit fit = MaterialCalc::FitLine(strainData, stressData, lower_limit, upper_limit);
    if (!fit.valid)
    {
        DockingApps::console.AddLog("[warning] E-Modulus: only %d data point(s) between the limits; move the limits further apart.", fit.points);
        return 0.0;
    }

    double e_module = fit.slope; // N/mm^2 (MPa) per strain, or psi with imperial units

    // SI Units
    if (LR10KAppGlobals::unitsConvertionIndex == 0)
        // Convert MPa to GPa
        e_module /= 1000;

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


void AppCalculator::AdaptTabDataForLoadError(TabControl::TabTestDataClass* tabData)
{
    // -------------------------------------------------------------------------
    // ----------- Convert load to new load that includes load error -----------
    // -------------------------------------------------------------------------

    const double currentLoadErrorGUI    = (double)LR10KAppGlobals::testing_speedAndPlotActual[2];
    const double oldLoadErrorValue      = (double)tabData->oldLoadErrorValue; // The load error the data is corrected for now

    if (currentLoadErrorGUI == oldLoadErrorValue)
        return; // Nothing to change

    // Undo the old correction and apply the new one: (1 + new/100) / (1 + old/100).
    // (The old "1 + (new - old)/100" drifted with every adjustment.) Tested in tests/material_calc_test.cpp.
    const double loadScalar = MaterialCalc::LoadErrorRescaleFactor(oldLoadErrorValue, currentLoadErrorGUI);
    if (loadScalar <= 0.0)
    {
        // -100 % or less would multiply every load by 0 (or flip the sign): impossible to undo
        DockingApps::console.AddLog("[error] A load error of %.1f %% is not possible; the test data was not changed.", currentLoadErrorGUI);
        return;
    }

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
    tabData->settings_speedAndPlot[2] = (float)currentLoadErrorGUI;

    DockingApps::console.AddLog("Updated the finished test from %.1f %% to %.1f %% load error (loads x %.5f).", oldLoadErrorValue, currentLoadErrorGUI, loadScalar);

    // Stress points and E-modulus depend on the loads, and the tab has to be saved again
    tabData->NeedsRecalculation.markAsUnsaved = true;
    tabData->NeedsRecalculation.recalculate = true;
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

    double A_0 = MaterialCalc::CrossSectionArea(LR10KAppGlobals::areaOptionPicked, T_0, W_0, D_0);
    if (A_0 <= 0.0)
        A_0 = 1.0; // Default safe denominator (dimensions not entered yet)

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
    tabData->calcData_Compression[2] = MaterialCalc::BendingMoment3Point(tabData->calcData_Compression[0], L_0);

    // Flexural Strength (UTS): Sigma = (3 * F * L) / (2 * w * t^2)
    tabData->calcData_Compression[3] = MaterialCalc::FlexuralStress3Point(tabData->calcData_Compression[0], L_0, W_0, T_0);

    // -------------------------------------------------------------------------
    // 3-Point Bending Flexural Modulus (E-Modulus) using Linear Regression
    // -------------------------------------------------------------------------
    if (tabData->LR10KTensileData.Data.Size > 10)
    {
        // 1. Default limits for the linear region (0.5 mm .. 2.0 mm of travel, in the current length unit)
        const double unitConv_dist = LR10KAppGlobals::GetUnitOption<double>(8);
        double ext_lower = 0.5 * unitConv_dist;
        double ext_upper = 2.0 * unitConv_dist;

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

        // 4. Slope of the load-deflection line (m = F / dL) in the chosen window
        const MaterialCalc::LinearFit fit = MaterialCalc::FitLine(extensionData, loadData, ext_lower, ext_upper);

        // 5. 3-point bending formula: E = (slope * L^3) / (4 * w * t^3), in N/mm^2 (MPa) or psi
        // (Re-using the safe L_0, W_0, and T_0 variables from the top of the function)
        double flexuralModulus = fit.valid ? MaterialCalc::FlexuralModulus3Point(fit.slope, L_0, W_0, T_0) : 0.0;
        if (!fit.valid)
            DockingApps::console.AddLog("[warning] Flexural modulus: only %d data point(s) between the limits; move the limits further apart.", fit.points);

        // Shown as GPa with SI units (same as for tension), psi with imperial units
        if (LR10KAppGlobals::unitsConvertionIndex == 0)
            flexuralModulus /= 1000.0;

        tabData->calcYoungsModulus = flexuralModulus;
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

    // Calculate area (circular used to be pi * d / 4, missing the square: wrong stresses in Excel)
    tabData->area = MaterialCalc::CrossSectionArea(areaOption, thickness, width, diameter);
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

            Extra::tools.DrawLegendChip(IMPLOT_RED);
            ImGui::Text("Engineering Stress");
            Extra::tools.DrawLegendChip(IMPLOT_BLUE);
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
                    Extra::tools.TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_PropLimit[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_PropLimit[0] * 100);
                    ImGui::SeparatorText("True stress");
                    str3 = "Stress, σ_t: %0.3f ";
                    str3.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
                    Extra::tools.TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_PropLimit[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_PropLimit[0] * 100);

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
                    Extra::tools.TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_yield[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_yield[0] * 100);
                    ImGui::SeparatorText("True stress");
                    Extra::tools.TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_yield[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_yield[0] * 100);

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
                    Extra::tools.TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrainUTS[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrainUTS[0] * 100);
                    ImGui::SeparatorText("True stress");
                    Extra::tools.TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrainUTS[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrainUTS[0] * 100);

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
                    Extra::tools.TextWithWhiteBackground(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_fracture[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_fracture[0] * 100);
                    ImGui::SeparatorText("True stress");
                    Extra::tools.TextWithWhiteBackground(colorText_true, str3.c_str(), tabData->calcPoint_TrueStressStrain_fracture[1]);
                    Extra::tools.TextWithWhiteBackground(colorText_true, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_fracture[0] * 100);

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
                    Extra::tools.TextWithWhiteBackground(colorText_nom, str.c_str(), tabData->calcData_Compression[3]);

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
                    Extra::tools.TextWithBackground(colorText2, IM_COL32(52, 52, 52, 250), str2.c_str(), (double)tabData->calcYoungsModulus);
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
