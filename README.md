# protobuf_parser

需搭配proto2json使用，在不使用protoc编译中间文件的情况下反序列化字节序列。

# Using

使用时，项目结构如下所示
```
- directory
  - jsoncpp.dll
  - Parser.dll
  - packet_serialization.json
  - ucn_serialization.json
```

# python调用示例

```python
from ctypes import cdll, c_int, c_char_p
import json

dll = cdll.LoadLibrary(".\\Parser.dll")

dll.parser.restype = c_char_p

byte_str = b'J\x05H\xca\xe6\xb1\x03'
result:bytes = dll.parser(c_char_p(byte_str), c_int(len(byte_str)), c_char_p("117".encode("ascii")))
index = result.rfind(b"}")

result = result[:index+1].decode()
dict_result = json.loads(result)
```
