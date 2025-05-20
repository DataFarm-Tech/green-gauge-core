# DataFarm LoRa Message Protocol

## CN001 Request
Length: 49
| Byte Num | Purpose                   | Length |
| ---------| --------------------------| -------|
|0         |  msg_id                   | 1      |
|1-6       |  des ID                   | 6      |
|7-12      |  src ID                   | 6      |
|13        |  num_nodes                | 1      |
|14        |  ttl                      | 1      |
|15-46     |  sha256                   | 32     |
|47-48     |  CRC                      | 2      |


## Successful SN001 Response
Length: 56
| Byte Num | Purpose                   | Length |
| ---------| --------------------------| -------|
|0         |  msg_id                   | 1      |
|1-6       |  des ID                   | 6      |
|7-12      |  src ID                   | 6      |
|13-19     |  payload                  | 7      |
|20        |  ttl                      | 1      |
|21-52     |  sha256                   | 32     |
|53        |  battery_level            | 1      |
|54-55     |  CRC                      | 2      |


## Failure SN001 Response
Length: 50
| Byte Num | Purpose                   | Length |
| ---------| --------------------------|--------|
|0         |  msg_id                   | 1      |
|1-6       |  des ID                   | 6      |
|7-12      |  src ID                   | 6      |
|13        |  err_code                 | 1      |
|14        |  ttl                      | 1      |
|15-46     |  sha256                   | 32     |
|47        |  battery_level            | 1      |   
|48-49     |  CRC                      | 2      |