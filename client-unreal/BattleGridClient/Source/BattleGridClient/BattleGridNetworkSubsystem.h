// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleGridNetworkSubsystem.generated.h"

class IWebSocket;
class FJsonObject;

UCLASS()
class BATTLEGRIDCLIENT_API UBattleGridNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void Connect(const FString& InServerUrl, const FString& InNickname);
	void Disconnect();
	void SendPing();
	void SendJoin();
	void SendInput(int32 Seq, float MoveX, float MoveY, float AimX, float AimY, bool bFire);

	bool IsConnected() const;
	bool HasJoined() const;
	int32 GetPlayerId() const;
	int32 GetRoomId() const;
	FString GetServerUrl() const;
	FString GetNickname() const;
	FString GetLastServerMessage() const;
	FString GetLastError() const;
	FString GetConnectionStatusText() const;

private:
	void HandleConnected();
	void HandleConnectionError(const FString& Error);
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleMessage(const FString& Message);
	bool SendJsonObject(const TSharedRef<FJsonObject>& JsonObject, const TCHAR* LogLabel);

private:
	TSharedPtr<IWebSocket> Socket;
	bool bIsConnected = false;
	bool bHasJoined = false;
	FString ServerUrl = TEXT("ws://127.0.0.1:7777");
	FString Nickname = TEXT("player1");
	int32 PlayerId = 0;
	int32 RoomId = 0;
	FString LastServerMessage;
	FString LastError;
};
