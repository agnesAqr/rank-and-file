#include "Modules/ModuleManager.h"

// 주 게임 모듈. 세 번째 인자는 5.8에서 쓰이지 않는다. 게임 이름은 UBT가 .uproject 이름으로 넣는다(Modules/ModuleManager.h의 IMPLEMENT_PRIMARY_GAME_MODULE).
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, RankGame, "RankAndFile");
