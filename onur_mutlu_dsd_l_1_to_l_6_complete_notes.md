# Digital System Design – Exhaustive Notes (L1–L6)

> Deep, implementation-driven notes inspired by Onur Mutlu (Spring 2025). Includes design flows, waveforms (ASCII), SystemVerilog + testbenches, timing equations, and interview insights.

---

# L1: Fundamentals, Transistors, Gates

## Digital vs Analog
- Digital uses discrete levels (0/1) → robust to noise
- Noise margin: ability to tolerate voltage variations

## Abstraction Stack
Devices → Circuits → Gates → RTL → Microarchitecture → ISA → Software
- Contract: each layer assumes correctness of lower layer

## MOSFET Operation (CMOS)
- NMOS: strong 0, weak 1; conducts when Vgs > Vt
- PMOS: strong 1, weak 0; conducts when Vgs < -Vt

### CMOS Inverter
- Pull-up (PMOS) + Pull-down (NMOS)
- No direct VDD–GND path in steady state → low static power

#### Truth/Behavior
```
Vin  Vout
 0    1
 1    0
```

## Gate Construction (Conceptual)
- AND via series NMOS (pull-down), parallel PMOS (pull-up)
- OR via parallel NMOS, series PMOS
- NAND/NOR: natural in CMOS → fewer transistors

## Delay & Power (intro)
- Propagation delay ~ RC of network
- Dynamic power: P ≈ α C V^2 f

## Interview Insights
- Why NAND is preferred? (area, speed in CMOS)
- Tradeoff: delay vs power vs area

---

# L2: Combinational Logic

## Definition
Output = f(inputs), no memory

## Design Flow (strict)
1. Spec → inputs/outputs, constraints
2. Truth table
3. Boolean equation (SOP/POS)
4. Minimize (algebra/K-map)
5. Map to gates / RTL
6. Verify (sim + corner cases)

## K-Map (3/4 vars)
- Group sizes: 1,2,4,8...
- Wrap-around adjacency
- Don’t-care usage

## Hazards/Glitches
- Static-1, Static-0, Dynamic hazards
- Cause: unequal path delays
- Fix: add consensus terms or balance paths

### Example: 2:1 MUX
Equation: Y = S'·A + S·B

```systemverilog
module mux2 (
  input  logic a, b, s,
  output logic y
);
  assign y = (~s & a) | (s & b);
endmodule
```

### Testbench
```systemverilog
module tb_mux2;
  logic a,b,s,y;
  mux2 dut(.a(a),.b(b),.s(s),.y(y));
  initial begin
    for (int i=0;i<8;i++) begin
      {a,b,s} = i; #1;
    end
    $finish;
  end
endmodule
```

## Timing (intro)
- tpHL, tpLH
- Worst-case path defines speed

## Interview
- Difference SOP vs POS
- How to remove hazards?

---

# L3: Sequential Logic

## Definition
Output = f(input, state)
State stored in memory elements

## Latch vs Flip-Flop
- Latch: level-sensitive (transparent)
- FF: edge-triggered (posedge/negedge)

## D Flip-Flop
```
Q(next) = D @ clk edge
```

### Waveform (ASCII)
```
clk:  _/‾\_/‾\_/‾\_
D:    0 1  0  1  1
Q:    0 1  0  1  1  (updates at edges)
```

### RTL
```systemverilog
module dff(
  input  logic clk, rst, d,
  output logic q
);
  always_ff @(posedge clk or posedge rst) begin
    if (rst) q <= 0;
    else     q <= d;
  end
endmodule
```

## Registers & Counters
- Register = vector of FFs
- Counter: increments per clock

## FSM
- Moore: output=f(state)
- Mealy: output=f(state,input)

### FSM Example (Moore)
```systemverilog
typedef enum logic [1:0] {S0,S1,S2} state_t;
state_t state, next;

always_ff @(posedge clk) state <= next;

always_comb begin
  next = state;
  case(state)
    S0: next = S1;
    S1: next = S2;
    S2: next = S0;
  endcase
end
```

## Interview
- Why FF over latch in synchronous design?

---

# L4: Sequential II, Labs, Verilog

## Coding Styles
- always_ff → sequential
- always_comb → combinational

## Blocking vs Non-blocking
- `=`: immediate (combinational style)
- `<=`: scheduled (sequential)

## Race Conditions
- Avoid mixing blocking in sequential blocks

## FSM Full Template
```systemverilog
always_ff @(posedge clk) state <= next;

always_comb begin
  next = state;
  // transitions
end

always_comb begin
  // outputs
end
```

## Lab Focus
- Waveform debugging
- Assertions (basic)

## Interview
- Difference between simulation and synthesis?

---

# L5: HDL, Verilog II, Timing & Verification

## RTL vs Gate-level
- RTL: behavior
- Gate: actual mapped hardware

## Timing Parameters
- t_pd: propagation delay
- t_setup: data stable before clk
- t_hold: data stable after clk

### Constraint
```
Tclk ≥ t_clk→Q + t_logic + t_setup
```

## Setup/Hold Violations
- Setup fail → wrong value captured
- Hold fail → metastability risk

## Testbench Structure
```systemverilog
initial begin
  reset();
  apply_inputs();
  check_outputs();
end
```

## Assertions (example)
```systemverilog
assert property (@(posedge clk) disable iff(rst) req |-> ##1 gnt);
```

## Interview
- How to detect timing violations?

---

# L6: Timing & Verification II

## Clock Issues
- Skew: different arrival times
- Jitter: variation in period

## Metastability
- Occurs when setup/hold violated
- Output undefined for some time

### Mitigation
- 2-FF synchronizer

```systemverilog
always_ff @(posedge clk) begin
  sync1 <= async_in;
  sync2 <= sync1;
end
```

## Critical Path
- Longest combinational delay path
- Determines max frequency

### Max Frequency
```
f_max = 1 / Tclk
```

## Pipelining
- Break long paths with registers
- Increases throughput, adds latency

## Verification Advanced
- Functional coverage
- Code coverage
- Formal methods (model checking)

## Interview
- Explain metastability clearly
- How pipelining improves performance?

---

# End-to-End Design Example (Mini)

## Spec
Design a 4-bit up counter with enable + sync reset

```systemverilog
module counter(
  input  logic clk, rst, en,
  output logic [3:0] q
);
  always_ff @(posedge clk) begin
    if (rst) q <= 0;
    else if (en) q <= q + 1;
  end
endmodule
```

### Testbench
```systemverilog
module tb;
  logic clk=0, rst=1, en=0;
  logic [3:0] q;
  counter dut(clk,rst,en,q);

  always #5 clk = ~clk;

  initial begin
    #10 rst=0; en=1;
    #100 $finish;
  end
endmodule
```

---

# Mastery Checklist
- [ ] Can derive K-map quickly
- [ ] Can design FSM from spec
- [ ] Can write synthesizable SV
- [ ] Understand setup/hold deeply
- [ ] Can debug waveforms

---

# Final Insight

> Digital design mastery = logic + timing + verification. Ignore any one → system fails.

