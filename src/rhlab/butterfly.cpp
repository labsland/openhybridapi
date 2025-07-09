#include <iostream>
#include <stdio.h>
#include <string.h>
#include "butterfly.h"

using namespace std;

void ButterflySimulation::initialize(){

    // this->targetDevice.setGpio(0, true); //first output gpio
    // this->targetDevice.getGpio(0);  // first input gpio, which is different

    // this->log() << "Number of Simulation Outputs: " << this->getNumberOfSimulationOutputs() << endl;
    this->targetDevice->initializeSimulation(this->getNumberOfSimulationOutputs(), this->getNumberOfSimulationInputs());

    for(int i = 0; i < this->getNumberOfSimulationOutputs(); i++){
        this->output_gpio_tracker.push_back(false);
    }

    for(int i = 0; i < this->getNumberOfSimulationInputs(); i++){
        this->input_gpio_tracker.push_back(this->targetDevice->getGpio(i));
    }

    for(int i = 0; i < BUFFER_ARRAY_SIZE; i++){
        this->buffer.push_back(false);
    }


    for(int i = 0; i < LED_ARRAY_SIZE; i++){
        mState.virtual_led[i] = false;
    }

    setReportWhenMarked(true);
}

// Prints the current gpio header states
void ButterflySimulation::print_gpio_header_states(){
    this->log() << "----- Input -----" << endl;
    for(int i = 0; i < this->getNumberOfSimulationInputs(); i++){
        this->log() << "g" << i << ": " << this->input_gpio_tracker[i] << endl;
    }

    this->log() << "----- Output -----" << endl;
    for(int i = 0; i < this->getNumberOfSimulationOutputs(); i++){
        this->log() << "g" << i << ": " << this->output_gpio_tracker[i] << endl;
    }
}

// Prints the current buffer states
void ButterflySimulation::print_buffer_states(){
    this->log() << "buffer: ";
    for(int i = 0; i < BUFFER_ARRAY_SIZE; i++){
        this->log() << this->buffer[i] << " ";
    }
    this->log() << endl;
}

// Prints the current LED states
void ButterflySimulation::print_led_states(){
    this->log() << "virtual led: ";
    for(int i = 0; i < LED_ARRAY_SIZE; i++){
        this->log() << mState.virtual_led[i] << " ";
    }
    this->log() << endl;
}

// Converts LT or LF to a boolean True or False
bool ButterflySimulation::read_literal_logic(string s){
    return s[0] == 'T';
}

// Converts ST or SF to a boolean True or False
bool ButterflySimulation::read_switch_logic(string s){
    return s[0] == 'T';
}

// From a string that provdes the gpio index (i.e. g02), obtain the current GPIO value
bool ButterflySimulation::read_gpio_logic(string s){
    int index = stoi(s);
    this->input_gpio_tracker[index] = this->targetDevice->getGpio(index);
    return this->targetDevice->getGpio(index);
}

// From a string that provides the gpio index (i.e. g02), set the current GPIO value
void ButterflySimulation::update_gpio_logic(string s, bool o){
    int index = stoi(s);
    this->output_gpio_tracker[index] = o;
    this->targetDevice->setGpio(index, o);

}

// From a string that provides the buffer index (i.e. b0), obtain the current buffer value
bool ButterflySimulation::read_buffer_logic(string s){
    int index = stoi(s);
    return this->buffer[index];
}

// From a string that provides the buffer index (i.e. b0), set the current buffer value
void ButterflySimulation::update_buffer_logic(string s, bool o){
    int index = stoi(s);
    this->buffer[index] = o;
}

// From a string that provides the led index (i.e. d0), set the current led value
void ButterflySimulation::update_led_logic(string s, bool o){
    int index = stoi(s);
    mState.virtual_led[index] = o;
    // requestReportState();
}

