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
    return (delta_L - defaultValuesGUI::offsetZeroPointExtensionAxis) / l_0;
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
        const bool snappedOnto = std::get<bool>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index, snappedOnto);
    }

    // Crucial points
    for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
        points.y *= loadScalar;

        const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

        // Save
        tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
    }

    // Save new load error
    tabData->oldLoadErrorValue = currentLoadErrorGUI;
}

void AppCalculator::CalculateStressPoints(TabControl::TabTestDataClass* tabData, bool initDimensions, bool playSound, bool markAsSaved, bool saveAppSettings)
{
    // -------------------------------------------------------------------------
    // ------------------------------ Calculations -----------------------------
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // T_0 = Start thickness, W_0 = Start width, L_0 = Start length
    // Do the calculations based on the crucial points added (F/A)

    const double T_0 = LR10KAppGlobals::calc_thickness;      // Start Thickness
    const double W_0 = LR10KAppGlobals::calc_width;          // Start Width
    const double L_0 = LR10KAppGlobals::calc_length;         // Start Length
    const double D_0 = LR10KAppGlobals::calc_diameter;       // Start Diameter
    double A_0 = 0; // Start Area
    if (LR10KAppGlobals::areaOptionPicked == 0)
    {
        A_0 = T_0 * W_0;
        tabData->area = A_0; // Calculated above

        if (initDimensions)
        {
            tabData->thickness = T_0;
            tabData->width = W_0;
        }
    }
    else if (LR10KAppGlobals::areaOptionPicked == 1)
    {
        A_0 = (M_PI * D_0) / 4;
        tabData->area = A_0; // Calculated above

        if (initDimensions)
            tabData->diameter   = D_0;
    }
    if (initDimensions)
        tabData->length = L_0;

    // -------------------------------------------------------------------------
    // Reset all previous calculated properties
    // -------------------------------------------------------------------------
    auto resetProp = [](double arr[2]) { for (int i = 0; i < 2; i++) { arr[i] = 0; } };

    resetProp(tabData->calcPoint_NominalStressStrain_PropLimit);
    resetProp(tabData->calcPoint_TrueStressStrain_PropLimit);

    resetProp(tabData->calcPoint_NominalStressStrain_yield);
    resetProp(tabData->calcPoint_TrueStressStrain_yield);

    resetProp(tabData->calcPoint_NominalStressStrainUTS);
    resetProp(tabData->calcPoint_TrueStressStrainUTS);

    resetProp(tabData->calcPoint_NominalStressStrain_fracture);
    resetProp(tabData->calcPoint_TrueStressStrain_fracture);

    // Find the most important data point by picking the first of that index ID
    auto findImportantDataPoint = [](TabControl::TabTestDataClass* tabData, int indexID, ImPlotPoint &ForceExtension, bool &found) -> void {
            for (int i = 0; i < tabData->plot_dataPoints.size(); i++)
            {
                ImPlotPoint dataPoint = std::get<ImPlotPoint>(tabData->plot_dataPoints[i]);
                int dataPointPropID = std::get<int>(tabData->plot_dataPoints[i]);

                if (dataPointPropID == indexID)
                {
                    // Save the force load
                    ForceExtension.x = dataPoint.x;  // Extension
                    ForceExtension.y = dataPoint.y;  // Load

                    found = true;

                    break;
                }
            }
        };

    // -------------------------------------------------------------------------
    // ----------------- Find the first propotional limit point ----------------
    // ------------------- based on the ID of the data points ------------------
    // -------------------------------------------------------------------------
    ImPlotPoint propLimitForceExtension;
    bool foundPropLimit = false;

    // Index ID = 0 is the Prop. limit
    findImportantDataPoint(tabData, 0, propLimitForceExtension, foundPropLimit);

    // -------------------------------------------------------------------------
    // --------------------- Find the first yielding point ---------------------
    // ------------------- based on the ID of the data points ------------------
    // -------------------------------------------------------------------------
    ImPlotPoint yieldingForceExtension;
    bool foundY = false;
    // Index ID = 1 is the Yield
    findImportantDataPoint(tabData, 1, yieldingForceExtension, foundY);

    // -------------------------------------------------------------------------
    // ---------------------- Find the first maximum point ---------------------
    // ------------------- based on the ID of the data points ------------------
    // -------------------------------------------------------------------------
    ImPlotPoint ultimateTensileStrengthForceExtension;
    bool foundUts = false;
    // Index ID = 2 is the Ultimate Tensile Strength
    findImportantDataPoint(tabData, 2, ultimateTensileStrengthForceExtension, foundUts);

    // -------------------------------------------------------------------------
    // --------------------- Find the first fracture point ---------------------
    // ------------------- based on the ID of the data points ------------------
    // -------------------------------------------------------------------------
    ImPlotPoint fractureForceExtension;
    bool foundFracture = false;
    // Index ID = 3 is the Yield
    findImportantDataPoint(tabData, 3, fractureForceExtension, foundFracture);

    // -------------------------------------------------------------------------
    // --------------------------- PROPORTIONAL LIMIT --------------------------
    // -------------------------------------------------------------------------
    if (foundPropLimit)
    {
        DockingApps::console.AddLog("Found a Proportional limit point! Calculating...");

        // The engineering/nominal stress and strain
        tabData->calcPoint_NominalStressStrain_PropLimit[0] = LR10KAppGlobals::CalcEpsilon_e(propLimitForceExtension.x, L_0);
        tabData->calcPoint_NominalStressStrain_PropLimit[1] = LR10KAppGlobals::CalcSigma_e(propLimitForceExtension.y, A_0);

        // The true stress and strain
        tabData->calcPoint_TrueStressStrain_PropLimit[0] = LR10KAppGlobals::CalcEpsilon_t(tabData->calcPoint_NominalStressStrain_PropLimit[0]);
        tabData->calcPoint_TrueStressStrain_PropLimit[1] = LR10KAppGlobals::CalcSigma_t(tabData->calcPoint_NominalStressStrain_PropLimit[0], tabData->calcPoint_NominalStressStrain_PropLimit[1]);
    }
    else
    {
        DockingApps::console.AddLog("[warn] Could not find a Proportional limit point for calculation...");
    }

    // -------------------------------------------------------------------------
    // --------------------------- YIELDING STRENGTH ---------------------------
    // -------------------------------------------------------------------------
    if (foundY)
    {
        DockingApps::console.AddLog("Found a Yielding point! Calculating...");

        // The engineering/nominal stress and strain
        tabData->calcPoint_NominalStressStrain_yield[0] = LR10KAppGlobals::CalcEpsilon_e(yieldingForceExtension.x, L_0);
        tabData->calcPoint_NominalStressStrain_yield[1] = LR10KAppGlobals::CalcSigma_e(yieldingForceExtension.y, A_0);

        // The true stress and strain
        tabData->calcPoint_TrueStressStrain_yield[0] = LR10KAppGlobals::CalcEpsilon_t(tabData->calcPoint_NominalStressStrain_yield[0]);
        tabData->calcPoint_TrueStressStrain_yield[1] = LR10KAppGlobals::CalcSigma_t(tabData->calcPoint_NominalStressStrain_yield[0], tabData->calcPoint_NominalStressStrain_yield[1]);
    }
    else
    {
        DockingApps::console.AddLog("[error] Could not find a Yielding point for calculation...");
    }

    // -------------------------------------------------------------------------
    // ------------------ MAXIMUM (ULTIMATE TENSILE STRENGTH) ------------------
    // -------------------------------------------------------------------------
    if (foundUts)
    {
        DockingApps::console.AddLog("Found an Ultimate Tensile Strength point! Calculating...");

        // The engineering strain/nominal stress is: F/A_0 and strain: Delta_L / L_0
        tabData->calcPoint_NominalStressStrainUTS[0] = LR10KAppGlobals::CalcEpsilon_e(ultimateTensileStrengthForceExtension.x, L_0);  // Epsilon_e or Epsilon_n
        tabData->calcPoint_NominalStressStrainUTS[1] = LR10KAppGlobals::CalcSigma_e(ultimateTensileStrengthForceExtension.y, A_0);    // Sigma_e or Sigma_n

        // The true stress is: Sigma_e * ( 1 + Epsilon_e ) and strain: ln * ( 1 + Epsilon_e )
        tabData->calcPoint_TrueStressStrainUTS[0] = LR10KAppGlobals::CalcEpsilon_t(tabData->calcPoint_NominalStressStrainUTS[0]);                                       // Epsilon_t
        tabData->calcPoint_TrueStressStrainUTS[1] = LR10KAppGlobals::CalcSigma_t(tabData->calcPoint_NominalStressStrainUTS[0], tabData->calcPoint_NominalStressStrainUTS[1]);   // Sigma_t
    }
    else
    {
        DockingApps::console.AddLog("[error] Could not find a Ultimate Tensile Strength point for calculation...");
    }

    // -------------------------------------------------------------------------
    // -------------------------------- FRACTURE -------------------------------
    // -------------------------------------------------------------------------
    if (foundFracture)
    {
        DockingApps::console.AddLog("Found a Fracture point! Calculating...");

        // The engineering/nominal stress and strain
        tabData->calcPoint_NominalStressStrain_fracture[0] = LR10KAppGlobals::CalcEpsilon_e(fractureForceExtension.x, L_0);
        tabData->calcPoint_NominalStressStrain_fracture[1] = LR10KAppGlobals::CalcSigma_e(fractureForceExtension.y, A_0);

        // The true stress and strain
        tabData->calcPoint_TrueStressStrain_fracture[0] = LR10KAppGlobals::CalcEpsilon_t(tabData->calcPoint_NominalStressStrain_fracture[0]);
        tabData->calcPoint_TrueStressStrain_fracture[1] = LR10KAppGlobals::CalcSigma_t(tabData->calcPoint_NominalStressStrain_fracture[0], tabData->calcPoint_NominalStressStrain_fracture[1]);
    }
    else
    {
        DockingApps::console.AddLog("[error] Could not find a Fracture point for calculation...");
    }

    // -------------------------------------------------------------------------
    // ------------------ Save any calculations to current tab -----------------
    // -------------------------------------------------------------------------
    if (saveAppSettings) // Default
        DockingApps::mainFileSystem.SaveApplicationSettings();

    if (markAsSaved)
        tabData->MarkTabAsUnsaved();

    if (playSound)
        PlaySoundEffect("buttonClick1");
}

void AppCalculator::CalculateCompressionBendMomentAndUTS(TabControl::TabTestDataClass* tabData, bool playSound, bool markAsSaved, bool saveAppSettings)
{
    const double T_0 = LR10KAppGlobals::calc_thickness;     // Thickness
    const double W_0 = LR10KAppGlobals::calc_width;         // Width
    const double L_0 = LR10KAppGlobals::calc_length;        // Length
    double A_0 = T_0 * W_0;                                 // Area

    tabData->thickness  = T_0;
    tabData->width      = W_0;
    tabData->length     = L_0;

    ImPlotPoint UTS = tabData->GetUTS();

    tabData->calcData_Compression[0] = UTS.y;    // Load             (max y)
    tabData->calcData_Compression[1] = UTS.x;    // Travel distance  (max x)

    // Max bending moment
    tabData->calcData_Compression[2] = (tabData->calcData_Compression[0] * tabData->calcData_Compression[1]) / 4;
    // UTS
    tabData->calcData_Compression[3] = (3 * tabData->calcData_Compression[0] * tabData->calcData_Compression[1]) / (2 * W_0 * sqrt(T_0));

    // -------------------------------------------------------------------------
    // ------------------ Save any calculations to current tab -----------------
    // -------------------------------------------------------------------------
    if (saveAppSettings) // Default
        DockingApps::mainFileSystem.SaveApplicationSettings();

    if (markAsSaved)
        tabData->MarkTabAsUnsaved();

    if (playSound)
        PlaySoundEffect("buttonClick1");
}

void AppCalculator::CalculateStresses(TabControl::TabTestDataClass* tabData)
{
    // -------------------------------------------------------------------------
    // Reset all previous stress point data
    // -------------------------------------------------------------------------
    tabData->LR10KTensileDataNominal    = RollingBuffer();
    tabData->LR10KTensileDataTrue       = RollingBuffer();

    const double L_0 = tabData->length;        // Start Length
    const double A_0 = tabData->area;          // Start Area

    // Convert to nominal/engineering stress and strain
    for (int i = 0; i < tabData->LR10KTensileData.Data.Size; i++)
    {
        ImVec2 tensilDataPointNom;
        tensilDataPointNom.x = LR10KAppGlobals::CalcEpsilon_e(tabData->LR10KTensileData.Data[i].x, L_0);
        tensilDataPointNom.y = LR10KAppGlobals::CalcSigma_e(tabData->LR10KTensileData.Data[i].y, A_0);

        tabData->LR10KTensileDataNominal.AddPoint(tensilDataPointNom.x, tensilDataPointNom.y);

        if (i == tabData->LR10KTensileData.Data.Size - 1)
        {
            // Convert to true stress and strain also
            for (int i = 0; i < tabData->LR10KTensileDataNominal.Data.Size; i++)
            {
                ImVec2 tensilDataPointTrue;
                tensilDataPointTrue.x = LR10KAppGlobals::CalcEpsilon_t(tabData->LR10KTensileDataNominal.Data[i].x);
                tensilDataPointTrue.y = LR10KAppGlobals::CalcSigma_t(tabData->LR10KTensileDataNominal.Data[i].x, tabData->LR10KTensileDataNominal.Data[i].y);

                tabData->LR10KTensileDataTrue.AddPoint(tensilDataPointTrue.x, tensilDataPointTrue.y);
            }
        }
    }
}

