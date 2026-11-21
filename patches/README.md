# CAN-Patches: RTNL-Stalls, net-next-Folgepatches und CAN_ISOTP

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

## Patches 9 und 10: Folgepatches für net-next (kein Fix, nicht stable)

| # | Patch |
|---|---|
| 9 | can: raw: read socket options under conf_lock in raw_getsockopt() |
| 10 | can: isotp: read socket options under conf_lock in isotp_getsockopt() |

Beide lesen die Optionswerte unter `conf_lock` in eine lokale Variable und
kopieren sie erst nach `mutex_unlock()` in den User-Space. Sie setzen auf
Patch 3 bzw. 4 auf und stellen den Struct-Kommentar auf "sockopts".

## Patch 11: memcg-Abrechnung für net-next (zurückgestellt)

| # | Patch |
|---|---|
| 11 | can: enable memcg accounting for user triggered allocations |

Der für v5 vorbereitete Stand ohne Abrechnung des isotp-RX-Puffers in
`isotp_rcv_ff()` (dort nur ein Kommentar), auf Patch 10 aufgesetzt. Der
Code ist gegenüber der vorbereiteten Fassung unverändert, die
Changelog-Notizen unter dem `---` sind übernommen. Der Patch wird erst
verschickt, wenn die Fixes in net-next angekommen sind; Changelog und
Links sind dann anzupassen.

## Patches 12 bis 26: CAN_ISOTP für net-next (Fixes und Aufräum-Patches)

Reihenfolge: erst die Fixes, dann die Aufräum-Patches. Keiner der Patches
hat einen `Fixes:`-Tag, keiner ist für stable vorgesehen.

| # | Patch | bisher (`75_net_next`/`split`) |
|---|---|---|
| | **Fixes (API)** | |
| 12 | can: isotp: check the CAN_ISOTP_OPTS values before applying them | 22 |
| 13 | can: isotp: add the frame transmission time to the protocol timeouts | 23 |
| | **Fixes (Empfangspfad)** | |
| 14 | can: isotp: drop frames without N_PCI | 31 |
| 15 | can: isotp: drop FF frames with invalid length extension | 12 |
| 16 | can: isotp: check the minimum FF_DL | 13 + 30 + 33 |
| 17 | can: isotp: check the data length of the last CF | 32 |
| 18 | can: isotp: do not send FC.OVFLW in listen mode | 34 |
| | **Aufräum-Patches** | |
| 19 | can: isotp: drop inline keyword, let the compiler decide | 14 |
| 20 | can: isotp: rename rx_lock as is has become state machine lock | 15 |
| 21 | can: isotp: introduce isotp_invalid_canid() to verify proper CAN-IDs | 16 |
| 22 | can: isotp: rename variable for the required content length | 17 |
| 23 | can: isotp: rename checkpad functions returning bool values | 18 |
| 24 | can: isotp: change return type of functions to void | 19 |
| 25 | can: isotp: rename pointer in isotp_rcv() prototype | 20 |
| 26 | can: isotp: introduce CAN_ISOTP_MIN_TX_DL | 21 |

- **Patch 12** prüft die Werte von `CAN_ISOTP_OPTS` in der Kopie, die
  `isotp_setsockopt()` vor `conf_lock` anlegt, bevor sie nach `so->opt`
  kopiert werden. Mehr als ein Broadcast-Flag lehnt `hweight32()` mit
  `-EINVAL` ab, und ein abgelehnter Aufruf lässt alle Optionen
  unverändert. Bisher wurde bei SF|CF das CF-Flag verworfen, der Rest
  übernommen und trotzdem `-EINVAL` gemeldet.
