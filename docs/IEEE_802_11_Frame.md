# 802.11 frame types

- In the IEEE 802.11 Wireless LAN protocols (such as WiFi), a MAC frame is constructed of common fields (which are present in all types of frames) and specific fields (present in certain cases, depending on the type and sub-type specified in the first octet of the frame).

```text
|<--2-->|<---2-->|<--6-->|<--6-->|<--6-->|<-0--2->|<--6-->|<-0-2->|<-0-4->|<0-2312>|<-4->|
| Frame |Duration|Address|Address|Address|Sequence|Address|  QoS  |   HT  | Frame  | FCS |
|Control|  /ID   |   1   |   2   |   3   |Control |   4   |Control|Control|  Body  |     |
```

- the very first two octets transmitted by a station are the **Frame Control**. The first three subfields within the frame control and the last field (FCS) are always present in all types of 802.11 frames. These three subfields consists of two bits Protocol Version subfield, two bits Type subfield, and four bits Subtype subfield.

## 1. Frame Control

- 802.11 Frame Control Field:

```text
|<-B0-B1>|<B2-B3>|<B4-B7>|<-B8>|<-B9-->|<--B10-->|<B11>|<B12>|<B13>|<--B14-->|<-B15->|
|Protocol| Type  |Subtype|To DS|From DS|   More  |Retry|Power|More |Protected| +HTC/ |
| version|       |       |     |       |Fragments|     |Mgmt |Data |  Frame  | Order |
```

- The first three fields (Protocol version, type and subtype) in the Frame Control are always present.

### 1.1 Protocol version subfield

- The two bit **protocol version** subfield is set to 0 for WLAN (PV0) and 1 for IEEE 802.11ah (PV1).

### 1.2. Types and subtypes

- Various 802.11 frame types and subtypes

|Type Value|   Type    |Subtype value|Subtype Description           |
|(bits 3-2)|Description| (bits 7-4)  |                              |
|----------|-----------|-------------|------------------------------|
|    00    |Management |    0000     |Association Request           |
|    00    |Management |    0001     |Association Response          |
|   ...    |...        |    ...      |...                           |
|    00    |Management |    1000     |Beacon                        |
|   ...    |...        |    ...      |...                           |
|    00    |Management |    1011     |Authentication                |
|   ...    |...        |    ...      |...                           |
|    10    |Data       |    0000     |Data                          |
|   ...    |...        |    ...      |...                           |
