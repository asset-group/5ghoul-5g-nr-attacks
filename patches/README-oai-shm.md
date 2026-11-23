# OAI SHM patch (v3.0.0)

`patches/oai-shm-v3.0.0.patch` adds the 5Ghoul shared-memory interface to a
vanilla OpenAirInterface checkout at tag **v3.0.0** (`6f89078`).

Apply it with:

```bash
./scripts/apply_oai_shm_patch.sh <path-to-oai-v3.0.0-checkout>
```

The script refuses to patch anything whose `git describe --tags` is not
`v3.0.0*`, creates the `shm_interface` symlink into
`src/drivers/shm_interface`, and then `git apply`s the patch.

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
| `CMakeLists.txt` | `wd_shm.c` added to the `nr-softmodem` and `nr-uesoftmodem` source lists |
| `executables/nr-softmodem.c` | include + `shm_init()` / `shm_set_max_timeout()` / `shm_mq_init(W_MQ_MAC_DL)` in `main()` |
| `executables/nr-uesoftmodem.c` | same |
| `openair2/LAYER2/nr_pdcp/nr_pdcp_entity.c` | include + three interception hooks: RX PDCP PDU (`nr_pdcp_entity_recv_pdu`), TX plaintext (`nr_pdcp_entity_process_sdu`, MAC-I cleared), TX post-security |

## What is NOT in the patch

The 2023.w03 patch also instrumented the MAC and PHY for packet injection and
observation. Those hunks could not be ported to v3.0.0 mechanically, because
the anchor functions/structures no longer exist upstream:

| 2023.w03 anchor | v3.0.0 status |
| --- | --- |
| `nr_store_dlsch_buffer()` | renamed/refactored away |
| `pf_dl()` | gone |
| `dl_lc_ids`, `dl_lc_num` | removed from `NR_sched_ctrl_t` |
| `harq->transportBlock` (array) | now a struct with `.buf`, allocated via `allocate_transportBlock_buffer()` |
| `nr_schedule_ra()`, `nr_schedule_sibs()`, `nr_schedule_uci()` | renamed |
| `nr_rx_prach()`, `nr_phy_data_tx()`, `nr_rx_pucch()` | renamed |
| `nr_pdcp_entity_data_req()` | replaced by the `process_sdu` hook (ported) |
| `nr_pdcp_entity_recv_sdu()` | replaced by `nr_pdcp_entity_process_sdu()` (ported) |
| `dl_lc` hold hunk | lost: the structure it patched is gone in v3.0.0 |

The czmq/json "duplication" feature of the old patch is also omitted: czmq is
not a dependency of this repository and those hunks were already commented out
in the original patch.

Consequence: the resulting binary exposes and initialises the SHM interface and
captures PDCP traffic, but does **not** yet inject fuzzed MAC transport blocks.
Porting the MAC/PHY injection sites is a separate task.

## Validation

* applies to a pristine v3.0.0 checkout and to a `--depth 1 --branch v3.0.0`
  clone with 0 rejects / 0 fuzz (`git apply --check` then apply);
* builds `nr-softmodem` and `nr-uesoftmodem` (12,161 targets, RelWithDebInfo,
  GCC 15.2, `-DAUTO_DOWNLOAD_ASN1C=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5`)
  with `shm_init`, `shm_mq_init`, `shm_mq_recv`, `shm_mq_available` and
  `shm_mq_thread_handler` present in both binaries;
* a patched gNB run prints `[GNB_APP] [SHM] SHM not started` when no 5Ghoul
  server owns the segment, and then behaves exactly like an unpatched build of
  the same tree.

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
| 8 | `nr_ue_scheduler` | `nr_ue_ul_scheduler()` at the UL TX fill | `W_UE_UL_PDU_WITH_DATA` | capture (UE UL) |
| 9 | `nr_pdcp_entity_recv_pdu` | same function | `W_GNB_PDCP_PLAIN_UL` | capture (PDCP UL) |
| 10 | `nr_pdcp_entity_recv_sdu` | `nr_pdcp_entity_process_sdu()` (plain + post-security) | `W_GNB_PDCP_PLAIN_DL`, `W_GNB_PDCP_ENC_DL` | capture (PDCP DL) |

`send_pdu_data_nr()` blocks after notifying, then copies the shared-memory
payload back into the caller's buffer, which is how fuzzer-side mutations reach
the gNB/UE (points 3, 4, 5, 6, 7, 8). Points 9 and 10 use the same mechanism on
the PDCP queues.

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
  clone, 0 rejects / 0 fuzz, and the eight patched files are byte-identical to
  the tree the binaries were built from;
* builds `nr-softmodem`, `nr-uesoftmodem`, `params_libconfig` and `rfsimulator`
  (RelWithDebInfo, GCC 15.2) with all SHM symbols in both binaries;
* the gNB starts, prints `[SHM] SHM not started` when no 5Ghoul server owns the
  segment, and reaches the rfsimulator stage exactly like an unpatched build.

Runtime validation through the 5Ghoul fuzzer could NOT be completed on this
host: `bin/5g_fuzzer` fails its runtime module compilation
(`src/ModulesInclude.hpp:108: 'string' does not name a type`, so 12/12 exploit
modules fail to build and the fuzzer exits 1). That is a defect in the
5Ghoul tooling, independent of this patch; the port is therefore validated at
build level and at gNB-startup level only.

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
