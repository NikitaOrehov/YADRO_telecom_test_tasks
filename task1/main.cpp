#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <bitset>
#include <map>
#include <algorithm>

enum Status {unknown, known, visible, visited};

struct Node{
    uint8_t gold;
    uint8_t iron;
    uint8_t gems;
    uint8_t exp;
    std::bitset<4> flags;
    Status status = unknown;
};


class Bot{
private: 
    uint8_t N_;
    uint8_t food_;
    uint64_t result_[5] = {};
    std::vector<std::vector<uint8_t>> graph_;
    std::map<std::string, int> price_ = {{"gold", 11}, {"iron", 7}, {"gems", 23}, {"exp", 1}};
    std::vector<Node> info_;
    std::ofstream file_result;

    bool parsing_N(std::string& line){
        std::stringstream first(line);
        int N;
        if (!(first >> N) || (N > 255 || N < 1)){
            return false;
        }
        if (!(first.eof())) return false;
        N_ = static_cast<uint8_t>(N);
        info_.resize(N_ + 1);
        graph_.resize(N_ + 1);
        return true;
    }

    bool parsing_neighbours(std::stringstream& line_stream, int number){ 
        std::string neighbours;
        if (!(line_stream >> neighbours)){
            return false;
        }  
        std::stringstream neighbours_stream(neighbours);
        std::string temp;
        while(std::getline(neighbours_stream, temp, ',')){
            try{
                int neighbour = std::stoi(temp);
                if (neighbour > 255 || neighbour < 0){
                    return false;
                }
                if (std::find(graph_[number].begin(), graph_[number].end(), static_cast<uint8_t>(neighbour)) == graph_[number].end()){
                    graph_[number].push_back(static_cast<uint8_t>(neighbour));
                }
                if (std::find(graph_[neighbour].begin(), graph_[neighbour].end(), static_cast<uint8_t>(number)) == graph_[neighbour].end()){
                    graph_[neighbour].push_back(static_cast<uint8_t>(number));
                }

            }
            catch(...){
                return false;
            }
        }
        return true;
    }

    bool parsing_resources(std::stringstream& line_stream, int number, Node& node){
        int gold, iron, gems, exp;
        if (!(line_stream >> iron) || !(line_stream >> gold) || !(line_stream >> gems) || !(line_stream >> exp)){
            return false;
        }
        if (!(line_stream.eof())) return false;
        if (!number){
            if (gold || iron || gems || exp) return false;
        }
        else if ((gold < 0 || gold > 255) || (iron < 0 || iron > 255) || (gems < 0 || gems > 255) || (exp < 0 || exp > 255)){
            return false;
        }
        node.gold = static_cast<uint8_t>(gold);
        node.iron = static_cast<uint8_t>(iron);
        node.gems = static_cast<uint8_t>(gems);
        node.exp = static_cast<uint8_t>(exp);
        return true;
    }

    bool parsing_food_target(std::string& last){
        std::stringstream last_stream(last);
        int food;
        if (!(last_stream >> food)){
            return false;
        }
        food_ = static_cast<uint8_t>(food);

        std::string target;
        if (!(last_stream >> target)){
            return false;
        }

        if (target == "gold") price_[target] *= 2;
        else if (target == "iron") price_[target] *= 2;
        else if (target == "gems") price_[target] *= 2;
        else if (target == "exp") price_[target] *= 2;
        else return false;
        return true;
    }

    std::string Check_and_parsing(const std::string path){
        std::ifstream file(path);
        std::string line;
        if (file.is_open()){
            std::getline(file, line);
            if (!parsing_N(line)) return line;

            for (size_t i = 0; i <= static_cast<size_t>(N_); i++){
                Node node;
                int number;

                std::getline(file, line);
                std::stringstream line_stream(line);
                if (!(line_stream >> number) || (number < 0 || number > 255)){
                    return line;
                }

                if (!parsing_neighbours(line_stream, number)) return line;

                if (!parsing_resources(line_stream, number, node)) return line;
            
                info_[number] = node;
            }
            
            std::string last;
            std::getline(file, last);
            if (!parsing_food_target(last)) return last;
            
        }   
        else{
            return "file not found";
        }
        return "";
    }

    void UpdateGraph(int current_room){
        info_[current_room].status = visited;
        for (auto& p: graph_[current_room]){
            int nextRoom = static_cast<int>(p);
            if (info_[nextRoom].status != visited){
                info_[nextRoom].status = visible;
            }
            for (auto& pp: graph_[nextRoom]){
                int nextNextRoom = static_cast<int>(pp);
                if (info_[nextNextRoom].status == unknown){
                    info_[nextNextRoom].status = known;
                }
            }
        }
    }

    int FindNextRoom(int current_room){
        int min_number = 256;
        int nextRoom = 0;
        int count1 = 1;
        int count2 = 0;
        std::queue<int> queue;
        std::bitset<256> unique;
        
        queue.push(current_room);
        unique.set(current_room, true);
        std::vector<int> from(graph_.size(), -1);
        while (!queue.empty()){
            int cur = queue.front();
            queue.pop();

            for (auto& p: graph_[cur]){
                nextRoom = static_cast<int>(p);
                if (!unique.test(nextRoom) && info_[nextRoom].status != unknown){
                    count2 += 1;
                    queue.push(nextRoom);
                    unique.set(nextRoom, true);
                    from[nextRoom] = cur;
                }
                if (nextRoom < min_number && info_[nextRoom].status != visited){
                    min_number = nextRoom;
                }
            }
            count1--;

            if (!count1){
                if (min_number != 256){
                    int cur = min_number;
                    std::vector<int> way;
                    way.push_back(cur);
                    while (from[cur] != current_room){
                        cur = from[cur];
                        way.push_back(cur);
                    }
                    std::reverse(way.begin(), way.end());

                    for (auto& p: way){
                        file_result << "go " << p << "\n";
                        PrintState(p);
                        if (p != min_number) UpdateGraph(p);
                        food_--;
                    }

                    return min_number;
                }
                count1 = count2;
                count2 = 0;
            }
        }
        return -1;
    }