// In a logic gate input, check to see if it is connected to one of the 4 things:
//       - Power rail (i.e. LT or LF)
//       - GPIO with index (i.e. g02)
//       - buffer with index (i.e. b0)
//       - Switch (i.e. ST or SF)
int ButterflySimulation::read_gate_input(string substring){
    int in;
    if(substring == "L"){
        return IS_LITERAL;
    }
    else if(substring == "g"){
        return IS_GPIO;
    }
    else if(substring == "b"){
        return IS_BUFFER;
    }
    else if(substring == "S"){
        return IS_SWITCH;
    }
    else{
        return 5;
    }
}

// In a logic gate output, check to see if it is connected to one of the 4 things:
//       - GPIO with index (i.e. g02)
//       - buffer with index (i.e. b0)
//       - LED with index (i.e. d0)
int ButterflySimulation::read_gate_output(string substring){
    int out;
    if(substring == "g"){
        return IS_GPIO;
    }
    else if(substring == "b"){
        return IS_BUFFER;
    }
    else if(substring == "d"){
        return IS_LED;
    }
    else{
        return 5;
    }

}

// Read the first parts of the string that handles the input. Check to see if
// connected to a power rail, switch, gpio, or buffer
bool ButterflySimulation::handle_input(string substring, int &start_index){
    int p;
    p = read_gate_input(substring.substr(start_index++, 1));
    bool my_input = 0;
    int temp = start_index;
    switch(p){
        case IS_LITERAL:
            my_input = read_literal_logic(substring.substr(temp, IS_LITERAL_NEXT_CHAR_SIZE));
            start_index += IS_LITERAL_NEXT_CHAR_SIZE;
            break;
        case IS_SWITCH:
            my_input = read_switch_logic(substring.substr(temp, IS_SWITCH_NEXT_CHAR_SIZE));
            start_index += IS_SWITCH_NEXT_CHAR_SIZE;
            break;
        case IS_GPIO:
            my_input = read_gpio_logic(substring.substr(temp, IS_GPIO_NEXT_CHAR_SIZE));
            start_index += IS_GPIO_NEXT_CHAR_SIZE;
            break;
        case IS_BUFFER:
            my_input = read_buffer_logic(substring.substr(temp, IS_BUFFER_NEXT_CHAR_SIZE));
            start_index += IS_BUFFER_NEXT_CHAR_SIZE;
            break;
        default:
            break;
    }
    return my_input;
}

// Reads the later parts of the string that handles the output. Check to see if
// connected to a gpio, buffer, or led
void ButterflySimulation::handle_output(string substring, int &start_index, bool logic_gate_output){
    int p;
    p = read_gate_output(substring.substr(start_index++, 1));
    int temp = start_index;
    switch(p){
        case IS_GPIO:
            update_gpio_logic(substring.substr(temp, IS_GPIO_NEXT_CHAR_SIZE), logic_gate_output);
            start_index += IS_GPIO_NEXT_CHAR_SIZE;
            break;
        case IS_BUFFER:
            update_buffer_logic(substring.substr(temp, IS_BUFFER_NEXT_CHAR_SIZE), logic_gate_output);
            start_index += IS_BUFFER_NEXT_CHAR_SIZE;
            break;
        case IS_LED:
            update_led_logic(substring.substr(temp, IS_LED_NEXT_CHAR_SIZE), logic_gate_output);
            start_index += IS_LED_NEXT_CHAR_SIZE;
            break;
        default:
            break;
    }
}

