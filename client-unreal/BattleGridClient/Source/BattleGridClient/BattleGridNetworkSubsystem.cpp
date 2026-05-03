// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridNetworkSubsystem.h"

#include "Dom/JsonObject.h"
#include "IWebSocket.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WebSocketsModule.h"

void UBattleGridNetworkSubsystem::Deinitialize()
{
	Disconnect();

	Super::Deinitialize();
}

void UBattleGridNetworkSubsystem::Connect(const FString& InServerUrl, const FString& InNickname)
{
	if (Socket.IsValid())
	{
		Socket->Close();
		Socket.Reset();
	}

	ServerUrl = InServerUrl.IsEmpty() ? FString(TEXT("ws://127.0.0.1:7777")) : InServerUrl;
	Nickname = InNickname.IsEmpty() ? FString(TEXT("anonymous")) : InNickname;
	PlayerId = 0;
	RoomId = 0;
	bIsConnected = false;
	bHasJoined = false;
	LastServerMessage.Empty();
	LastError.Empty();

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Connecting to server: %s"), *ServerUrl);

	Socket = FWebSocketsModule::Get().CreateWebSocket(ServerUrl);
	if (!Socket.IsValid())
	{
		LastError = TEXT("failed to create websocket");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket connection failed: %s"), *LastError);
		return;
	}

	Socket->OnConnected().AddUObject(this, &UBattleGridNetworkSubsystem::HandleConnected);
	Socket->OnConnectionError().AddUObject(this, &UBattleGridNetworkSubsystem::HandleConnectionError);
	Socket->OnClosed().AddUObject(this, &UBattleGridNetworkSubsystem::HandleClosed);
	Socket->OnMessage().AddUObject(this, &UBattleGridNetworkSubsystem::HandleMessage);
	Socket->Connect();
}

void UBattleGridNetworkSubsystem::Disconnect()
{
	if (Socket.IsValid())
	{
		Socket->Close();
		Socket.Reset();
	}

	bIsConnected = false;
	bHasJoined = false;
}

void UBattleGridNetworkSubsystem::SendPing()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("ping"));

	SendJsonObject(JsonObject, TEXT("Ping sent"));
}

void UBattleGridNetworkSubsystem::SendJoin()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("join"));
	JsonObject->SetStringField(TEXT("nickname"), Nickname);

	SendJsonObject(JsonObject, TEXT("Join sent"));
}

void UBattleGridNetworkSubsystem::SendInput(
	int32 Seq,
	float MoveX,
	float MoveY,
	float AimX,
	float AimY,
	bool bFire
)
{
	if (!Socket.IsValid() || !Socket->IsConnected() || !bHasJoined)
	{
		return;
	}

	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("input"));
	JsonObject->SetNumberField(TEXT("seq"), Seq);
	JsonObject->SetNumberField(TEXT("player_id"), PlayerId);
	JsonObject->SetNumberField(TEXT("move_x"), MoveX);
	JsonObject->SetNumberField(TEXT("move_y"), MoveY);
	JsonObject->SetNumberField(TEXT("aim_x"), AimX);
	JsonObject->SetNumberField(TEXT("aim_y"), AimY);
	JsonObject->SetBoolField(TEXT("fire"), bFire);

	if (SendJsonObject(JsonObject, nullptr))
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Sent input seq=%d"), Seq);
	}
}

bool UBattleGridNetworkSubsystem::IsConnected() const
{
	return bIsConnected;
}

bool UBattleGridNetworkSubsystem::HasJoined() const
{
	return bHasJoined;
}

int32 UBattleGridNetworkSubsystem::GetPlayerId() const
{
	return PlayerId;
}

int32 UBattleGridNetworkSubsystem::GetRoomId() const
{
	return RoomId;
}

FString UBattleGridNetworkSubsystem::GetServerUrl() const
{
	return ServerUrl;
}

FString UBattleGridNetworkSubsystem::GetNickname() const
{
	return Nickname;
}

FString UBattleGridNetworkSubsystem::GetLastServerMessage() const
{
	return LastServerMessage;
}

FString UBattleGridNetworkSubsystem::GetLastError() const
{
	return LastError;
}