- **Patch 13** addiert `frame_txtime` (N_As/N_Ar) auf den rx-Timeout nach
  dem Senden eines Flow Control in `isotp_send_fc()` und auf den Echo-
  bzw. FC-Timeout nach dem ersten Frame in `isotp_sendmsg()`, wie in
  `isotp.h` dokumentiert. Werte über 100 ms lehnt er wie Patch 12 vor dem
  Kopieren mit `-EINVAL` ab. Die Echo-Timeouts vor Consecutive Frames
  bleiben ohne `frame_txtime`, da das Echo lokal ist. Anlass ist die
  Diskussion zu Wentian Yes N_Ar-Patch (`Reported-by`, `Closes:` auf
  `eb5e9618-3dbf-42cb-a635-752ff9fbcb99@hartkopp.net`).
- **Patch 14** verwirft Frames mit CAN_DL < AE + N_PCI, die bisher mit
  Daten jenseits von CAN_DL als SF/CF/FC verarbeitet wurden (can-tests
  RX-SF-003..005, RX-CF-006).
- **Patch 15** verwirft First Frames mit unnötiger 32-Bit-Längenangabe
  (FF_DL <= 4095), wie ISO 15765-2:2016 es verlangt.
- **Patch 16** prüft FF_DL gegen FF_DLmin: Eine PDU, die in einen SF
  passt, wird nicht als FF angenommen (bisher z.B. FF_DL 7 bei LL_DL 8,
  RX-FF-004-CC8/-ae). Er entfernt die nur auf der Sendeseite nötige
  SF_DL-ESC-Offsetberechnung auf der Empfangsseite (bisher Patch 13). Da
  nur FF_DL links steht, läuft die Prüfung für FF_DL nahe 2^32 nicht über
  (bisher Patch 30, RX-FF-011/-ae).
- **Patch 17** verwirft einen kurzen CF, wenn die fehlenden Bytes der PDU
  seine mögliche Nutzlast übersteigen (RX-CF-014/015). Setzt Patch 14
  voraus.
- **Patch 18** verlegt die Listen-Mode-Prüfung an den Anfang von
  `isotp_send_fc()`. Bisher sendete ein Socket im Listen-Mode FC.OVFLW,
  wenn FF_DL den Empfangspuffer übersteigt (RX-FF-006).
- **Patch 19** entfernt das `inline` bei `isotp_sk()`.
- **Patch 20** benennt `rx_lock` in `sm_lock` um, da der Spinlock die
  ganze Zustandsmaschine schützt. Der Mutex `conf_lock` aus Patch 4 bleibt
  unverändert.
- **Patch 21 bis 26** sind Aufräum-Patches ohne funktionale Änderung:
  gemeinsame CAN-ID-Prüfung in `isotp_bind()`, `num` heißt `reqlen`,
  `chk_pad_fail()`/`chk_optimized_fail()` liefern `bool`, `void` statt
  ungenutzter Rückgabewerte von `isotp_send_fc()` und `isotp_rcv_*()`,
  `data` heißt `skdata` in `isotp_rcv()`, eigenes Define
  `CAN_ISOTP_MIN_TX_DL` für TX_DL-Prüfungen statt `CAN_MAX_DLEN`.

Die Patches setzen auf Patch 11 auf. Anpassungen gegenüber `75_net_next`
und `split`:

- Patch 14, 16, 17 und 18 sind auf `net/can/isotp.c` vor der Aufteilung
  zurückportiert (`cf->len` statt `datalen`, `return 1`/`return 0` bis
  Patch 24).
- Patch 16 nutzt `CAN_MAX_DLEN` und `SF_PCI_SZ8`; Patch 26 stellt auf
  `CAN_ISOTP_MIN_TX_DL` um, der CAN-XL-Patch 31 auf `SF_PCI_SZ11`.
- Patch 18 entfernt die Listen-Mode-Prüfungen in `isotp_rcv_ff()` und
  `isotp_rcv_cf()`; Patch 24 enthält sie daher nicht mehr.
- In `isotp_rcv_cf()` prüft Patch 6 die CF-Länge als Erstes und startet
  den Empfangs-Timeout schon vor der Blocksize-Prüfung. Patch 24 behält
  beides bei und entfernt nur die Rückgabewerte.

## Patches 27 bis 32: CAN_ISOTP-Aufteilung und CAN XL

