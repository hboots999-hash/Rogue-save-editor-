// Diagnostic only: deliberately include all three branch APIs in a single TU.
// Failure is a release blocker, not an excuse to patch upstream branches here.
#include "phase19/read_only_import.hpp"
#include "phase18/editor_model.hpp"
#include "recovery_evidence_gate.hpp"
int main(){return phase14::LiveWritesEnabled?1:0;}
