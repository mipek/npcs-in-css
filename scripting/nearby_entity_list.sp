// Simple plugin to list nearby entities (useful for debugging and fixing hl2dm maps via Stripper).
#include <sourcemod>
#include <sdktools>

#define MAX_NEAR_ENTS 64

#define NEAR_ENTITY_RADIUS 100.0

public Plugin myinfo = 
{
    name = "Nearby Entities List",
    author = "donrevan",
    description = "Lists nearby entities for admins",
    version = "1.0",
    url = ""
};

public void OnPluginStart()
{
    RegAdminCmd("sm_listentities", Command_ListEntities, ADMFLAG_SLAY, "Lists nearby entities.");
}

int FindEntityInSphere(int startEntity, float origin[3], float radius)
{
    int maxEntities = GetMaxEntities();
    for (int entity = startEntity + 1; entity < maxEntities; entity++)
    {
        if (!IsValidEntity(entity)) 
            continue;

        float entityPosition[3];
        GetEntPropVector(entity, Prop_Data, "m_vecOrigin", entityPosition);

        float distance = GetVectorDistance(origin, entityPosition);

        if (distance <= radius)
        {
            return entity;
        }
    }

    return -1;
}

public int SortEntitiesByDistance(int a, int b, const int[] array, Handle hndl)
{
    float pos1[3], pos2[3];
    GetEntPropVector(a, Prop_Data, "m_vecOrigin", pos1);
    GetEntPropVector(b, Prop_Data, "m_vecOrigin", pos2);
    float distance = GetVectorDistance(pos1, pos2);
    if (distance > 0.0) {
        return -1;
    } else if (distance < 0.0) {
        return 1;
    }
    return 0;
}

public Action:Command_ListEntities(client, args)
{
    if (!IsClientInGame(client) || !IsPlayerAlive(client))
    {
        ReplyToCommand(client, "You need to be in-game and alive to use this command.");
        return Plugin_Handled;
    }

    float adminPosition[3];
    GetClientAbsOrigin(client, adminPosition);

    float radius = NEAR_ENTITY_RADIUS;

    int nearbyEntities[MAX_NEAR_ENTS];
    int nearEntityCount = 0;
    int entity = -1;
    while ((entity = FindEntityInSphere(entity, adminPosition, radius)) != -1)
    {
        nearbyEntities[nearEntityCount++] = entity;
        if (nearEntityCount == MAX_NEAR_ENTS)
        {
            break;
        }
    }
    SortCustom1D(nearbyEntities, nearEntityCount, SortEntitiesByDistance);

    float entityPosition[3];
    char entityName[128], targetName[128];
    ReplyToCommand(client, "--------------------------------------");
    for (int i=0; i<nearEntityCount; ++i)
    {
        entity = nearbyEntities[i];

        GetEntPropVector(entity, Prop_Data, "m_vecOrigin", entityPosition);

        GetEntityClassname(entity, entityName, sizeof(entityName));
        GetEntPropString(entity, Prop_Data, "m_iName", targetName, sizeof(targetName));

        ReplyToCommand(client, "%s (%s), Position: [%.1f, %.1f, %.1f]", entityName, targetName, entityPosition[0], entityPosition[1], entityPosition[2]);
    }

    return Plugin_Handled;
}
