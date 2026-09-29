#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Campaign1851Army.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"

class ACampaign1851Map;
class ACampaign1851Camera;
class APlayerController;
struct FCampaign1851City;

/**
 * Screen overlay for the 1851 campaign map, painted directly each frame: title cartouche, legend,
 * compass, scale bar, Bornholm inset, city info and all place names (projected, zoom-filtered and
 * collision-resolved by priority). Dark navy panels with gold rules and a serif face.
 */
class GAME1864_API SCampaign1851Overlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCampaign1851Overlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ACampaign1851Map>, Map)
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

	/** City to show in the info panel; -1 hides it. */
	void SetSelectedCity(int32 Index) { SelectedCity = Index; }
	int32 GetSelectedCity() const { return SelectedCity; }
	/** Regiments selected (a stack, a column or single ones); they take the info panel. */
	void SetSelectedRegiments(const TArray<int32>& In) { SelectedRegiments = In; if (In.Num() == 0) { Picker = EPicker::None; InspectedOfficer = INDEX_NONE; } }
	const TArray<int32>& GetSelectedRegiments() const { return SelectedRegiments; }
	/** Amt to show when no town is selected (0 = none). */
	void SetSelectedAmt(int32 Id) { SelectedAmt = Id; }
	int32 GetSelectedAmt() const { return SelectedAmt; }

	enum class EButton : uint8 { None, Build, ShowOnMap, BuildModule, Menu, SaveSlot, LoadSlot, CloseMenu, NewGame, Speed, Treasury, BuildTown, ShowSite, BuildLink, ShowLink,
		Regiment, RegimentPiece, RegimentRow, ArmyHome, ArmyHalt, OfficerChange, GeneralChange, OfficerPick, OfficerRecruit, PickerClose, TrainingProgram, OfficerInfo, OfficerCardClose, ProgramPick, RouteMode, ArmyCancel, TownTab,
		MainMenu, WindowClose, TableSort, TableRow, TablePage, OfficerFilter, OfficerDismiss, ClosePanel, ExitGame,
		OfficerPromote, OpenOOB, OOBCommand, CommandGeneralChange, TrainOrder,
		OrderAll, OrderUnit, OrderExecute, OrderCancel,
		TreeRow, TreeToggle, TreeNew, FormationChief, FormationDissolve, FormationDeputy, FormationStaff,
		TownBuildingsTab, Delegate, Reserve, DecisionExecute, Deviation,
		FortTool, FortChoose, FortSelect, FortGuns, FortDefence, FortTurn, FortShow, FortTrenches, FortPickCompany, FortAddCompany, FortReturn, RaiseBattalion };
	/** Kinds of rows in the order-of-battle tree; a row's key is Kind * 100000 + Id. */
	enum class ETreeKind : uint8 { None, Formation, Regiment, Command, FieldArmy, Garrisons, ArmGroup, Company, NewFormation };
	static int32 TreeKey(ETreeKind Kind, int32 Id) { return int32(Kind) * 100000 + Id; }
	static ETreeKind TreeKind(int32 Key) { return ETreeKind(Key / 100000); }
	static int32 TreeId(int32 Key) { return Key % 100000; }
	void ToggleCollapsed(int32 Key) { if (Collapsed.Contains(Key)) { Collapsed.Remove(Key); } else { Collapsed.Add(Key); } }
	void ScrollTree(int32 Rows) { TreeScroll = FMath::Max(0, TreeScroll + Rows); }
	bool IsOverTree(const FVector2D& ViewportPixel) const;
	/** Over the order-of-battle chart (the wheel scrolls it sideways). */
	bool IsOverChart(const FVector2D& ViewportPixel) const;
	void ScrollChart(int32 Steps, bool bVertical = false) { float& S = bVertical ? ChartScrollY : ChartScroll; S = FMath::Max(0.f, S + Steps * 60.f); }
	bool IsOOBOpen() const { return bOOB; }
	void SetCivilTab(bool bIn) { bCivilTab = bIn; }
	/** The fort list and the choice of a new fort (the SKANSER button). */
	void ToggleFortTool() { bFortTool = !bFortTool; }
	void HideFortTool() { bFortTool = false; }
	/** The fort shown in its panel (id, 0 = none); placing mode shows the hint. */
	void SelectFort(int32 Id) { SelectedFort = Id; if (Id != 0) { SelectedCity = INDEX_NONE; SelectedRegiments.Reset(); } }
	int32 GetSelectedFort() const { return SelectedFort; }
	void SetFortPlacing(int32 Kind) { FortPlacing = Kind; }   // 0 none, 1 small, 2 large
	void ToggleFortPickCompany() { bFortPickCompany = !bFortPickCompany; }
	int32 GetFortPlacing() const { return FortPlacing; }
	/** Drag and drop in the tree: what is dragged, the cursor (viewport pixels) and the row under it. */
	void SetDrag(bool bOn, int32 Key, const FVector2D& ViewportPixel, int32 Hover) { bDragging = bOn; DragKey = Key; DragPos = ViewportPixel / FMath::Max(PaintScale, 0.01f); HoverKey = Hover; }
	/** The officer picker for a formation's commander (generals for divisions, officers for brigades). */
	void OpenFormationPicker(int32 Formation, bool bGenerals, int32 Post = 0) { Picker = bGenerals ? EPicker::FormationGeneral : EPicker::FormationOfficer; PickerFormation = Formation; PickerPost = Post; InspectedOfficer = INDEX_NONE; bTrainingMenu = false; }
	int32 GetPickerFormation() const { return PickerFormation; }
	/** The headquarters post being filled: 0 chief, 1 deputy, 2 chief of staff. */
	int32 GetPickerPost() const { return PickerPost; }
	/**
	 * The march order dialog (right click): for every selected unit on foot / by train / straight across,
	 * the times, and the columns it makes (each way its own column).
	 */
	struct FOrderDialog
	{
		bool bOpen = false;
		int32 Town = INDEX_NONE;
		FVector2D Km = FVector2D::ZeroVector;
		FString Goal;
		TArray<int32> Units;
		TArray<uint8> Ways;            // per unit: 0 on foot, 1 by train, 2 straight across
		FString AllTimes[3];           // the whole selection each way
		TArray<FString> Columns;       // one line per column the order makes
	};
	FOrderDialog& EditOrder() { return OrderDialog; }
	const FOrderDialog& GetOrder() const { return OrderDialog; }
	/** What an X in a panel's corner closes (the Module of EButton::ClosePanel). */
	enum : int32 { CloseTownTab = 1, CloseTraining, ClosePicker, CloseOfficerCard, CloseWindow, CloseSelection, CloseLedger, CloseOOB, CloseOrder, CloseFortPanel, CloseFort };
	/** The big windows opened from the menu bar under the calendar (one at a time). */
	enum class EWindow : uint8 { None, Army, Officers, Budget, Towns, Trains, Chart, Council };
	void OpenWindow(EWindow In) { Window = In; SortColumn = 0; bSortDesc = false; Page = 0; if (In != EWindow::Officers) { InspectedOfficer = INDEX_NONE; } }
	EWindow GetWindow() const { return Window; }
	/** Sort a table by a column (again: the other way round). */
	void SetSort(int32 Column) { bSortDesc = Column == SortColumn ? !bSortDesc : Column > 3; SortColumn = Column; Page = 0; }
	void TurnPage(int32 Delta) { Page = FMath::Max(0, Page + Delta); }
	void SetOfficerFilter(int32 Filter) { OfficerFilter = Filter; Page = 0; }
	/** The town card's side panel: 0 none, 1 garrison, 2 buildings, 3 roads and railways. */
	void SetTownTab(int32 Tab) { TownTab = Tab; }
	int32 GetTownTab() const { return TownTab; }
	/** How the next march order goes (the three buttons in the army panel). */
	void SetRouteMode(ECampaign1851RouteMode In) { RouteMode = In; }
	ECampaign1851RouteMode GetRouteMode() const { return RouteMode; }
	/** The training menu beside the army panel. */
	void ToggleTrainingMenu() { bTrainingMenu = !bTrainingMenu; if (bTrainingMenu) { Picker = EPicker::None; InspectedOfficer = INDEX_NONE; } }
	void CloseTrainingMenu() { bTrainingMenu = false; }
	bool IsTrainingMenuOpen() const { return bTrainingMenu; }
	/** The officer list beside the army panel: chiefs or generals to appoint. */
	enum class EPicker : uint8 { None, Chief, General, CommandGeneral, FormationGeneral, FormationOfficer };
	void OpenPicker(EPicker In) { Picker = In; InspectedOfficer = INDEX_NONE; bTrainingMenu = false; }
	/** The general picker for a general command's commanding general. */
	void OpenCommandPicker(int32 Command) { OpenPicker(EPicker::CommandGeneral); PickerCommand = Command; }
	int32 GetPickerCommand() const { return PickerCommand; }
	/** The order of battle beside the army panel. */
	void ToggleOOB() { bOOB = !bOOB; }
	void ExpandOOB(int32 Command) { OOBExpanded = OOBExpanded == Command ? -2 : Command; }
	/** The officer card (all qualities) beside the army panel; INDEX_NONE closes it. */
	void InspectOfficer(int32 Officer) { InspectedOfficer = Officer; }
	int32 GetInspectedOfficer() const { return InspectedOfficer; }
	EPicker GetPicker() const { return Picker; }
	/** Module of a BuildLink button: link * 2 + 0 for a chaussée, + 1 for a railway. */
	static int32 LinkButton(int32 Link, bool bRailway) { return Link * 2 + (bRailway ? 1 : 0); }
	void ToggleLedger() { bLedgerOpen = !bLedgerOpen; }

	/** A row of the game menu (save/load). */
	struct FSlotInfo
	{
		FString Label;   // "Autogem", "Plads 1", ...
		bool bExists = false;
		bool bCanSave = true;
		FString Info;    // date and summary, or "Tom"
	};
	void OpenMenu(const TArray<FSlotInfo>& Slots) { MenuSlots = Slots; bMenuOpen = true; bConfirmNewGame = false; }
	/** "New game" needs a second click; the button then reads "BEKRÆFT: NYT SPIL". */
	void SetConfirmNewGame(bool bConfirm) { bConfirmNewGame = bConfirm; }
	bool IsConfirmingNewGame() const { return bConfirmNewGame; }
	void CloseMenu() { bMenuOpen = false; }
	bool IsMenuOpen() const { return bMenuOpen; }
	/** A short message at the top of the screen (fades after a few seconds). */
	void ShowToast(const FString& Text) { Toast = Text; ToastTime = FPlatformTime::Seconds(); }
	/** The panel button under a viewport pixel (as from APlayerController::GetMousePosition). */
	EButton HitButton(const FVector2D& ViewportPixel, int32* OutModule = nullptr) const;

