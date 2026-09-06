#include "HMT_SessionManager.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

void UHMT_SessionManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind delegates
	CreateSessionDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UHMT_SessionManager::OnCreateSessionComplete);
	JoinSessionDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UHMT_SessionManager::OnJoinSessionComplete);
	FindSessionsDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UHMT_SessionManager::OnFindSessionsComplete);
	DestroySessionDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UHMT_SessionManager::OnDestroySessionComplete);
}

void UHMT_SessionManager::CreateSession(const FString& SessionName, int32 MaxPlayers)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		OnSessionCreated.Broadcast(false, TEXT("No online subsystem"));
		return;
	}

	IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionCreated.Broadcast(false, TEXT("No session interface"));
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = !OnlineSub->IsEnabled();
	SessionSettings.bUsesPresence = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bShouldAdvertise = true;

	CreateSessionHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegate);
	if (!SessionInterface->CreateSession(0, *SessionName, SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		OnSessionCreated.Broadcast(false, TEXT("Failed to start create session"));
	}
}

void UHMT_SessionManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSessionCreated.Broadcast(true, TEXT("Session created"));
	}
	else
	{
		OnSessionCreated.Broadcast(false, TEXT("Failed to create session"));
	}
}

void UHMT_SessionManager::JoinSessionByName(const FString& SessionName)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		OnSessionJoined.Broadcast(false, TEXT("No online subsystem"));
		return;
	}

	IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionJoined.Broadcast(false, TEXT("No session interface"));
		return;
	}

	// Find the session in search results
	if (!SessionSearch.IsValid())
	{
		OnSessionJoined.Broadcast(false, TEXT("No search results. Call FindSessions first."));
		return;
	}

	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		if (Result.Session.OwningUserName == SessionName)
		{
			JoinSessionHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegate);
			if (!SessionInterface->JoinSession(0, NAME_GameSession, Result))
			{
				SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
				OnSessionJoined.Broadcast(false, TEXT("Failed to start join session"));
			}
			return;
		}
	}

	OnSessionJoined.Broadcast(false, TEXT("Session not found in search results"));
}

void UHMT_SessionManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
		}
	}

	OnSessionJoined.Broadcast(Result == EOnJoinSessionCompleteResult::Success,
		Result == EOnJoinSessionCompleteResult::Success ? TEXT("Session joined") : TEXT("Failed to join session"));
}

void UHMT_SessionManager::FindSessions()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		OnSessionsFound.Broadcast(false, TArray<FString>());
		return;
	}

	IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionsFound.Broadcast(false, TArray<FString>());
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = !OnlineSub->IsEnabled();
	SessionSearch->MaxSearchResults = 20;

	FindSessionsHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegate);
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		OnSessionsFound.Broadcast(false, TArray<FString>());
	}
}

void UHMT_SessionManager::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		}
	}

	TArray<FString> SessionNames;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			SessionNames.Add(Result.Session.OwningUserName);
		}
	}

	OnSessionsFound.Broadcast(bWasSuccessful, SessionNames);
}

void UHMT_SessionManager::DestroySession()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		OnSessionDestroyed.Broadcast();
		return;
	}

	IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionDestroyed.Broadcast();
		return;
	}

	DestroySessionHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegate);
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		OnSessionDestroyed.Broadcast();
	}
}

void UHMT_SessionManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		}
	}

	OnSessionDestroyed.Broadcast();
}
