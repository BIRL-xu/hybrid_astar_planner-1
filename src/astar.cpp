#include <math.h>
#include <cstring>
#include <queue>
#include <algorithm>
#include "astar.h"

AStar::~AStar(){
    _graph.clear();
    _cells_cache.clear();
    _visited.clear();
}

uint8_t AStar::getCost(int distance) {
    uint8_t cost = 0;
    if (distance < 1.0){
        cost = _grid_lethal_obstacle;
    }else if (distance <= _inflation_radius){
        cost = _grid_inflation_radius;
    }else {
        // make sure cost falls off by Euclidean distance
        double euclidean_distance = distance;
        double factor = exp(-1.0 * _cost_descend_rate 
                        * (euclidean_distance - _inflation_radius));
        cost = (uint8_t)((_grid_inflation_radius - 1) * factor);
    }
    return cost;
}

void AStar::buildGraph(const uint8_t* cdata,
                    const int grid_width,
                    const int grid_height,
                    const int inflation_cell_size){
    _width = grid_width;
    _height = grid_height;
    _inflation_radius = inflation_cell_size;

    _visited.clear();
    _visited.resize(_width*_height, false);
    
    if(_graph.size() != _width*_height){
        _graph.resize(_width*_height);
    }

    std::memcpy(&_graph[0], cdata, _width*_height);
    std::vector<CellWithDist>& obs_cells = _cells_cache[0];
    for(int r = 0; r < _height; r++){
        for(int c = 0; c < _width; c++){
            int index = r*_width+c;
            if(_graph[index] > _grid_inflation_radius){
                obs_cells.push_back(CellWithDist(r,c,0));
            }
        }
    }

    auto iter = _cells_cache.begin();
    while(1){
        if(iter == _cells_cache.end()) break;
        for(int i = 0; i < iter->second.size(); i++){
            int r = iter->second[i]._row;
            int c = iter->second[i]._col;
            int dist = iter->second[i]._dist_from_obstacle;
            int index = r*_width + c;
            if(_visited[index]) continue; 

            uint8_t cost = getCost(dist);
            _graph[index] = _graph[index] > cost ? _graph[index]: cost;
            for(int k = -1; k < 2; k++){
                for(int l = -1; l < 2; l++){
                    if(k==0 && l==0)continue;
                    appendCells(r+k,c+l,dist+1);    
                }
            }
            _visited[index] = true;
        }
        iter++;
    }
    _cells_cache.clear();
}

void AStar::appendCells(int r, int c, int distance){
    int idx = r*_width+c;
    if(idx<0 || idx >= _width*_height) return;
    if(_visited[idx]) return;
    if(_graph[idx]>_grid_inflation_radius) return;
    _cells_cache[distance].push_back(CellWithDist(r,c,distance));
}

bool AStar::findPath(const Cell& start,
                     const Cell& goal,
                     std::vector<Cell>& path){   
    std::vector<Cell> come_from={};
    come_from.resize(_width*_height, Cell(-1,-1));
    std::vector<bool> visit_flag = {};
    visit_flag.resize(_width*_height, false);

    auto cmp = [](SearchPoint left, SearchPoint right) {
         return left._priority > right._priority;
    };
    std::priority_queue<SearchPoint,
        std::vector<SearchPoint>, decltype(cmp)> frontier(cmp);
    frontier.push(SearchPoint(start._row, start._col, 0, 0));
    visit_flag[start._row*_width+start._col] = true;
    Cell former(-1,-1);
    bool succeed = false;
    while(!frontier.empty()){
        SearchPoint current = frontier.top();
        frontier.pop();
        former = Cell(current._row, current._col);
        if(current._col==goal._col && current._row==goal._row){
            succeed = true;
            break;
        }
        for(int i = -1; i < 2; i++){
            for(int j = -1; j < 2; j++){
                if(i == 0 && j == 0) continue;
                int c = current._col + j;
                int r = current._row + i;
                int index = r*_width+c;
                if(index<0 || index>_width*_height-1) continue;
                if(visit_flag[index]) continue;
                if(_graph[index] > 253) continue;
                int heuristic = std::abs(goal._col-c) + std::abs(goal._row-r);
                int next_cost = _graph[index] + current._cost_so_far;
                int priority = next_cost + heuristic;
                frontier.push(SearchPoint(r, c, next_cost, priority));
                visit_flag[index] = true;
                come_from[index] = Cell(current._row, current._col);
            }
        }
    }
    if(!succeed) {
        return false;
    }
    //traceback to generate path.
    while(1){
        int idx = former._row*_width+former._col;
        if(come_from.at(idx)._col == start._col
            && come_from.at(idx)._row == start._row) break;
        path.push_back(former);
        former = come_from.at(idx);
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return true;
}