void AppCalculator::Draw(TabControl::TabTestDataClass* tabData, bool* showCalculator)
{
    if (tabData == nullptr) return;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 750), ImGuiCond_FirstUseEver);

    ImGui::Begin("Calculator", showCalculator);

    ImGui::SeparatorText("Extra Tools");

    // Calculate percentage of length of material to millimeter
    ImGui::Text("Percentage of\nthe Start length:");
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

    // -------------------------------------------------------------------------
    // -------------------------- Material properties --------------------------
    // -------------------------------------------------------------------------
    ImGui::SeparatorText("Material properties");

    float static check_Thickness            = -1.0f;
    float static check_Width                = -1.0f;
    float static check_Length               = -1.0f;
    float static check_Diameter             = -1.0f;
    int static   check_areaOption           = LR10KAppGlobals::areaOptionPicked;
    float static check_offsetZeroPoint      = defaultValuesGUI::offsetZeroPointExtensionAxis;

    // Check if file was loaded
    float static fileIsLoaded = false;
    std::string static lastLoadedFileName = "";
    if (tabData->loadedFromFileName.size() > 0 && lastLoadedFileName != tabData->loadedFromFileName)
    {
        lastLoadedFileName = tabData->loadedFromFileName;
        fileIsLoaded = true;
    }

    // On initialization
    if (fileIsLoaded)
    {
        fileIsLoaded        = false;
        lastLoadedFileName  = tabData->loadedFromFileName;

        // ---------------------------------------------------------------------

        check_Thickness         = tabData->thickness;
        check_Width             = tabData->width;
        check_Length            = tabData->length;
        check_Diameter          = tabData->diameter;
        check_areaOption        = tabData->areaOptionPicked;

        // Offset from zero point extension axis
        check_offsetZeroPoint   = tabData->LR10OffsetZeroPointExtensionAxis;

        LR10KAppGlobals::calc_thickness     = tabData->thickness;
        LR10KAppGlobals::calc_width         = tabData->width;
        LR10KAppGlobals::calc_length        = tabData->length;
        LR10KAppGlobals::calc_diameter      = tabData->diameter;
        LR10KAppGlobals::areaOptionPicked   = tabData->areaOptionPicked;

        // Offset from zero point extension axis
        defaultValuesGUI::offsetZeroPointExtensionAxis = tabData->LR10OffsetZeroPointExtensionAxis;
    }

    // If the user wants to insert parameters from loaded file
    ImGui::BeginDisabled(!tabData->loadedFromFileName.size() > 0);
    if (ImGui::Button("Load from current tab\ninto global values"))
    {
        check_Thickness     = tabData->thickness;
        check_Width         = tabData->width;
        check_Length        = tabData->length;
        check_Diameter      = tabData->diameter;
        check_areaOption    = tabData->areaOptionPicked;

        // Offset from zero point extension axis
        check_offsetZeroPoint = tabData->LR10OffsetZeroPointExtensionAxis;

        LR10KAppGlobals::calc_thickness     = tabData->thickness;
        LR10KAppGlobals::calc_width         = tabData->width;
        LR10KAppGlobals::calc_length        = tabData->length;
        LR10KAppGlobals::calc_diameter      = tabData->diameter;
        LR10KAppGlobals::areaOptionPicked   = tabData->areaOptionPicked;

        // Offset from zero point extension axis
        defaultValuesGUI::offsetZeroPointExtensionAxis = tabData->LR10OffsetZeroPointExtensionAxis;
    }
    ImGui::SetItemTooltip("If you load from several files, you might need to load the data in again.");
    ImGui::EndDisabled();
    ImGui::Spacing();

    // Have the option to move the starting strain point (x-axis) an offset distance from zero point
    ImGui::Text("Remove some slack from\nthe extension/strain axis:");
    ImGui::Text("0 mm = \"Default setup\".");
    ImGui::Text("Will only affect the\nresult for calculation(s).");
    ImGui::Text("* Fracture point is needed.");
    double fracturePointExtension = tabData->GetFracturePoint(tabData).x;
    ImGui::BeginDisabled(fracturePointExtension <= 0 && !mainGUIGlobals::useDemoMode || !LR10KAppGlobals::toggleGraphType);
    ImGui::InputFloat("##Offset zero point ext.", &defaultValuesGUI::offsetZeroPointExtensionAxis, (0.01f), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(defaultValuesGUI::offsetZeroPointExtensionAxis), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));

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

    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start values
    if (LR10KAppGlobals::areaOptionPicked == 0)
    {
        ImGui::Text("Start thickness (σ_n, σ_t):");
        ImGui::InputFloat("##Start thickness", &LR10KAppGlobals::calc_thickness, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_thickness), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_thickness), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
        ImGui::Text("Start width (σ_n, σ_t):");
        ImGui::InputFloat("##Start width", &LR10KAppGlobals::calc_width, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_width), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_width), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
    }
    else if (LR10KAppGlobals::areaOptionPicked == 1)
    {
        ImGui::Text("Start diameter (σ_n, σ_t):");
        ImGui::InputFloat("##Diameter", &LR10KAppGlobals::calc_diameter, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_diameter), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_diameter), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));
    }

    ImGui::Text("Start length (ε_n, σ_t, ε_t):");
    ImGui::InputFloat("##Start length", &LR10KAppGlobals::calc_length, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(LR10KAppGlobals::calc_length), (0.1f), LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::calc_length), ImGuiInputTextFlags_CharsScientific); ImGui::SameLine(); ImGui::Text(LR10KAppGlobals::GetUnitOption<const char*>(0));

    ImGui::Text("Cross-sectional area:");
    bool option1_rectangular    = ImGui::RadioButton("Rectangle", &LR10KAppGlobals::areaOptionPicked, 0); ImGui::SameLine();
    bool option2_circual        = ImGui::RadioButton("Circle", &LR10KAppGlobals::areaOptionPicked, 1);
    if (option1_rectangular || option2_circual) DockingApps::mainFileSystem.SaveApplicationSettings();
    
    if (!std::isnan(tabData->area) && tabData->area > -1)
    {
        std::string str = "(area calculated: %.2f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
        str.append("^2)");
        ImGui::Text(str.c_str(), tabData->area);
    }

    // Warning if any values have changed but no calculated again
    static ImVec4 colorNormal  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    static ImVec4 colorNormal2  = ImVec4(0.695f, 1.000f, 0.442f, 0.784f);
    static ImVec4 colorNormal3  = ImVec4(0.647f, 1.000f, 0.780f, 0.784f);
    static ImVec4 colorWarning = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

    ImVec4 colorText        = colorNormal;
    ImVec4 colorText_nom    = colorNormal3;
    ImVec4 colorText2       = colorNormal2;
    if (
        check_Thickness != LR10KAppGlobals::calc_thickness
        || check_Width != LR10KAppGlobals::calc_width
        || check_Length != LR10KAppGlobals::calc_length
        || check_Diameter != LR10KAppGlobals::calc_diameter
        || check_areaOption != LR10KAppGlobals::areaOptionPicked
        || check_offsetZeroPoint != defaultValuesGUI::offsetZeroPointExtensionAxis
        )
    {
        // Only needed to warn about a change if a test is finished
        if (tabData->test_finished)
        {
            colorText       = colorWarning;
            colorText_nom   = colorWarning;
            colorText2      = colorWarning;

            defaultValuesGUI::invalidStressDataNotUpdated = true;
        }
    }
    else
        defaultValuesGUI::invalidStressDataNotUpdated = false;


    ImGui::Spacing();

    // Output the calculations
    std::string str = "Strength (";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(3));
    str.append(" | ");
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
    str.append(" | σ)");
    ImGui::SeparatorText(str.c_str());
    ImGui::Spacing();

    bool unvalidCalculationStart =
        (!tabData->test_finished || tabData->plot_dataPoints.size() < 1
        || (LR10KAppGlobals::calc_length == 0 && (LR10KAppGlobals::calc_thickness == 0 || LR10KAppGlobals::calc_width == 0)));

    ImGui::BeginDisabled(unvalidCalculationStart);
    bool calcButton = ImGui::Button("Calculate", { 219, 30 });
    ImGui::SetItemTooltip("Material properties will be saved in GUI after each calculation");
    if (calcButton)
    {
        check_Thickness         = LR10KAppGlobals::calc_thickness;
        check_Width             = LR10KAppGlobals::calc_width;
        check_Length            = LR10KAppGlobals::calc_length;
        check_Diameter          = LR10KAppGlobals::calc_diameter;
        check_areaOption        = LR10KAppGlobals::areaOptionPicked;

        // Offset from zero point extension axis
        check_offsetZeroPoint   = defaultValuesGUI::offsetZeroPointExtensionAxis;

        TabControl::CalculateDataFromTest(tabData);
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
    {
        // Calculate Young's Moduls (stress / strain at prop. limit point)
        tabData->calcYoungsModulus = (tabData->calcPoint_NominalStressStrain_PropLimit[1] / tabData->calcPoint_NominalStressStrain_PropLimit[0]);
        if (isnan(tabData->calcYoungsModulus))
            tabData->calcYoungsModulus = 0.000f;

        ImGui::Text(">>PROPORTIONAL LIMIT:");
        ImGui::Separator();
        std::string str1 = "Stress, σ_nom/σ_e: %0.3f ";
        str1.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
        ImGui::TextColored(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_PropLimit[1]);
        ImGui::TextColored(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_PropLimit[0] * 100);
        ImGui::Separator();
        std::string str2 = "Young's modulus,\nE = %0.3f ";
        str2.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
        ImGui::TextColored(colorText2, str2.c_str(), tabData->calcYoungsModulus);
        ImGui::Separator();
        std::string str3 = "Stress, σ_t: %0.3f ";
        str3.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
        ImGui::TextColored(colorText, str3.c_str(), tabData->calcPoint_TrueStressStrain_PropLimit[1]);
        ImGui::TextColored(colorText, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_PropLimit[0] * 100);
        ImGui::Separator();

        ImGui::NewLine();

        ImGui::Text(">>YIELDING STRENGTH (lower):");
        ImGui::Separator();
        ImGui::TextColored(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_yield[1]);
        ImGui::TextColored(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_yield[0] * 100);
        ImGui::Separator();
        ImGui::TextColored(colorText, str3.c_str(), tabData->calcPoint_TrueStressStrain_yield[1]);
        ImGui::TextColored(colorText, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_yield[0] * 100);
        ImGui::Separator();

        ImGui::NewLine();

        ImGui::Text(">>ULTIMATE TENSILE STRENGTH:");
        ImGui::Separator();
        ImGui::TextColored(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrainUTS[1]);
        ImGui::TextColored(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrainUTS[0] * 100);
        ImGui::Separator();
        ImGui::TextColored(colorText, str3.c_str(), tabData->calcPoint_TrueStressStrainUTS[1]);
        ImGui::TextColored(colorText, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrainUTS[0] * 100);
        ImGui::Separator();

        ImGui::NewLine();

        ImGui::Text(">>FRACTURE:");
        ImGui::Separator();
        ImGui::TextColored(colorText_nom, str1.c_str(), tabData->calcPoint_NominalStressStrain_fracture[1]);
        ImGui::TextColored(colorText_nom, "Strain, ε_nom/ε_e %%: %0.2f %%", tabData->calcPoint_NominalStressStrain_fracture[0] * 100);
        ImGui::Separator();
        ImGui::TextColored(colorText, str3.c_str(), tabData->calcPoint_TrueStressStrain_fracture[1]);
        ImGui::TextColored(colorText, "Strain, ε_t %%: %0.2f %%", tabData->calcPoint_TrueStressStrain_fracture[0] * 100);
        ImGui::Separator();
    }
    else if (!tabData->tension && 1 == tabData->compressionOptionPicked)
    {
        // Calculate Young's Moduls
        tabData->calcYoungsModulus = (tabData->calcData_Compression[0] * pow(tabData->length, 3)) / (4 * tabData->width * pow(tabData->thickness, 3) * tabData->calcData_Compression[1]);
        if (isnan(tabData->calcYoungsModulus))
            tabData->calcYoungsModulus = 0.000f;

        ImGui::Text(">>ULTIMATE TENSILE STRENGTH:");
        ImGui::Separator();
        str = "UTS_bend: %0.2f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
        ImGui::TextColored(colorText_nom, str.c_str(), tabData->calcData_Compression[3]);
        ImGui::Separator();
        str = "Young's modulus,\nE_bend = %0.3f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
        ImGui::TextColored(colorText2, str.c_str(), tabData->calcYoungsModulus);
        ImGui::Separator();

        ImGui::NewLine();

        ImGui::Text(">>MAXIMUM BENDING MOMENT:");
        ImGui::Separator();
        str = "Moment_bend: %0.2f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(1)).append(LR10KAppGlobals::GetUnitOption<const char*>(0)); // Nmm or lbf * in
        ImGui::TextColored(colorText_nom, str.c_str(), tabData->calcData_Compression[2]);
        ImGui::Separator();

        ImGui::NewLine();

        ImGui::Text(">>MAXIMUM LOAD & TRAVEL:");
        ImGui::Separator();
        std::string str = "Load_bend: %0.2f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
        ImGui::TextColored(colorText, str.c_str(), tabData->calcData_Compression[0]);
        ImGui::Separator();
        str = "Travel_bend: %0.2f ";
        str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
        ImGui::TextColored(colorText, str.c_str(), tabData->calcData_Compression[1]);
        ImGui::Separator();
    }

    ImGui::NewLine();

    ImGui::Text("Calculations will be saved\nin Excel Books also.");

    ImGui::Separator();
    ImGui::Text("Complete test, and add \ncrucial points to\nget started.");

    ImGui::Separator();

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

/**
    @brief  Function to perform linear regression and calculate R²
    @details If this value is above 0.9, it is very certain of linear regression.
    @param  data - 
    @retval      - 
**/
double linearRegressionR2(const std::vector<DataPoint>& data) {
    // Calculate means of x and y values
    double xMean = calculateMean(data, &DataPoint::x);
    double yMean = calculateMean(data, &DataPoint::y);

    // Calculate slope (m) and intercept (b) of the linear equation
    double numerator = 0.0;
    double denominator = 0.0;
    for (const DataPoint& point : data) {
        double xDiff = point.x - xMean;
        double yDiff = point.y - yMean;
        numerator += xDiff * yDiff;
        denominator += xDiff * xDiff;
    }
    double slope = numerator / denominator;
    double intercept = yMean - slope * xMean;

    // Calculate R²
    double totalVariation = 0.0;
    double regressionVariation = 0.0;
    for (const DataPoint& point : data) {
        double predictedY = slope * point.x + intercept;
        totalVariation += pow(point.y - yMean, 2);
        regressionVariation += pow(predictedY - yMean, 2);
    }
    double rSquared = regressionVariation / totalVariation;

    return rSquared;
}

// -----------------------------------------------------------------------------
// ------------------------------- Excel related -------------------------------
// -----------------------------------------------------------------------------

std::pair<std::vector<double>, std::vector<double>> AppMainExcel::InsertLineCells(CWorkbook* book, CWorksheet* sheet, RollingBuffer* plotData, std::vector<ImPlotPoint> impDataPoints, bool stressData, const double A_0, const double L_0, std::vector<double> sigma_e, std::vector<double> epsilon_e)
{
    Style style;
    style.horizAlign = ALIGN_H_CENTER;
    style.numFormat.numberStyle = NUMSTYLE_NUMERIC;
    style.numFormat.numberOfDigitsAfterPoint = 3; // !!Important!!
    const size_t smallTitleCenterdStyleIndex = book->AddStyle(style);

    style.numFormat.numberOfDigitsAfterPoint = 1;
    const size_t smallTitle2CenterdStyleIndex = book->AddStyle(style);

    // Save the sigma_e and epsilon_e values (can be used later if needed like i.e. for true stress)
    bool addNominalStressStrainValues = (sigma_e.size() == 0 && epsilon_e.size() == 0) ? true : false;

    // -------------------------------------------------------------------------
    // ------------------------------- Plot data -------------------------------
    // -------------------------------------------------------------------------

    int amountOfPointsAdded = 0;

    // Keep control over how many points are added for each crucial point, so it doesn't repeat
    bool crucialPointIndexControlHaveAddedPoint[4]{ false, false, false, false };

    // "Remove" the plot data was not included because of offset extension set
    RollingBuffer filteredPlotData = RollingBuffer();
    for (int i = 0; i < plotData->Data.size(); i++)
    {
        if (plotData->Data[i].x >= defaultValuesGUI::offsetZeroPointExtensionAxis)
            filteredPlotData.AddPoint(plotData->Data[i].x, plotData->Data[i].y);
    }

    for (int i = 0; i < filteredPlotData.Data.size(); i++)
    {
        ImVec2 dataPlot = filteredPlotData.Data[i];

        // If offset zero point from extension axis is set, then don't add
        // the points lower than this to the Excel sheet. This is only valid for normal stress test
        if (defaultValuesGUI::offsetZeroPointExtensionAxis != 0 && Extra::tools.SetPrecisionFloat(dataPlot.x, 3) < Extra::tools.SetPrecisionFloat(defaultValuesGUI::offsetZeroPointExtensionAxis, 3))
            continue;

        if (stressData && addNominalStressStrainValues)
        {
            // Redefine for Nominal/Engineering Stress
            dataPlot.x = LR10KAppGlobals::CalcEpsilon_e(dataPlot.x, L_0);
            dataPlot.y = LR10KAppGlobals::CalcSigma_e(dataPlot.y, A_0);

            // Finished converted to stress-strain points
            epsilon_e.push_back(dataPlot.x);
            sigma_e.push_back(dataPlot.y);
        }
        else if (stressData)
        {
            // Redefined for True Stress
            dataPlot.x = LR10KAppGlobals::CalcEpsilon_t(epsilon_e[i]);
            dataPlot.y = LR10KAppGlobals::CalcSigma_t(epsilon_e[i], sigma_e[i]);
        }

        // Used for checking if a graph without crucial points needs to be added
        bool haveAlreadyAddedData = false;

        // Check each time if there is a match
        if ((!stressData || addNominalStressStrainValues) && amountOfPointsAdded < impDataPoints.size())
        {
            for (int j = 0; j < impDataPoints.size(); j++)
            {
                ImPlotPoint dataPoint = impDataPoints[j];

                if (stressData)
                {
                    // Redefine data point for Nominal Stress
                    dataPoint.x = LR10KAppGlobals::CalcEpsilon_e(dataPoint.x, L_0);
                    dataPoint.y = LR10KAppGlobals::CalcSigma_e(dataPoint.y, A_0);
                }

                // -------------------------------------------------------------
                //  Find matches to draw the crucial point into the Excel sheet 
                // -------------------------------------------------------------

                // Since the point is snapped onto the graph, we only need to check the X-axis. And also need to round of since nominal stress also uses this function
                bool pointCheckForMatchForX = Extra::tools.SetPrecisionFloat(dataPlot.x, 6) == Extra::tools.SetPrecisionFloat(dataPoint.x, 6);

                if (pointCheckForMatchForX && !crucialPointIndexControlHaveAddedPoint[j])
                {
                    crucialPointIndexControlHaveAddedPoint[j] = true;

                    // ---------------------------------------------------------
                    // ---------- Add data with crucial point(s) data ----------
                    // ---------------------------------------------------------

                    // When the axis is extended above last load point, we only want to extend and not draw a graph line
                    if (dataPlot.y == 0)
                    {
                        sheet->BeginRow()
                            .AddCell(dataPlot.x, smallTitleCenterdStyleIndex)
                            .AddCell("")
                            .AddCell("")
                            .AddCell(dataPoint.x, smallTitleCenterdStyleIndex)
                            .AddCell(dataPoint.y, smallTitle2CenterdStyleIndex)
                            .EndRow();
                    }
                    else
                    {
                        sheet->BeginRow()
                            .AddCell(dataPlot.x, smallTitleCenterdStyleIndex)
                            .AddCell(dataPlot.y, smallTitle2CenterdStyleIndex)
                            .AddCell("")
                            .AddCell(dataPoint.x, smallTitleCenterdStyleIndex)
                            .AddCell(dataPoint.y, smallTitle2CenterdStyleIndex)
                            .EndRow();
                    }

                    amountOfPointsAdded++; haveAlreadyAddedData = true;
                }
            }
        }

        // ---------------------------------------------------------------------
        // --------------- Add data without crucial point(s) data --------------
        // ---------------------------------------------------------------------

        if (!haveAlreadyAddedData || !addNominalStressStrainValues)
        {
            if (dataPlot.y == 0)
            {
                sheet->BeginRow()
                    .AddCell(dataPlot.x, smallTitleCenterdStyleIndex)
                    .AddCell("")
                    .AddCell("")
                    .AddCell("")
                    .AddCell("")
                    .EndRow();
            }
            else
            {
                sheet->BeginRow()
                    .AddCell(dataPlot.x, smallTitleCenterdStyleIndex)
                    .AddCell(dataPlot.y, smallTitle2CenterdStyleIndex)
                    .AddCell("")
                    .AddCell("")
                    .AddCell("")
                    .EndRow();
            }
        }
    }

    // Save changes...
    plotData = &filteredPlotData;

    return std::make_pair(epsilon_e, sigma_e);
}

void AppMainExcel::AddLR10KLineChart(CWorkbook& book, CWorksheet& sheet, XLSXColorLib& cl_lib, XLSXColorLib& gs10_lib, int plotAxisAmountOfPoints, int pointAxisAmountOfPoints, const std::string date)
{
    // Adding chart to the workbook the reference to a newly created object is returned
    CChartsheet& chart_sheet = book.AddChartSheet("Plot Graph", CHART_LINEAR);

    std::string diagramName = "DIRECT INPUT LOAD-EXTENSION\n\n";
    diagramName.append("LLOYD LR 10K Tensile Machine Test\n");
    diagramName.append(date.c_str());

    chart_sheet.Chart().SetDiagrammName(diagramName.c_str());
    chart_sheet.Chart().SetLegendPos(CChart::POS_TOP);

    std::string str = "Extension (";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
    str.append(")");
    chart_sheet.Chart().SetXAxisName(str.c_str());
    str = "Load (";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(1));
    str.append(")");
    chart_sheet.Chart().SetYAxisName(str.c_str());

    chart_sheet.Chart().SetXAxisToCat(false);
    chart_sheet.Chart().SetYAxisToCat(true);

    /*chart_sheet.Chart().SetXAxisGrid(CChart::GRID_MAJOR_N_MINOR);
    chart_sheet.Chart().SetYAxisGrid(CChart::GRID_MAJOR_N_MINOR);
    chart_sheet.Chart().SetYAxisCross(CChart::CROSS_MIN);
    chart_sheet.Chart().SetXAxisCross(CChart::CROSS_MIN);*/

    // -------------------------------------------------------------------------
    // ------------------------------- PLOT GRAPH ------------------------------
    // -------------------------------------------------------------------------

    // Create series object, that contains most chart settings
    CChart::Series series_plotGraph;

    // Leave category sequence (X axis) not specified (optional) - MS Excel will generate the default sequence automatically
    series_plotGraph.catSheet = &sheet;
    series_plotGraph.catAxisFrom = CellCoord(5, 0);
    series_plotGraph.catAxisTo = CellCoord(plotAxisAmountOfPoints, 0);

    // Specify range for values` sequence (Y axis)
    series_plotGraph.valSheet = &sheet; // Don`t forget to set the pointer to the data sheet
    series_plotGraph.valAxisFrom = CellCoord(5, 1);
    series_plotGraph.valAxisTo = CellCoord(plotAxisAmountOfPoints, 1);

    series_plotGraph.JoinType = CChart::Series::joinSmooth;
    series_plotGraph.DashType = CChart::Series::dashSolid;
    series_plotGraph.LineColor = "D22B2B"; //gs10_lib.GetColor("Black");

    series_plotGraph.Marker.Type = CChart::Series::symNone;
    series_plotGraph.Marker.LineWidth = 1;
    series_plotGraph.title = "Plot Data";

    // -------------------------------------------------------------------------
    // ------------------------------ PLOT POINTS ------------------------------
    // -------------------------------------------------------------------------

    // Create series object, that contains most chart settings
    CChart::Series series_plotPoint;

    // Leave category sequence (X axis) not specified (optional) - MS Excel will generate the default sequence automatically
    series_plotPoint.catSheet = &sheet;
    series_plotPoint.catAxisFrom = CellCoord(5, 3);
    series_plotPoint.catAxisTo = CellCoord(pointAxisAmountOfPoints, 3);

    // Specify range for values` sequence (Y axis)
    series_plotPoint.valSheet = &sheet; // Don`t forget to set the pointer to the data sheet
    series_plotPoint.valAxisFrom = CellCoord(5, 4);
    series_plotPoint.valAxisTo = CellCoord(pointAxisAmountOfPoints, 4);

    series_plotPoint.JoinType = CChart::Series::joinNone;

    series_plotPoint.Marker.Type = CChart::Series::symCircle;
    series_plotPoint.Marker.Size = 5;
    series_plotPoint.Marker.FillColor = "0494d2"; //cl_lib.GetColor("Gold");
    series_plotPoint.Marker.LineColor = "04a6eb"; //cl_lib.GetColor("Gold"); //cl_lib.GetColor("Plum");
    series_plotPoint.Marker.LineWidth = 1.;
    series_plotPoint.title = "Crucial Point(s)";

    series_plotPoint.dataLabels.showVal = true; // Doesn't appear to do anything

    // Add series into the chart (you can add as many series as you wish into the same chart)
    chart_sheet.Chart().AddSeries(series_plotGraph);
    chart_sheet.Chart().AddSeries(series_plotPoint);
}