FString UBattleGridNetworkSubsystem::GetConnectionStatusText() const
{
	if (bHasJoined)
	{
		return FString::Printf(TEXT("Server: Connected player=%d room=%d"), PlayerId, RoomId);
	}

	if (bIsConnected)
	{
		return TEXT("Server: Connected");
	}

	if (!LastError.IsEmpty())
	{
		return FString::Printf(TEXT("Server: Disconnected (%s)"), *LastError);
	}

	return TEXT("Server: Disconnected");
}

void UBattleGridNetworkSubsystem::HandleConnected()
{
	bIsConnected = true;
	bHasJoined = false;
	LastError.Empty();

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] WebSocket connected."));

	SendPing();
	SendJoin();
}

void UBattleGridNetworkSubsystem::HandleConnectionError(const FString& Error)
{
	bIsConnected = false;
	bHasJoined = false;
	LastError = Error.IsEmpty() ? FString(TEXT("connection failed")) : Error;

	UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket connection failed: %s"), *LastError);
}

void UBattleGridNetworkSubsystem::HandleClosed(
	int32 StatusCode,
	const FString& Reason,
	bool bWasClean
)
{
	bIsConnected = false;
	bHasJoined = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] WebSocket closed. Status=%d Clean=%s Reason=%s"),
		StatusCode,
		bWasClean ? TEXT("true") : TEXT("false"),
		*Reason
	);
}

void UBattleGridNetworkSubsystem::HandleMessage(const FString& Message)
{
	LastServerMessage = Message;

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] WebSocket received raw message: %s"), *Message);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		LastError = TEXT("invalid json from server");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Failed to parse server JSON."));
		return;
	}

	FString Type;
	if (!JsonObject->TryGetStringField(TEXT("type"), Type))
	{
		LastError = TEXT("server message missing type");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Server message missing type."));
		return;
	}

	if (Type == TEXT("pong"))
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Pong received."));
		return;
	}

	if (Type == TEXT("join_ok"))
	{
		double PlayerIdValue = 0.0;
		double RoomIdValue = 0.0;
		FString ResponseNickname;

		if (JsonObject->TryGetNumberField(TEXT("player_id"), PlayerIdValue))
		{
			PlayerId = static_cast<int32>(PlayerIdValue);
		}

		if (JsonObject->TryGetNumberField(TEXT("room_id"), RoomIdValue))
		{
			RoomId = static_cast<int32>(RoomIdValue);
		}

		if (JsonObject->TryGetStringField(TEXT("nickname"), ResponseNickname) && !ResponseNickname.IsEmpty())
		{
			Nickname = ResponseNickname;
		}

		bHasJoined = true;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] join_ok parsed. player_id=%d room_id=%d nickname=%s"),
			PlayerId,
			RoomId,
			*Nickname
		);
		return;
	}

	if (Type == TEXT("error"))
	{
		FString ErrorMessage;
		JsonObject->TryGetStringField(TEXT("message"), ErrorMessage);
		LastError = ErrorMessage.IsEmpty() ? FString(TEXT("server error")) : ErrorMessage;
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Server error response parsed: %s"), *LastError);
		return;
	}

	if (Type == TEXT("input_ack"))
	{
		double SequenceValue = 0.0;
		JsonObject->TryGetNumberField(TEXT("seq"), SequenceValue);
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Input ack received. seq=%d"), static_cast<int32>(SequenceValue));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Unknown server message type: %s"), *Type);
}

bool UBattleGridNetworkSubsystem::SendJsonObject(
	const TSharedRef<FJsonObject>& JsonObject,
	const TCHAR* LogLabel
)
{
	if (!Socket.IsValid() || !Socket->IsConnected())
	{
		LastError = TEXT("not connected");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket send failed: %s"), *LastError);
		return false;
	}

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (!FJsonSerializer::Serialize(JsonObject, Writer))
	{
		LastError = TEXT("failed to serialize json");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket send failed: %s"), *LastError);
		return false;
	}

	Socket->Send(Payload);
	if (LogLabel)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] %s: %s"), LogLabel, *Payload);
	}

	return true;
}
