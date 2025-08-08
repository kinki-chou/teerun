#include <engine/server.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <ctime>
#include <random>
#include <string>

#include "TeeRun.h"

// Exchange this to a string that identifies your game mode.
// DM, TDM and CTF are reserved for teeworlds original modes.
// DDraceNetwork and TestDDraceNetwork are used by DDNet.
#define GAME_TYPE_NAME "TeeRun"
#define TEST_TYPE_NAME "TestTR"

CGameControllerTeeRun::CGameControllerTeeRun(class CGameContext *pGameServer) :
	CGameControllerVanilla(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;

	m_DefaultWeapon = WEAPON_HAMMER;

	// m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

CGameControllerTeeRun::~CGameControllerTeeRun() = default;

/*
    内容：
	游戏开始时，重置所有上局内容。
	确定目标顺序，开启一轮追逐：
	    每次追逐，按顺序依次确定追的目标并公布
	    其余人要在规定时间内抓捕目标
	    若时间到（目标逃脱）或目标被捕，则转到下一目标。
	    重复上述2操作。
	在规定轮数后，若场上还有不止1人，则开启死斗模式，直至只剩1人及以下。
	（注：任何人一旦死亡，本场游戏不得重新加入。）
*/

void CGameControllerTeeRun::OnRoundStart()
{
	CGameControllerVanilla::OnRoundStart();

	// char aBuf[255];
	// int m_AlivePlayers = 0;

	// for(int i = 0; i < MAX_CLIENTS; i++)
	// {
	// 	CPlayer *pPlayer = GameServer()->m_apPlayers[i];
	// 	if(!pPlayer)
	// 		continue;
	// 	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	// 		m_AlivePlayers++;

	// 	// if(pPlayer->m_WantsToJoinSpectators)
	// 	// 	pPlayer->m_DeadSpec = -1;
	// 	// if(pPlayer->m_WantsToJoinGame || pPlayer->m_DeadSpec == 1)
	// 	// 	pPlayer->m_DeadSpec = 0;
	// 	// m_vTargetList.push_back(i);
	// }

	// std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	// std::shuffle(m_vTargetList.begin(), m_vTargetList.end(), rng);

	// std::string str = "m_vTargetList: ";
	// for(int i = 0; i < m_vTargetList.size(); i++)
	// {
	// 	str.append(m_vTargetList[i] + "0");
	// 	str.append(",");
	// }
	// str_format(aBuf, sizeof(aBuf), str.c_str());
	// dbg_msg("TeeRun", aBuf);
}

void CGameControllerTeeRun::OnRoundEnd()
{
	m_IsRoundEnding = true;
	m_RoundEndTick = Server()->Tick();
	EndRound();
	CGameControllerVanilla::OnRoundEnd();
	m_IsRoundStart = false;
	m_IsSolomode = false;
	m_IsTargetmode = false;
	m_IsPvpmode = false;
	m_vTargetList.clear();
	m_CurrentTarget = -1;
	m_RoundPassed = 0;
	m_BroadcastPvp = false;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer)
			continue;
		pPlayer->m_DeadSpec = 0;
		pPlayer->m_IsDead = 0;
		pPlayer->SetTeamNoKill(TEAM_RED);
	}
}

void CGameControllerTeeRun::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerVanilla::OnPlayerConnect(pPlayer);

	if(m_IsRoundStart || m_IsRoundEnding)
	{
		pPlayer->SetTeam(TEAM_SPECTATORS);
		pPlayer->m_IsDead = 1;
		pPlayer->m_DeadSpec = 1;
		GameServer()->SendBroadcast("Please wait until this round is over", pPlayer->GetCid());
		return;
	}
}

void CGameControllerTeeRun::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerVanilla::OnCharacterSpawn(pChr);

	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

int CGameControllerTeeRun::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	CGameControllerVanilla::OnCharacterDeath(pVictim, pKiller, Weapon);

	if(!pVictim)
		return 0;

	// pVictim->GetPlayer()->SetTeamNoKill(TEAM_SPECTATORS);
	pVictim->GetPlayer()->m_IsDead = 1;
	pVictim->GetPlayer()->m_DeadSpec = 1;
	// GameServer()->SendBroadcast("You are dead. Please wait until this round is over.", pVictim->GetPlayer()->GetCid());

	return 0;
}

bool CGameControllerTeeRun::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
	if(pPlayer && Team == TEAM_RED)
		return true; // bro waht
	if(pPlayer && pPlayer->m_IsDead && Team != TEAM_SPECTATORS && m_IsRoundStart)
	{
		str_format(pErrorReason, ErrorReasonSize, "Wait until this round is over");
		return false;
	}
	if(pPlayer && Team == TEAM_SPECTATORS)
		return true;
	CGameControllerVanilla::CanJoinTeam(Team, NotThisId, pErrorReason, ErrorReasonSize);
	return false;
}

