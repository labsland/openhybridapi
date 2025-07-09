
#ifndef BUTTERFLYSIMULATION_H
#define BUTTERFLYSIMULATION_H

#include "../labsland/simulations/simulation.h"
#include <string>
#include <cstring>
#include <sstream>
#include <stdio.h>

using namespace std;

// My constant defines
#define IS_LITERAL  0
#define IS_GPIO     1
#define IS_BUFFER   2
#define IS_SWITCH   3
#define IS_LED      4

#define IS_LITERAL_NEXT_CHAR_SIZE 1
#define IS_GPIO_NEXT_CHAR_SIZE    2
#define IS_BUFFER_NEXT_CHAR_SIZE  1
#define IS_SWITCH_NEXT_CHAR_SIZE  1
#define IS_LED_NEXT_CHAR_SIZE     1

#define BUFFER_ARRAY_SIZE   10
#define LED_ARRAY_SIZE      5
#define MAX_CHAR_ARRAY_SIZE 1024

// struct that receives the string
struct ButterflyRequest : public BaseInputDataType {
    char my_string[MAX_CHAR_ARRAY_SIZE];

    bool deserialize(std::string const & input) {
        if (input.size() < MAX_CHAR_ARRAY_SIZE - 1) {
            strcpy(my_string, input.c_str());
            return true;
        }
        return false;
    }
};

// struct that tracks the virtual LED states
struct ButterflyData : public BaseOutputDataType {
    bool virtual_led[LED_ARRAY_SIZE];

    string serialize() const {
        stringstream stream;
        for(int i = 0; i < LED_ARRAY_SIZE; i++){
            stream << "led" << i << "=" << virtual_led[i];
            if(i < LED_ARRAY_SIZE - 1){
                stream << "&";
            }
        }

        return stream.str();
    }
};

class ButterflySimulation : public Simulation<ButterflyData, ButterflyRequest> {
    private:
        vector<bool> buffer;
        vector<bool> output_gpio_tracker;
        vector<bool> input_gpio_tracker;
        string my_string = "";
    public:

        ButterflySimulation() = default;
        virtual void update(double delta) override;
        virtual void initialize() override;

        virtual int getNumberOfSimulationInputs(void) = 0;
        virtual int getNumberOfSimulationOutputs(void) = 0;

        void print_gpio_header_states();
        void print_buffer_states();
        void print_led_states();
        bool read_literal_logic(string s);
        bool read_switch_logic(string s);
        bool read_gpio_logic(string s);
        void update_gpio_logic(string s, bool o);
        bool read_buffer_logic(string s);
        void update_buffer_logic(string s, bool o);
        void update_led_logic(string s, bool o);
        int read_gate_input(string substring);
        int read_gate_output(string substring);
        bool handle_input(string substring, int &start_index);
        void handle_output(string substring, int &start_index, bool my_output);
        int read_logic_gate(string substring);
};

class FPGA_DE1SoC_ButterflySimulation : public ButterflySimulation {
    public:
        virtual int getNumberOfSimulationInputs(void) override;
        virtual int getNumberOfSimulationOutputs(void) override;
};

class STM32_WB55RG_ButterflySimulation : public ButterflySimulation {
    public:
        virtual int getNumberOfSimulationInputs(void) override;
        virtual int getNumberOfSimulationOutputs(void) override;
};

#endif
