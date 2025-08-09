#ifndef GAME_SERVER_GAMEMODES_TEERUN_TEERUN_H
#define GAME_SERVER_GAMEMODES_TEERUN_TEERUN_H

#include "../vanilla/base_vanilla.h"
#include <game/server/gamecontroller.h>
#include <vector>

class CGameControllerTeeRun : public CGameControllerVanilla
{
public:
	CGameControllerTeeRun(class CGameContext *pGameServer);
	~CGameControllerTeeRun();

	bool m_IsRoundStart = false;
	bool m_IsRoundEnding = false;
	int m_RoundEndTick;

	bool m_IsSolomode = false;
	bool m_IsTargetmode = false;
	bool m_IsPvpmode = false;

	int m_SoloTick;

	std::vector<int> m_vTargetList;
	int m_CurrentTarget = -1;
	int m_TargetPointer = -1;
	int m_vSize;
	bool m_BroadcastTarget = false;
	int m_TargetTick;

	int m_RoundPassed = 0;
	bool m_BroadcastPvp = false;

	void Tick() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnPlayerConnect(class CPlayer *pPlayer) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	void OnRoundStart() override;
	void OnRoundEnd() override;

	virtual void Snap(int SnappingClient);
	virtual void StartTeeRun();
};
#endif // GAME_SERVER_GAMEMODES_TEERUN_TEERUN_H
