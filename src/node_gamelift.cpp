#include <napi.h>
#include "napi-thread-safe-callback.hpp"

#define GAMELIFT_USE_STD 1
#include <aws/gamelift/server/GameLiftServerAPI.h>

template <typename E>
static Napi::Object ObjectFromError(Napi::Env env, E error)
{
    auto obj = Napi::Object::New(env);
    obj["name"] = Napi::String::New(env, error.GetErrorName());
    obj["message"] = Napi::String::New(env, error.GetErrorMessage());
    return obj;
}

template <typename R, typename E>
static Napi::Object ObjectFromOutcome(Napi::Env env, Aws::GameLift::Outcome<R, E> outcome)
{
    auto obj = Napi::Object::New(env);
    obj["result"] = Napi::Boolean::New(env, outcome.IsSuccess());
    obj["error"] = outcome.IsSuccess() ? env.Null() : ObjectFromError(env, outcome.GetError());
    return obj;
}

static Napi::Object ObjectFromGameSession(Napi::Env env, Aws::GameLift::Server::Model::GameSession gameSession)
{
    auto obj = Napi::Object::New(env);
    obj["id"] = Napi::String::New(env, gameSession.GetGameSessionId());
    obj["name"] = Napi::String::New(env, gameSession.GetName());
    obj["fleetId"] = Napi::String::New(env, gameSession.GetFleetId());
    obj["maximumPlayerSessionCount"] = Napi::Number::New(env, gameSession.GetMaximumPlayerSessionCount());
    obj["status"] = Napi::Number::New(env, (double)gameSession.GetStatus());
    obj["ipAddress"] = Napi::String::New(env, gameSession.GetIpAddress());
    obj["port"] = Napi::Number::New(env, (double)gameSession.GetPort());
    obj["gameSessionData"] = Napi::String::New(env, gameSession.GetGameSessionData());
    obj["matchmakerData"] = Napi::String::New(env, gameSession.GetMatchmakerData());
    return obj;
}

static Napi::Array ArrayFromPlayerSessions(Napi::Env env, std::vector<Aws::GameLift::Server::Model::PlayerSession> playerSessions)
{
    auto arr = Napi::Array::New(env, playerSessions.size());
    for (size_t i = 0; i < playerSessions.size(); ++i)
    {
        auto playerSession = playerSessions[i];
        auto obj = Napi::Object::New(env);
        obj["playerSessionId"] = Napi::String::New(env, playerSession.GetPlayerSessionId());
        obj["playerId"] = Napi::String::New(env, playerSession.GetPlayerId());
        obj["gameSessionId"] = Napi::String::New(env, playerSession.GetGameSessionId());
        obj["fleetId"] = Napi::String::New(env, playerSession.GetFleetId());
        obj["creationTime"] = Napi::Date::New(env, playerSession.GetCreationTime());
        obj["terminationTime"] = Napi::Date::New(env, playerSession.GetTerminationTime());
        obj["status"] = Napi::Number::New(env, (double)playerSession.GetStatus());
        obj["ipAddress"] = Napi::String::New(env, playerSession.GetIpAddress());
        obj["port"] = Napi::Number::New(env, playerSession.GetPort());
        obj["playerData"] = Napi::String::New(env, playerSession.GetPlayerData());
        obj["dnsName"] = Napi::String::New(env, playerSession.GetDnsName());
        arr[i] = obj;
    }
    return arr;
}

static Aws::GameLift::Server::Model::AttributeValue StringListFromArray(Napi::Array stringListArray)
{
    auto stringList = Aws::GameLift::Server::Model::AttributeValue::ConstructStringList();
    for (uint32_t i = 0; i < stringListArray.Length(); ++i)
    {
        auto string = static_cast<Napi::Value>(stringListArray[i]).As<Napi::String>().Utf8Value();
        stringList.AddString(string);
    }
    return stringList;
}

static Aws::GameLift::Server::Model::AttributeValue StringDoubleMapFromObject(Napi::Object stringDoubleMapObj)
{
    auto stringDoubleMap = Aws::GameLift::Server::Model::AttributeValue::ConstructStringDoubleMap();
    auto names = stringDoubleMapObj.GetPropertyNames();
    for (uint32_t i = 0; i < names.Length(); ++i)
    {
        auto key = static_cast<Napi::Value>(names[i]).As<Napi::String>().Utf8Value();
        auto value = stringDoubleMapObj.Get(key).As<Napi::Number>().DoubleValue();
        stringDoubleMap.AddStringAndDouble(key, value);
    }
    return stringDoubleMap;
}