void AppMainExcel::AddStressLineChart(CWorkbook& book, CWorksheet& sheet, XLSXColorLib& cl_lib, XLSXColorLib& gs10_lib, int plotAxisAmountOfPoints, int pointAxisAmountOfPoints, const std::string date, bool itIsNominalStress)
{
    // Adding chart to the workbook the reference to a newly created object is returned
    CChartsheet& chart_sheet = book.AddChartSheet(itIsNominalStress ? "Stress Graph 1" : "Stress Graph 2", CHART_LINEAR);

    std::string diagramName = itIsNominalStress ? "NOMINAL (ENGINEERING)" : "TRUE";
    diagramName.append(" STRESS\n\n");
    diagramName.append("LLOYD LR 10K Tensile Machine Test\n");
    diagramName.append(date.c_str());

    chart_sheet.Chart().SetDiagrammName(diagramName.c_str());
    chart_sheet.Chart().SetLegendPos(CChart::POS_TOP);

    std::string str = itIsNominalStress ? "Nominal Stress (σ_nom/ε_e) [" : "True Stress (σ_t) [";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(7));
    str.append("]");
    chart_sheet.Chart().SetXAxisName(itIsNominalStress ? "Nominal Strain (ε_nom/ε_e)" : "True Strain (ε_t)");
    chart_sheet.Chart().SetYAxisName(str.c_str());

    chart_sheet.Chart().SetXAxisToCat(false);
    chart_sheet.Chart().SetYAxisToCat(true);

    // -------------------------------------------------------------------------
    // --------------------------- PLOT STRESS GRAPH ---------------------------
    // -------------------------------------------------------------------------

    // Create series object, that contains most chart settings
    CChart::Series series_plotGraph;

    // Leave category sequence (X axis) not specified (optional) - MS Excel will generate the default sequence automatically
    series_plotGraph.catSheet = &sheet;
    series_plotGraph.catAxisFrom = CellCoord(5, 0);
    series_plotGraph.catAxisTo = CellCoord(plotAxisAmountOfPoints, 0);

    // Specify range for values` sequence (Y axis)
    series_plotGraph.valSheet = &sheet; // Don`t forget to set the pointer to the data sheet
    series_plotGraph.valAxisFrom = CellCoord(5, 1);
    series_plotGraph.valAxisTo = CellCoord(plotAxisAmountOfPoints, 1);

    series_plotGraph.JoinType = CChart::Series::joinSmooth;
    series_plotGraph.DashType = CChart::Series::dashSolid;
    series_plotGraph.LineColor = "D22B2B"; // gs10_lib.GetColor("Black");

    series_plotGraph.Marker.Type = CChart::Series::symNone;
    series_plotGraph.Marker.LineWidth = 1.;
    series_plotGraph.title = itIsNominalStress ? "Nominal Stress" : "True Stress";

    // Add series into the chart (you can add as many series as you wish into the same chart)
    chart_sheet.Chart().AddSeries(series_plotGraph);

    if (itIsNominalStress)
    {
        // ---------------------------------------------------------------------
        // ---------------------------- PLOT POINTS ----------------------------
        // ---------------------------------------------------------------------

        // Create series object, that contains most chart settings
        CChart::Series series_plotPoint;

        // Leave category sequence (X axis) not specified (optional) - MS Excel will generate the default sequence automatically
        series_plotPoint.catSheet = &sheet;
        series_plotPoint.catAxisFrom = CellCoord(5, 3);
        series_plotPoint.catAxisTo = CellCoord(pointAxisAmountOfPoints, 3);

        // Specify range for values` sequence (Y axis)
        series_plotPoint.valSheet = &sheet; // Don`t forget to set the pointer to the data sheet
        series_plotPoint.valAxisFrom = CellCoord(5, 4);
        series_plotPoint.valAxisTo = CellCoord(pointAxisAmountOfPoints, 4);

        series_plotPoint.JoinType = CChart::Series::joinNone;

        series_plotPoint.Marker.Type = CChart::Series::symCircle;
        series_plotPoint.Marker.Size = 5;
        series_plotPoint.Marker.FillColor = "0494d2"; //cl_lib.GetColor("Gold");
        series_plotPoint.Marker.LineColor = "04a6eb"; //cl_lib.GetColor("Gold"); //cl_lib.GetColor("Plum");
        series_plotPoint.Marker.LineWidth = 1.;
        series_plotPoint.title = "Nominal Stress Point(s)";

        series_plotPoint.dataLabels.showVal = true; // Doesn't appear to do anything

        chart_sheet.Chart().AddSeries(series_plotPoint);
    }
}

