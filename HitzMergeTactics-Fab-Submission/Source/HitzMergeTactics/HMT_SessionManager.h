#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "HMT_SessionManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnSessionCreated, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnSessionJoined, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnSessionsFound, bool, bSuccess, const TArray<FString>&, SessionNames);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHMT_OnSessionDestroyed);

/**
 * Session management for main menu — hosts, joins, and discovers multiplayer sessions.
 * Uses Unreal's default IOnlineSessionInterface. Buyers can override with custom subsystems.
 */
UCLASS()
class HITZMERGETACTICS_API UHMT_SessionManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Creates a new session with the given name (host-side). */
	UFUNCTION(BlueprintCallable, Category = "HMT Sessions")
	void CreateSession(const FString& SessionName, int32 MaxPlayers = 8);

	/** Joins an existing session by name. Must call FindSessions first to populate the search results. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sessions")
	void JoinSessionByName(const FString& SessionName);

	/** Queries available sessions. Results posted to OnSessionsFound. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sessions")
	void FindSessions();

	/** Destroys the current session. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sessions")
	void DestroySession();

	UPROPERTY(BlueprintAssignable, Category = "HMT Sessions")
	FHMT_OnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "HMT Sessions")
	FHMT_OnSessionJoined OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "HMT Sessions")
	FHMT_OnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "HMT Sessions")
	FHMT_OnSessionDestroyed OnSessionDestroyed;

private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	FOnCreateSessionCompleteDelegate CreateSessionDelegate;
	FDelegateHandle CreateSessionHandle;

	FOnJoinSessionCompleteDelegate JoinSessionDelegate;
	FDelegateHandle JoinSessionHandle;

	FOnFindSessionsCompleteDelegate FindSessionsDelegate;
	FDelegateHandle FindSessionsHandle;

	FOnDestroySessionCompleteDelegate DestroySessionDelegate;
	FDelegateHandle DestroySessionHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
};