static std::map<std::string, Aws::GameLift::Server::Model::AttributeValue> PlayerAttributesFromObject(Napi::Object playerAttributesObj)
{
    std::map<std::string, Aws::GameLift::Server::Model::AttributeValue> playerAttributes;
    auto playerAttributesNames = playerAttributesObj.GetPropertyNames();
    for (uint32_t i = 0; i < playerAttributesNames.Length(); ++i)
    {
        auto name = static_cast<Napi::Value>(playerAttributesNames[i]).As<Napi::String>().Utf8Value();
        auto playerAttributeObj = playerAttributesObj.Get(name).As<Napi::Object>();
        auto type = playerAttributeObj.Get("attributeType").As<Napi::String>().Utf8Value();
        if (type == "NONE")
        {
            playerAttributes[name] = Aws::GameLift::Server::Model::AttributeValue();
        }
        else if (type == "STRING")
        {
            auto value = playerAttributeObj.Get("valueAttribute").As<Napi::String>().Utf8Value();
            playerAttributes[name] = Aws::GameLift::Server::Model::AttributeValue(value);
        }
        else if (type == "DOUBLE")
        {
            auto value = playerAttributeObj.Get("valueAttribute").As<Napi::Number>().DoubleValue();
            playerAttributes[name] = Aws::GameLift::Server::Model::AttributeValue(value);
        }
        else if (type == "STRING_LIST")
        {
            auto value = StringListFromArray(playerAttributeObj.Get("valueAttribute").As<Napi::Array>());
            playerAttributes[name] = value;
        }
        else if (type == "STRING_DOUBLE_MAP")
        {
            auto value = StringDoubleMapFromObject(playerAttributeObj.Get("valueAttribute").As<Napi::Object>());
            playerAttributes[name] = value;
        }
    }
    return playerAttributes;
}

static std::map<std::string, int> LatencyInMsFromObject(Napi::Object latencyInMsObj)
{
    std::map<std::string, int> latencyInMs;
    auto latencyInMsObjNames = latencyInMsObj.GetPropertyNames();
    for (uint32_t i = 0; i < latencyInMsObjNames.Length(); ++i)
    {
        auto name = static_cast<Napi::Value>(latencyInMsObjNames[i]).As<Napi::String>().Utf8Value();
        latencyInMs[name] = latencyInMsObj.Get(name).As<Napi::Number>().Int32Value();
    }
    return latencyInMs;
}

static std::vector<Aws::GameLift::Server::Model::Player> PlayersFromArray(Napi::Array playersArray)
{
    std::vector<Aws::GameLift::Server::Model::Player> players;
    for (uint32_t i = 0; i < playersArray.Length(); ++i)
    {
        auto playerObj = static_cast<Napi::Value>(playersArray[i]).As<Napi::Object>();

        Aws::GameLift::Server::Model::Player player;
        player.SetPlayerId(playerObj.Get("playerId").As<Napi::String>().Utf8Value());
        player.SetTeam(playerObj.Get("team").As<Napi::String>().Utf8Value());

        auto playerAttributes = PlayerAttributesFromObject(playerObj.Get("attributes").As<Napi::Object>());
        player.SetPlayerAttributes(playerAttributes);

        auto latencyInMs = LatencyInMsFromObject(playerObj.Get("latencyInMs").As<Napi::Object>());
        player.SetLatencyInMs(latencyInMs);

        players.push_back(player);
    }
    return players;
}

static Napi::Object ObjectFromStartMatchBackfillOutcome(Napi::Env env, Aws::GameLift::StartMatchBackfillOutcome outcome)
{
    auto obj = Napi::Object::New(env);
    if (outcome.IsSuccess())
    {
        auto result = outcome.GetResult();
        auto resultObj = Napi::Object::New(env);
        resultObj["ticketId"] = Napi::String::New(env, result.GetTicketId());
        obj["result"] = resultObj;
        obj["error"] = env.Null();
    }
    else
    {
        obj["result"] = env.Null();
        obj["error"] = ObjectFromError(env, outcome.GetError());
    }
    return obj;
}

static Napi::Object ObjectFromDescribePlayerSessionsOutcome(Napi::Env env,
                                                            Aws::GameLift::DescribePlayerSessionsOutcome outcome)
{
    auto obj = Napi::Object::New(env);
    if (outcome.IsSuccess())
    {
        auto result = outcome.GetResult();
        auto resultObj = Napi::Object::New(env);
        resultObj["playerSessions"] = ArrayFromPlayerSessions(env, result.GetPlayerSessions());
        resultObj["nextToken"] = Napi::String::New(env, result.GetNextToken());
        obj["result"] = resultObj;
        obj["error"] = env.Null();
    }
    else
    {
        obj["result"] = env.Null();
        obj["error"] = ObjectFromError(env, outcome.GetError());
    }
    return obj;
}

