`timescale 1ns/1ps
module airport_garage_adapter_tb;
 reg clock=0,reset=1,entryWaitingAsync=0,exitWaitingAsync=0,entryCompleteAsync=0,exitCompleteAsync=0,worldReadyAsync=0;
 wire entryWaiting,exitWaiting,ready,arrived,departed,completionAck;
 airport_garage_adapter dut(.*);
 always #5 clock=~clock;
 integer arrivals=0,departures=0;
 always @(negedge clock)begin if(arrived)arrivals=arrivals+1;if(departed)departures=departures+1;end
 task clocks(input integer n); repeat(n)@(negedge clock);endtask
 initial begin
 clocks(3);reset=0;worldReadyAsync=1;entryWaitingAsync=1;clocks(5);
 if(!ready||!entryWaiting)$fatal(1,"Input synchronizer failed");
 entryCompleteAsync=1;clocks(20);if(arrivals!=1||!completionAck)$fatal(1,"Held entry must count exactly once");
 worldReadyAsync=0;clocks(5);entryCompleteAsync=0;clocks(5);if(!completionAck)$fatal(1,"Stale world must hold ack");
 worldReadyAsync=1;clocks(6);if(completionAck)$fatal(1,"Ack must clear after completion low");
 exitCompleteAsync=1;clocks(20);if(departures!=1)$fatal(1,"Held departure must count once");
 exitCompleteAsync=0;clocks(5);entryCompleteAsync=1;exitCompleteAsync=1;clocks(10);
 if(arrivals!=1||departures!=1)$fatal(1,"Conflicting directions must not count");
 reset=1;clocks(3);if(completionAck||arrived||departed)$fatal(1,"Board reset failed");
 $display("Adapter synchronizers, single pulses, stale hold, fault and reset passed");$finish;
 end
endmodule