void CGameControllerTeeRun::StartTeeRun()
{
	char aBuf[255];

	dbg_msg("TeeRun", "oonp, TeeRun Start!");
	GameServer()->SendChat(-1, TEAM_ALL, "oonp, TeeRun Start!");
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer)
			continue;
		pPlayer->m_DeadSpec = 0;
		pPlayer->m_IsDead = 0;
		pPlayer->SetTeamNoKill(TEAM_RED);
		m_vTargetList.push_back(i); // for Target mode
	}

	// Solo mode
	m_IsSolomode = true;
	m_SoloTick = Server()->Tick(); // 50tick/s
	// str_format(aBuf, sizeof(aBuf), "%ds hiding time, running away from the crowd might be the better choice ..?", g_Config.m_SvTeeRunSolomodeTime);
	str_format(aBuf, sizeof(aBuf), "%d 秒躲藏时间, 逃离人群也许会更好...吧?", g_Config.m_SvTeeRunSolomodeTime);
	GameServer()->SendBroadcast(aBuf, -1);

	// set Target
	std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::shuffle(m_vTargetList.begin(), m_vTargetList.end(), rng);

	std::string str = "m_vTargetList: ";
	m_vSize = m_vTargetList.size();
	for(int i = 0; i < m_vSize; i++)
	{
		str.append(std::to_string(m_vTargetList[i]));
		str.push_back(", "[i == m_vSize - 1]);
	}
	str_format(aBuf, sizeof(aBuf), str.c_str());
	dbg_msg("TeeRun", aBuf);
}

bool CGameControllerTeeRun::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	if(From == pCharacter->GetPlayer()->GetCid())
		return !g_Config.m_SvTeeRunSelfDamage;
	if(m_IsSolomode)
		return true;
	if(m_IsTargetmode && pCharacter->GetPlayer()->GetCid() != m_CurrentTarget)
		return true;
	if(m_IsPvpmode)
		return false;

	CGameControllerVanilla::SkipDamage(Dmg, From, Weapon, pCharacter, ApplyForce);
	return false;
}

