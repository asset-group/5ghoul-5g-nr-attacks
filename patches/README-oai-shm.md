# OAI SHM patch (v3.0.0)

`patches/oai-shm-v3.0.0.patch` adds the 5Ghoul shared-memory interface to a
vanilla OpenAirInterface checkout at tag **v3.0.0** (`6f89078`).

The normal installation/build entry points are:

```bash
./requirements.sh
./build.sh
```

The internal `scripts/build_oai_shm.sh` helper builds the exact v3.0.0 source,
its pinned ASN1 compiler and SIMDe headers, then packages the gated binaries
and modules under `.deps/oai-shm/bin`. It calls
`scripts/apply_oai_shm_patch.sh`, which requires the exact tag and full commit
`6f89078b409e671fa566782f67aae9de52fd55d8`, accepts vanilla or exactly patched
sources, and rejects conflicting tracked/staged edits before changing the
driver symlink. Repeated installation is incremental and does not discard
local files. No `/opt` installation is used or replaced.

## Why this is not `3rd-party/oai_5g_sa.patch`

`scripts/apply_patches.sh` derives the target submodule from the patch
filename (`<folder>/<name>.patch` is applied inside submodule `<folder>/<name>`).
`3rd-party/oai_5g_sa.patch` therefore belongs to the vendored
`3rd-party/oai_5g_sa` submodule, which is pinned at **2023.w03**. That patch is
the original 5Ghoul OAI patch (61 hunks, 26 files) and still applies to that
submodule only. Overwriting it with a v3.0.0 diff would silently break the
submodule build, so the v3.0.0 patch lives in `patches/` with its own entry
point instead.

## What the patch contains

| File | Change |
| --- | --- |
| `CMakeLists.txt` | `wd_shm.c` added to the `nr-softmodem` source list only, plus `target_compile_definitions(NR_L2_UE PRIVATE WD_SHM_GNB_ONLY)` so the shared PDCP hooks compile out of the UE build |
| `executables/nr-softmodem.c` | include + `shm_init()` / `shm_set_max_timeout()` / `shm_mq_init(W_MQ_MAC_DL)` in `main()` |
| `openair2/LAYER2/nr_pdcp/nr_pdcp_entity.c` | include + three interception hooks, all inside `#ifndef WD_SHM_GNB_ONLY` so only the gNB build compiles them: RX PDCP PDU (`nr_pdcp_entity_recv_pdu`), TX plaintext (`nr_pdcp_entity_process_sdu`, MAC-I cleared), TX post-security |

The table above lists only the build wiring and the PDCP hooks. The MAC DL/UL,
RAR, Msg4 and Msg3 points ported on top of them are listed in
*Interception points* further down.

## What is NOT in the patch

Everything the 2023.w03 patch instrumented on the MAC and PHY has a v3.0.0
equivalent and is now ported there (see *Interception points* below); no point
was dropped because its original anchor disappeared. What remains out of scope:

| Item | Why |
| --- | --- |
| `fuzz_nr_dup` duplication feature (czmq/json-c hunks) | not an interception point, and czmq is not a dependency of this tree; those hunks were already commented out in 2023.w03 |
| `remove_nr_list()` `last == NULL` guard | robustness fix for list corruption, not interception; v3.0.0's list code differs, so it was not carried over |
| MIB / SIB1 / RA-initiation broadcast hooks | anchors exist in v3.0.0 but the calls are inert in the 2023.w03 reference (commented out upstream); enabling them is optional broadcast-channel work |
| Mac/PHY `pf_dl` hold | commented out in the reference; the DL hold is implemented in `update_dlsch_buffer()` instead |

## Interception points

Every ACTIVE interception point of `3rd-party/oai_5g_sa.patch` (2023.w03), and
where it lives in v3.0.0. The event ids, queues and framing are unchanged, so
the 5Ghoul side needs no modification.