bool AppMainExcel::SavePlotDataAsAnXlsxFile(TabControl::TabTestDataClass tabData, const char* filename)
{
    // Get the filename the user wants to save it as
    std::string name = filename;
    std::string extension = ".xlsx";

    // Figure out if file extension ending is needed
    std::size_t extensionFound = name.find(extension);
    if (extensionFound == std::string::npos) name.append(extension.c_str()); // Extension not found, so add

    // ------------------------------------------
    // Init color libs.
    XLSXColorLib cl_lib, gs10_lib;
    make_excell_like_named_colors(cl_lib);
    make_grayscale10(gs10_lib);

    // Main object - workbook
    CWorkbook book("LR 10K");

    Style style;
    style.font.attributes = FONT_BOLD;
    const size_t smallTitleStyleIndex = book.AddStyle(style);

    style.horizAlign = ALIGN_H_CENTER;
    const size_t smallTitleCenterdStyleIndex = book.AddStyle(style);

    style.horizAlign = ALIGN_H_CENTER;
    style.font.attributes = FONT_BOLD;
    style.wrapText = false;
    const size_t CenterStyleIndex = book.AddStyle(style);

    Style stPanel;
    stPanel.font.attributes = FONT_BOLD;
    stPanel.horizAlign = ALIGN_H_LEFT;
    stPanel.border.top.color = "FF000000";
    stPanel.border.bottom.color = "FF000000";
    stPanel.fill.patternType = PATTERN_SOLID;
    stPanel.fill.fgColor = "FFFFFFCC";
    stPanel.numFormat.numberStyle = NUMSTYLE_NUMERIC;
    stPanel.numFormat.numberOfDigitsAfterPoint = 3; // !!Important!!
    const size_t alignLeftStyleIndex = book.AddStyle(stPanel);

    stPanel.fill.fgColor = "FFCCCCFF";
    const size_t alignLeft2StyleIndex = book.AddStyle(stPanel);

    stPanel.fill.fgColor = "FFE6FFCC";
    const size_t alignLeft3StyleIndex = book.AddStyle(stPanel);

    stPanel.fill.fgColor = "FFFFFFCC";
    stPanel.numFormat.numberStyle = NUMSTYLE_PERCENTAGE;
    stPanel.numFormat.numberOfDigitsAfterPoint = 2;
    const size_t alignLeftStylePercentageIndex = book.AddStyle(stPanel);

    stPanel.fill.fgColor = "FFCCCCFF";
    const size_t alignLeftStylePercentage2Index = book.AddStyle(stPanel);

    Style stDateTime;
    stDateTime.font.attributes = FONT_BOLD;
    stDateTime.numFormat.numberStyle = NUMSTYLE_DATETIME;
    const size_t dateTimeStyleIndex = book.AddStyle(stDateTime);

    Style stText;
    stText.font.attributes = FONT_ITALIC;
    const size_t text1StyleIndex = book.AddStyle(stText);

    stText.font.attributes = FONT_UNDERLINED;
    const size_t text2StyleIndex = book.AddStyle(stText);

    // -------------------------------------------------------------------------
    // --------------- Data sheet to store data to be referenced ---------------
    // -------------------------------------------------------------------------

    std::vector<ColumnWidth> Col1Width;
    Col1Width.push_back(ColumnWidth(0, 2, 20));

    bool dimensionsAdded = tabData.thickness > 0 && tabData.width > 0 && tabData.length > 0; if (dimensionsAdded)
    {
        // Add nominal and true stresses found for ultimate tensile strength and yielding in its own book
        std::vector<ColumnWidth> ColWidth2;
        ColWidth2.push_back(ColumnWidth(0, 3, 30));
        CWorksheet& sheetStresses = book.AddSheet("Full Report", ColWidth2);

        // Material properties
        sheetStresses.BeginRow().AddCell("MATERIAL TESTING REPORT", CenterStyleIndex).EndRow();
        sheetStresses.MergeCells(CellCoord(1, 0), CellCoord(1, 2));
        sheetStresses.BeginRow().AddCell(tabData.date, dateTimeStyleIndex).EndRow();
        // ---------------------------------------------------------------------
        // ------------------------------ Details ------------------------------
        // ---------------------------------------------------------------------
        // Styling
        Style stDetailsText;

        stDetailsText.fill.fgColor          = "FFECECEC";
        stDetailsText.border.bottom.style   = BORDER_THICK;
        stDetailsText.border.top.style      = BORDER_THICK;
        stDetailsText.border.left.style     = BORDER_THICK;
        stDetailsText.border.right.style    = BORDER_THICK;
        stDetailsText.border.bottom.color   = "FFFF8B00";
        stDetailsText.border.top.color      = "FFFF8B00";
        stDetailsText.border.left.color     = "FFFF8B00";
        stDetailsText.border.right.color    = "FFFF8B00";

        stDetailsText.horizAlign    = EAlignHoriz::ALIGN_H_LEFT;
        stDetailsText.vertAlign     = EAlignVert::ALIGN_V_TOP;
        stDetailsText.wrapText      = true;

        Style stDetailsBorder;
        stDetailsBorder.fill.fgColor        = "FFECECEC";
        stDetailsBorder.border.top.style    = BORDER_THICK;
        stDetailsBorder.border.top.color    = "FFFF8B00";
        stDetailsBorder.border.bottom.style = BORDER_THICK;
        stDetailsBorder.border.bottom.color = "FFFF8B00";
        stDetailsBorder.border.left.style   = BORDER_THICK;
        stDetailsBorder.border.left.color   = "FFFF8B00";
        stDetailsBorder.border.right.style  = BORDER_THICK;
        stDetailsBorder.border.right.color  = "FFFF8B00";

        const size_t DetailsStyleIndex          = book.AddStyle(stDetailsText);
        const size_t DetailsBorderStyleIndex    = book.AddStyle(stDetailsBorder);

        // Merge cells to one big cell (B3)
        sheetStresses.MergeCells(CellCoord(3, 1), CellCoord(7, 3));

        // Add cell
        sheetStresses.BeginRow().AddCell("Details:", text1StyleIndex).AddCell(tabData.description.c_str(), DetailsStyleIndex).AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).EndRow();
        sheetStresses.BeginRow().AddCell("").AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).EndRow();
        sheetStresses.BeginRow().AddCell("").AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).EndRow();
        sheetStresses.BeginRow().AddCell("").AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).EndRow();
        sheetStresses.BeginRow().AddCell("").AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).AddCell("", DetailsBorderStyleIndex).EndRow();

        // ---------------------------------------------------------------------
        // ---------------------------------------------------------------------

        sheetStresses.BeginRow().AddCell("Material Properties:", smallTitleStyleIndex).EndRow();
        sheetStresses.BeginRow().AddCell("").EndRow();
        
        sheetStresses.BeginRow().AddCell("Start Thickness").AddCell(tabData.thickness, alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(0)).EndRow();
        sheetStresses.BeginRow().AddCell("Start Width").AddCell(tabData.width, alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(0)).EndRow();
        sheetStresses.BeginRow().AddCell("Start Length").AddCell(tabData.length, alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(0)).EndRow();

        sheetStresses.BeginRow().AddCell("").EndRow();

        if (tabData.tension || (!tabData.tension && 0 == tabData.compressionOptionPicked))
        {
            // Stresses calculated
            sheetStresses.BeginRow().AddCell("Proportional limit:", smallTitleStyleIndex).AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Stress, σ_nom/σ_e (S_y)").AddCell(tabData.calcPoint_NominalStressStrain_PropLimit[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Strain, ε_n").AddCell(tabData.calcPoint_NominalStressStrain_PropLimit[0], alignLeftStylePercentageIndex).EndRow();
            sheetStresses.BeginRow().AddCell("Young's modulus, E", text2StyleIndex).AddCell(tabData.calcYoungsModulus, alignLeft3StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("True Stress, σ_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_PropLimit[1], alignLeft2StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("True Strain, ε_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_PropLimit[0], alignLeftStylePercentage2Index).AddCell("").EndRow();

            sheetStresses.BeginRow().AddCell("").EndRow();

            sheetStresses.BeginRow().AddCell("Yielding (lower):", smallTitleStyleIndex).AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Stress, σ_nom/σ_e (S_y)").AddCell(tabData.calcPoint_NominalStressStrain_yield[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Strain, ε_n").AddCell(tabData.calcPoint_NominalStressStrain_yield[0], alignLeftStylePercentageIndex).EndRow();
            sheetStresses.BeginRow().AddCell("True Stress, σ_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_yield[1], alignLeft2StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("True Strain, ε_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_yield[0], alignLeftStylePercentage2Index).AddCell("").EndRow();

            sheetStresses.BeginRow().AddCell("").EndRow();

            sheetStresses.BeginRow().AddCell("Ultimate Tensile Strength:", smallTitleStyleIndex).EndRow();
            sheetStresses.BeginRow().AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Stress, σ_nom/σ_e (S_ut)").AddCell(tabData.calcPoint_NominalStressStrainUTS[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Strain, ε_n").AddCell(tabData.calcPoint_NominalStressStrainUTS[0], alignLeftStylePercentageIndex).EndRow();
            sheetStresses.BeginRow().AddCell("True Stress, σ_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrainUTS[1], alignLeft2StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("True Strain, ε_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrainUTS[0], alignLeftStylePercentage2Index).EndRow();

            sheetStresses.BeginRow().AddCell("").EndRow();

            sheetStresses.BeginRow().AddCell("Fracture:", smallTitleStyleIndex).AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Stress, σ_nom/σ_e (S_y)").AddCell(tabData.calcPoint_NominalStressStrain_fracture[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("Nominal Strain, ε_n").AddCell(tabData.calcPoint_NominalStressStrain_fracture[0], alignLeftStylePercentageIndex).EndRow();
            sheetStresses.BeginRow().AddCell("True Stress, σ_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_fracture[1], alignLeft2StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("True Strain, ε_t", text1StyleIndex).AddCell(tabData.calcPoint_TrueStressStrain_fracture[0], alignLeftStylePercentage2Index).AddCell("").EndRow();
        }
        else if (!tabData.tension && 1 == tabData.compressionOptionPicked)
        {
            // Stresses calculated
            sheetStresses.BeginRow().AddCell("Ultimate Tensile Strength:", smallTitleStyleIndex).EndRow();
            sheetStresses.BeginRow().AddCell("Bending Stress, σ_bend (S_bend)").AddCell(tabData.calcData_Compression[3], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();
            sheetStresses.BeginRow().AddCell("Young's modulus, E", text2StyleIndex).AddCell(tabData.calcYoungsModulus, alignLeft3StyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(6)).EndRow();

            sheetStresses.BeginRow().AddCell("").EndRow();

            // Bending moment
            sheetStresses.BeginRow().AddCell("Maximum bending moment:", smallTitleStyleIndex).AddCell("").EndRow();
            std::string str = LR10KAppGlobals::GetUnitOption<const char*>(1); str.append(LR10KAppGlobals::GetUnitOption<const char*>(0));
            sheetStresses.BeginRow().AddCell("Moment Bend Stress:").AddCell(tabData.calcData_Compression[2], alignLeftStyleIndex).AddCell(str.c_str()).EndRow();

            sheetStresses.BeginRow().AddCell("").EndRow();

            // Load and travel
            sheetStresses.BeginRow().AddCell("Maximum load & travel:", smallTitleStyleIndex).AddCell("").EndRow();
            sheetStresses.BeginRow().AddCell("Load:").AddCell(tabData.calcData_Compression[0], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(1)).EndRow();
            sheetStresses.BeginRow().AddCell("Travel:").AddCell(tabData.calcData_Compression[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(0)).EndRow();
        }
    }

    CWorksheet& sheetSettings = book.AddSheet("Setup", Col1Width);

    sheetSettings.BeginRow().AddCell("LR 10K Tensile Machine Settings", CenterStyleIndex).EndRow();
    sheetSettings.MergeCells(CellCoord(1, 0), CellCoord(1, 2));
    sheetSettings.BeginRow().AddCell(tabData.date, dateTimeStyleIndex).EndRow();
    sheetSettings.BeginRow().AddCell("Details:", text1StyleIndex).AddCell("...", smallTitleStyleIndex).EndRow();
    sheetSettings.BeginRow().AddCell("").EndRow();

    sheetSettings.BeginRow().AddCell("Crosshead Speed").AddCell(tabData.settings_speedAndPlot[0], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(2)).EndRow();
    sheetSettings.BeginRow().AddCell("(Jog speed)").AddCell(tabData.settings_speedAndPlot[1], alignLeftStyleIndex).AddCell(LR10KAppGlobals::GetUnitOption<const char*>(2)).EndRow(); // THIS IS IRRELEVANT FOR THE TEST, AND CAN CHANGE AFTER TEST IS FINISHED
    sheetSettings.BeginRow().AddCell("Load error (graph)").AddCell(tabData.settings_speedAndPlot[2], alignLeftStyleIndex).AddCell("%").EndRow();

    // -------------------------------------------------------------------------
    // --------------- Data sheet to store data to be referenced ---------------
    // -------------------------------------------------------------------------

    std::vector<ColumnWidth> ColWidth3;
    ColWidth3.push_back(ColumnWidth(0, 4, 15));
    CWorksheet& sheet = book.AddSheet("LR 10K", ColWidth3);

    sheet.BeginRow().AddCell("Raw Data from LR 10K Tensile Machine", CenterStyleIndex).EndRow();
    sheet.MergeCells(CellCoord(1, 0), CellCoord(1, 4));
    sheet.BeginRow().AddCell(tabData.date, dateTimeStyleIndex).EndRow();
    sheet.BeginRow().AddCell("").EndRow();

    // Make a header
    sheet.BeginRow().AddCell("Plot X", smallTitleCenterdStyleIndex).AddCell("Plot Y", smallTitleCenterdStyleIndex).AddCell("").AddCell("Points X", smallTitleCenterdStyleIndex).AddCell("Points Y", smallTitleCenterdStyleIndex).EndRow();
    sheet.BeginRow().AddCell("").EndRow();

    // Find all important points, and put only the valid ones in (not = 0)
    // Remove points that are zero
    std::vector<ImPlotPoint> impDataPoints;
    for (size_t i = 0; i < tabData.plot_importantDataPoints.size(); i++)
    {
        ImPlotPoint dataPoint = tabData.plot_importantDataPoints[i].first;

        if (dataPoint.x > 0.0f && dataPoint.y > 0.0f)
            impDataPoints.push_back(dataPoint);
    }

    // Add more points to the end to expand the graph as it is adjusted/shown in the LR 10K app graph
    // TODO:: Might have to be adjusted for the LR 10K machine stepper - Taking into account the speed set by user | Edit: Seems fine
    RollingBuffer tensileData = tabData.LR10KTensileData;
    ImVec2 lastPoint = tensileData.Data[tensileData.Data.Size - 1];
    float incrementer = 0.01f;
    while (lastPoint.x + incrementer < LR10KAppGlobals::testing_plot_only_extensionAxis.load())
    {
        tensileData.AddPoint(lastPoint.x + incrementer, 0);
        incrementer += 0.01f;
    }

    AppMainExcel::InsertLineCells(&book, &sheet, &tensileData, impDataPoints, false, smallTitleCenterdStyleIndex);

    // Produce a line chart
    AddLR10KLineChart(
        book, sheet,
        cl_lib, gs10_lib,
        tensileData.Data.Size + 1,
        tensileData.Data.Size + 1,
        tabData.date
    );

    // ------------------------------------------
    // CALCULATE NEW GRAPHS
    // ------------------------------------------

    const double T_0 = LR10KAppGlobals::calc_thickness;    // Start Thickness
    const double W_0 = LR10KAppGlobals::calc_width;        // Start Width
    const double L_0 = LR10KAppGlobals::calc_length;       // Start Length
    const double A_0 = tabData.area;                       // Start Area

    if (dimensionsAdded)
    {
        // ---------------------------------------------------------------------
        // --------------------- NOMINAL STRESS and STRAIN ---------------------
        // ---------------------------------------------------------------------

        // ------------------------------------------
        // Data sheet to store data to be referenced
        CWorksheet& sheetNom = book.AddSheet("Engineering Stress", ColWidth3);

        sheetNom.BeginRow().AddCell("Raw Data from Engineering Stress", CenterStyleIndex).EndRow();
        sheetNom.MergeCells(CellCoord(1, 0), CellCoord(1, 4));
        sheetNom.BeginRow().AddCell(tabData.date, dateTimeStyleIndex).EndRow();
        sheetNom.BeginRow().AddCell("").EndRow();

        // Make a header
        sheetNom.BeginRow().AddCell("Plot X", smallTitleCenterdStyleIndex).AddCell("Plot Y", smallTitleCenterdStyleIndex).AddCell("").AddCell("Points X", smallTitleCenterdStyleIndex).AddCell("Points Y", smallTitleCenterdStyleIndex).EndRow();
        sheetNom.BeginRow().AddCell("").EndRow();

        // Convertion of important points to nominal stress
        // happens in "InsertLineCells"

        std::pair nomStressStrain = AppMainExcel::InsertLineCells(&book, &sheetNom, &tensileData, impDataPoints, true, A_0, L_0);
        std::vector<double> epsilon_e = nomStressStrain.first, sigma_e = nomStressStrain.second;

        // Produce a line chart
        AddStressLineChart(
            book, sheetNom,
            cl_lib, gs10_lib,
            tensileData.Data.Size + 1,
            tensileData.Data.Size + 1,
            tabData.date, true
        );

        // ---------------------------------------------------------------------
        // ----------------------- TRUE STRESS and STRAIN ----------------------
        // ---------------------------------------------------------------------

        // ------------------------------------------
        // Data sheet to store data to be referenced
        CWorksheet& sheetTrue = book.AddSheet("True Stress", ColWidth3);

        sheetTrue.BeginRow().AddCell("Raw Data from True Stress", CenterStyleIndex).EndRow();
        sheetTrue.MergeCells(CellCoord(1, 0), CellCoord(1, 2));
        sheetTrue.BeginRow().AddCell(tabData.date, dateTimeStyleIndex).EndRow();
        sheetTrue.BeginRow().AddCell("").EndRow();

        // Make a header
        sheetTrue.BeginRow().AddCell("Plot X", smallTitleCenterdStyleIndex).AddCell("Plot Y", smallTitleCenterdStyleIndex).AddCell("").AddCell("Points X", smallTitleCenterdStyleIndex).AddCell("Points Y", smallTitleCenterdStyleIndex).EndRow();
        sheetTrue.BeginRow().AddCell("").EndRow();

        AppMainExcel::InsertLineCells(&book, &sheetTrue, &tensileData, impDataPoints, true, A_0, L_0, sigma_e, epsilon_e);

        // Produce a line chart
        AddStressLineChart(
            book, sheetTrue,
            cl_lib, gs10_lib,
            tensileData.Data.Size + 1,
            tensileData.Data.Size + 1,
            tabData.date, false
        );
    }

    // At the end save created workbook wherever you need
    if (book.Save(name.c_str()))
    {
        DockingApps::console.AddLog("An Excel Book has been saved successfully!");
        PlaySoundEffect("saveFileSuccess");

        return true;
    }
    else
    {
        DockingApps::console.AddLog("[error] Excel Book saving failed...\nTry closing any open Excel document with the same name.");
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
    if (fileBrowserOpenLR10KTest.HasSelected())
    {
        // Get current selection (path + filename)
        const std::string filename = fileBrowserOpenLR10KTest.GetSelected().string();
        fileBrowserOpenLR10KTest.ClearSelected();

        // Want to save the last folder location used
        DockingApps::mainFileSystem.SaveApplicationFolderSettingsOnly();

        // Load from JSON file
        TabControl::TabTestDataClass* tabDataLoaded = DockingApps::mainFileSystem.LoadJSONDataFromFileForANewTab(filename.c_str());
        tabDataLoaded->loadedFromFileName = filename;

        // Load current file folder location
        // Get current selection (path + filename). Filname will be folder name.
        std::stringstream filenameStream(filename);
        std::string segment;
        std::vector<std::string> seglist;

        // Split
        while (std::getline(filenameStream, segment, '\\'))
        {
            seglist.push_back(segment);
        }
        std::string newFilename = "";
        for (int i = 0; i < seglist.size() - 1; i++)
        {
            newFilename.append(seglist[i]);
            newFilename.append("\\");
        }
        // Save it for later, if user wants to save the file again.
        tabDataLoaded->current_temp_preSaveLocation = newFilename;

        // Check if any data was loaded
        if (tabDataLoaded->isValid())
        {
            // Add new tab with the retrived tab data
            DockingApps::mainTabSystem.CreateANewTab(tabDataLoaded);

            // Update the GUI after the new tab has been created
            TabControl::RefreshUpdateImportantDataPoints(tabDataLoaded);
            if ((tabDataLoaded->thickness > 0 && tabDataLoaded->width > 0) || tabDataLoaded->length > 0)
                tabDataLoaded->ConvertUnitsToNewUnitAfterLoad(tabDataLoaded);

            PlaySoundEffect("saveFileSuccess");

            DockingApps::console.AddLog("Successfully loaded plot data to a new tab!");
        }
        else
            DockingApps::console.AddLog("[error] Could not load plot data to new tab...");
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
        settings["gui_showConsole"]     = mainGUIGlobals::showConsole;
        settings["gui_showCalculator"]  = mainGUIGlobals::showCalculator;
        settings["enableSound"]         = mainGUIGlobals::enableSound;

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

        settings["areaOptionPicked"]        = LR10KAppGlobals::areaOptionPicked;
        settings["compressionOptionPicked"] = LR10KAppGlobals::compressionOptionPicked;

        settings["diameter"]    = LR10KAppGlobals::calc_diameter;

        settings["LR10KTravelDistanceControl"]      = TabControl::LR10KTravelDistanceControl;
        settings["LR10KTravelDistanceActionAfter"]  = TabControl::LR10KTravelDistanceActionAfter;

        settings["maximumLoadAllowedInsideLR10K"] = LR10KAppGlobals::maximumLoadAllowedInsideLR10K;
        settings["maximumExtensionAllowedInsideLR10K"] = LR10KAppGlobals::maximumExtensionAllowedInsideLR10K;

        DockingApps::mutexControl.BreakDetection();
        settings["breakThreshold"] = LR10KAppGlobals::breakThreshold.load();
        settings["enable_breakThreshold"] = LR10KAppGlobals::enable_breakThreshold.load();

        settings["extensiometerRange"]  = LR10KAppGlobals::extensiometerRange;
        settings["enable_extensiomter"] = LR10KAppGlobals::enable_extensiomter;

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        settings["fileBrowsing_LastLR10KSaveAsLocation"]    = fileBrowserSaveAsLR10KTest.GetSelected();
        settings["fileBrowsing_LastLR10KSaveLocation"]      = fileBrowserPreSaveLR10KTest.GetSelected();
        settings["fileBrowsing_LastXlxsSaveLocation"]       = fileBrowserSaveXlsx.GetSelected();
        settings["fileBrowsing_LastLR10KOpenLoadLocation"]  = fileBrowserOpenLR10KTest.GetSelected();

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
        settings["gui_showConsole"]     = false;
        settings["gui_showCalculator"]  = false;
        settings["enableSound"]         = true;

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

        settings["thickness"]   = defaultValuesGUI::calc_thickness;
        settings["width"]       = defaultValuesGUI::calc_width;
        settings["length"]      = defaultValuesGUI::calc_length;
        settings["diameter"]    = defaultValuesGUI::calc_diameter;

        settings["areaOptionPicked"]        = 0;
        settings["compressionOptionPicked"] = 0;

        settings["LR10KTravelDistanceControl"]          = 0.000f;
        settings["LR10KTravelDistanceActionAfter"]      = 0;

        settings["maximumLoadAllowedInsideLR10K"]       = 10000.0f;
        settings["maximumExtensionAllowedInsideLR10K"]  = 500.0f;

        settings["breakThreshold"] = 0.1f;
        settings["enable_breakThreshold"] = 0;

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

        settings["showHotkeyTips"]                          = false;
    }

    return settings;
}

void AppMainFileSystem::InsertJSONSettingsToGUISettings(json settings)
{
    // All valid values that must exsist, or else try and create a new settings file
    std::string_view gui_showConsole                            = "gui_showConsole";
    std::string_view gui_showCalculator                         = "gui_showCalculator";
    std::string_view enableSound                                = "enableSound";
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
    std::string_view areaOptionPicked                           = "areaOptionPicked";
    std::string_view compressionOptionPicked                    = "compressionOptionPicked";
    std::string_view LR10KTravelDistanceControl                 = "LR10KTravelDistanceControl";
    std::string_view LR10KTravelDistanceActionAfter             = "LR10KTravelDistanceActionAfter";
    std::string_view maximumLoadAllowedInsideLR10K              = "maximumLoadAllowedInsideLR10K";
    std::string_view maximumExtensionAllowedInsideLR10K         = "maximumExtensionAllowedInsideLR10K";
    std::string_view breakThreshold                             = "breakThreshold";
    std::string_view enable_breakThreshold                      = "enable_breakThreshold";
    std::string_view extensiometerRange                         = "extensiometerRange";
    std::string_view enable_extensiomter                        = "enable_extensiomter";
    std::string_view fileBrowsing_LastLR10KSaveAsLocation       = "fileBrowsing_LastLR10KSaveAsLocation";
    std::string_view fileBrowsing_LastLR10KSaveLocation         = "fileBrowsing_LastLR10KSaveLocation";
    std::string_view fileBrowsing_LastXlxsSaveLocation          = "fileBrowsing_LastXlxsSaveLocation";
    std::string_view fileBrowsing_LastLR10KOpenLoadLocation     = "fileBrowsing_LastLR10KOpenLoadLocation";
    std::string_view showHotkeyTips                             = "showHotkeyTips";

    // Check every key
    bool thereIsAnUnvalidKey = false;

    if (!settings.contains(std::string{ gui_showConsole                         })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ gui_showCalculator                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enableSound                             })) thereIsAnUnvalidKey = true;
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
    if (!settings.contains(std::string{ areaOptionPicked                        })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ compressionOptionPicked                 })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceControl              })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ LR10KTravelDistanceActionAfter          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumLoadAllowedInsideLR10K           })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ maximumExtensionAllowedInsideLR10K      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ breakThreshold                          })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_breakThreshold                   })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ extensiometerRange                      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ enable_extensiomter                     })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveAsLocation    })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KSaveLocation      })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastXlxsSaveLocation       })) thereIsAnUnvalidKey = true;
    if (!settings.contains(std::string{ fileBrowsing_LastLR10KOpenLoadLocation  })) thereIsAnUnvalidKey = true;
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
        mainGUIGlobals::showConsole     = settings["gui_showConsole"];
        mainGUIGlobals::showCalculator  = settings["gui_showCalculator"];
        mainGUIGlobals::enableSound     = settings["enableSound"];

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

        LR10KAppGlobals::calc_thickness = settings["thickness"];
        LR10KAppGlobals::calc_width     = settings["width"];
        LR10KAppGlobals::calc_length    = settings["length"];
        LR10KAppGlobals::calc_diameter  = settings["diameter"];

        LR10KAppGlobals::areaOptionPicked           = settings["areaOptionPicked"];
        LR10KAppGlobals::compressionOptionPicked    = settings["compressionOptionPicked"];

        TabControl::LR10KTravelDistanceControl      = settings["LR10KTravelDistanceControl"];
        TabControl::LR10KTravelDistanceActionAfter  = settings["LR10KTravelDistanceActionAfter"];

        LR10KAppGlobals::maximumLoadAllowedInsideLR10K      = settings["maximumLoadAllowedInsideLR10K"];
        LR10KAppGlobals::maximumExtensionAllowedInsideLR10K = settings["maximumExtensionAllowedInsideLR10K"];

        LR10KAppGlobals::breakThreshold                     = settings["breakThreshold"];
        LR10KAppGlobals::enable_breakThreshold              = settings["enable_breakThreshold"];

        LR10KAppGlobals::extensiometerRange                 = settings["extensiometerRange"];
        LR10KAppGlobals::enable_extensiomter                = settings["enable_extensiomter"];

        // ---------------------------------------------------------------------
        // --------------------------- File Browsing ---------------------------
        // ---------------------------------------------------------------------
        fileBrowserSaveAsLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveAsLocation"]);
        fileBrowserPreSaveLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KSaveLocation"]);
        fileBrowserSaveXlsx.SetPwd(settings["fileBrowsing_LastXlxsSaveLocation"]);
        fileBrowserOpenLR10KTest.SetPwd(settings["fileBrowsing_LastLR10KOpenLoadLocation"]);

        LR10KAppGlobals::showHotkeyTips = settings["showHotkeyTips"];

        // ---------------------------------------------------------------------
        // ------------- Update the GUI information for current tab ------------
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
    @brief Used for refreshing and updating important crucial points list, used in the quick report.
