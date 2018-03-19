
#include "CAI_NPC.h"
#include "eventqueue.h"
#include "CCombatWeapon.h"

ConVar *sv_gravity = NULL;
ConVar *phys_pushscale = NULL;
ConVar *npc_height_adjust = NULL;
ConVar *ai_path_adjust_speed_on_immediate_turns = NULL;
ConVar *ai_path_insert_pause_at_obstruction = NULL;
ConVar *ai_path_insert_pause_at_est_end = NULL;
ConVar *scene_flatturn = NULL;
ConVar *ai_use_clipped_paths = NULL;
ConVar *ai_moveprobe_usetracelist = NULL;
ConVar *ai_use_visibility_cache = NULL;
ConVar *ai_strong_optimizations = NULL;

ConVar *violence_hblood = NULL;
ConVar *violence_ablood = NULL;
ConVar *violence_hgibs = NULL;
ConVar *violence_agibs = NULL;

ConVar *sv_suppress_viewpunch = NULL;
ConVar *ai_navigator_generate_spikes = NULL;
ConVar *ai_navigator_generate_spikes_strength = NULL;

ConVar *ai_no_node_cache = NULL;
ConVar *sv_stepsize = NULL;
ConVar *hl2_episodic = NULL;
ConVar *ai_follow_use_points = NULL;
ConVar *ai_LOS_mode = NULL;
ConVar *ai_follow_use_points_when_moving = NULL;

ConVar *ammo_hegrenade_max = NULL;
ConVar *sk_autoaim_mode = NULL;

ConVar *sv_strict_notarget = NULL;

ConVar *ai_shot_bias_min = NULL;
ConVar *ai_shot_bias_max = NULL;
ConVar *ai_shot_bias = NULL;
ConVar *ai_spread_pattern_focus_time = NULL;

ConVar *ai_lead_time = NULL;

ConVar *scene_clamplookat = NULL;
ConVar *scene_showfaceto = NULL;
ConVar *flex_maxawaytime = NULL;
ConVar *flex_minawaytime = NULL;
ConVar *flex_maxplayertime = NULL;
ConVar *flex_minplayertime = NULL;

ConVar *ai_find_lateral_los = NULL;

ConVar *npc_sentences = NULL;

ConVar *ai_find_lateral_cover = NULL;


ConVar npc_create_equipment("npc_create_equipment", "");

void CC_Weapon_GiveGun( const CCommand &args )
{
	int index = g_Monster.GetCommandClient();
	CPlayer* pPlayer = UTIL_PlayerByIndex(index);
	if(pPlayer == NULL)
		return;

	CCombatWeapon *gun = dynamic_cast< CCombatWeapon * >( CreateEntityByName(args[1]) );
	if (gun)
	{
		gun->SetLocalOrigin(pPlayer->GetLocalOrigin());
		gun->AddSpawnFlags(SF_NORESPAWN);
		DispatchSpawn(gun->BaseEntity());
		gun->Equip(pPlayer->BaseEntity());
		gun->Touch(pPlayer);
	}
}

void CC_NPC_Create( const CCommand &args )
{
	int index = g_Monster.GetCommandClient();
	CPlayer* pPlayer = UTIL_PlayerByIndex(index);
	if(pPlayer == NULL)
		return;

	gamehelpers->TextMsg(index, TEXTMSG_DEST_CHAT, "[MONSTER] Creating NPC...");

	// Try to create entity
	CAI_NPC *baseNPC = dynamic_cast< CAI_NPC * >( CreateEntityByName(args[1]) );
	if (baseNPC)
	{
		baseNPC->DispatchKeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();

		if ( args.ArgC() == 3 )
		{
			baseNPC->SetName(args[2]);
		}

		DispatchSpawn(baseNPC->BaseEntity());
		// Now attempt to drop into the world		
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer->BaseEntity(), COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			UTIL_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
			}
		}

		baseNPC->Activate();
	}
}