| # | 2023.w03 anchor | v3.0.0 site | SHM event / queue | kind |
|---|-----------------|-------------|-------------------|------|
| 1 | `nr_store_dlsch_buffer` | `update_dlsch_buffer()` (gNB_scheduler_dlsch.c) | `W_MQ_MAC_DL` availability | hold |
| 2 | `nr_schedule_ue_spec` LC loop | `generate_dl_mac_pdu()` RLC-data gate | `W_MQ_MAC_DL` availability | hold |
| 3 | `nr_schedule_ue_spec` TB | `generate_dl_mac_pdu()` before `T(T_GNB_MAC_DL_PDU_WITH_DATA)` | `W_GNB_MAC_UE_DL_PDU_WITH_DATA` (+`W_INJECTED`) | injection + capture |
| 4 | `nr_generate_Msg2` | `nr_generate_Msg2()` before `nr_add_msg3()` | `W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA` | capture (RAR) |
| 5 | `nr_generate_Msg4` | `nr_generate_Msg4_MsgB()` before `T(...)` | `W_GNB_MAC_UE_DL_PDU_WITH_DATA` | capture (Msg4/MsgB) |
| 6 | `nr_rx_sdu` (connected UE) | `_nr_rx_sdu()` | `W_GNB_MAC_UE_UL_PDU_WITH_DATA` | capture (UL) |
| 7 | `nr_rx_sdu` (RA path) | `nr_rx_ra_sdu()` before its `T(...)` | `W_GNB_MAC_UE_UL_PDU_WITH_DATA` | capture (Msg3) |
| 8 | `nr_pdcp_entity_recv_pdu` | same function | `W_GNB_PDCP_PLAIN_UL` | capture (PDCP UL) |
| 9 | `nr_pdcp_entity_recv_sdu` | `nr_pdcp_entity_process_sdu()` (plain + post-security) | `W_GNB_PDCP_PLAIN_DL`, `W_GNB_PDCP_ENC_DL` | capture (PDCP DL) |

`send_pdu_data_nr()` blocks after notifying, then copies the shared-memory
payload back into the caller's buffer, which is how fuzzer-side mutations reach
the gNB (points 3, 4, 5, 6, 7). Points 8 and 9 use the same mechanism on the
PDCP queues.

### Removed by design: the UE-side UL capture point

The port originally mirrored 2023.w03 point 8, `nr_ue_ul_scheduler()` in
`nr_ue_scheduler.c` (`W_UE_UL_PDU_WITH_DATA`, evt 210), and also initialised the
segment from `nr-uesoftmodem.c`. That is dead code: the fuzzer owns the segment
for the gNB process only, so in every run the UE logged `[SHM] SHM not started`
while the gNB logged `[SHM] SHM started`. UE-originated uplink is captured at the
gNB side instead, in `_nr_rx_sdu()` and `nr_rx_ra_sdu()` (points 6 and 7,
`W_GNB_MAC_UE_UL_PDU_WITH_DATA`, evt 205) - 6,900 / 18,060 / 17,230 PDUs in the
380 s and 120 s validation runs - which is where the fuzzer can act on it.

The hook and the UE-side `shm_init()`/`shm_set_max_timeout()`/`shm_mq_init()`
were dropped, with their includes, so `executables/nr-uesoftmodem.c` and
`openair2/LAYER2/NR_MAC_UE/nr_ue_scheduler.c` are identical to vanilla v3.0.0 and
no longer appear in the patch.

The UE build also carries no SHM code at all, enforced at compile time rather
than left to a runtime flag:

* `wd_shm.c` is gone from the `nr-uesoftmodem` source list, so the UE binary no
  longer links the driver;
* the `wd_shm_nr_utils.h` include and the three PDCP hook call sites in the
  shared `nr_pdcp_entity.c` sit inside `#ifndef WD_SHM_GNB_ONLY`, and
  `WD_SHM_GNB_ONLY` is defined on `NR_L2_UE`, the UE build of the L2 sources.
  `L2_NR`, the gNB build of the same file, defines nothing and keeps the hooks
  exactly as they were. The hooks were already gated at runtime on
  `entity->is_gnb`, so this removes only code the UE could never execute.

