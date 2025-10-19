#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

struct Node {
    int id;
    string name;
};

struct Edge {
    int to_node_id;
    double distance;
    double speed_limit;
    double traffic_multiplier;
    double toll;
};

map<int, Node> nodes_info;
map<int, vector<Edge>> adj_list;

void reconstruct_path(int start_node, int end_node,
                      const map<int, int>& parent_map,
                      const map<int, double>& g_costs) {
    
    cout << "\n--- Recommended Optimal Path ---" << endl;
    vector<int> path;
    int current_node = end_node;
    while (parent_map.count(current_node)) {
        path.push_back(current_node);
        if (current_node == start_node) break;
        current_node = parent_map.at(current_node);
    }
    
    if (path.empty() || path.back() != start_node) {
        cout << "No path found." << endl; 
        return;
    }
    
    reverse(path.begin(), path.end());

    cout << "Path: " << endl;
    for (size_t i = 0; i < path.size(); ++i) {
        cout << "  " << (i + 1) << ". " << nodes_info[path[i]].name;
        if (i < path.size() - 1) cout << endl;
    }
    cout << "\n\nOptimality Score: " << g_costs.at(end_node) << " (lower is better)" << endl;
}

void find_optimal_path(int start_node, int end_node, double w_time, double w_dist, double w_cost) {
    
    double max_time = 0, max_dist = 0, max_cost = 0;
    const double fuel_price_per_km = 1.5;

    for(const auto& pair : adj_list) {
        for(const auto& edge : pair.second) {
            double time = (edge.distance / edge.speed_limit) * edge.traffic_multiplier * 60.0;
            double cost = edge.toll + (edge.distance * fuel_price_per_km);
            if (time > max_time) max_time = time;
            if (edge.distance > max_dist) max_dist = edge.distance;
            if (cost > max_cost) max_cost = cost;
        }
    }
    
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    map<int, double> g_costs;
    map<int, int> parent_map;

    for (const auto& pair : nodes_info) {
        g_costs[pair.first] = numeric_limits<double>::infinity();
    }
    
    g_costs[start_node] = 0.0;
    parent_map[start_node] = -1;
    pq.push({0.0, start_node});

    while (!pq.empty()) {
        int current_node_id = pq.top().second;
        pq.pop();

        if (current_node_id == end_node) {
            reconstruct_path(start_node, end_node, parent_map, g_costs);
            return;
        }

        if (adj_list.find(current_node_id) == adj_list.end()) continue;

        for (const auto& edge : adj_list.at(current_node_id)) {
            double time = (edge.distance / edge.speed_limit) * edge.traffic_multiplier * 60.0;
            double cost = edge.toll + (edge.distance * fuel_price_per_km);

            double norm_time = max_time > 0 ? time / max_time : 0;
            double norm_dist = max_dist > 0 ? edge.distance / max_dist : 0;
            double norm_cost = max_cost > 0 ? cost / max_cost : 0;
            
            double unified_score = (norm_time * w_time) + (norm_dist * w_dist) + (norm_cost * w_cost);
            double new_g_cost = g_costs[current_node_id] + unified_score;

            if (new_g_cost < g_costs[edge.to_node_id]) {
                g_costs[edge.to_node_id] = new_g_cost;
                parent_map[edge.to_node_id] = current_node_id;
                pq.push({new_g_cost, edge.to_node_id});
            }
        }
    }
    reconstruct_path(start_node, end_node, parent_map, g_costs);
}

bool parse_map_file(const string& file_path) {
    ifstream file(file_path);
    if (!file.is_open()) {
        cerr << "Error: Could not open map file '" << file_path << "'" << endl;
        return false;
    }
    try {
        json data = json::parse(file);
        for (const auto& node : data["nodes"]) {
            nodes_info[node["id"]] = {node["id"], node["name"]};
        }
        for (const auto& edge : data["edges"]) {
            adj_list[edge["from"]].push_back({edge["to"], edge["distance"], edge["speed_limit"], edge["traffic"], edge["toll"]});
            adj_list[edge["to"]].push_back({edge["from"], edge["distance"], edge["speed_limit"], edge["traffic"], edge["toll"]});
        }
    } catch (json::parse_error& e) {
        cerr << "JSON parse error: " << e.what() << endl; 
        return false;
    }
    return true;
}

int main() {
    string map_file;
    int start_node, end_node;
    double w_time, w_dist, w_cost;

    cout << "--- Apex Pathfinder ---" << endl;
    cout << "Enter map filename (e.g., map.json): ";
    cin >> map_file;

    cout << "Enter starting node ID: ";
    cin >> start_node;

    cout << "Enter ending node ID: ";
    cin >> end_node;

    cout << "\nEnter your priorities (weights must sum to 1.0):" << endl;
    cout << "Weight for time (e.g., 0.7): ";
    cin >> w_time;
    cout << "Weight for distance (e.g., 0.1): ";
    cin >> w_dist;
    cout << "Weight for cost (e.g., 0.2): ";
    cin >> w_cost;

    if (abs(w_time + w_dist + w_cost - 1.0) > 0.01) {
        cerr << "Error: Weights must sum to 1.0. Please run again." << endl;
        return 1;
    }

    if (!parse_map_file(map_file)) {
        return 1;
    }

    cout << "\nCalculating the most optimal path based on your preferences..." << endl;
    find_optimal_path(start_node, end_node, w_time, w_dist, w_cost);

    return 0;
}

