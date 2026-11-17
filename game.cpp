#include "game.h"
#include "choice.h"
#include "state.h"


bool isValidAction(const State& state, const Game& game, Choice action) {
    switch (action) {
        case LEFT:  
            return state.agentX > 0;
        case RIGHT: 
            return state.agentX < game.gridSize - 1;
        case UP:    
            return state.agentY > 0;
        case DOWN:  
            return state.agentY < game.gridSize - 1;
    }

    return false;
}