**/
void TabControl::RefreshUpdateImportantDataPoints(TabControl::TabTestDataClass* tabData, bool markTabUnsaved)
{
    if (tabData == nullptr) return;

    // Refresh and update crucial points added to the important data points vector plot_importantDataPoints
    std::vector<std::tuple<ImPlotPoint, int, bool>> allDataPoints   = tabData->plot_dataPoints;
    tabData->plot_importantDataPoints.clear();

    // Try and write all of the important data properties
    bool propIndexFound[4]{ false, false, false, false };
    for (int i = 0; i < IM_ARRAYSIZE(propIndexFound); i++)
    {
        // Reset important data points vector
        tabData->plot_importantDataPoints.push_back({});

        // Check every data point, so every property can be added in an acending order
        for (int j = 0; j < allDataPoints.size(); j++)
        {
            // Current looped data
            ImPlotPoint dataPoint = std::get<ImPlotPoint>(allDataPoints[j]);
            int dataPointPropIndex = std::get<int>(allDataPoints[j]);

            // Only find one of each
            if (propIndexFound[i]) continue;

            // Check if it is within the range of valid options
            //std::cout << "PropIndex: " << propIndex << std::endl;
            if (dataPointPropIndex == i)
            {
                // Write correspondingly in acending order
                tabData->plot_importantDataPoints.at(i).first   = dataPoint;
                tabData->plot_importantDataPoints.at(i).second  = dataPointPropIndex;
                propIndexFound[i] = true;
            }
        }
    }

    if (markTabUnsaved)
        tabData->MarkTabAsUnsaved();
}
/**
    @brief Used for undoing crucial point.
**/
void TabControl::CrucialPointsUndoPoint(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return;

    std::vector<std::tuple<ImPlotPoint, int, bool>>* dataPoints = &tabData->plot_dataPoints;

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
    @brief Update the extension and load variabels for the normal plot graph view.
    @param tabData -
    @param updateGraphType    - 
    @param doNotMarkAsUnsaved - 
**/
void TabControl::AutoAdjustNormalExtensionLoadVariabels(TabControl::TabTestDataClass* tabData, bool updateGraphType, bool doNotMarkAsUnsaved)
{
    if (tabData == nullptr) return;

    if (updateGraphType)
        LR10KAppGlobals::toggleGraphType = true;

    RollingBuffer LR10KTensileData = tabData->LR10KTensileData;

    if (LR10KTensileData.Data.Size > 0)
    {   
        if (!LR10KAppGlobals::toggleGraphType
            && tabData->LR10KTensileDataNominal.Data.Size > 0
            && tabData->LR10KTensileDataTrue.Data.Size > 0
            && !LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate.load())
        {
            // Make a copy, since the values could change (since it is calulated every time) and then the application will crash
            // Only need to read also
            RollingBuffer testing_LR10KTensileDataNomCopy   = tabData->LR10KTensileDataNominal;
            RollingBuffer testing_LR10KTensileDataTrueCopy  = tabData->LR10KTensileDataTrue;

            // Nominal and true stres
            // Find the largest values
            float largestY = 0;
            float largestX = testing_LR10KTensileDataTrueCopy.Data[testing_LR10KTensileDataTrueCopy.Data.Size - 1].x;

            for (int i = 0; i < testing_LR10KTensileDataNomCopy.Data.Size; i++)
            {
                if (testing_LR10KTensileDataNomCopy.Data[i].y > largestY)
                    largestY = testing_LR10KTensileDataNomCopy.Data[i].y;
                if (testing_LR10KTensileDataNomCopy.Data[i].x > largestX)
                    largestX = testing_LR10KTensileDataNomCopy.Data[i].x;
            }
            for (int i = 0; i < tabData->LR10KTensileDataTrue.Data.Size; i++)
            {
                if (testing_LR10KTensileDataTrueCopy.Data[i].y > largestY)
                    largestY = testing_LR10KTensileDataTrueCopy.Data[i].y;
                if (testing_LR10KTensileDataTrueCopy.Data[i].x > largestX)
                    largestX = testing_LR10KTensileDataTrueCopy.Data[i].x;
            }

            LR10KAppGlobals::testing_plot_only_loadAxis     .store(largestY + defaultValuesGUI::stressAxisIncrement * LR10KAppGlobals::GetUnitOption<double>(10));
            DockingApps::mutexControl.testing_plot_only_loadAxisStatusChanged.notify_all();
            LR10KAppGlobals::testing_plot_only_extensionAxis.store(largestX + defaultValuesGUI::strainAxisIncrement);
            DockingApps::mutexControl.testing_plot_only_extensionAxisStatusChanged.notify_all();
        }
        else if (LR10KAppGlobals::toggleGraphType && LR10KTensileData.Data.Size > 0)
        {
            // Find the largest values
            float largestY = 0;
            float largestX = LR10KTensileData.Data[LR10KTensileData.Data.Size - 1].x;
            for (int i = 0; i < LR10KTensileData.Data.Size; i++)
            {
                if (LR10KTensileData.Data[i].y > largestY)
                    largestY = LR10KTensileData.Data[i].y;
            }

            LR10KAppGlobals::testing_plot_only_extensionAxis.store(largestX + defaultValuesGUI::extensionAxisIncrement * LR10KAppGlobals::GetUnitOption<double>(8));
            DockingApps::mutexControl.testing_plot_only_extensionAxisStatusChanged.notify_all();
            LR10KAppGlobals::testing_plot_only_loadAxis     .store(largestY + defaultValuesGUI::loadAxisIncrement * LR10KAppGlobals::GetUnitOption<double>(9));
            DockingApps::mutexControl.testing_plot_only_loadAxisStatusChanged.notify_all();
        }
    }

    if (!doNotMarkAsUnsaved)
        tabData->MarkTabAsUnsaved();
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

    while (!mainGUIGlobals::done && !LR10KAppGlobals::m_pThreadPlotAxisUpdater_stopFlag.load())
    {
        // The controller for "PauseUpdate2" is in the main_dx11.cpp, and will destroy this thread when a test is finished
        if (tabData != nullptr && !LR10KAppGlobals::m_pThreadPlotAxisUpdater_PauseUpdate2.load())
        {
            // Check if the LR 10K hits any limits to End Test

            // Extreme limits
            if (!mainGUIGlobals::useDemoMode)
                PreventExtremeLimitsLR10K(tabData);
            // User set limits
            if (!mainGUIGlobals::useDemoMode)
                PreventUserLimitsLR10K(tabData);

            // Update the graph (this will create a smooth animation also inside this hook)
            AutoAdjustNormalExtensionLoadVariabels(tabData, false, true);

            if (!tabData->test_finished)
            {
                if (tabData->LR10KTensileData.Data.Size > 0)
                {
                    // Take the latest data point
                    int latestPointIndex = tabData->LR10KTensileData.Data.Size - 1;
                    const ImVec2 latestDataPoint = tabData->LR10KTensileData.Data[latestPointIndex];

                    // -------------------------------------------------------------
                    // ---------- Try detecting a propotional limit point ----------
                    // -------------------------------------------------------------
                    // Calculate delta time in seconds
                    static int limit = 9; // Needs 9 points to check if there is a linear regression

                    std::chrono::duration<double> deltaTime = std::chrono::high_resolution_clock::now() - TabControl::timestampForLastAddedRegressionPoint;

                    // 165 ms | 6 points a second
                    // Always try and search for a new point, and remove the old one,
                    // if a UTS has not been added yet
                    if (0 == tabData->plot_importantDataPoints[0].first.y && deltaTime.count() >= 0.165 && LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad() > 100.0f)
                    {
                        // Try detecting the propotional limit based on linear regression
                        static std::vector<DataPoint> data = {  };
                        data.push_back({ latestDataPoint.x, latestDataPoint.y });

                        // Find the linear regression
                        double rSquared = linearRegressionR2(data);

                        // rSquared > 0.9 is very certain
                        // Only add if tension
                        if (data.size() == limit && rSquared > tabData->calcLinRegressionConst)
                        {
                            tabData->AssignPoint_PropotionalLimit(tabData, latestDataPoint);
                        }

                        // Reset and try to find again
                        if (data.size() > limit)
                            data = { };

                        // Set again
                        TabControl::timestampForLastAddedRegressionPoint = std::chrono::high_resolution_clock::now();
                    }

                    // -------------------------------------------------------------
                    // ---------------- Try detecting yielding point ---------------
                    // -------------------------------------------------------------
                    // REMOVED SINCE IT DOESN'T WORK FOR EVERY CENARIO. OLD CODE. Should be done manually
                    // Try figuring out if there is a big jump in load, and assume a dip in material strength resistance has occured
                    // If load decreases over 25 % of load in last point, then there should be a material strength loss that occured for most materials... (assumes)
                    //if (0 == tabData->plot_importantDataPoints[1].first.y && tabData->LR10KTensileData.Data.Size > 1)
                    //{
                    //    int prevPointIndex = tabData->LR10KTensileData.Data.Size - 2;
                    //    const ImVec2 prevDataPoint = tabData->LR10KTensileData.Data[prevPointIndex];

                    //    // 25 % decrease - Actual ending of test occurs when this functions returns true
                    //    if (latestDataPoint.y <= (prevDataPoint.y - prevDataPoint.y * 0.25))
                    //    {
                    //        tabData->AssignPoint_YieldPoint(tabData, latestDataPoint);
                    //    }
                    //}
                }

                // Get latest UTS point (used for detecting break underneath)
                ImPlotPoint UTSPoint = tabData->GetUTS(); // UTS

                // Try detecting break
                DockingApps::mutexControl.BreakDetection();
                if (DetectIfABreakHasOccured(tabData, UTSPoint, LR10KAppGlobals::enable_breakThreshold.load(), LR10KAppGlobals::breakThreshold.load()))
                    LR10KControlSystem::EndTest(tabData);

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
                    }
                }
            }
        }
    }

    return 0;
}