| # | Patch |
|---|---|
| 27 | can: isotp: move isotp.c into in separate directory |
| 28 | can: isotp: move common definitions to separate header file |
| 29 | can: isotp: split isotp code |
| 30 | can: isotp: rework CAN frame structure access for CAN XL |
| 31 | can: isotp: add CAN XL protocol extension support |
| 32 | can: isotp: add official support for 1-to-n multicast mode |

- **Patch 27** verschiebt `net/can/isotp.c` nach `net/can/isotp/isotp.c`
  mit eigenem Makefile.
- **Patch 28** verschiebt Defines, `enum` und Strukturen nach
  `isotp_defines.h`.
- **Patch 29** teilt den Code in `isotp.c` (Socket-API), `protocolrx.c`,
  `protocoltx.c` und `hrtimer.c` mit `isotp_protocol.h` und
  `isotp_hrtimer.h` auf.
- **Patch 30** greift über Datenzeiger und Datenlänge statt über
  `struct canfd_frame` auf die CAN-Frames zu und führt Hilfsfunktionen für
  Frame-Header und PCI ein.
- **Patch 31** ergänzt die Socket-Option `CAN_ISOTP_XL_OPTS`, die
  CAN-XL-Frame-Header, die SF-Längenkodierung für CAN XL und die Kapselung
  der CiA SDTs 6/7/9.
- **Patch 32** führt den N_PCI-Typ Multicast First Frame (MF) mit
  `CAN_ISOTP_MF_BROADCAST` ein.

Die Patches setzen auf Patch 26 auf. Anpassungen an den 7.4-Code und die
Patches 1 bis 26:

- Patch 29 übernimmt die Funktionen unverändert aus dem aktuellen
  `isotp.c`, also mit `conf_lock` (Patch 4), der CF-Längenprüfung und dem
  Empfangs-Timeout aus Patch 6, dem späten Lesen von `ae` und
  `wait_tx_done` in `isotp_sendmsg()` (Patch 7), der memcg-Abrechnung
  (Patch 11), der Prüfung von `CAN_ISOTP_OPTS` vor dem Kopieren
  (Patch 12), `frame_txtime` in den Timeouts (Patch 13), den Fixes für
  den Empfangspfad (Patch 14 bis 18) und den
  7.4-Änderungen (`can_skb_uid` als Echo-Kennung,
  `READ_ONCE(so->bound)`, Prüfung des CC/FD-Frame-Inhalts in
  `isotp_rcv()`). `conf_lock` liegt seit Patch 28 mit in
  `struct isotp_sock` in `isotp_defines.h`.
- Ab Patch 30 steht die Prüfung des CC/FD-Frame-Inhalts
  (`can_is_can_skb()` bzw. `can_is_canfd_skb()`) in
  `isotp_check_frame_head()`; CAN-XL-Frames prüft dort
  `can_is_canxl_skb()`. Patch 30 und 31 lesen in `isotp_sendmsg()` keine
  Socket-Optionen vor der Belegung der Übertragung.
- Patch 31 enthält die Prüfung getunnelter CC/FD-N_PDUs (SDT 6/7) in
  `isotp_check_frame_head()` (bisher Patch 35 auf `split`) und stellt die
  FF_DLmin-Prüfung aus Patch 16 auf `SF_PCI_SZ11` um.
- `CAN_ISOTP_XL_OPTS` (Patch 31) folgt dem Schema der 7.4-Socket-Optionen:
  `isotp_setsockopt()` kopiert den Wert vor `conf_lock` mit
  Längenprüfung, `isotp_setsockopt_locked()` prüft ihn über einen Zeiger,
  und `isotp_getsockopt()` liest ihn unter `conf_lock` in eine lokale
  Kopie (Patch 10).
- Hilfsfunktionen stehen in der `.c`-Datei, die sie nutzt. Die
  tx_result/tx_gen-Funktionen (`isotp_inc_tx_gen()`,
  `isotp_pack_tx_result()`, `isotp_get_tx_gen()`, `isotp_get_tx_err()`,
  `isotp_get_tx_result()`, `isotp_tx_gen_done()`) sind `static` in
  `protocoltx.c`. `isotp_set_tx_result()`, das auch `protocolrx.c` und
  `hrtimer.c` aufrufen, ist dort eine normale Funktion mit Prototyp in
  `isotp_protocol.h`.