Verified on this tree: `nm nr-uesoftmodem` has zero `shm_*`/`wd_shm` symbols,
defined or undefined, and `objdump -d` finds no call to `shm_init`;
`nr-softmodem` keeps all 12 and its single `shm_init()` call. Sizes: UE
48,902,008 -> 48,848,768 B, gNB 131,171,040 B unchanged. A 100 s run with
mutation on still reports `[SHM] SHM started` on the gNB side, zero `[SHM]`
lines on the UE side, Registration Accept, PDN established, 11 mutated PDUs and
no UE abort.

### Receiver robustness: CCCH length check (not an interception point)

`openair2/LAYER2/NR_MAC_UE/nr_ue_procedures.c`, CCCH branch of
`nr_ue_process_mac_pdu()`, used to abort the whole UE on any MAC PDU whose CCCH
subheader declared a length that does not fit the received PDU:

```c
ret = get_mac_len(pduP, pdu_len, &mac_len, &mac_subheader_len);
AssertFatal(ret, "The mac_len (%d) has an invalid size. PDU len = %d! \n", ...);
```

A mutated Msg4 / RRC Setup reaches exactly this check, so the UE process died and
had to be restarted, ending the iteration and hiding the effect of the mutation.
The patch replaces the assert with a log-and-drop mirroring the "Invalid CCCH
message" path a few lines below (`done = 1; break;`).

Observed with mutation on: 2 such PDUs dropped in a 120 s run and 0 UE aborts,
against 1 abort at this exact assertion in the same setup before the fix.

This is a robustness change, not an interception point: it adds no SHM traffic
and changes no framing. It exists only because the fuzzer sends malformed MAC
PDUs on purpose.

### Present in 2023.w03 but not interception points

* `W_GNB_PHY_MIB` (`nr_common_signal_procedures`), `W_GNB_MAC_UE_DL_SIB`
  (`schedule_nr_sib1`) and `W_GNB_PHY_INITIATE_RA_PROCEDURE`
  (`L1_nr_prach_procedures`) exist in the 2023.w03 patch **commented out** and
  never executed, so the fuzzer never received them. All three anchors still
  exist in v3.0.0 and can be enabled later if broadcast-channel fuzzing is
  wanted.
* The `fuzz_nr_dup` duplication feature (hunks 19/21/23/25/26 plus the
  `fuzz_duplication.py` helper) needs czmq and an external duplication source;
  it is inactive in 2023.w03 at the sites that matter and is not part of the
  mutation path. Not ported.
* The czmq/json-c CMake hunks are build plumbing, not interception points; the
  SHM code in `wd_shm.c` does not use either library.

## Validation

* applies to a pristine v3.0.0 checkout and to a `--depth 1 --branch v3.0.0`
  clone, 0 rejects / 0 fuzz, and the seven patched files are byte-identical to
  the tree the binaries were built from;
* builds `nr-softmodem`, `nr-uesoftmodem`, `params_libconfig` and `rfsimulator`
  (RelWithDebInfo, GCC 15.2) with all SHM symbols in both binaries;
* the gNB starts, prints `[SHM] SHM not started` when no 5Ghoul server owns the
  segment, and reaches the rfsimulator stage exactly like an unpatched build.

Runtime validation through the 5Ghoul fuzzer now succeeds end to end.
Conditions on this WSL host: bundled 5GC rebuilt in place (see above),
`netconf.sh` run, mongod up, `BASICSIM_DELAY_US=2000`, both binaries under
`prlimit --memlock=unlimited:unlimited`, mutation on at P=0.2 / 2 fields.