private:
	struct FPlaced
	{
		FVector2D Min, Max;
	};

	bool ToLocal(const FGeometry& Geometry, const FVector& World, FVector2D& OutLocal) const;
	int32 PaintLabels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float DistanceKm) const;
	void PaintPanel(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintText(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FString& Text, const FVector2D& Pos,
		const FSlateFontInfo& Font, const FLinearColor& Colour, float AlignX = 0.f, bool bShadow = true) const;
	void PaintTitle(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintLegend(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintCompass(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float Yaw) const;
	void PaintScaleBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintBornholm(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The town's buildings beside the garrison card: built, under way, or what they cost and need. */
	void PaintTownBuildings(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The town's roads to its neighbours: distance, march or train time, and chaussée / railway projects. */
	void PaintTownLinks(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, float Left) const;
	/** Progress line for a link project, as ProgressLine for a building. */
	FString LinkProgressLine(int32 Link) const;
	/** The selected amt: region, seat, population, area, towns, taxes, men of military age, garrisons. */
	void PaintAmtInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** Unit counters (NATO style, Danish blue) on every town with regiments and on every column on the march; routes of the selected. */
	void PaintArmy(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The selected regiments: strength, qualities, place, march pace and arrival, chief and general, orders. */
	void PaintArmyInfo(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** Unassigned officers or generals with their qualities; click to appoint, or recruit a new one. */
	void PaintOfficerPicker(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const;
	/** A small X button in a panel's top right corner. */
	void PaintCloseX(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& TopRight, int32 What) const;
	/** Army / Officers / Treasury / Towns buttons under the calendar. */
	void PaintMenuBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The open big window: a sortable table (or the budget), rows clickable. */
	void PaintWindow(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	struct FTableColumn { FString Title; float Width = 80.f; bool bRight = false; };
	struct FTableRow { TArray<FString> Cells; TArray<double> Keys; int32 Id = INDEX_NONE; };
	/** Headings (click to sort), striped rows (click: RowAction with the row's Id), pages. */
	void PaintTable(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, int32 VisibleRows,
		const TArray<FTableColumn>& Columns, TArray<FTableRow> Rows, EButton RowAction, int32 Highlight) const;
	/** The march order dialog. */
	void PaintOrderDialog(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	struct FTreeRow
	{
		int32 Key = 0;
		int32 Depth = 0;
		FString Text;
		FString Info;
		bool bSelected = false;
		bool bFormation = false;
		bool bHasChildren = false;
		bool bOpen = true;
	};
	/** The SKANSER panel (new fort, list of forts) and a fort's own panel. */
	void PaintFortTool(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintFort(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The council window: the nations, the player's ministries (delegation) and their decisions. */
	void PaintCouncil(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	/** The field army as an organisation chart: HQ boxes, units stacked under them, connecting lines. */
	void PaintOOBChart(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	/** The tree's rows as they are open now: the field army's formations, then the garrisons by command and arm. */
	void BuildTreeRows(TArray<FTreeRow>& Rows) const;
public:
	FString TreeKeyText(int32 Key) const;
private:
	/** Training menu, officer picker or card: beside the army panel (or beside the tree when that is open). */
	void PaintSidePanels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The order of battle: the army, its general commands and their regiments by arm; the selected lit. */
	void PaintOOB(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const;
	/** Every training programme: what it trains, days for +10 under the chief, cost a month; click to choose. */
	void PaintTrainingMenu(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const;
	/** One officer's card: rank, age, post, experience and every quality with what it means. */
	void PaintOfficerCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft) const;
	/** Left-aligned (or AlignX) text cut with "..." to fit MaxWidth. */
	void PaintTextFit(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FString& Text, const FVector2D& Pos,
		const FSlateFontInfo& Font, const FLinearColor& Colour, float MaxWidth, float AlignX = 0.f) const;
	/** "Før 7  Insp 6  ...  ·  erf. 45" */
	FString OfficerStatLine(const struct FCampaign1851Officer& O) const;
	/** Progress rings at the middle of roads and railways under construction. */
	void PaintLinkWorks(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** Progress rings over towns with a building project. */
	void PaintProjects(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintButton(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Text, EButton Action, int32 Module = INDEX_NONE, bool bHighlight = false, bool bDisabled = false) const;
	/** Cash, monthly grant and upkeep under the game menu button; click for the account book. */
	void PaintTreasury(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintLedger(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** Stage line for a module under way: stage, money shortage or winter pace, expected date. */
	FString ProgressLine(const class ACampaign1851ConstructionSite* Site, int32 Module) const;
	/** Date, season and the speed buttons, top centre. */
	void PaintCalendar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintBar(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, float Width, float Fraction) const;
	void PaintMenu(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintToast(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintDot(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Centre, float Diameter, const FLinearColor& Colour) const;
	FVector2D Measure(const FString& Text, const FSlateFontInfo& Font) const;

	TWeakObjectPtr<ACampaign1851Map> Map;
	TWeakObjectPtr<APlayerController> Controller;
	int32 SelectedCity = INDEX_NONE;
	int32 SelectedAmt = 0;
	TArray<int32> SelectedRegiments;
	EPicker Picker = EPicker::None;
	int32 InspectedOfficer = INDEX_NONE;
	bool bTrainingMenu = false;
	bool bOOB = false;
	int32 OOBExpanded = INDEX_NONE;   // the general command shown open (INDEX_NONE: the selected unit's; -2: none)
	int32 PickerCommand = INDEX_NONE;
	int32 PickerFormation = 0;
	int32 PickerPost = 0;
	TSet<int32> Collapsed;
	mutable int32 TreeScroll = 0;
	mutable float ChartScroll = 0.f;
	bool bFortTool = false;
	int32 SelectedFort = 0;
	int32 FortPlacing = 0;
	bool bFortPickCompany = false;
	/** The town's building list: military (false) or civil (true). */
	bool bCivilTab = false;
	mutable float ChartScrollY = 0.f;
	mutable FVector2D ChartMin = FVector2D::ZeroVector, ChartMax = FVector2D::ZeroVector;
	mutable FVector2D TreeMin = FVector2D::ZeroVector, TreeMax = FVector2D::ZeroVector;
	bool bDragging = false;
	int32 DragKey = 0;
	int32 HoverKey = 0;
	FVector2D DragPos = FVector2D::ZeroVector;
	FOrderDialog OrderDialog;
	int32 TownTab = 0;
	EWindow Window = EWindow::None;
	int32 SortColumn = 0;
	bool bSortDesc = false;
	int32 Page = 0;
	int32 OfficerFilter = 0;
	ECampaign1851RouteMode RouteMode = ECampaign1851RouteMode::RoadsAndRail;
	TSharedPtr<FSlateBrush> BornholmBrush;
	TSharedPtr<FSlateBrush> DotBrush;
	TArray<TSharedPtr<FSlateBrush>> ModuleBrushes;   // card images per garrison module
	TArray<TSharedPtr<FSlateBrush>> TownBrushes;     // card images per town building
	TArray<TStrongObjectPtr<UTexture2D>> CardTextures;   // keeps the card images alive

	struct FButtonRect { FVector2D Min, Max; EButton Action; int32 Module; };
	mutable TArray<FButtonRect> Buttons;   // local units, rebuilt every paint
	mutable float PaintScale = 1.f;

	bool bMenuOpen = false;
	bool bLedgerOpen = false;
	bool bConfirmNewGame = false;
	TArray<FSlotInfo> MenuSlots;
	FString Toast;
	double ToastTime = -100.0;
};