- In `isotp_protocol.h` bleiben nur die Einzeiler `isotp_sk()`,
  `isotp_bc_flags()`, `fd_pdu()` (Patch 30) und `xl_encap()` (Patch 31),
  die mehrere `.c`-Dateien nutzen. Sie sind `static inline`, da eine
  `static`-Funktion im Header in jeder `.c`-Datei, die sie nicht nutzt,
  `-Wunused-function` auslöst (mit `CONFIG_WERROR` ein Build-Fehler). In
  `.c`-Dateien gibt es kein `inline`.
- Patch 32 ergänzt `CAN_ISOTP_MF_BROADCAST` nur in `ISOTP_ALL_BC_FLAGS`;
  die Prüfung auf ein einziges Broadcast-Flag aus Patch 12 deckt damit
  alle drei Flags ab. Der ursprüngliche Zählblock mit `bcfl_num` in
  `isotp_setsockopt_locked()` entfällt.

## Basis

Die Patches wenden auf `main` dieses Repositorys an (CAN-Code aus
Mainline `20eef29b7c84`, 7.4-rc3+). Die Mails der v7 nennen als Basis
net/main `4d67cf2fd235`; in `can_mutex` sind die Patches 1 bis 11 gegen
Mainline `067c5c24ce` (10.11.2026) geprüft, die betroffenen Dateien sind
dort identisch.

## Prüfung

- Die Dateien 1 bis 7 und 9 bis 32 wenden in dieser Reihenfolge per `git am` auf
  `main` an.
- `net/can/` übersetzt nach jedem der Patches 1 bis 7 und 9 bis 11 fehler- und
  warnungsfrei (gcc 13.3, `W=1`, mit `CONFIG_PROVE_LOCKING`,
  `CONFIG_DEBUG_MUTEXES`, `CONFIG_PROC_FS` und `CONFIG_MEMCG`). Kein vollständiger
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

- Patch 9 und 10 sind nur übersetzt. Als Funktionstest bietet sich
  `sockopt_test` aus der Test-Suite an, das alle CAN_RAW- und
  CAN_ISOTP-Optionen setzt und zurückliest.
- Patch 11 ist nur übersetzt; die memcg-Abrechnung selbst ist ungetestet.
- Der Code nach Patch 32 ist bis auf einen Kommentar in `isotp_rcv_ff()`
  (Patch 30) identisch mit dem nach Patch 35 auf `split` (bisherige
  Reihenfolge).
- Der Kernel übersetzt nach jedem der Patches 1 bis 7 und 9 bis 32
  fehler- und warnungsfrei (21.11.2026, `git am` auf Mainline
  `c44104f3642d`, vollständiger Build mit gcc und ccache, ohne `W=1`).
  `net/can/isotp.o` bzw. ab Patch 29 die Dateien in `net/can/isotp/`
  werden in jedem Schritt neu übersetzt.
- Patch 12 bis 32 sind in dieser Reihenfolge nicht zur Laufzeit getestet.
  Die Fixes 14 bis 18 und die SDT-6/7-Prüfung aus Patch 31 sind in der
  bisherigen Reihenfolge (`split`) mit can-tests `functest/isotp` (Branch
  `isotp_ft`, Kernel `7.4.0-rc3-xlisotp-00837`) ohne DEVIATION geprüft.
- checkpatch meldet nichts außer den bei mir unbekannten Commit-IDs in
  den `Fixes:`-Zeilen bzw. Commit-Referenzen, den MAINTAINERS-Hinweisen
  bei den Dateiverschiebungen in Patch 27 bis 29 und (mit `--strict`)
  einer Leerzeile nach `{` in `struct can_isotp_xl_options` in Patch 31
  (Stand der bisherigen Reihenfolge).