void CGameControllerTeeRun::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	CGameControllerVanilla::Tick();

	char aBuf[255];

	int m_AlivePlayers = 0;
	std::string m_LastAlivePlayer;
	std::string temp;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer)
			continue;

		if(!pPlayer->m_IsDead)
		{
			if(Server()->ClientName(i) == "(connecting)" || Server()->ClientName(i) == "(invalid)")
				continue;
			m_AlivePlayers++;
			m_LastAlivePlayer = Server()->ClientName(i);
		}

		// a piece of shit
		if(m_IsRoundStart)
		{
			if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				pPlayer->m_IsDead = 1;
			}
			if(pPlayer->m_IsDead && pPlayer->GetTeam() != TEAM_SPECTATORS)
			{
				pPlayer->SetTeamNoKill(TEAM_SPECTATORS);
			}
		}
		else
		{
			if(pPlayer->GetTeam() == TEAM_RED)
			{
				pPlayer->m_IsDead = 0;
			}
			if(!pPlayer->m_IsDead && pPlayer->GetTeam() != TEAM_RED)
			{
				pPlayer->SetTeamNoKill(TEAM_RED);
			}
			// pPlayer->m_IsDead = 0;
			// pPlayer->SetTeamNoKill(TEAM_RED);
		}
	}

	// str_format(aBuf, sizeof(aBuf), "Currently there are %d players! ", m_AlivePlayers);
	// dbg_msg("TeeRun", aBuf);

	// str_format(aBuf, sizeof(aBuf), "Round start: %d", m_IsRoundStart);
	// dbg_msg("TeeRun", aBuf);

	// if the game can start ...
	if(m_AlivePlayers >= 2)
	{
		if(m_IsRoundStart)
		{
			// Solo mode
			if(m_IsSolomode)
			{
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[i];
					if(!pPlayer)
						continue;
					pPlayer->m_Score = g_Config.m_SvTeeRunSolomodeTime - (Server()->Tick() - m_SoloTick) / SERVER_TICK_SPEED;
				}

				// solo mode is over
				if(Server()->Tick() - m_SoloTick > SERVER_TICK_SPEED * g_Config.m_SvTeeRunSolomodeTime)
				{
					m_IsSolomode = false;
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						CPlayer *pPlayer = GameServer()->m_apPlayers[i];
						if(!pPlayer)
							continue;
						pPlayer->m_Score = 0;
					}
					m_IsTargetmode = true;
				}
			}
			// Target mode
			if(m_IsTargetmode)
			{
				// if the target hasn't generated
				if(m_CurrentTarget == -1)
				{
					m_TargetPointer = 0;
					m_CurrentTarget = m_vTargetList[m_TargetPointer];
				}

				// TODO: delete this and find a better way to clear scores
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[i];
					if(!pPlayer)
						continue;
					if(pPlayer->GetCid() != m_CurrentTarget && pPlayer->m_Score)
						pPlayer->m_Score = 0;
				}

				CPlayer *pTarget = GameServer()->m_apPlayers[m_CurrentTarget];
				if(!m_BroadcastTarget)
				{
					// str_format(aBuf, sizeof(aBuf), "The next target is ... '%s'!\n You have %ds to catch that guy.", Server()->ClientName(m_CurrentTarget), g_Config.m_SvTeeRunTargetEscapeTime);
					str_format(aBuf, sizeof(aBuf), "下一个目标是 ... '%s'!\n你们有 %d 秒的时间抓捕他.", Server()->ClientName(m_CurrentTarget), g_Config.m_SvTeeRunTargetEscapeTime);
					GameServer()->SendBroadcast(aBuf, -1);
					GameServer()->SendChat(-1, TEAM_ALL, aBuf);
					str_format(aBuf, sizeof(aBuf), "Next target: cid=%d", m_CurrentTarget);
					dbg_msg("TeeRun", aBuf);

					m_TargetTick = Server()->Tick();
					m_BroadcastTarget = true;
				}
				if(pTarget)
				{
					pTarget->m_Score = g_Config.m_SvTeeRunTargetEscapeTime - (Server()->Tick() - m_TargetTick) / SERVER_TICK_SPEED;
				}

				// if target escaped
				if(Server()->Tick() - m_TargetTick > SERVER_TICK_SPEED * g_Config.m_SvTeeRunTargetEscapeTime)
				{
					// GameServer()->SendChat(-1, TEAM_ALL, "Time's up! The target has escaped...");
					GameServer()->SendChat(-1, TEAM_ALL, "时间到! 目标已逃脱...");
					pTarget->m_Score = 0;
					do
					{
						if(!((m_TargetPointer + 1) % m_vSize))
							m_RoundPassed++;
						m_TargetPointer = (m_TargetPointer + 1) % m_vSize;
						m_CurrentTarget = m_vTargetList[m_TargetPointer];
					} while(GameServer()->m_apPlayers[m_CurrentTarget]->m_IsDead);
					str_format(aBuf, sizeof(aBuf), "Round passed: %d", m_RoundPassed);
					dbg_msg("TeeRun", aBuf);
					m_BroadcastTarget = false;
				}
				// if target died
				if(pTarget->m_IsDead)
				{
					// GameServer()->SendChat(-1, TEAM_ALL, "The target has been caught!");
					GameServer()->SendChat(-1, TEAM_ALL, "目标已被捕! ");
					pTarget->m_Score = 0;
					do
					{
						if(!((m_TargetPointer + 1) % m_vSize))
							m_RoundPassed++;
						m_TargetPointer = (m_TargetPointer + 1) % m_vSize;
						m_CurrentTarget = m_vTargetList[m_TargetPointer];
					} while(GameServer()->m_apPlayers[m_CurrentTarget]->m_IsDead);
					str_format(aBuf, sizeof(aBuf), "Round passed: %d", m_RoundPassed);
					dbg_msg("TeeRun", aBuf);
					m_BroadcastTarget = false;
				}

				// target mode is over
				if(m_RoundPassed >= g_Config.m_SvTeeRunEnablePvpAfterRound)
				{
					m_IsTargetmode = false;
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						CPlayer *pPlayer = GameServer()->m_apPlayers[i];
						if(!pPlayer)
							continue;
						pPlayer->m_Score = 0;
					}
					m_IsPvpmode = true;
				}
			}
			// PVP mode
			if(m_IsPvpmode)
			{
				if(!m_BroadcastPvp)
				{
					// str_format(aBuf, sizeof(aBuf), "%d round(s) have passed, who will be the final winner?\nPVP MODE IS NOW ENABLED.", g_Config.m_SvTeeRunEnablePvpAfterRound);
					str_format(aBuf, sizeof(aBuf), "%d 轮已过去, 决出最终的胜者吧!\nPVP死斗模式已开启.", g_Config.m_SvTeeRunEnablePvpAfterRound);
					GameServer()->SendBroadcast(aBuf, -1);
					GameServer()->SendChat(-1, TEAM_ALL, aBuf);
					m_BroadcastPvp = true;
				}
			}
		}
		else if(m_IsRoundEnding)
		{
			if(Server()->Tick() - m_RoundEndTick >= SERVER_TICK_SPEED * 10)
			{
				m_IsRoundEnding = false;
			}
		}
		else
		{
			m_IsRoundStart = true;
			StartTeeRun();
		}
	}
	// if the game ends ...
	else if(m_AlivePlayers == 1)
	{
		if(m_IsRoundStart)
		{
			// temp = "The winner is '";
			// temp.append(m_LastAlivePlayer);
			// temp.append("'!");
			temp = "'";
			temp.append(m_LastAlivePlayer);
			temp.append("'赢了!");
			str_format(aBuf, sizeof(aBuf), temp.c_str());
			GameServer()->SendChat(-1, TEAM_ALL, aBuf);
			OnRoundEnd();
		}
		// m_IsRoundStart = false; //???
		else
		{
			GameServer()->SendBroadcast("Waiting for players", -1);
		}
	}
	else if(!m_AlivePlayers)
	{
		if(m_IsRoundStart)
		{
			GameServer()->SendChat(-1, TEAM_ALL, "No one wins...");
			OnRoundEnd();
		}
		GameServer()->SendBroadcast("Waiting for players", -1);
		m_IsRoundStart = false; //???
	}
	else
	{
		dbg_msg("TeeRun", "ERROR: m_AlivePlayers < 0");
	}
}