/**
    @brief  @brief  Checks if a break has occured in the testing part
    @details Will assume there is a break and end the test, if the graph goes under e.g. 10 % of the UTS point.
                This percentage can be adjusted and the logic is confirmed to be in the original software.
    @param  tabData                        - 
    @param  UTSPoint                       - 
    @param  enable_automaticBreakDetection - 
    @param  breakThreshold                 - 
    @retval                                - 
**/
bool TabControl::DetectIfABreakHasOccured(TabControl::TabTestDataClass* tabData, ImPlotPoint UTSPoint, int enable_automaticBreakDetection, float breakThreshold)
{
    if (tabData == nullptr || tabData->plot_importantDataPoints[3].first.y > 0) return false;

    // Latest data point from tabData
    RollingBuffer tabDataLoaded = tabData->LR10KTensileData;

    if (tabDataLoaded.Data.Size > 0)
    {
        int latestPointIndex = tabDataLoaded.Data.Size - 1;
        const ImVec2 latestDataPoint = tabDataLoaded.Data[latestPointIndex];

        const float limit = 15.0f;
        // Check if the current test have not passed 15 N
        if (latestDataPoint.y >= limit)
            tabData->testHasGoneAbove15N = true;

        if (0 == enable_automaticBreakDetection) // Manual (default)
        {
            if (tabData->testHasGoneAbove15N && latestDataPoint.y <= UTSPoint.y * breakThreshold)
            {
                tabData->AssignPoint_UltimateTensileStrength(tabData);
                tabData->AssignPoint_BreakPoint(tabData, latestDataPoint);

                return true;
            }
        }
        else if (1 == enable_automaticBreakDetection) // Automatic
        {
            // Try figuring out if there is a big jump in load, and assume a clean break on part has occured
            // If load increases over 25 % of load in last point, then there should be a break that occured for most materials... (assumes)
            if (tabData->testHasGoneAbove15N && tabDataLoaded.Data.Size > 1 && 0 == tabData->plot_importantDataPoints[3].first.y)
            {
                int prevPointIndex = tabDataLoaded.Data.Size - 2;
                const ImVec2 prevDataPoint = tabDataLoaded.Data[prevPointIndex];

                // 25 % increase - Actual ending of test occurs when this functions returns true
                if (latestDataPoint.y >= prevDataPoint.y * 1.25)
                {
                    tabData->AssignPoint_BreakPoint(tabData, prevDataPoint);
                }
            }

            // Check if the current point is lower or equal to 15 N, after it has passed 15 N
            if (tabData->testHasGoneAbove15N && latestDataPoint.y < limit)
            {
                tabData->AssignPoint_UltimateTensileStrength(tabData);

                if (0 == tabData->plot_importantDataPoints[3].first.y)
                    tabData->AssignPoint_BreakPoint(tabData, latestDataPoint);

                return true;
            }
        }
    }

    return false;
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
    @brief Clean up.
    @details Because of a bug that made important points not have the index added etc., this is needed for ALL old files <= 10.10.23.
    @param tabData - 
**/
void TabControl::CleanUpImportantPoints(TabControl::TabTestDataClass* tabData)
{
    for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
    {
        // Add correct index if there is a match, otherwise erase it
        bool aMatch = false;
        for (int j = 0; j < tabData->plot_dataPoints.size(); j++)
        {
            if (tabData->plot_importantDataPoints[i].first.x == std::get<ImPlotPoint>(tabData->plot_dataPoints[j]).x)
            {
                aMatch = true;

                // Set the correct index
                tabData->plot_importantDataPoints[i].second = std::get<int>(tabData->plot_dataPoints[j]);
            }
        }

        // Remove the unused ones
        if (!aMatch && i < tabData->plot_importantDataPoints.size()) //!!Important!!
            tabData->plot_importantDataPoints.erase(tabData->plot_importantDataPoints.begin() + i);
    }
}

/**
    @brief  Checks if the data point index is an important point or not.
    @param  tabData        - 
    @param  dataPointIndex - 
    @retval                - 
**/
bool TabControl::CheckIfImportantPoint(TabControl::TabTestDataClass* tabData, int dataPointIndex)
{
    if (dataPointIndex > tabData->plot_dataPoints.size() - 1) return false;

    ImPlotPoint dataPoint = std::get<ImPlotPoint>(tabData->plot_dataPoints[dataPointIndex]);
    for (int j = 0; j < tabData->plot_importantDataPoints.size(); j++)
    {
        ImPlotPoint impDataPoint = tabData->plot_importantDataPoints[j].first;

        if (dataPoint.x == impDataPoint.x)
            return true;
    }

    return false;
}

/**
    @brief Main function for calculating e.g. stresses and strain.
    @param tabData - 
**/
void TabControl::CalculateDataFromTest(TabControl::TabTestDataClass* tabData)
{
    bool unvalidCalculationStart =
        (!tabData->test_finished || tabData->plot_dataPoints.size() < 1
            || (LR10KAppGlobals::calc_length == 0 && (LR10KAppGlobals::calc_thickness == 0 || LR10KAppGlobals::calc_width == 0)));

    if (unvalidCalculationStart)
        return;

    // Save dimensions to tabData
    tabData->thickness          = LR10KAppGlobals::calc_thickness;
    tabData->width              = LR10KAppGlobals::calc_width;
    tabData->length             = LR10KAppGlobals::calc_length;
    tabData->diameter           = LR10KAppGlobals::calc_diameter;
    tabData->areaOptionPicked   = LR10KAppGlobals::areaOptionPicked;

    // Offset from zero point extension axis
    defaultValuesGUI::offsetZeroPointExtensionAxis = tabData->LR10OffsetZeroPointExtensionAxis;

    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
    {
        // Calculate stresses
        DockingApps::calculator.CalculateStressPoints(tabData);
        DockingApps::calculator.CalculateStresses(tabData);
    }
    else if (!tabData->tension && 1 == tabData->compressionOptionPicked)
    {
        // Calculate bending moment and UTS
        DockingApps::calculator.CalculateCompressionBendMomentAndUTS(tabData);
    }

    // Update graph
    TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);
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

    // TabItemButton() and Leading/Trailing flags are distinct features which we will demo together.
    // (It is possible to submit regular tabs with Leading/Trailing flags, or TabItemButton tabs without Leading/Trailing flags...
    // but they tend to make more sense together)

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

        // Demo Trailing Tabs: click the "+" button to add a new tab (in your app you may want to use a font icon instead of the "+")
        // Note that we submit it before the regular tabs, but because of the ImGuiTabItemFlags_Trailing flag it will always appear at the end.
        if (ImGui::TabItemButton("+ New Test", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)
            && TabControl::activeTestTabsId.Size < TabControl::MAXTABS && notALockedTab)
        {
            TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData, true, true);
            TabControl::TabTestDataClass* newTabTestData = new TabControl::TabTestDataClass;

            CreateANewTab(newTabTestData);

            PlaySoundEffect("buttonClick1");
        }

        // Submit our regular 
        for (int i = 0; i < TabControl::activeTestTabsId.Size; i++)
        {
            // Get new tab and previous tab
            int tabTestId = TabControl::activeTestTabsId[i];
            TabControl::TabTestDataClass* loopedTabData = TabControl::allTabTestData[tabTestId];

            bool open = true;
            char name[16];

            // Changing flag depending on if the document is saved or not
            int flags = loopedTabData->unsavedThingsInTab ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;

            // Lambda functions for closing current tab.
            static auto closeTab = [](const char* name, int i)
            {
                ImGui::SetTabItemClosed(name);
                TabControl::activeTestTabsId.erase(TabControl::activeTestTabsId.Data + i); // Remove tab

                PlaySoundEffect("buttonClick0");
            };

            // Lambda functions for tab information.
            static auto tabInformation = [](
                TabControl::TabTestDataClass* activeTabData
                , TabControl::TabTestDataClass* tabData
                , int activeTabTestId
                , const char* name
                , bool notALockedTab)
            {
                    // Reset previous active tab
                    if (!activeTabData->updatedGraphOnOpen)
                        tabData->updatedGraphOnOpen = false; // Reset previous, so it can be updated again later

                    // Set current active tab index id
                    TabControl::currActiveTabIndexId = activeTabTestId;

                    // Update graph
                    if (!activeTabData->updatedGraphOnOpen)
                    {
                        TabControl::AutoAdjustNormalExtensionLoadVariabels(activeTabData, false, true);
                        activeTabData->updatedGraphOnOpen = true;
                    }

                    // If no test has started, set it back to normal view... Always
                    if (notALockedTab && !activeTabData->test_finished)
                    {
                        if (!LR10KAppGlobals::toggleGraphType)
                            LR10KAppGlobals::toggleGraphType = true;
                    }

                    // Name of tab
                    std::string nameOut = name;

                    ImGui::Text("This is tensile test: %s. You can save the data within the \"Graph\" panel as a .XLSX file, and as a loadable file in the menu.", Trim(nameOut).c_str());
                    ImGui::Separator();
                    if (activeTabData->test_finished)
                    {
                        ImGui::Text("This test is finished. It was completed: %s", activeTabData->date.c_str());

                        if (activeTabData->loadedFromFileName.size() > 0)
                        {
                            ImGui::Text("(Represents same data as in file: %s).", activeTabData->loadedFromFileName.c_str());
                        }
                        ImGui::Separator();
                    }

                    // Clean the temporary description, if needed
                    static int lastTempDescriptionIdUpdated = -1;
                    if (lastTempDescriptionIdUpdated != tabData->tabId)
                    {
                        // Clear temporary
                        for (int i = 0; i < IM_ARRAYSIZE(TabControl::tempDescription); i++)
                        {
                            TabControl::tempDescription[i] = {};
                        }
                        // Insert loaded from tabData, once
                        for (int i = 0; i < tabData->description.length(); i++)
                        {
                            TabControl::tempDescription[i] = tabData->description.c_str()[i];
                        }

                        // Save status
                        lastTempDescriptionIdUpdated = tabData->tabId;
                    }

                    // Update some GUI values, that will be inherited from tabData (this could be done elsewhere also.., but this one is nice to have here at least)
                    defaultValuesGUI::offsetZeroPointExtensionAxis = tabData->LR10OffsetZeroPointExtensionAxis;
            };

            // Tab name
            snprintf(name, IM_ARRAYSIZE(name), "%04d      ", tabTestId);

            if (TabControl::activeTestTabsId.Size > 1)
            {
                // When a tab becomes active, this returns true
                // When a tab is closed/removed, "open" will be false

                // When a test has started running, it should not be
                // possible to change to another tab.
                if (TabControl::forcedTabIndexId == tabTestId && tabData->test_unfinished_test_started)
                    flags |= ImGuiTabItemFlags_SetSelected;

                bool tabIsActive = ImGui::BeginTabItem(name, &open, flags);
                if (tabIsActive)
                {
                    // Update the settings for vertical sliders if it is not the same as the settings from GUI
                    if (LR10KAppGlobals::testing_speedAndPlotActual[0] != tabData->settings_speedAndPlot[0] || LR10KAppGlobals::testing_speedAndPlotActual[2] != tabData->settings_speedAndPlot[2])
                    {
                        LR10KAppGlobals::testing_speedAndPlotGUI[0] = ((tabData->settings_speedAndPlot[0] / LR10KAppGlobals::testing_speedAndPlotMaxValuesActual[0]) * 100);
                        LR10KAppGlobals::testing_speedAndPlotGUI[2] = tabData->settings_speedAndPlot[2];

                        LR10KAppGlobals::testing_speedAndPlotActual[0] = tabData->settings_speedAndPlot[0];
                        LR10KAppGlobals::testing_speedAndPlotActual[2] = tabData->settings_speedAndPlot[2];
                    }

                    // Update the units of the tab if it is not the same as the settings from GUI
                    if (tabData->fileFinishedLoading && tabData->unitsConvertionIndex != LR10KAppGlobals::unitsConvertionIndex)
                    {
                        LR10KAppGlobals::unitsConvertionIndex = tabData->unitsConvertionIndex;
                        tabData->ConvertUnitsToNewUnit(tabData, LR10KAppGlobals::unitsConvertionIndex, false);
                    }

                    tabInformation(loopedTabData, tabData, tabTestId, name, notALockedTab);
                    ImGui::EndTabItem();
                }
            }
            else
            {
                bool tabIsActive = ImGui::BeginTabItem(name, NULL, flags);

                if (tabIsActive)
                {
                    tabInformation(loopedTabData, tabData, tabTestId, name, notALockedTab);
                    ImGui::EndTabItem();
                }
            }

            // When trying to close a tab (don't care to remove the actual tab data, to keep everything uniq for this "session")

            // Name of tab
            std::string nameOut = name;

            std::string popUpID = "Close tab #";
            popUpID.append(Trim(nameOut));
            popUpID.append(" without saving?");

            if (!open && TabControl::activeTestTabsId.Size > 1 && notALockedTab)
            {
                if (mainGUIGlobals::ignoreSavingPrompt || !loopedTabData->unsavedThingsInTab)
                    closeTab(name, i);
            }

            // Prompt window for a single tab when trying to close it.
            bool closeTabCheck = false;
            DockingApps::mainFileSystem.PromptUserToCloseTabBeforeSaving(popUpID.c_str(), (!open && TabControl::activeTestTabsId.Size > 1 && notALockedTab && !mainGUIGlobals::ignoreSavingPrompt && TabControl::allTabTestData[tabTestId]->unsavedThingsInTab), &closeTabCheck);
            if (closeTabCheck) closeTab(name, i);
        }

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


