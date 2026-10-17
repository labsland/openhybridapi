// Supplied transport wrapper, not the assessed Lab 6 controller.
// Board reset is independent of browser Restart activity.
module airport_garage_adapter (
 input logic clock, reset,
 input logic entryWaitingAsync, exitWaitingAsync,
 input logic entryCompleteAsync, exitCompleteAsync, worldReadyAsync,
 output logic entryWaiting, exitWaiting, ready,
 output logic arrived, departed, completionAck
);
 (* ASYNC_REG="TRUE" *) logic [4:0] sync1, sync2;
 logic waitingForClear;
 always_ff @(posedge clock or posedge reset) begin
  if(reset) begin sync1<='0;sync2<='0;end
  else begin
   sync1<={worldReadyAsync,exitCompleteAsync,entryCompleteAsync,exitWaitingAsync,entryWaitingAsync};
   sync2<=sync1;
  end
 end
 assign entryWaiting=sync2[0];
 assign exitWaiting=sync2[1];
 assign ready=sync2[4];
 always_ff @(posedge clock or posedge reset) begin
  if(reset)begin arrived<=0;departed<=0;completionAck<=0;waitingForClear<=0;end
  else begin
   arrived<=0;departed<=0;
   if(ready)begin
    if(!waitingForClear && (sync2[2]||sync2[3]))begin
     // Simultaneous directions are a protocol fault. Flush without counting.
     arrived<=sync2[2]&&!sync2[3];departed<=sync2[3]&&!sync2[2];
     completionAck<=1;waitingForClear<=1;
    end else if(waitingForClear&&!sync2[2]&&!sync2[3])begin
     completionAck<=0;waitingForClear<=0;
    end
   end
  end
 end
endmodule
