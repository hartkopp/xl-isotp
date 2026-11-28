# CAN-Patches für can-next mit CAN_ISOTP-Aufteilung und CAN XL

Die Patches wenden per `git am` auf den CAN-Code dieses Repositorys an
(`main`, "Add the CAN related documentation" auf "Update to Linux 7.4-rc4+
CAN code", Mainline `9437ee8b6cf6`). Die Patches 1 bis 7 der Serie v7
(RTNL-Stalls, CAN_ISOTP- und netlink-Fixes) sind über den net-Tree in
Mainline angekommen und hier nicht mehr enthalten.

Die Patches 1 bis 22 entsprechen `75_net_next_v3` (Stand `75_can_next`
nach dem Sashiko-Review der als "[PATCH can-next 00/22]" verschickten
Serie). Die Patches 23 bis 28 teilen CAN_ISOTP auf und ergänzen CAN XL.

## Patches 1 bis 3: Rest der bisherigen Serie

| # | Patch | bisher (`75_net_next_v2`) |
|---|---|---|
| 1 | can: raw: read socket options under conf_lock in raw_getsockopt() | 9 |
| 2 | can: isotp: read socket options under conf_lock in isotp_getsockopt() | 10 |
| 3 | can: enable memcg accounting for user triggered allocations | 11 |

- **Patch 1 und 2** lesen die Optionswerte unter `conf_lock` in eine lokale
  Variable und kopieren sie erst nach `mutex_unlock()` in den User-Space.
- **Patch 3** rechnet die von unprivilegierten Benutzern auslösbaren
  CAN-Allokationen der memcg des Prozesses an. Die Versionshistorie der
  früheren Serie ist entfernt.

## Patch 4: Fix mit Fixes-Tag

| # | Patch | bisher |
|---|---|---|
| 4 | can: isotp: do not send FC.OVFLW in listen mode | 21 |

Ein Socket im Listen-Mode sendete bei einem FF über `max_pdu_size` einen
FC.OVFLW und brach damit die Übertragung zwischen anderen Knoten ab. Das
betrifft eine korrekte Konfiguration, daher `Fixes: e057dd3fc20f` und
`Cc: stable`. Der Patch wendet auch auf net bzw. Mainline an.

## Patches 5 bis 14: Socket-Optionen, Timing und Empfangspfad (ohne Fixes-Tag)

| # | Patch | bisher |
|---|---|---|
| 5 | can: isotp: check the CAN_ISOTP_OPTS values before applying them | 12 |
| 6 | can: isotp: add the frame transmission time to the CF broadcast gap | neu |
| 7 | can: isotp: add the frame transmission time to the protocol timeouts | 13 |
| 8 | can: isotp: check the CAN FD tx_flags in CAN_ISOTP_LL_OPTS | 14 |
| 9 | can: isotp: reject reserved STmin values in CAN_ISOTP_RECV_FC | 15 |
| 10 | can: isotp: ignore CAN_ISOTP_CHK_PAD_DATA without CAN_ISOTP_RX_PADDING | 16 (umgekehrt) |
| 11 | can: isotp: drop frames without N_PCI | 17 |
| 12 | can: isotp: drop FF frames with invalid length extension | 18 |
| 13 | can: isotp: check the minimum FF_DL | 19 |
| 14 | can: isotp: check the data length of the last CF | 20 |

- **Patch 5** prüft `CAN_ISOTP_OPTS` vor dem Kopieren nach `so->opt`; ein
  abgelehnter Aufruf lässt alle Optionen unverändert.
- **Patch 6** addiert `frame_txtime` (N_As) im CF-Broadcast-Modus auch mit
  `CAN_ISOTP_FORCE_TXSTMIN` auf den CF-Abstand (N_As + N_Cs wie mit Flow
  Control) und korrigiert in `iso15765-2.rst` die Einheit der Werte von
  `CAN_ISOTP_TX_STMIN`/`CAN_ISOTP_RX_STMIN` (Nanosekunden).
- **Patch 7** addiert `frame_txtime` auf die Timeouts nach dem Senden eines
  CAN-Frames und begrenzt den Wert auf den N_As/N_Ar-Timeout von 1 s aus
  ISO 15765-2 (`-EINVAL`, dokumentiert in `isotp.h` und `iso15765-2.rst`).
  Größere CF-Abstände gehen über `CAN_ISOTP_TX_STMIN` mit
  `CAN_ISOTP_FORCE_TXSTMIN`.
- **Patch 8** lässt bei CAN FD nur `CANFD_BRS` und `CANFD_FDF` in
  `tx_flags` zu (bewusste Verschärfung der uAPI) und setzt `CANFD_FDF` für
  CAN FD immer, sodass `getsockopt()` die Flags der erzeugten Frames
  liefert. Dokumentiert in `isotp.h` und `iso15765-2.rst`.
- **Patch 9** lehnt reservierte STmin-Werte in `CAN_ISOTP_RECV_FC` ab.
- **Patch 10** ignoriert `CAN_ISOTP_CHK_PAD_DATA` ohne
  `CAN_ISOTP_RX_PADDING` beim Empfang wie dokumentiert. Bisher verwarf
  `check_pad()` in diesem Fall (ohne `CHK_PAD_LEN`) jeden Frame. Die
  frühere Ablehnung in `setsockopt()` entfällt, da `CHK_PAD_LEN |
  CHK_PAD_DATA` ohne `RX_PADDING` funktioniert und z. B. von `isotprecv
  -P a` ohne RX-Padding genutzt wird.
- **Patch 11 bis 14** legen fest, was ein gültiges N_PDU ist: kein Lesen
  jenseits von CAN_DL, keine FC-Antwort auf ungültige FFs (Längenerweiterung,
  FF_DLmin), korrekte Nutzlast des letzten CF.

Keiner der Patches 5 bis 14 hat einen Fixes-Tag: sie betreffen nur
ungültige Konfigurationen oder Frames bzw. verschärfen die uAPI. Die
Commit-Messages begründen das jeweils.

Der bisherige Patch 22 ("ignore invalid SF and FF frames during a
reception") ist entfallen: er änderte nur die Reaktion auf fremde
ungültige Frames, die ohnehin jeder Knoten mit gültigen Frames stören kann.

## Patches 15 bis 22: Aufräum-Patches

| # | Patch | bisher |
|---|---|---|
| 15 | can: isotp: drop inline keyword, let the compiler decide | 23 |
| 16 | can: isotp: rename rx_lock as it has become state machine lock | 24 |
| 17 | can: isotp: introduce isotp_invalid_canid() to verify proper CAN-IDs | 25 |
| 18 | can: isotp: rename variable for the required content length | 26 |
| 19 | can: isotp: rename checkpad functions returning bool values | 27 |
| 20 | can: isotp: change return type of functions to void | 28 |
| 21 | can: isotp: rename pointer in isotp_rcv() prototype | 29 |
| 22 | can: isotp: introduce CAN_ISOTP_MIN_TX_DL | 30 |

- **Patch 16** aktualisiert zusätzlich den Locking-Kommentar in
  `isotp_rcv()`.
- **Patch 20** entfernt das abschließende `return` in den void-Funktionen.
- **Patch 22** nutzt `CAN_ISOTP_MIN_TX_DL` nur für Prüfungen von TX_DL- bzw.
  LL_DL-Werten; die CAN_DL eines Frames wird weiter mit `CAN_MAX_DLEN`
  geprüft. Das Define steht in `isotp.h` in einem eigenen Block vor den
  Defaults.

## Patches 23 bis 28: CAN_ISOTP-Aufteilung und CAN XL

| # | Patch | bisher (`split_v2`) |
|---|---|---|
| 23 | can: isotp: move isotp.c into in separate directory | 31 |
| 24 | can: isotp: move common definitions to separate header file | 32 |
| 25 | can: isotp: split isotp code | 33 |
| 26 | can: isotp: rework CAN frame structure access for CAN XL | 34 |
| 27 | can: isotp: add CAN XL protocol extension support | 35 |
| 28 | can: isotp: add official support for 1-to-n multicast mode | 36 |

- **Patch 23** verschiebt `net/can/isotp.c` nach `net/can/isotp/isotp.c`
  mit eigenem Makefile.
- **Patch 24** verschiebt Defines, `enum` und Strukturen nach
  `isotp_defines.h`.
- **Patch 25** teilt den Code in `isotp.c` (Socket-API), `protocolrx.c`,
  `protocoltx.c` und `hrtimer.c` mit `isotp_protocol.h` und
  `isotp_hrtimer.h` auf.
- **Patch 26** greift über Datenzeiger und Datenlänge statt über
  `struct canfd_frame` auf die CAN-Frames zu und führt Hilfsfunktionen für
  Frame-Header und PCI ein.
- **Patch 27** ergänzt die Socket-Option `CAN_ISOTP_XL_OPTS`, die
  CAN-XL-Frame-Header, die SF-Längenkodierung für CAN XL und die Kapselung
  der CiA SDTs 6/7/9.
- **Patch 28** führt den N_PCI-Typ Multicast First Frame (MF) mit
  `CAN_ISOTP_MF_BROADCAST` ein.

Gegenüber `split_v2` sind die Änderungen der Patches 1 bis 22 übernommen:

- Patch 25 übernimmt die Funktionen aus dem `isotp.c` nach Patch 22, also
  mit 1-s-Grenze für `frame_txtime`, `frame_txtime` im CF-Broadcast-Abstand,
  `CANFD_FDF` in den CAN FD `tx_flags`, ignoriertem `CHK_PAD_DATA` ohne
  `RX_PADDING`, dem Locking-Kommentar in `isotp_rcv()` und ohne die
  Behandlung ungültiger SF/FF während eines Empfangs (SF/FF beenden einen
  laufenden Empfang wieder vor ihren Prüfungen, wie in Mainline).
- Patch 26 bis 28 sind entsprechend angepasst. Patch 28 setzt den
  Broadcast-Abstand auch für `CAN_ISOTP_MF_BROADCAST` aus `frame_txtime`
  plus `CAN_ISOTP_TX_STMIN` (mit `CAN_ISOTP_FORCE_TXSTMIN`) zusammen.
- `CAN_ISOTP_MIN_TX_DL` steht auch in Patch 26 und 27 nur bei Prüfungen von
  TX_DL- bzw. LL_DL-Werten (z. B. TX_DL 8 für SDT 6, Mindestlänge des FF,
  FF_DLmin). Prüfungen der CAN_DL eines Frames (Pflicht-Padding bei CAN FD,
  LL_DL aus einem FD-FF, gepolstertes FC) nutzen `CAN_MAX_DLEN`.
- `CANFD_FDF` in `ll.tx_flags` gelangt nur in CAN-FD-Frames; für SDT 7
  wird nur `CANFD_BRS` ausgewertet.

Die Festlegungen für CAN XL gelten unverändert:

- Die N_PDU-Arten im XL-Modus unterscheiden sich nur über das FD-Flag
  (`ll.mtu`) und das LL_DL: CC-N_PDU (`CAN_MTU`, TX_DL = 8, optionales
  Padding), FD-N_PDU (`CANFD_MTU`, FD-TX_DL, Padding wie bei CAN FD, auch
  mit SDT 9) und XL-N_PDU (`CAN_MTU`, TX_DL > 8, kein Padding).
  `TX_PADDING` mit einer XL-N_PDU lehnt `isotp_bind()` ab.
- Beim Empfang dekodiert eine FD-N_PDU einen SF mit CAN_DL > 8 nur mit dem
  FD-Escape (N_PCI `0x00`), eine XL-N_PDU nur mit dem XL-SF-Flag
  (`0x08 | SF_DL >> 8`). FD-N_PDUs müssen auch mit SDT 9 eine gültige
  FD-Länge haben.
- Die Padding-Prüfungen beim Empfang folgen auch bei XL-N_PDUs den
  CC/FD-Regeln; die optimierte Länge (`CHK_PAD_LEN` ohne `RX_PADDING`) ist
  für XL-N_PDUs nicht definiert. Der Empfang prüft nur, ob die Daten im
  Frame enthalten sind, nicht die genaue Frame-Länge.
- Der wirksame TX_DL wird in `isotp_bind()` gesetzt (XL-Modus:
  `CAN_ISOTP_XL_OPTS`, sonst `CAN_ISOTP_LL_OPTS`); im XL-Modus prüft
  `isotp_bind()` die MTU gegen `CANXL_HDR_SIZE` + TX_DL (mindestens
  `CANXL_MIN_MTU`).
- `struct can_isotp_xl_options` hat drei reservierte Bytes (`__res[3]`),
  die 0 sein müssen.

## Prüfung

- Die Patches 1 bis 28 wenden per `git am` auf `main` an; checkpatch meldet
  nur die unbekannte Commit-ID im Fixes-Tag von Patch 4 und die
  MAINTAINERS-Hinweise bei den Dateiverschiebungen in Patch 23 bis 25.
- Noch nicht übersetzt und nicht zur Laufzeit getestet. Die Funktionstests
  in can-tests `functest/isotp` (Branch `isotp_ft`) sind an die Änderungen
  angepasst.
- Die neuen Socket-Optionen (`CAN_ISOTP_XL_OPTS`, `CAN_ISOTP_MF_BROADCAST`)
  sind in `iso15765-2.rst` noch nicht beschrieben.