std::vector<ImPlotPoint> TabControl::TabTestDataClass::GetImportantDataPointsOnly()
{
    std::vector<ImPlotPoint> onlyDataPoints;

    for (int i = 0; i < plot_dataPoints.size(); i++)
    {
        ImPlotPoint dataPoint;
        dataPoint.x = std::get<ImPlotPoint>(plot_dataPoints[i]).x;
        dataPoint.y = std::get<ImPlotPoint>(plot_dataPoints[i]).y;

        onlyDataPoints.push_back(dataPoint);
    }

    return onlyDataPoints;
}

std::vector<ImPlotPoint> TabControl::TabTestDataClass::GetCrucialDataPointsOnly(TabTestDataClass* tabData)
{
    // Only four - and it will be placed in order since the text underneath will be displayed in order.
    std::vector<ImPlotPoint> onlyCrucialDataPoints{ {}, {}, {}, {} };

    int pointsAdded = 0;
    for (int i = 0; i < plot_dataPoints.size(); i++)
    {
        if (pointsAdded == 5) break;

        ImPlotPoint dataPoint;
        dataPoint.x = std::get<ImPlotPoint>(plot_dataPoints[i]).x;
        dataPoint.y = std::get<ImPlotPoint>(plot_dataPoints[i]).y;

        if (TabControl::CheckIfImportantPoint(tabData, i))
        {
            onlyCrucialDataPoints[std::get<int>(plot_dataPoints[i])] = dataPoint;
            pointsAdded++;
        }
    }

    return onlyCrucialDataPoints;
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
    std::vector<std::pair<ImPlotPoint, int>> plot_importantDataPoints = tabData->plot_importantDataPoints;
    ImPlotPoint fracturePoint(0, 0);

    // Find last important point with index 3
    for (int i = 0; i < plot_importantDataPoints.size(); i++)
    {
        ImPlotPoint dataPoint       = plot_importantDataPoints[i].first;
        int pointIndex              = plot_importantDataPoints[i].second;

        if (3 == pointIndex)
        {
            fracturePoint = dataPoint;
            break;
        }
    }

    return fracturePoint;
}

