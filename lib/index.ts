// eslint-disable-next-line @typescript-eslint/no-var-requires
const gamelift = require('../build/Release/node-gamelift')

export interface GameLiftError {
  readonly name: string
  readonly message: string
}

export class Outcome<R> {
  readonly result: R | null
  readonly error: GameLiftError | null

  constructor(result: R | null, error: GameLiftError | null) {
    this.result = result
    this.error = error
  }
}

export enum GameSessionStatus {
  Not_Set,
  Active,
  Activating,
  Terminated,
  Terminating,
}

enum AttributeType {
  None = 'NONE',
  String = 'STRING',
  Double = 'DOUBLE',
  String_List = 'STRING_LIST',
  String_Double_Map = 'STRING_DOUBLE_MAP',
}

export class AttributeValue {
  attributeType: AttributeType
  valueAttribute:
    | string
    | number
    | string[]
    | { [key: string]: number }
    | null
    | undefined

  constructor(
    valueAttribute:
      | string
      | number
      | string[]
      | { [key: string]: number }
      | null
      | undefined
  ) {
    if (typeof valueAttribute === 'string') {
      this.attributeType = AttributeType.String
    } else if (typeof valueAttribute === 'number') {
      this.attributeType = AttributeType.Double
    } else if (valueAttribute instanceof Array) {
      this.attributeType = AttributeType.String_List
    } else if (valueAttribute instanceof Object) {
      this.attributeType = AttributeType.String_Double_Map
    } else {
      this.attributeType = AttributeType.None
    }
    this.valueAttribute = valueAttribute
  }
}

export class Player {
  readonly playerId: string = ''
  readonly team: string = ''
  readonly attributes: { [attributeName: string]: AttributeValue } = {}
  readonly latencyInMs: { [region: string]: number } = {}

  constructor(
    playerId: string,
    team: string,
    attributes: { [playerId: string]: AttributeValue } = {},
    latencyInMs: { [region: string]: number } = {}
  ) {
    this.playerId = playerId
    this.team = team
    this.attributes = attributes
    this.latencyInMs = latencyInMs
  }
}

interface Team {
  readonly name: string
  readonly players: Player[]
}

export class MatchmakerData {
  readonly matchId?: string
  readonly matchmakingConfigurationArn?: string
  readonly autoBackfillTicketId?: string
  readonly players?: Player[]

  private constructor(
    matchId: string,
    matchmakingConfigurationArn: string,
    autoBackfillTicketId: string,
    players: Player[]
  ) {
    this.matchId = matchId
    this.matchmakingConfigurationArn = matchmakingConfigurationArn
    this.autoBackfillTicketId = autoBackfillTicketId
    this.players = players
  }

  static fromJSON(jsonString: string): MatchmakerData | null {
    if (jsonString.length == 0) {
      return null
    }

    const json = JSON.parse(jsonString)
    const matchId = json.matchId
    const matchmakingConfigurationArn = json.matchmakingConfigurationArn
    const autoBackfillTicketId = json.autoBackfillTicketId
    const players: Player[] = []
    const teams = json.teams
    teams.forEach((team: Team) => {
      team.players.forEach((player: Player) => {
        players.push(
          new Player(
            player.playerId,
            team.name,
            player.attributes,
            player.latencyInMs
          )
        )
      })
    })
    return new MatchmakerData(
      matchId,
      matchmakingConfigurationArn,
      autoBackfillTicketId,
      players
    )
  }
}

export interface GameSession {
  readonly id: string
  readonly name: string
  readonly fleetId: string
  readonly maximumPlayerSessionCount: number
  readonly status: GameSessionStatus
  readonly ipAddress: string
  readonly port: number
  readonly gameSessionData: string
  readonly matchmakerData: string
}

export enum PlayerSessionStatus {
  Not_Set,
  Reserved,
  Active,
  Completed,
  Timedout,
}

export interface PlayerSession {
  readonly playerSessionId: string
  readonly playerId: string
  readonly gameSessionId: string
  readonly fleetId: string
  readonly creationTime: Date
  readonly terminationTime: Date
  readonly status: PlayerSessionStatus
  readonly ipAddress: string
  readonly port: number
  readonly playerData: string
  readonly dnsName: string
}

export enum UpdateReason {
  Matchmaking_Data_Updated,
  Backfill_Failed,
  Backfill_Timed_Out,
  Backfill_Cancelled,
  Unknown,
}

export interface UpdateGameSession {
  readonly gameSession: GameSession
  readonly updateReason: UpdateReason
}

export type OnStartGameSession = (gameSession: GameSession) => void

export type OnUpdateGameSession = (updateGameSession: UpdateGameSession) => void

export type OnProcessTerminate = () => void

export type OnHealthCheck = () => boolean

export class StartMatchBackfillRequest {
  ticketId?: string | null
  matchmakingConfigurationArn?: string | null
  gameSessionArn?: string | null
  players?: Player[] | null
}

export interface StartMatchBackfillResult {
  ticketId: string
}

export class StopMatchBackfillRequest {
  ticketId?: string | null
  matchmakingConfigurationArn?: string | null
  gameSessionArn?: string | null
}

export class DescribePlayerSessionsRequest {
  gameSessionId?: string | null
  playerId?: string | null
  playerSessionId?: string | null
  playerSessionStatusFilter?: string | null
  limit?: number | null
  nextToken?: string | null
}

export interface DescribePlayerSessionsResult {
  readonly playerSessions: PlayerSession[]
  readonly nextToken: string | null
}

export default {
  initSDK(): Outcome<boolean> {
    return gamelift.initSDK()
  },

  processReady(
    onStartGameSession: OnStartGameSession,
    onUpdateGameSession: OnUpdateGameSession,
    onProcessTerminate: OnProcessTerminate,
    onHealthCheck: OnHealthCheck,
    port: number,
    logPath: string
  ): Outcome<boolean> {
    return gamelift.processReady(
      onStartGameSession,
      onUpdateGameSession,
      onProcessTerminate,
      onHealthCheck,
      port,
      logPath
    )
  },

  processEnding(): Outcome<boolean> {
    return gamelift.processEnding()
  },

  activateGameSession(): Outcome<boolean> {
    return gamelift.activateGameSession()
  },

  terminateGameSession(): Outcome<boolean> {
    return gamelift.terminateGameSession()
  },

  startMatchBackfill(
    request: StartMatchBackfillRequest
  ): Outcome<StartMatchBackfillResult> {
    return gamelift.startMatchBackfill(request)
  },

  stopMatchBackfill(request: StopMatchBackfillRequest): Outcome<boolean> {
    return gamelift.stopMatchBackfill(request)
  },

  acceptPlayerSession(playerSessionId: string): Outcome<boolean> {
    return gamelift.acceptPlayerSession(playerSessionId)
  },

  removePlayerSession(playerSessionId: string): Outcome<boolean> {
    return gamelift.removePlayerSession(playerSessionId)
  },

  describePlayerSessions(
    request: DescribePlayerSessionsRequest
  ): Outcome<DescribePlayerSessionsResult> {
    return gamelift.describePlayerSessions(request)
  },

  destroy(): Outcome<boolean> {
    return gamelift.destroy()
  },
}
