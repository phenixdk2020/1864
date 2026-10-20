#include "Campaign1851DecisionLog.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Containers/StringConv.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"

FString FCampaign1851DecisionLog::Directory() { return FPaths::ProjectSavedDir() / TEXT("CampaignLogs"); }
FCampaign1851DecisionLog::~FCampaign1851DecisionLog() { Close(); }
bool FCampaign1851DecisionLog::Open(int32 LogScenario, const FString& LogResumeFile)
{
 Close();
 IPlatformFile& LogPlatform = FPlatformFileManager::Get().GetPlatformFile();
 bLogFailed = false;
 if (!LogPlatform.CreateDirectoryTree(*Directory())) { bLogFailed = true; UE_LOG(LogTemp, Warning, TEXT("Kampagneloggens mappe kunne ikke oprettes")); return false; }
 // Sidecars may only name a local JSONL file, never an arbitrary path.
 if (!LogResumeFile.IsEmpty() && FPaths::GetCleanFilename(LogResumeFile) == LogResumeFile && LogResumeFile.EndsWith(FString::Printf(TEXT("_%d.jsonl"), LogScenario))
     && LogPlatform.FileExists(*(Directory() / LogResumeFile)))
  LogFilename = LogResumeFile;
 else
 {
  const FString LogStem = FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S_%sZ"));
  LogFilename = FString::Printf(TEXT("%s_%d.jsonl"), *LogStem, LogScenario);
  for (int32 LogSuffix = 1; LogPlatform.FileExists(*(Directory() / LogFilename)); ++LogSuffix)
   LogFilename = FString::Printf(TEXT("%s_%d_%d.jsonl"), *LogStem, LogSuffix, LogScenario);
 }
 // A crash may leave an incomplete last record. Isolate it so later valid rows stay readable.
 {
  TUniquePtr<IFileHandle> LogReader(LogPlatform.OpenRead(*(Directory() / LogFilename)));
  if (LogReader && LogReader->Size() > 0)
  {
   uint8 LogLastByte = 0;
   if (LogReader->Seek(LogReader->Size() - 1) && LogReader->Read(&LogLastByte, 1) && LogLastByte != '\n') LogBuffer = TEXT("\r\n");
  }
 }
 TUniquePtr<IFileHandle> LogHandle(LogPlatform.OpenWrite(*(Directory() / LogFilename), true, true));
 if (!LogHandle) { bLogFailed = true; UE_LOG(LogTemp, Warning, TEXT("Kampagnelog kunne ikke åbnes: %s"), *LogFilename); }
 return !bLogFailed;
}
void FCampaign1851DecisionLog::Append(const TSharedRef<FJsonObject>& LogRecord, const TCHAR* LogRecordType)
{
 if (LogFilename.IsEmpty() || bLogFailed) return;
 LogRecord->SetStringField(TEXT("t"), LogRecordType);
 FString LogLine;
 const auto LogWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&LogLine);
 if (FJsonSerializer::Serialize(LogRecord, LogWriter)) LogBuffer += LogLine + TEXT("\r\n");
 if (LogBuffer.Len() >= 32768) Flush();
}
void FCampaign1851DecisionLog::Session(const FString& LogDate, const FString& LogEvent, int32 LogScenario, int32 LogVersion, const FString& LogSlot)
{
 const auto LogRecord = MakeShared<FJsonObject>();
 LogRecord->SetStringField(TEXT("date"), LogDate);
 LogRecord->SetStringField(TEXT("event"), LogEvent);
 LogRecord->SetNumberField(TEXT("scenario"), LogScenario);
 LogRecord->SetNumberField(TEXT("save_version"), LogVersion);
 LogRecord->SetStringField(TEXT("slot"), LogSlot);
 Append(LogRecord, TEXT("session"));
 Flush();
}
void FCampaign1851DecisionLog::Decision(const TSharedRef<FJsonObject>& LogRecord) { Append(LogRecord, TEXT("decision")); }
void FCampaign1851DecisionLog::Event(const TSharedRef<FJsonObject>& LogRecord) { Append(LogRecord, TEXT("event")); }
void FCampaign1851DecisionLog::Snapshot(const TSharedRef<FJsonObject>& LogRecord) { Append(LogRecord, TEXT("snapshot")); }
void FCampaign1851DecisionLog::Flush()
{
 if (LogBuffer.IsEmpty() || bLogFailed || LogFilename.IsEmpty()) return;
 IPlatformFile& LogPlatform = FPlatformFileManager::Get().GetPlatformFile();
 TUniquePtr<IFileHandle> LogHandle(LogPlatform.OpenWrite(*(Directory() / LogFilename), true, true));
 const FTCHARToUTF8 LogBytes(*LogBuffer);
 if (!LogHandle || !LogHandle->Write(reinterpret_cast<const uint8*>(LogBytes.Get()), LogBytes.Length()) || !LogHandle->Flush())
 {
  // A partial write must never be retried: it would duplicate records. Disable only analysis.
  bLogFailed = true;
  UE_LOG(LogTemp, Warning, TEXT("Kampagnelog stoppet efter skrivefejl: %s"), *LogFilename);
 }
 LogBuffer.Reset();
}
void FCampaign1851DecisionLog::Close() { Flush(); LogFilename.Reset(); LogBuffer.Reset(); }

