#include <sourcemod>
#include <sdktools>

ConVar g_Cvar_HealOverTime;
ConVar g_Cvar_ShowMyHP;
ConVar g_Cvar_ShowEnemyHP;

Handle g_hHealthDisplayTimer = INVALID_HANDLE;

float g_flLastHealTime[MAXPLAYERS+1];

int m_iMaxHealth = -1;
int m_iHealth = -1;

public Plugin:myinfo =
{
	name = "Monster HPBar",
	author = "raydan and donrevan",
	description = "HUD for npcs-in-css",
	version = "1.0",
	url = "https://forums.alliedmods.net/showthread.php?p=2583637"
};

public void OnPluginStart()
{
	g_Cvar_HealOverTime = CreateConVar("monster_heal_over_time", "1", "Specifies how fast a player should heal over time (set to 0 to disable)", _, true, 0.0);
	g_Cvar_ShowMyHP = CreateConVar("monster_hud_showplayer", "1", "Specifies if current player health and armor should be displayed", _, true, 0.0, true, 1.0);
	g_Cvar_ShowEnemyHP = CreateConVar("monster_hud_showenemy", "1", "Specifies if npc in players aim cone should be displayed", _, true, 0.0, true, 1.0);

	g_Cvar_HealOverTime.AddChangeHook(OnHPBarConVarChange);
	g_Cvar_ShowMyHP.AddChangeHook(OnHPBarConVarChange);
	g_Cvar_ShowEnemyHP.AddChangeHook(OnHPBarConVarChange);
}

public void OnMapStart()
{
	CreateHealthTimer();
}

public void OnMapEnd()
{
	KillHealthTimer();
}

public void OnClientPutInServer(int client)
{
	g_flLastHealTime[client] = 0.0;
}

public void OnHPBarConVarChange(ConVar convar, const char[] oldValue, const char[] newValue)
{
	float healOverTime = g_Cvar_HealOverTime.FloatValue;
	bool showMyHP = g_Cvar_ShowMyHP.BoolValue;
	bool showEnemyHP = g_Cvar_ShowEnemyHP.BoolValue;

	if (!showMyHP && !showEnemyHP && healOverTime < 0.1)
	{
		KillHealthTimer();
	} else {
		CreateHealthTimer();
	}
}

void CreateHealthTimer()
{
	if (g_hHealthDisplayTimer == INVALID_HANDLE)
	{
		g_hHealthDisplayTimer = CreateTimer(0.2, OnHealthTimer, 0, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE );
	}
}

void KillHealthTimer()
{
	if (g_hHealthDisplayTimer != INVALID_HANDLE)
	{
		KillTimer(g_hHealthDisplayTimer);
		g_hHealthDisplayTimer = INVALID_HANDLE;
	}
}

public Action OnHealthTimer(Handle timer, any data)
{
	float healOverTime = g_Cvar_HealOverTime.FloatValue;
	bool showMyHP = g_Cvar_ShowMyHP.BoolValue;
	bool showEnemyHP = g_Cvar_ShowEnemyHP.BoolValue;

	float gameTime = GetGameTime();
	for (int i=1; i<=MaxClients; i++)
	{
		if (IsClientConnected(i) && IsClientInGame(i) && !IsFakeClient(i) && IsPlayerAlive(i))
		{
			char hpInfo[16] = "";
			if (showMyHP || healOverTime >= 0.1)
			{
				int health = GetClientHealth(i);
				int armor = GetClientArmor(i);

				if (healOverTime >= 0.1)
				{
					if (gameTime - g_flLastHealTime[i] >= healOverTime)
					{
						g_flLastHealTime[i] = gameTime;
						if(health < 100)
						{
							++health;
							SetEntityHealth(i, health);
						}
					}
				}

				Format(hpInfo, sizeof(hpInfo), "HP: %d\nAP: %d", health, armor);
			}

			int targetHealth = 0;
			int targetMaxHealth = 0;
			char npcInfo[64];
			if (showEnemyHP)
			{
				int targetEntity = GetClientAimTarget(i, false);
				if (IsValidEntity(targetEntity))
				{
					char className[32];
					GetEntityClassname(targetEntity, className, sizeof(className));

					if (className[0] == 'n' && className[1] == 'p' && className[2] == 'c' && className[3] == '_' && className[4] != '\0')
					{
						char npcName[32];
						strcopy(npcName, sizeof(npcName), className[4]);

						if(m_iMaxHealth == -1) {
							m_iMaxHealth = FindDataMapInfo(targetEntity, "m_iMaxHealth");
						}
						if(m_iHealth == -1) {
							m_iHealth = FindDataMapInfo(targetEntity, "m_iHealth");
						}

						targetMaxHealth = GetEntData(targetEntity, m_iMaxHealth);
						targetHealth = GetEntData(targetEntity, m_iHealth);

						if (targetHealth < 0)
							targetHealth = 0;

						Format(npcInfo, sizeof(npcInfo), "%s (%.0f/100)", npcName, float(targetHealth)/float(targetMaxHealth)*100);
					}
				}
			}

			if (hpInfo[0] != '\0') {
				PrintKeyHintText(i, "==============\n%s\n==============\n%s", hpInfo, npcInfo);
				//PrintKeyHintText(i, "%s\n%s", hpInfo, npcInfo);
			} else {
				PrintKeyHintText(i, npcInfo);
			}
		}
	}

	return Plugin_Continue;
}

void PrintKeyHintText(int client, const char[] format, any ...)
{
	char buffer[192];
	VFormat(buffer, sizeof(buffer), format, 3);
	Handle hBuffer = StartMessageOne("KeyHintText", client); 
	BfWriteByte(hBuffer, 1); 
	BfWriteString(hBuffer, buffer); 
	EndMessage();
}