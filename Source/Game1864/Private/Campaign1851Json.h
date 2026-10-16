// Optional overlay access: absence/null preserves defaults; unknown fields are ignored.
#pragma once
#include "Dom/JsonObject.h"
namespace Campaign1851Json
{
    inline TSharedPtr<FJsonObject> Row(const TSharedPtr<FJsonValue>& ImportValue)
    {
        const TSharedPtr<FJsonObject>* ImportObject = nullptr;
        return ImportValue.IsValid() && ImportValue->TryGetObject(ImportObject) && ImportObject->IsValid()
            ? *ImportObject : MakeShared<FJsonObject>();
    }
    inline TSharedPtr<FJsonObject> Object(const FJsonObject& ImportObject, const TCHAR* ImportKey)
    {
        const TSharedPtr<FJsonObject>* ImportChild = nullptr;
        return ImportObject.TryGetObjectField(ImportKey, ImportChild) && ImportChild->IsValid()
            ? *ImportChild : MakeShared<FJsonObject>();
    }
    inline const TArray<TSharedPtr<FJsonValue>>& Array(const FJsonObject& ImportObject, const TCHAR* ImportKey)
    {
        const TArray<TSharedPtr<FJsonValue>>* ImportArray = nullptr;
        static const TArray<TSharedPtr<FJsonValue>> ImportEmptyArray;
        return ImportObject.TryGetArrayField(ImportKey, ImportArray) ? *ImportArray : ImportEmptyArray;
    }
    inline FString String(const FJsonObject& ImportObject, const TCHAR* ImportKey)
    {
        FString ImportText;
        ImportObject.TryGetStringField(ImportKey, ImportText);
        return ImportText;
    }
}