#include "Campaign1851Map.h"
FCampaign1851LogActorScope::FCampaign1851LogActorScope(ACampaign1851Map* LogMap, const TCHAR* LogActorName) : LogMapPtr(LogMap)
{
 if (LogMapPtr) { LogPreviousActor = LogMapPtr->CampaignLogActor; LogMapPtr->CampaignLogActor = LogActorName; }
}
FCampaign1851LogActorScope::~FCampaign1851LogActorScope()
{
 if (LogMapPtr) LogMapPtr->CampaignLogActor = LogPreviousActor;
}

FString ACampaign1851Map::CampaignLogNation() const
{
 return Nations.IsValidIndex(GetPlayedNation()) ? Nations[GetPlayedNation()].Id : TEXT("DK");
}
void ACampaign1851Map::SuspendCampaignLog()
{
 if (CampaignDecisionLog) CampaignDecisionLog->Close();
 CampaignDecisionLog.Reset();
 CampaignLogLastDay = INDEX_NONE;
 CampaignLogLastAutosaveDate.Reset();
 LogCampaignIncome.Reset();
 LogCampaignExpenses.Reset();
}
void ACampaign1851Map::CampaignLogSession(const FString& LogEvent, const FString& LogSlot, int32 LogVersion, const FString& LogStamp)
{
 const FString LogDate = GetDate().ToString(TEXT("%Y-%m-%d"));
 const FString LogSidecar = FCampaign1851DecisionLog::Directory() / (TEXT("slot_") + FPaths::MakeValidFileName(LogSlot) + TEXT(".txt"));
 if (LogEvent == TEXT("new") || LogEvent == TEXT("load"))
 {
  SuspendCampaignLog();
  FString LogResume;
  if (LogEvent == TEXT("load"))
  {
   FString LogLink;
   if (FFileHelper::LoadFileToString(LogLink, *LogSidecar))
   {
    TArray<FString> LogLinkParts;
    LogLink.ParseIntoArray(LogLinkParts, TEXT("\n"), false);
    if (LogLinkParts.Num() >= 2 && LogLinkParts[0] == LogStamp)
    {
     LogResume = LogLinkParts[1].TrimStartAndEnd();
     if (LogLinkParts.Num() >= 3) CampaignLogLastAutosaveDate = LogLinkParts[2].TrimStartAndEnd();
    }
   }
  }
  CampaignDecisionLog = MakeShared<FCampaign1851DecisionLog>();
  CampaignDecisionLog->Open(ActiveScenario().Year, LogResume);
  CampaignLogLastDay = LogEvent == TEXT("load") ? FMath::FloorToInt(CampaignDays) : INDEX_NONE;
 }
 if (!CampaignDecisionLog) return;
 // Every successful save updates the slot link, even when its session row is throttled.
 if (LogEvent == TEXT("save"))
 {
  const FString LogPreviousAutosave = CampaignLogLastAutosaveDate;
  if (LogSlot == TEXT("Autosave")) CampaignLogLastAutosaveDate = LogDate;
  const FString LogLink = LogStamp + TEXT("\n") + CampaignDecisionLog->Filename() + TEXT("\n") + CampaignLogLastAutosaveDate;
  if (!FFileHelper::SaveStringToFile(LogLink, *LogSidecar, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
   UE_LOG(LogTemp, Warning, TEXT("Kampagneloggens slot-link kunne ikke gemmes: %s"), *LogSlot);
  if (LogSlot == TEXT("Autosave") && LogPreviousAutosave == LogDate) { CampaignDecisionLog->Flush(); return; }
  if (LogSlot == TEXT("Autosave")) CampaignLogLastAutosaveDate = LogDate;
 }
 CampaignDecisionLog->Session(LogDate, LogEvent, ActiveScenario().Year, LogVersion, LogSlot);
 if (LogEvent == TEXT("quit")) CampaignDecisionLog->Close();
}
void ACampaign1851Map::LogCampaignDecision(const FString& LogNation, const FString& LogKind, const FString& LogWhat, const FString& LogWhy,
 const FString& LogActorName, const TMap<FString, double>& LogNumbers, const TMap<FString, FString>& LogStrings)
{
 if (!CampaignDecisionLog) return;
 const auto LogRecord = MakeShared<FJsonObject>();
 LogRecord->SetStringField(TEXT("date"), GetDate().ToString(TEXT("%Y-%m-%d")));
 LogRecord->SetStringField(TEXT("nation"), LogNation);
 LogRecord->SetStringField(TEXT("actor"), LogActorName.IsEmpty() ? CampaignLogActor : LogActorName);
 LogRecord->SetStringField(TEXT("kind"), LogKind);
 LogRecord->SetStringField(TEXT("what"), LogWhat);
 LogRecord->SetStringField(TEXT("why"), LogWhy);
 const auto LogInputs = MakeShared<FJsonObject>();
 for (const auto& LogPair : LogNumbers) if (FMath::IsFinite(LogPair.Value)) LogInputs->SetNumberField(LogPair.Key, LogPair.Value);
 for (const auto& LogPair : LogStrings) LogInputs->SetStringField(LogPair.Key, LogPair.Value);
 LogRecord->SetObjectField(TEXT("inputs"), LogInputs);
 CampaignDecisionLog->Decision(LogRecord);
}
void ACampaign1851Map::LogCampaignEvent(const FString& LogId, bool bLogPossible, float LogProbability, bool bLogFired, const FString& LogWhy)
{
 if (!CampaignDecisionLog) return;
 const auto LogRecord = MakeShared<FJsonObject>();
 LogRecord->SetStringField(TEXT("date"), GetDate().ToString(TEXT("%Y-%m-%d")));
 LogRecord->SetStringField(TEXT("id"), LogId);
 LogRecord->SetStringField(TEXT("nation"), Nations.IsValidIndex(PlayerNation) ? Nations[PlayerNation].Id : TEXT("DK"));
 LogRecord->SetBoolField(TEXT("possible"), bLogPossible);
 LogRecord->SetNumberField(TEXT("probability"), LogProbability);
 LogRecord->SetBoolField(TEXT("fired"), bLogFired);
 LogRecord->SetStringField(TEXT("why"), LogWhy);
 CampaignDecisionLog->Event(LogRecord);
}
void ACampaign1851Map::TickCampaignLog()
{
 if (!CampaignDecisionLog) return;
 const int32 LogToday = FMath::FloorToInt(CampaignDays);
 if (LogToday == CampaignLogLastDay) return;
 CampaignLogLastDay = LogToday;
 if (GetDate().GetDayOfWeek() == EDayOfWeek::Monday) SnapshotCampaignLog();
 CampaignDecisionLog->Flush();
}
void ACampaign1851Map::SnapshotCampaignLog()
{
 if (!CampaignDecisionLog) return;
 for (int32 LogIndex = 0; LogIndex < Nations.Num(); ++LogIndex)
 {
  const FCampaign1851Nation& LogNation = Nations[LogIndex];
  const auto LogRecord = MakeShared<FJsonObject>();
  LogRecord->SetStringField(TEXT("date"), GetDate().ToString(TEXT("%Y-%m-%d")));
  LogRecord->SetStringField(TEXT("nation"), LogNation.Id);
  const auto LogRelations = MakeShared<FJsonObject>();
  if (LogIndex == PlayerNation)
  {
   for (const FCampaign1851Nation& LogOther : Nations) if (LogOther.Id != LogNation.Id) LogRelations->SetNumberField(LogOther.Id, LogOther.Relation);
  }
  else if (Nations.IsValidIndex(PlayerNation)) LogRelations->SetNumberField(Nations[PlayerNation].Id, LogNation.Relation);
  LogRecord->SetObjectField(TEXT("relations"), LogRelations);
  if (LogNation.Id == TEXT("DK"))
  {
   LogRecord->SetNumberField(TEXT("treasury"), Treasury);
   double LogIncome = 0.0, LogExpenses = 0.0, LogArmy = 0.0;
   for (const FCampaign1851BudgetLine& LogLine : MonthlyBudget())
    if (LogLine.PerMonth >= 0.0) LogIncome += LogLine.PerMonth; else LogExpenses -= LogLine.PerMonth;
   for (const FCampaign1851Regiment& LogRegiment : Regiments) if (LogRegiment.Nation == LogNation.Id) LogArmy += LogRegiment.PresentMen();
   LogRecord->SetNumberField(TEXT("income"), LogIncome);
   LogRecord->SetNumberField(TEXT("expenses"), LogExpenses);
   LogRecord->SetNumberField(TEXT("army_men"), LogArmy);
   LogRecord->SetNumberField(TEXT("mobilisation"), int32(Footing));
   LogRecord->SetNumberField(TEXT("districts"), Amter.Num());
  }
  else
  {
   LogRecord->SetNumberField(TEXT("treasury"), ForeignTreasuries.Contains(LogNation.Id) ? ForeignTreasury(LogNation.Id) : LogNation.Treasury);
   int32 LogDistricts = 0, LogStage = 0;
   double LogArmy = 0.0;
   bool bLogHasArmy = false;
   for (const FCampaign1851NeighbourDistrict& LogDistrict : NeighbourDistricts) if (LogDistrict.Nation == LogNation.Id) ++LogDistricts;
   for (const FCampaign1851NeighbourArmy& LogUnit : NeighbourArmies) if (LogUnit.Nation == LogNation.Id)
   {
    bLogHasArmy = true;
    LogStage = FMath::Max(LogStage, LogUnit.MobilisationStage);
    if (LogUnit.CorpsId == 0) LogArmy += LogUnit.Men;
   }
   for (const FCampaign1851EnemyCorps& LogCorps : EnemyCorps) if (LogCorps.Nation == LogNation.Id) LogArmy += LogCorps.Men;
   // On-map regiments (including allied units) retain their nation, so do not count them as Danish.
   for (const FCampaign1851Regiment& LogRegiment : Regiments) if (LogRegiment.Nation == LogNation.Id) { LogArmy += LogRegiment.PresentMen(); bLogHasArmy = true; }
   LogRecord->SetNumberField(TEXT("army_men"), bLogHasArmy ? LogArmy : LogNation.ArmyMen);
   if (bLogHasArmy) LogRecord->SetNumberField(TEXT("mobilisation"), LogStage);
   if (LogDistricts > 0) LogRecord->SetNumberField(TEXT("districts"), LogDistricts);
  }
  if (const double* LogIncome = LogCampaignIncome.Find(LogNation.Id)) LogRecord->SetNumberField(TEXT("income"), *LogIncome);
  if (const double* LogExpenses = LogCampaignExpenses.Find(LogNation.Id)) LogRecord->SetNumberField(TEXT("expenses"), *LogExpenses);
  CampaignDecisionLog->Snapshot(LogRecord);
 }
}
void ACampaign1851Map::SetDelegation(ECampaign1851Portfolio P, ECampaign1851Delegation Mode)
{
 if (Nations.IsValidIndex(PlayerNation))
 {
  Nations[PlayerNation].Modes[int32(P)] = Mode;
  LogCampaignDecision(CampaignLogNation(), TEXT("other"), TEXT("Delegation ændret"), TEXT("Valgt ministerstyring"), FString(),
   { { TEXT("portfolio"), double(int32(P)) }, { TEXT("mode"), double(int32(Mode)) } });
 }
}
