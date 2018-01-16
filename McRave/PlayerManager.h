#pragma once
#include "McRave.h"
#include "PlayerInfo.h"
#include "Singleton.h"

namespace McRave
{
	class PlayerTrackerClass
	{
		map <Player, PlayerInfo> thePlayers;
		int eZerg, eProtoss, eTerran, eRandom;
	public:
		map <Player, PlayerInfo>& getPlayers() { return thePlayers; }
		void onStart(), update(), updateUpgrades(PlayerInfo&);

		int getNumberZerg() { return eZerg; }
		int getNumberProtoss() { return eProtoss; }
		int getNumberTerran() { return eTerran; }
		int getNumberRandom() { return eRandom; }
	};
}

typedef Singleton<McRave::PlayerTrackerClass> PlayerTracker;