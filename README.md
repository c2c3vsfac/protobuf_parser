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
import os

current_path = os.path.dirname(os.path.abspath(__file__))
dll = cdll.LoadLibrary(current_path + "/Parser.dll")

dll.parser.restype = c_char_p

byte_str = b'J\x05H\xca\xe6\xb1\x03'
result:bytes = dll.parser(c_char_p(byte_str), c_int(len(byte_str)), c_char_p("117".encode("ascii")))
str_result = result.decode()
dict_result = json.loads(str_result)
# dll.free_memory(c_char_p(result))
```