// Reads a logic gate section and digitally computes the new output logic
// state, depending on the inputs to that logic gate
int ButterflySimulation::read_logic_gate(string substring){
    int start_index = 0;
    bool logic_gate_input_1 = false;
    bool logic_gate_input_2 = false;
    bool logic_gate_output = false;

    // Handles the NOT gate logic
    if(substring[start_index] == 'n'){
        // Handle input portion
        start_index++;
        logic_gate_input_1 = handle_input(substring, start_index);

        // Compute boolean logic
        logic_gate_output = !logic_gate_input_1;

        // Handle output portion
        handle_output(substring, start_index, logic_gate_output);
        while(substring[start_index] == ','){
            start_index++;
            handle_output(substring, start_index, logic_gate_output);
        }
    }
    // Handles the AND gate logic
    else if(substring[start_index] == 'a'){
        // Handle input portion
        start_index++;
        logic_gate_input_1 = handle_input(substring, start_index);
        logic_gate_input_2 = handle_input(substring, start_index);

        // Compute boolean logic
        logic_gate_output = logic_gate_input_1 && logic_gate_input_2;

        // Handle output portion
        handle_output(substring, start_index, logic_gate_output);
        while(substring[start_index] == ','){
            start_index++;
            handle_output(substring, start_index, logic_gate_output);
        }
    }
    // Handles the OR gate logic
    else if(substring[start_index] == 'o'){
        // Handle input portion
        start_index++;
        logic_gate_input_1 = handle_input(substring, start_index);
        logic_gate_input_2 = handle_input(substring, start_index);

        // Compute boolean logic
        logic_gate_output = logic_gate_input_1 || logic_gate_input_2;

        // Handle output portion
        handle_output(substring, start_index, logic_gate_output);
        while(substring[start_index] == ','){
            start_index++;
            handle_output(substring, start_index, logic_gate_output);
        }
    }
    // Handles the XOR gate logic
    else if(substring[start_index] == 'x'){
        // Handle input portion
        start_index++;
        logic_gate_input_1 = handle_input(substring, start_index);
        logic_gate_input_2 = handle_input(substring, start_index);

        // Compute boolean logic
        logic_gate_output = logic_gate_input_1 ^ logic_gate_input_2;

        // Handle output portion
        handle_output(substring, start_index, logic_gate_output);
        while(substring[start_index] == ','){
            start_index++;
            handle_output(substring, start_index, logic_gate_output);
        }
    }
    // Handles the non-logic gate logic
    else if(substring[start_index] == 'y'){
        // Handle input portion
        start_index++;
        logic_gate_input_1 = handle_input(substring, start_index);

        // Compute boolean logic
        logic_gate_output = logic_gate_input_1;

        // Handle output portion
        handle_output(substring, start_index, logic_gate_output);
        while(substring[start_index] == ','){
            start_index++;
            handle_output(substring, start_index, logic_gate_output);
        }
    }

    return start_index;
}

void ButterflySimulation::update(double delta){

    ButterflyRequest request;
    bool requestWasRead = readRequest(request);
    if(requestWasRead) {
        this->my_string = string(request.my_string);
    }
    string my_string = this->my_string;

    int index = 0;
    int my_string_length = my_string.length();
    int while_loop_counter = 1;

    if(my_string_length < 2){
        return;
    }

    this->log() << endl << "===== GPIO STATES =====" << endl;
    print_gpio_header_states();
    this->log() << endl << "===== BUFFER STATES =====" << endl;
    print_buffer_states();
    this->log() << endl << "===== LED STATES =====" << endl;
    print_led_states();
    this->log() << mState.serialize() << endl;

    this->log() << "===== STRING PROTOCOL =====" << endl;
    while(index < my_string_length){
        this->log() << "My string: " << my_string << endl;
        this->log() << "Iter " << while_loop_counter << " : " << my_string.substr(index, my_string_length - index);
        while_loop_counter++;
        index += read_logic_gate(my_string.substr(index, my_string_length - index));
        if(my_string[index] == ';' || my_string[index] == '\n'){
            index = index + 1;
        }
    }

    index = 0;
    while_loop_counter = 1;

    this->log() << endl << "===== GPIO STATES =====" << endl;
    print_gpio_header_states();
    this->log() << endl << "===== BUFFER STATES =====" << endl;
    print_buffer_states();
    this->log() << endl << "===== LED STATES =====" << endl;
    print_led_states();
    this->log() << mState.serialize() << endl;

    requestReportState();
    // reportUpdate();
}

int FPGA_DE1SoC_ButterflySimulation::getNumberOfSimulationInputs() {
    return 7;
}

int FPGA_DE1SoC_ButterflySimulation::getNumberOfSimulationOutputs() {
    return 5;
}

int STM32_WB55RG_ButterflySimulation::getNumberOfSimulationInputs() {
    return 6;
}

int STM32_WB55RG_ButterflySimulation::getNumberOfSimulationOutputs() {
    return 5;
}
