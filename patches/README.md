# RTNL-Stalls in CAN_RAW, CAN_ISOTP und CAN_BCM

Mainline-Patches, keine Stable-Backports. Sie wenden per `git am` auf den
CAN-Code dieses Repositorys an (`main`, "Update to Linux 7.4-rc3+ CAN
code").

## Patches 1 bis 7: Serie v7 (Fixes, für stable markiert)

"[PATCH v7 0/8] can: avoid RTNL stalls and fix CAN_ISOTP and netlink
issues", unverändert die mit b4 erzeugten Mails vom 20.11.2026 (lokale
Ausgabe, die Message-IDs der verschickten Serie können abweichen). Die
Serie ersetzt die acht Patches der v6 im Pull-Request "pull-request: can
2026-11-17" von Marc Kleine-Budde.

Hier sind nur die CAN-Netlayer-Patches 1 bis 7 enthalten. Patch 8 der
Serie (Zhu Ling, "can: netlink: fix static FD-NON-ISO mode validation")
ändert `drivers/net/can/dev/netlink.c` und gehört nicht dazu.

| # | Patch | Autor | Fixes | gegenüber v6 |
|---|---|---|---|---|
| 1 | can: bcm: copy the CAN frames outside of lock_sock() | Oliver Hartkopp | `ffd980f976e7` | unverändert |
| 2 | can: bcm: do not remove the procfs entry in bcm_notify() | Oliver Hartkopp | `76fe372ccb81` | unverändert |
| 3 | can: raw: use a private mutex instead of lock_sock() | Oliver Hartkopp | `c18ce101f2e4` | unverändert |
| 4 | can: isotp: use a private mutex instead of lock_sock() | Oliver Hartkopp | `e057dd3fc20f` | unverändert |
| 5 | can: bcm: use a private mutex instead of lock_sock() | Oliver Hartkopp | `ffd980f976e7` | unverändert |
| 6 | can: isotp: always start the rx timeout in ISOTP_WAIT_DATA state | Oliver Hartkopp | `e057dd3fc20f` | Code und Text |
| 7 | can: isotp: reject socket options while a transfer is active | Oliver Hartkopp | `e057dd3fc20f` | Code und Text |

- **Patch 1 bis 5** vermeiden RTNL-Stalls in CAN_BCM bzw. ersetzen
  `lock_sock()` durch den Mutex `conf_lock`, damit die generischen
  `SOL_SOCKET`-Optionen die Notifier nicht mehr unter RTNL aufhalten
  können. Patch 3 und 4 brauchen den Commit 43c973a0887d, Patch 5 braucht
  Patch 1 und 2.
- **Patch 6** startet den Empfangs-Timeout in `isotp_rcv_ff()` und
  `isotp_rcv_cf()`, bevor ein Flow Control gesendet wird. Ohne ihn bleibt
  `so->rx.state` ohne Timer in `ISOTP_WAIT_DATA` stehen, wenn das Flow
  Control nicht gesendet werden kann oder im Listen-Mode gar nicht
  gesendet wird; nach dem Entfernen des Devices lehnt `isotp_bind()` den
  Socket dann dauerhaft mit `-EAGAIN` ab. Im Listen-Mode meldet ein First
  Frame ohne Folge-Frames jetzt `ETIMEDOUT`, wie eine Lücke mitten im
  Block es schon tut. Neu in v7 (nach dem Sashiko-Review zu v6,
  `Reported-by`/`Closes`): `isotp_rcv_cf()` prüft die Länge eines
  Consecutive Frame als Erstes, vor der Zustandsprüfung, dem
  `force_rx_stmin`-Block und `hrtimer_cancel()`. Ein ungültiger CF wird so
  verworfen, ohne den laufenden Timeout anzuhalten oder den Zeitstempel
  für `CAN_ISOTP_FORCE_RXSTMIN` zu setzen.
- **Patch 7** prüft in `isotp_setsockopt_locked()` zusätzlich zu
  `so->bound`, dass Sende- und Empfangszustand idle sind, wie
  `isotp_bind()` es schon tut. Neu in v7: `isotp_sendmsg()` liest
  `CAN_ISOTP_EXTEND_ADDR` und `CAN_ISOTP_WAIT_TX_DONE` erst nach der
  Belegung der Übertragung und der erneuten Prüfung von `so->bound`.
  Patch 6 muss vorher kommen, sonst würde ein hängender Empfangszustand
  zusätzlich `setsockopt()` sperren.

Die bisher als Folgepatch vorbereitete CF-Längenprüfung ist in Patch 6
aufgegangen.

## Basis

Die Patches wenden auf `main` dieses Repositorys an (CAN-Code aus
Mainline `20eef29b7c84`, 7.4-rc3+). Die Mails der v7 nennen als Basis
net/main `4d67cf2fd235`; in `can_mutex` sind die Patches 1 bis 11 gegen
Mainline `067c5c24ce` (10.11.2026) geprüft, die betroffenen Dateien sind
dort identisch.

## Prüfung

- Die Dateien 1 bis 7 wenden in dieser Reihenfolge per `git am` auf
  `main` an.
- `net/can/` übersetzt nach jedem der Patches 1 bis 7 fehler- und
  warnungsfrei (gcc 13.3, `W=1`, mit `CONFIG_PROVE_LOCKING`,
  `CONFIG_DEBUG_MUTEXES` und `CONFIG_PROC_FS`). Kein vollständiger
  Kernel-Build.
- Laufzeittest der v7 in `can_mutex` (20.11.2026, Test-Suite
  `can-lock-tests-mutex-v5`, short, lockdep und KASAN): auf
  `7.4.0-rc3-net-locktest-00448-gb4092438c4d1` bestehen alle 14 Tests,
  `debug_locks` danach 1. `isotp_state_test` hat zwei neue Fälle für
  ungültige Consecutive Frames (CAN CC zu kurz, CAN FD zu lang):

  | Kernel | Fall 1 | Fall 2 bis 4 | Fall 5 und 6 |
  |---|---|---|---|
  | ohne Patch 6 und 7 (`...-00141-g9dac799e47f8`) | FAIL | FAIL | FAIL |
  | v6 (`...-00143-gab966e98efcf`) | PASS | PASS | FAIL |
  | v7 (`...-net-locktest-00448-gb4092438c4d1`) | PASS | PASS | PASS |

- checkpatch meldet nichts außer den bei mir unbekannten Commit-IDs in
  den `Fixes:`-Zeilen.
