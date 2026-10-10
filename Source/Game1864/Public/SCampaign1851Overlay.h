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
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyChar(const FGeometry& Geometry, const FCharacterEvent& Event) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	void BeginUnitRename(int32 RegimentIndex);
	void OpenUnitUniform(int32 RegimentIndex);
	void CloseUnitCustomisation(bool bAccept = false);
	void ChooseUnitUniform(int32 Swatch);
	int32 CustomUnitIndex = INDEX_NONE;
	bool bEditingUnitName = false;
	bool bEditingUnitUniform = false;
	bool bReplaceUnitName = false;
	FString UnitNameDraft;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920.f, 1080.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

	/** City to show in the info panel; -1 hides it. */
	void SetSelectedCity(int32 Index) { SelectedCity = Index; }
	int32 GetSelectedCity() const { return SelectedCity; }
	/** Regiments selected (a stack, a column or single ones); they take the info panel. */
	void SetSelectedRegiments(const TArray<int32>& In) { if (In != SelectedRegiments) { CloseUnitCustomisation(); StackScroll = 0; } SelectedRegiments = In; if (In.Num() == 0) { Picker = EPicker::None; InspectedOfficer = INDEX_NONE; } }
	const TArray<int32>& GetSelectedRegiments() const { return SelectedRegiments; }
	/** Amt to show when no town is selected (0 = none). */
	void SetSelectedAmt(int32 Id) { SelectedAmt = Id; }
	int32 GetSelectedAmt() const { return SelectedAmt; }

	enum class EButton : uint8 { None, Build, ShowOnMap, BuildModule, Menu, SaveSlot, LoadSlot, CloseMenu, NewGame, Speed, Treasury, BuildTown, ShowSite, BuildLink, ShowLink,
		Regiment, RegimentPiece, RegimentRow, ArmyHome, ArmyHalt, OfficerChange, GeneralChange, OfficerPick, OfficerRecruit, PickerClose, TrainingProgram, OfficerInfo, OfficerCardClose, ProgramPick, RouteMode, ArmyCancel, TownTab,
		MainMenu, WindowClose, TableSort, TableRow, TablePage, OfficerFilter, OfficerDismiss, ClosePanel, ExitGame,
		OfficerPromote, OpenOOB, OOBCommand, OOBUnitChief, CommandGeneralChange, TrainOrder, TrainMove, MinistryBudget, BattleViewEnter, BattleViewLeave,
		OrderAll, OrderUnit, OrderExecute, OrderCancel,
		TreeRow, TreeToggle, TreeNew, FormationInsertHQ, FormationChief, FormationDissolve, FormationDeputy, FormationStaff, FormationChiefRemove,
		TownBuildingsTab, Delegate, Reserve, DecisionExecute, Deviation,
		FortTool, FortChoose, FortSelect, FortGuns, FortDefence, FortTurn, FortShow, FortTrenches, FortPickCompany, FortAddCompany, FortReturn, RaiseBattalion, Demolish, SupplySend, SupplyBuy, SupplyMap, Footing, BattleFight3D, BattleAuto, BattleRetreat, Diplomacy, MakePeace, ResearchStart, DoctrineSet, ShipOrder, Blockade, Loan, OpenGazette, GazetteTab, NewGameNation, NationWeight, DelegateAll, MinisterDismiss, MinisterAppoint, MinisterPickClose, BridgeSelect, BridgeDo, OpenMateriel, RawBuy, KitBuy, ResearchPick, ForeignTab, UnitSize, UnitType, UnitTown, UnitCommand, UnitProgram, UnitRaise, OpenBattlefield, BattlefieldSize, BattlefieldHere, BattlefieldAtBattle, ConfirmYes, ConfirmNo, BuildingInfo, BuildingScroll, UnitCard, HorseBattery, SplitUnit, OOBFocusClear, MapView, Engage, MergeUnit, Block, MinisterInfo, TestBattle, EqualizeUnit, TransferAdj, TransferYes, TransferNo, TransferWhole, TransferGun, Ransom, PoolFold, UnitCardPart, StartMenu, StartLoad, StartTest, ScrollBarV, ScrollBarH, StackList, Scenario, UnitDeployEarly, RaisingPage, UnitRename, UnitUniform, UnitUniformSwatch, UnitUpgrade, UnitCustomClose, ResearchTab, ResearchScroll };
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
	/** The scrollbars of the order-of-battle chart: a click on the thumb or the track starts a drag. */
	void BeginScrollDrag(int32 Which, const FVector2D& ViewportPixel);
	void DragScrollTo(const FVector2D& ViewportPixel);
	void EndScrollDrag() { ScrollDrag = 0; }
	void ToggleStackList() { bStackListOpen = !bStackListOpen; StackScroll = 0; }
	void ScrollStack(int32 Rows) { StackScroll = FMath::Clamp(StackScroll + Rows, 0, FMath::Max(0, StackScrollMax)); }
	bool IsOverUnitCard(const FVector2D& ViewportPixel) const
	{
		const FVector2D Local = ViewportPixel / FMath::Max(PaintScale, 0.01f);
		if (!SelectedRegiments.IsEmpty() && bStackListOpen && Local.X >= StackListMin.X && Local.Y >= StackListMin.Y && Local.X <= StackListMax.X && Local.Y <= StackListMax.Y) { return true; }
		return !SelectedRegiments.IsEmpty() && UnitCardMax.X > UnitCardMin.X && Local.X >= UnitCardMin.X && Local.Y >= UnitCardMin.Y && Local.X <= UnitCardMax.X && Local.Y <= UnitCardMax.Y;
	}
	bool IsScrollDragging() const { return ScrollDrag != 0; }
	void ScrollChart(int32 Steps, bool bVertical = false) { float& S = bVertical ? ChartScrollY : ChartScroll; S = FMath::Max(0.f, S + Steps * 60.f); }
	bool IsOOBOpen() const { return bOOB; }
	void SetCivilTab(bool bIn) { bCivilTab = bIn; }
	/** The supply map: depot ranges and each unit's supply in colour (key F). */
	void ToggleSupplyMap() { bSupplyMap = !bSupplyMap; }
	/** The map's view: 0 normal, 1 supply (the depots' reach), 2 control (occupied towns and their liberation). */
	void SetMapView(int32 View) { MapView = View; bSupplyMap = View == 1; }
	bool IsSupplyMap() const { return bSupplyMap; }
	/** The fort list and the choice of a new fort (the SKANSER button). */
	void ToggleFortTool() { bFortTool = !bFortTool; }
	void HideFortTool() { bFortTool = false; }
	/** The fort shown in its panel (id, 0 = none); placing mode shows the hint. */
	void SelectFort(int32 Id) { SelectedFort = Id; if (Id != 0) { SelectedCity = INDEX_NONE; SelectedRegiments.Reset(); SelectedBridge = 0; } }
	int32 GetSelectedFort() const { return SelectedFort; }
	void SetFortPlacing(int32 Kind) { FortPlacing = Kind; }   // 0 none, 1 small, 2 large
	void ToggleFortPickCompany() { bFortPickCompany = !bFortPickCompany; }
	/** Pulling down asks once: the armed target (fort 1000000 + id; town building city * 100 + kind; garrison city * 100 + 99). */
	void ArmDemolish(int32 Code) { DemolishArmed = Code; }
	int32 GetDemolishArmed() const { return DemolishArmed; }
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
	enum class EWindow : uint8 { None, Army, Officers, Budget, Towns, Trains, Chart, Council, Supply, Foreign, Research, Navy, Gazette, End, Battlefield, Materiel, Nations, ArmyStatus };
	void OpenWindow(EWindow In) { bStackListOpen = false; CloseUnitCustomisation(); if (In == EWindow::Chart && Window != EWindow::Chart) { OOBPlace = INDEX_NONE; OOBFilter.Reset(); OOBFocus = INDEX_NONE; OOBBuilding = INDEX_NONE; } Window = In; SortColumn = 0; bSortDesc = false; Page = 0; if (In != EWindow::Officers) { InspectedOfficer = INDEX_NONE; } }
	EWindow GetWindow() const { return Window; }

	/** A question before a step that costs or cannot be undone (mobilisation, ...): the title, what it does, and
	 *  the button it stands for (JA carries it out with the module). */
	void AskConfirm(const FString& Title, const FString& Text, EButton Action, int32 Module)
	{
		ConfirmTitle = Title; ConfirmText = Text; ConfirmAction = Action; ConfirmModule = Module; bConfirmOpen = true;
	}
	bool IsConfirmOpen() const { return bConfirmOpen; }
	/** Moving men from one company to another: the window with the number to move (buttons change it). */
	void OpenTransfer(int32 Regiment, int32 From, int32 ToRegiment, int32 To, int32 Max, int32 Count) { TransferReg = Regiment; TransferToReg = ToRegiment; TransferFrom = From; TransferTo = To; TransferMax = Max; TransferCount = FMath::Clamp(Count, 1, FMath::Max(Max, 1)); bTransferOpen = true; }
	bool IsTransferOpen() const { return bTransferOpen; }
	void CloseTransfer() { bTransferOpen = false; }
	int32 GetTransferReg() const { return TransferReg; }
	int32 GetTransferFrom() const { return TransferFrom; }
	int32 GetTransferTo() const { return TransferTo; }
	int32 GetTransferToReg() const { return TransferToReg; }
	int32 GetTransferCount() const { return TransferCount; }
	void SetTransferCount(int32 N) { TransferCount = FMath::Clamp(N, 1, FMath::Max(TransferMax, 1)); }
	int32 GetTransferMax() const { return TransferMax; }
	/** JA: the action asked about (and its module); the question closes. */
	EButton TakeConfirm(int32& OutModule) { bConfirmOpen = false; OutModule = ConfirmModule; return ConfirmAction; }
	void CloseConfirm() { bConfirmOpen = false; }

	/** The town's building list: scrolled by rows (wheel or arrows), and the card of one building. */
	bool IsOverBuildings(const FVector2D& ViewportPixel) const
	{
		const FVector2D Local = ViewportPixel / FMath::Max(PaintScale, 0.01f);
		return BuildingRowsTotal > 0 && Local.X >= BuildingMin.X && Local.Y >= BuildingMin.Y && Local.X <= BuildingMax.X && Local.Y <= BuildingMax.Y;
	}
	void ScrollBuildings(int32 Delta) { BuildingScroll = FMath::Clamp(BuildingScroll + Delta, 0, FMath::Max(0, BuildingRowsTotal - BuildingRowsShown)); }
	void ToggleBuildingInfo(int32 Index) { BuildingInfo = BuildingInfo == Index ? INDEX_NONE : Index; }
	/** The unit card beside the unit panel: the soldier in his uniform, the colours, the service record. */
	void ToggleUnitCard() { CloseUnitCustomisation(); bUnitCard = !bUnitCard; }
	/** The order of battle for one unit only (its companies; split it there), or the whole army. */
	/** The units the order-of-battle window shows (from KAMPORDEN on a selection); empty: all of them. */
	/** Back one step: from the two halves to the filtered list, from that to every unit. */
	/** Kamporden from a unit: the garrison and the field army at that town only (INDEX_NONE: everything or the chosen units). */
	void SetOOBPlace(int32 Town) { OOBFilter.Reset(); OOBFocus = INDEX_NONE; OOBBuilding = INDEX_NONE; OOBPlace = Town; TreeScroll = 0; }
	void ClearOOBView() { OOBPlace = INDEX_NONE; OOBBuilding = INDEX_NONE; if (OOBFocus != INDEX_NONE) { const int32 Was = OOBFocus; OOBFocus = INDEX_NONE; if (OOBFilter.Num() > 0 && !OOBFilter.Contains(Was)) { OOBFilter.Add(Was); } } else { OOBFilter.Reset(); } TreeScroll = 0; }
	const TArray<int32>& GetOOBFilter() const { return OOBFilter; }
	/** The new unit being built in the middle of the window (companies dragged there stay there); INDEX_NONE if none. */
	void SetOOBBuilding(int32 Unit) { OOBBuilding = Unit; }
	/** The garrison list in the order of battle: a unit unfolded to show its companies or squadrons. */
	/** The unit card shows the unit's average (INDEX_NONE) or one of its companies or squadrons. */
	void SetUnitCardCompany(int32 Company) { UnitCardCompany = Company; }
	int32 UnitCardCompany = INDEX_NONE;
	/** The scenario chosen in the game menu for the next new game (-1: the one being played). */
	void SetMenuScenario(int32 Index) { MenuScenario = Index; }
	int32 GetMenuScenario() const { return MenuScenario; }
	int32 MenuScenario = -1;
	/** The research window's tab: 0 military, 1 civil. */
	void SetResearchTab(int32 Tab) { ResearchTab = Tab; ResearchPick = -1; ResearchScrollRow = 0; }
	int32 ResearchTab = 0;
	mutable int32 ResearchScrollRow = 0;
	mutable int32 ResearchScrollMax = 0;
	void ScrollResearch(int32 Delta) { ResearchScrollRow = FMath::Clamp(ResearchScrollRow + Delta, 0, ResearchScrollMax); }
	mutable int32 RaisingPageIndex = 0;
	void TogglePoolOpen(int32 Unit) { if (PoolOpen.Contains(Unit)) { PoolOpen.Remove(Unit); } else { PoolOpen.Add(Unit); } }
	TSet<int32> PoolOpen;
	int32 GetOOBBuilding() const { return OOBBuilding; }
	void FilterOOB(const TArray<int32>& Units) { OOBFilter = Units; OOBFocus = INDEX_NONE; }
	/** Show a newly created army, including a company detached from a filtered garrison unit. */
	void RevealOOBArmy(int32 ArmyId, const TArray<int32>& Units)
	{
		FilterOOB(Units);
		OOBBuilding = INDEX_NONE;
		Collapsed.Remove(TreeKey(ETreeKind::Formation, ArmyId));
		ChartScroll = 0.f;
		ChartScrollY = 0.f;
	}
	void FocusOOB(int32 RegimentIndex) { OOBFocus = RegimentIndex; TreeScroll = 0; }

	/** A tooltip over a part of the screen (paint coordinates); the buttons get theirs from ButtonTip. */
	void AddTip(const FVector2D& Pos, const FVector2D& Size, const FString& Text) const { Tips.Add({ Pos, Pos + Size, Text }); }
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
	void OpenMenu(const TArray<FSlotInfo>& Slots) { MenuSlots = Slots; bMenuOpen = true; }
	void CloseMenu() { bMenuOpen = false; bStartMenu = false; bStartLoad = false; }
	void ShowStartMenu() { bStartMenu = true; bStartLoad = false; bMenuOpen = true; MenuScenario = 0; Window = EWindow::None; }
	bool IsStartMenu() const { return bStartMenu; }
	void ShowStartLoad(bool bShow) { bStartLoad = bShow; }
	bool IsMenuOpen() const { return bMenuOpen; }
	/** A short message at the top of the screen (fades after a few seconds). */
	void ShowToast(const FString& Text) { Toast = Text; ToastTime = FPlatformTime::Seconds(); }
	/** A click that starts something slow (new game, load, quit) says so at once, before the work freezes the frame. */
	void ShowBusy(const FString& Text) { BusyText = Text; }
	bool IsBusy() const { return !BusyText.IsEmpty(); }
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
	/** A battle at hand: the forces, the odds, and the choice. */
	void PaintBattle(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	/** The supply window: depots, columns, units in the field, the stores. */
	void PaintSupply(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	/** The council window: the nations, the player's ministries (delegation) and their decisions. */
	void PaintCouncil(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintForeign(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintResearch(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintNavy(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintMenuIcon(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, int32 Index, const FVector2D& Centre, float Size, const FLinearColor& Colour) const;
	void PaintBranchSymbol(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, int32 Branch, const FVector2D& Centre, float Radius) const;
	void PaintGazette(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintEnd(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintBattlefield(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	mutable TSharedPtr<FSlateBrush> BattlefieldBrush;
public:
	/** The candidates for a minister's post shown in the council (-1 none). */
	void SetMinisterPick(int32 Portfolio) { MinisterPick = Portfolio; }
	/** A minister's card (his portrait and record) in the council's right column; -1 none. */
	void SetMinisterInfo(int32 Portfolio) { MinisterInfo = MinisterInfo == Portfolio ? -1 : Portfolio; MinisterPick = -1; }
	int32 GetMinisterInfo() const { return MinisterInfo; }
	/** The bridge shown in its panel (id, 0 = none). */
	void SelectBridge(int32 Id) { SelectedBridge = Id; if (Id != 0) { SelectedCity = INDEX_NONE; SelectedRegiments.Reset(); SelectedFort = 0; } }
	int32 GetSelectedBridge() const { return SelectedBridge; }
	int32 GetMinisterPick() const { return MinisterPick; }
private:
	int32 MinisterPick = -1;
	int32 MinisterInfo = -1;
	mutable FVector2D UnitCardAnchor = FVector2D(-1.f, -1.f);
	mutable FVector2D BuildingCardAnchor = FVector2D(-1.f, -1.f);   // set by the building list: where the building's card goes   // set by the selection panel: where the unit card goes
	void PaintMinisterCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, int32 Portfolio) const;
	/** A portrait in a frame of its own (dark mount, gold double frame). */
	void PaintPortraitBox(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Name, int32 Kind, int32 Age = -1, int32 Rank = -1) const;
	/** The rank as a badge (epaulettes and stars) drawn over the portrait's lower corner: the picture is the person, this is the rank. */
	void PaintRankBadge(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Corner, float Scale, int32 Rank) const;
	int32 SelectedBridge = 0;
public:
	/** The research topic shown in its box (-1 none). */
	int32 ResearchPick = -1;
	int32 ForeignTab = 0;
private:
	void PaintMateriel(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	/** The war minister's survey: the army by arm, the losses on both sides and the booty. */
	void PaintArmyStatus(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
	void PaintNations(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size) const;
public:
	/** The new unit being prepared in the MATERIEL window. */
	int32 RaiseSize = 2;
	int32 RaiseType = 0, RaiseTownPick = 0, RaiseCommand = 0, RaiseProgram = 1;
private:
	void PaintBridge(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
public:
	/** The newspaper's tabs: 0 the paper, 1 the market, 2 statistics, 3 the reference book (4 + n: an entry). */
	void SetGazetteTab(int32 Tab) { if (Tab >= 100) { LexiconEntry = Tab - 100; GazetteTab = 3; } else { GazetteTab = Tab; } }
private:
	int32 GazetteTab = 0;
	int32 LexiconEntry = 0;
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
	bool bSupplyMap = false;
	int32 MapView = 0;
	int32 SelectedFort = 0;
	int32 FortPlacing = 0;
	bool bFortPickCompany = false;
	int32 DemolishArmed = 0;
	/** The town's building list: military (false) or civil (true). */
	bool bCivilTab = false;
	mutable float ChartScrollY = 0.f;
	// The chart's scrollbars: the size of the content (measured while painting), the bars and the drag in progress.
	mutable float ChartContentH = 1200.f, ChartContentW = 0.f, ChartViewH = 0.f, ChartViewW = 0.f;
	mutable FVector2D BarVMin = FVector2D::ZeroVector, BarVMax = FVector2D::ZeroVector, BarHMin = FVector2D::ZeroVector, BarHMax = FVector2D::ZeroVector;
	mutable float ThumbV0 = 0.f, ThumbV1 = 0.f, ThumbH0 = 0.f, ThumbH1 = 0.f;
	int32 ScrollDrag = 0;          // 0 none, 1 chart vertical, 2 chart horizontal, 3 the stack list on the unit card
	// The stack table on the unit card: the first row shown, and its scrollbar.
	mutable int32 StackScroll = 0, StackScrollMax = 0;
	bool bStackListOpen = false;   // the box with all the units of a stack
	mutable FVector2D StackListMin = FVector2D::ZeroVector, StackListMax = FVector2D::ZeroVector;
	mutable FVector2D UnitCardMin = FVector2D::ZeroVector, UnitCardMax = FVector2D::ZeroVector;
	mutable FVector2D CardBarMin = FVector2D::ZeroVector, CardBarMax = FVector2D::ZeroVector;
	mutable float CardThumb0 = 0.f, CardThumb1 = 0.f;
	float ScrollGrab = 0.f;
	mutable FVector2D ChartMin = FVector2D::ZeroVector, ChartMax = FVector2D::ZeroVector;
	mutable FVector2D TreeMin = FVector2D::ZeroVector, TreeMax = FVector2D::ZeroVector;
	bool bDragging = false;
	int32 DragKey = 0;
	int32 HoverKey = 0;

	// ---- tooltips and the confirmation dialog
	struct FTipRect { FVector2D Min; FVector2D Max; FString Text; };
	mutable TArray<FTipRect> Tips;
	mutable FString TipShown;
	mutable double TipSince = 0.0;
	/** What a button does (empty: no tip). */
	FString ButtonTip(EButton Action, int32 Module) const;
	void PaintTooltip(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintConfirm(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintTransfer(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer) const;
	bool bTransferOpen = false;
	int32 TransferToReg = INDEX_NONE;
	int32 TransferReg = INDEX_NONE, TransferFrom = 0, TransferTo = 0, TransferCount = 1, TransferMax = 1;
	FString ConfirmTitle;
	FString ConfirmText;
	EButton ConfirmAction = EButton::None;
	int32 ConfirmModule = 0;
	bool bConfirmOpen = false;
	int32 BuildingScroll = 0;
	bool bUnitCard = false;
	FString BusyText;
	int32 OOBFocus = INDEX_NONE;
	TArray<int32> OOBFilter;
	int32 OOBPlace = INDEX_NONE;   // Kamporden for one town: its garrison and the army standing there
	int32 OOBBuilding = INDEX_NONE;
	TArray<TSharedPtr<FSlateBrush>> UniformBrushes;   // a soldier per arm (ECampaign1851Arm)
	TSharedPtr<FSlateBrush> FlagBrush;
	/** Portraits (types of the time): officers, generals, ministers; one by the person's name. */
	TArray<TSharedPtr<FSlateBrush>> OfficerPortraits, GeneralPortraits, MinisterPortraits;
	const FSlateBrush* PortraitFor(const FString& Name, int32 Kind, int32 Age = -1) const;   // 0 officer, 1 general, 2 minister; Age picks the age group of the officers' pool
	TArray<TSharedPtr<FSlateBrush>> AgePortraits[3];   // officers by age: young, middle, old (no rank on the coat)
	void PaintPortrait(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, const FVector2D& Size, const FString& Name, int32 Kind) const;
	void PaintUnitCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& BottomLeft, int32 RegimentIndex) const;
	void PaintStackList(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& CardPos, const FVector2D& CardSize, const TArray<const FCampaign1851Regiment*>& Sel) const;
	int32 BuildingInfo = INDEX_NONE;
	mutable FVector2D BuildingMin = FVector2D::ZeroVector;
	mutable FVector2D BuildingMax = FVector2D::ZeroVector;
	mutable int32 BuildingRowsTotal = 0;
	mutable int32 BuildingRowsShown = 0;
	void PaintBuildingCard(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 Layer, const FVector2D& Pos, int32 TypeIndex) const;
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
	bool bStartMenu = false;
	bool bStartLoad = false;
	TArray<FSlotInfo> MenuSlots;
	FString Toast;
	double ToastTime = -100.0;
};