    bool CollectResource(int current_room){
        std::vector<std::string> names = {"gold", "iron", "gems", "exp"};
        int max = -1;
        std::string name_resource;
        for (auto& resource: names){
            int count = 0;
            if (resource == "gold") count = info_[current_room].gold;
            else  if (resource == "iron") count = info_[current_room].iron;
            else  if (resource == "gems") count =  info_[current_room].gems;
            else  if (resource == "exp") count = info_[current_room].exp;
            if (price_[resource] > max && count > 0){
                max = price_[resource];
                name_resource = resource;
            }
        }

        if (max == -1) return false;
        if (info_[current_room].status == visited) food_--;

        if (name_resource == "gold"){
            result_[0] += (max * static_cast<int>(info_[current_room].gold));
            result_[2] += static_cast<int>(info_[current_room].gold);
            info_[current_room].gold = 0;
            info_[current_room].flags.set(1, true);
        }
        else  if (name_resource == "iron"){
            result_[0] += (max * static_cast<int>(info_[current_room].iron));
            result_[1] += static_cast<int>(info_[current_room].iron);
            info_[current_room].iron = 0;
            info_[current_room].flags.set(0, true);
        }
        else  if (name_resource == "gems"){
            result_[0] += (max * static_cast<int>(info_[current_room].gems));
            result_[3] += static_cast<int>(info_[current_room].gems);
            info_[current_room].gems = 0;
            info_[current_room].flags.set(2, true);
        }
        else  if (name_resource == "exp"){
            result_[0] += (max * static_cast<int>(info_[current_room].exp));
            result_[4] += static_cast<int>(info_[current_room].exp);
            info_[current_room].exp = 0;
            info_[current_room].flags.set(3, true);
        }
        
        file_result << "collect " << name_resource << "\n";
        PrintState(current_room);
        return true;
    }

    void PrintState(int current_room){
        file_result << "state " << current_room << " " <<
        ((info_[current_room].flags.test(0)) ? "_" : std::to_string(info_[current_room].iron)) << " " <<
        ((info_[current_room].flags.test(1)) ? "_" : std::to_string(info_[current_room].gold)) << " " << 
        ((info_[current_room].flags.test(2)) ? "_" : std::to_string(info_[current_room].gems)) << " " <<
        ((info_[current_room].flags.test(3)) ? "_" : std::to_string(info_[current_room].exp)) << "\n";
    }

    void FindShortestWay(int current_room, std::vector<uint8_t>& way){
        int nextRoom = 0;
        std::queue<int> queue;
        std::bitset<256> unique;
        std::vector<int> from(graph_.size(), -1);
        
        queue.push(current_room);
        unique.set(current_room, true);

        while (!queue.empty()){
            int cur = queue.front();
            queue.pop();

            if (cur == 0) break;

            std::vector<uint8_t> neighbours = graph_[cur];
            std::sort(neighbours.begin(), neighbours.end());

            for (auto& p: neighbours){
                nextRoom = static_cast<int>(p);
                if (!unique.test(nextRoom) && info_[nextRoom].status == visited){
                    queue.push(nextRoom);
                    unique.set(nextRoom, true);
                    from[nextRoom] = cur;
                }
            }
        }

        uint8_t cur = static_cast<uint8_t>(0);
        way.push_back(cur);
        while (from[static_cast<int>(cur)] != current_room){
            cur = static_cast<uint8_t>(from[static_cast<int>(cur)]);
            way.push_back(cur);
        }

        std::reverse(way.begin(), way.end());
        return;
    }


    int Phase1(){
        uint8_t start_food = food_;
        int current_room = 0;
        while (food_ > (start_food / static_cast<uint8_t>(2))){
            UpdateGraph(current_room);
            int next = FindNextRoom(current_room);
            if (next == -1) break;
            current_room = next;
            CollectResource(current_room);
            UpdateGraph(current_room);
        }
        return current_room;
    }

    void Phase2(int current_room){
        int i = 0;
        std::vector<uint8_t> way;
        FindShortestWay(current_room, way);
        file_result << "go " << static_cast<int>(way[i]) << "\n";
        if (way[i]) PrintState(way[i]);
        food_ -= (way.size());
        while (true){
            if ((food_ > 0 && !CollectResource(way[i])) || !food_){
                i++;
                if (i >= way.size()) break;
                current_room = way[i];
                file_result << "go " << static_cast<int>(way[i]) << "\n";
                if (current_room) PrintState(current_room);
            }
        }
    }

public:
    Bot() = delete;
    Bot(const std::string path){
        std::string check = Check_and_parsing(path);
        file_result.open("result.txt");
        if (!check.empty()){
            file_result << check;
            return;
        }
        int current_room = Phase1();
        Phase2(current_room);
        file_result << "result "  << result_[1] << " " << result_[2] << " " <<
         result_[3] << " " << result_[4] << " " << result_[0];
        file_result.close();
    }

};

int main(int argc, char* argv[]){
    //if (argc < 2) return 0;
    //std::string path = argv[1];
    std::string path = "D:/institute/YADRO/task1/data/in.txt";
    Bot bot(path);
    return 0;
}