void cmd1_CommandCallback(const CCommand &command)
{
	int client = g_Monster.GetCommandClient();
	if(client)
	{
		gamehelpers->TextMsg(client, TEXTMSG_DEST_CHAT, "[MONSTER] You don't have access to this command!");
	} else {
		Vector vec(129.54f, 34.93f, 207.84f);//(118.25f, -490.82f, 90.03f);

		//vec.Init(6220.0f, 2813.0f, 1090.0f);

		//vec.Init(73.18,-54.81,-60.0);

		//vec.Init(952.65466,61.566082,-58.339985);


		CEntity *cent = CreateEntityByName("npc_barnacle");
		//CEntity *cent = CreateEntityByName("npc_headcrab_fast");
		//CEntity *cent = CreateEntityByName("npc_headcrab_black");

		//CEntity *cent = CreateEntityByName("npc_fastzombie");
		//CEntity cent = CreateEntityByName("npc_fastzombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie");
		//CEntity *cent = CreateEntityByName("npc_poisonzombie");
		
		//CEntity *cent = CreateEntityByName("npc_manhack");
		//CEntity *cent = CreateEntityByName("npc_antlionguard");

		//CEntity *cent = CreateEntityByName("npc_stalker");

		//CEntity *cent = CreateEntityByName("npc_antlion");

		//CEntity *cent = CreateEntityByName("npc_vortigaunt");

		//CEntity *cent = CreateEntityByName("npc_rollermine");
		
		//CEntity *cent = CreateEntityByName("npc_test");
		
		/*CEntity *cent = CreateEntityByName("env_headcrabcanister");
	
		cent->CustomDispatchKeyValue("HeadcrabType", "0");
		cent->CustomDispatchKeyValue("HeadcrabCount", "10");
		
		cent->CustomDispatchKeyValue("SmokeLifetime","60");
		cent->CustomDispatchKeyValue("SkyboxCannisterCount","1");
		cent->CustomDispatchKeyValue("DamageRadius","0");
		cent->CustomDispatchKeyValue("Damage","100");*/

		//CEntity *cent = CreateEntityByName("npc_turret_floor");

		//CEntity *cent = CreateEntityByName("npc_combine");
			
		//CEntity *cent = CreateEntityByName("npc_combine_s");

		//cent->DispatchKeyValue("additionalequipment","weapon_ak47");
		//cent->DispatchKeyValue("tacticalvariant","1");

		CBaseEntity *cbase = cent->BaseEntity();

		CAI_NPC *hc = dynamic_cast<CAI_NPC *>(cent);
		
		//hc->AddSpawnFlags(( 1 << 18 ));

		cent->Teleport(&vec, NULL,NULL);

		//cent->AddSpawnFlags(4096);

		cent->Spawn();
		cent->Activate();


		//hc->GetSequenceKeyValues( 0 );
		

		//g_CEventQueue->AddEvent( cbase, "SelfDestruct", 0.5f, cbase,cbase );

		//hc->Dissolve(NULL, gpGlobals->curtime, false, 0 );

		edict_t *pEdict = servergameents->BaseEntityToEdict(cbase);
		META_CONPRINTF("%p %d %d\n",cbase, cent->entindex_non_network(), engine->IndexOfEdict(pEdict));

	}
}

void monster_dump_CommandCallback(const CCommand &command)
{
	GetEntityManager()->PrintDump();
}

#define GET_CONVAR(name) \
	name = g_pCVar->FindVar(#name); \
	if(name == NULL) { \
		META_CONPRINTF("[%s] %s - CVar not found\n",g_Monster.GetLogTag(), #name); \
		return false; \
	}

bool CommandInitialize()
{
	new ConCommand("npc1",cmd1_CommandCallback, "", 0);
	new ConCommand("monster_dump",monster_dump_CommandCallback, "", 0);
	new ConCommand("npc_create", CC_NPC_Create, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT|FCVAR_GAMEDLL);

	new ConCommand("sm_givegun", CC_Weapon_GiveGun, "", 0);

	GET_CONVAR(sv_gravity);
	GET_CONVAR(phys_pushscale);
	GET_CONVAR(npc_height_adjust);

	GET_CONVAR(ai_path_adjust_speed_on_immediate_turns);
	GET_CONVAR(ai_path_insert_pause_at_obstruction);
	GET_CONVAR(ai_path_insert_pause_at_est_end);
	GET_CONVAR(ai_use_clipped_paths);
	GET_CONVAR(ai_moveprobe_usetracelist);
	GET_CONVAR(scene_flatturn);

	GET_CONVAR(violence_hblood);
	GET_CONVAR(violence_ablood);
	GET_CONVAR(violence_hgibs);
	GET_CONVAR(violence_agibs);

	GET_CONVAR(sv_suppress_viewpunch);
	GET_CONVAR(ai_use_visibility_cache);
	GET_CONVAR(ai_strong_optimizations);
	GET_CONVAR(ai_navigator_generate_spikes);
	GET_CONVAR(ai_navigator_generate_spikes_strength);
	GET_CONVAR(ai_no_node_cache);

	GET_CONVAR(sv_stepsize);
	GET_CONVAR(hl2_episodic);
	GET_CONVAR(ai_follow_use_points);
	GET_CONVAR(ai_LOS_mode);
	GET_CONVAR(ai_follow_use_points_when_moving);

	GET_CONVAR(ammo_hegrenade_max);
	GET_CONVAR(sk_autoaim_mode);
	
	GET_CONVAR(sv_strict_notarget);

	GET_CONVAR(ai_shot_bias_min);
	GET_CONVAR(ai_shot_bias_max);
	GET_CONVAR(ai_shot_bias);
	GET_CONVAR(ai_spread_pattern_focus_time);

	GET_CONVAR(ai_lead_time);

	GET_CONVAR(scene_clamplookat);
	GET_CONVAR(scene_showfaceto);
	GET_CONVAR(flex_maxawaytime);
	GET_CONVAR(flex_minawaytime);
	GET_CONVAR(flex_maxplayertime);
	GET_CONVAR(flex_minplayertime);

	GET_CONVAR(ai_find_lateral_los);
	GET_CONVAR(npc_sentences);

	GET_CONVAR(ai_find_lateral_cover);

	return true;
}