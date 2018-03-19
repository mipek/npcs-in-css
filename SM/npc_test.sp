#pragma semicolon 1
#include <sourcemod>
#include <sdktools>


enum Entity_Function
{
	EF_NONE = 0,

	EF_Init,
	EF_PostInit,
	EF_Spawn,
	EF_Teleport,
	EF_UpdateOnRemove,
	EF_OnTakeDamage,


	EF_CENTITY,


	EF_CANIMATING,


	EF_CANIMATINGOVERLAY,


	EF_CFLEX,


	EF_CCOMBATCHARACTER,


	EF_CAI_NPC,

	EF_LAST
};


native MM_CreateNPC(data_size=0);
native MM_DestoryNPC();
native any:MM_CallBase(Entity_Function:type, any:...);


public g_pEntity;
public g_NPC_ID;
public String:g_NPC_Name[] = "npc_test";


public Plugin:myinfo =
{
	name = "npc_test",
	author = "",
	description = "npc_test",
	version = "1.0.0.0",
	url = ""
};


enum MemberVar
{
	id,
	Float:pos,
	Float:vec[3],
	String:ss[512],
	hh,
	gg,
	Float:ff[6],
	syy[128],
	fff[88],
	String:qq[325],
	vvv
};

public pNPC[MemberVar];

new g_count = 0;
new g_index = 0;

public OnPluginStart()
{
	PrintToServer("%d",sizeof(pNPC));
	
	pNPC[id] = 999;
	PrintToServer("%d %d %d",g_NPC_ID, g_pEntity, pNPC[id]);

	g_NPC_ID = MM_CreateNPC(sizeof(pNPC));
	
	RegConsoleCmd("e1",e1);
}

public Action:e1(client, args)
{
	MM_CallBase(EF_Spawn);
	return Plugin_Handled;
}

public OnPluginEnd()
{
	MM_DestoryNPC();
}

public M_Init()
{
	PrintToServer("M_Init");
	
}

public M_PostInit() // custom!
{
	//pNPC[id] = 88;
	//PrintToServer("M_PostInit");
	//pNPC[id] += 1;
	
	//no base
}

public M_Spawn()
{
	new local_count = g_count;
	g_index += 11;
	pNPC[id] = g_index;
	
	if(local_count <= 0)
	{
		new ent = CreateEntityByName("npc_test");
		g_count++;
		DispatchSpawn(ent);
	}
	
	MM_CallBase(EF_Spawn);
	PrintToServer("%d M_Spawn End %d",local_count, pNPC[id]);
	
	//CreateTimer(1.0, FFF);
}

public Action:FFF(Handle:timer)
{
	PrintToServer("FFF %d",pNPC[id]);
	return Plugin_Handled;
}

public M_InitCustomSchedules()
{
	
	
}

