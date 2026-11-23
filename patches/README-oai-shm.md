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

### Known pre-existing issue (not caused by this patch)

A v3.0.0 build produced by the plain CMake+GCC 15.2 route above aborts during
`Initializing NR L1` with `Assertion (ptr) failed! ... calloc fails`, while the
prebuilt `/opt/oai-latest/bin` binaries (GCC 11.4) get further. A controlled
A/B in the same tree, same build dir, same flags and same config shows the
**unpatched** build failing at exactly the same point, so the fault is in the
build method/toolchain, not in this patch. Max RSS at the abort is only 65 MB,
so it is not memory exhaustion. Resolve (older GCC / different flags) before
using a self-built binary for fuzzing.
