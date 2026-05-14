#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cstdint>

struct Detail{
    uint16_t type;
    uint64_t number;
};

struct Machine{
    bool busy = false;
    uint64_t summary_time = 0;
    uint64_t finish_time = 0;
    std::queue<Detail> queue;
};

enum TypeMessege {finish, start, wait, ready};

struct Messege{
    TypeMessege type;
    uint64_t k;
    uint16_t i;
    uint16_t j;
    uint16_t p;
    bool operator<(const Messege& o) const {
        return type < o.type;
    }
};

class Manufactory{
private:
    uint16_t M_;
    uint16_t N_;
    std::vector<Machine> machines_;
    std::vector<Messege> messeges_;
    uint16_t* times_;
    uint64_t current_time = 0;
    uint64_t finished_items = 0;
    uint64_t total_items = 0;

    std::string parsing_params(std::ifstream& file){
        std::string first;
        std::getline(file, first);
        std::stringstream first_stream(first);
        int temp;
        if (!(first_stream >> temp) || (temp < 1 || temp > 100)) return first;
        M_ = temp;
        if (!(first_stream >> temp) || (temp < 1 || temp > 100)) return first;
        N_ = temp;
        if (!first_stream.eof()) return first;
        return "";
    }

    std::string parsing_times(std::ifstream& file){
        times_ = new uint16_t[(M_ - 1) * N_];
        for (uint16_t i = 0; i < M_ - 1; i++){
            std::string line;
            std::getline(file, line);
            std::stringstream stream_line(line);
            int time = 0;
            for (uint16_t j = 0; j < N_; j++){
                if (!(stream_line >> time) || (time < 0 || time > 10000)) return line;
                times_[i * N_ + j] = static_cast<uint16_t>(time);
            }
            if (!(stream_line.eof())) return line;
        }
        return "";
    }

    std::string parsing_queues(std::ifstream& file){
        uint64_t count = 0;
        machines_.resize(N_);
        for (uint16_t i = 0; i < N_; i++){
            std::string line;
            std::getline(file, line);
            std::stringstream stream_line(line);
            int quantity;
            if (!(stream_line >> quantity) || (quantity < 0)) return line;
            Machine machine;
            for (int j = 0; j < quantity; j++){
                int temp;
                if (!(stream_line >> temp) || (temp < 0 || temp > (M_ - 2))) return line;
                Detail detail;
                detail.type = static_cast<uint16_t>(temp);
                detail.number = count++;
                machine.queue.push(detail);
                machine.summary_time += times_[detail.type * N_ + i];
            }
            machines_[i] = machine;
            if (!(stream_line.eof())) return line;
        }
        total_items = count;
        return "";
    }

    std::string Check_and_parsing(std::string path){
        std::ifstream file(path);
        std::string check = parsing_params(file);
        if (!check.empty()) return check;
        check = parsing_times(file);
        if (!check.empty()) return check;
        check = parsing_queues(file);
        if (!check.empty()) return check;
        return "";
    }

    int FindNextMachine(){
        uint64_t min = std::numeric_limits<uint64_t>::max();
        int min_machine = 0;
        for (int i = 0; i < N_; i++){
            if (machines_[i].summary_time < min){
                min = machines_[i].summary_time;
                min_machine = i;
            } else if (machines_[i].summary_time == min && i < min_machine){
                min_machine = i;
            }
        }
        return min_machine;
    }

    void PrintMessege(){
        std::sort(messeges_.begin(), messeges_.end());
        for (auto& p: messeges_){
            switch (p.type){
                case finish: std::cout << "finish "; break;
                case start:  std::cout << "start ";  break;
                case wait:   std::cout << "wait ";   break;
                case ready:  std::cout << "ready ";  break;
            }
            std::cout << current_time << " " << p.k << " ";
            if (p.type != ready) std::cout << p.i << " ";
            std::cout << p.j;
            if (p.type == wait) std::cout << " " << p.p;
            std::cout << "\n";
        }
        messeges_.clear();
    }

    void Initialization(){
        for (int i = 0; i < N_; i++){
            Machine& mac = machines_[i];
            if (!mac.queue.empty()){
                Detail& first = mac.queue.front();
                mac.busy = true;
                mac.finish_time = times_[first.type * N_ + i];
                messeges_.push_back({start, first.number, first.type, static_cast<uint16_t>(i), 0});
            }
        }
        PrintMessege();
    }

    void StatusMachine(int number){
        Machine& mac = machines_[number];
        if (mac.queue.empty()) return;

        Detail finished = mac.queue.front();
        mac.queue.pop();
        mac.summary_time -= times_[finished.type * N_ + number];
        mac.busy = false;
        messeges_.push_back({finish, finished.number, finished.type, static_cast<uint16_t>(number), 0});

        if (!mac.queue.empty()){
            Detail& next = mac.queue.front();
            mac.busy = true;
            mac.finish_time = current_time + times_[next.type * N_ + number];
            messeges_.push_back({start, next.number, next.type, static_cast<uint16_t>(number), 0});
        }

        finished.type++;
        if (finished.type == M_ - 1){
            messeges_.push_back({ready, finished.number, finished.type, static_cast<uint16_t>(number), 0});
            finished_items++;
        } else {
            int next_machine = FindNextMachine();
            Detail next_item = {finished.type, finished.number};

            if (machines_[next_machine].summary_time == 0 && !machines_[next_machine].busy){
                machines_[next_machine].queue.push(next_item);
                machines_[next_machine].summary_time += times_[next_item.type * N_ + next_machine];
                machines_[next_machine].busy = true;
                machines_[next_machine].finish_time = current_time + times_[next_item.type * N_ + next_machine];
                messeges_.push_back({start, next_item.number, next_item.type, static_cast<uint16_t>(next_machine), 0});
            } else {
                int p;
                if (machines_[next_machine].busy){
                    p = static_cast<int>(machines_[next_machine].queue.size() - 1);
                }
                else{
                    p = static_cast<int>(machines_[next_machine].queue.size());
                }
                machines_[next_machine].queue.push(next_item);
                machines_[next_machine].summary_time += times_[next_item.type * N_ + next_machine];
                messeges_.push_back({wait, next_item.number, next_item.type, static_cast<uint16_t>(next_machine), static_cast<uint16_t>(p)});
            }
        }
    }

    void Start(){
        Initialization();
        while (finished_items < total_items){

            uint64_t next_time = std::numeric_limits<uint64_t>::max();
            for (int i = 0; i < N_; i++){
                if (machines_[i].busy && machines_[i].finish_time < next_time){
                    next_time = machines_[i].finish_time;
                }
            }
            if (next_time == std::numeric_limits<uint64_t>::max()) break;

            current_time = next_time;
            for (int i = 0; i < N_; i++){
                if (machines_[i].busy && machines_[i].finish_time == current_time){
                    StatusMachine(i);
                }
            }
            PrintMessege();
        }
        std::cout << "stop " << current_time << "\n";
    }

public:
    Manufactory() = delete;
    Manufactory(std::string path){
        std::string check = Check_and_parsing(path);
        if (!check.empty()){
            std::cerr << check << std::endl;
            exit(1);
        }
        Start();
    }
};