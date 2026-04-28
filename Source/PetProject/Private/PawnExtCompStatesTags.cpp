#include "PawnExtCompStatesTags.h"

namespace InitStateTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "Actor or Component spawned in world");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvaliable,"InitState.DataAvaliable", "Actor or Component data available");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized,"InitState.DataInitialized", "Actor or Component data initialized");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady,"InitState.GameplayReady", "Actor or Component game play ready");
}