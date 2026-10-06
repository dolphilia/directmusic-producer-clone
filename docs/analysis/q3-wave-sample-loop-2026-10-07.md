# Wave default WSMP loop bounded closure

Candidate 20261006T154607013Z, 194 saved source files. Configure/build/install exit0; Producer SHA256 b232c90206c224b2edbb7582f032209977a22536a54fcfec07e689d85ec43ab4; native SHA256 c6becf1db0f889c4858ee6d39944272196dea5cc3d832ca487122e02b14cc885; build-summary SHA256 857a66ed78f39462035417be5949140141fb225984e0b9fa1630f99d92e96b64. Full acceptance false; all40/all8 incomplete.

Owned PCM Wave now supports WSMP forward/release loop type, start+length, multiple-loop apply, exact selected-record removal, Disable and UndoRedo through shared WaveDocument/main Wave Documents. DWORD values and 64-bit bounds are checked before mutation; no-op/invalid preserve bytes dirty and Redo. Extended header/records/tail and all unrelated PCM/SMPL/identity/metadata are preserved. Contract: [wave-sample-loop-contract](wave-sample-loop-contract.md).

Dedicated --wave-sample-loop:22 checks passed. The existing --wave-document replacement Windows error5 remains blocked without unchanged retry. This new mode creates fresh outputs and is separately registered. Fixed milestone75 native:45pass30blocked;107 drivers:20pass21blocked66unexecuted. No blocked/unexecuted counts as pass, no earlier candidate deployment or audio credit.

Main author PID16844:2 loops→add forward loop start0/length100→3, Undo saved2, Redo modified3, save WVP and native Project, normalexit0. SaveProject in another directory correctly refused by existing unimplemented native relocation restriction; failure retained. Saving LoopedCow.wvp inside original Project directory and then saving native WaveSampleLoops.pro succeeded. Separate PID1904 restored saved3, original loop1 1000/5000 and added loop3 0/100, normalexit0. Independent audit confirms only WSMP change, all other raw chunks exact and native Project owns LoopedCow.wvp. Reload inputs unchanged. This audit is not original dynamic comparison or audio.

Original Wave component currently unavailable; event waih loop enable/end encoding, SMPL editing, actual default-loop inheritance/track/region override and WASAPI loop behavior remain unverified. Standalone Project has no Segment/Play route; no audio pass claimed. Q2 independent original-free Windows unavailable; producer-specific dependency closure incomplete.

Preserved failed build153610308 (C++20 default comparison in C++17 project), corrected explicit operator;154607013 includes exact selected-removal fix. Intermediate successful153802348/154243313 have no transferable test credit.

Latest unit and reproducible commands: [unit-record](../../work/analysis/q3-wave-sample-loop/20261006T153219Z/unit-record.json). New full round [native](../../work/acceptance/regression/20261006T155017607Z/run.json), [drivers](../../work/acceptance/registered-drivers/20261006T155015293Z/run.json).

Next priority: keep154607013 fixed and connect Q1 fresh native five-document main author/save/exit/reload/Play Stop restart/tempo automated WASAPI. More Wave detail remains queued; do not use native/GUI limited closure as complete Wave or global acceptance.