Napi::Value InitSDK(const Napi::CallbackInfo &info)
{
    auto outcome = Aws::GameLift::Server::InitSDK();
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value ProcessReady(const Napi::CallbackInfo &info)
{
    auto onStartGameSessionJS = std::make_shared<ThreadSafeCallback>(info[0].As<Napi::Function>());
    auto onStartGameSession = [onStartGameSessionJS](Aws::GameLift::Server::Model::GameSession gameSession) {
        auto cb = onStartGameSessionJS;
        cb->call([gameSession](Napi::Env env, std::vector<napi_value> &args) {
            args = {ObjectFromGameSession(env, gameSession)};
        });
    };

    auto onUpdateGameSessionJS = std::make_shared<ThreadSafeCallback>(info[1].As<Napi::Function>());
    auto onUpdateGameSession = [onUpdateGameSessionJS](Aws::GameLift::Server::Model::UpdateGameSession updateGameSession) {
        auto cb = onUpdateGameSessionJS;
        cb->call([updateGameSession](Napi::Env env, std::vector<napi_value> &args) {
            auto obj = Napi::Object::New(env);
            obj["gameSession"] = ObjectFromGameSession(env, updateGameSession.GetGameSession());
            obj["updateReason"] = Napi::Number::New(env, (double)updateGameSession.GetUpdateReason());
            args = {obj};
        });
    };

    auto onProcessTerminateJS = std::make_shared<ThreadSafeCallback>(info[2].As<Napi::Function>());
    auto onProcessTerminate = [onProcessTerminateJS]() {
        auto cb = onProcessTerminateJS;
        cb->call([](Napi::Env env, std::vector<napi_value> &args) {});
    };

    auto onHealthCheckJS = std::make_shared<ThreadSafeCallback>(info[3].As<Napi::Function>());
    auto onHealthCheck = [onHealthCheckJS]() {
        auto cb = onHealthCheckJS;
        auto future = cb->call<bool>([](Napi::Env env, std::vector<napi_value> &args) {},
                                     [](const Napi::Value &val) { return val.As<Napi::Boolean>().Value(); });
        return future.get();
    };

    int port = info[4].As<Napi::Number>();

    std::vector<std::string> logPaths;
    logPaths.push_back(info[5].As<Napi::String>().Utf8Value());

    Aws::GameLift::Server::ProcessParameters processReadyParameters(onStartGameSession,
                                                                    onUpdateGameSession,
                                                                    onProcessTerminate,
                                                                    onHealthCheck,
                                                                    port,
                                                                    Aws::GameLift::Server::LogParameters(logPaths));

    auto outcome = Aws::GameLift::Server::ProcessReady(processReadyParameters);
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value ProcessEnding(const Napi::CallbackInfo &info)
{
    auto outcome = Aws::GameLift::Server::ProcessEnding();
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value ActivateGameSession(const Napi::CallbackInfo &info)
{
    auto outcome = Aws::GameLift::Server::ActivateGameSession();
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value TerminateGameSession(const Napi::CallbackInfo &info)
{
    auto outcome = Aws::GameLift::Server::TerminateGameSession();
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value StartMatchBackfill(const Napi::CallbackInfo &info)
{
    auto obj = info[0].As<Napi::Object>();
    Aws::GameLift::Server::Model::StartMatchBackfillRequest request;
    auto ticketId = obj.Get("ticketId");
    if (!ticketId.IsNull() && !ticketId.IsUndefined())
    {
        request.SetTicketId(ticketId.As<Napi::String>().Utf8Value());
    }
    auto matchmakingConfigurationArn = obj.Get("matchmakingConfigurationArn");
    if (!matchmakingConfigurationArn.IsNull() && !matchmakingConfigurationArn.IsUndefined())
    {
        request.SetMatchmakingConfigurationArn(matchmakingConfigurationArn.As<Napi::String>().Utf8Value());
    }
    auto gameSessionArn = obj.Get("gameSessionArn");
    if (!gameSessionArn.IsNull() && !gameSessionArn.IsUndefined())
    {
        request.SetGameSessionArn(gameSessionArn.As<Napi::String>().Utf8Value());
    }
    auto players = obj.Get("players");
    if (!players.IsNull() && !players.IsUndefined())
    {
        request.SetPlayers(PlayersFromArray(players.As<Napi::Array>()));
    }
    auto outcome = Aws::GameLift::Server::StartMatchBackfill(request);
    return ObjectFromStartMatchBackfillOutcome(info.Env(), outcome);
}

Napi::Value StopMatchBackfill(const Napi::CallbackInfo &info)
{
    auto obj = info[0].As<Napi::Object>();
    Aws::GameLift::Server::Model::StopMatchBackfillRequest request;
    auto ticketId = obj.Get("ticketId");
    if (!ticketId.IsNull() && !ticketId.IsUndefined())
    {
        request.SetTicketId(ticketId.As<Napi::String>().Utf8Value());
    }
    auto matchmakingConfigurationArn = obj.Get("matchmakingConfigurationArn");
    if (!matchmakingConfigurationArn.IsNull() && !matchmakingConfigurationArn.IsUndefined())
    {
        request.SetMatchmakingConfigurationArn(matchmakingConfigurationArn.As<Napi::String>().Utf8Value());
    }
    auto gameSessionArn = obj.Get("gameSessionArn");
    if (!gameSessionArn.IsNull() && !gameSessionArn.IsUndefined())
    {
        request.SetGameSessionArn(gameSessionArn.As<Napi::String>().Utf8Value());
    }
    auto outcome = Aws::GameLift::Server::StopMatchBackfill(request);
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value AcceptPlayerSession(const Napi::CallbackInfo &info)
{
    auto playerSessionId = info[0].As<Napi::String>().Utf8Value();
    auto outcome = Aws::GameLift::Server::AcceptPlayerSession(playerSessionId);
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value RemovePlayerSession(const Napi::CallbackInfo &info)
{
    auto playerSessionId = info[0].As<Napi::String>().Utf8Value();
    auto outcome = Aws::GameLift::Server::RemovePlayerSession(playerSessionId);
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Value DescribePlayerSessions(const Napi::CallbackInfo &info)
{
    auto obj = info[0].As<Napi::Object>();
    Aws::GameLift::Server::Model::DescribePlayerSessionsRequest request;
    auto gameSessionId = obj.Get("gameSessionId");
    if (!gameSessionId.IsNull() && !gameSessionId.IsUndefined())
    {
        request.SetGameSessionId(gameSessionId.As<Napi::String>().Utf8Value());
    }
    auto playerId = obj.Get("playerId");
    if (!playerId.IsNull() && !playerId.IsUndefined())
    {
        request.SetPlayerId(playerId.As<Napi::String>().Utf8Value());
    }
    auto playerSessionId = obj.Get("playerSessionId");
    if (!playerSessionId.IsNull() && !playerSessionId.IsUndefined())
    {
        request.SetPlayerSessionId(playerSessionId.As<Napi::String>().Utf8Value());
    }
    auto playerSessionStatusFilter = obj.Get("playerSessionStatusFilter");
    if (!playerSessionStatusFilter.IsNull() && !playerSessionStatusFilter.IsUndefined())
    {
        request.SetPlayerSessionStatusFilter(playerSessionStatusFilter.As<Napi::String>().Utf8Value());
    }
    auto limit = obj.Get("limit");
    if (!limit.IsNull() && !limit.IsUndefined())
    {
        request.SetLimit(limit.As<Napi::Number>());
    }
    auto nextToken = obj.Get("nextToken");
    if (!nextToken.IsNull() && !nextToken.IsUndefined())
    {
        request.SetNextToken(nextToken.As<Napi::String>().Utf8Value());
    }
    auto outcome = Aws::GameLift::Server::DescribePlayerSessions(request);
    return ObjectFromDescribePlayerSessionsOutcome(info.Env(), outcome);
}

Napi::Value Destroy(const Napi::CallbackInfo &info)
{
    auto outcome = Aws::GameLift::Server::Destroy();
    return ObjectFromOutcome(info.Env(), outcome);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports["initSDK"] = Napi::Function::New(env, InitSDK);
    exports["processReady"] = Napi::Function::New(env, ProcessReady);
    exports["processEnding"] = Napi::Function::New(env, ProcessEnding);
    exports["activateGameSession"] = Napi::Function::New(env, ActivateGameSession);
    exports["terminateGameSession"] = Napi::Function::New(env, TerminateGameSession);
    exports["startMatchBackfill"] = Napi::Function::New(env, StartMatchBackfill);
    exports["stopMatchBackfill"] = Napi::Function::New(env, StopMatchBackfill);
    exports["acceptPlayerSession"] = Napi::Function::New(env, AcceptPlayerSession);
    exports["removePlayerSession"] = Napi::Function::New(env, RemovePlayerSession);
    exports["describePlayerSessions"] = Napi::Function::New(env, DescribePlayerSessions);
    exports["destroy"] = Napi::Function::New(env, Destroy);
    return exports;
}

NODE_API_MODULE(NodeGameLift, Init)