| Run | Duration | MAC DL | MAC UL | PDCP DL | PDCP UL | Mutated | Attach | UE aborts |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| B | 380 s | n/m | 6,900 | - | - | 7 | 1/2 RRC, 2/2 PDN | 0 |
| C | 120 s | 150 | 18,060 | - | - | 20 | 1/2 RRC, 2/2 PDN | 0 |
| D | 120 s | 70 | 17,230 | 20 | 15 | 9 | none, mutated PDU failed integrity | 0 |

`n/m` = not measured in that run, `-` = that block was not reached. The counts
come from a temporary counter inside `send_pdu_data_nr()` /
`send_pdu_data_pdcp_nr()` (printed every 10 MAC / 5 PDCP round trips), removed
again afterwards; the committed patch carries no counters.

Mutated PDUs seen on the DL MAC channel: RLC AM SRB:1 DATA and CONTROL PDUs,
`DL-SCH` padding, an LCID 4 subPDU, `DL Information Transfer` Authentication
request / Security mode command / Registration accept. They reach the UE and are
acted on: in run D the UE logged `discard NR PDU, integrity failed` twice and did
not complete the attach, which is the expected result of mutating a protected
PDU. No run produced a UE abort or an `Open5GS stopped` restart.

RAR/Msg4 and MIB/SIB1 are not in the table: the RAR hook fires only a handful of
times per attach (below the counter's print threshold), and the MIB/SIB1/RA
initiation hooks are the inert ones listed above.

### Bundled 5GC: rebuild it in place after the tree moves

`3rd-party/open5gs-core/build/tests/app/5gc` bakes the absolute meson build
prefix at configure time and execs every NF from
`<MESON_BUILD_ROOT>/src/<nf>/open5gs-<nf>d`. If the 5Ghoul tree was copied or
moved after that build, each NF exec fails before it can log anything and `5gc`
aborts with `child_main: Assertion 'out_return_code == 0' failed`. The fuzzer
shows this as an endless `[!] Open5GS stopped` loop with no AMF, so nothing ever
attaches. Rebuild the core at the current path:

```bash
cd 3rd-party/open5gs-core
mv build build.stale-issue46            # keep the old build aside
export LDFLAGS="-Wl,-rpath=\$ORIGIN:\$ORIGIN/../libs/x86_64-linux-gnu"
meson setup build --prefix="$PWD/install" --wrap-mode=nodownload
ninja -C build
```

Also make sure no NF from an earlier aborted run is still alive
(`pkill -9 -f 'open5gs-.*d -c'`): the orphans hold the SBI ports, the next start
fails on bind, and that looks like the same abort.

Two 5Ghoul-side behaviours that also stop a run before it starts:

* the fuzzer rebuilds its exploit modules at start-up and kills a `g++`
  invocation that exceeds its 5 s `ProcessExec` default; on a loaded machine one
  module then fails to link at random while the same command succeeds by hand.
  Pre-build the modules (or make the `.so` newer than the `.cpp`) before running;
* `src/ModulesInclude.hpp` needs `<string>` and `std::string` (fixed on this
  branch) or all 14 modules fail to compile.

### Running the binaries: the memlock limit

OAI locks its memory with `mlockall(MCL_CURRENT | MCL_FUTURE)`
(`common/utils/system.c`). Under WSL the default `RLIMIT_MEMLOCK` is 64 MiB, so
as soon as the process' locked footprint crosses 64 MiB every further
allocation fails and the gNB aborts with

```
Assertion (ptr) failed!
In config_allocate_new() .../common/config/config_common.c:90
calloc fails
```

during `Initializing NR L1`. Raise the limit before running:

```bash
sudo prlimit --memlock=unlimited:unlimited -- \
     ./build/nr-softmodem --rfsim -O ../5ghoul/configs/5gnr_gnb/rfsim.n78.106.conf
```

Build the radio module too, otherwise startup fails with
`library librfsimulator.so is not loaded`:

```bash
ninja nr-softmodem nr-uesoftmodem params_libconfig rfsimulator
```
