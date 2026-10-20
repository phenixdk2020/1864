#pragma once
#include "CoreMinimal.h"
class FJsonObject;

/** Analysis only; never serialized into a campaign save. All calls are on the game thread. */
class FCampaign1851DecisionLog
{
public:
 ~FCampaign1851DecisionLog();
 bool Open(int32 LogScenario, const FString& LogResumeFile = FString());
 void Session(const FString& LogDate, const FString& LogEvent, int32 LogScenario, int32 LogVersion, const FString& LogSlot);
 void Decision(const TSharedRef<FJsonObject>& LogRecord);
 void Event(const TSharedRef<FJsonObject>& LogRecord);
 void Snapshot(const TSharedRef<FJsonObject>& LogRecord);
 void Flush();
 void Close();
 const FString& Filename() const { return LogFilename; }
 static FString Directory();
private:
 void Append(const TSharedRef<FJsonObject>& LogRecord, const TCHAR* LogRecordType);
 FString LogFilename, LogBuffer;
 bool bLogFailed = false;
};

class ACampaign1851Map;
class FCampaign1851LogActorScope
{
public:
 FCampaign1851LogActorScope(ACampaign1851Map* LogMap, const TCHAR* LogActorName);
 ~FCampaign1851LogActorScope();
private:
 ACampaign1851Map* LogMapPtr;
 FString LogPreviousActor;
};