void TabControl::TabTestDataClass::AssignPoint_PropotionalLimit(TabControl::TabTestDataClass* tabData, ImPlotPoint propotionalLimitPoint)
{
    if (!tabData->tension && 1 == tabData->compressionOptionPicked) return;

    // Only assign if point x and y is not NaN, which can happen...
    if (!std::isnan(propotionalLimitPoint.x) && !std::isnan(propotionalLimitPoint.y) && propotionalLimitPoint.y > 0 && propotionalLimitPoint.x > 0)
    {
        plot_dataPoints.push_back({ propotionalLimitPoint, 0, false });

        // Assign correct data point index ID Globally
        plot_importantDataPoints[0] = { propotionalLimitPoint, 0 }; // Propotional limit point has index ID 0
        DockingApps::console.AddLog("Automatically added a Propotional Limit point. (just an assumtion)");
    }
    else
        DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");

    TabControl::RefreshUpdateImportantDataPoints(tabData);
}
void TabControl::TabTestDataClass::AssignPoint_YieldPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint yieldingPoint)
{
    // THIS LOGIC IS NOT FROM THEORY, ONLY ASSUMTION AND IS REMOVED. Easier to do it manually...
    //// Order checker
    //if (plot_importantDataPoints[0].first.x == 0) return;

    //// Only assign if point x and y is not NaN, which can happen...
    //if (!std::isnan(yieldingPoint.x) && !std::isnan(yieldingPoint.y) && yieldingPoint.y > 0 && yieldingPoint.x > 0)
    //{
    //    plot_dataPoints.push_back({ yieldingPoint, 1, false });

    //    // Assign correct data point index ID Globally
    //    plot_importantDataPoints[1] = { yieldingPoint, 1 }; // Yield point has index ID 1
    //    DockingApps::console.AddLog("Automatically added a Yielding point. (just an assumtion)");
    //}
    //else
    //    DockingApps::console.AddLog("Could not add Propotional Limit Point since one or more points were NaN...");

    //TabControl::RefreshUpdateImportantDataPoints(tabData);
}
void TabControl::TabTestDataClass::AssignPoint_UltimateTensileStrength(TabControl::TabTestDataClass* tabData)
{
    // Order checker
    if ((tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked)) && 0 == plot_importantDataPoints[0].first.x
        /*|| plot_importantDataPoints[1].first.x == 0*/) return; // This is the best logic it seems

    ImPlotPoint UTSPoint = this->GetUTS(); // UTS

    // Only assign if point x and y is not NaN, which can happen when using Demo mode...
    if ((!std::isnan(UTSPoint.x) && !std::isnan(UTSPoint.y) && UTSPoint.y > 0 && UTSPoint.x > 0))
    {
        plot_dataPoints.push_back({ UTSPoint, 2, false });

        // Assign correct data point index ID Globally
        plot_importantDataPoints[2] = { UTSPoint, 2 }; // Ultimate Tensile Strength
        DockingApps::console.AddLog("Automatically added a Ultimate Tensile Strength point. (just an assumtion)");
    }
    else
        DockingApps::console.AddLog("Could not add Ultimate Tensile Strength since one or more points were NaN...");

    TabControl::RefreshUpdateImportantDataPoints(tabData);
}
void TabControl::TabTestDataClass::AssignPoint_BreakPoint(TabControl::TabTestDataClass* tabData, ImPlotPoint breakPoint)
{
    // Order checker
    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
    {
        if (0 == plot_importantDataPoints[0].first.x
            /*|| plot_importantDataPoints[1].first.x == 0*/
            || 0 == plot_importantDataPoints[2].first.x) return;
    }

    // Only assign if point x and y is not NaN, which can happen...
    if (!std::isnan(breakPoint.x) && !std::isnan(breakPoint.y) && breakPoint.y > 0 && breakPoint.x > 0)
    {
        plot_dataPoints.push_back({ breakPoint, 3, false });

        // Assign correct data point index ID Globally
        plot_importantDataPoints[3] = { breakPoint, 3 }; // Break point has index ID 3
        DockingApps::console.AddLog("Automatically added a Break/Fracture point. (just an assumption)");
    }
    else
        DockingApps::console.AddLog("Could not add Break Point since one or more points were NaN...");

    TabControl::RefreshUpdateImportantDataPoints(tabData);
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
            const bool snappedOnto = std::get<bool>(tabData->plot_dataPoints[i]);

            // Save
            tabData->plot_dataPoints[i] = std::make_tuple(points, index, snappedOnto);
        }

        // Crucial points
        for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
        {
            ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
            points.x /= unitConvOld_dist;
            points.y /= unitConvOld_force;

            const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

            // Save
            tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
        }
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
        const bool snappedOnto      = std::get<bool>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index, snappedOnto);
    }

    // Crucial points
    for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

        // Save
        tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
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

    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
    {
        // Calculated points for different types of stresses
        DockingApps::calculator.CalculateStressPoints(tabData, false, false, markAsSaved);
        DockingApps::calculator.CalculateStresses(tabData);
    }
    else if (!tabData->tension && 1 == tabData->compressionOptionPicked)
    {
        // Calculate bending moment and UTS
        DockingApps::calculator.CalculateCompressionBendMomentAndUTS(tabData, false, false, markAsSaved);
    }

    // Update graph
    if (updateGlobals)
        TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);
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
        const bool snappedOnto      = std::get<bool>(tabData->plot_dataPoints[i]);

        // Save
        tabData->plot_dataPoints[i] = std::make_tuple(points, index, snappedOnto);
    }

    // Crucial points
    for (int i = 0; i < tabData->plot_importantDataPoints.size(); i++)
    {
        ImPlotPoint points = std::get<ImPlotPoint>(tabData->plot_importantDataPoints[i]);
        points.x *= unitConv_dist;
        points.y *= unitConv_force;

        const int index = std::get<int>(tabData->plot_importantDataPoints[i]);

        // Save
        tabData->plot_importantDataPoints[i] = std::make_pair(points, index);
    }

    // -------------------------------------------------------------------------
    // -------------------------------- Finished -------------------------------
    // -------------------------------------------------------------------------

    // Update global values
    LR10KAppGlobals::calc_thickness    = tabData->thickness;
    LR10KAppGlobals::calc_width        = tabData->width;
    LR10KAppGlobals::calc_length       = tabData->length;
    LR10KAppGlobals::calc_diameter     = tabData->diameter;

    if (tabData->tension || (!tabData->tension && 0 == tabData->compressionOptionPicked))
    {
        // Calculated points for different types of stresses
        DockingApps::calculator.CalculateStressPoints(tabData, true, true, true, false);
        DockingApps::calculator.CalculateStresses(tabData);
    }
    else if (!tabData->tension && 1 == tabData->compressionOptionPicked)
    {
        // Calculate bending moment and UTS
        DockingApps::calculator.CalculateCompressionBendMomentAndUTS(tabData, true, true, false);
    }

    // Update graph
    TabControl::AutoAdjustNormalExtensionLoadVariabels(tabData);

    // Update to new unit
    LR10KAppGlobals::unitsConvertionIndex = tabData->unitsConvertionIndex;
}
void TabControl::TabTestDataClass::MarkTabAsSaved()
{
    if (unsavedThingsInTab)
        unsavedThingsInTab = false;
}
void TabControl::TabTestDataClass::MarkTabAsUnsaved()
{
    if (!unsavedThingsInTab)
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
    float value = TabControl::LR10KRawExtension * LR10KAppGlobals::Convert_to_mm;
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

            // A real test will never start when demo mode is activated
            if (mainGUIGlobals::useDemoMode && LR10KAppGlobals::setZeroPointOnStarTest)
                SetZeroPoint(tabData);
            else if (LR10KAppGlobals::configurationDone)
            {
                LR10KControlSystem::SendTestSetupToLR10K();
                _LR10KController.w_Go();
            }

            tabData->tension = LR10KAppGlobals::tensionTesting;
            LR10KAppGlobals::toggleGraphType = true;

            tabData->test_running_test = true;
            tabData->test_unfinished_test_started = true;

            TabControl::aTestIsStartedOnTabIndexId = TabControl::currActiveTabIndexId;

            tabData->MarkTabAsUnsaved();

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

    // Check if user has added start length, start thickness etc. for test
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
        return executeTest();

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
            _LR10KController.w_EndTest();

        LR10KAppGlobals::zeroPointIsSet = false;

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
        if (0 == tabData->plot_importantDataPoints[2].first.y) // Will also be assigned at automatic break point detection to get this point first... (just for visuals)
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

void AppConsole::Draw(TabControl::TabTestDataClass* tabData, const char* title, bool* p_open)
{
    if (tabData == nullptr) return;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open))
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
        /*else if (Stricmp(command_line, "FUP") == 0)
        {
            if (LR10KControlSystem::Translation_FASTUP())
                AddLog("[translation] Moving FAST UP");
            else
                AddLog("[error] Could not move fast up...");
        }*/
        else if (Stricmp(command_line, "DOWN") == 0)
        {
            if (LR10KControlSystem::Translation_DOWN(tabData))
                AddLog("[translation] Moving DOWN");
            else
                AddLog("[error] Could not move down...");
        }
        /*else if (Stricmp(command_line, "FDOWN") == 0)
        {
            if (LR10KControlSystem::Translation_FASTDOWN())
                AddLog("[translation] Moving FAST DOWN");
            else
                AddLog("[error] Could not move fast down...");
        }*/
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
